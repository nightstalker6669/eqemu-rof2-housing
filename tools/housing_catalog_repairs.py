"""Narrow, transactional repairs for recognized unused housing placeholders."""
import hashlib
import struct


def _palatial_zone_offset(payload):
    """Validate the complete bootstrap framing before locating definition40."""
    if len(payload) > 4 * 1024 * 1024:
        raise ValueError('Housing bootstrap exceeds repair bound')
    position = 0

    def word():
        nonlocal position
        if position + 4 > len(payload):
            raise ValueError('Truncated housing bootstrap')
        value = struct.unpack_from('<I', payload, position)[0]
        position += 4
        return value

    def bounded_count(maximum):
        value = word()
        if value > maximum:
            raise ValueError('Housing bootstrap count exceeds repair bound')
        return value

    found = None
    identities = set()
    for _ in range(bounded_count(65535)):
        start = position
        fields = [word() for _ in range(11)]
        if fields[0] in identities:
            raise ValueError('Duplicate housing definition')
        identities.add(fields[0])
        length = bounded_count(255)
        if position + length > len(payload):
            raise ValueError('Truncated housing definition address')
        address = payload[position:position + length]
        position += length
        for _ in range(6):  # icon, NPC limit, three coordinates, heading
            word()
        for _ in range(bounded_count(1000)):
            word()
        if fields[0] == 40 and fields[7] == 712 and fields[9] == 2 and address == b'EXPERIMENTAL: classification only; NO INTERIOR ROUTE':
            found = start + 28
    for _ in range(bounded_count(65535)):
        word()  # cost scheme ID
        for _ in range(bounded_count(65535)):
            word()  # component ID
            for _ in range(bounded_count(65535)):
                for _ in range(4):  # type, subtype, signed64 amount
                    word()
    if position != len(payload):
        raise ValueError('Unexpected housing bootstrap trailing bytes')
    return found


def repair_unused_palatial_routes(db):
    """Correct only the known unused definition40 zone712 placeholder to737.

    This fixes a placeholder within its existing catalogue generation. Other
    definitions, regions, prices, node IDs and existing content fingerprints
    remain unchanged. A bound Palatial interior requires explicit migration
    and is refused here. The caller holds the installer lock, has stopped all
    writers, and has completed the standard database backup.
    """
    repaired = []
    db.query('START TRANSACTION;')
    try:
        catalogs = db.query('SELECT HEX(catalog_key),revision,HEX(definitions_blob) FROM housing_catalogs FOR UPDATE;')
        for key, revision, raw in catalogs:
            old = bytes.fromhex(raw)
            offset = _palatial_zone_offset(old)
            if offset is None:
                continue
            catalog = "UNHEX('" + key + "')"
            bound = db.scalar('SELECT COUNT(*) FROM housing_house_instances h JOIN housing_house_interiors i USING(house_id) WHERE i.catalog_key=' + catalog + ' AND h.item_template_id=66620;')
            if bound != '0':
                raise ValueError('Palatial placeholder has a bound interior; explicit migration required')
            new = bytearray(old)
            struct.pack_into('<I', new, offset, 737)
            before = hashlib.sha256(old).hexdigest()
            after = hashlib.sha256(new).hexdigest()
            policies = {}
            for table in ('housing_native_plot_purchase_policies', 'housing_upkeep_enrollment_policy'):
                rows = db.query('SELECT catalog_revision,definitions_sha256 FROM ' + table + ' WHERE catalog_key=' + catalog + ' FOR UPDATE;')
                if any(str(r[0]) != str(revision) or r[1].lower() != before for r in rows):
                    raise ValueError('Housing policy does not match the placeholder catalogue')
                policies[table] = len(rows)
            changed = db.scalar('UPDATE housing_catalogs SET definitions_blob=UNHEX(\'' + new.hex() + '\') WHERE catalog_key=' + catalog + ' AND revision=' + str(int(revision)) + " AND SHA2(definitions_blob,256)='" + before + "'; SELECT ROW_COUNT();")
            if changed != '1':
                raise ValueError('Housing catalogue repair comparison failed')
            for table, count in policies.items():
                extra = ',policy_revision=policy_revision+1' if table == 'housing_native_plot_purchase_policies' else ''
                changed = db.scalar("UPDATE " + table + " SET definitions_sha256='" + after + "'" + extra + ' WHERE catalog_key=' + catalog + " AND definitions_sha256='" + before + "'; SELECT ROW_COUNT();")
                if int(changed) != count:
                    raise ValueError('Housing policy repair comparison failed')
            repaired.append(dict(catalog_key_hex=key, revision=int(revision), definition_id=40, zone_before=712, zone_after=737, before_sha256=before, after_sha256=after))
        db.query('COMMIT;')
    except Exception:
        db.query('ROLLBACK;')
        raise
    return repaired
