#!/usr/bin/env python3
"""Install the complete production housing overlay on an existing EQEmu server.

Default is a read-only plan. --apply takes private backups, applies the explicit
migration manifest, builds/deploys production binaries and authenticates original
client resources. No passwords, test characters or synthetic property are created.
"""
import argparse
import contextlib
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import uuid
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(Path(__file__).resolve().parent))
import housing_schema_contract as schema_contract
CREATED_JOURNAL = None
IDENT = r'`?([A-Za-z_][A-Za-z_0-9]*)`?'
BASE_ENGINES = ('account','character_data','character_currency','inventory','character_evolving_items',
                'items','items_evolving_details','guilds','guild_members','instance_list','instance_list_player',
                'tributes','tribute_levels','character_tribute','guild_tributes','npc_types','spawn2',
                'spawnentry','spawngroup','zone','doors','merchantlist','merchantlist_temp','sharedbank','guild_bank','character_parcels','character_parcels_containers','variables','rule_values','rule_sets')

def require(ok, why):
    if not ok:
        raise ValueError(why)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def run(args, cwd=None, log=None):
    args = [str(x) for x in args]
    if log:
        with open(log, 'ab') as out:
            r = subprocess.run(args, cwd=cwd, stdout=out, stderr=subprocess.STDOUT)
        require(r.returncode == 0, 'Command failed; private log: ' + str(log))
    else:
        subprocess.run(args, cwd=cwd, check=True)

def pieces(text, delimiter=','):
    result, current, depth, quote, escaped = [], [], 0, '', False
    for c in text:
        if quote:
            current.append(c)
            if escaped: escaped = False
            elif c == '\\': escaped = True
            elif c == quote: quote = ''
        elif c in "'\"`": quote = c; current.append(c)
        elif c == '(': depth += 1; current.append(c)
        elif c == ')': depth -= 1; current.append(c)
        elif c == delimiter and not depth: result.append(''.join(current).strip()); current = []
        else: current.append(c)
    if current: result.append(''.join(current).strip())
    return result

def statements(raw):
    # SQL-client DELIMITER is retained as explicit statement framing for procedures.
    delimiter, buffer, result = ';', '', []
    for line in raw.splitlines(True):
        if line.lstrip().startswith('--'): continue
        m = re.match(r'\s*DELIMITER\s+(\S+)\s*$', line, re.I)
        if m:
            require(not buffer.strip(), 'Unfinished SQL before DELIMITER')
            delimiter = m.group(1); continue
        buffer += line
        # Shipped migrations terminate statements at line boundaries; never split
        # a procedure's inner semicolons or a quoted semicolon.
        if buffer.rstrip().endswith(delimiter):
            body = buffer.rstrip()[:-len(delimiter)].strip()
            if body: result.append(body)
            buffer = ''
    require(not buffer.strip(), 'Unterminated migration SQL')
    return result

def normalized_type(spec):
    m = re.match(r'\s*([A-Za-z]+)(\s*\([^)]*\))?(\s+UNSIGNED)?', spec, re.I)
    require(m, 'Unrecognized column type: ' + spec[:70])
    kind = m.group(1).lower().replace('integer', 'int').replace('boolean', 'tinyint')
    width = re.sub(r'\s+', '', m.group(2) or '').lower()
    if kind in ('tinyint','smallint','mediumint','int','bigint'): width = ''
    return kind + width + (' unsigned' if m.group(3) else '')

def column(part):
    m = re.match(r'^'+IDENT+r'\s+(.+)$', part, re.I|re.S)
    if not m or m.group(1).upper() in ('CONSTRAINT','PRIMARY','UNIQUE','KEY','INDEX','FOREIGN','CHECK'): return None
    spec = m.group(2)
    expr = re.search(r'\bAS\s*\((.*)\)\s*(?:STORED|VIRTUAL)', spec, re.I|re.S)
    return m.group(1), dict(type=normalized_type(spec), nullable=not bool(re.search(r'NOT\s+NULL|PRIMARY\s+KEY', spec, re.I)),
                           expression=re.sub(r'[\s`()]','',expr.group(1)).lower() if expr else '')

