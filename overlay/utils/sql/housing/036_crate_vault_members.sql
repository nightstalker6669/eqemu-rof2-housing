-- Additive, unseeded typed list-only membership. Never substitutes dummy poses
-- into housing_crate_members; custody and original vault markers remain intact.
CREATE TABLE IF NOT EXISTS housing_crate_vault_members (
 custody_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 crate_id BIGINT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL,
 origin_native_estate_id INT UNSIGNED NOT NULL,
 origin_house_id BIGINT UNSIGNED NULL,
 origin_realm_estate_id INT UNSIGNED NOT NULL,
 vault_revision BIGINT UNSIGNED NOT NULL,
 source_row MEDIUMBLOB NOT NULL,
 catalog_blob BLOB NOT NULL,
 schema_descriptor TEXT NOT NULL,
 KEY(crate_id,custody_id)
) ENGINE=InnoDB;
