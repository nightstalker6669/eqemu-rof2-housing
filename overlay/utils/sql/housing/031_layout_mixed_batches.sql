-- Optional typed full layout journal. Requires030 plus property outcome hooks.
-- Do not activate native loads until live property/inventory application/ACK
-- and certified writer-death reconciliation consume this exact family.
CREATE TABLE IF NOT EXISTS housing_layout_mixed_batches (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 format_version SMALLINT UNSIGNED NOT NULL,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,
 session_key VARBINARY(96) NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,
 outcome_blob MEDIUMBLOB NOT NULL,
 applied TINYINT UNSIGNED NOT NULL DEFAULT 0,
 CONSTRAINT housing_layout_mixed_state CHECK(format_version=1 AND applied<=1)
) ENGINE=InnoDB;
