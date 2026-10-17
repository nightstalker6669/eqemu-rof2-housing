-- Optional additive per-reservation server-computed content identity.
-- Existing rows remain bothNULL. No backfill, catalog, geometry or Live seeds.
ALTER TABLE housing_live_placement_reservations
 ADD COLUMN IF NOT EXISTS content_binding_version SMALLINT UNSIGNED NULL,
 ADD COLUMN IF NOT EXISTS content_binding_digest BINARY(32) NULL;
-- Runtime validates bothNULL legacy exact-global-revision semantics or supported
-- version1 plus exactly32 bytes. Partial/unknown values fail closed.
