#ifndef EQEMU_HOUSING_OFFLINE_PROCESS_GUARD_H
#define EQEMU_HOUSING_OFFLINE_PROCESS_GUARD_H
#include <stdexcept>
#include <string>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>
#elif defined(__linux__)
#include <filesystem>
#include <fstream>
#include <system_error>
#endif
namespace Housing::OfflineProcessGuard {
namespace Detail {
template<class C>bool WriterName(std::basic_string<C> name){
 const auto slash=name.find_last_of(std::basic_string<C>{C('/'),C('\\')});if(slash!=name.npos)name.erase(0,slash+1);
 for(auto& c:name)if(c>=C('A')&&c<=C('Z'))c=C(c-C('A')+C('a'));
 const auto text=[](const char* s){std::basic_string<C> out;while(*s)out+=C(*s++);return out;};
 const auto deleted=text(" (deleted)");if(name.size()>=deleted.size()&&name.compare(name.size()-deleted.size(),deleted.size(),deleted)==0)name.resize(name.size()-deleted.size());
 return name==text("world")||name==text("zone")||name==text("world.exe")||name==text("zone.exe");
}
inline void RejectWriter(){throw std::runtime_error("Stop world and every zone before housing maintenance");}
inline void Unavailable(){throw std::runtime_error("Process enumeration unavailable; refusing housing maintenance");}
}
// A read-only point-in-time check. The operator must keep writers stopped for
// the complete maintenance operation; this neither terminates nor locks them.
inline void RequireWorldAndZoneStopped(){
#if defined(_WIN32)
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
 if(snapshot==INVALID_HANDLE_VALUE)Detail::Unavailable();
 struct Close {HANDLE value;~Close(){CloseHandle(value);}} close{snapshot};
 PROCESSENTRY32W entry{};entry.dwSize=static_cast<DWORD>(sizeof(entry));
 if(!Process32FirstW(snapshot,&entry))Detail::Unavailable();
 do{if(Detail::WriterName(std::wstring(entry.szExeFile)))Detail::RejectWriter();}while(Process32NextW(snapshot,&entry));
 if(GetLastError()!=ERROR_NO_MORE_FILES)Detail::Unavailable();
#elif defined(__linux__)
 const std::filesystem::path root("/proc");std::error_code error;
 if(!std::filesystem::is_directory(root/"self",error)||error)Detail::Unavailable();
 std::filesystem::directory_iterator current(root,error),end;if(error)Detail::Unavailable();
 while(current!=end){
  const auto path=current->path();const auto name=path.filename().string();
  if(!name.empty()&&name.find_first_not_of("0123456789")==std::string::npos){
   std::ifstream file(path/"comm");std::string executable;
   if(!file||!std::getline(file,executable)||executable.empty()){
    // Exit between directory enumeration and opening comm is ordinary. A live
    // but unreadable process cannot establish that every writer is stopped.
    error.clear();const bool present=std::filesystem::exists(path,error);
    if(error||present)Detail::Unavailable();
   }else if(Detail::WriterName(executable))Detail::RejectWriter();
  }
  current.increment(error);if(error)Detail::Unavailable();
 }
#else
 Detail::Unavailable();
#endif
}
}
#endif
