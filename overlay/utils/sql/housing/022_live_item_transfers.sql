-- Optional immutable transfer journal. Requires010/018/019/020/021.
-- Custody and original creator remain unchanged; native placement identity is
-- replaced so historical reservations/receipts can remain byte-for-byte intact.
CREATE TABLE IF NOT EXISTS housing_live_transfer_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,
 custody_id BIGINT UNSIGNED NOT NULL,
 before_native_item_id INT UNSIGNED NOT NULL,
 after_native_item_id INT UNSIGNED NOT NULL,
 target_native_estate_id INT UNSIGNED NOT NULL,
 storage_revision BIGINT UNSIGNED NOT NULL,
 CONSTRAINT housing_transfer_shape CHECK(character_id>0 AND account_id>0 AND custody_id>0
  AND before_native_item_id>0 AND before_native_item_id<4294967295
  AND after_native_item_id>0 AND after_native_item_id<4294967295
  AND before_native_item_id<>after_native_item_id
  AND target_native_estate_id>0 AND target_native_estate_id<4294967295
  AND storage_revision>0)
) ENGINE=InnoDB;
