-- Genuine RoF Holgresh House Pet77408: later collector physical/key metadata,
-- original HLG715/2 model association, explicit source-derived model-default
-- sizing policy. Historical Live raw actor height remains unknown, stored0.
-- Runtime emits size6 from exact original racedata; source0 is not wire0.
-- Opaque respawn4500 is preserved on the item wire; no timing unit/AI is seeded.
-- Install matching NPC runtime first, stop all world/zones and item writers,
-- apply prerequisite NPC source/runtime schemas, and regenerate shared items.
-- Audit that item77408 has no pre-existing retained custody before activation.
-- Preserve every unrelated item column and every original house/custody identity.
-- Reviewed binding JSON SHA256: 4367281d3ea244b1f8eba86d5f8428535b0eb698f9cde375f822223444f74a00
DELIMITER //
DROP PROCEDURE IF EXISTS housing_holgresh_npc_binding_20261021//
CREATE PROCEDURE housing_holgresh_npc_binding_20261021()
BEGIN
 DECLARE found_count INT DEFAULT 0;
 DECLARE before_count INT DEFAULT 0;
 DECLARE after_count INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 START TRANSACTION;
 SELECT COUNT(*) INTO found_count FROM information_schema.tables
 WHERE table_schema=DATABASE() AND table_name IN ('items','housing_native_npc_item_sources') AND engine='InnoDB';
 IF found_count<>2 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Holgresh binding requires InnoDB item and NPC source tables'; END IF;
 SELECT COUNT(*) INTO found_count FROM information_schema.columns
 WHERE table_schema=DATABASE() AND table_name='housing_native_npc_item_sources' AND column_name='actor_height_bits' AND data_type='int' AND column_type LIKE '%unsigned' AND is_nullable='NO';
 IF found_count<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Holgresh binding requires the NPC runtime column'; END IF;
 SELECT COUNT(*) INTO found_count FROM items WHERE id=77408 AND BINARY name=BINARY 'Holgresh House Pet'
 AND BINARY idfile=BINARY 'IT63' AND itemclass=0 AND bagslots=0 AND stackable=0 AND stacksize=1 AND evoitem=0
 AND UNK225=0 AND UNK226=0 AND UNK227=0 AND UNK230=0
 AND housing_disable_rotation IS NULL AND housing_disable_free_placement IS NULL FOR UPDATE;
 IF found_count<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Holgresh physical item/unknown flag fields conflict; no changes made'; END IF;
 SELECT COUNT(*) INTO before_count FROM items WHERE id=77408 AND UNK224=0 AND UNK228=0 AND UNK229=0 AND UNK231=0;
 SELECT COUNT(*) INTO after_count FROM items WHERE id=77408 AND UNK224=142 AND UNK228=1065353216 AND UNK229=1065353216 AND UNK231=-1;
 IF before_count=0 AND after_count=0 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Holgresh housing metadata differs from exact before/after; no changes made'; END IF;
 SELECT COUNT(*) INTO found_count FROM housing_native_npc_item_sources WHERE item_id=77408 FOR UPDATE;
 IF found_count=0 THEN
  INSERT INTO housing_native_npc_item_sources(item_id,housepet_filename,interactive_object,npc_respawn_interval,source_sha256,actor_height_bits)
  VALUES(77408,'a_holgresh',0,4500,UNHEX('4367281d3ea244b1f8eba86d5f8428535b0eb698f9cde375f822223444f74a00'),0);
 ELSE
  SELECT COUNT(*) INTO after_count FROM housing_native_npc_item_sources WHERE item_id=77408
   AND BINARY housepet_filename=BINARY 'a_holgresh' AND interactive_object=0 AND npc_respawn_interval=4500
   AND source_sha256=UNHEX('4367281d3ea244b1f8eba86d5f8428535b0eb698f9cde375f822223444f74a00') AND actor_height_bits=0;
  IF found_count<>1 OR after_count<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Existing Holgresh NPC source differs; no changes made'; END IF;
 END IF;
 IF before_count=1 THEN UPDATE items SET UNK224=142,UNK228=1065353216,UNK229=1065353216,UNK231=-1 WHERE id=77408; END IF;
 COMMIT;
END//
CALL housing_holgresh_npc_binding_20261021()//
DROP PROCEDURE housing_holgresh_npc_binding_20261021//
DELIMITER ;
