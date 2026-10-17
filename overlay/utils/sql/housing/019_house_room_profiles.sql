-- Optional separate room content, preserving the exterior house v2 definition.
-- Requires016. No seeds, runtime enablement or historically verified data.
-- Room native ID/route and current access are resolved from locked HouseID;
-- profile presence or FK alone never authorizes a room or an item operation.
CREATE TABLE IF NOT EXISTS housing_house_room_profiles (
 house_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 revision BIGINT UNSIGNED NOT NULL DEFAULT 1,
 origin TINYINT UNSIGNED NOT NULL,
 client_build INT UNSIGNED NOT NULL,
 zone_short_name VARBINARY(96) NOT NULL,
 client_asset_sha256 BINARY(32) NOT NULL,
 collision_map_sha256 BINARY(32) NOT NULL,
 manifest_sha256 BINARY(32) NOT NULL,
 definitions_blob BLOB NOT NULL,
 regions_blob BLOB NOT NULL,
 CONSTRAINT housing_room_profile_house FOREIGN KEY(house_id)
  REFERENCES housing_house_interiors(house_id) ON DELETE RESTRICT,
 CONSTRAINT housing_room_profile_shape CHECK(house_id>0 AND revision>0
  AND origin IN (1,2) AND client_build=20130510
  AND OCTET_LENGTH(zone_short_name)>0
  AND OCTET_LENGTH(definitions_blob)>0 AND OCTET_LENGTH(regions_blob)>0)
) ENGINE=InnoDB;
