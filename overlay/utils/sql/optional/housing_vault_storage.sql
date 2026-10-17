-- Optional typed native vault storage, installed before enabling the new path.
-- Uses the SAME immutable canonical custody row and native item ID allocator;
-- no spatial placement row/world actor is created for vault items.
CREATE TABLE housing_live_vault_items (
 custody_id BIGINT UNSIGNED NOT NULL,
 native_item_id INT UNSIGNED NOT NULL,
 realm_estate_id INT UNSIGNED NOT NULL,
 house_id BIGINT UNSIGNED NULL,
 revision BIGINT UNSIGNED NOT NULL,
 PRIMARY KEY(custody_id),UNIQUE KEY(native_item_id),KEY(house_id),KEY(realm_estate_id),
 CONSTRAINT housing_vault_custody_fk FOREIGN KEY(custody_id) REFERENCES housing_live_item_custody(custody_id),
 CONSTRAINT housing_vault_native_fk FOREIGN KEY(native_item_id) REFERENCES housing_live_placement_reservations(native_item_id),
 CONSTRAINT housing_vault_revision_check CHECK(revision>0)
) ENGINE=InnoDB;
CREATE TABLE housing_live_vault_transfers (
 request_key VARBINARY(96) NOT NULL,
 character_id INT UNSIGNED NOT NULL,
 account_id INT UNSIGNED NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,
 custody_id BIGINT UNSIGNED NOT NULL,
 native_item_id INT UNSIGNED NOT NULL,
 native_estate_id INT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL,
 PRIMARY KEY(request_key)
) ENGINE=InnoDB;
