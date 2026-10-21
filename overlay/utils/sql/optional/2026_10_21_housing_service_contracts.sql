-- Real guild-hall purchased service contracts. No automatic/default placement.
-- Key/interactive/opaque respawn/scale metadata is later collector evidence.
-- Appearance and size are labelled server equivalents of basic guild-hall NPCs;
-- the pad uses original I28/706 resources under a reviewed item association.
-- Stop all world/zones and item writers before applying; rebuild shared items.
-- Prerequisites: housing NPC metadata/runtime schemas and matching source code.
-- No existing estate, custody, inventory, spawnpoint or merchant row is changed.
DELIMITER //
DROP PROCEDURE IF EXISTS housing_service_contracts_20261021//
CREATE PROCEDURE housing_service_contracts_20261021()
BEGIN
 DECLARE n INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; DROP TEMPORARY TABLE IF EXISTS housing_service_npc_stage; RESIGNAL; END;
 START TRANSACTION;
 SELECT COUNT(*) INTO n FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN('items','npc_types','housing_native_npc_item_sources') AND engine='InnoDB';
 IF n<>3 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing services require InnoDB items, NPC and metadata tables'; END IF;
 SELECT COUNT(*) INTO n FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_native_npc_item_sources' AND column_name='actor_height_bits' AND data_type='int' AND column_type LIKE '%unsigned' AND is_nullable='NO';
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing services require NPC runtime column'; END IF;
 SELECT COUNT(*) INTO n FROM items WHERE id=66639 AND BINARY name=BINARY 'Friendly Guild General Supply Merchant' AND BINARY idfile=BINARY 'IT63'
 AND itemclass=0 AND bagslots=0 AND stackable=0 AND stacksize=1 AND evoitem=0 AND UNK225=0 AND UNK226=0 AND UNK227=0 AND UNK230=0
 AND housing_disable_rotation IS NULL AND housing_disable_free_placement IS NULL
 AND ((UNK224=0 AND UNK228=0 AND UNK229=0 AND UNK231=0) OR (UNK224=2 AND UNK228=1065353216 AND UNK229=1065353216 AND UNK231=-1)) FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service item 66639 differs; no changes made'; END IF;
 SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66639 FOR UPDATE;
 IF n=0 THEN
  INSERT INTO housing_native_npc_item_sources(item_id,housepet_filename,interactive_object,npc_respawn_interval,source_sha256,actor_height_bits) VALUES(66639,'NGH_Merchant_General',1,750,UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5'),0);
 ELSE
  SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66639 AND BINARY housepet_filename=BINARY 'NGH_Merchant_General' AND interactive_object=1 AND npc_respawn_interval=750 AND source_sha256=UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5') AND actor_height_bits=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service source 66639 conflicts; no changes made'; END IF;
 END IF;
 UPDATE items SET UNK224=2,UNK228=1065353216,UNK229=1065353216,UNK231=-1 WHERE id=66639 AND UNK224=0;
 SELECT COUNT(*) INTO n FROM items WHERE id=66640 AND BINARY name=BINARY 'Friendly Guild Teleportation Assistant' AND BINARY idfile=BINARY 'IT63'
 AND itemclass=0 AND bagslots=0 AND stackable=0 AND stacksize=1 AND evoitem=0 AND UNK225=0 AND UNK226=0 AND UNK227=0 AND UNK230=0
 AND housing_disable_rotation IS NULL AND housing_disable_free_placement IS NULL
 AND ((UNK224=0 AND UNK228=0 AND UNK229=0 AND UNK231=0) OR (UNK224=2 AND UNK228=1065353216 AND UNK229=1065353216 AND UNK231=-1)) FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service item 66640 differs; no changes made'; END IF;
 SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66640 FOR UPDATE;
 IF n=0 THEN
  INSERT INTO housing_native_npc_item_sources(item_id,housepet_filename,interactive_object,npc_respawn_interval,source_sha256,actor_height_bits) VALUES(66640,'NGH_Teleport_Assistant',1,750,UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5'),0);
 ELSE
  SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66640 AND BINARY housepet_filename=BINARY 'NGH_Teleport_Assistant' AND interactive_object=1 AND npc_respawn_interval=750 AND source_sha256=UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5') AND actor_height_bits=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service source 66640 conflicts; no changes made'; END IF;
 END IF;
 UPDATE items SET UNK224=2,UNK228=1065353216,UNK229=1065353216,UNK231=-1 WHERE id=66640 AND UNK224=0;
 SELECT COUNT(*) INTO n FROM items WHERE id=66641 AND BINARY name=BINARY 'Friendly Guild Personal-Banker' AND BINARY idfile=BINARY 'IT63'
 AND itemclass=0 AND bagslots=0 AND stackable=0 AND stacksize=1 AND evoitem=0 AND UNK225=0 AND UNK226=0 AND UNK227=0 AND UNK230=0
 AND housing_disable_rotation IS NULL AND housing_disable_free_placement IS NULL
 AND ((UNK224=0 AND UNK228=0 AND UNK229=0 AND UNK231=0) OR (UNK224=2 AND UNK228=1065353216 AND UNK229=1065353216 AND UNK231=-1)) FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service item 66641 differs; no changes made'; END IF;
 SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66641 FOR UPDATE;
 IF n=0 THEN
  INSERT INTO housing_native_npc_item_sources(item_id,housepet_filename,interactive_object,npc_respawn_interval,source_sha256,actor_height_bits) VALUES(66641,'NGH_Banker_Personal',1,750,UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5'),0);
 ELSE
  SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66641 AND BINARY housepet_filename=BINARY 'NGH_Banker_Personal' AND interactive_object=1 AND npc_respawn_interval=750 AND source_sha256=UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5') AND actor_height_bits=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service source 66641 conflicts; no changes made'; END IF;
 END IF;
 UPDATE items SET UNK224=2,UNK228=1065353216,UNK229=1065353216,UNK231=-1 WHERE id=66641 AND UNK224=0;
 SELECT COUNT(*) INTO n FROM items WHERE id=66642 AND BINARY name=BINARY 'Friendly Guild Guild-Banker' AND BINARY idfile=BINARY 'IT63'
 AND itemclass=0 AND bagslots=0 AND stackable=0 AND stacksize=1 AND evoitem=0 AND UNK225=0 AND UNK226=0 AND UNK227=0 AND UNK230=0
 AND housing_disable_rotation IS NULL AND housing_disable_free_placement IS NULL
 AND ((UNK224=0 AND UNK228=0 AND UNK229=0 AND UNK231=0) OR (UNK224=2 AND UNK228=1065353216 AND UNK229=1065353216 AND UNK231=-1)) FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service item 66642 differs; no changes made'; END IF;
 SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66642 FOR UPDATE;
 IF n=0 THEN
  INSERT INTO housing_native_npc_item_sources(item_id,housepet_filename,interactive_object,npc_respawn_interval,source_sha256,actor_height_bits) VALUES(66642,'NGH_Banker_Guild',1,750,UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5'),0);
 ELSE
  SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66642 AND BINARY housepet_filename=BINARY 'NGH_Banker_Guild' AND interactive_object=1 AND npc_respawn_interval=750 AND source_sha256=UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5') AND actor_height_bits=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service source 66642 conflicts; no changes made'; END IF;
 END IF;
 UPDATE items SET UNK224=2,UNK228=1065353216,UNK229=1065353216,UNK231=-1 WHERE id=66642 AND UNK224=0;
 SELECT COUNT(*) INTO n FROM items WHERE id=66643 AND BINARY name=BINARY 'Friendly Guild Guild-Tribute Master' AND BINARY idfile=BINARY 'IT63'
 AND itemclass=0 AND bagslots=0 AND stackable=0 AND stacksize=1 AND evoitem=0 AND UNK225=0 AND UNK226=0 AND UNK227=0 AND UNK230=0
 AND housing_disable_rotation IS NULL AND housing_disable_free_placement IS NULL
 AND ((UNK224=0 AND UNK228=0 AND UNK229=0 AND UNK231=0) OR (UNK224=2 AND UNK228=1065353216 AND UNK229=1065353216 AND UNK231=-1)) FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service item 66643 differs; no changes made'; END IF;
 SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66643 FOR UPDATE;
 IF n=0 THEN
  INSERT INTO housing_native_npc_item_sources(item_id,housepet_filename,interactive_object,npc_respawn_interval,source_sha256,actor_height_bits) VALUES(66643,'NGH_Tribute_Guild',1,750,UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5'),0);
 ELSE
  SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66643 AND BINARY housepet_filename=BINARY 'NGH_Tribute_Guild' AND interactive_object=1 AND npc_respawn_interval=750 AND source_sha256=UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5') AND actor_height_bits=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service source 66643 conflicts; no changes made'; END IF;
 END IF;
 UPDATE items SET UNK224=2,UNK228=1065353216,UNK229=1065353216,UNK231=-1 WHERE id=66643 AND UNK224=0;
 SELECT COUNT(*) INTO n FROM items WHERE id=66644 AND BINARY name=BINARY 'Friendly Guild Personal-Tribute Master' AND BINARY idfile=BINARY 'IT63'
 AND itemclass=0 AND bagslots=0 AND stackable=0 AND stacksize=1 AND evoitem=0 AND UNK225=0 AND UNK226=0 AND UNK227=0 AND UNK230=0
 AND housing_disable_rotation IS NULL AND housing_disable_free_placement IS NULL
 AND ((UNK224=0 AND UNK228=0 AND UNK229=0 AND UNK231=0) OR (UNK224=2 AND UNK228=1065353216 AND UNK229=1065353216 AND UNK231=-1)) FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service item 66644 differs; no changes made'; END IF;
 SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66644 FOR UPDATE;
 IF n=0 THEN
  INSERT INTO housing_native_npc_item_sources(item_id,housepet_filename,interactive_object,npc_respawn_interval,source_sha256,actor_height_bits) VALUES(66644,'NGH_Tribute_Personal',1,750,UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5'),0);
 ELSE
  SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66644 AND BINARY housepet_filename=BINARY 'NGH_Tribute_Personal' AND interactive_object=1 AND npc_respawn_interval=750 AND source_sha256=UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5') AND actor_height_bits=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service source 66644 conflicts; no changes made'; END IF;
 END IF;
 UPDATE items SET UNK224=2,UNK228=1065353216,UNK229=1065353216,UNK231=-1 WHERE id=66644 AND UNK224=0;
 SELECT COUNT(*) INTO n FROM items WHERE id=66646 AND BINARY name=BINARY 'Guild Teleportation Device' AND BINARY idfile=BINARY 'IT63'
 AND itemclass=0 AND bagslots=0 AND stackable=0 AND stacksize=1 AND evoitem=0 AND UNK225=0 AND UNK226=0 AND UNK227=0 AND UNK230=0
 AND housing_disable_rotation IS NULL AND housing_disable_free_placement IS NULL
 AND ((UNK224=0 AND UNK228=0 AND UNK229=0 AND UNK231=0) OR (UNK224=2 AND UNK228=1065353216 AND UNK229=1065353216 AND UNK231=1)) FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service item 66646 differs; no changes made'; END IF;
 SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66646 FOR UPDATE;
 IF n=0 THEN
  INSERT INTO housing_native_npc_item_sources(item_id,housepet_filename,interactive_object,npc_respawn_interval,source_sha256,actor_height_bits) VALUES(66646,'NGH_Teleport_Device',1,750,UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5'),0);
 ELSE
  SELECT COUNT(*) INTO n FROM housing_native_npc_item_sources WHERE item_id=66646 AND BINARY housepet_filename=BINARY 'NGH_Teleport_Device' AND interactive_object=1 AND npc_respawn_interval=750 AND source_sha256=UNHEX('f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5') AND actor_height_bits=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Housing service source 66646 conflicts; no changes made'; END IF;
 END IF;
 UPDATE items SET UNK224=2,UNK228=1065353216,UNK229=1065353216,UNK231=1 WHERE id=66646 AND UNK224=0;
 -- These two templates provide missing service roles, with explicit server
 -- equivalent appearance. Existing basic guild-hall templates stay unchanged.
 SELECT COUNT(*) INTO n FROM npc_types WHERE id=345001 AND BINARY name=BINARY 'Melody_the_Singer_of_Great_Deeds' AND race=1 AND gender=1 AND size=6 AND `class`=64 AND merchant_id=0 FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Basic guild tribute template differs'; END IF;
 SELECT COUNT(*) INTO n FROM npc_types WHERE id=345002 AND BINARY name=BINARY 'a_bank_broker' AND race=1 AND gender=1 AND size=6 AND `class`=40 AND merchant_id=0 FOR UPDATE;
 IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Basic guild banker template differs'; END IF;
 SELECT COUNT(*) INTO n FROM npc_types WHERE id=345007 FOR UPDATE;
 IF n=0 THEN
  CREATE TEMPORARY TABLE housing_service_npc_stage LIKE npc_types;
  INSERT INTO housing_service_npc_stage SELECT * FROM npc_types WHERE id=345001;
  UPDATE housing_service_npc_stage SET id=345007,name='Housing_Personal_Tribute_Master',lastname='Personal Tribute Master',`class`=63;
  INSERT INTO npc_types SELECT * FROM housing_service_npc_stage;
  DROP TEMPORARY TABLE housing_service_npc_stage;
 ELSE
  SELECT COUNT(*) INTO n FROM npc_types WHERE id=345007 AND BINARY name=BINARY 'Housing_Personal_Tribute_Master' AND BINARY lastname=BINARY 'Personal Tribute Master' AND race=1 AND gender=1 AND size=6 AND `class`=63 AND merchant_id=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='NPC template345007 conflicts'; END IF;
 END IF;
 SELECT COUNT(*) INTO n FROM npc_types WHERE id=345008 FOR UPDATE;
 IF n=0 THEN
  CREATE TEMPORARY TABLE housing_service_npc_stage LIKE npc_types;
  INSERT INTO housing_service_npc_stage SELECT * FROM npc_types WHERE id=345002;
  UPDATE housing_service_npc_stage SET id=345008,name='Housing_Guild_Teleportation_Device',lastname='',race=706,gender=2,size=6,`class`=1,merchant_id=0,texture=0,helmtexture=0,herosforgemodel=0,bodytype=11,no_target_hotkey=1,findable=0,show_name=0,trackable=0;
  INSERT INTO npc_types SELECT * FROM housing_service_npc_stage;
  DROP TEMPORARY TABLE housing_service_npc_stage;
 ELSE
  SELECT COUNT(*) INTO n FROM npc_types WHERE id=345008 AND BINARY name=BINARY 'Housing_Guild_Teleportation_Device' AND BINARY lastname=BINARY '' AND race=706 AND gender=2 AND size=6 AND `class`=1 AND merchant_id=0 AND texture=0 AND helmtexture=0 AND herosforgemodel=0 AND bodytype=11 AND no_target_hotkey=1 AND findable=0 AND show_name=0 AND trackable=0;
  IF n<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='NPC template345008 conflicts'; END IF;
 END IF;
 COMMIT;
END//
CALL housing_service_contracts_20261021()//
DROP PROCEDURE housing_service_contracts_20261021//
DELIMITER ;
