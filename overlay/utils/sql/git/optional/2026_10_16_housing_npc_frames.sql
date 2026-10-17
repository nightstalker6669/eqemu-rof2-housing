-- Unseeded per-placement native NPC frame. Original actor origin is distinct
-- from submitted ground position and from another viewer's mutable model mode.
-- Rows retire on Return; custody/native identities and last frame remain history.
CREATE TABLE IF NOT EXISTS housing_live_npc_frames (
 custody_id BIGINT UNSIGNED NOT NULL,
 native_item_id INT UNSIGNED NOT NULL,
 pose_revision BIGINT UNSIGNED NOT NULL,
 height_bits INT UNSIGNED NOT NULL,
 origin_z_bits INT UNSIGNED NOT NULL,
 active TINYINT UNSIGNED NOT NULL DEFAULT 1,
 PRIMARY KEY(custody_id), UNIQUE KEY native_item_id(native_item_id)
) ENGINE=InnoDB;
