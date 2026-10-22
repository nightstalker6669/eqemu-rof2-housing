#!/usr/bin/env python3
"""Generate private, source-bound housing content from reviewed production metadata.

No database connection, service control, ownership grant or original game asset
is supplied. Generated files belong in a private deployment directory. Existing
manifests are never overwritten, and preserved templates are never redefined.
"""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import shutil
import struct
import sys
import tempfile

MAX_JSON = 4 * 1024 * 1024
MAX_REGIONS = 262144
U32 = 0xffffffff


def require(value, message):
    if not value:
        raise ValueError(message)


def integer(value, label, minimum=0, maximum=U32):
    require(type(value) is int and minimum <= value <= maximum, 'Invalid ' + label)
    return value


def unique(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, 'Duplicate JSON key: ' + key)
        result[key] = value
    return result


def read_json(path):
    require(path.is_file() and not path.is_symlink() and path.stat().st_size <= MAX_JSON,
            'Missing, linked or oversized JSON: ' + path.name)
    result = json.loads(path.read_text(encoding='utf-8'), object_pairs_hook=unique)
    require(isinstance(result, dict), 'Expected JSON object: ' + path.name)
    return result


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def file_sha(path):
    require(path.is_file() and not path.is_symlink(), 'Missing regular resource: ' + path.name)
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for raw in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(raw)
    return value.hexdigest()


def text(value, label):
    require(isinstance(value, str) and value and len(value) <= 4096, 'Invalid ' + label)
    return value


def source_include_text(path, active=()):
    require(path.is_file() and not path.is_symlink() and path.stat().st_size <= MAX_JSON,
            'Missing, linked or oversized compiled registry: ' + path.name)
    identity = path.resolve()
    require(identity not in active and len(active) <= 4, 'Cyclic or excessive registry include depth')
    raw = path.read_text(encoding='utf-8')
    includes = re.findall(r'^\s*#include "([^"\n]+\.inc)"', raw, re.MULTILINE)
    require(len(includes) == len(set(includes)), 'Duplicate registry include')
    for name in includes:
        require(re.fullmatch(r'[A-Za-z0-9_]+\.inc', name), 'Unsafe compiled registry include')
        pattern = r'^\s*#include "' + re.escape(name) + r'"[ \t]*$'
        require(len(re.findall(pattern, raw, re.MULTILINE)) == 1, 'Unsupported registry include syntax')
        extra = source_include_text(path.parent / name, active + (identity,))
        raw = re.sub(pattern, lambda match: extra, raw, flags=re.MULTILINE)
    return raw


def templates(source):
    raw = source_include_text(source / 'common/housing/house_template_registry.h')
    rows = re.findall(r'\{(\d+),(\d+),(\d+),(\d+),"(IT\d+)"\}', raw)
    result = {}
    for item, definition, kind, zone, model in rows:
        item = int(item)
        require(item not in result, 'Duplicate compiled house template')
        result[item] = dict(item_id=item, definition_id=int(definition), type=int(kind),
                            zone_id=int(zone), model=model)
    require(result, 'Missing compiled template registry')
    geometry_file = source / 'common/housing/house_geometry_registry.h'
    geometry_text = source_include_text(geometry_file)
    models = {}
    for item, model in re.findall(r'\{(\d+),\d+,"(IT\d+)"', geometry_text):
        require(int(item) not in models, 'Duplicate compiled exterior geometry')
        models[int(item)] = model
    require(all(models.get(item) == row['model'] for item, row in result.items()),
            'Reviewed template is missing its exact compiled exterior geometry')
    return result


def preserved_items(registries, manifests):
    result = set()
    for registry in registries:
        doc = read_json(registry)
        require(doc.get('schema') in ('housing-personal-content-registry-v1',
                'housing-guild-content-registry-v1', 'housing-personal-content-registry-test-v1',
                'housing-guild-content-registry-test-v1'), 'Unsupported preserved registry')
        require(isinstance(doc.get('entries'), list), 'Invalid preserved registry entries')
        for entry in doc['entries']:
            require(isinstance(entry, dict), 'Invalid preserved entry')
            name = text(entry.get('manifest'), 'preserved manifest name')
            require(re.fullmatch(r'[A-Za-z0-9._-]{1,128}', name) and '..' not in name,
                    'Unsafe preserved manifest path')
            manifests.append(registry.parent / name)
    for manifest in manifests:
        doc = read_json(manifest)
        item = doc.get('item_template_id')
        # Match only the explicit legacy personal filename's existing loader
        # default. This excludes content from generation; it grants no route.
        if item is None and manifest.name == '.house-personal-test.json' and doc.get('version') == 1:
            item = 61823
        result.add(integer(item, 'preserved item', 1))
    return result


