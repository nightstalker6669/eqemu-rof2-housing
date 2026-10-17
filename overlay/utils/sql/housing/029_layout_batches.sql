-- Immutable property-only layout outcome. Same transaction as all member writes.
-- Inventory application/recovery is a separate required integration before
-- inventory-bearing layout activation; this table alone supplies no ACK proof.
CREATE TABLE IF NOT EXISTS housing_layout_batches (
 request_key VARBINARY(96) NOT NULL,
 format_version SMALLINT UNSIGNED NOT NULL,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,
 session_key VARBINARY(96) NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,
 outcome_blob MEDIUMBLOB NOT NULL,
 PRIMARY KEY (request_key)
) ENGINE=InnoDB;
