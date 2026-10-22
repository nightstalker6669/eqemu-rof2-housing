#!/usr/bin/env python3
"""Restore one complete private housing-install backup with all writers stopped."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from install_housing import Database, require, stopped, sha, q
from housing_platform import read_utf8, write_utf8, link_info

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',required=True,type=Path);p.add_argument('--backup',required=True,type=Path)
    p.add_argument('--apply',action='store_true');a=p.parse_args();a.server=a.server.resolve();a.backup=a.backup.resolve()
    info=json.loads(read_utf8(a.backup/'backup-info.json'));require(Path(info['server']).resolve()==a.server,'Backup belongs to another server directory')
    for name,key in [('database.sql','database_sha256'),('server-files.tar.gz','files_sha256'),('deployment-paths.json','paths_sha256')]:
        require(sha(a.backup/name)==info[key],'Backup checksum mismatch: '+name)
    paths=json.loads(read_utf8(a.backup/'deployment-paths.json'));created=json.loads(read_utf8(a.backup/'created-files.json')) if (a.backup/'created-files.json').exists() else []
    # Resolve all conflicting changes before altering the database or any file.
    for row in created:
        target=Path(row['path'])
        if row.get('kind')=='symlink':
            require(link_info(target) and link_info(target)['link']==row['link'],'A newly installed link was changed: '+str(target))
        elif target.exists():require(target.is_file() and not target.is_symlink() and sha(target)==row['sha256'],'A newly installed file was subsequently changed: '+str(target))
    for row in paths:
        requested=Path(row['requested'])
        require(requested.resolve(strict=False)==Path(row['resolved']),'Deployment target was redirected after backup: '+str(requested))
        if row['symlink'] is not None:require(link_info(requested) and link_info(requested)['link']==row['symlink'],'Deployment link was changed after backup')
    with tarfile.open(a.backup/'server-files.tar.gz') as archive:
        for row in paths:
            if row['archive'] is None:continue
            prefix=row['archive'];members=[m for m in archive.getmembers() if m.name==prefix or m.name.startswith(prefix+'/')]
            require(members,'Backup target is absent from archive')
            require(all((m.isfile() or m.isdir()) and not Path(m.name).is_absolute() and '..' not in Path(m.name).parts and ':' not in m.name and '\\' not in m.name for m in members),'Unsupported linked or unsafe backup member; use manual reviewed restore')
    print('Restore plan: full database plus '+str(len(paths))+' resolved deployment targets; no automatic service start.')
    if not a.apply:return
    stopped();db=Database(a.server)
    try:
        require(db.name==info['database'],'Configured database differs from backup')
        require(db.scalar("SELECT GET_LOCK('eqemu-production-housing-installer',0)")=='1','Another installer holds the database lock')
        for table,predicate in [('housing_inventory_sessions','active=1'),('housing_live_item_leases','active=1 OR pending_key IS NOT NULL')]:
            if db.exists(table):require(db.scalar('SELECT COUNT(*) FROM '+table+' WHERE '+predicate)=='0','Active inventory state prevents restore')
        for table in ('housing_live_escrow_pending','housing_house_inventory_pending','housing_native_plot_purchase_pending'):
            if db.exists(table):require(db.scalar('SELECT COUNT(*) FROM '+table)=='0','Unresolved inventory application prevents restore: '+table)
        # A full dump recreates prior tables. Remove only additional housing
        # tables introduced by this installer; unrelated new tables are retained.
        current={r[0] for r in db.query("SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE'")}
        extra=sorted(t for t in current-set(info['tables']) if t.startswith('housing_'))
        if extra:db.query('SET FOREIGN_KEY_CHECKS=0;'+''.join('DROP TABLE `'+t+'`;' for t in extra)+'SET FOREIGN_KEY_CHECKS=1;')
        with (a.backup/'database.sql').open('rb') as source:
            result=subprocess.run([db.client,'--defaults-extra-file='+str(db.option),'--default-character-set=utf8mb4',db.name],stdin=source,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
        require(result.returncode==0,'Database restore failed: '+result.stderr.decode(errors='replace')[-1000:])
        for row in sorted(created,key=lambda r:r.get('kind')=='symlink'):
            target=Path(row['path'])
            if target.exists() or target.is_symlink():target.unlink()
        with tarfile.open(a.backup/'server-files.tar.gz') as archive:
            for row in paths:
                if row['archive'] is None:continue
                target=Path(row['resolved']);prefix=row['archive']
                for member in archive.getmembers():
                    if member.name!=prefix and not member.name.startswith(prefix+'/'):continue
                    relative=member.name[len(prefix):].lstrip('/');destination=target/relative if relative else target
                    if member.isdir():destination.mkdir(parents=True,exist_ok=True);continue
                    destination.parent.mkdir(parents=True,exist_ok=True)
                    require(not destination.is_symlink(),'Resolved restore target became a symlink: '+str(destination))
                    with archive.extractfile(member) as source, destination.open('wb') as output:shutil.copyfileobj(source,output)
                    destination.chmod(member.mode)
        print('HOUSING_RESTORE_PASS. Database, binaries, mappings, configuration and prior cache restored; keep stopped until reviewed, then start the matching server.')
    finally:db.close()

if __name__=='__main__':
    try:main()
    except (OSError,ValueError,subprocess.CalledProcessError) as e:raise SystemExit('Housing restore stopped: '+str(e))
