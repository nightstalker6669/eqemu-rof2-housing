-- Supported original guild-room zone registration / normal-player access.
-- Original client IDs/short names:737guildhalllrg,738guildhallsml,751guildhall3.
-- Registered zone long names are independent of the purchased hall SKU:
-- reviewed physical Palatial66620 routes737; Grand66619 routes738; Modest66660 routes751.
-- min_status0 is EXPLICIT operator housing-access policy, NOT a recovered
-- historical status limit. Existing matched rows change ONLY255->0; other
-- columns, safe points, auto IDs and historical room bindings stay untouched.
-- Missing rows receive explicitly technical housing defaults and the existing
-- reviewed entry references737(0,0,4.13),738/751(60,100,-17.9), not Live spawns.
-- Grand capacity is a separately approved configurable default, not a recovered Live value.
-- Stop conflicting zone-metadata writers; refresh world/zone caches afterward.
DELIMITER //
DROP PROCEDURE IF EXISTS housing_supported_guild_zones_20261021//
CREATE PROCEDURE housing_supported_guild_zones_20261021()
BEGIN
 DECLARE rows_found INT DEFAULT 0;
 DECLARE identity_matches INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 START TRANSACTION;
 SELECT COUNT(*) INTO rows_found FROM information_schema.tables
 WHERE table_schema=DATABASE() AND table_name='zone' AND engine='InnoDB';
 IF rows_found<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Supported guild-room registration requires InnoDB zone table';
 END IF;
 SELECT COUNT(*) INTO rows_found FROM zone
 WHERE zoneidnumber=737 OR short_name='guildhalllrg' FOR UPDATE;
 IF rows_found=0 THEN
  INSERT INTO zone(zoneidnumber,version,short_name,long_name,map_file_name,file_name,
   safe_x,safe_y,safe_z,safe_heading,min_status,canbind,cancombat,ztype,
   underworld,minclip,maxclip,bypass_expansion_check,suspendbuffs,note)
  VALUES(737,0,'guildhalllrg','Grand Guild Hall','guildhalllrg','guildhalllrg',
   0,0,4.13,0,0,0,0,0,-1000,50,2200,1,1,
   'Housing20261021: supported original guild-room identity; explicit normal-player access and technical housing defaults');
 ELSE
  SELECT COUNT(*) INTO identity_matches FROM zone WHERE zoneidnumber=737 AND version=0
   AND BINARY short_name=BINARY 'guildhalllrg'
   AND (BINARY long_name=BINARY 'Grand Guild Hall' OR BINARY long_name=BINARY 'EXPERIMENTAL original guildhalllrg interior')
   AND BINARY map_file_name=BINARY 'guildhalllrg' AND BINARY file_name=BINARY 'guildhalllrg'
   AND min_status IN(0,255);
  IF rows_found<>1 OR identity_matches<>1 THEN
   SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Guild-room 737 identity/version/status conflicts with reviewed rows; no changes committed';
  END IF;
  UPDATE zone SET min_status=0 WHERE zoneidnumber=737 AND version=0 AND min_status=255;
 END IF;
 SELECT COUNT(*) INTO rows_found FROM zone
 WHERE zoneidnumber=738 OR short_name='guildhallsml' FOR UPDATE;
 IF rows_found=0 THEN
  INSERT INTO zone(zoneidnumber,version,short_name,long_name,map_file_name,file_name,
   safe_x,safe_y,safe_z,safe_heading,min_status,canbind,cancombat,ztype,
   underworld,minclip,maxclip,bypass_expansion_check,suspendbuffs,note)
  VALUES(738,0,'guildhallsml','Greater Guild Hall','guildhallsml','guildhallsml',
   60,100,-17.9,0,0,0,0,0,-1000,50,2200,1,1,
   'Housing20261021: supported original guild-room identity; explicit normal-player access and technical housing defaults');
 ELSE
  SELECT COUNT(*) INTO identity_matches FROM zone WHERE zoneidnumber=738 AND version=0
   AND BINARY short_name=BINARY 'guildhallsml'
   AND (BINARY long_name=BINARY 'Greater Guild Hall' OR BINARY long_name=BINARY 'EXPERIMENTAL original guildhallsml interior')
   AND BINARY map_file_name=BINARY 'guildhallsml' AND BINARY file_name=BINARY 'guildhallsml'
   AND min_status IN(0,255);
  IF rows_found<>1 OR identity_matches<>1 THEN
   SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Guild-room 738 identity/version/status conflicts with reviewed rows; no changes committed';
  END IF;
  UPDATE zone SET min_status=0 WHERE zoneidnumber=738 AND version=0 AND min_status=255;
 END IF;
 SELECT COUNT(*) INTO rows_found FROM zone
 WHERE zoneidnumber=751 OR short_name='guildhall3' FOR UPDATE;
 IF rows_found=0 THEN
  INSERT INTO zone(zoneidnumber,version,short_name,long_name,map_file_name,file_name,
   safe_x,safe_y,safe_z,safe_heading,min_status,canbind,cancombat,ztype,
   underworld,minclip,maxclip,bypass_expansion_check,suspendbuffs,note)
  VALUES(751,0,'guildhall3','Modest Guild Hall','guildhall3','guildhall3',
   60,100,-17.9,0,0,0,0,0,-1000,50,2200,1,1,
   'Housing20261021: supported original guild-room identity; explicit normal-player access and technical housing defaults');
 ELSE
  SELECT COUNT(*) INTO identity_matches FROM zone WHERE zoneidnumber=751 AND version=0
   AND BINARY short_name=BINARY 'guildhall3'
   AND (BINARY long_name=BINARY 'Modest Guild Hall' OR BINARY long_name=BINARY 'EXPERIMENTAL original guildhall3 interior')
   AND BINARY map_file_name=BINARY 'guildhall3' AND BINARY file_name=BINARY 'guildhall3'
   AND min_status IN(0,255);
  IF rows_found<>1 OR identity_matches<>1 THEN
   SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Guild-room 751 identity/version/status conflicts with reviewed rows; no changes committed';
  END IF;
  UPDATE zone SET min_status=0 WHERE zoneidnumber=751 AND version=0 AND min_status=255;
 END IF;
 COMMIT;
END//
CALL housing_supported_guild_zones_20261021()//
DROP PROCEDURE housing_supported_guild_zones_20261021//
DELIMITER ;
