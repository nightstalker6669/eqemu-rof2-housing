"""Platform boundaries for the production installer; no game/runtime state."""
import csv
import ctypes
import hashlib
import io
import json
import os
from pathlib import Path
import sys
import re
import shutil
import struct
import subprocess

WINDOWS = os.name == 'nt'
if WINDOWS:
    for stream in (sys.stdout,sys.stderr):
        if hasattr(stream,'reconfigure'):stream.reconfigure(encoding='utf-8',errors='backslashreplace')
TARGETS = ('world','zone','ucs','shared_memory','housing_install','housing_capacity','housing_recovery','housing_profile_upgrade')

def require(ok, message):
    if not ok:raise ValueError(message)

def read_utf8(path):return Path(path).read_bytes().decode('utf-8')
def write_utf8(path, text):return Path(path).write_bytes(text.encode('utf-8'))
def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def executable(name):return name+('.exe' if WINDOWS else '')

def package_files(root):
    manifest=json.loads(read_utf8(root/'source-manifest.json'))
    for entry in manifest['files']:
        path=root/entry['path'];require(path.is_file(),'Production package file missing: '+entry['path'])
        data=path.read_bytes()
        if hashlib.sha256(data).hexdigest()!=entry['sha256']:
            if b'\r\n' in data and hashlib.sha256(data.replace(b'\r\n',b'\n')).hexdigest()==entry['sha256']:
                raise ValueError('Git converted production package bytes to CRLF: '+entry['path']+'. Re-clone using git -c core.autocrlf=false clone <repository> <new-folder>; do not regenerate the release checksum manifest locally.')
            raise ValueError('Production package checksum mismatch: '+entry['path'])
    return manifest

def windows_text(raw):
    require(WINDOWS,'Windows command decoding is unavailable on this platform')
    cp=ctypes.windll.kernel32.GetOEMCP()
    return raw.decode('cp'+str(cp),errors='strict')

def system_tool(name):
    require(WINDOWS,'Windows system tool requested on another platform')
    kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.GetSystemDirectoryW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32];kernel.GetSystemDirectoryW.restype=ctypes.c_uint32
    buffer=ctypes.create_unicode_buffer(32768);length=kernel.GetSystemDirectoryW(buffer,len(buffer));require(0<length<len(buffer),'Cannot locate Windows System32')
    tool=Path(buffer.value)/name;require(tool.is_file(),'Required Windows system utility missing: '+name);return str(tool)

def parse_tasklist(text):
    names=[]
    for row in csv.reader(io.StringIO(text)):
        if not row:continue
        require(len(row)>=5 and row[1].strip().isdigit() and row[0].strip(),'Cannot parse complete tasklist CSV; process safety could not be established')
        names.append(row[0].strip().casefold())
    require(names,'Empty tasklist output; process safety could not be established')
    return names

def stopped():
    if WINDOWS:
        command=system_tool('tasklist.exe')
        result=subprocess.run([command,'/FO','CSV','/NH'],capture_output=True)
        require(result.returncode==0,'tasklist failed; stop verification is unavailable')
        active=set(parse_tasklist(windows_text(result.stdout)))&{executable(n).casefold() for n in TARGETS}
        require(not active,'Stop every EQEmu writer/build helper before continuing: '+', '.join(sorted(active)))
        return
    require(Path('/proc').is_dir(),'Process census is unsupported on this platform; Linux or native Windows is required')
    for entry in Path('/proc').iterdir():
        if not entry.name.isdigit():continue
        try:name=(entry/'comm').read_text(encoding='utf-8').strip()
        except (FileNotFoundError,ProcessLookupError):continue
        except OSError as error:raise ValueError('Unable to inspect process '+entry.name+'; stop verification failed') from error
        require(name not in ('world','zone'),'Stop world and every zone before applying housing.')

def link_info(path):
    path=Path(path)
    if path.is_symlink():return dict(kind='symlink',link=os.readlink(path))
    if WINDOWS:
        try:info=path.lstat()
        except FileNotFoundError:return None
        if getattr(info,'st_reparse_tag',0)==0xA0000003:return dict(kind='junction',link=os.readlink(path))
        require(not (getattr(info,'st_file_attributes',0)&0x400),'Unsupported reparse point in deployment path: '+str(path))
    return None

