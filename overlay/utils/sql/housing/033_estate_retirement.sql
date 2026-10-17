-- Target runtime uses MariaDB. Retain ALL old estate IDs, keys, owners and FKs.
-- One active ownership generation per plot, arbitrarily many retired generations.
ALTER TABLE housing_estates ADD COLUMN IF NOT EXISTS active TINYINT UNSIGNED NOT NULL DEFAULT 1;
ALTER TABLE housing_estates ADD COLUMN IF NOT EXISTS active_plot_id INT UNSIGNED AS (CASE WHEN active=1 THEN plot_id ELSE NULL END) STORED;
ALTER TABLE housing_estates ADD INDEX IF NOT EXISTS housing_estate_plot_history(plot_id);
ALTER TABLE housing_estates DROP INDEX IF EXISTS housing_estate_plot;
ALTER TABLE housing_estates ADD UNIQUE INDEX IF NOT EXISTS housing_estate_active_plot(active_plot_id);
-- Preserve additive/idempotent upgrades where the fresh base already has it.
SET @housing_retirement_check_sql = (SELECT IF(COUNT(*)=0,
 'ALTER TABLE housing_estates ADD CONSTRAINT housing_estate_active_flag CHECK (active<=1)',
 'SELECT 1') FROM information_schema.table_constraints
 WHERE constraint_schema=DATABASE() AND table_name='housing_estates'
 AND constraint_name='housing_estate_active_flag');
PREPARE housing_retirement_schema_stmt FROM @housing_retirement_check_sql;
EXECUTE housing_retirement_schema_stmt;
DEALLOCATE PREPARE housing_retirement_schema_stmt;
CREATE OR REPLACE ALGORITHM=MERGE VIEW housing_current_estates AS
 SELECT id,request_key,plot_id,owner_kind,owner_id,created_at FROM housing_estates WHERE active=1;
CREATE TABLE IF NOT EXISTS housing_estate_retirements (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 request_key VARBINARY(96) NOT NULL UNIQUE,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,
 session_key VARBINARY(96) NOT NULL,
 lease_token BINARY(32) NOT NULL,
 native_plot_id INT UNSIGNED NOT NULL,
 plot_id INT UNSIGNED NOT NULL,
 catalog_revision BIGINT UNSIGNED NOT NULL,
 created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 CONSTRAINT housing_retirement_generation FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
