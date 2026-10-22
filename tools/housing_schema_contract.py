"""Read-only structural checks for the ordered housing migrations.

Existing tables are adopted only when their final keys, checks, foreign keys and
transactional engines agree with the shipped DDL. No schema repairs occur here.
"""
import copy
import re

IDENT = r'`?([A-Za-z_][A-Za-z_0-9]*)`?'

def require(ok, why):
    if not ok:
        raise ValueError('Housing schema contract: ' + why)

def quote(value):
    return "'" + str(value).replace('\\', '\\\\').replace("'", "''") + "'"

def expression(value):
    # MariaDB omits redundant parentheses around AND arms of an OR. Normalize
    # that grammar, preserving both boolean precedence and quoted literal case.
    raw = re.findall(r"'(?:[^'\\]|\\.|'')*'|\"(?:[^\"\\]|\\.|\"\")*\"|`[^`]+`|[A-Za-z_][A-Za-z_0-9]*|[0-9]+|<>|!=|<=|>=|[^\s]", value)
    tokens = [part if part.startswith(("'", '\"')) else part.strip('`').lower().replace('!=', '<>') for part in raw]
    def tree(parts):
        while len(parts) > 1 and parts[0] == '(' and parts[-1] == ')':
            depth = 0
            for index, part in enumerate(parts):
                if part == '(': depth += 1
                elif part == ')': depth -= 1
                if depth == 0: break
            if index != len(parts) - 1: break
            parts = parts[1:-1]
        for operator in ('or', 'and'):
            cuts, depth, between = [], 0, False
            for index, part in enumerate(parts):
                if part == '(': depth += 1
                elif part == ')': depth -= 1
                elif depth == 0:
                    if part == 'between': between = True
                    elif part == 'and' and between: between = False
                    elif part == operator: cuts.append(index)
            if cuts:
                children, start = [], 0
                for end in cuts + [len(parts)]:
                    child = tree(parts[start:end])
                    children.extend(child[1:] if child[0] == operator else [child])
                    start = end + 1
                return (operator, *children)
        return ('predicate', *parts)
    return tree(tokens)

def columns(text, pieces):
    result = []
    for part in pieces(text):
        m = re.fullmatch(r'\s*' + IDENT + r'\s*(?:\((\d+)\))?\s*(?:ASC)?\s*', part, re.I)
        require(m, 'unsupported key component ' + part)
        result.append((m.group(1), int(m.group(2)) if m.group(2) else None))
    return tuple(result)

def definition(sql, pieces):
    """Return (kind, optional name, structural value), or None for a column."""
    sql = sql.strip()
    named = re.match(r'^CONSTRAINT\s+' + IDENT + r'\s+(.+)$', sql, re.I | re.S)
    name = named.group(1) if named else None
    body = named.group(2) if named else sql
    check = re.fullmatch(r'CHECK\s*\((.*)\)', body, re.I | re.S)
    if check:
        return 'check', name, expression(check.group(1))
    fk = re.fullmatch(r'FOREIGN\s+KEY\s*\(([^)]+)\)\s+REFERENCES\s+' + IDENT + r'\s*\(([^)]+)\)(.*)', body, re.I | re.S)
    if fk:
        tail = fk.group(4)
        rules = {}
        for action in ('DELETE', 'UPDATE'):
            m = re.search(r'ON\s+' + action + r'\s+(RESTRICT|CASCADE|SET\s+NULL|NO\s+ACTION)', tail, re.I)
            rules[action] = re.sub(r'\s+', ' ', m.group(1)).upper() if m else 'RESTRICT'
            if rules[action] == 'NO ACTION': rules[action] = 'RESTRICT'
        return 'foreign', name, (tuple(x[0] for x in columns(fk.group(1), pieces)), fk.group(2), tuple(x[0] for x in columns(fk.group(3), pieces)), rules['DELETE'], rules['UPDATE'])
    pk = re.fullmatch(r'PRIMARY\s+KEY\s*\((.*)\)', body, re.I | re.S)
    if pk:
        return 'index', 'PRIMARY', (0, columns(pk.group(1), pieces))
    key = re.fullmatch(r'(UNIQUE\s+)?(?:KEY|INDEX)\s+(?:IF\s+NOT\s+EXISTS\s+)?' + IDENT + r'\s*\((.*)\)', body, re.I | re.S)
    if key:
        return 'index', key.group(2), (0 if key.group(1) else 1, columns(key.group(3), pieces))
    return None

