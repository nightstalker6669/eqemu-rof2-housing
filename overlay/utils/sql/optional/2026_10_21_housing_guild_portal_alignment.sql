-- Persistent housing-only guild hall portal alignment. Legacy basic guildhall
-- guild_portal-* quest buckets are deliberately untouched. Apply after016.
CREATE TABLE IF NOT EXISTS housing_guild_portal_alignments (
 house_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 estate_id INT UNSIGNED NOT NULL,
 guild_id INT UNSIGNED NOT NULL,
 zone_id INT UNSIGNED NOT NULL,
 instance_id INT NOT NULL,
 stone_item_id INT UNSIGNED NOT NULL,
 expires_epoch BIGINT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL,
 CONSTRAINT housing_portal_house_fk FOREIGN KEY (house_id)
  REFERENCES housing_house_instances(house_id) ON DELETE RESTRICT,
 CONSTRAINT housing_portal_estate_fk FOREIGN KEY (estate_id)
  REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_portal_identity CHECK(estate_id>0 AND guild_id>0 AND zone_id>0
  AND instance_id>0 AND instance_id<=32000 AND expires_epoch>0 AND revision>0),
 CONSTRAINT housing_portal_destination CHECK(stone_item_id IN
  (10092,10094,64191,76000,76001,76002,76003,76004,76005,76006,76007,
   76009,76010,76011,76012,76070,88735,88736,88738,88739,88740))
) ENGINE=InnoDB;