def contracts(migrations):
    columns, constraints = {}, {}
    for _, raw in migrations:
        for sql in statements(raw):
            create = re.match(r'CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?'+IDENT+r'\s*\((.*)\)\s*ENGINE', sql, re.I|re.S)
            alter = re.match(r'ALTER\s+TABLE\s+(?:IF\s+EXISTS\s+)?'+IDENT+r'\s+(.+)', sql, re.I|re.S)
            if create:
                table = create.group(1)
                for part in pieces(create.group(2)):
                    c = column(part)
                    if c: columns.setdefault(table,{})[c[0]] = c[1]
                    constraint = re.match(r'CONSTRAINT\s+'+IDENT+r'\s+(.+)', part,re.I|re.S)
                    if constraint: constraints[(table,constraint.group(1))] = re.sub(r'\s+',' ',part).strip()
            like=re.match(r'CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?'+IDENT+r'\s+LIKE\s+'+IDENT+r'$',sql,re.I)
            if like and like.group(2) in columns:columns[like.group(1)]={k:dict(v) for k,v in columns[like.group(2)].items()}
            if alter:
                table = alter.group(1)
                for part in pieces(alter.group(2)):
                    add = re.match(r'(?:ADD\s+(?:COLUMN\s+)?(?:IF\s+NOT\s+EXISTS\s+)?|MODIFY\s+(?:COLUMN\s+)?)'+IDENT+r'\s+(.+)', part,re.I|re.S)
                    if add and add.group(1).upper() not in ('CONSTRAINT','INDEX','UNIQUE','PRIMARY','KEY','FOREIGN'):
                        c=column(add.group(1)+' '+add.group(2))
                        if c: columns.setdefault(table,{})[c[0]]=c[1]
                    constraint = re.match(r'ADD\s+CONSTRAINT\s+(?:IF\s+NOT\s+EXISTS\s+)?'+IDENT+r'\s+(.+)',part,re.I|re.S)
                    if constraint: constraints[(table,constraint.group(1))] = 'CONSTRAINT '+constraint.group(1)+' '+re.sub(r'\s+',' ',constraint.group(2)).strip()
    return columns,constraints

class Database:
    def __init__(self, server):
        config=json.loads((server/'eqemu_config.json').read_text())['server']
        self.config=config;self.settings=config['database']; self.name=self.settings['db']
        content=config.get('content_database', self.settings)
        require(all(str(content.get(k,''))==str(self.settings.get(k,'')) for k in ('host','port','db')),
                'This installer requires the configured item/content and housing database to be the same database.')
        require(re.fullmatch(r'[A-Za-z0-9_]+',self.name),'Unsafe database identifier')
        self.client=shutil.which('mariadb') or shutil.which('mysql')
        self.dump=shutil.which('mariadb-dump') or shutil.which('mysqldump')
        require(self.client and self.dump,'Install MariaDB client and dump utilities first.')
        self.temp=tempfile.TemporaryDirectory(prefix='housing-db-')
        self.option=Path(self.temp.name)/'client.cnf'
        def escape(s): return '"'+str(s).replace('\\','\\\\').replace('"','\\"').replace('\n','\\n')+'"'
        self.option.write_text('[client]\n'+''.join(k+'='+escape(self.settings[v])+'\n' for k,v in
                               (('host','host'),('port','port'),('user','username'),('password','password'))))
        self.option.chmod(0o600)
        self.errors=tempfile.TemporaryFile(mode='w+t')
        self.process=subprocess.Popen([self.client,'--defaults-extra-file='+str(self.option),'--batch','--raw','--skip-column-names','--unbuffered',self.name],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=self.errors,text=True,bufsize=1)

    def query(self, sql):
        marker='HOUSING_END_'+uuid.uuid4().hex
        require(self.process.poll() is None,'Database client connection closed')
        self.process.stdin.write(sql.rstrip() + (';' if not sql.rstrip().endswith(';') and 'DELIMITER ' not in sql else '')+'\nSELECT '+q(marker)+';\n');self.process.stdin.flush()
        rows=[]
        while True:
            line=self.process.stdout.readline()
            if not line:
                self.errors.seek(0);raise ValueError('Database operation refused: '+self.errors.read()[-1500:])
            line=line.rstrip('\n')
            if line==marker:return rows
            rows.append(line.split('\t'))
    def scalar(self, sql):
        r=self.query(sql);require(len(r)==1 and len(r[0])==1,'Expected one database scalar');return r[0][0]
    def exists(self, table):
        return self.scalar("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='"+table+"'")=='1'
    def columns(self, table):
        rows=self.query("SELECT column_name,column_type,is_nullable,COALESCE(generation_expression,'') FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='"+table+"'")
        return {r[0]:dict(type=normalized_type(r[1]),nullable=r[2]=='YES',expression=re.sub(r'[\s`()]','',r[3]).lower()) for r in rows}
    def backup(self, path):
        with path.open('wb') as out:
            r=subprocess.run([self.dump,'--defaults-extra-file='+str(self.option),'--single-transaction','--routines','--triggers','--hex-blob',self.name],stdout=out,stderr=subprocess.PIPE)
        path.chmod(0o600);require(r.returncode==0 and path.stat().st_size>100,'Database backup failed; no migration applied.')
    def close(self):
        if self.process.poll() is None:
            self.process.stdin.close();self.process.wait(timeout=10)
        self.errors.close();self.temp.cleanup()

