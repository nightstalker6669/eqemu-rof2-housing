-- Hermit's Hideaway54999: reviewed later collector physical metadata.
-- Source https://items.sodeq.org/downloads/items.txt.gz row47932, collector2.5.2.3.
-- Compressed SHA256 ec0699f4f0e7f070b2e99297f521e9a266b13010b41b0b443216da2be2865302
-- Raw row SHA256 19dce41c7e21ffa0073c83426b435324413c998252fb396784ce01bf926c4ca6
-- Created2015-12-11,updated2021-09-18; NOT authenticated May2013packet.
-- Route723 is a reviewed reconstruction from original dragon-themed723geometry,
-- authenticdragon-loveritemlore and later primary direct723 association.
-- This migration only repairs exact zero physical fields; no route/capacity data.
-- 0.9f/1.1f use IEEE754bits1063675494/1066192077. Named unknown flags preserved.
-- Quiesce item writers and reload shared cache after successful separate deployment.
DELIMITER //
DROP PROCEDURE IF EXISTS housing_hermit_metadata_20261021//
CREATE PROCEDURE housing_hermit_metadata_20261021()
BEGIN
 DECLARE matches_found INT DEFAULT 0;
 DECLARE before_matches INT DEFAULT 0;
 DECLARE after_matches INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 START TRANSACTION;
 SELECT COUNT(*) INTO matches_found FROM information_schema.tables
 WHERE table_schema=DATABASE() AND table_name='items' AND engine='InnoDB';
 IF matches_found<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Hermit correction requires InnoDB items';
 END IF;
 SELECT COUNT(*) INTO matches_found FROM items WHERE id=54999 FOR UPDATE;
 IF matches_found<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Hermit item54999 is missing or duplicated';
 END IF;
 SELECT COUNT(*) INTO before_matches FROM items WHERE id=54999
 AND BINARY name=BINARY 'Hermit''s Hideaway' AND BINARY lore=BINARY 'A fitting abode for the reclusive dragon lover'
 AND BINARY idfile=BINARY 'IT20022' AND itemclass=0
 AND UNK224=0 AND UNK225=0 AND UNK226=0 AND UNK227=0
 AND UNK228=0 AND UNK229=0 AND UNK230=0 AND UNK231=0;
 SELECT COUNT(*) INTO after_matches FROM items WHERE id=54999
 AND BINARY name=BINARY 'Hermit''s Hideaway' AND BINARY lore=BINARY 'A fitting abode for the reclusive dragon lover'
 AND BINARY idfile=BINARY 'IT20022' AND itemclass=0
 AND UNK224=8 AND UNK225=0 AND UNK226=0 AND UNK227=30
 AND UNK228=1063675494 AND UNK229=1066192077 AND UNK230=25 AND UNK231=-1;
 IF before_matches=0 AND after_matches=0 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Hermit item metadata differs from reviewed before/after images; no changes made';
 END IF;
 IF before_matches=1 THEN
  SELECT COUNT(*) INTO matches_found FROM housing_house_instances WHERE item_template_id=54999 FOR UPDATE;
  IF matches_found<>0 THEN
   SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Hermit already has retained house bindings; review provenance instead of rewriting';
  END IF;
  UPDATE items SET UNK224=8,UNK227=30,UNK228=1063675494,
   UNK229=1066192077,UNK230=25,UNK231=-1 WHERE id=54999;
 END IF;
 COMMIT;
END//
CALL housing_hermit_metadata_20261021()//
DROP PROCEDURE housing_hermit_metadata_20261021//
DELIMITER ;
