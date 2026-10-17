-- Optional actual furniture placement, no content/geometry/catalog seeds.
CREATE TABLE IF NOT EXISTS housing_live_placement_reservations (
 native_item_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
 request_key VARBINARY(96) NOT NULL UNIQUE,
 character_id INT UNSIGNED NOT NULL,account_id INT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,session_key VARBINARY(96) NOT NULL,
 estate_id INT UNSIGNED NOT NULL,native_estate_id INT UNSIGNED NOT NULL,
 catalog_key VARBINARY(96) NOT NULL,catalog_revision BIGINT UNSIGNED NOT NULL,
 source_row MEDIUMBLOB NOT NULL,runtime_serial INT NOT NULL,
 state TINYINT UNSIGNED NOT NULL DEFAULT 0,custody_id BIGINT UNSIGNED NULL UNIQUE,
 CONSTRAINT housing_preview_state CHECK(state<=2 AND runtime_serial>0),
 CONSTRAINT housing_preview_native_estate FOREIGN KEY(native_estate_id) REFERENCES housing_native_estates(native_id) ON DELETE RESTRICT,
 CONSTRAINT housing_preview_domain_estate FOREIGN KEY(estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT
) ENGINE=InnoDB AUTO_INCREMENT=10000000;
CREATE TABLE IF NOT EXISTS housing_live_placed_items (
 custody_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 native_item_id INT UNSIGNED NOT NULL UNIQUE,
 pose_revision BIGINT UNSIGNED NOT NULL DEFAULT 1,
 pose_blob BINARY(28) NOT NULL,
 CONSTRAINT housing_placed_custody FOREIGN KEY(custody_id) REFERENCES housing_live_item_custody(custody_id) ON DELETE RESTRICT,
 CONSTRAINT housing_placed_reservation FOREIGN KEY(native_item_id) REFERENCES housing_live_placement_reservations(native_item_id) ON DELETE RESTRICT,
 CONSTRAINT housing_placed_revision CHECK(pose_revision>0)
) ENGINE=InnoDB;

-- Pose-only journal. Rendering is reconstructed from current rows, never this
-- historical outcome. No inventory pending/application acknowledgement is used.
CREATE TABLE IF NOT EXISTS housing_live_pose_moves (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 character_id INT UNSIGNED NOT NULL,account_id INT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,session_key VARBINARY(96) NOT NULL,
 binding_blob MEDIUMBLOB NOT NULL,outcome_blob MEDIUMBLOB NOT NULL
) ENGINE=InnoDB;
