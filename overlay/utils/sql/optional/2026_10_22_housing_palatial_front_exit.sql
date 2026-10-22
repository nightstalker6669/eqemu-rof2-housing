-- Original RoF2 FrontEntrance mesh; reviewed XY/yaw, collision floor/lintel Z.
-- Explicit emulator switch-pose reconstruction; not a captured Live door record.
-- Server policy: type58 clickable exit, RoF2 mask64, parent-bound destination.
-- Source geometry and transform reviewed; no client asset changes.
DROP PROCEDURE IF EXISTS housing_palatial_front_exit_20261022;
DELIMITER $$
CREATE PROCEDURE housing_palatial_front_exit_20261022()
BEGIN
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 START TRANSACTION;
 IF EXISTS(SELECT 1 FROM doors WHERE zone='guildhalllrg' AND version=0 AND doorid=1 AND
  (name<>'OBJ_FRONTENTRANCE' OR ABS(pos_x-88.89984894)>0.05 OR ABS(pos_y-0.68527031)>0.05 OR (ABS(pos_z+13.65774345)>0.05 AND ABS(pos_z-8.25539354)>0.05) OR heading<>384 OR opentype<>58 OR size<>100 OR client_version_mask<>64 OR dest_zone<>'NONE')) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Custom Palatial door1 conflicts with reviewed front entrance';
 END IF;
 INSERT INTO doors(doorid,zone,version,name,pos_x,pos_y,pos_z,heading,opentype,size,client_version_mask,dest_zone)
 SELECT 1,'guildhalllrg',0,'OBJ_FRONTENTRANCE',88.89984894,0.68527031,8.25539354,384,58,100,64,'NONE'
 WHERE NOT EXISTS(SELECT 1 FROM doors WHERE zone='guildhalllrg' AND version=0 AND doorid=1);
 UPDATE doors SET pos_z=8.25539354 WHERE zone='guildhalllrg' AND version=0 AND doorid=1 AND name='OBJ_FRONTENTRANCE' AND ABS(pos_z+13.65774345)<0.05;
 -- Only replace our known experimental labels; preserve operator names.
 UPDATE zone SET long_name='Palatial Guild Hall' WHERE zoneidnumber=737 AND version=0 AND short_name='guildhalllrg' AND long_name='EXPERIMENTAL original guildhalllrg interior';
 UPDATE zone SET long_name='Grand Guild Hall' WHERE zoneidnumber=738 AND version=0 AND short_name='guildhallsml' AND long_name='EXPERIMENTAL original guildhallsml interior';
 COMMIT;
END$$
DELIMITER ;
CALL housing_palatial_front_exit_20261022();
DROP PROCEDURE housing_palatial_front_exit_20261022;
