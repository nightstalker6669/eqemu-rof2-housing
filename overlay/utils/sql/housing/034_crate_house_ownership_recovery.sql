-- Requires026 and the matching generation-aware crate reader/writer.
-- Stop housing writers and back up the ledger before this one-time ALTER.
-- Legacy cross-plot history remains valid with both ownership IDs NULL.
-- No seeds, pricing changes or automatic lifecycle activation.
ALTER TABLE housing_crate_house_relocations
 ADD COLUMN before_ownership_estate_id INT UNSIGNED NULL,
 ADD COLUMN after_ownership_estate_id INT UNSIGNED NULL,
 DROP CONSTRAINT housing_crate_house_move_shape,
 ADD CONSTRAINT housing_crate_house_move_shape CHECK(before_revision>0
  AND after_revision=before_revision+1 AND before_parent_native_id>0
  AND after_parent_native_id>0
  AND ((before_ownership_estate_id IS NULL AND after_ownership_estate_id IS NULL
        AND before_parent_native_id<>after_parent_native_id)
    OR (before_ownership_estate_id IS NOT NULL AND after_ownership_estate_id IS NOT NULL
        AND before_ownership_estate_id>0 AND after_ownership_estate_id>0
        AND (before_parent_native_id<>after_parent_native_id
             OR before_ownership_estate_id<>after_ownership_estate_id)))
  AND interior_native_id>0 AND interior_instance_id>0 AND interior_instance_id<=32000);
