-- Server-issued reservation intent: a stored house is not a second placed
-- child. The marker never grants custody, ACL, adoption or placement authority.
CREATE TABLE IF NOT EXISTS housing_house_storage_reservations (
 native_item_id INT UNSIGNED NOT NULL PRIMARY KEY,
 CONSTRAINT housing_house_storage_reservation FOREIGN KEY(native_item_id)
  REFERENCES housing_live_placement_reservations(native_item_id) ON DELETE RESTRICT
) ENGINE=InnoDB;
