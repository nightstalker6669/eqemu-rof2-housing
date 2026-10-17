-- Optional pre-load inventory admission reservation. Housing rule applies to ALL
-- character client versions. No expiration, takeover or automatic crash cleanup.
CREATE TABLE IF NOT EXISTS housing_inventory_sessions (
 character_id INT UNSIGNED NOT NULL PRIMARY KEY,
 account_id INT UNSIGNED NOT NULL,
 epoch BIGINT UNSIGNED NOT NULL,
 token BINARY(32) NOT NULL,
 process_key VARBINARY(96) NOT NULL,
 client_key VARBINARY(96) NOT NULL,
 zone_id INT UNSIGNED NOT NULL,
 instance_id SMALLINT UNSIGNED NOT NULL,
 active TINYINT UNSIGNED NOT NULL,
 CONSTRAINT housing_inventory_session_state CHECK (epoch>0 AND active<=1 AND zone_id>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_inventory_session_history (
 character_id INT UNSIGNED NOT NULL,
 epoch BIGINT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 token BINARY(32) NOT NULL,
 process_key VARBINARY(96) NOT NULL,
 client_key VARBINARY(96) NOT NULL,
 zone_id INT UNSIGNED NOT NULL,
 instance_id SMALLINT UNSIGNED NOT NULL,
 released TINYINT UNSIGNED NOT NULL DEFAULT 0,
 PRIMARY KEY(character_id,epoch),
 UNIQUE KEY housing_inventory_claim_identity(character_id,process_key,client_key),
 CONSTRAINT housing_inventory_history_state CHECK (epoch>0 AND released<=1 AND zone_id>0)
) ENGINE=InnoDB;
-- Binding columns/history identity never change. released advances0->1 only in
-- the same commit that clears exact active ownership. Retain historical epochs.
-- No inventory, currency, catalog, actor or client fixture seeds.
