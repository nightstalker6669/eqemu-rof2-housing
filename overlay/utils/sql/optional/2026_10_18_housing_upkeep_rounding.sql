-- Optional explicit compatibility rounding, not recovered original phase.
-- Integral daily tariffs keep existing behavior without any policy row.
-- No existing property is enrolled and no price or wallet policy is seeded.
CREATE TABLE IF NOT EXISTS housing_upkeep_rounding_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 mode TINYINT UNSIGNED NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_upkeep_rounding_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_upkeep_rounding_shape CHECK(native_plot_id>0 AND native_plot_id<4294967295 AND catalog_revision>0 AND mode=1 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;

-- Mode1: first assessment one hour after purchase; copper-only tariffs;
-- reviewed UTC fractional-copper phase. No alternate currency or arrears policy.
-- Existing independent purchase authorization must also admit the same catalog.
CREATE TABLE IF NOT EXISTS housing_upkeep_enrollment_policy (
 catalog_key VARBINARY(96) NOT NULL,
 definition_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 definitions_sha256 CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 regions_sha256 CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 mode TINYINT UNSIGNED NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 PRIMARY KEY(catalog_key,definition_id),
 CONSTRAINT housing_upkeep_enrollment_shape CHECK(definition_id>0 AND catalog_revision>0 AND mode=1 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;

-- Original 2010 housing guide specifies 90 days since login for personal plots.
-- This initial disposition supports ONLY empty properties with zero escrow.
-- Occupied property/crates, vaults and refunds are deferred, never discarded.
CREATE TABLE IF NOT EXISTS housing_inactivity_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 inactive_seconds INT UNSIGNED NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_inactivity_policy_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_inactivity_policy_shape CHECK(native_plot_id>0 AND native_plot_id<4294967295 AND catalog_revision>0 AND inactive_seconds=7776000 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_inactivity_retirements (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 plot_id INT UNSIGNED NOT NULL,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 last_login INT UNSIGNED NOT NULL,
 retired_epoch INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_inactivity_receipt_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_inactivity_receipt_shape CHECK(native_plot_id>0 AND plot_id>0 AND character_id>0 AND account_id>0 AND last_login>0 AND retired_epoch>=last_login AND retired_epoch-last_login>=7776000 AND catalog_revision>0 AND CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