def windows_sid():
    kernel=ctypes.WinDLL('kernel32',use_last_error=True);security=ctypes.WinDLL('advapi32',use_last_error=True)
    kernel.GetCurrentProcess.restype=ctypes.c_void_p;kernel.CloseHandle.argtypes=[ctypes.c_void_p];kernel.LocalFree.argtypes=[ctypes.c_void_p]
    security.OpenProcessToken.argtypes=[ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(ctypes.c_void_p)]
    security.GetTokenInformation.argtypes=[ctypes.c_void_p,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)]
    security.ConvertSidToStringSidW.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_void_p)]
    token=ctypes.c_void_p();require(security.OpenProcessToken(kernel.GetCurrentProcess(),8,ctypes.byref(token)),'Cannot inspect current Windows security token')
    try:
        needed=ctypes.c_uint32();security.GetTokenInformation(token,1,None,0,ctypes.byref(needed));require(0<needed.value<65536,'Invalid Windows token size')
        data=ctypes.create_string_buffer(needed.value);require(security.GetTokenInformation(token,1,data,len(data),ctypes.byref(needed)),'Cannot read current Windows user SID')
        sid=ctypes.c_void_p.from_buffer(data).value;text=ctypes.c_void_p()
        require(security.ConvertSidToStringSidW(sid,ctypes.byref(text)),'Cannot format Windows user SID')
        try:value=ctypes.wstring_at(text)
        finally:kernel.LocalFree(text)
        require(re.fullmatch(r'S-1-[0-9-]+',value),'Invalid current Windows SID');return value
    finally:kernel.CloseHandle(token)

def private_directory(path):
    path=Path(path)
    if not WINDOWS:path.chmod(0o700);return
    acl=system_tool('icacls.exe')
    sid=windows_sid()
    result=subprocess.run([acl,str(path),'/inheritance:r','/grant:r','*'+sid+':(OI)(CI)F','*S-1-5-18:(OI)(CI)F','*S-1-5-32-544:(OI)(CI)F'],capture_output=True)
    require(result.returncode==0,'Cannot secure the private backup/credential directory ACL: '+str(path))

def build_executable(build, name, config):
    filename=executable(name)
    tiers=[]
    if config:tiers.append([build/'bin'/config/filename,build/config/filename])
    tiers.append([build/'bin'/filename,build/filename])
    for candidates in tiers:
        found=[p for p in candidates if p.is_file() and not p.is_symlink()]
        if found:
            require(len({digest(p) for p in found})==1,'Conflicting build outputs for '+filename)
            return found[0]
    raise ValueError('Built executable missing: '+filename+'; inspected bin/<config>, bin, <config> and build root')

