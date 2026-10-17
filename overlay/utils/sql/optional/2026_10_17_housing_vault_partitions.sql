-- Additive custody-only partial transfer evidence; no policy/content seeds.
CREATE TABLE IF NOT EXISTS housing_live_vault_partitions (
 request_key VARCHAR(96) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 format_version INT UNSIGNED NOT NULL,
 outcome_blob MEDIUMBLOB NOT NULL,
 PRIMARY KEY(request_key)
) ENGINE=InnoDB;
