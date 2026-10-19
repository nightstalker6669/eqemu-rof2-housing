-- No catch-up review or retrospective checkpoint is seeded. Known unchanged
-- on-time state is required before any historical assessment is authorized.
CREATE TABLE IF NOT EXISTS housing_upkeep_catchup_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 owner_kind TINYINT UNSIGNED NOT NULL,
 owner_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 definitions_sha256 CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 regions_sha256 CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 max_overdue_hours INT UNSIGNED NOT NULL,
 enabled TINYINT UNSIGNED NOT NULL DEFAULT 0,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_catchup_policy_shape CHECK(estate_id>0 AND native_plot_id>0 AND native_plot_id<4294967295 AND owner_kind IN(1,3) AND owner_id>0 AND catalog_revision>0 AND max_overdue_hours BETWEEN 1 AND 2160 AND enabled IN(0,1) AND CHAR_LENGTH(review_reference)>0),
 FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_upkeep_catchup_checkpoints (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 next_due_epoch INT UNSIGNED NOT NULL,
 anchor_due_epoch INT UNSIGNED NOT NULL,
 observed_at_epoch INT UNSIGNED NOT NULL,
 contract_revision BIGINT UNSIGNED NOT NULL,
 escrow_copper BIGINT UNSIGNED NOT NULL,
 obligations_sha256 BINARY(32) NOT NULL,
 policy_sha256 BINARY(32) NOT NULL,
 CONSTRAINT housing_catchup_checkpoint_shape CHECK(anchor_due_epoch>0 AND next_due_epoch>=anchor_due_epoch AND observed_at_epoch>0 AND observed_at_epoch<anchor_due_epoch AND contract_revision>0 AND escrow_copper<=9223372036854775807),
 FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_upkeep_catchup_audits (
 estate_id INT UNSIGNED NOT NULL,
 due_epoch INT UNSIGNED NOT NULL,
 anchor_due_epoch INT UNSIGNED NOT NULL,
 observed_at_epoch INT UNSIGNED NOT NULL,
 processed_at_epoch INT UNSIGNED NOT NULL,
 before_sha256 BINARY(32) NOT NULL,
 after_sha256 BINARY(32) NOT NULL,
 policy_sha256 BINARY(32) NOT NULL,
 PRIMARY KEY(estate_id,due_epoch),
 FOREIGN KEY(estate_id,due_epoch) REFERENCES housing_upkeep_cycles(estate_id,due_epoch) ON DELETE RESTRICT
) ENGINE=InnoDB;
-- Scope the immutable funding-history scan to one generation. The original
-- escrow receipt migration had no estate index; this adds no policy or data.
ALTER TABLE IF EXISTS housing_live_escrow_receipts
 ADD INDEX IF NOT EXISTS housing_escrow_estate_history (estate_id);

-- Optional future-purchase enrollment only. Apply after upkeep rounding/enrollment.
-- Existing review rows default to zero; no existing estate, catch-up policy or
-- checkpoint is enabled or synthesized by this migration. Values 1..2160 opt in
-- only successful subsequent purchases under the exact reviewed catalogue.
ALTER TABLE housing_upkeep_enrollment_policy
 ADD COLUMN IF NOT EXISTS max_catchup_hours INT UNSIGNED NOT NULL DEFAULT 0;
ALTER TABLE housing_upkeep_enrollment_policy
 ADD CONSTRAINT IF NOT EXISTS housing_enrollment_catchup_shape CHECK(max_catchup_hours BETWEEN 0 AND 2160);
