-- Reviewed Evantil's Abode (77407) housing metadata only.
-- Source: retained items.sodeq.org collector row 68671, collectversion 2.5.2.2,
-- created 2015-11-28, updated 2021-09-18; this is later collection evidence,
-- Retained compressed dump SHA256:
-- ec0699f4f0e7f070b2e99297f521e9a266b13010b41b0b443216da2be2865302
-- Row77407 line SHA256:
-- bcb84a4acb0a5a329d401282c7e706077886d457b15ea0333ba5ed8bc54a0a14
-- NOT an authenticated May2013 item packet. Original RoF2 client independently
-- registers Evantil's Abode at zone766/phinteriortree; original IT23651 geometry
-- has been reviewed separately. Install matching runtime/profile support first.
-- UNKNOWN40..44 map to UNK227..231. Scale fields store IEEE754 binary32 bits,
-- not numeric percentages: 0.11f=1038174126, 1.0f=1065353216.
-- Stop world/zone and other item writers; reload shared items after success.
-- Exact previously-unpopulated metadata or exact reviewed postimage only.
-- Unknown named rotation/free-placement flags and every other field are preserved.
DELIMITER //
DROP PROCEDURE IF EXISTS housing_evantil_metadata_20261021//
CREATE PROCEDURE housing_evantil_metadata_20261021()
BEGIN
 DECLARE matches_found INT DEFAULT 0;
 DECLARE before_matches INT DEFAULT 0;
 DECLARE after_matches INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 START TRANSACTION;
 SELECT COUNT(*) INTO matches_found FROM information_schema.tables
 WHERE table_schema=DATABASE() AND table_name='items' AND engine='InnoDB';
 IF matches_found<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Evantil correction requires InnoDB items';
 END IF;
 SELECT COUNT(*) INTO matches_found FROM items WHERE id=77407 FOR UPDATE;
 IF matches_found<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Evantil item77407 is missing or duplicated';
 END IF;
 SELECT COUNT(*) INTO before_matches FROM items WHERE id=77407
 AND BINARY name=BINARY 'Evantil''s Abode' AND BINARY lore=BINARY 'A 2 room Tree House.'
 AND BINARY idfile=BINARY 'IT23651' AND itemclass=0
 AND UNK224=0 AND UNK225=0 AND UNK226=0 AND UNK227=0
 AND UNK228=0 AND UNK229=0 AND UNK230=0 AND UNK231=0;
 SELECT COUNT(*) INTO after_matches FROM items WHERE id=77407
 AND BINARY name=BINARY 'Evantil''s Abode' AND BINARY lore=BINARY 'A 2 room Tree House.'
 AND BINARY idfile=BINARY 'IT23651' AND itemclass=0
 AND UNK224=8 AND UNK225=0 AND UNK226=0 AND UNK227=42
 AND UNK228=1038174126 AND UNK229=1065353216 AND UNK230=25 AND UNK231=-1;
 IF before_matches=0 AND after_matches=0 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Evantil item metadata differs from reviewed before/after images; no changes made';
 END IF;
 IF before_matches=1 THEN
  SELECT COUNT(*) INTO matches_found FROM housing_house_instances WHERE item_template_id=77407 FOR UPDATE;
  IF matches_found<>0 THEN
   SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Evantil already has retained house bindings; review provenance instead of rewriting';
  END IF;
  UPDATE items SET UNK224=8,UNK227=42,UNK228=1038174126,
   UNK229=1065353216,UNK230=25,UNK231=-1 WHERE id=77407;
 END IF;
 COMMIT;
END//
CALL housing_evantil_metadata_20261021()//
DROP PROCEDURE housing_evantil_metadata_20261021//
DELIMITER ;
