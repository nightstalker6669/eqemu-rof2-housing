-- Optional native action2 payment-option selection. No rates/timers/content seeds.
CREATE TABLE IF NOT EXISTS housing_upkeep_selection (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 native_plot_id INT UNSIGNED NOT NULL,
 owner_kind TINYINT UNSIGNED NOT NULL,
 owner_id INT UNSIGNED NOT NULL,
 definition_id INT UNSIGNED NOT NULL,
 component_id INT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_upkeep_selection_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,
 session_key VARBINARY(96) NOT NULL,
 lease_token BINARY(32) NOT NULL,
 estate_id INT UNSIGNED NOT NULL,
 native_plot_id INT UNSIGNED NOT NULL,
 component_id INT UNSIGNED NOT NULL,
 selection_revision BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB;
