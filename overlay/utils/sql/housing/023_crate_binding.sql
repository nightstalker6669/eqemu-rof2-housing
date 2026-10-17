-- Optional non-zoned crate identity and containment. No seeds/runtime enablement.
-- Requires007/010/016. Install only after parent FK types have been checked.
-- A crate can be carried or placed; native identity and contents are independent
-- of its mutable inventory slot/custody location. No house interior instance.
CREATE TABLE IF NOT EXISTS housing_crate_instances (
 crate_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
 item_template_id INT UNSIGNED NOT NULL,
 created_by_character_id INT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL DEFAULT 1,
 packed TINYINT UNSIGNED NOT NULL DEFAULT 0,
 creation_request_key VARBINARY(96) NOT NULL UNIQUE,
 content_manifest_sha256 BINARY(32) NOT NULL,
 CONSTRAINT housing_crate_creation_parent FOREIGN KEY(creation_request_key)
  REFERENCES housing_live_item_receipts(request_key) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_instance_shape CHECK(item_template_id>0
  AND created_by_character_id>0 AND revision>0 AND packed<=1)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_crate_locations (
 crate_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 location_kind TINYINT UNSIGNED NOT NULL,
 character_id INT UNSIGNED NULL,
 slot_id INT UNSIGNED NULL,
 custody_id BIGINT UNSIGNED NULL UNIQUE,
 UNIQUE KEY housing_crate_inventory_location(character_id,slot_id),
 CONSTRAINT housing_crate_location_identity FOREIGN KEY(crate_id)
  REFERENCES housing_crate_instances(crate_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_location_custody FOREIGN KEY(custody_id)
  REFERENCES housing_live_item_custody(custody_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_location_shape CHECK(
  (location_kind=1 AND character_id IS NOT NULL AND character_id>0
   AND slot_id IS NOT NULL AND custody_id IS NULL) OR
  (location_kind=2 AND character_id IS NULL AND slot_id IS NULL
   AND custody_id IS NOT NULL AND custody_id>0))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_crate_estates (
 crate_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 native_estate_id INT UNSIGNED NOT NULL UNIQUE,
 parent_native_estate_id INT UNSIGNED NOT NULL,
 ownership_estate_id INT UNSIGNED NOT NULL,
 definition_id INT UNSIGNED NOT NULL,
 catalog_key VARBINARY(96) NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 CONSTRAINT housing_crate_estate_identity FOREIGN KEY(crate_id)
  REFERENCES housing_crate_instances(crate_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_estate_native FOREIGN KEY(native_estate_id)
  REFERENCES housing_native_estates(native_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_estate_parent FOREIGN KEY(parent_native_estate_id)
  REFERENCES housing_native_estates(native_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_estate_owner FOREIGN KEY(ownership_estate_id)
  REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_estate_catalog FOREIGN KEY(catalog_key)
  REFERENCES housing_catalogs(catalog_key) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_estate_shape CHECK(native_estate_id>0
  AND native_estate_id<4294967295 AND parent_native_estate_id>0
  AND parent_native_estate_id<4294967295
  AND native_estate_id<>parent_native_estate_id AND ownership_estate_id>0
  AND definition_id>0 AND catalog_revision>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_crate_members (
 custody_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 crate_id BIGINT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL DEFAULT 1,
 origin_native_estate_id INT UNSIGNED NOT NULL,
 origin_house_id BIGINT UNSIGNED NULL,
 origin_pose_blob BINARY(28) NOT NULL,
 origin_stowed TINYINT UNSIGNED NOT NULL,
 CONSTRAINT housing_crate_member_custody FOREIGN KEY(custody_id)
  REFERENCES housing_live_item_custody(custody_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_member_container FOREIGN KEY(crate_id)
  REFERENCES housing_crate_estates(crate_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_member_origin FOREIGN KEY(origin_native_estate_id)
  REFERENCES housing_native_estates(native_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_member_house FOREIGN KEY(origin_house_id)
  REFERENCES housing_house_interiors(house_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_member_shape CHECK(custody_id>0 AND crate_id>0
  AND revision>0 AND origin_native_estate_id>0
  AND origin_native_estate_id<4294967295 AND origin_stowed<=1)
) ENGINE=InnoDB;
-- Services must join mint/location/native binding atomically and reject nested
-- cycles, self-containment, capacity violations and cross-scope membership.
-- No generic inventory/custody path may retire or move a crate with members.
