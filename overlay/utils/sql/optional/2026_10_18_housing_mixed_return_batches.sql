-- Apply after guild inactivity returns, direct returns and return settlement.
-- Explicit, generation/catalogue-bound all-recipient disposition. No seed.
CREATE TABLE IF NOT EXISTS housing_offline_return_batch_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 definitions_sha256 BINARY(32) NOT NULL,
 regions_sha256 BINARY(32) NOT NULL,
 crate_manifest_sha256 BINARY(32) NOT NULL,
 max_recipients INT UNSIGNED NOT NULL,
 max_crates INT UNSIGNED NOT NULL,
 items_per_crate INT UNSIGNED NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_return_batch_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_return_batch_shape CHECK(native_plot_id>0 AND native_plot_id<4294967295 AND catalog_revision>0 AND max_recipients BETWEEN 1 AND 32 AND max_crates BETWEEN 1 AND 32 AND items_per_crate BETWEEN 1 AND 1000 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_offline_return_finance (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 recipient_character_id INT UNSIGNED NOT NULL,
 recipient_account_id INT UNSIGNED NOT NULL,
 refund_copper BIGINT UNSIGNED NOT NULL,
 currency_before BINARY(64) NOT NULL,
 currency_after BINARY(64) NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_return_finance_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_return_finance_shape CHECK(recipient_character_id>0 AND recipient_account_id>0 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
-- Multiple immutable recipient/ordinal origins may share one retired estate.
-- Keep an estate index supporting the existing FK before removing uniqueness.
ALTER TABLE housing_crate_creation_origins ADD INDEX IF NOT EXISTS housing_crate_origin_estate(offline_estate_id);
ALTER TABLE housing_crate_creation_origins DROP INDEX IF EXISTS offline_estate_id;
ALTER TABLE housing_crate_creation_origins ADD COLUMN IF NOT EXISTS offline_return_ordinal INT UNSIGNED NULL;
ALTER TABLE housing_crate_creation_origins DROP CONSTRAINT IF EXISTS housing_crate_origin_shape;
ALTER TABLE housing_crate_creation_origins ADD CONSTRAINT housing_crate_origin_shape CHECK(
 (kind=1 AND live_request_key IS NOT NULL AND live_request_key=request_key AND offline_estate_id IS NULL AND offline_character_id IS NULL AND offline_return_ordinal IS NULL)
 OR (kind=2 AND live_request_key IS NULL AND offline_estate_id IS NOT NULL AND offline_character_id IS NULL AND offline_return_ordinal IS NULL AND request_key=CONCAT('offline-inactivity:',offline_estate_id))
 OR (kind=3 AND live_request_key IS NULL AND offline_estate_id IS NOT NULL AND offline_character_id IS NOT NULL AND offline_character_id>0 AND offline_return_ordinal IS NULL AND request_key=CONCAT('offline-guild-inactivity:',offline_estate_id))
 OR (kind=4 AND live_request_key IS NULL AND offline_estate_id IS NOT NULL AND offline_character_id IS NOT NULL AND offline_character_id>0 AND offline_return_ordinal IS NOT NULL AND offline_return_ordinal BETWEEN 1 AND 32 AND request_key=CONCAT('offline-return:',offline_estate_id,':',offline_character_id,':',offline_return_ordinal))
);
