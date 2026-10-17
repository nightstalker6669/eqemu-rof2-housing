-- Optional direct-supervisor inventory recovery. No timeout or takeover.
-- Only the supervisor owning the exact spawned child records reaped death.
-- Existing EQlaunch STOPPED/TCP/CLE/heartbeat state is never death evidence.
CREATE TABLE IF NOT EXISTS housing_inventory_processes (
 process_key VARBINARY(96) NOT NULL PRIMARY KEY,
 supervisor_token BINARY(32) NOT NULL,
 child_pid BIGINT UNSIGNED NULL,
 reaped TINYINT UNSIGNED NOT NULL DEFAULT 0,
 exit_code INT NULL,
 reaped_at DATETIME NULL,
 CONSTRAINT housing_inventory_process_reaped CHECK (
  reaped<=1 AND (reaped=0 OR (child_pid IS NOT NULL AND child_pid>0 AND exit_code IS NOT NULL AND reaped_at IS NOT NULL))
 )
) ENGINE=InnoDB;

-- Both lease identities are immutable; one custody epoch has exactly one writer.
CREATE TABLE IF NOT EXISTS housing_inventory_custody_links (
 character_id INT UNSIGNED NOT NULL,
 inventory_epoch BIGINT UNSIGNED NOT NULL,
 inventory_token BINARY(32) NOT NULL,
 process_key VARBINARY(96) NOT NULL,
 client_key VARBINARY(96) NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 custody_epoch BIGINT UNSIGNED NOT NULL,
 custody_session VARBINARY(96) NOT NULL,
 custody_token BINARY(32) NOT NULL,
 PRIMARY KEY(character_id,inventory_epoch,custody_epoch),
 UNIQUE KEY housing_inventory_custody_identity(character_id,custody_epoch),
 KEY housing_inventory_custody_process(process_key),
 CONSTRAINT housing_inventory_link_epochs CHECK(inventory_epoch>0 AND custody_epoch>0)
) ENGINE=InnoDB;

-- A verified durable outcome after writer death is distinct from m_inv apply.
-- Never set housing_live_item_receipts.applied=1 from restart recovery.
CREATE TABLE IF NOT EXISTS housing_inventory_death_reconciliations (
 character_id INT UNSIGNED NOT NULL,
 inventory_epoch BIGINT UNSIGNED NOT NULL,
 process_key VARBINARY(96) NOT NULL,
 inventory_token BINARY(32) NOT NULL,
 custody_epoch BIGINT UNSIGNED NULL,
 custody_session VARBINARY(96) NULL,
 pending_key VARBINARY(96) NULL,
 outcome_blob MEDIUMBLOB NULL,
 reconciled_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 PRIMARY KEY(character_id,inventory_epoch)
) ENGINE=InnoDB;
-- No process, actor, catalog or recovery fixture seeds; opt-in stays disabled.
