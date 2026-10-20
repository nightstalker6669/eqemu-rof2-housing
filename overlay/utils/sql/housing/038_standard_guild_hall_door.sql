-- Optional RoF2 Standard housing-hall model door visibility.
-- The C++ click hook independently proves a current permanent housing hall and
-- resolves its source-bound safe exit. Legacy Lobby routing fields are retained.
-- Do not activate the duplicate GUILD_DOOR or alter basic version1 halls.
-- Exact known model/type/position guards avoid replacing customized door rows.
UPDATE doors
SET client_version_mask = client_version_mask | 64
WHERE zone = 'guildhall' AND version = 0 AND doorid = 4
  AND name = 'OBJ_GUILD_DOOR' AND opentype = 58
  AND ABS(pos_x - (-3.4)) < 0.01 AND ABS(pos_y - (-162.9)) < 0.01
  AND ABS(pos_z - 6.9) < 0.01 AND ABS(heading - 128) < 0.01
  AND client_version_mask IN (0, 64);
