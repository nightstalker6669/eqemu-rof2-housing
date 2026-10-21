-- Keep opt-in guild recipient policies compatible with the upstream character
-- save path, which uses REPLACE and therefore transiently deletes the old row.
-- Drop ONLY the exact incompatible recipient FK; preserve every policy/receipt.
-- Current guild/leader/recipient/account/deleted_at checks remain in RetireGuild.
-- No change to upstream character save code or its other constraints.
-- Stop concurrent schema writers. ALTER TABLE commits implicitly; this procedure
-- performs every check before its single DDL operation and is idempotent.
DELIMITER //
DROP PROCEDURE IF EXISTS housing_character_save_compatibility_20261021//
CREATE PROCEDURE housing_character_save_compatibility_20261021()
BEGIN
 DECLARE found_count INT DEFAULT 0;
 DECLARE exact_count INT DEFAULT 0;
 SELECT COUNT(*) INTO found_count FROM information_schema.tables
 WHERE table_schema=DATABASE() AND table_name='housing_guild_inactivity_policy' AND engine='InnoDB';
 IF found_count<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Character-save compatibility requires installed InnoDB guild policy';
 END IF;
 SELECT COUNT(*) INTO found_count FROM information_schema.columns
 WHERE table_schema=DATABASE() AND table_name='housing_guild_inactivity_policy'
 AND column_name='recipient_character_id' AND data_type='int'
 AND column_type LIKE '%unsigned' AND is_nullable='NO';
 IF found_count<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Guild recipient policy column differs; nothing changed';
 END IF;
 SELECT COUNT(*) INTO found_count FROM information_schema.table_constraints
 WHERE constraint_schema=DATABASE() AND table_name='housing_guild_inactivity_policy'
 AND constraint_name='housing_guild_inactive_recipient';
 IF found_count<>0 THEN
  SELECT COUNT(*) INTO exact_count FROM information_schema.key_column_usage k
  JOIN information_schema.referential_constraints r
   ON r.constraint_schema=k.constraint_schema AND r.table_name=k.table_name
   AND r.constraint_name=k.constraint_name
  WHERE k.constraint_schema=DATABASE() AND k.table_name='housing_guild_inactivity_policy'
   AND k.constraint_name='housing_guild_inactive_recipient'
   AND k.column_name='recipient_character_id' AND k.ordinal_position=1
   AND k.referenced_table_schema=DATABASE() AND k.referenced_table_name='character_data'
   AND k.referenced_column_name='id' AND r.delete_rule='RESTRICT' AND r.update_rule='RESTRICT';
  IF found_count<>1 OR exact_count<>1 OR
   (SELECT COUNT(*) FROM information_schema.key_column_usage WHERE constraint_schema=DATABASE()
    AND table_name='housing_guild_inactivity_policy' AND constraint_name='housing_guild_inactive_recipient')<>1 THEN
   SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Recipient FK differs from exact supported constraint; nothing changed';
  END IF;
 END IF;
 SELECT COUNT(*) INTO exact_count FROM information_schema.key_column_usage
 WHERE constraint_schema=DATABASE() AND table_name='housing_guild_inactivity_policy'
 AND column_name='recipient_character_id' AND referenced_table_name IS NOT NULL
 AND constraint_name<>'housing_guild_inactive_recipient';
 IF exact_count<>0 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Unexpected recipient FK remains; nothing changed';
 END IF;
 IF found_count=1 THEN
  ALTER TABLE housing_guild_inactivity_policy DROP FOREIGN KEY housing_guild_inactive_recipient;
 END IF;
END//
CALL housing_character_save_compatibility_20261021()//
DROP PROCEDURE housing_character_save_compatibility_20261021//
DELIMITER ;
