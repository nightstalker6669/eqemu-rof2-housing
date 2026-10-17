-- Optional, reviewed original native item-definition metadata only.
-- No automatic seeds/backfill from names, IDFile icons, filename or UNK fields.
-- An absent row is UNKNOWN, retaining the old empty/zero serializer bytes.
-- These sources do NOT authorize housing NPC placement, local entity height,
-- animated collision, services, respawn timers or pet capacity. Those classes
-- remain rejected by the existing ordinary placement/scene/content paths.
CREATE TABLE IF NOT EXISTS housing_native_npc_item_sources (
  item_id int NOT NULL,
  housepet_filename varbinary(31) NOT NULL,
  interactive_object tinyint unsigned NOT NULL,
  npc_respawn_interval int unsigned NOT NULL,
  source_sha256 binary(32) NOT NULL,
  PRIMARY KEY (item_id)
) ENGINE=InnoDB;
