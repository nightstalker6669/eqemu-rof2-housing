-- Explicit opt-in extension of the 90-day personal-property inactivity review.
-- Mode1 is an emulator compatibility policy: free ordinary saved-inventory slot,
-- and unused escrow credited as carried copper without denomination conversion.
-- This does not claim recovery of Live's offline wallet-selection rules.
-- No automatic activation; populated/loose/vault/foreign items defer safely.
CREATE TABLE IF NOT EXISTS housing_inactivity_delivery_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 mode TINYINT UNSIGNED NOT NULL,
 crate_manifest_sha256 BINARY(32) NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_inactivity_delivery_policy_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_inactivity_delivery_policy_shape CHECK(native_plot_id>0 AND native_plot_id<4294967295 AND catalog_revision>0 AND mode=1 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_inactivity_deliveries (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 crate_id BIGINT UNSIGNED NULL,
 source_custody_id BIGINT UNSIGNED NULL,
 destination_slot INT UNSIGNED NULL,
 refund_copper BIGINT UNSIGNED NOT NULL,
 currency_before BINARY(64) NOT NULL,
 currency_after BINARY(64) NOT NULL,
 root_before MEDIUMBLOB NOT NULL,
 root_after MEDIUMBLOB NOT NULL,
 cargo_blob MEDIUMBLOB NOT NULL,
 cargo_sha256 BINARY(32) NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_inactivity_delivery_receipt FOREIGN KEY(estate_id) REFERENCES housing_inactivity_retirements(estate_id) ON DELETE RESTRICT,
 CONSTRAINT housing_inactivity_delivery_shape CHECK(character_id>0 AND CHAR_LENGTH(review_reference)>0 AND ((crate_id IS NULL AND source_custody_id IS NULL AND destination_slot IS NULL AND OCTET_LENGTH(root_before)=0 AND OCTET_LENGTH(root_after)=0) OR (crate_id IS NOT NULL AND source_custody_id IS NOT NULL AND destination_slot IS NOT NULL AND crate_id>0 AND source_custody_id>0 AND destination_slot BETWEEN 23 AND 30 AND OCTET_LENGTH(root_before)>0 AND OCTET_LENGTH(root_after)>0)))
) ENGINE=InnoDB;
