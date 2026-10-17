-- Optional REAL inventory-row custody foundation. Runtime integration stays disabled.
-- Current Client save/mutation fence is a trusted adapter contract, not implemented here.
CREATE TABLE IF NOT EXISTS housing_live_item_leases (
 character_id INT UNSIGNED NOT NULL PRIMARY KEY,
 account_id INT UNSIGNED NOT NULL,
 epoch BIGINT UNSIGNED NOT NULL,
 session_key VARBINARY(96) NOT NULL,
 token BINARY(32) NOT NULL,
 active TINYINT UNSIGNED NOT NULL DEFAULT 1,
 pending_key VARBINARY(96) NULL,
 CONSTRAINT housing_live_lease_state CHECK (epoch>0 AND active<=1 AND (active=1 OR pending_key IS NULL))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_live_item_custody (
 custody_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
 estate_id INT UNSIGNED NOT NULL,
 owner_character_id INT UNSIGNED NOT NULL,
 schema_descriptor VARBINARY(768) NOT NULL,
 row_blob MEDIUMBLOB NOT NULL,
 catalog_blob VARBINARY(16) NOT NULL,
 CONSTRAINT housing_live_custody_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_live_item_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,
 session_key VARBINARY(96) NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,
 completed TINYINT UNSIGNED NOT NULL DEFAULT 0,
 applied TINYINT UNSIGNED NOT NULL DEFAULT 0,
 after_blob MEDIUMBLOB NULL,
 created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 CONSTRAINT housing_live_receipt_state CHECK (completed<=1 AND applied<=1 AND (applied=0 OR completed=1) AND (completed=0 OR after_blob IS NOT NULL))
) ENGINE=InnoDB;
-- Receipt/journalpending/custody/source delete or destination insert share ONE commit.
-- No pending record commits separately, no auto lease timeout, no compensating restore.
-- No catalog/runtime seeds. Receipts retained independently; closure/recovery explicit.
