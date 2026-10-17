-- UNSEEDED SERVER REVIEW POLICY. This is not an asserted RoF2 wallet rule.
-- Whole escrow -> carried copper only, without denomination normalization.
-- Native funded Move Out remains rejected absent exact generation/catalog policy.
CREATE TABLE IF NOT EXISTS housing_occupied_refund_policy (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 destination_mode TINYINT UNSIGNED NOT NULL,
 enabled TINYINT UNSIGNED NOT NULL DEFAULT 0,
 review_reference VARCHAR(255) NOT NULL,
 CONSTRAINT housing_refund_review_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_refund_review_mode CHECK(destination_mode=1),
 CONSTRAINT housing_refund_review_enabled CHECK(enabled IN (0,1)),
 CONSTRAINT housing_refund_review_reference CHECK(CHAR_LENGTH(review_reference)>0)
) ENGINE=InnoDB;
-- Deliberately no INSERT, default economy, scheduler, fee or 90-day disposition.

-- Separate immutable proof; receipt.applied remains 0 after certified-death load.
CREATE TABLE IF NOT EXISTS housing_occupied_refund_death_reconciliations (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 inventory_epoch BIGINT UNSIGNED NOT NULL,
 inventory_token BINARY(32) NOT NULL,
 process_key VARBINARY(96) NOT NULL,
 outcome_blob MEDIUMBLOB NOT NULL,
 reconciled_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 CONSTRAINT housing_refund_death_receipt FOREIGN KEY(request_key) REFERENCES housing_live_escrow_receipts(request_key) ON DELETE RESTRICT
) ENGINE=InnoDB;
