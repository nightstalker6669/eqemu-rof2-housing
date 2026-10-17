-- Additive only, no historical tariffs or active policy enabled by this schema.
CREATE TABLE IF NOT EXISTS housing_native_plot_purchase_policies (
 catalog_key VARCHAR(128) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 definition_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 definitions_sha256 CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 regions_sha256 CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 policy_revision BIGINT UNSIGNED NOT NULL,
 allow_carried_copper TINYINT UNSIGNED NOT NULL DEFAULT 0,
 max_owned_plots INT UNSIGNED NOT NULL,
 enabled TINYINT UNSIGNED NOT NULL DEFAULT 0,
 review_source VARCHAR(255) NOT NULL,
 PRIMARY KEY(catalog_key,definition_id)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_native_plot_purchase_receipts (
 request_key VARCHAR(96) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 character_id INT UNSIGNED NOT NULL, account_id INT UNSIGNED NOT NULL,
 estate_id INT UNSIGNED NOT NULL, native_plot_id INT UNSIGNED NOT NULL,
 inventory_epoch BIGINT UNSIGNED NOT NULL, inventory_token BINARY(32) NOT NULL,
 process_key VARCHAR(96) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL, outcome_blob MEDIUMBLOB NOT NULL,
 applied TINYINT UNSIGNED NOT NULL DEFAULT 0, reconciled TINYINT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY(request_key), UNIQUE KEY estate_purchase(estate_id), KEY actor(character_id)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_native_plot_purchase_pending (
 character_id INT UNSIGNED NOT NULL,account_id INT UNSIGNED NOT NULL,
 request_key VARCHAR(96) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 inventory_epoch BIGINT UNSIGNED NOT NULL,inventory_token BINARY(32) NOT NULL,
 PRIMARY KEY(character_id),UNIQUE KEY pending_request(request_key),
 CONSTRAINT housing_plot_purchase_pending_receipt FOREIGN KEY(request_key) REFERENCES housing_native_plot_purchase_receipts(request_key)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_native_plot_purchase_reconciliations (
 request_key VARCHAR(96) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 character_id INT UNSIGNED NOT NULL,inventory_epoch BIGINT UNSIGNED NOT NULL,
 inventory_token BINARY(32) NOT NULL,process_key VARCHAR(96) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 outcome_blob MEDIUMBLOB NOT NULL,PRIMARY KEY(request_key)
) ENGINE=InnoDB;
