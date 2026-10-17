-- Additive, unseeded reviewed actor input. Requires frozen NPC metadata schema.
-- Bits are native float32 Actor.NPCHeight, NOT the derived entity collision height.
ALTER TABLE housing_native_npc_item_sources ADD COLUMN IF NOT EXISTS actor_height_bits INT UNSIGNED NOT NULL DEFAULT 0;
-- Zero preserves metadata-only behavior and does not enable a runtime class.
