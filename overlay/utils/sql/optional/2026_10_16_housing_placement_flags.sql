-- Required before a binary containing the named housing placement flag loader.
-- Historical UNK232..238 are intentionally not backfilled: their packed layout
-- is not established by the retained 2013 native reader or current item schema.
-- NULL means unknown; it must not authorize a new placement class/mode.
ALTER TABLE items
  ADD COLUMN housing_disable_rotation TINYINT UNSIGNED NULL DEFAULT NULL,
  ADD COLUMN housing_disable_free_placement TINYINT UNSIGNED NULL DEFAULT NULL,
  ADD CONSTRAINT chk_housing_disable_rotation CHECK (housing_disable_rotation IS NULL OR housing_disable_rotation IN (0,1)),
  ADD CONSTRAINT chk_housing_disable_free_placement CHECK (housing_disable_free_placement IS NULL OR housing_disable_free_placement IN (0,1));
