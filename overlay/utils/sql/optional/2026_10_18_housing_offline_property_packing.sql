-- Optional extension of existing reviewed offline personal-property delivery.
-- Only an existing small moving crate; no generated container or policy seeds.
-- Mode1 packs supported loose yard items (including one furnished house),
-- preserving the original payloads/poses and origin stowed flags in one TX.
CREATE TABLE IF NOT EXISTS housing_inactivity_packing_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 mode TINYINT UNSIGNED NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_inactivity_packing_policy_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_inactivity_packing_policy_shape CHECK(native_plot_id>0 AND native_plot_id<4294967295 AND catalog_revision>0 AND mode=1 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_inactivity_packings (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 crate_id BIGINT UNSIGNED NOT NULL,
 member_count INT UNSIGNED NOT NULL,
 before_allocation BLOB NOT NULL,
 after_allocation BLOB NOT NULL,
 before_blob MEDIUMBLOB NOT NULL,
 after_blob MEDIUMBLOB NOT NULL,
 before_sha256 BINARY(32) NOT NULL,
 after_sha256 BINARY(32) NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_inactivity_packing_receipt FOREIGN KEY(estate_id) REFERENCES housing_inactivity_deliveries(estate_id) ON DELETE RESTRICT,
 CONSTRAINT housing_inactivity_packing_shape CHECK(crate_id>0 AND member_count BETWEEN 1 AND 1000 AND OCTET_LENGTH(before_allocation)>0 AND OCTET_LENGTH(after_allocation)>0 AND OCTET_LENGTH(before_blob)>0 AND OCTET_LENGTH(after_blob)>0 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
