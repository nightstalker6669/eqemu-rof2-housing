#!/usr/bin/env python3
"""Apply this production housing overlay to the exact clean upstream base."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

BASE = '8175ae61874cbf20d7b05102821f6f8a4f4d1c39'
ROOT = Path(__file__).resolve().parents[1]

def git(target, *args):
    return subprocess.run(['git', '-C', str(target), *args], check=True, capture_output=True, text=True).stdout.strip()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('checkout', type=Path)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument('--check', action='store_true')
    modes.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    target = args.checkout.resolve()
    if Path(git(target, 'rev-parse', '--show-toplevel')).resolve() != target:
        raise SystemExit('The target must be the root of its own EQEmu Git checkout.')
    if git(target, 'rev-parse', 'HEAD') != BASE:
        raise SystemExit('Checkout must be at the documented exact upstream commit.')
    if git(target, 'status', '--porcelain', '--untracked-files=all'):
        raise SystemExit('Refusing a modified checkout. Start from a separate clean checkout.')
    manifest = json.loads((ROOT / 'source-manifest.json').read_text())
    for item in manifest['files']:
        path = ROOT / item['path']
        if hashlib.sha256(path.read_bytes()).hexdigest() != item['sha256']:
            raise SystemExit('Export checksum mismatch: ' + item['path'])
    patch = ROOT / 'patches/eqemu-housing.patch'
    git(target, 'apply', '--check', str(patch))
    files = sorted(p for p in (ROOT / 'overlay').rglob('*') if p.is_file())
    for src in files:
        dest = target / src.relative_to(ROOT / 'overlay')
        if dest.exists():
            raise SystemExit('Overlay path already exists: ' + str(dest.relative_to(target)))
    if args.check:
        print(f'Compatible clean upstream checkout; {len(files)} production overlay files ready.')
        return
    git(target, 'apply', str(patch))
    for src in files:
        dest = target / src.relative_to(ROOT / 'overlay')
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dest)
    print('Housing source installed. No schema, configuration, database, build or running service was changed.')
    print('Review docs/INSTALL.md before building and provisioning content.')

if __name__ == '__main__':
    main()