def q(value): return "'"+str(value).replace('\\','\\\\').replace("'","''")+"'"

def adapt(sql, db, final_constraints):
    create=re.match(r'(CREATE\s+TABLE\s+)(?:IF\s+NOT\s+EXISTS\s+)?'+IDENT+r'\b',sql,re.I)
    if create:
        if db.exists(create.group(2)): return []
        return [sql]
    alter=re.match(r'ALTER\s+TABLE\s+(?:IF\s+EXISTS\s+)?'+IDENT+r'\s+(.+)',sql,re.I|re.S)
    if not alter: return [sql]
    table,parts=alter.group(1),pieces(alter.group(2)); require(db.exists(table),'Missing prerequisite table: '+table)
    actual=db.columns(table);out=[];dropped=set()
    for part in parts:
        add=re.match(r'ADD\s+(?:COLUMN\s+)?(?:IF\s+NOT\s+EXISTS\s+)?'+IDENT+r'\s+(.+)',part,re.I|re.S)
        constraint=re.match(r'ADD\s+CONSTRAINT\s+(?:IF\s+NOT\s+EXISTS\s+)?'+IDENT+r'\s+(.+)',part,re.I|re.S)
        drop=re.match(r'DROP\s+(?:FOREIGN\s+KEY|CONSTRAINT|INDEX)\s+(?:IF\s+EXISTS\s+)?'+IDENT+r'\s*$',part,re.I)
        index=re.match(r'ADD\s+(?:UNIQUE\s+)?(?:INDEX|KEY)\s+(?:IF\s+NOT\s+EXISTS\s+)?'+IDENT+r'\b',part,re.I)
        if constraint:
            canonical='CONSTRAINT '+constraint.group(1)+' '+re.sub(r'\s+',' ',constraint.group(2)).strip()
            if canonical!=final_constraints.get((table,constraint.group(1))): continue
            count=db.scalar("SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name="+q(table)+" AND constraint_name="+q(constraint.group(1)))
            if count!='0' and constraint.group(1) not in dropped:
                kind='FOREIGN KEY' if re.search(r'\bFOREIGN\s+KEY',constraint.group(2),re.I) else 'CONSTRAINT'
                out.append('ALTER TABLE `'+table+'` DROP '+kind+' `'+constraint.group(1)+'`')
            part='ADD '+canonical
        elif drop:
            if re.search(r'INDEX',part,re.I):
                count=db.scalar("SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name="+q(table)+" AND index_name="+q(drop.group(1)))
            else: count=db.scalar("SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name="+q(table)+" AND constraint_name="+q(drop.group(1)))
            if count=='0': continue
            dropped.add(drop.group(1))
            part=re.sub(r'\s+IF\s+EXISTS','',part,flags=re.I)
        elif index:
            if db.scalar("SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name="+q(table)+" AND index_name="+q(index.group(1)))!='0' and index.group(1) not in dropped: continue
            part=re.sub(r'\s+IF\s+NOT\s+EXISTS','',part,flags=re.I)
        elif add and add.group(1).upper() not in ('CONSTRAINT','INDEX','UNIQUE','PRIMARY','KEY','FOREIGN'):
            if add.group(1) in actual: continue
            part=re.sub(r'\s+IF\s+NOT\s+EXISTS','',part,flags=re.I)
        elif re.match(r'MODIFY\b',part,re.I):
            c=column(re.sub(r'^MODIFY\s+(?:COLUMN\s+)?','',part,flags=re.I))
            if c and actual.get(c[0])==c[1]: continue
        out.append('ALTER TABLE `'+table+'` '+part)
    return out

def frame(sql):
    return 'DELIMITER $$\n'+sql+'\n$$\nDELIMITER ;\n' if re.match(r'CREATE\s+PROCEDURE',sql,re.I) else sql+';\n'

def stopped():
    for item in Path('/proc').iterdir():
        if not item.name.isdigit(): continue
        try: name=(item/'comm').read_text().strip()
        except OSError: continue
        require(name not in ('world','zone'),'Stop world and every zone before applying housing.')

def metadata_seed(sql,db):
    m=re.match(r'INSERT\s+INTO\s+'+IDENT+r'\s*\((.*?)\)\s*VALUES\s*\((.*)\)\s*$',sql,re.I|re.S)
    if not m:return sql
    table=m.group(1);cols=[c.strip().strip('`') for c in pieces(m.group(2))];vals=pieces(m.group(3));require(len(cols)==len(vals),'Unsupported seed value framing')
    primary=[r[0] for r in db.query("SELECT column_name FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name="+q(table)+" AND index_name='PRIMARY' ORDER BY seq_in_index")]
    if table=='tribute_levels':primary=['tribute_id','level']
    require(primary and all(k in cols for k in primary),'Seed requires a known complete natural key: '+table)
    where=' AND '.join('`'+c+'` <=> '+vals[cols.index(c)] for c in primary)
    count=int(db.scalar('SELECT COUNT(*) FROM `'+table+'` WHERE '+where))
    if not count:return sql
    exact=' AND '.join('BINARY `'+c+'` <=> BINARY '+v for c,v in zip(cols,vals))
    require(count==1 and db.scalar('SELECT COUNT(*) FROM `'+table+'` WHERE '+where+' AND '+exact)=='1','Existing supported seed conflicts: '+table)
    return ''

