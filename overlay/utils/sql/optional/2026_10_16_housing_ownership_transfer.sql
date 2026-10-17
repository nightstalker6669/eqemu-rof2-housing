-- Internal ownership generations; no native fields, catalog/content or tariff seeds.
CREATE TABLE IF NOT EXISTS housing_ownership_state (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 revision BIGINT UNSIGNED NOT NULL,
 owner_character_id INT UNSIGNED NOT NULL,
 CONSTRAINT housing_ownership_state_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id),
 CONSTRAINT housing_ownership_state_bounds CHECK(revision>0 AND owner_character_id>0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_ownership_transfer_receipts (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,account_id INT UNSIGNED NOT NULL,
 estate_id INT UNSIGNED NOT NULL,native_plot_id INT UNSIGNED NOT NULL,
 old_owner INT UNSIGNED NOT NULL,new_owner INT UNSIGNED NOT NULL,
 ownership_revision BIGINT UNSIGNED NOT NULL,acl_revision BIGINT UNSIGNED NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,outcome_blob MEDIUMBLOB NOT NULL,
 UNIQUE KEY estate_generation(estate_id,ownership_revision),
 CONSTRAINT housing_ownership_transfer_bounds CHECK(ownership_revision>0 AND acl_revision>0 AND old_owner>0 AND new_owner>0 AND old_owner<>new_owner)
) ENGINE=InnoDB;
