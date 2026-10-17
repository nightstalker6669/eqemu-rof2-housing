-- Optional server-owned operation journal for complete May2013 native ACL edits.
-- Requires004+012. Native3146 has neither request ID nor expected revision.
-- Caller supplies a unique operation key and trusted sent-snapshot cache revision.
CREATE TABLE IF NOT EXISTS housing_native_acl_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,
 session_key VARBINARY(96) NOT NULL,
 lease_token BINARY(32) NOT NULL,
 estate_id INT UNSIGNED NOT NULL,
 native_plot_id INT UNSIGNED NOT NULL,
 expected_revision BIGINT UNSIGNED NOT NULL,
 committed_revision BIGINT UNSIGNED NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,
 outcome_blob MEDIUMBLOB NOT NULL,
 created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 CONSTRAINT housing_native_acl_receipt_bounds CHECK(lease_epoch>0)
) ENGINE=InnoDB;
-- No native content, estate, member, capability, grant or runtime fixture seeds.
