-- Optional atomic purchase foundation. No Live catalog, plot, currency or prices seeded.
-- Funding0 is unsupported. Fixture1 is usable only through the isolated service API.
CREATE TABLE IF NOT EXISTS housing_purchase_offers (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    plot_id INT UNSIGNED NOT NULL,
    revision INT UNSIGNED NOT NULL,
    cost_scheme_id INT UNSIGNED NOT NULL,
    cost_component_id INT UNSIGNED NOT NULL,
    currency_type_id INT UNSIGNED NOT NULL,
    currency_subtype_id INT UNSIGNED NOT NULL,
    copper_price BIGINT NOT NULL,
    funding_source TINYINT UNSIGNED NOT NULL DEFAULT 0,
    enabled TINYINT UNSIGNED NOT NULL DEFAULT 0,
    fixture_only TINYINT UNSIGNED NOT NULL DEFAULT 1,
    UNIQUE KEY housing_offer_plot (plot_id),
    CONSTRAINT housing_offer_plot_fk FOREIGN KEY (plot_id)
        REFERENCES housing_plot_definitions (id) ON DELETE RESTRICT
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS housing_purchase_requests (
    request_key VARBINARY(96) NOT NULL PRIMARY KEY,
    character_id INT UNSIGNED NOT NULL,
    account_id INT UNSIGNED NOT NULL,
    plot_id INT UNSIGNED NOT NULL,
    offer_id INT UNSIGNED NOT NULL,
    offer_revision INT UNSIGNED NOT NULL,
    estate_id INT UNSIGNED NOT NULL DEFAULT 0,
    charged_copper BIGINT NOT NULL DEFAULT 0,
    remaining_carried_copper BIGINT NOT NULL DEFAULT 0,
    completed TINYINT UNSIGNED NOT NULL DEFAULT 0,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY housing_purchase_estate (estate_id, request_key)
) ENGINE=InnoDB;

-- A pending request is never committed separately. Request, currency debit and
-- estate ownership commit together. Receipts are retained for uncertain-commit replay.
