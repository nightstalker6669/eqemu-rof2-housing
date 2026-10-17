-- Synthetic isolated domain only. NEVER use ItemInstance::Serialize pointers here.
-- Canonical internal versioned record_blob preserves exact opaque item bytes and
-- immutable provenance/identity/lore/no-drop; not a guessed native item serializer.
CREATE TABLE IF NOT EXISTS housing_fixture_item_custody (
 instance_key VARBINARY(96) NOT NULL PRIMARY KEY,
 owner_character_id INT UNSIGNED NOT NULL,
 catalog_item_id INT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL,
 custody_state TINYINT UNSIGNED NOT NULL,
 holder_character_id INT UNSIGNED NULL,
 inventory_slot INT UNSIGNED NULL,
 estate_id INT UNSIGNED NULL,
 record_blob MEDIUMBLOB NOT NULL,
 UNIQUE KEY housing_fixture_inventory_slot(holder_character_id,inventory_slot),
 CONSTRAINT housing_fixture_custody_bounds CHECK
  (revision>0 AND owner_character_id>0 AND catalog_item_id>0 AND
  ((custody_state=0 AND holder_character_id IS NOT NULL AND holder_character_id=owner_character_id AND inventory_slot IS NOT NULL AND inventory_slot>0 AND estate_id IS NULL) OR
   (custody_state IN (1,2) AND holder_character_id IS NULL AND inventory_slot IS NULL AND estate_id IS NOT NULL AND estate_id>0))),
 CONSTRAINT housing_fixture_item_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_fixture_item_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 estate_id INT UNSIGNED NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,
 completed TINYINT UNSIGNED NOT NULL DEFAULT 0,
 after_blob MEDIUMBLOB NULL,
 created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 CONSTRAINT housing_fixture_item_receipt_state CHECK (completed<=1 AND (completed=0 OR after_blob IS NOT NULL))
) ENGINE=InnoDB;
-- No automatic seeds, runtime bindings or production inventory reads/writes.
-- Custody is ONE row per item instance, updated atomically, never copied on move.
-- Receipts retained independently of estate lifetime; later retention/custody policy required.
