-- Original guild frame models for the four reconstructed Sunrise sites.
-- Adds only previously unused neighborhood/version0 static IDs199..202.
-- Coordinate/heading reconstruction is documented in sunrise_neighborhood_defaults.md.
-- Does not create plots or replace existing personal markers/catalogues.
DELIMITER //
CREATE PROCEDURE housing_add_sunrise_guild_markers_1018()
BEGIN
 DECLARE conflict_count INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION
 BEGIN
  ROLLBACK;
  RESIGNAL;
 END;
 START TRANSACTION;
 SELECT COUNT(*) INTO conflict_count FROM doors WHERE zone='neighborhood' AND version=0 AND doorid=199 AND (name<>'OBP_GUILDSQUARE' OR ABS(pos_x-(2743.542))>0.01 OR ABS(pos_y-(2612.9705))>0.01 OR ABS(pos_z-(110))>0.01 OR ABS(heading-(128.0))>0.01 OR opentype<>160 OR size<>100 OR door_param NOT IN(-1,0));
 IF conflict_count<>0 THEN ROLLBACK; SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Sunrise guild marker identity collision'; END IF;
 INSERT INTO doors(doorid,zone,version,name,pos_x,pos_y,pos_z,heading,opentype,size,door_param) SELECT 199,'neighborhood',0,'OBP_GUILDSQUARE',2743.542,2612.9705,110,128.0,160,100,-1 WHERE NOT EXISTS(SELECT 1 FROM doors WHERE zone='neighborhood' AND version=0 AND doorid=199);
 SELECT COUNT(*) INTO conflict_count FROM doors WHERE zone='neighborhood' AND version=0 AND doorid=200 AND (name<>'OBP_GUILDSQUARE' OR ABS(pos_x-(-1104.2726))>0.01 OR ABS(pos_y-(2418.262))>0.01 OR ABS(pos_z-(110))>0.01 OR ABS(heading-(128.0))>0.01 OR opentype<>160 OR size<>100 OR door_param NOT IN(-1,0));
 IF conflict_count<>0 THEN ROLLBACK; SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Sunrise guild marker identity collision'; END IF;
 INSERT INTO doors(doorid,zone,version,name,pos_x,pos_y,pos_z,heading,opentype,size,door_param) SELECT 200,'neighborhood',0,'OBP_GUILDSQUARE',-1104.2726,2418.262,110,128.0,160,100,-1 WHERE NOT EXISTS(SELECT 1 FROM doors WHERE zone='neighborhood' AND version=0 AND doorid=200);
 SELECT COUNT(*) INTO conflict_count FROM doors WHERE zone='neighborhood' AND version=0 AND doorid=201 AND (name<>'OBP_GUILDSQUARE' OR ABS(pos_x-(2993.658))>0.01 OR ABS(pos_y-(-765.9966))>0.01 OR ABS(pos_z-(0))>0.01 OR ABS(heading-(128.0))>0.01 OR opentype<>160 OR size<>100 OR door_param NOT IN(-1,0));
 IF conflict_count<>0 THEN ROLLBACK; SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Sunrise guild marker identity collision'; END IF;
 INSERT INTO doors(doorid,zone,version,name,pos_x,pos_y,pos_z,heading,opentype,size,door_param) SELECT 201,'neighborhood',0,'OBP_GUILDSQUARE',2993.658,-765.9966,0,128.0,160,100,-1 WHERE NOT EXISTS(SELECT 1 FROM doors WHERE zone='neighborhood' AND version=0 AND doorid=201);
 SELECT COUNT(*) INTO conflict_count FROM doors WHERE zone='neighborhood' AND version=0 AND doorid=202 AND (name<>'OBP_GUILDSQUARE' OR ABS(pos_x-(571.65))>0.01 OR ABS(pos_y-(-2227.774825))>0.01 OR ABS(pos_z-(0))>0.01 OR ABS(heading-(508.9409912886563))>0.01 OR opentype<>160 OR size<>100 OR door_param NOT IN(-1,0));
 IF conflict_count<>0 THEN ROLLBACK; SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Sunrise guild marker identity collision'; END IF;
 INSERT INTO doors(doorid,zone,version,name,pos_x,pos_y,pos_z,heading,opentype,size,door_param) SELECT 202,'neighborhood',0,'OBP_GUILDSQUARE',571.65,-2227.774825,0,508.9409912886563,160,100,-1 WHERE NOT EXISTS(SELECT 1 FROM doors WHERE zone='neighborhood' AND version=0 AND doorid=202);
 UPDATE doors SET door_param=-1 WHERE zone='neighborhood' AND version=0 AND doorid BETWEEN 199 AND 202 AND name='OBP_GUILDSQUARE' AND door_param=0;
 COMMIT;
END//
DELIMITER ;
CALL housing_add_sunrise_guild_markers_1018();
DROP PROCEDURE housing_add_sunrise_guild_markers_1018;
