-- Optional bounded upkeep charging. No contracts, prices, currency or dates seeded.
-- An administrator must supply the reviewed first billing epoch and provenance.
-- The service intentionally refuses paid plots/houses, room-scoped paid items,
-- fractional copper, alternate currencies, ambiguous partial funding and catch-up.
CREATE TABLE IF NOT EXISTS housing_upkeep_contracts (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 owner_kind TINYINT UNSIGNED NOT NULL,
 owner_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 next_due_epoch INT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL DEFAULT 1,
 review_reference VARBINARY(255) NOT NULL,
 CONSTRAINT housing_upkeep_contract_shape CHECK(estate_id>0 AND native_plot_id>0 AND native_plot_id<4294967295 AND owner_id>0 AND catalog_revision>0 AND next_due_epoch>0 AND next_due_epoch<=4294963695 AND revision>0 AND OCTET_LENGTH(review_reference)>0),
 FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_upkeep_cycles (
 estate_id INT UNSIGNED NOT NULL,
 due_epoch INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 contract_revision BIGINT UNSIGNED NOT NULL,
 escrow_before BIGINT NOT NULL,
 escrow_after BIGINT NOT NULL,
 charged_copper BIGINT NOT NULL,
 stowed_count INT UNSIGNED NOT NULL,
 PRIMARY KEY(estate_id,due_epoch),
 CONSTRAINT housing_upkeep_cycle_conservation CHECK(escrow_before>=0 AND escrow_after>=0 AND charged_copper>=0 AND escrow_before=escrow_after+charged_copper)
) ENGINE=InnoDB;
-- Immutable per-cycle witnesses survive Return and ownership retirement.
CREATE TABLE IF NOT EXISTS housing_upkeep_cycle_items (
 estate_id INT UNSIGNED NOT NULL,
 due_epoch INT UNSIGNED NOT NULL,
 custody_id BIGINT UNSIGNED NOT NULL,
 native_item_id INT UNSIGNED NOT NULL,
 creator_character_id INT UNSIGNED NOT NULL,
 component_id INT UNSIGNED NOT NULL,
 hourly_copper BIGINT NOT NULL,
 charged_copper BIGINT NOT NULL,
 storage_before BIGINT UNSIGNED NOT NULL,
 storage_after BIGINT UNSIGNED NOT NULL,
 source_row MEDIUMBLOB NOT NULL,
 PRIMARY KEY(estate_id,due_epoch,custody_id),
 FOREIGN KEY(estate_id,due_epoch) REFERENCES housing_upkeep_cycles(estate_id,due_epoch) ON DELETE RESTRICT,
 CONSTRAINT housing_upkeep_item_shape CHECK(custody_id>0 AND native_item_id>0 AND creator_character_id>0 AND hourly_copper>0 AND charged_copper>=0 AND (charged_copper=hourly_copper OR charged_copper=0) AND storage_after>=storage_before)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_upkeep_item_state (
 custody_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 estate_id INT UNSIGNED NOT NULL,
 native_item_id INT UNSIGNED NOT NULL,
 expired_since INT UNSIGNED NOT NULL,
 last_due_epoch INT UNSIGNED NOT NULL,
 CONSTRAINT housing_upkeep_item_state_shape CHECK(custody_id>0 AND estate_id>0 AND native_item_id>0 AND last_due_epoch>0)
) ENGINE=InnoDB;
