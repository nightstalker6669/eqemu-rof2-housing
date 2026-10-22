-- Empty until the installer explicitly enrolls this configured database.
-- This authorizes reviewed reconstructed Origin2 content, not fixture funding.
CREATE TABLE IF NOT EXISTS housing_runtime_activation (
 id TINYINT UNSIGNED NOT NULL PRIMARY KEY,
 database_name VARCHAR(64) NOT NULL,
 client_build INT UNSIGNED NOT NULL,
 policy_revision INT UNSIGNED NOT NULL,
 allow_reconstructed_content TINYINT UNSIGNED NOT NULL DEFAULT 0,
 review_reference VARCHAR(192) NOT NULL
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS housing_content_installations (
 catalog_key VARCHAR(128) CHARACTER SET ascii COLLATE ascii_bin NOT NULL PRIMARY KEY,
 source_sha256 CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 neighborhood_id INT UNSIGNED NOT NULL
) ENGINE=InnoDB;
