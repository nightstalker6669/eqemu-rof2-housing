-- Optional estate disposition. The real custody row, native item ID, creator,
-- room scope and last pose are retained while an item is in storage.
-- Requires007/010. No content seeds or runtime enablement.
CREATE TABLE IF NOT EXISTS housing_live_item_storage (
 custody_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 revision BIGINT UNSIGNED NOT NULL,
 stowed TINYINT UNSIGNED NOT NULL,
 native_estate_id INT UNSIGNED NOT NULL,
 CONSTRAINT housing_storage_custody FOREIGN KEY(custody_id)
  REFERENCES housing_live_item_custody(custody_id) ON DELETE RESTRICT,
 CONSTRAINT housing_storage_estate FOREIGN KEY(native_estate_id)
  REFERENCES housing_native_estates(native_id) ON DELETE RESTRICT,
 CONSTRAINT housing_storage_shape CHECK(custody_id>0 AND revision>0
  AND stowed<=1 AND native_estate_id>0 AND native_estate_id<4294967295)
) ENGINE=InnoDB;
-- Historical outcomes survive inventory return, so custody is not a journal FK.
CREATE TABLE IF NOT EXISTS housing_live_storage_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 binding_blob BLOB NOT NULL,
 custody_id BIGINT UNSIGNED NOT NULL,
 before_revision BIGINT UNSIGNED NOT NULL,
 after_revision BIGINT UNSIGNED NOT NULL,
 CONSTRAINT housing_storage_receipt_shape CHECK(character_id>0 AND account_id>0
  AND custody_id>0 AND after_revision>before_revision AND OCTET_LENGTH(binding_blob)>0)
) ENGINE=InnoDB;
