-- UID locator only: deliberately no FK to REPLACE-written character/evolving rows.
-- All item/progress changes share the existing fenced housing transaction.
CREATE TABLE IF NOT EXISTS housing_evolving_item_locations (
 evolving_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 location_kind TINYINT UNSIGNED NOT NULL,
 character_id INT UNSIGNED NOT NULL,
 item_id INT UNSIGNED NOT NULL,
 slot_id INT UNSIGNED NOT NULL,
 inventory_guid BIGINT UNSIGNED NOT NULL,
 component TINYINT UNSIGNED NOT NULL,
 custody_id BIGINT UNSIGNED NOT NULL,
 path TINYINT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL,
 KEY inventory_location(character_id,slot_id,component),
 KEY custody_location(custody_id,path,component)
) ENGINE=InnoDB;