def migrate(db, manifest, backup):
    migrations=[]
    for entry in manifest['migrations']:
        path=ROOT/entry['path'];require(path.is_file() and sha(path)==entry['sha256'],'Migration source hash mismatch: '+entry['path']);migrations.append((entry,path.read_text()))
    expected,final_constraints=contracts(migrations)
    db.query('CREATE TABLE IF NOT EXISTS housing_install_migrations (migration_key VARCHAR(192) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,source_sha256 CHAR(64) NOT NULL,installed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB;')
    for entry,raw in migrations:
        key=entry['path'];present=db.query('SELECT source_sha256 FROM housing_install_migrations WHERE migration_key='+q(key))
        if present:
            require(present==[[entry['sha256']]],'Previously installed migration changed: '+key);continue
        transformed=[]
        for sql in statements(raw):
            if key.endswith('/trophy_supported_catalogue.sql'):sql=metadata_seed(sql,db)
            if sql:
                for statement in adapt(sql,db,final_constraints):
                    transformed.append(statement)
                    db.query(frame(statement))
        (backup/('migration-'+Path(key).name)).write_text(''.join(frame(s) for s in transformed))
        db.query('INSERT INTO housing_install_migrations(migration_key,source_sha256) VALUES('+q(key)+','+q(entry['sha256'])+');')
        print('Installed/adopted '+Path(key).name,flush=True)
    for table,fields in expected.items():
        require(db.exists(table),'Final schema missing table '+table)
        actual=db.columns(table)
        for name,spec in fields.items():
            require(actual.get(name)==spec,'Final schema conflict: '+table+'.'+name+'; inspect private backup/migration logs')
    schema_contract.verify(db,migrations,statements,pieces)
    schema_contract.verify_native(db)
    return len(migrations)

def record_created(target):
    if CREATED_JOURNAL is None:return
    rows={row['path']:row for row in json.loads(CREATED_JOURNAL.read_text())} if CREATED_JOURNAL.exists() else {}
    path=str(target.absolute())
    rows[path]=dict(path=path,kind='symlink',link=os.readlink(target)) if target.is_symlink() else dict(path=path,kind='file',sha256=sha(target))
    CREATED_JOURNAL.write_text(json.dumps(list(rows.values()),indent=2)+'\n')

def private_files(roots):
    result=set()
    for root in roots:
        if not root.exists():continue
        if root.is_file() or root.is_symlink():result.add(root.absolute());continue
        for directory,dirs,files in os.walk(root,followlinks=False):
            dirs[:]=[d for d in dirs if not (Path(directory)/d).is_symlink()]
            result.update((Path(directory)/name).absolute() for name in files)
    return result

def tracked_run(args,roots,**kwargs):
    before=private_files(roots)
    try:run(args,**kwargs)
    finally:
        for item in private_files(roots)-before:record_created(item)

def copy_verified(source,target):
    require(source.is_file() and not source.is_symlink(),'Deployment source is not regular: '+str(source))
    if target.exists():
        require(target.is_file() and not target.is_symlink(),'Deployment target is not regular: '+str(target))
        if sha(source)==sha(target):return
    missing=not target.exists()
    target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    if missing and CREATED_JOURNAL is not None:
        record_created(target)

def deploy_verified(source,target,allowed):
    resolved=target.resolve(strict=False)
    require(resolved.is_relative_to(allowed.resolve()),'Deployment symlink leaves its configured directory: '+str(target))
    copy_verified(source,resolved)