def expected(migrations, statements, pieces):
    tables = {}
    def apply(table, part, alter=False):
        target = tables.setdefault(table, {'index': {}, 'check': {}, 'foreign': {}, 'engine': 'InnoDB', 'removed': {'index': set(), 'check': set(), 'foreign': set()}})
        drop = re.fullmatch(r'DROP\s+(FOREIGN\s+KEY|CONSTRAINT|INDEX|KEY)\s+(?:IF\s+EXISTS\s+)?' + IDENT, part.strip(), re.I)
        if drop:
            kinds = ('index',) if drop.group(1).upper() in ('INDEX', 'KEY') else ('check', 'foreign')
            for kind in kinds:
                target[kind].pop(drop.group(2), None)
                target['removed'][kind].add(drop.group(2))
            return
        if re.fullmatch(r'DROP\s+PRIMARY\s+KEY', part.strip(), re.I):
            target['index'].pop('PRIMARY', None); target['removed']['index'].add('PRIMARY'); return
        part = re.sub(r'^ADD\s+', '', part.strip(), flags=re.I) if alter else part.strip()
        part = re.sub(r'^(CONSTRAINT\s+)IF\s+NOT\s+EXISTS\s+', r'\1', part, flags=re.I)
        parsed = definition(part, pieces)
        if parsed:
            kind, name, value = parsed
            target[kind][name or ('anonymous:' + str(value))] = value
            if name: target['removed'][kind].discard(name)
            return
        field = re.match(r'^' + IDENT + r'\s+(.+)', part, re.I | re.S)
        if field and field.group(1).upper() not in ('MODIFY', 'DROP', 'ENGINE'):
            if re.search(r'\bPRIMARY\s+KEY\b', field.group(2), re.I):
                target['index']['PRIMARY'] = (0, ((field.group(1), None),))
            elif re.search(r'\bUNIQUE\b', field.group(2), re.I):
                target['index'][field.group(1)] = (0, ((field.group(1), None),))
    for _, raw in migrations:
        for sql in statements(raw):
            candidates = [sql]
            # Shipped guarded PREPARE migrations use a complete literal ALTER.
            candidates += re.findall(r"'(ALTER\s+TABLE\s+[^']+)'", sql, re.I)
            candidates += re.findall(r'^\s*(ALTER\s+TABLE\s+[^;]+);', sql, re.I | re.M)
            for statement in candidates:
                create = re.fullmatch(r'CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?' + IDENT + r'\s*\((.*)\)\s*ENGINE\s*=\s*(\w+).*', statement, re.I | re.S)
                like = re.fullmatch(r'CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?' + IDENT + r'\s+LIKE\s+' + IDENT, statement, re.I)
                alter = re.fullmatch(r'ALTER\s+TABLE\s+(?:IF\s+EXISTS\s+)?' + IDENT + r'\s+(.+)', statement, re.I | re.S)
                if create:
                    table = create.group(1)
                    if table not in tables:
                        tables[table] = {'index': {}, 'check': {}, 'foreign': {}, 'engine': create.group(3), 'removed': {'index': set(), 'check': set(), 'foreign': set()}}
                        for part in pieces(create.group(2)): apply(table, part)
                elif like:
                    require(like.group(2) in tables, 'missing CREATE LIKE prerequisite ' + like.group(2))
                    tables[like.group(1)] = copy.deepcopy(tables[like.group(2)])
                    # MariaDB LIKE copies indexes/checks, deliberately not FKs.
                    tables[like.group(1)]['foreign'] = {}
                    tables[like.group(1)]['removed'] = {'index': set(), 'check': set(), 'foreign': set()}
                elif alter:
                    for part in pieces(alter.group(2)): apply(alter.group(1), part, True)
    return tables

