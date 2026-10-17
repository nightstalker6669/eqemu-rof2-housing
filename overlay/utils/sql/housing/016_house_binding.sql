-- Optional durable HOUSE identity/location foundation. No seeds or runtime hook.
-- Requires 007/008/009; caller-owned transactions join existing custody receipts.
-- Current legacy instance_list.id is signed INT. Check FK compatibility before
-- installing on another schema; do not rewrite the legacy table to fit this DDL.
-- No custom_data or GUID identity and no inventory.v1 descriptor changes.
CREATE TABLE IF NOT EXISTS housing_house_instances (
 house_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
 item_template_id INT UNSIGNED NOT NULL,
 created_by_character_id INT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL DEFAULT 1,
 pending_request_key VARBINARY(96) NULL,
 CONSTRAINT housing_house_template CHECK(item_template_id>0),
 CONSTRAINT housing_house_revision CHECK(revision>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_house_locations (
 house_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 location_kind TINYINT UNSIGNED NOT NULL,
 character_id INT UNSIGNED NULL,
 slot_id INT UNSIGNED NULL,
 custody_id BIGINT UNSIGNED NULL,
 UNIQUE KEY housing_house_inventory_location(character_id,slot_id),
 UNIQUE KEY housing_house_custody_location(custody_id),
 CONSTRAINT housing_house_location_identity FOREIGN KEY(house_id)
  REFERENCES housing_house_instances(house_id) ON DELETE RESTRICT,
 CONSTRAINT housing_house_location_custody FOREIGN KEY(custody_id)
  REFERENCES housing_live_item_custody(custody_id) ON DELETE RESTRICT,
 CONSTRAINT housing_house_location_shape CHECK(
  (location_kind=1 AND character_id IS NOT NULL AND character_id>0
   AND slot_id IS NOT NULL AND custody_id IS NULL) OR
  (location_kind=2 AND character_id IS NULL AND slot_id IS NULL
   AND custody_id IS NOT NULL AND custody_id>0))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_house_interiors (
 house_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 native_estate_id INT UNSIGNED NOT NULL UNIQUE,
 instance_id INT NOT NULL UNIQUE,
 catalog_key VARBINARY(96) NOT NULL,
 definition_id INT UNSIGNED NOT NULL,
 zone_id INT UNSIGNED NOT NULL,
 zone_version TINYINT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 CONSTRAINT housing_house_interior_identity FOREIGN KEY(house_id)
  REFERENCES housing_house_instances(house_id) ON DELETE RESTRICT,
 CONSTRAINT housing_house_interior_native FOREIGN KEY(native_estate_id)
  REFERENCES housing_native_estates(native_id) ON DELETE RESTRICT,
 CONSTRAINT housing_house_interior_instance FOREIGN KEY(instance_id)
  REFERENCES instance_list(id) ON DELETE RESTRICT,
 CONSTRAINT housing_house_interior_catalog FOREIGN KEY(catalog_key)
  REFERENCES housing_catalogs(catalog_key) ON DELETE RESTRICT,
 CONSTRAINT housing_house_interior_ids CHECK(native_estate_id>0 AND native_estate_id<4294967295
  AND instance_id>0 AND instance_id<=32000 AND zone_id>0 AND definition_id>0 AND catalog_revision>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_house_receipt_effects (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 house_id BIGINT UNSIGNED NOT NULL,
 effects_version INT UNSIGNED NOT NULL,
 effects_blob MEDIUMBLOB NOT NULL,
 CONSTRAINT housing_house_effect_parent FOREIGN KEY(request_key)
  REFERENCES housing_live_item_receipts(request_key) ON DELETE RESTRICT,
 CONSTRAINT housing_house_effect_identity FOREIGN KEY(house_id)
  REFERENCES housing_house_instances(house_id) ON DELETE RESTRICT,
 CONSTRAINT housing_house_effect_version CHECK(effects_version=1)
) ENGINE=InnoDB;
-- Exactly one location is enforced per house by PK and service invariants. SQL
-- cannot assert every instance has a child location: mint + location is ONE tx.
-- No deletion, trade, move/swap, guild policy or automatic crash recovery adapter.
