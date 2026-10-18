-- Prerequisites: offline_property_delivery and offline_property_packing.
-- Source kind1 retains all existing pickup/refund semantics. Kind2 is a newly
-- allocated carried crate, without fabricated placed custody or Client lease.
ALTER TABLE housing_inactivity_deliveries ADD COLUMN IF NOT EXISTS source_kind TINYINT UNSIGNED NOT NULL DEFAULT 1;
ALTER TABLE housing_inactivity_deliveries DROP CONSTRAINT IF EXISTS housing_inactivity_delivery_shape;
ALTER TABLE housing_inactivity_deliveries ADD CONSTRAINT housing_inactivity_delivery_shape CHECK (
 character_id>0 AND CHAR_LENGTH(review_reference)>0 AND (
 (source_kind=1 AND ((crate_id IS NULL AND source_custody_id IS NULL AND destination_slot IS NULL AND OCTET_LENGTH(root_before)=0 AND OCTET_LENGTH(root_after)=0) OR (crate_id IS NOT NULL AND source_custody_id IS NOT NULL AND destination_slot IS NOT NULL AND crate_id>0 AND source_custody_id>0 AND destination_slot BETWEEN 23 AND 30 AND OCTET_LENGTH(root_before)>0 AND OCTET_LENGTH(root_after)>0))) OR
 (source_kind=2 AND crate_id IS NOT NULL AND crate_id>0 AND source_custody_id IS NULL AND destination_slot IS NOT NULL AND destination_slot BETWEEN 23 AND 30 AND OCTET_LENGTH(root_before)=0 AND OCTET_LENGTH(root_after)>0))
);
CREATE TABLE IF NOT EXISTS housing_inactivity_crate_creation_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 mode TINYINT UNSIGNED NOT NULL,
 crate_manifest_sha256 BINARY(32) NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_inactivity_creation_policy_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_inactivity_creation_policy_shape CHECK(native_plot_id>0 AND native_plot_id<4294967295 AND catalog_revision>0 AND mode=1 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_inactivity_crate_creations (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 crate_id BIGINT UNSIGNED NOT NULL UNIQUE,
 native_estate_id INT UNSIGNED NOT NULL UNIQUE,
 inventory_row BLOB NOT NULL,
 initial_allocation BLOB NOT NULL,
 delivered_allocation BLOB NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_inactivity_created_delivery FOREIGN KEY(estate_id) REFERENCES housing_inactivity_deliveries(estate_id) ON DELETE RESTRICT,
 CONSTRAINT housing_inactivity_created_crate FOREIGN KEY(crate_id) REFERENCES housing_crate_instances(crate_id) ON DELETE RESTRICT,
 CONSTRAINT housing_inactivity_created_native FOREIGN KEY(native_estate_id) REFERENCES housing_native_estates(native_id) ON DELETE RESTRICT,
 CONSTRAINT housing_inactivity_created_shape CHECK(crate_id>0 AND native_estate_id>0 AND OCTET_LENGTH(inventory_row)>0 AND OCTET_LENGTH(initial_allocation)>0 AND OCTET_LENGTH(delivered_allocation)>0 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