def validate_geometry(zone):
    integer(zone.get('zone_id'), 'geometry zone', 1)
    require(re.fullmatch(r'[a-z0-9_]+', text(zone.get('short_name'), 'zone short name')),
            'Invalid zone resource name')
    for field in ('client_asset_sha256', 'collision_map_sha256', 'regions_words_sha256'):
        require(re.fullmatch(r'[a-f0-9]{64}', text(zone.get(field), field)), 'Invalid source hash')
    words = zone.get('regions_words')
    require(isinstance(words, list) and words and 4 + 88 * len(words) <= MAX_REGIONS,
            'Unsupported region count')
    body = bytearray()
    for row in words:
        require(isinstance(row, list) and len(row) == 21, 'Invalid region word record')
        for word in row:
            integer(word, 'region word')
        floats = struct.unpack('<19f', struct.pack('<19I', *row[:19]))
        require(all(math.isfinite(value) for value in floats), 'Nonfinite region word')
        m = floats[:16]
        determinant = (m[0] * (m[5]*m[10]-m[6]*m[9]) -
                       m[1] * (m[4]*m[10]-m[6]*m[8]) +
                       m[2] * (m[4]*m[9]-m[5]*m[8]))
        require(math.isfinite(determinant) and determinant != 0, 'Singular region matrix')
        body.extend(struct.pack('<21I', *row))
    require(sha(body) == zone['regions_words_sha256'], 'Region blueprint hash mismatch')
    spawn = zone.get('spawn')
    require(isinstance(spawn, list) and len(spawn) == 4 and
            all(type(value) in (int, float) and math.isfinite(value) for value in spawn) and
            0 <= spawn[3] <= 512, 'Invalid reviewed spawn')
    text(zone.get('region_policy'), 'region policy')
    text(zone.get('spawn_policy'), 'spawn policy')


def region_payload(zone, definition):
    area = 2000000 + integer(definition, 'definition', 1, 100000) * 10000
    require(area + len(zone['regions_words']) < U32, 'Profile-local region range exhausted')
    raw = struct.pack('<I', len(zone['regions_words']))
    for index, words in enumerate(zone['regions_words']):
        raw += struct.pack('<I21I', area + index, *words)
    return area, raw


def definition_payload(definition, child_definitions):
    names = ('id', 'area_id', 'purchase_scheme', 'upkeep_scheme', 'max_items',
             'max_non_estate_items', 'max_children', 'zone_id', 'dynamic_zone_id', 'type', 'group_id')
    records = [definition] + [child_definitions[key] for key in definition['children']]
    raw = struct.pack('<I', len(records))
    for record in records:
        address = record.get('address', '')
        require(isinstance(address, str) and len(address) <= 255 and
                all(32 <= ord(c) <= 126 for c in address), 'Invalid definition address')
        encoded = address.encode('ascii')
        coordinates = record.get('switch_coordinates', [0, 0, 0])
        heading = record.get('switch_heading', 0)
        require(isinstance(coordinates, list) and len(coordinates) == 3 and
                all(type(v) in (int, float) and math.isfinite(v) for v in coordinates + [heading]),
                'Invalid definition switch coordinates')
        children = record['children']
        raw += struct.pack('<11I', *(integer(record.get(name, 0), name) for name in names))
        raw += struct.pack('<I', len(encoded)) + encoded
        raw += struct.pack('<2I4fI', integer(record.get('icon', 0), 'icon'),
                           integer(record['max_npc_items'], 'NPC capacity'), *coordinates,
                           heading, len(children))
        raw += b''.join(struct.pack('<I', child) for child in children)
    return raw + struct.pack('<I', 0)  # No cost schemes supplied.


