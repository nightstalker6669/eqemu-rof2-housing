-- Optional content repair for physical Landing68727 and Giant68730 only.
-- Retained later item collector metadata + matching original RoF2 IT11223 geometry.
-- Not authenticated May2013 metadata, not an invented Live catalogue.
-- Provenance: common/housing/personal_trophy_placement.{md,json}.
-- Run while world/zone are stopped, then rebuild/reload shared item data.
-- Existing nonempty conflicting metadata fails; no benefits, costs or unknown
-- placement restriction flags are granted or rewritten by this repair.
DROP PROCEDURE IF EXISTS housing_install_verified_personal_trophy_placement;
DELIMITER //
CREATE PROCEDURE housing_install_verified_personal_trophy_placement()
BEGIN
 DECLARE matched INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 SELECT COUNT(*) INTO matched FROM information_schema.tables
 WHERE table_schema=DATABASE() AND table_name='items' AND ENGINE='InnoDB';
 IF matched<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Personal trophy repair requires InnoDB items';
 END IF;
 START TRANSACTION;
 SELECT id FROM items WHERE id IN (68727,68730) ORDER BY id FOR UPDATE;
 SELECT COUNT(*) INTO matched FROM items
 WHERE ((id=68727 AND BINARY Name=BINARY 'Trophy of the Landing')
     OR (id=68730 AND BINARY Name=BINARY 'Trophy of the Giant'))
   AND BINARY idfile=BINARY 'IT11223' AND itemclass=0 AND bagslots=0
   AND stackable=0 AND UNK225=0 AND UNK226=0 AND UNK227=0
   AND ((UNK224=0 AND UNK228=0 AND UNK229=0 AND UNK230=0 AND UNK231=0)
     OR (UNK224=12 AND UNK228=1036831949 AND UNK229=1075838976 AND UNK230=4 AND UNK231=1));
 IF matched<>2 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Personal trophy identity or placement metadata conflict';
 END IF;
 UPDATE items SET UNK224=12,UNK228=1036831949,UNK229=1075838976,UNK230=4,UNK231=1
 WHERE id IN (68727,68730);
 COMMIT;
END//
DELIMITER ;
CALL housing_install_verified_personal_trophy_placement();
DROP PROCEDURE housing_install_verified_personal_trophy_placement;
