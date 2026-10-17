-- Optional room-specific witness attached to a normal native item reservation.
-- Requires010/019. No seeds or runtime activation. Exterior content codecs remain
-- v1/v2; room identity is explicit here until ALL mutation/recovery paths exist.
CREATE TABLE IF NOT EXISTS housing_interior_placement_reservations (
 native_item_id INT UNSIGNED NOT NULL PRIMARY KEY,
 house_id BIGINT UNSIGNED NOT NULL,
 house_revision BIGINT UNSIGNED NOT NULL,
 profile_revision BIGINT UNSIGNED NOT NULL,
 source_zone INT UNSIGNED NOT NULL,
 source_version TINYINT UNSIGNED NOT NULL,
 source_instance INT NOT NULL,
 content_version INT UNSIGNED NOT NULL,
 content_digest BINARY(32) NOT NULL,
 CONSTRAINT housing_interior_preview_native FOREIGN KEY(native_item_id)
  REFERENCES housing_live_placement_reservations(native_item_id) ON DELETE RESTRICT,
 CONSTRAINT housing_interior_preview_profile FOREIGN KEY(house_id)
  REFERENCES housing_house_room_profiles(house_id) ON DELETE RESTRICT,
 CONSTRAINT housing_interior_preview_instance FOREIGN KEY(source_instance)
  REFERENCES instance_list(id) ON DELETE RESTRICT,
 CONSTRAINT housing_interior_preview_shape CHECK(native_item_id>0 AND native_item_id<4294967295
  AND house_id>0 AND house_revision>0 AND profile_revision>0
  AND source_zone>0 AND source_instance>0 AND source_instance<=32000 AND content_version=3)
) ENGINE=InnoDB;