def reviewed_capacity_policy(path):
    doc = read_json(path)
    require(doc.get('format') == 'housing-house-capacity-defaults-v1', 'Unsupported capacity policy')
    integer(doc.get('policy_revision'), 'capacity policy revision', 1)
    child_definitions = {}
    # This is the only complete typed child payload reviewed in this package.
    # Do not convert recognition of other crate IDs into an invented capacity.
    approved = dict(id=174, area_id=0, purchase_scheme=0, upkeep_scheme=0, max_items=100,
                    max_non_estate_items=100, max_children=0, zone_id=0, dynamic_zone_id=0,
                    type=7, group_id=0, address='', icon=0, max_npc_items=0,
                    switch_coordinates=[0, 0, 0], switch_heading=0, children=[])
    for child in doc.get('child_definitions', []):
        require(child == approved and child['id'] not in child_definitions,
                'Unreviewed or duplicated child-estate definition')
        child_definitions[child['id']] = child
    policies = {}
    for row in doc.get('definitions', []):
        require(isinstance(row, dict), 'Invalid capacity policy row')
        key = integer(row.get('definition_id'), 'policy definition', 1)
        require(key not in policies and row.get('type') in (2, 3), 'Duplicate or invalid policy definition')
        fields = row.get('effective_fields')
        require(isinstance(fields, dict) and set(fields) ==
                {'max_items', 'max_non_estate_items', 'max_npc_items', 'max_children', 'children'},
                'Incomplete effective capacity fields')
        total = integer(fields['max_items'], 'effective total capacity', 1)
        for name in ('max_non_estate_items', 'max_npc_items', 'max_children'):
            integer(fields[name], name, 0, total)
        children = fields['children']
        require(isinstance(children, list) and len(children) == len(set(children)) and
                all(type(child) is int and child in child_definitions for child in children) and
                (bool(children) == bool(fields['max_children'])), 'Unreviewed child-estate policy')
        sources = row.get('field_sources')
        require(isinstance(sources, dict) and set(sources) == set(fields), 'Missing capacity field provenance')
        for name, evidence in sources.items():
            require(isinstance(evidence, dict), 'Invalid capacity field provenance')
            text(evidence.get('classification'), 'capacity provenance classification')
            text(evidence.get('description'), 'capacity provenance description')
        policies[key] = row
    require(policies, 'Empty capacity policy')
    return doc, policies, child_definitions


def write_json(directory, name, doc):
    raw = (json.dumps(doc, indent=2, sort_keys=True) + '\n').encode('utf-8')
    (directory / name).write_bytes(raw)
    return sha(raw)


