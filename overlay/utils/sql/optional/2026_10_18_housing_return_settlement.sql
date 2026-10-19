-- Optional immutable conservation receipts for reviewed atomic offline returns.
-- No existing return is certified, and no guild is deleted by this migration.
-- Child witnesses are inserted first inside the caller transaction; the sealed
-- parent manifest is written LAST. Child rows alone never authorize dissolution.
CREATE TABLE IF NOT EXISTS housing_return_settlement_items (
 estate_id INT UNSIGNED NOT NULL,
 custody_id BIGINT UNSIGNED NOT NULL,
 route TINYINT UNSIGNED NOT NULL,
 recipient_character_id INT UNSIGNED NOT NULL,
 recipient_account_id INT UNSIGNED NOT NULL,
 destination_slot INT UNSIGNED NOT NULL DEFAULT 0,
 crate_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
 house_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
 source_blob MEDIUMBLOB NOT NULL,
 source_sha256 BINARY(32) NOT NULL,
 delivery_blob MEDIUMBLOB NOT NULL,
 delivery_sha256 BINARY(32) NOT NULL,
 PRIMARY KEY(estate_id,custody_id),
 CONSTRAINT housing_return_item_shape CHECK(custody_id>0 AND recipient_character_id>0 AND recipient_account_id>0 AND route IN(1,2,3)),
 FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_return_settlement_crates (
 estate_id INT UNSIGNED NOT NULL,
 crate_id BIGINT UNSIGNED NOT NULL,
 recipient_character_id INT UNSIGNED NOT NULL,
 recipient_account_id INT UNSIGNED NOT NULL,
 identity_blob MEDIUMBLOB NOT NULL,
 identity_sha256 BINARY(32) NOT NULL,
 PRIMARY KEY(estate_id,crate_id),
 CONSTRAINT housing_return_crate_shape CHECK(crate_id>0 AND recipient_character_id>0 AND recipient_account_id>0),
 FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_return_settlements (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 owner_kind TINYINT UNSIGNED NOT NULL,
 owner_id INT UNSIGNED NOT NULL,
 catalog_key VARBINARY(96) NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 escrow_copper BIGINT UNSIGNED NOT NULL,
 refund_recipient_character_id INT UNSIGNED NOT NULL,
 refund_recipient_account_id INT UNSIGNED NOT NULL,
 source_count INT UNSIGNED NOT NULL,
 crate_count INT UNSIGNED NOT NULL,
 manifest_sha256 BINARY(32) NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 CONSTRAINT housing_return_settlement_shape CHECK(estate_id>0 AND native_plot_id>0 AND owner_kind IN(1,3) AND owner_id>0 AND catalog_revision>0 AND source_count<=10000 AND crate_count<=1000 AND CHAR_LENGTH(review_reference)>0),
 FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
