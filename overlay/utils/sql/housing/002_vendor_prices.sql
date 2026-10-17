-- Optional merchant-specific housing prices. No item or ordinary vendor prices change.
-- Core integration must additionally gate lookups on Housing:EnablePlayerHousing,
-- verified housing zone/session and the actual NPC type; this table is not authorization.
CREATE TABLE IF NOT EXISTS housing_vendor_prices (
    merchant_npc_type_id INT UNSIGNED NOT NULL,
    item_id INT UNSIGNED NOT NULL,
    copper_price INT UNSIGNED NOT NULL,
    enabled TINYINT UNSIGNED NOT NULL DEFAULT 0,
    category VARCHAR(32) NOT NULL,
    evidence_uri VARCHAR(512) NOT NULL,
    PRIMARY KEY (merchant_npc_type_id, item_id)
) ENGINE=InnoDB;

-- No catalog data is seeded by this migration. Verified catalogs are an explicit,
-- isolated provisioning action. No guessed package contents are defined here.