def generate(args):
    source = args.source_root.resolve()
    require(not args.output.exists(), 'Output already exists; refusing to overwrite deployment content')
    require(args.output.parent.is_dir(), 'Output parent directory must exist')
    compiled = templates(source)
    geometry_file = source / 'common/housing/house_variant_geometry.json'
    capacity_file = source / 'common/housing/house_variant_capacities.json'
    policy_file = args.capacity_policy or source / 'common/housing/house_variant_capacity_defaults.json'
    policy_doc, effective_policies, child_definitions = reviewed_capacity_policy(policy_file)
    geometry = read_json(geometry_file)
    require(geometry.get('format') == 'housing-variant-geometry-blueprints-v1' and
            geometry.get('authentic_live_regions') is False, 'Unsupported geometry provenance')
    capacity = read_json(capacity_file)
    require(capacity.get('format') == 'reviewed-house-variant-capacities-v1' and
            capacity.get('unknown_fields_are_null') is True and
            isinstance(capacity.get('definitions'), list), 'Unsupported capacity registry')
    zones, definitions = {}, {}
    for zone in geometry.get('zones', []):
        validate_geometry(zone)
        require(zone['zone_id'] not in zones, 'Duplicate geometry zone')
        require(not zone.get('root_approval_pending', False), 'Unapproved technical geometry blueprint')
        zones[zone['zone_id']] = zone
    for definition in capacity['definitions']:
        require(isinstance(definition, dict), 'Invalid capacity entry')
        key = integer(definition.get('definition_id'), 'capacity definition', 1)
        require(key not in definitions, 'Duplicate capacity definition')
        if definition.get('max_items') is not None:
            integer(definition['max_items'], 'reviewed total capacity', 1)
            require(definition.get('type') in (2, 3), 'Invalid capacity definition type')
            text(definition.get('evidence'), 'capacity evidence')
            text(definition.get('provenance'), 'capacity provenance')
            urls = definition.get('source_urls')
            require(isinstance(urls, list) and urls and all(isinstance(url, str) and
                    re.fullmatch(r'https?://[^\s]+', url) for url in urls), 'Missing capacity source URLs')
        definitions[key] = definition
    preserved = preserved_items(list(args.preserve_registry), list(args.preserve_manifest))
    preserved.update(args.preserve_item)
    require(preserved <= set(compiled), 'Preserved item is outside the reviewed template registry')
    # The existing importer authenticates the original executable, renderer and
    # source manifests. Import without creating public __pycache__ artifacts.
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location('housing_original_geometry', Path(__file__).with_name('install_original_geometry.py'))
    importer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(importer)
    importer.client_profile(args.client, source)
    authenticated, excluded, rows, catalogue_rows = {}, [], [], []
    for item, template in sorted(compiled.items()):
        definition = definitions.get(template['definition_id'])
        zone = zones.get(template['zone_id'])
        effective = effective_policies.get(template['definition_id'])
        if not definition or not effective or not zone:
            excluded.append(dict(item_id=item, reason='Missing reviewed effective capacity policy or source-bound room geometry'))
            continue
        require(definition.get('type') == template['type'] and definition.get('zone_id') == template['zone_id'],
                'Capacity/template route mismatch for item ' + str(item))
        require(effective['type'] == template['type'], 'Capacity policy type mismatch')
        catalogue_rows.append((template, definition, zone))
        if item in preserved:
            excluded.append(dict(item_id=item, reason='Existing configured template preserved byte-for-byte; no replacement generated'))
            continue
        if zone['zone_id'] not in authenticated:
            archive = importer.child(args.client, zone['short_name'] + '.eqg')
            map_file = args.maps / (zone['short_name'] + '.map')
            require(file_sha(archive) == zone['client_asset_sha256'] and file_sha(map_file) == zone['collision_map_sha256'],
                    'Original room/map hash mismatch: ' + zone['short_name'])
            authenticated[zone['zone_id']] = (archive, map_file)
        rows.append((template, definition, zone))
    for capacity_row in definitions.values():
        for item in capacity_row.get('template_ids', []):
            integer(item, 'capacity template item', 1)
            if item not in compiled:
                excluded.append(dict(item_id=item, reason='Capacity evidence alone does not admit an unresolved template/model/route association'))
    require(catalogue_rows, 'No reviewed templates are available for the catalogue')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(tempfile.mkdtemp(prefix='.housing-variant-content-', dir=args.output.parent))
    try:
        registries = {kind: {'schema': 'housing-' + label + '-content-registry-v1', 'entries': []}
                      for kind, label in ((3, 'personal'), (2, 'guild'))}
        catalog_definitions, artifacts = {}, []
        # Fresh catalogues need all reviewed child definitions, including SKUs
        # whose already-configured runtime manifests were deliberately preserved.
        for template, capacity_row, zone in catalogue_rows:
            key = template['definition_id']
            outer = dict(id=key, area_id=0, purchase_scheme=0, upkeep_scheme=0,
                         zone_id=template['zone_id'], dynamic_zone_id=0, type=template['type'], group_id=0,
                         **effective_policies[key]['effective_fields'])
            require(key not in catalog_definitions or catalog_definitions[key] == outer,
                    'Conflicting shared house definition')
            catalog_definitions[key] = outer
        for template, capacity_row, zone in rows:
            item, definition_id, kind = template['item_id'], template['definition_id'], template['type']
            area, regions = region_payload(zone, definition_id)
            definition = dict(id=definition_id, area_id=area, purchase_scheme=0, upkeep_scheme=0,
                              zone_id=template['zone_id'], dynamic_zone_id=0, type=kind, group_id=0,
                              **effective_policies[definition_id]['effective_fields'])
            policy = dict(unknown_historical_values=None, wire_limits_are_configurable_emulator_policy=True,
                          non_estate_storage='ENABLED_SHARED_TOTAL_BUDGET' if definition['max_non_estate_items'] else 'DISABLED_BY_OPERATOR_POLICY',
                          npc_items='ENABLED_CONFIGURABLE_ACTOR_LIMIT' if definition['max_npc_items'] else 'DISABLED_BY_OPERATOR_POLICY',
                          child_estates_including_crates='ENABLED_REVIEWED_CHILD_DEFINITION_174' if definition['max_children'] else 'DISABLED_BY_OPERATOR_POLICY',
                          upkeep='EXPLICIT_NO_UPKEEP_COMPATIBILITY_LOADER_POLICY')
            manifest = dict(version=1, client_sha256=geometry['source_client_sha256'],
                            renderer_sha256=geometry['source_renderer_sha256'], experimental_no_upkeep=True,
                            item_template_id=item, definition_id=definition_id, interior_zone_id=zone['zone_id'],
                            interior_short_name=zone['short_name'], interior_asset_sha256=zone['client_asset_sha256'],
                            collision_map_sha256=zone['collision_map_sha256'], spawn=zone['spawn'], exit=[0,0,0,0],
                            exit_policy='CURRENT_SOURCE_BOUND_PARENT_EXIT; legacy global point unused',
                            content_classification='REVIEWED_RECONSTRUCTED_VARIANT_ORDINARY_FURNISHING',
                            authentic_live_route=False, authentic_live_regions=False, temporary_policy=policy,
                            spawn_policy=zone['spawn_policy'], capacity_provenance=capacity_row,
                            capacity_policy_revision=policy_doc['policy_revision'],
                            capacity_policy_source_sha256=file_sha(policy_file),
                            capacity_policy=effective_policies[definition_id])
            house_name, room_name = 'housing-house-' + str(item) + '.json', 'housing-room-' + str(item) + '.json'
            room = dict(schema='housing-room-' + ('guild' if kind == 2 else 'personal') + '-v1',
                        profile_revision=1, definitions_hex=definition_payload(definition, child_definitions).hex(),
                        regions_hex=regions.hex(), geometry_origin=zone['region_policy'], temporary_policy=policy)
            house_hash = write_json(staging, house_name, manifest)
            room_hash = write_json(staging, room_name, room)
            registries[kind]['entries'].append(dict(manifest=house_name, room_manifest=room_name))
            outer = dict(definition, area_id=0)
            require(definition_id not in catalog_definitions or catalog_definitions[definition_id] == outer,
                    'Conflicting shared house definition')
            catalog_definitions[definition_id] = outer
            artifacts.append(dict(item_id=item, definition_id=definition_id, manifest=house_name,
                                  manifest_sha256=house_hash, room_manifest=room_name, room_manifest_sha256=room_hash,
                                  region_payload_sha256=sha(regions)))
        for kind, label in ((3, 'personal'), (2, 'guild')):
            if registries[kind]['entries']:
                write_json(staging, 'housing-' + label + '-content-registry.json', registries[kind])
        write_json(staging, 'housing-catalogue-proposal.json', dict(
            format='housing-fresh-variant-catalogue-v1', fresh_neighborhood_required=True,
            overwrite_existing_bound_catalogues=False, allocate_domain_and_native_ids_through_services=True,
            authentic_live_catalogue=False, house_definitions=list(catalog_definitions.values()),
            child_estate_definitions=list(child_definitions.values()),
            capacity_policy_revision=policy_doc['policy_revision'],
            capacity_policy_source_sha256=file_sha(policy_file),
            personal_plot_child_definitions=sorted(d['id'] for d in catalog_definitions.values() if d['type'] == 3),
            guild_plot_child_definitions=sorted(d['id'] for d in catalog_definitions.values() if d['type'] == 2),
            excluded_items=excluded, capacities_source_sha256=file_sha(capacity_file),
            geometry_source_sha256=file_sha(geometry_file)))
        if args.copy_resources:
            for zone_id, (archive, map_file) in authenticated.items():
                (staging / 'housing-test-assets').mkdir(exist_ok=True)
                (staging / 'maps/base').mkdir(parents=True, exist_ok=True)
                shutil.copyfile(archive, staging / 'housing-test-assets' / archive.name.lower())
                shutil.copyfile(map_file, staging / 'maps/base' / map_file.name)
        write_json(staging, 'housing-capacity-policy.json', policy_doc)
        write_json(staging, 'housing-content-manifest.json', dict(
            format='housing-generated-variant-content-v1', activated=False, database_writes=False,
            runtime_native_ids_supplied=False, profile_local_region_ids_only=True, artifacts=artifacts,
            preserved_items=sorted(preserved), excluded_items=excluded,
            capacity_registry_sha256=file_sha(capacity_file), geometry_registry_sha256=file_sha(geometry_file),
            capacity_policy_revision=policy_doc['policy_revision'], capacity_policy_sha256=file_sha(policy_file),
            resources_copied=args.copy_resources,
            required_private_resources=[dict(zone_id=z, archive=a.name.lower(), archive_sha256=file_sha(a),
                                            map=m.name, map_sha256=file_sha(m))
                                        for z, (a, m) in sorted(authenticated.items())]))
        staging.rename(args.output)
    finally:
        if staging.exists():
            shutil.rmtree(staging)
    print('Generated ' + str(len(rows)) + ' reviewed template profiles; ' + str(len(excluded)) +
          ' exclusions/preserved templates. No database or running service changed.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', required=True, type=Path)
    parser.add_argument('--maps', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[1] / 'overlay')
    parser.add_argument('--capacity-policy', type=Path, help='Reviewed revisioned emulator capacity defaults; never historical evidence')
    parser.add_argument('--preserve-registry', action='append', type=Path, default=[])
    parser.add_argument('--preserve-manifest', action='append', type=Path, default=[])
    parser.add_argument('--preserve-item', action='append', type=int, default=[])
    parser.add_argument('--copy-resources', action='store_true', help='Copy authenticated operator resources into the private output only')
    args = parser.parse_args()
    try:
        generate(args)
    except (ValueError, OSError, KeyError, TypeError, struct.error) as error:
        parser.exit(1, 'Refused: ' + str(error) + '\n')


if __name__ == '__main__':
    main()
