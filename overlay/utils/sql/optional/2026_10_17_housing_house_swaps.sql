-- Optional property-only house replacement journal. No runtime enablement.
-- Complete immutable outcome and all custody/property mutations share one TX.
CREATE TABLE IF NOT EXISTS housing_house_swap_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 format_version INT UNSIGNED NOT NULL,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 binding_blob BLOB NOT NULL,
 outcome_blob MEDIUMBLOB NOT NULL,
 CONSTRAINT housing_house_swap_shape CHECK(format_version=1 AND character_id>0 AND account_id>0
  AND OCTET_LENGTH(binding_blob)>0 AND OCTET_LENGTH(outcome_blob)>0)
) ENGINE=InnoDB;
