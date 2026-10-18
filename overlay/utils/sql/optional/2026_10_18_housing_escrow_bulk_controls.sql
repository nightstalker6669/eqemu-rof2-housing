-- Explicit compatibility policy: carried-only day/week funding and Withdraw All.
-- Native May 2013 requests contain no funding account. This is not a claim of
-- recovered Live server debit selection. No policy is enabled by installing SQL.
CREATE TABLE IF NOT EXISTS housing_escrow_bulk_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 mode TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '1 carried-only normalized copper; never bank/shared-bank/cursor',
 enabled TINYINT UNSIGNED NOT NULL DEFAULT 0,
 review_reference VARCHAR(96) NOT NULL
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_escrow_bulk_death_reconciliations (
 request_key VARCHAR(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 inventory_epoch BIGINT UNSIGNED NOT NULL,
 inventory_token BINARY(32) NOT NULL,
 process_key VARCHAR(96) NOT NULL,
 outcome_blob MEDIUMBLOB NOT NULL,
 reconciled_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- Optional future-purchase opt-in. Existing fee review rows remain rent-only.
-- Absence of this column is supported by the enrollment reader as false.
ALTER TABLE IF EXISTS housing_upkeep_enrollment_policy
 ADD COLUMN IF NOT EXISTS allow_bulk_carried_copper TINYINT UNSIGNED NOT NULL DEFAULT 0;
