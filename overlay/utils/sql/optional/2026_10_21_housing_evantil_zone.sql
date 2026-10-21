-- Register the missing server zone for reviewed Evantil's Abode content.
-- Original May2013 client registration identifies zone766/phinteriortree and
-- long name Evantil's Abode; original EQG/map hashes are pinned by the separately
-- reviewed house_variant_geometry.json. This does not install client assets.
-- Safe0,0,4 is a source-map-checked TECHNICAL entry, not a recovered Live spawn.
-- No-bind/noncombat, housing ztype0, bounds/clip, expansion-check bypass and
-- suspended buffs are EXPLICIT emulator housing policies; other columns use
-- this installation's ordinary zone schema defaults, not asserted Live values.
-- No existing zone row is overwritten. Conflicts/duplicate routes fail closed.
-- Stop conflicting metadata writers; refresh world/zone caches after success.
DELIMITER //
DROP PROCEDURE IF EXISTS housing_evantil_zone_20261021//
CREATE PROCEDURE housing_evantil_zone_20261021()
BEGIN
 DECLARE rows_found INT DEFAULT 0;
 DECLARE exact_matches INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 START TRANSACTION;
 SELECT COUNT(*) INTO rows_found FROM information_schema.tables
 WHERE table_schema=DATABASE() AND table_name='zone' AND engine='InnoDB';
 IF rows_found<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Evantil zone registration requires InnoDB zone table';
 END IF;
 SELECT COUNT(*) INTO rows_found FROM zone
 WHERE zoneidnumber=766 OR short_name='phinteriortree' FOR UPDATE;
 IF rows_found=0 THEN
  INSERT INTO zone(zoneidnumber,version,short_name,long_name,map_file_name,file_name,
   safe_x,safe_y,safe_z,safe_heading,min_status,canbind,cancombat,ztype,
   underworld,minclip,maxclip,bypass_expansion_check,suspendbuffs,note)
  VALUES(766,0,'phinteriortree','Evantil''s Abode','phinteriortree','phinteriortree',
   0,0,4,0,0,0,0,0,-1000,50,2200,1,1,
   'Housing20261021: original RoF2 zone identity; source-map technical safe0,0,4 and explicit emulator housing policy');
 ELSE
  SELECT COUNT(*) INTO exact_matches FROM zone WHERE zoneidnumber=766 AND version=0
   AND BINARY short_name=BINARY 'phinteriortree' AND BINARY long_name=BINARY 'Evantil''s Abode'
   AND BINARY map_file_name=BINARY 'phinteriortree' AND BINARY file_name=BINARY 'phinteriortree'
   AND safe_x=0 AND safe_y=0 AND safe_z=4 AND safe_heading=0 AND min_status=0
   AND canbind=0 AND cancombat=0 AND ztype=0 AND underworld=-1000
   AND minclip=50 AND maxclip=2200 AND bypass_expansion_check=1 AND suspendbuffs=1
   AND BINARY note=BINARY 'Housing20261021: original RoF2 zone identity; source-map technical safe0,0,4 and explicit emulator housing policy';
  IF rows_found<>1 OR exact_matches<>1 THEN
   SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Existing Evantil zone ID/name conflicts with reviewed registration; nothing overwritten';
  END IF;
 END IF;
 COMMIT;
END//
CALL housing_evantil_zone_20261021()//
DROP PROCEDURE housing_evantil_zone_20261021//
DELIMITER ;
