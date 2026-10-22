#!/usr/bin/env python3
"""Install private original-client housing geometry from the operator's own client.

Python 3.9+ standard library only. Catalogs contain hashes, never client meshes.
This command writes only SERVER/housing-original-meshes and temporary staging
inside SERVER; it neither runs services nor reads/writes a game database.
"""
import argparse
import hashlib
import math
import os
from pathlib import Path
import re
import shutil
import struct
import sys
import tempfile
import zlib
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from housing_platform import WINDOWS, read_utf8, private_directory, link_info

MAX_ARCHIVE = 512 * 1024 * 1024
MAX_MEMBER = 64 * 1024 * 1024
MAX_FILES = 20000
MAX_SCAN = 64 * 1024 * 1024 * 1024
CATALOGS = (
    'original_terrain_source.h', 'original_zon_personal_door_catalog.h',
    'original_zon_authored_actor_catalog.h', 'original_mesh_catalog.h',
    'selected_wall_cache_data.h', 'original_furniture_source_catalog.h',
)
SHA = r'[a-f0-9]{64}'
ROW = re.compile(r'\{\s*\d+\s*,\s*"([^"\\]+)"\s*,\s*"(' + SHA + r')"\s*,\s*"(' + SHA + r')"\s*,\s*"(' + SHA + r')"\s*\}')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest_file(path, maximum=MAX_ARCHIVE):
    require(path.is_file() and not link_info(path), f'Not a regular source file: {path.name}')
    require(0 < path.stat().st_size <= maximum, f'File outside size limit: {path.name}')
    value = hashlib.sha256()
    with path.open('rb') as stream:
        while True:
            chunk = stream.read(1024 * 1024)
            if not chunk:
                break
            value.update(chunk)
    return value.hexdigest()


class Reader:
    def __init__(self, raw):
        self.raw, self.at = raw, 0

    def take(self, count):
        require(0 <= count <= len(self.raw) - self.at, 'Truncated original format')
        result = self.raw[self.at:self.at + count]
        self.at += count
        return result

    def unpack(self, fmt):
        return struct.unpack('<' + fmt, self.take(struct.calcsize('<' + fmt)))


def crc_name(name):
    value = 0
    for byte in name.encode('ascii') + b'\0':
        value ^= byte << 24
        for _ in range(8):
            value = ((value << 1) ^ (0x04c11db7 if value & 0x80000000 else 0)) & 0xffffffff
    return value


