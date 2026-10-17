-- Requires housing_upkeep_cycles.sql; no enrollment, tariff, date or wallet seeds.
CREATE TABLE IF NOT EXISTS housing_upkeep_property_state (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 expired_since INT UNSIGNED NOT NULL,
 last_due_epoch INT UNSIGNED NOT NULL,
 CHECK(estate_id>0 AND native_plot_id>0 AND native_plot_id<4294967295 AND last_due_epoch>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_upkeep_property_cycles (
 estate_id INT UNSIGNED NOT NULL,
 due_epoch INT UNSIGNED NOT NULL,
 component_id INT UNSIGNED NOT NULL,
 hourly_copper BIGINT NOT NULL,
 charged_copper BIGINT NOT NULL,
 expired_before INT UNSIGNED NOT NULL,
 expired_after INT UNSIGNED NOT NULL,
 PRIMARY KEY(estate_id,due_epoch),
 FOREIGN KEY(estate_id,due_epoch) REFERENCES housing_upkeep_cycles(estate_id,due_epoch) ON DELETE RESTRICT,
 CHECK(hourly_copper>=0 AND charged_copper>=0 AND (charged_copper=0 OR charged_copper=hourly_copper))
) ENGINE=InnoDB;
-- Additional immutable billing witnesses do not confer placement/entry rights.
-- Existing real placement journals continue to own raw item/geometry authority.
CREATE TABLE IF NOT EXISTS housing_upkeep_cycle_scopes (
 estate_id INT UNSIGNED NOT NULL,
 due_epoch INT UNSIGNED NOT NULL,
 custody_id BIGINT UNSIGNED NOT NULL,
 subject_kind TINYINT UNSIGNED NOT NULL,
 native_estate_id INT UNSIGNED NOT NULL,
 house_id BIGINT UNSIGNED NOT NULL,
 profile_revision BIGINT UNSIGNED NOT NULL,
 proof_blob MEDIUMBLOB NOT NULL,
 PRIMARY KEY(estate_id,due_epoch,custody_id),
 FOREIGN KEY(estate_id,due_epoch,custody_id) REFERENCES housing_upkeep_cycle_items(estate_id,due_epoch,custody_id) ON DELETE RESTRICT,
 CHECK(subject_kind BETWEEN 1 AND 3 AND native_estate_id>0 AND OCTET_LENGTH(proof_blob)>0)
) ENGINE=InnoDB;
-- Unpaid assessments above are HISTORY, not proof of a debt or an arrears rule.
-- No row is installed. Current-cycle-only recovery requires an explicit review;
-- unresolved original recovery/arrears semantics otherwise defer unchanged.
CREATE TABLE IF NOT EXISTS housing_upkeep_reactivation_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 mode TINYINT UNSIGNED NOT NULL,
 review_reference VARBINARY(255) NOT NULL,
 CHECK(estate_id>0 AND native_plot_id>0 AND native_plot_id<4294967295 AND catalog_revision>0 AND mode=1 AND OCTET_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