def actual_indexes(db, table):
    rows = db.query('SELECT index_name,non_unique,column_name,sub_part FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name=' + quote(table) + ' ORDER BY index_name,seq_in_index')
    result = {}
    for name, nonunique, field, prefix in rows:
        require(field != 'NULL', 'functional index unsupported on ' + table)
        value = result.setdefault(name, [int(nonunique), []])
        value[1].append((field, None if prefix == 'NULL' else int(prefix)))
    return {name: (value[0], tuple(value[1])) for name, value in result.items()}

def actual_foreign(db, table):
    rows = db.query('SELECT k.constraint_name,k.column_name,k.referenced_table_name,k.referenced_column_name,r.delete_rule,r.update_rule FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.table_name=k.table_name AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.table_name=' + quote(table) + ' AND k.referenced_table_name IS NOT NULL ORDER BY k.constraint_name,k.ordinal_position')
    grouped = {}
    for name, field, parent, parentfield, deletion, update in rows:
        v = grouped.setdefault(name, [[], parent, [], deletion.replace('NO ACTION', 'RESTRICT'), update.replace('NO ACTION', 'RESTRICT')])
        v[0].append(field); v[2].append(parentfield)
    return {name: (tuple(v[0]), v[1], tuple(v[2]), v[3], v[4]) for name, v in grouped.items()}

def verify(db, migrations, statements, pieces):
    tables = expected(migrations, statements, pieces)
    for table, contract in tables.items():
        require(db.exists(table), 'missing table ' + table)
        engines = db.query('SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name=' + quote(table))
        require(engines == [[contract['engine']]] and contract['engine'].lower() == 'innodb', 'transactional engine mismatch ' + table)
        indexes = actual_indexes(db, table)
        for name, value in contract['index'].items():
            require(indexes.get(name) == value, 'missing or incompatible key ' + table + '.' + name)
        foreign = actual_foreign(db, table)
        for name, value in contract['foreign'].items():
            require((value in foreign.values()) if name.startswith('anonymous:') else foreign.get(name) == value, 'missing or incompatible foreign key ' + table + '.' + name)
        checks = {name: expression(clause) for name, clause in db.query('SELECT constraint_name,check_clause FROM information_schema.check_constraints WHERE constraint_schema=DATABASE() AND table_name=' + quote(table))}
        for name, value in contract['check'].items():
            require((value in checks.values()) if name.startswith('anonymous:') else checks.get(name) == value, 'missing or incompatible CHECK ' + table + '.' + name)
        for kind, actual in (('index', indexes), ('foreign', foreign), ('check', checks)):
            require(not (contract['removed'][kind] & actual.keys()), 'superseded ' + kind + ' retained on ' + table + ': ' + ','.join(sorted(contract['removed'][kind] & actual.keys())))
    return len(tables)

