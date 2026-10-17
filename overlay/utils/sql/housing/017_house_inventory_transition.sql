-- Optional dormant ordinary HOUSE inventory permutation journal. No seeds.
-- Uses existing009 reservation identity; NOT another lease or007 Store/Return.
CREATE TABLE IF NOT EXISTS housing_house_inventory_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 inventory_epoch BIGINT UNSIGNED NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,
 outcome_blob MEDIUMBLOB NOT NULL,
 applied TINYINT UNSIGNED NOT NULL DEFAULT 0,
 CONSTRAINT housing_house_inventory_receipt_state CHECK(inventory_epoch>0 AND applied<=1 AND LEFT(request_key,9)=_binary'houseinv:')
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_house_inventory_pending (
 character_id INT UNSIGNED NOT NULL PRIMARY KEY,
 inventory_epoch BIGINT UNSIGNED NOT NULL,
 request_key VARBINARY(96) NOT NULL UNIQUE,
 CONSTRAINT housing_house_inventory_pending_receipt FOREIGN KEY(request_key)
  REFERENCES housing_house_inventory_receipts(request_key) ON DELETE RESTRICT,
 CONSTRAINT housing_house_inventory_pending_epoch CHECK(inventory_epoch>0)
) ENGINE=InnoDB;
-- Existing016 house pending keys join this journal in the SAME transaction.
-- Successful ACK clears actor pending + every exact house pending + marks its
-- receipt.applied together. No automatic takeover/recovery/independent mint.
-- Required cross-service optional guard hooks remain INACTIVE until reviewed.
