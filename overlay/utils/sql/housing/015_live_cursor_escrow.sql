-- Optional, disabled runtime foundation. No pricing/catalog/currency fixture seeds.
-- Requires005 integer balances and the existing007/009 trusted session fences.
CREATE TABLE IF NOT EXISTS housing_live_escrow_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,account_id INT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,session_key VARBINARY(96) NOT NULL,
 lease_token BINARY(32) NOT NULL,
 inventory_epoch BIGINT UNSIGNED NOT NULL,inventory_token BINARY(32) NOT NULL,
 process_key VARBINARY(96) NOT NULL,client_key VARBINARY(96) NOT NULL,
 estate_id INT UNSIGNED NOT NULL,native_plot_id INT UNSIGNED NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,outcome_blob MEDIUMBLOB NOT NULL,
 applied TINYINT UNSIGNED NOT NULL DEFAULT 0,
 created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 CONSTRAINT housing_live_escrow_applied CHECK(applied<=1 AND lease_epoch>0 AND inventory_epoch>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_live_escrow_pending (
 character_id INT UNSIGNED NOT NULL PRIMARY KEY,
 account_id INT UNSIGNED NOT NULL,
 inventory_epoch BIGINT UNSIGNED NOT NULL,inventory_token BINARY(32) NOT NULL,
 request_key VARBINARY(96) NOT NULL UNIQUE,
 CONSTRAINT housing_live_escrow_pending_receipt FOREIGN KEY(request_key)
  REFERENCES housing_live_escrow_receipts(request_key) ON DELETE RESTRICT,
 CONSTRAINT housing_live_escrow_pending_epoch CHECK(inventory_epoch>0)
) ENGINE=InnoDB;
-- Pending blocks new admission, final009release, and item-only crash recovery.
-- Only real AFTER memory proof clears it. Coin-specific certified-death recovery
-- is deliberately not implemented; old coin pending remains quarantined.
