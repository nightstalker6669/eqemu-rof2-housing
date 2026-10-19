-- Optional administrative catalog transition journal. No content is activated.
CREATE TABLE housing_catalog_crate_upgrades (
 request_key VARBINARY(128) NOT NULL PRIMARY KEY,
 catalog_key VARBINARY(128) NOT NULL,
 from_revision BIGINT UNSIGNED NOT NULL,
 to_revision BIGINT UNSIGNED NOT NULL,
 plan_sha256 BINARY(32) NOT NULL,
 before_definitions MEDIUMBLOB NOT NULL,
 after_definitions MEDIUMBLOB NOT NULL,
 regions_sha256 BINARY(32) NOT NULL,
 bindings_blob MEDIUMBLOB NOT NULL,
 created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 UNIQUE KEY one_catalog_transition(catalog_key,from_revision)
) ENGINE=InnoDB;