class PE:
    """Bounded PE32/PE32+ import, delay-import and export reader (Microsoft PE format)."""
    def __init__(self,path):
        self.path=Path(path);self.raw=self.path.read_bytes();require(64<=len(self.raw)<=1024*1024*1024,'Invalid PE size: '+str(path))
        require(self.raw[:2]==b'MZ','Not a Windows PE binary: '+str(path));pe=self.u32(60)
        require(self.data(pe,4)==b'PE\0\0','Invalid PE signature: '+str(path));self.machine=self.u16(pe+4);count=self.u16(pe+6);size=self.u16(pe+20);opt=pe+24
        magic=self.u16(opt);require(magic in (0x10b,0x20b),'Unsupported PE optional header');self.width=8 if magic==0x20b else 4
        require(self.machine in (0x14c,0x8664) and ((self.machine==0x8664)==(self.width==8)),'Only matching x86/x64 native server binaries are supported')
        self.image_base=self.u64(opt+24) if self.width==8 else self.u32(opt+28);self.headers=self.u32(opt+60);directory=112 if self.width==8 else 96;n=self.u32(opt+directory-4)
        require(0<count<=96 and size>=directory and n<=32 and size>=directory+8*n,'Invalid PE header bounds')
        self.directories=[(self.u32(opt+directory+8*i),self.u32(opt+directory+8*i+4)) for i in range(n)];self.sections=[]
        for i in range(count):
            pos=opt+size+40*i;self.sections.append((self.u32(pos+12),self.u32(pos+8),self.u32(pos+20),self.u32(pos+16)))
        self.imports={};self._imports(1,False);self._imports(13,True)
    def data(self,offset,size):
        require(offset>=0 and size>=0 and offset+size<=len(self.raw),'Truncated PE structure: '+str(self.path));return self.raw[offset:offset+size]
    def u16(self,p):return struct.unpack('<H',self.data(p,2))[0]
    def u32(self,p):return struct.unpack('<I',self.data(p,4))[0]
    def u64(self,p):return struct.unpack('<Q',self.data(p,8))[0]
    def at(self,rva,size=1):
        if rva<self.headers:self.data(rva,size);return rva
        for address,virtual,raw,length in self.sections:
            if address<=rva<address+max(virtual,length):
                delta=rva-address;require(delta+size<=length,'PE structure points outside file-backed section');self.data(raw+delta,size);return raw+delta
        raise ValueError('Unmapped PE RVA in '+str(self.path))
    def string(self,rva):
        pos=self.at(rva);end=self.raw.find(b'\0',pos,min(len(self.raw),pos+4096));require(end>=0,'Unterminated PE string');return self.raw[pos:end].decode('ascii')
    def dll(self,rva):
        name=self.string(rva);require(re.fullmatch(r'[A-Za-z0-9_+.-]+\.dll',name,re.I) and '..' not in name,'Invalid imported DLL basename');return name.lower()
    def _imports(self,index,delay):
        if index>=len(self.directories):return
        address,size=self.directories[index]
        if not address and not size:return
        step=32 if delay else 20;require(address and size>=step,'Invalid PE import directory')
        for i in range(min(size//step,4096)):
            pos=self.at(address+i*step,step);values=struct.unpack('<'+'I'*(step//4),self.data(pos,step))
            if not any(values):return
            if delay:
                require(values[0] in (0,1),'Unknown delay-import address mode');base=0 if values[0] else self.image_base;name=values[1]-base;thunk=(values[4] or values[3])-base
            else:name=values[3];thunk=values[0] or values[4]
            symbols=self.imports.setdefault(self.dll(name),set());require(thunk>0,'Missing PE import lookup table')
            for j in range(65536):
                p=self.at(thunk+j*self.width,self.width);v=self.u64(p) if self.width==8 else self.u32(p)
                if not v:break
                symbols.add(v&0xffff if v&(1<<(8*self.width-1)) else self.string(v+2))
            else:raise ValueError('Unterminated PE import lookup table')
        raise ValueError('Unterminated PE import descriptor table')
    def export(self,symbol):
        require(self.directories,'PE binary has no export directory');address,size=self.directories[0];require(address and size>=40,'DLL has no exports: '+str(self.path));pos=self.at(address,40)
        base,count,names,functions,strings,ordinals=[self.u32(pos+n) for n in (16,20,24,28,32,36)];require(count<=1000000 and names<=count,'Invalid PE export counts')
        index=None
        if isinstance(symbol,int):index=symbol-base
        else:
            for n in range(names):
                if self.string(self.u32(self.at(strings+4*n,4)))==symbol:index=self.u16(self.at(ordinals+2*n,2));break
        require(index is not None and 0<=index<count,'Missing DLL export '+str(symbol)+' in '+str(self.path));rva=self.u32(self.at(functions+4*index,4));require(rva,'Missing DLL ordinal')
        return self.string(rva) if address<=rva<address+size else None

class SystemLibraries:
    def __init__(self):
        require(WINDOWS,'System DLL resolution requires native Windows')
        import winreg
        self.known=set()
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,r'SYSTEM\CurrentControlSet\Control\Session Manager\KnownDLLs') as key:
                for i in range(winreg.QueryInfoKey(key)[1]):
                    _,value,kind=winreg.EnumValue(key,i)
                    if kind in (winreg.REG_SZ,winreg.REG_EXPAND_SZ) and isinstance(value,str) and re.fullmatch(r'[A-Za-z0-9_+.-]+\.dll',value,re.I):self.known.add(value.lower())
        except OSError as error:raise ValueError('Cannot inspect Windows KnownDLLs precedence; dependency safety could not be established') from error
        self.kernel=ctypes.WinDLL('kernel32',use_last_error=True);self.handles={}
        self.kernel.LoadLibraryExW.argtypes=[ctypes.c_wchar_p,ctypes.c_void_p,ctypes.c_uint32];self.kernel.LoadLibraryExW.restype=ctypes.c_void_p
        self.kernel.GetModuleFileNameW.argtypes=[ctypes.c_void_p,ctypes.c_wchar_p,ctypes.c_uint32];self.kernel.GetModuleFileNameW.restype=ctypes.c_uint32
        self.kernel.GetProcAddress.argtypes=[ctypes.c_void_p,ctypes.c_void_p];self.kernel.GetProcAddress.restype=ctypes.c_void_p
        self.kernel.FreeLibrary.argtypes=[ctypes.c_void_p];self.kernel.FreeLibrary.restype=ctypes.c_int
        self.system_dirs=set()
        for name in ('GetSystemDirectoryW','GetSystemWow64DirectoryW'):
            getter=getattr(self.kernel,name);getter.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32];getter.restype=ctypes.c_uint32
            buffer=ctypes.create_unicode_buffer(32768);n=getter(buffer,len(buffer))
            if 0<n<len(buffer):self.system_dirs.add(os.path.normcase(str(Path(buffer.value).resolve())))
    def preferred(self,name):
        return name in self.known or (name.startswith(('api-ms-','ext-ms-')) and self.resolve(name,set()) is not None)
    def resolve(self,name,symbols):
        handle=self.handles.get(name)
        if not handle:
            handle=self.kernel.LoadLibraryExW(name,None,0x800) # LOAD_LIBRARY_SEARCH_SYSTEM32 only
            if not handle:return None
            self.handles[name]=handle
        path=ctypes.create_unicode_buffer(32768);n=self.kernel.GetModuleFileNameW(handle,path,len(path));require(0<n<len(path),'Cannot identify resolved Windows system library')
        if os.path.normcase(str(Path(path.value).resolve().parent)) not in self.system_dirs:return None
        for symbol in symbols:
            encoded=None if isinstance(symbol,int) else symbol.encode('ascii')
            pointer=ctypes.c_void_p(symbol) if isinstance(symbol,int) else ctypes.cast(ctypes.c_char_p(encoded),ctypes.c_void_p)
            require(self.kernel.GetProcAddress(handle,pointer),'Missing Windows system export '+name+'!'+str(symbol))
        return path.value
    def close(self):
        for handle in self.handles.values():self.kernel.FreeLibrary(handle)
        self.handles.clear()

def windows_cohort(executables,runtime_dirs=(),runtime_dlls=(),system=None):
    """Resolve actual import closure; non-system DLLs come only from explicit/build roots."""
    own_system=system is None;system=system or SystemLibraries();roots={p.parent.resolve() for p in executables.values()}|{Path(p).resolve() for p in runtime_dirs}|{Path(p).resolve().parent for p in runtime_dlls};candidates={}
    for root in roots:
        require(root.is_dir(),'Runtime DLL directory missing: '+str(root))
        for p in root.iterdir():
            if p.is_file() and p.suffix.lower()=='.dll':candidates.setdefault(p.name.lower(),[]).append(p)
    for p in runtime_dlls:
        p=Path(p).resolve();require(p.is_file() and p.suffix.lower()=='.dll','Explicit runtime DLL missing: '+str(p));candidates.setdefault(p.name.lower(),[]).append(p)
    parsed={name:PE(path) for name,path in executables.items()};machines={p.machine for p in parsed.values()};require(len(machines)==1,'The eight executables mix x86/x64 architectures');machine=next(iter(machines))
    if own_system:require((machine==0x8664)==(ctypes.sizeof(ctypes.c_void_p)==8),'Use a Windows Python interpreter matching the built server architecture (x64 Python for x64 server)')
    bundled={};systems={};queue=[];visited=set()
    for pe in parsed.values():queue.extend((name,symbols,()) for name,symbols in pe.imports.items())
    for p in runtime_dlls:queue.append((Path(p).name.lower(),set(),()))
    try:
        while queue:
            name,symbols,chain=queue.pop()
            require(not any((name,symbol) in chain for symbol in symbols),'Cyclic DLL forwarded export')
            key=(name,tuple(sorted(map(str,symbols))))
            if key in visited:continue
            visited.add(key)
            if name not in bundled:
                options=candidates.get(name,[])
                if not options or (hasattr(system,'preferred') and system.preferred(name)):
                    resolved=system.resolve(name,symbols)
                    require(resolved,'Unresolved DLL '+name+'; supply its matching build/dependency folder with --runtime-dir or its exact file with --runtime-dll')
                    systems[name]=resolved;continue
                require(len({digest(p) for p in options})==1,'Ambiguous DLL versions for '+name+'; supply a coherent dependency cohort')
                path=options[0];require(not path.is_symlink(),'Symlinked runtime DLL refused');pe=PE(path);require(pe.machine==machine,'Runtime DLL architecture differs: '+name);bundled[name]=(path,pe);queue.extend((dependency,imports,()) for dependency,imports in pe.imports.items())
            path,pe=bundled[name]
            for symbol in symbols:
                forward=pe.export(symbol)
                if forward:
                    library,sep,target=forward.rpartition('.');require(sep and library and target,'Invalid DLL forwarded export');library=library.lower();library=library if library.endswith('.dll') else library+'.dll'
                    require(re.fullmatch(r'[a-z0-9_+.-]+\.dll',library) and '..' not in library,'Unsafe DLL forwarder');queue.append((library,{int(target[1:]) if target.startswith('#') else target},chain+((name,symbol),)))
        return dict(machine=machine,executables={name:dict(path=str(p),sha256=digest(p),imports=sorted(parsed[name].imports)) for name,p in executables.items()},dlls={name:dict(path=str(path),sha256=digest(path),imports=sorted(pe.imports)) for name,(path,pe) in sorted(bundled.items())},system_libraries=dict(sorted(systems.items())),dynamic_loading='PE and delay imports plus explicitly supplied runtime DLLs; application-selected plugins must be supplied explicitly')
    finally:
        if own_system:system.close()
