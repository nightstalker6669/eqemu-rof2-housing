-- Optional native parcel overflow for reviewed atomic retirement batches.
-- No recipient, policy or historical retirement is activated by installation.
-- Sidecars survive native parcel deletion: a claimed ID can never fall through
-- to template-only parcel retrieval. All source bytes remain authoritative.
CREATE TABLE IF NOT EXISTS housing_return_parcels (
 parcel_id INT UNSIGNED NOT NULL PRIMARY KEY,
 estate_id INT UNSIGNED NOT NULL,
 owner_kind TINYINT UNSIGNED NOT NULL,
 estate_owner_id INT UNSIGNED NOT NULL,
 owner_character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 source_custody_id BIGINT UNSIGNED NULL,
 crate_id BIGINT UNSIGNED NULL,
 generated TINYINT UNSIGNED NOT NULL DEFAULT 0,
 native_slot INT UNSIGNED NOT NULL,
 native_quantity INT UNSIGNED NOT NULL,
 sent_epoch BIGINT UNSIGNED NOT NULL,
 schema_descriptor VARBINARY(768) NOT NULL,
 row_blob MEDIUMBLOB NOT NULL,
 row_sha256 BINARY(32) NOT NULL,
 source_state MEDIUMBLOB NOT NULL,
 state_sha256 BINARY(32) NOT NULL,
 claim_key VARBINARY(96) NULL UNIQUE,
 claim_slot INT UNSIGNED NULL,
 claim_row MEDIUMBLOB NULL,
 claim_sha256 BINARY(32) NULL,
 lease_epoch BIGINT UNSIGNED NULL,
 session_key VARBINARY(96) NULL,
 lease_token BINARY(32) NULL,
 claimed_epoch BIGINT UNSIGNED NULL,
 UNIQUE KEY housing_return_parcel_source(estate_id,source_custody_id),
 KEY housing_return_parcel_recipient(owner_character_id),
 CONSTRAINT housing_return_parcel_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_return_parcel_crate FOREIGN KEY(crate_id) REFERENCES housing_crate_instances(crate_id) ON DELETE RESTRICT,
 CONSTRAINT housing_return_parcel_shape CHECK(parcel_id>0 AND estate_id>0 AND owner_kind IN(1,3) AND estate_owner_id>0 AND owner_character_id>0 AND account_id>0 AND native_slot>0 AND native_quantity>0 AND sent_epoch>0 AND generated<=1 AND ((generated=0 AND source_custody_id IS NOT NULL AND source_custody_id>0) OR (generated=1 AND source_custody_id IS NULL AND crate_id IS NOT NULL AND crate_id>0)) AND OCTET_LENGTH(row_blob)>0 AND OCTET_LENGTH(source_state)>0 AND ((claim_key IS NULL AND claim_slot IS NULL AND claim_row IS NULL AND claim_sha256 IS NULL AND lease_epoch IS NULL AND session_key IS NULL AND lease_token IS NULL AND claimed_epoch IS NULL) OR (claim_key IS NOT NULL AND OCTET_LENGTH(claim_key)>0 AND claim_slot BETWEEN 23 AND 30 AND claim_row IS NOT NULL AND OCTET_LENGTH(claim_row)>0 AND claim_sha256 IS NOT NULL AND lease_epoch>0 AND session_key IS NOT NULL AND OCTET_LENGTH(session_key)>0 AND lease_token IS NOT NULL AND claimed_epoch>0)))
) ENGINE=InnoDB;
ALTER TABLE housing_crate_locations ADD COLUMN IF NOT EXISTS parcel_id INT UNSIGNED NULL;
ALTER TABLE housing_crate_locations ADD UNIQUE INDEX IF NOT EXISTS housing_crate_parcel_location(parcel_id);
ALTER TABLE housing_crate_locations DROP CONSTRAINT IF EXISTS housing_crate_location_shape;
ALTER TABLE housing_crate_locations ADD CONSTRAINT housing_crate_location_shape CHECK(
 (location_kind=1 AND character_id IS NOT NULL AND character_id>0 AND slot_id IS NOT NULL AND custody_id IS NULL AND parcel_id IS NULL) OR
 (location_kind=2 AND character_id IS NULL AND slot_id IS NULL AND custody_id IS NOT NULL AND custody_id>0 AND parcel_id IS NULL) OR
 (location_kind=3 AND character_id IS NULL AND slot_id IS NULL AND custody_id IS NULL AND parcel_id IS NOT NULL AND parcel_id>0));
ALTER TABLE housing_offline_return_batch_policy ADD COLUMN IF NOT EXISTS allow_parcel_overflow TINYINT UNSIGNED NOT NULL DEFAULT 0;
-- Extend the route set without changing existing immutable route encodings.
ALTER TABLE housing_return_settlement_items DROP CONSTRAINT IF EXISTS housing_return_item_shape;
ALTER TABLE housing_return_settlement_items ADD CONSTRAINT housing_return_item_shape CHECK(custody_id>0 AND recipient_character_id>0 AND recipient_account_id>0 AND route IN(1,2,3,4));
-- The sidecar is a durable tombstone; references must outlive native deletion.
SET @housing_parcel_fk_sql = (SELECT IF(COUNT(*)=0,
 'ALTER TABLE housing_crate_locations ADD CONSTRAINT housing_crate_location_parcel FOREIGN KEY(parcel_id) REFERENCES housing_return_parcels(parcel_id) ON DELETE RESTRICT',
 'SELECT 1') FROM information_schema.table_constraints
 WHERE constraint_schema=DATABASE() AND table_name='housing_crate_locations'
 AND constraint_name='housing_crate_location_parcel');
PREPARE housing_parcel_fk_stmt FROM @housing_parcel_fk_sql;
EXECUTE housing_parcel_fk_stmt;
DEALLOCATE PREPARE housing_parcel_fk_stmt;