def deployment_paths(a,config):
    dirs=config.get('directories',{})
    def path(value):
        p=Path(value);return (p if p.is_absolute() else a.server/p).resolve()
    if a.bin_dir:a.bin_dir=a.bin_dir.resolve()
    elif (a.server/'world').exists():a.bin_dir=(a.server/'world').resolve().parent
    elif (a.server/'bin/world').exists():a.bin_dir=(a.server/'bin').resolve()
    else:raise ValueError('Cannot identify deployed server binaries; supply --bin-dir')
    a.opcode_dir=(a.opcode_dir.resolve() if a.opcode_dir else path(dirs.get('patches','./')))
    require((a.opcode_dir/'patch_RoF2.conf').is_file(),'Active RoF2 mapping missing; supply --opcode-dir matching server directories.patches')
    a.quests_dir=(a.quests_dir.resolve() if a.quests_dir else path(dirs.get('quests','quests/')))
    a.maps_dir=(a.maps_dir.resolve() if a.maps_dir else path(dirs.get('maps','maps/')))
    require((a.maps_dir/'base').is_dir(),'Configured collision-map base directory missing; supply --maps-dir')
    a.shared_dir=path(dirs.get('shared_memory','shared/'))
    a.logical_links=[a.server/name for name in ('world','zone','ucs','shared_memory','maps')]
    for key,default in [('patches','./'),('quests','quests/'),('shared_memory','shared/')]:
        value=Path(dirs.get(key,default));a.logical_links.append(value if value.is_absolute() else a.server/value)
    alias=a.server/'maps'
    if alias.exists():require(alias.resolve()==a.maps_dir,'The runtime maps/base alias disagrees with configured maps; resolve the paths before installation.')

def append_logical_links(a,receipt):
    for link in a.logical_links:
        if link.is_symlink() and not any(row['requested']==str(link) for row in receipt):
            receipt.append(dict(requested=str(link),resolved=str(link.resolve(strict=True)),archive=None,symlink=os.readlink(link)))

def backup_deployment(a):
    targets=[a.server/'eqemu_config.json',a.opcode_dir/'patch_RoF2.conf',a.quests_dir/'guildhall/player.lua',a.shared_dir,a.server/'housing-test-assets',a.server/'housing-original-meshes']
    targets += [a.bin_dir/x for x in ('world','zone','ucs','shared_memory','housing_install','housing_capacity','housing_recovery','housing_profile_upgrade')]
    targets += [x for x in a.server.iterdir() if x.is_file() and x.suffix in ('.json','.conf')]
    receipt=[];seen=set()
    with tarfile.open(a.backup/'server-files.tar.gz','w:gz') as archive:
        for target in targets:
            if not target.exists():continue
            resolved=target.resolve(strict=True)
            if str(resolved) in seen:continue
            seen.add(str(resolved));name='resolved/'+str(len(receipt))+'/'+resolved.name
            archive.add(resolved,arcname=name,recursive=True)
            receipt.append(dict(requested=str(target),resolved=str(resolved),archive=name,symlink=os.readlink(target) if target.is_symlink() else None))
    append_logical_links(a,receipt)
    (a.backup/'deployment-paths.json').write_text(json.dumps(receipt,indent=2)+'\n')

