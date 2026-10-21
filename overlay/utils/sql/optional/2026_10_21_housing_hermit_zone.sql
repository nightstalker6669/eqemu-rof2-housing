-- Register723/phinterior1d1 for the reviewed reconstructed Hermit54999 route.
-- Exact original May2013 registered long name is Dragon House Interior.
-- Item-to-zone association is reconstructed from original dragon-themedgeometry,
-- retained authenticdragon-loveritemlore and later primary723name association.
-- NOT a recovered May2013 server definition route or original Live spawn.
-- Safe0,0,4 passed actualMap floor and105boundedclearance-ray checks.
-- No-bind/noncombat, clip bounds, suspension and bypass are explicit emulatorpolicy.
-- Only insert when absent; exact policy postimage is idempotent. Conflicts refuse.
-- Other columns retain installation schema defaults; no active deployment here.
DELIMITER //
DROP PROCEDURE IF EXISTS housing_hermit_zone_20261021//
CREATE PROCEDURE housing_hermit_zone_20261021()
BEGIN
 DECLARE rows_found INT DEFAULT 0;
 DECLARE exact_matches INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 START TRANSACTION;
 SELECT COUNT(*) INTO rows_found FROM information_schema.tables
 WHERE table_schema=DATABASE() AND table_name='zone' AND engine='InnoDB';
 IF rows_found<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Hermit zone registration requires InnoDB zone table';
 END IF;
 SELECT COUNT(*) INTO rows_found FROM zone
 WHERE zoneidnumber=723 OR short_name='phinterior1d1' FOR UPDATE;
 IF rows_found=0 THEN
  INSERT INTO zone(zoneidnumber,version,short_name,long_name,map_file_name,file_name,
   safe_x,safe_y,safe_z,safe_heading,min_status,canbind,cancombat,ztype,
   underworld,minclip,maxclip,bypass_expansion_check,suspendbuffs,note)
  VALUES(723,0,'phinterior1d1','Dragon House Interior','phinterior1d1','phinterior1d1',
   0,0,4,0,0,0,0,0,-1000,50,2200,1,1,
   'Housing20261021: original RoF2 zone identity; source-map technical safe0,0,4 and explicit emulator housing policy');
 ELSE
  SELECT COUNT(*) INTO exact_matches FROM zone WHERE zoneidnumber=723 AND version=0
   AND BINARY short_name=BINARY 'phinterior1d1' AND BINARY long_name=BINARY 'Dragon House Interior'
   AND BINARY map_file_name=BINARY 'phinterior1d1' AND BINARY file_name=BINARY 'phinterior1d1'
   AND safe_x=0 AND safe_y=0 AND safe_z=4 AND safe_heading=0 AND min_status=0
   AND canbind=0 AND cancombat=0 AND ztype=0 AND underworld=-1000
   AND minclip=50 AND maxclip=2200 AND bypass_expansion_check=1 AND suspendbuffs=1
   AND BINARY note=BINARY 'Housing20261021: original RoF2 zone identity; source-map technical safe0,0,4 and explicit emulator housing policy';
  IF rows_found<>1 OR exact_matches<>1 THEN
   SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Existing Hermit zone ID/name conflicts with reviewed registration; nothing overwritten';
  END IF;
 END IF;
 COMMIT;
END//
CALL housing_hermit_zone_20261021()//
DROP PROCEDURE housing_hermit_zone_20261021//
DELIMITER ;
