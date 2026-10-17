-- Atomic member transfers; identity/pose/creator custody are preserved.
-- Requires023 and021. No runtime enablement or content seeds.
CREATE TABLE IF NOT EXISTS housing_crate_storage_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,
 crate_id BIGINT UNSIGNED NOT NULL,
 custody_id BIGINT UNSIGNED NOT NULL,
 crate_revision BIGINT UNSIGNED NOT NULL,
 storage_revision BIGINT UNSIGNED NOT NULL,
 CONSTRAINT housing_crate_storage_identity FOREIGN KEY(crate_id)
  REFERENCES housing_crate_instances(crate_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_storage_shape CHECK(character_id>0 AND account_id>0
  AND crate_id>0 AND custody_id>0 AND crate_revision>1 AND storage_revision>0)
) ENGINE=InnoDB;
