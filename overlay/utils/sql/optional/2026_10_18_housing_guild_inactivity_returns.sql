-- Opt-in RoF2 guild inactivity disposition. Apply AFTER the personal offline
-- delivery, packing, crate creation and typed creation-origin migrations.
-- Original client text specifies 30 days without any guild-member login.
-- Current-roster activity, explicit current-leader recipient, one reviewed
-- small crate, and carried-copper refund are compatibility policies. No seed.
-- Recipient identity is checked/locked by the retirement service. No FK to
-- legacy character_data: normal upstream character saves use DELETE-like REPLACE.
CREATE TABLE IF NOT EXISTS housing_guild_inactivity_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 definitions_sha256 BINARY(32) NOT NULL,
 regions_sha256 BINARY(32) NOT NULL,
 recipient_character_id INT UNSIGNED NOT NULL,
 inactive_seconds INT UNSIGNED NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_guild_inactive_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_guild_inactive_shape CHECK(native_plot_id>0 AND native_plot_id<4294967295 AND catalog_revision>0 AND recipient_character_id>0 AND inactive_seconds=2592000 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_guild_inactivity_retirements (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 guild_id INT UNSIGNED NOT NULL,
 native_plot_id INT UNSIGNED NOT NULL,
 plot_id INT UNSIGNED NOT NULL,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 last_login INT UNSIGNED NOT NULL,
 retired_epoch INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_guild_inactive_receipt_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_guild_inactive_receipt_shape CHECK(guild_id>0 AND native_plot_id>0 AND plot_id>0 AND character_id>0 AND account_id>0 AND last_login>0 AND retired_epoch>=last_login AND retired_epoch-last_login>=2592000 AND catalog_revision>0 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
-- LIKE preserves the already-installed byte-level receipt shape and checks;
-- foreign keys are deliberately rebound to the guild disposition receipt.
CREATE TABLE IF NOT EXISTS housing_guild_inactivity_deliveries LIKE housing_inactivity_deliveries;
CREATE TABLE IF NOT EXISTS housing_guild_inactivity_packings LIKE housing_inactivity_packings;
CREATE TABLE IF NOT EXISTS housing_guild_inactivity_crate_creations LIKE housing_inactivity_crate_creations;
ALTER TABLE housing_guild_inactivity_deliveries DROP FOREIGN KEY IF EXISTS housing_guild_delivery_receipt;
ALTER TABLE housing_guild_inactivity_deliveries ADD CONSTRAINT housing_guild_delivery_receipt FOREIGN KEY(estate_id) REFERENCES housing_guild_inactivity_retirements(estate_id) ON DELETE RESTRICT;
ALTER TABLE housing_guild_inactivity_packings DROP FOREIGN KEY IF EXISTS housing_guild_packing_receipt;
ALTER TABLE housing_guild_inactivity_packings ADD CONSTRAINT housing_guild_packing_receipt FOREIGN KEY(estate_id) REFERENCES housing_guild_inactivity_retirements(estate_id) ON DELETE RESTRICT;
ALTER TABLE housing_guild_inactivity_crate_creations DROP FOREIGN KEY IF EXISTS housing_guild_creation_receipt;
ALTER TABLE housing_guild_inactivity_crate_creations ADD CONSTRAINT housing_guild_creation_receipt FOREIGN KEY(estate_id) REFERENCES housing_guild_inactivity_retirements(estate_id) ON DELETE RESTRICT;
ALTER TABLE housing_crate_creation_origins ADD COLUMN IF NOT EXISTS offline_character_id INT UNSIGNED NULL;
ALTER TABLE housing_crate_creation_origins DROP CONSTRAINT IF EXISTS housing_crate_origin_shape;
ALTER TABLE housing_crate_creation_origins ADD CONSTRAINT housing_crate_origin_shape CHECK(
 (kind=1 AND live_request_key IS NOT NULL AND live_request_key=request_key AND offline_estate_id IS NULL AND offline_character_id IS NULL)
 OR (kind=2 AND live_request_key IS NULL AND offline_estate_id IS NOT NULL AND offline_character_id IS NULL AND request_key=CONCAT('offline-inactivity:',offline_estate_id))
 OR (kind=3 AND live_request_key IS NULL AND offline_estate_id IS NOT NULL AND offline_character_id IS NOT NULL AND offline_character_id>0 AND request_key=CONCAT('offline-guild-inactivity:',offline_estate_id))
);