def main():
    global CREATED_JOURNAL
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',required=True,type=Path);p.add_argument('--server',required=True,type=Path)
    p.add_argument('--client',required=True,type=Path);p.add_argument('--build',required=True,type=Path)
    p.add_argument('--backup',required=True,type=Path);p.add_argument('--jobs',type=int,default=2)
    p.add_argument('--key',default='sunrise-housing-v1');p.add_argument('--name',default='Sunrise Hills')
    p.add_argument('--bin-dir',type=Path);p.add_argument('--opcode-dir',type=Path);p.add_argument('--quests-dir',type=Path);p.add_argument('--maps-dir',type=Path)
    p.add_argument('--apply',action='store_true');p.add_argument('--stop-command',help='JSON argv array; executed only with --apply')
    p.add_argument('--start-command',help='JSON argv array; executed only after all verification passes')
    p.add_argument('--schema-only',action='store_true',help=argparse.SUPPRESS)
    a=p.parse_args();a.source=a.source.resolve();a.server=a.server.resolve();a.client=a.client.resolve();a.build=a.build.resolve();a.backup=a.backup.resolve()
    for command in (a.stop_command,a.start_command):
        if command is not None:
            argv=json.loads(command);require(isinstance(argv,list) and argv and all(isinstance(v,str) and v for v in argv),'Service command must be a nonempty JSON string array')
    require(1<=a.jobs<=8,'Build jobs must be1..8');require(a.server.is_dir() and a.source.is_dir() and a.client.is_dir(),'Source/server/client directory missing')
    manifest=json.loads((ROOT/'installer-migrations.json').read_text())
    for entry in json.loads((ROOT/'source-manifest.json').read_text())['files']:
        require((ROOT/entry['path']).is_file() and sha(ROOT/entry['path'])==entry['sha256'],'Production package checksum mismatch: '+entry['path'])
    require(shutil.which('git') and shutil.which('cmake'),'Git and CMake are required')
    subprocess.run(['git','-C',str(a.source),'cat-file','-e','8175ae61874cbf20d7b05102821f6f8a4f4d1c39^{commit}'],check=True,capture_output=True)
    db=Database(a.server)
    try:
        deployment_paths(a,db.config)
        print('Housing plan: '+str(len(manifest['migrations']))+' ordered migrations; authenticated original geometry; full house/guild profiles;75 priced plots; native purchases/rent; service NPCs; trophy profile; extended item custody. No fake owners.')
        for table in set(manifest['required_native_tables'])|{'rule_values','rule_sets','variables','instance_list_player'}:
            require(db.exists(table),'Existing EQEmu prerequisite missing: '+table)
        schema_contract.verify_native(db)
        require(db.scalar('SELECT COUNT(*) FROM spells_new')!='0','The existing EQEmu spell catalogue is empty')
        crate=db.query('SELECT idfile,UNK224,UNK227,UNK228,UNK229 FROM items WHERE id=62586')
        require(len(crate)==1 and crate[0][:3]==['IT20028','142','174'] and all(int(v)>0 for v in crate[0][3:]),'Original moving-crate item 62586 metadata is missing/conflicting; restore its reviewed original item data before installation')
        require(db.scalar('SELECT DATABASE()')==db.name,'Configured database identity mismatch')
        require('MariaDB' in db.scalar('SELECT VERSION()'),'The pinned housing migrations require MariaDB; MySQL variants are not silently rewritten.')
        require(int(db.scalar('SELECT @@max_allowed_packet'))>=67108864,'MariaDB max_allowed_packet must be at least 64MiB for complete bag receipts. A database administrator must run SET GLOBAL max_allowed_packet=67108864 and persist max_allowed_packet=64M in the MariaDB server configuration, then open a new installer connection.')
        if not a.apply:
            print('Read-only plan complete. --apply requires stopped writers, takes backups, and enables the documented reconstructed defaults.');return
        if a.stop_command:run(json.loads(a.stop_command))
        stopped()
        for table,predicate in [('housing_inventory_sessions','active=1'),('housing_live_item_leases','active=1 OR pending_key IS NOT NULL')]:
            if db.exists(table):require(db.scalar('SELECT COUNT(*) FROM '+table+' WHERE '+predicate)=='0','Active inventory state blocks installation: '+table)
        require(db.scalar("SELECT GET_LOCK('eqemu-production-housing-installer',0)")=='1','Another housing installer holds the database lock')
        a.backup.mkdir(parents=True,exist_ok=False);a.backup.chmod(0o700)
        db.backup(a.backup/'database.sql')
        backup_deployment(a)
        (a.backup/'backup-info.json').write_text(json.dumps(dict(database=db.name,server=str(a.server),database_sha256=sha(a.backup/'database.sql'),files_sha256=sha(a.backup/'server-files.tar.gz'),paths_sha256=sha(a.backup/'deployment-paths.json'),tables=[r[0] for r in db.query("SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE'")]),indent=2)+'\n')
        CREATED_JOURNAL=a.backup/'created-files.json'
        if not (a.server/'maps').exists():
            (a.server/'maps').symlink_to(a.maps_dir,target_is_directory=True);record_created(a.server/'maps')

        for table in set(BASE_ENGINES)|set(manifest['required_native_tables'])|set(schema_contract.NATIVE_COLUMNS):
            rows=db.query('SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='+q(table))
            if not rows:continue
            require(rows[0][0] in ('InnoDB','MyISAM'),'Unsupported engine for required table '+table)
            if rows[0][0]=='MyISAM':db.query('ALTER TABLE `'+table+'` ENGINE=InnoDB;')
        for table in set(BASE_ENGINES)|set(manifest['required_native_tables'])|set(schema_contract.NATIVE_COLUMNS):
            require(db.scalar('SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='+q(table))=='InnoDB','Required base table is not transactional: '+table)
        count=migrate(db,manifest,a.backup)
        db.query("INSERT INTO housing_runtime_activation(id,database_name,client_build,policy_revision,allow_reconstructed_content,review_reference) VALUES(1,DATABASE(),20130510,1,1,'Production housing installer; labelled reconstructed content/defaults') ON DUPLICATE KEY UPDATE database_name=VALUES(database_name),client_build=VALUES(client_build),policy_revision=VALUES(policy_revision),allow_reconstructed_content=1,review_reference=VALUES(review_reference);")
        if a.schema_only:
            print('SCHEMA_INSTALL_PASS '+str(count)+' migrations; services remain stopped.');return
        package_hash=sha(ROOT/'source-manifest.json')[:16]
        original_source=a.source
        a.build.mkdir(parents=True,exist_ok=True)
        managed=a.build/('housing-source-'+package_hash)
        build_dir=a.build/('housing-build-'+package_hash)
        if not managed.exists():
            run(['git','clone','--no-hardlinks','--no-checkout',original_source,managed],log=a.backup/'source-clone.log')
            run(['git','-C',managed,'checkout','--detach','8175ae61874cbf20d7b05102821f6f8a4f4d1c39'],log=a.backup/'source-clone.log')
            run(['git','-C',managed,'submodule','update','--init','--recursive'],log=a.backup/'source-submodules.log')
            run([sys.executable,ROOT/'tools/apply_overlay.py',managed,'--apply'],log=a.backup/'source-install.log')
            changed=subprocess.run(['git','-C',str(managed),'diff','--name-only'],check=True,capture_output=True,text=True).stdout.splitlines()
            files=set(changed)|{str(x.relative_to(ROOT/'overlay')) for x in (ROOT/'overlay').rglob('*') if x.is_file()}
            (managed/'.housing-source-receipt.json').write_text(json.dumps({f:sha(managed/f) for f in sorted(files)},indent=2)+'\n')
        else:
            receipt=json.loads((managed/'.housing-source-receipt.json').read_text())
            require(all((managed/f).is_file() and sha(managed/f)==digest for f,digest in receipt.items()),'Managed source was modified; preserve/review it before reinstalling.')
        a.source=managed;a.build=build_dir
        run(['cmake','-S',a.source,'-B',a.build,'-DEQEMU_BUILD_SERVER=ON','-DEQEMU_BUILD_TESTS=OFF','-DEQEMU_BUILD_LOGIN=OFF','-DEQEMU_BUILD_PCH=OFF'],log=a.backup/'configure.log')
        targets=['world','zone','ucs','shared_memory','housing_install','housing_capacity','housing_recovery','housing_profile_upgrade']
        run(['cmake','--build',a.build,'--parallel',str(a.jobs),'--target',*targets],log=a.backup/'build.log')
        for binary in targets:deploy_verified(a.build/'bin'/binary,a.bin_dir/binary,a.bin_dir)
        # Operator-owned archives and meshes stay private to the installation.
        tracked_run([sys.executable,ROOT/'tools/install_original_geometry.py','--client',a.client,'--server',a.server,'--overlay',a.source],roots=[a.server/'housing-original-meshes'],log=a.backup/'geometry.log')
        generated=a.backup/'generated-content'
        args=[sys.executable,ROOT/'tools/generate_house_variant_content.py','--client',a.client,'--maps',a.maps_dir/'base','--output',generated,'--source-root',a.source,'--copy-resources']
        for name in ('housing-personal-content-registry.json','housing-guild-content-registry.json','.house-personal-content-registry-test.json','.house-guild-content-registry-test.json'):
            if (a.server/name).is_file():args+=['--preserve-registry',a.server/name]
        for name in ('.house-personal-test.json','.house-guild-test.json'):
            if (a.server/name).is_file():args+=['--preserve-manifest',a.server/name]
        run(args,log=a.backup/'content-generation.log')
        # Diagnostic preservation lists change between first install and retry;
        # they are not part of the immutable catalogue's content identity.
        proposal=generated/'housing-catalogue-proposal.json'
        catalogue=json.loads(proposal.read_text());catalogue['excluded_items']=[]
        proposal.write_text(json.dumps(catalogue,sort_keys=True,indent=2)+'\n')
        for item in generated.rglob('*'):
            if not item.is_file() or item.name.startswith('housing-catalogue-') or item.name.endswith('generation-summary.json'):continue
            relative=item.relative_to(generated);target=a.server/relative
            if 'content-registry.json' in item.name and target.exists():
                old=json.loads(target.read_text());new=json.loads(item.read_text());existing={json.dumps(e,sort_keys=True) for e in old['entries']}
                for entry in new['entries']:
                    same=[e for e in old['entries'] if e['manifest']==entry['manifest']]
                    require(not same or all(e==entry for e in same),'Conflicting existing house template registration')
                    if json.dumps(entry,sort_keys=True) not in existing:
                        old['entries'].append(entry);existing.add(json.dumps(entry,sort_keys=True))
                item.write_text(json.dumps(old,indent=2)+'\n')
            if relative.parts[:2]==('maps','base') and target.exists():
                require(sha(item)==sha(target),'Existing collision map differs from authenticated original: '+str(target))
            copy_verified(item,target)
        archive=next((x for x in a.client.iterdir() if x.name.lower()=='furniture06.eqg'),None);require(archive,'Original furniture06 archive missing')
        copy_verified(archive,a.server/'housing-test-assets/furniture06.eqg')
        # Merge only documented housing opcode symbols into the actual RoF2 file.
        opcodes=a.opcode_dir/'patch_RoF2.conf'
        require(opcodes.is_file(),'Runtime patch_RoF2.conf is missing')
        mappings={m['name']:m['opcode'] for m in json.loads((ROOT/'protocol/opcodes.json').read_text())['messages'] if isinstance(m.get('opcode'),str) and re.fullmatch(r'0x[0-9A-Fa-f]{4}',m['opcode']) and int(m['opcode'],16)}
        lines=opcodes.read_text().splitlines();found=set();merged=[]
        values={int(value,16) for value in mappings.values()}
        for line in lines:
            existing=re.match(r'\s*(OP_\w+)\s*=\s*(0x[0-9a-fA-F]+)',line)
            if existing and existing.group(1) not in mappings:
                require(int(existing.group(2),16) not in values,'Housing opcode collides with existing non-housing symbol: '+existing.group(1))
        for line in lines:
            m=re.match(r'\s*(OP_\w+)\s*=\s*0x[0-9a-fA-F]+',line)
            if m and m.group(1) in mappings:
                require(m.group(1) not in found,'Duplicate housing opcode symbol in runtime configuration')
                found.add(m.group(1));line=m.group(1)+'='+mappings[m.group(1)]
            merged.append(line)
        merged += [name+'='+value for name,value in mappings.items() if name not in found]
        op_target=opcodes.resolve();require(op_target.is_relative_to(a.opcode_dir),'Opcode symlink leaves configured opcode directory')
        op_target.write_text('\n'.join(merged)+'\n')
        quest_contract=json.loads((ROOT/'quest-overlay-manifest.json').read_text())
        for item in (ROOT/'quest-overlay').rglob('*'):
            if item.is_file():
                relative=str(item.relative_to(ROOT/'quest-overlay'));target=a.quests_dir/relative
                if target.exists() and sha(target)!=sha(item):
                    require(sha(target) in quest_contract['files'][relative]['accepted_base_sha256'],'Custom quest differs from reviewed base: '+relative+'; merge the housing patch before installation.')
                deploy_verified(item,target,a.quests_dir)
        # Select the active prefix AFTER shared_memory, which can clear hotfix_.
        prefix=db.scalar("SELECT COALESCE((SELECT value FROM variables WHERE varname='hotfix_name'),'')")
        require(re.fullmatch(r'[A-Za-z0-9_]*',prefix),'Unsupported shared memory prefix')
        tracked_run([a.bin_dir/'shared_memory','-hotfix='+prefix] if prefix else [a.bin_dir/'shared_memory'],roots=[a.shared_dir],cwd=a.server,log=a.backup/'shared-memory.log')
        db.query("UPDATE variables SET value="+q(prefix)+" WHERE varname='hotfix_name';")
        policy=a.source/'common/housing/house_variant_capacity_defaults.json';plan=a.backup/'capacity-plan.json'
        run([a.bin_dir/'housing_capacity','--plan',policy,plan],cwd=a.server,log=a.backup/'capacity.log')
        run([a.bin_dir/'housing_capacity','--dry-run',policy,plan],cwd=a.server,log=a.backup/'capacity.log')
        run([a.bin_dir/'housing_capacity','--apply',policy,plan],cwd=a.server,log=a.backup/'capacity.log')
        before=set(a.server.iterdir())
        try:run([a.bin_dir/'housing_install','--apply',proposal,a.key,a.name,a.backup/'content-result.json','--reconstructed-defaults'],cwd=a.server,log=a.backup/'content-install.log')
        finally:
            for item in set(a.server.iterdir())-before:
                if item.is_file():record_created(item)
        ruleset_name=db.scalar("SELECT COALESCE((SELECT value FROM variables WHERE varname='RuleSet'),'default')")
        sets=db.query('SELECT ruleset_id FROM rule_sets WHERE name='+q(ruleset_name))
        require(len(sets)==1,'Cannot identify the configured active rule set')
        ruleset=int(sets[0][0])
        for rule in ('EnablePlayerHousing','EnableNativeFurniturePlacement','EnableNativeEscrowTransfers','EnableNativeHouseSwap'):
            db.query("INSERT INTO rule_values(ruleset_id,rule_name,rule_value,notes) VALUES("+str(ruleset)+","+q('Housing:'+rule)+",'true','Production housing installer') ON DUPLICATE KEY UPDATE rule_value='true';")
        (a.backup/'installation.json').write_text(json.dumps(dict(database=db.name,migrations=count,database_backup_sha256=sha(a.backup/'database.sql'),state='installed-awaiting-native-check'),indent=2)+'\n')
        if a.start_command:run(json.loads(a.start_command))
        print('HOUSING_INSTALL_PASS. All production features installed; no synthetic players/properties. Private rollback files: '+str(a.backup))
        print('After startup, verify one real personal/guild workflow with the matching RoF2 client; installation is not a claim of universal Live equivalence.')
    finally:db.close()

if __name__=='__main__':
    try:main()
    except (OSError,ValueError,subprocess.CalledProcessError) as e:raise SystemExit('Housing installation stopped: '+str(e)+'\nServices are not automatically restarted after a failed step; use the private backup and logs.')
