-- Brent Harnak, Guildhall Accessories: six authentic assistant contracts.
-- Device66646 is an explicitly labelled additional SERVER STOCK POLICY; its
-- original direct vendor is unknown, but it is a genuine original item and
-- appears in Collection of Standard Guild Services66653. This avoids requiring
-- a GM grant while full package acquisition is outside this service addition.
-- Source: https://everquest.allakhazam.com/db/npc.html?id=38576
-- Dated November2011 location is /loc Y=-2700,X=2050,Z=2; EQEmu uses X,Y,Z.
-- The original NPC appearance/heading is not recovered: this explicit server
-- equivalent clones the existing human merchant712022 and uses heading0.
-- Six stock items retain their existing1000-copper base prices and normal
-- merchant price calculations. No claim of an exact universal purchase quote.
-- The seventh slot does NOT claim original Brent stock for the device.
-- No existing NPC, stock, price or spawnpoint is overwritten.
-- Stop all world/zones before applying. Matching service-contract seed is first.
DELIMITER //
DROP PROCEDURE IF EXISTS housing_service_vendor_20261021//
CREATE PROCEDURE housing_service_vendor_20261021()
BEGIN
 DECLARE n INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; DROP TEMPORARY TABLE IF EXISTS housing_service_vendor_stage; DROP TEMPORARY TABLE IF EXISTS housing_service_spawn_stage; RESIGNAL; END;
 START TRANSACTION;
 SELECT COUNT(*) INTO n FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN('npc_types','merchantlist','spawngroup','spawnentry','spawn2','items') AND engine='InnoDB';
 IF n<>6 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service merchant requires six InnoDB source tables'; END IF;
 SELECT COUNT(*) INTO n FROM items WHERE id IN(66639,66640,66641,66642,66643,66644,66646) AND itemclass=0 AND bagslots=0 AND stackable=0 AND stacksize=1 AND price=1000 AND sellrate=1 AND UNK224=2 AND UNK228=1065353216 AND UNK229=1065353216 AND ((id<>66646 AND UNK231=-1) OR (id=66646 AND UNK231=1)) FOR UPDATE;
 IF n<>7 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service vendor item metadata/price differs'; END IF;
 SELECT COUNT(*) INTO n FROM npc_types WHERE id=712022 AND BINARY name=BINARY '#Lorekeeper_Arbel_Neklene' AND race=1 AND gender=0 AND size=6 AND `class`=41 AND merchant_id=712022 AND bodytype=1 FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing merchant appearance source differs'; END IF;
 SELECT COUNT(*) INTO n FROM npc_types WHERE id=712091 FOR UPDATE;
 IF n=0 THEN
  CREATE TEMPORARY TABLE housing_service_vendor_stage LIKE npc_types;
  INSERT INTO housing_service_vendor_stage SELECT * FROM npc_types WHERE id=712022;
  UPDATE housing_service_vendor_stage SET id=712091,name='Brent_Harnak',lastname='Guildhall Accessories',merchant_id=712091;
  INSERT INTO npc_types SELECT * FROM housing_service_vendor_stage;
  DROP TEMPORARY TABLE housing_service_vendor_stage;
 ELSE
  SELECT COUNT(*) INTO n FROM npc_types WHERE id=712091 AND BINARY name=BINARY 'Brent_Harnak' AND BINARY lastname=BINARY 'Guildhall Accessories' AND race=1 AND gender=0 AND size=6 AND `class`=41 AND merchant_id=712091 AND bodytype=1;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing Brent NPC slot712091 conflicts'; END IF;
 END IF;
 SELECT COUNT(*) INTO n FROM merchantlist WHERE merchantid=712091 FOR UPDATE;
 IF n=0 THEN
  INSERT INTO merchantlist(merchantid,slot,item) VALUES(712091,1,66639),(712091,2,66640),(712091,3,66641),(712091,4,66642),(712091,5,66643),(712091,6,66644),(712091,7,66646);
 ELSE
  IF n<>7 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing Brent merchant stock conflicts'; END IF;
  SELECT COUNT(*) INTO n FROM merchantlist WHERE merchantid=712091 AND ((slot BETWEEN 1 AND 6 AND item=66638+slot) OR (slot=7 AND item=66646)) AND faction_required=-100 AND level_required=0 AND min_status=0 AND max_status=255 AND alt_currency_cost=0 AND classes_required=65535 AND probability=100 AND bucket_name='' AND bucket_value='' AND bucket_comparison=0 AND min_expansion=-1 AND max_expansion=-1 AND content_flags IS NULL AND content_flags_disabled IS NULL;
  IF n<>7 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing Brent stock policy differs'; END IF;
 END IF;
 SELECT COUNT(*) INTO n FROM spawngroup WHERE id=712091 OR BINARY name=BINARY 'Brent_Harnak_Housing_Service_Contracts_20261021' FOR UPDATE;
 IF n=0 THEN INSERT INTO spawngroup(id,name) VALUES(712091,'Brent_Harnak_Housing_Service_Contracts_20261021');
 ELSE
  SELECT COUNT(*) INTO n FROM spawngroup WHERE id=712091 AND BINARY name=BINARY 'Brent_Harnak_Housing_Service_Contracts_20261021' AND spawn_limit=0 AND dist=0 AND max_x=0 AND min_x=0 AND max_y=0 AND min_y=0 AND despawn=0 AND wp_spawns=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing Brent spawn group conflicts'; END IF;
 END IF;
 SELECT COUNT(*) INTO n FROM spawnentry WHERE spawngroupID=712091 FOR UPDATE;
 IF n=0 THEN INSERT INTO spawnentry(spawngroupID,npcID,chance) VALUES(712091,712091,100);
 ELSE
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing Brent spawn entries conflict'; END IF;
  SELECT COUNT(*) INTO n FROM spawnentry WHERE spawngroupID=712091 AND npcID=712091 AND chance=100 AND condition_value_filter=1 AND min_time=0 AND max_time=0 AND min_expansion=-1 AND max_expansion=-1 AND content_flags IS NULL AND content_flags_disabled IS NULL;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing Brent spawn entry policy differs'; END IF;
 END IF;
 SELECT COUNT(*) INTO n FROM spawn2 WHERE id=138007 AND zone='neighborhood' AND version=0 AND x=2015.625 AND y=-2849.125 AND z=3.25 FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing merchant zone source differs'; END IF;
 SELECT COUNT(*) INTO n FROM spawn2 WHERE id=712091 OR spawngroupID=712091 FOR UPDATE;
 IF n=0 THEN
  CREATE TEMPORARY TABLE housing_service_spawn_stage LIKE spawn2;
  INSERT INTO housing_service_spawn_stage SELECT * FROM spawn2 WHERE id=138007;
  UPDATE housing_service_spawn_stage SET id=712091,spawngroupID=712091,x=2050,y=-2700,z=2,heading=0,pathgrid=0,_condition=0,cond_value=1,animation=0;
  INSERT INTO spawn2 SELECT * FROM housing_service_spawn_stage;
  DROP TEMPORARY TABLE housing_service_spawn_stage;
 ELSE
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing Brent spawnpoint conflicts'; END IF;
  SELECT COUNT(*) INTO n FROM spawn2 WHERE id=712091 AND spawngroupID=712091 AND zone='neighborhood' AND version=0 AND x=2050 AND y=-2700 AND z=2 AND heading=0 AND pathgrid=0 AND _condition=0 AND cond_value=1 AND animation=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing Brent spawnpoint policy differs'; END IF;
 END IF;
 COMMIT;
END//
CALL housing_service_vendor_20261021()//
DROP PROCEDURE housing_service_vendor_20261021//
DELIMITER ;
