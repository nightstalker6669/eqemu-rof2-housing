-- Offline, economic-only catalog changes. No automatic debit or old rent reset.
CREATE TABLE IF NOT EXISTS housing_catalog_pricing_upgrades (
 catalog_key VARBINARY(96) NOT NULL,
 from_revision BIGINT UNSIGNED NOT NULL,
 to_revision BIGINT UNSIGNED NOT NULL,
 before_definitions MEDIUMBLOB NOT NULL,
 after_definitions MEDIUMBLOB NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 PRIMARY KEY(catalog_key,from_revision),
 CHECK(from_revision>0 AND to_revision=from_revision+1)
) ENGINE=InnoDB;
