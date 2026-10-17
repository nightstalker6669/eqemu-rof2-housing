-- Explicit server-reviewed binding only. NO original benefit/tier/cost seeds.
-- Native definitions come from tributes + tribute_levels; neither table in
-- current PEQ has the missing original trophy definitions. Importing invented
-- values as historical content is not part of this patch.
CREATE TABLE IF NOT EXISTS housing_trophy_benefit_sources (
 benefit_id INT UNSIGNED PRIMARY KEY,
 placed_item_id INT UNSIGNED NOT NULL,
 source_sha256 CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 source_description VARCHAR(255) NOT NULL,
 UNIQUE KEY one_benefit_per_trophy (placed_item_id)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_trophy_benefit_state (
 character_id INT UNSIGNED PRIMARY KEY,
 enabled TINYINT UNSIGNED NOT NULL DEFAULT 0,
 revision BIGINT UNSIGNED NOT NULL DEFAULT 1,
 CHECK (enabled IN (0,1)), CHECK (revision > 0)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_trophy_benefit_selections (
 character_id INT UNSIGNED NOT NULL,
 slot TINYINT UNSIGNED NOT NULL,
 benefit_id INT UNSIGNED NOT NULL,
 tier TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY (character_id,slot),
 UNIQUE KEY distinct_benefit (character_id,benefit_id),
 CHECK (slot < 10), CHECK (tier < 10)
) ENGINE=InnoDB;
