-- Optional additive catalog schema. No catalog or Live geometry is seeded.
-- Canonical binary records retain every source float bit and native opaque field.
CREATE TABLE IF NOT EXISTS housing_catalogs (
 catalog_key VARBINARY(96) NOT NULL PRIMARY KEY,
 revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
 origin TINYINT UNSIGNED NOT NULL,
 source VARCHAR(255) NOT NULL,
 source_sha256 CHAR(64) CHARACTER SET ascii NOT NULL,
 client_build INT UNSIGNED NOT NULL,
 definitions_blob MEDIUMBLOB NOT NULL,
 regions_blob MEDIUMBLOB NOT NULL,
 updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
 CONSTRAINT housing_catalog_origin CHECK(origin IN(1,2))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_native_estates (
 native_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
 catalog_key VARBINARY(96) NOT NULL,
 node_key VARBINARY(96) NOT NULL,
 parent_native_id INT UNSIGNED NULL,
 neighborhood_id INT UNSIGNED NOT NULL,
 plot_id INT UNSIGNED NULL,
 ownership_estate_id INT UNSIGNED NULL,
 definition_id INT UNSIGNED NOT NULL,
 static_id INT UNSIGNED NOT NULL,
 active TINYINT UNSIGNED NOT NULL DEFAULT 1,
 UNIQUE KEY housing_native_node(catalog_key,node_key),
 UNIQUE KEY housing_native_plot(plot_id),
 UNIQUE KEY housing_native_ownership(ownership_estate_id),
 INDEX housing_native_neighborhood(neighborhood_id,active),
 CONSTRAINT housing_native_catalog FOREIGN KEY(catalog_key) REFERENCES housing_catalogs(catalog_key) ON DELETE RESTRICT,
 CONSTRAINT housing_native_parent FOREIGN KEY(parent_native_id) REFERENCES housing_native_estates(native_id) ON DELETE RESTRICT,
 CONSTRAINT housing_native_neighborhood_fk FOREIGN KEY(neighborhood_id) REFERENCES housing_neighborhoods(id) ON DELETE RESTRICT,
 CONSTRAINT housing_native_plot_fk FOREIGN KEY(plot_id) REFERENCES housing_plot_definitions(id) ON DELETE RESTRICT,
 CONSTRAINT housing_native_owner_fk FOREIGN KEY(ownership_estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB AUTO_INCREMENT=10000000;
-- IDs are server-issued runtime identities, not copied historical static IDs.
-- Root/node graph references are checked transactionally by the service.
