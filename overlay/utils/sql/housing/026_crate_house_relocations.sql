-- Optional immutable per-house effects of a real parent crate Store.
-- No seeds or runtime enablement. Requires007/016/023.
CREATE TABLE IF NOT EXISTS housing_crate_house_relocations (
 request_key VARBINARY(96) NOT NULL,
 house_id BIGINT UNSIGNED NOT NULL,
 crate_id BIGINT UNSIGNED NOT NULL,
 before_revision BIGINT UNSIGNED NOT NULL,
 after_revision BIGINT UNSIGNED NOT NULL,
 before_parent_native_id INT UNSIGNED NOT NULL,
 after_parent_native_id INT UNSIGNED NOT NULL,
 interior_native_id INT UNSIGNED NOT NULL,
 interior_instance_id INT NOT NULL,
 PRIMARY KEY(request_key,house_id),
 CONSTRAINT housing_crate_house_move_parent FOREIGN KEY(request_key)
  REFERENCES housing_live_item_receipts(request_key) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_house_move_identity FOREIGN KEY(house_id)
  REFERENCES housing_house_instances(house_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_house_move_container FOREIGN KEY(crate_id)
  REFERENCES housing_crate_instances(crate_id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_house_move_shape CHECK(before_revision>0
  AND after_revision=before_revision+1 AND before_parent_native_id>0
  AND after_parent_native_id>0 AND before_parent_native_id<>after_parent_native_id
  AND interior_native_id>0 AND interior_instance_id>0 AND interior_instance_id<=32000)
) ENGINE=InnoDB;
