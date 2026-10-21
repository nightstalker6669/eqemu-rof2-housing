-- Optional capacity-only room policy history. Requires019. No content seeds.
-- Immutable room profile bytes/revisions and historical furniture tags remain
-- untouched. Each effective policy pins its exact base and predecessor hashes.
-- Apply through the checked maintenance API with world/all zones stopped.
CREATE TABLE IF NOT EXISTS housing_house_capacity_policies (
 house_id BIGINT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL,
 base_revision BIGINT UNSIGNED NOT NULL,
 base_manifest_sha256 BINARY(32) NOT NULL,
 base_definitions_sha256 BINARY(32) NOT NULL,
 base_regions_sha256 BINARY(32) NOT NULL,
 previous_sha256 BINARY(32) NOT NULL,
 source_reference VARBINARY(1024) NOT NULL,
 source_sha256 BINARY(32) NOT NULL,
 definitions_blob BLOB NOT NULL,
 policy_sha256 BINARY(32) NOT NULL,
 PRIMARY KEY(house_id,revision),
 CONSTRAINT housing_capacity_base FOREIGN KEY(house_id)
  REFERENCES housing_house_room_profiles(house_id) ON DELETE RESTRICT,
 CONSTRAINT housing_capacity_shape CHECK(house_id>0 AND revision>0
  AND base_revision>0 AND OCTET_LENGTH(source_reference)>0
  AND OCTET_LENGTH(definitions_blob)>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_house_capacity_heads (
 house_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 revision BIGINT UNSIGNED NOT NULL,
 CONSTRAINT housing_capacity_head FOREIGN KEY(house_id,revision)
  REFERENCES housing_house_capacity_policies(house_id,revision) ON DELETE RESTRICT,
 CONSTRAINT housing_capacity_head_shape CHECK(house_id>0 AND revision>0)
) ENGINE=InnoDB;

-- Exact immutable template/base grants cover future legacy-profile allocations.
-- Registration is maintenance-only; only newly inserted EMPTY room profiles
-- may consume a grant in their existing allocation transaction.
CREATE TABLE IF NOT EXISTS housing_house_capacity_templates (
 item_template_id INT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL,
 base_revision BIGINT UNSIGNED NOT NULL,
 base_manifest_sha256 BINARY(32) NOT NULL,
 base_definitions_sha256 BINARY(32) NOT NULL,
 base_regions_sha256 BINARY(32) NOT NULL,
 source_reference VARBINARY(1024) NOT NULL,
 source_sha256 BINARY(32) NOT NULL,
 definitions_blob BLOB NOT NULL,
 policy_sha256 BINARY(32) NOT NULL,
 PRIMARY KEY(item_template_id,base_manifest_sha256,revision),
 CONSTRAINT housing_capacity_template_shape CHECK(item_template_id>0
  AND revision>0 AND base_revision>0 AND OCTET_LENGTH(source_reference)>0
  AND OCTET_LENGTH(definitions_blob)>0)
) ENGINE=InnoDB;
