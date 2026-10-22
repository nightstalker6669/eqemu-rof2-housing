-- Complete RoF2 bag payloads can contain up to 200 children.
-- Individual canonical rows remain MEDIUMBLOB; aggregate receipts and HEX
-- witnesses may repeat that capsule. Widen without rewriting historical bytes.
-- Apply after ALL prior housing schema and lifecycle/layout extensions.
ALTER TABLE housing_live_item_receipts MODIFY COLUMN binding_blob LONGBLOB NOT NULL;
ALTER TABLE housing_live_item_receipts MODIFY COLUMN after_blob LONGBLOB NULL;
ALTER TABLE housing_inventory_death_reconciliations MODIFY COLUMN outcome_blob LONGBLOB NULL;
ALTER TABLE housing_layout_inventory_batches MODIFY COLUMN binding_blob LONGBLOB NOT NULL;
ALTER TABLE housing_layout_inventory_batches MODIFY COLUMN outcome_blob LONGBLOB NOT NULL;
ALTER TABLE housing_layout_mixed_batches MODIFY COLUMN binding_blob LONGBLOB NOT NULL;
ALTER TABLE housing_layout_mixed_batches MODIFY COLUMN outcome_blob LONGBLOB NOT NULL;
ALTER TABLE housing_live_pose_moves MODIFY COLUMN binding_blob LONGBLOB NOT NULL;
ALTER TABLE housing_live_pose_moves MODIFY COLUMN outcome_blob LONGBLOB NOT NULL;
ALTER TABLE housing_ownership_transfer_receipts MODIFY COLUMN binding_blob LONGBLOB NOT NULL;
ALTER TABLE housing_ownership_transfer_receipts MODIFY COLUMN outcome_blob LONGBLOB NOT NULL;
ALTER TABLE housing_live_transfer_receipts MODIFY COLUMN binding_blob LONGBLOB NOT NULL;
ALTER TABLE housing_live_vault_transfers MODIFY COLUMN binding_blob LONGBLOB NOT NULL;
ALTER TABLE housing_house_inventory_receipts MODIFY COLUMN binding_blob LONGBLOB NOT NULL;
ALTER TABLE housing_house_inventory_receipts MODIFY COLUMN outcome_blob LONGBLOB NOT NULL;
ALTER TABLE housing_house_swap_receipts MODIFY COLUMN outcome_blob LONGBLOB NOT NULL;
ALTER TABLE housing_return_settlement_items MODIFY COLUMN source_blob LONGBLOB NOT NULL;
ALTER TABLE housing_return_settlement_items MODIFY COLUMN delivery_blob LONGBLOB NOT NULL;
ALTER TABLE housing_offline_return_items MODIFY COLUMN state_before LONGBLOB NOT NULL;
ALTER TABLE housing_return_parcels MODIFY COLUMN source_state LONGBLOB NOT NULL;
ALTER TABLE housing_inactivity_packings MODIFY COLUMN before_blob LONGBLOB NOT NULL;
ALTER TABLE housing_inactivity_packings MODIFY COLUMN after_blob LONGBLOB NOT NULL;
ALTER TABLE housing_guild_inactivity_packings MODIFY COLUMN before_blob LONGBLOB NOT NULL;
ALTER TABLE housing_guild_inactivity_packings MODIFY COLUMN after_blob LONGBLOB NOT NULL;
ALTER TABLE housing_inactivity_deliveries MODIFY COLUMN cargo_blob LONGBLOB NOT NULL;
ALTER TABLE housing_guild_inactivity_deliveries MODIFY COLUMN cargo_blob LONGBLOB NOT NULL;
ALTER TABLE housing_upkeep_cycle_scopes MODIFY COLUMN proof_blob LONGBLOB NOT NULL;
ALTER TABLE housing_catalog_crate_upgrades MODIFY COLUMN bindings_blob LONGBLOB NOT NULL;
ALTER TABLE housing_crate_batch_receipts MODIFY COLUMN binding_blob LONGBLOB NOT NULL;
ALTER TABLE housing_crate_storage_receipts MODIFY COLUMN binding_blob LONGBLOB NOT NULL;