class Archive:
    def __init__(self, raw):
        self.raw = raw
        reader = Reader(raw)
        offset, magic = reader.unpack('I4s')
        require(magic == b'PFS ' and 8 <= offset <= len(raw) - 4, 'Unsupported PFS header')
        reader.at = offset
        count, = reader.unpack('I')
        require(0 < count <= 100000 and count <= (len(raw) - reader.at) // 12, 'Invalid PFS directory')
        self.entries = [reader.unpack('III') for _ in range(count)]
        names_entries = [entry for entry in self.entries if entry[0] == 0x61580ac9]
        require(len(names_entries) == 1, 'Ambiguous PFS name dictionary')
        names = Reader(self.inflate(names_entries[0]))
        name_count, = names.unpack('I')
        require(name_count <= count, 'Invalid PFS filename count')
        self.names = {}
        for _ in range(name_count):
            length, = names.unpack('I')
            require(0 < length <= 4096, 'Invalid PFS filename length')
            text = names.take(length)
            require(text[-1:] == b'\0' and b'\0' not in text[:-1], 'Invalid PFS filename')
            name = text[:-1].decode('ascii').lower()
            require(name not in self.names, 'Ambiguous PFS filename')
            self.names[name] = crc_name(name)

    def inflate(self, entry):
        _, start, size = entry
        require(0 <= start < len(self.raw) and 0 < size <= MAX_MEMBER, 'PFS member exceeds limit')
        reader = Reader(self.raw)
        reader.at = start
        output = bytearray()
        while len(output) < size:
            packed, plain = reader.unpack('II')
            require(0 < packed <= MAX_MEMBER and 0 < plain <= size - len(output), 'Invalid PFS block sizes')
            decoder = zlib.decompressobj()
            block = decoder.decompress(reader.take(packed), plain + 1)
            require(len(block) == plain and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail,
                    'Invalid or oversized zlib block')
            output.extend(block)
        return bytes(output)

    def member(self, model, expected):
        name = model.lower()
        if not name.endswith(('.ter', '.mod')):
            name += '.mod'
        require(name in self.names, f'Original member is absent: {name}')
        entries = [entry for entry in self.entries if entry[0] == self.names[name]]
        require(len(entries) == 1, f'Ambiguous PFS member: {name}')
        raw = self.inflate(entries[0])
        require(hashlib.sha256(raw).hexdigest() == expected, f'Original member hash mismatch: {name}')
        return raw


def mesh_sidecar(raw, archive_sha, member_sha):
    """Original binary32 XYZ; polygons source A,C,B and original low16 flags."""
    reader = Reader(raw)
    magic, version, strings_size, materials, vertices, polygons = reader.unpack('4s5I')
    require(magic in (b'EQGM', b'EQGT') and version in (1, 2, 3), 'Unsupported original mesh format')
    if magic == b'EQGM':
        bones, = reader.unpack('I')
        require(bones == 0, 'Animated model is outside reviewed class')
    require(0 < vertices <= 1000000 and 0 < polygons <= 1000000 and materials <= 65535,
            'Original mesh count outside limits')
    strings = reader.take(strings_size)

    def valid_string(offset):
        require(offset < len(strings) and b'\0' in strings[offset:], 'Invalid original material string')

    for _ in range(materials):
        _, name, shader, properties = reader.unpack('4I')
        valid_string(name)
        valid_string(shader)
        require(properties <= (len(raw) - reader.at) // 12, 'Invalid material property count')
        for _ in range(properties):
            name, kind, value = reader.unpack('3I')
            valid_string(name)
            if kind == 2:
                valid_string(value)
    stride = 32 if version < 3 else 44
    require(vertices <= (len(raw) - reader.at) // stride, 'Truncated original vertices')
    result = bytearray(b'EQHMSH01' + struct.pack('<II', vertices, polygons))
    result += bytes.fromhex(archive_sha) + bytes.fromhex(member_sha)
    for _ in range(vertices):
        vertex = reader.take(stride)
        require(all(math.isfinite(value) and abs(value) <= 2e7 for value in struct.unpack_from('<3f', vertex)),
                'Nonfinite or excessive original vertex')
        result += vertex[:12]
    require(polygons <= (len(raw) - reader.at) // 20, 'Truncated original polygons')
    for _ in range(polygons):
        a, b, c, material, flags = reader.unpack('3IiI')
        require(max(a, b, c) < vertices and -1 <= material < materials, 'Invalid original polygon')
        result += struct.pack('<IIIHH', a, c, b, flags & 65535, 0)
    require(reader.at == len(raw), 'Unsupported trailing original mesh data')
    return result


def catalog(overlay):
    directory = overlay / 'common' / 'housing'
    required, zones = {}, {}
    for name in CATALOGS:
        source = directory / name
        require(source.is_file() and source.stat().st_size <= 4 * 1024 * 1024, f'Missing/oversized catalog: {name}')
        text = read_utf8(source)
        # These two reviewed tables preserve their older entries and append
        # immutable variant records through a single bounded local include.
        include = {
            'original_terrain_source.h': 'original_variant_terrain.inc',
            'original_zon_authored_actor_catalog.h': 'original_zon_variant_actors.inc',
        }.get(name)
        if include and ('#include "' + include + '"') in text:
            marker = '#include "' + include + '"'
            require(text.count(marker) == 1, 'Duplicate variant catalog include')
            part = directory / include
            require(part.is_file() and not part.is_symlink() and part.stat().st_size <= 4 * 1024 * 1024,
                    f'Missing/oversized variant catalog: {include}')
            extra = read_utf8(part)
            require('#include' not in extra, 'Nested variant catalog include refused')
            text = text.replace(marker, extra)
        rows = ROW.findall(text)
        counts = re.findall(r'std::array<[^>\n]+,\s*(\d+)>\s+\w+\s*\{\{', text)
        require(len(counts) == 1 and len(rows) == int(counts[0]) and rows, f'Unsupported/incomplete catalog schema: {name}')
        for model, sidecar, archive, member in rows:
            require('/' not in model and '\\' not in model and '..' not in model, 'Invalid compiled model name')
            record = (model, archive, member)
            if sidecar in required:
                require(required[sidecar][1:] == record[1:], 'Conflicting compiled mesh identity')
            else:
                required[sidecar] = record
        if name == 'original_terrain_source.h':
            for zone, zon_sha in re.findall(r'\{\s*\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*"([a-z0-9]+)"\s*,\s*"(' + SHA + r')"', text):
                require(zone not in zones, 'Duplicate compiled room resource')
                zones[zone] = zon_sha
            require(len(zones) == len(rows), 'Incomplete compiled ZON identities')
    return required, zones


def child(directory, name):
    matches = [path for path in directory.iterdir() if path.name.lower() == name.lower()]
    require(len(matches) == 1 and not matches[0].is_symlink(), f'Missing/ambiguous original client resource: {name}')
    return matches[0]


def client_profile(client, overlay):
    registry = overlay / 'common' / 'housing' / 'static_furniture_registry.h'
    require(registry.is_file() and registry.stat().st_size <= 4 * 1024 * 1024, 'Missing reviewed original client profile')
    text = read_utf8(registry)
    resources = child(client, 'Resources')
    require(resources.is_dir(), 'Missing original resource directory')
    for field, path in (
        ('ClientSha256', child(client, 'eqgame.exe')),
        ('RendererSha256', child(client, 'eqgraphicsdx9.dll')),
        ('ManifestSha256', child(resources, 'OnDemandResources.txt')),
        ('ActorTagSha256', child(resources, 'actortaginfo.txt')),
    ):
        matches = re.findall(r'\b' + field + r'\s*=\s*"(' + SHA + r')"', text)
        require(len(matches) == 1 and digest_file(path) == matches[0], f'Original client/profile hash mismatch: {path.name}')


def install(client, server, overlay):
    require(client.is_dir() and server.is_dir() and overlay.is_dir(), 'Client, server and overlay must be existing directories')
    require(server != client and client not in server.parents, 'Server destination must be outside client directory')
    required, zones = catalog(overlay)
    client_profile(client, overlay)
    files = [path for path in client.iterdir() if path.is_file() and path.suffix.lower() in ('.eqg', '.zon')]
    require(len(files) <= MAX_FILES, 'Client resource inventory exceeds limit')
    lookup = {}
    for path in files:
        key = path.name.lower()
        require(key not in lookup, f'Ambiguous client resource filename: {key}')
        lookup[key] = path
    for name, expected in zones.items():
        path = lookup.get(name + '.zon')
        require(path is not None and digest_file(path, MAX_MEMBER) == expected, f'Original ZON hash mismatch/missing: {name}')
    wanted = {record[1] for record in required.values()}
    archives = {}
    scanned = 0
    # Exact content hashes confer authority; filenames are search hints only.
    candidates = sorted((p for p in files if p.suffix.lower() == '.eqg'), key=lambda p: p.name.lower())
    for path in candidates:
        require(not path.is_symlink(), f'Symlinked client archive refused: {path.name}')
        scanned += path.stat().st_size
        require(scanned <= MAX_SCAN, 'Client archive scan exceeds limit')
        sha = digest_file(path)
        if sha in wanted:
            archives[sha] = path
        if wanted <= archives.keys():
            break
    require(wanted <= archives.keys(), f'Wrong/incomplete client: {len(wanted - archives.keys())} required original archives missing')
    target = server / 'housing-original-meshes'
    require(not link_info(target) and (not target.exists() or target.is_dir()), 'Asset destination is not a regular directory')
    stage = Path(tempfile.mkdtemp(prefix='.housing-original-stage-', dir=server))
    private_directory(stage)
    try:
        grouped = {}
        for sha, record in required.items():
            grouped.setdefault(record[1], []).append((sha, record))
        for archive_sha, rows in grouped.items():
            raw = archives[archive_sha].read_bytes()
            require(len(raw) <= MAX_ARCHIVE and hashlib.sha256(raw).hexdigest() == archive_sha, 'Archive changed during import')
            archive = Archive(raw)
            for sha, (model, _, member_sha) in rows:
                sidecar = mesh_sidecar(archive.member(model, member_sha), archive_sha, member_sha)
                require(hashlib.sha256(sidecar).hexdigest() == sha, f'Generated geometry hash mismatch: {model}')
                with (stage / (sha + '.mesh')).open('xb') as stream:
                    if not WINDOWS:os.fchmod(stream.fileno(), 0o600)
                    stream.write(sidecar)
                    stream.flush()
                    os.fsync(stream.fileno())
        # All bytes validate before publishing any assets. Existing corruption
        # refuses; each new SHA-named file is atomically installed without clobber.
        for sha in required:
            existing = target / (sha + '.mesh')
            if existing.exists() or existing.is_symlink():
                require(digest_file(existing, MAX_MEMBER) == sha, 'Existing private geometry differs from compiled identity')
        target.mkdir(mode=0o700, exist_ok=True)
        private_directory(target)
        for sha in required:
            existing = target / (sha + '.mesh')
            try:
                if WINDOWS:os.rename(stage / existing.name, existing)
                else:os.link(stage / existing.name, existing)
            except FileExistsError:
                require(digest_file(existing, MAX_MEMBER) == sha, 'Concurrent private geometry mismatch')
            if not WINDOWS:os.chmod(existing, 0o600, follow_symlinks=False)
        if not WINDOWS:
            descriptor = os.open(target, os.O_RDONLY | getattr(os, 'O_DIRECTORY', 0))
            try:
                os.fsync(descriptor)
            finally:
                os.close(descriptor)
    finally:
        shutil.rmtree(stage)
    print(f'Installed {len(required)} authenticated private geometry files in {target}')
    print('Run the server under the same account, or grant that account access to this private directory.')


def main():
    default = Path(__file__).resolve().parents[1]
    if (default / 'overlay').is_dir():
        default /= 'overlay'
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', required=True, type=Path, help='Original supported RoF2 client directory (read only)')
    parser.add_argument('--server', required=True, type=Path, help='Existing server working directory')
    parser.add_argument('--overlay', type=Path, default=default, help='Overlay directory containing common/housing (default: repository overlay)')
    args = parser.parse_args()
    try:
        install(args.client.resolve(), args.server.resolve(), args.overlay.resolve())
    except (OSError, ValueError, struct.error, zlib.error, UnicodeError) as error:
        parser.exit(1, f'Geometry import refused: {error}\n')


if __name__ == '__main__':
    main()
