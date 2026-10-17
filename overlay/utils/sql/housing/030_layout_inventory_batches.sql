-- Internal inventory-member batch family. Native activation additionally needs
-- committed memory application/ACK, certified-death dispatch, mixed property
-- outcomes and full coordinator checks. Do not deploy this migration alone.
CREATE TABLE IF NOT EXISTS housing_layout_inventory_batches (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 format_version SMALLINT UNSIGNED NOT NULL,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,
 session_key VARBINARY(96) NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,
 outcome_blob MEDIUMBLOB NOT NULL,
 applied TINYINT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB;
