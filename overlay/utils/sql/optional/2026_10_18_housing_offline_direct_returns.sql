-- Optional server-owned direct item returns. No rows or authority enabled here.
-- No FK to deleted custody: immutable before/after witness outlives disposition.
CREATE TABLE IF NOT EXISTS housing_offline_return_items (
 estate_id INT UNSIGNED NOT NULL,
 custody_id BIGINT UNSIGNED NOT NULL,
 owner_character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 destination_slot INT UNSIGNED NOT NULL,
 source_kind TINYINT UNSIGNED NOT NULL COMMENT '1 non-placeable vault; 2 ordinary static furniture; 3 existing empty exterior crate',
 native_item_id INT UNSIGNED NOT NULL,
 house_id BIGINT UNSIGNED NULL,
 crate_id BIGINT UNSIGNED NULL,
 allocation_before MEDIUMBLOB NULL,
 allocation_after MEDIUMBLOB NULL,
 row_before MEDIUMBLOB NOT NULL,
 row_after MEDIUMBLOB NOT NULL,
 state_before MEDIUMBLOB NOT NULL,
 before_sha256 BINARY(32) NOT NULL,
 after_sha256 BINARY(32) NOT NULL,
 state_sha256 BINARY(32) NOT NULL,
 returned_epoch BIGINT UNSIGNED NOT NULL,
 PRIMARY KEY(estate_id,custody_id),
 KEY housing_offline_return_recipient(owner_character_id),
 CONSTRAINT housing_offline_return_shape CHECK(estate_id>0 AND custody_id>0 AND owner_character_id>0 AND account_id>0 AND destination_slot BETWEEN 23 AND 30 AND source_kind IN(1,2,3) AND native_item_id>0 AND native_item_id<4294967295 AND (house_id IS NULL OR house_id>0) AND ((source_kind=3 AND crate_id IS NOT NULL AND crate_id>0 AND allocation_before IS NOT NULL AND allocation_after IS NOT NULL AND OCTET_LENGTH(allocation_before)>0 AND OCTET_LENGTH(allocation_after)>0 AND house_id IS NULL) OR (source_kind IN(1,2) AND crate_id IS NULL AND allocation_before IS NULL AND allocation_after IS NULL)) AND OCTET_LENGTH(row_before)>0 AND OCTET_LENGTH(row_after)>0 AND OCTET_LENGTH(state_before)>0),
 CONSTRAINT housing_offline_return_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