# Native contracts predate the housing overlay. This is intentionally a minimum
# supported EQEmu baseline, not a request to create/guess missing content tables.
NATIVE_COLUMNS = {
    'account': 'id', 'character_data': 'id account_id deleted_at',
    'inventory': 'character_id slot_id item_id charges color augment_one augment_two augment_three augment_four augment_five augment_six instnodrop custom_data ornament_icon ornament_idfile ornament_hero_model guid',
    'sharedbank': 'account_id slot_id item_id charges augment_one augment_two augment_three augment_four augment_five augment_six guid',
    'character_currency': 'id platinum gold silver copper platinum_bank gold_bank silver_bank copper_bank platinum_cursor gold_cursor silver_cursor copper_cursor radiant_crystals career_radiant_crystals ebon_crystals career_ebon_crystals',
    'items': 'id itemclass bagslots loregroup nodrop notransfer UNK224 evoitem charmfileid idfile augtype',
    'items_evolving_details': 'id item_id',
    'character_evolving_items': 'id character_id item_id activated equipped current_amount progression final_item_id deleted_at',
    'guilds': 'id leader', 'guild_members': 'char_id guild_id rank banker',
    'guild_bank': 'id guild_id area slot item_id augment_one_id augment_two_id augment_three_id augment_four_id augment_five_id augment_six_id quantity donator permissions who_for',
    'tributes': 'id unknown name descr isguild', 'tribute_levels': 'tribute_id level cost item_id',
    'character_tribute': 'character_id tier tribute', 'guild_tributes': 'guild_id tribute_id_1 tribute_id_1_tier tribute_id_2 tribute_id_2_tier time_remaining enabled',
    'character_parcels': 'id char_id item_id aug_slot_1 aug_slot_2 aug_slot_3 aug_slot_4 aug_slot_5 aug_slot_6 slot_id quantity evolve_amount from_name note sent_date',
    'character_parcels_containers': 'id parcels_id slot_id item_id aug_slot_1 aug_slot_2 aug_slot_3 aug_slot_4 aug_slot_5 aug_slot_6 quantity evolve_amount',
    'instance_list': 'id zone version never_expires', 'instance_list_player': 'id charid',
    'zone': 'zoneidnumber short_name version', 'doors': 'id zone version doorid',
    'npc_types': 'id name class race gender size merchant_id',
    'merchantlist': 'merchantid slot item', 'merchantlist_temp': 'npcid slot itemid charges',
    'spawn2': 'id spawngroupID zone version x y z heading', 'spawnentry': 'spawngroupID npcID chance',
    'spawngroup': 'id name', 'variables': 'varname value',
    'rule_sets': 'ruleset_id name', 'rule_values': 'ruleset_id rule_name rule_value',
}
NATIVE_KEYS = {'account': ('id',), 'character_data': ('id',), 'inventory': ('character_id', 'slot_id'), 'sharedbank': ('account_id', 'slot_id'), 'character_currency': ('id',), 'items': ('id',), 'items_evolving_details': ('id',), 'character_evolving_items': ('id',), 'guilds': ('id',), 'guild_members': ('char_id',), 'guild_bank': ('id',), 'tributes': ('id', 'isguild'), 'character_parcels': ('id',), 'character_parcels_containers': ('id',), 'instance_list': ('id',), 'npc_types': ('id',), 'spawngroup': ('id',), 'spawn2': ('id',), 'rule_sets': ('ruleset_id',), 'rule_values': ('ruleset_id', 'rule_name')}

def verify_native(db):
    for table, fields in NATIVE_COLUMNS.items():
        require(db.exists(table), 'missing native prerequisite ' + table)
        shape = {name.lower(): spec for name, spec in db.columns(table).items()}
        actual = set(shape)
        require(set(fields.lower().split()) <= actual, 'native columns missing on ' + table + ': ' + ','.join(sorted(set(fields.lower().split()) - actual)))
        textual = {'deleted_at','sent_date','from_name','note','custom_data','idfile','charmfileid','donator','who_for','name','descr','short_name','varname','value','rule_name','rule_value'}
        if table in ('doors', 'spawn2'): textual.add('zone')
        for field in set(fields.lower().split()) - textual:
            require(re.fullmatch(r'(?:tinyint|smallint|mediumint|int|bigint|float(?:\([^)]*\))?|double(?:\([^)]*\))?)(?: unsigned)?', shape[field]['type']), 'native numeric column type incompatible: ' + table + '.' + field)
        if table in ('inventory','sharedbank'):
            require(shape['guid']['type'] == 'bigint unsigned', 'native persistent GUID width incompatible: ' + table)
    for table, names in NATIVE_KEYS.items():
        indexes = actual_indexes(db, table)
        require(any(not nonunique and tuple(field for field, _ in fields) == names for nonunique, fields in indexes.values()), 'native unique identity key missing on ' + table)
    return len(NATIVE_COLUMNS)
