-- Retain v1 historical house effects; v2 carries old/new catalog provenance
-- while preserving the native interior and permanent instance identity.
ALTER TABLE housing_house_receipt_effects
 DROP CONSTRAINT IF EXISTS housing_house_effect_version,
 ADD CONSTRAINT housing_house_effect_version CHECK(effects_version IN (1,2));
