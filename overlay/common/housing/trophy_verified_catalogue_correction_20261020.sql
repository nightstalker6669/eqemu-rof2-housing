-- Withdraw ONLY the exact ten level1 tradeskill benefit seeds from 2026-10-18.
-- Run with world/zone processes stopped and no other database writers.
-- Requires the housing trophy migrations and ordinary tribute schema to exist.
-- Refuses custom/partial rows and ANY affected selection; never clears selections,
-- paid time, favor, charge history, physical inventory, placements or evolution.
-- Already-absent complete seed triplets are accepted (idempotent reapplication).
DELIMITER //
DROP PROCEDURE IF EXISTS housing_withdraw_unverified_trophy_levels_20261020//
CREATE PROCEDURE housing_withdraw_unverified_trophy_levels_20261020()
BEGIN
 DECLARE found_rows INT DEFAULT 0;
 DECLARE exact_rows INT DEFAULT 0;
 DECLARE references_found INT DEFAULT 0;
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 START TRANSACTION;
 SELECT COUNT(*) INTO found_rows FROM information_schema.tables
 WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN
 ('tributes','tribute_levels','housing_trophy_benefit_sources',
 'housing_trophy_benefit_selections','housing_guild_trophy_selections',
 'character_tribute','guild_tributes');
 IF found_rows<>7 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy correction requires all seven reference tables using InnoDB';
 END IF;
 -- Benefit 313: Denmother's Rolling Pin.
 SET found_rows=0; SET exact_rows=0;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=313 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=313 AND `unknown`=1 AND BINARY name=BINARY 'Denmother''s Rolling Pin' AND BINARY descr=BINARY 'documented effect/zero-cost; reconstructed benefit binding; explicit level1 server policy' AND isguild=0;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=313 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=313 AND level=1 AND cost=0 AND item_id=57724;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=313 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=313 AND placed_item_id=95014 AND BINARY source_sha256=BINARY 'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1' AND BINARY source_description=BINARY 'Compatibility20261018: published effects/costs, reconstructed bindings; explicit level1 policy';
 SET exact_rows=exact_rows+references_found;
 IF found_rows<>0 AND (found_rows<>3 OR exact_rows<>3) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy 313 differs from original seed or is partially present; no changes made';
 END IF;
 -- Benefit 320: Brewmaster's Mug.
 SET found_rows=0; SET exact_rows=0;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=320 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=320 AND `unknown`=1 AND BINARY name=BINARY 'Brewmaster''s Mug' AND BINARY descr=BINARY 'documented effect/zero-cost; reconstructed benefit binding; explicit level1 server policy' AND isguild=0;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=320 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=320 AND level=1 AND cost=0 AND item_id=57731;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=320 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=320 AND placed_item_id=95021 AND BINARY source_sha256=BINARY 'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1' AND BINARY source_description=BINARY 'Compatibility20261018: published effects/costs, reconstructed bindings; explicit level1 policy';
 SET exact_rows=exact_rows+references_found;
 IF found_rows<>0 AND (found_rows<>3 OR exact_rows<>3) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy 320 differs from original seed or is partially present; no changes made';
 END IF;
 -- Benefit 327: Fletcher's Arrow.
 SET found_rows=0; SET exact_rows=0;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=327 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=327 AND `unknown`=1 AND BINARY name=BINARY 'Fletcher''s Arrow' AND BINARY descr=BINARY 'documented effect/zero-cost; reconstructed benefit binding; explicit level1 server policy' AND isguild=0;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=327 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=327 AND level=1 AND cost=0 AND item_id=57738;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=327 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=327 AND placed_item_id=95028 AND BINARY source_sha256=BINARY 'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1' AND BINARY source_description=BINARY 'Compatibility20261018: published effects/costs, reconstructed bindings; explicit level1 policy';
 SET exact_rows=exact_rows+references_found;
 IF found_rows<>0 AND (found_rows<>3 OR exact_rows<>3) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy 327 differs from original seed or is partially present; no changes made';
 END IF;
 -- Benefit 334: Intricate Jewelers Glass.
 SET found_rows=0; SET exact_rows=0;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=334 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=334 AND `unknown`=1 AND BINARY name=BINARY 'Intricate Jewelers Glass' AND BINARY descr=BINARY 'documented effect/zero-cost; reconstructed benefit binding; explicit level1 server policy' AND isguild=0;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=334 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=334 AND level=1 AND cost=0 AND item_id=57745;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=334 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=334 AND placed_item_id=95035 AND BINARY source_sha256=BINARY 'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1' AND BINARY source_description=BINARY 'Compatibility20261018: published effects/costs, reconstructed bindings; explicit level1 policy';
 SET exact_rows=exact_rows+references_found;
 IF found_rows<>0 AND (found_rows<>3 OR exact_rows<>3) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy 334 differs from original seed or is partially present; no changes made';
 END IF;
 -- Benefit 341: Peerless Pestle.
 SET found_rows=0; SET exact_rows=0;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=341 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=341 AND `unknown`=1 AND BINARY name=BINARY 'Peerless Pestle' AND BINARY descr=BINARY 'documented effect/zero-cost; reconstructed benefit binding; explicit level1 server policy' AND isguild=0;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=341 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=341 AND level=1 AND cost=0 AND item_id=57752;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=341 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=341 AND placed_item_id=95042 AND BINARY source_sha256=BINARY 'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1' AND BINARY source_description=BINARY 'Compatibility20261018: published effects/costs, reconstructed bindings; explicit level1 policy';
 SET exact_rows=exact_rows+references_found;
 IF found_rows<>0 AND (found_rows<>3 OR exact_rows<>3) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy 341 differs from original seed or is partially present; no changes made';
 END IF;
 -- Benefit 348: Clay Flinger's Loop.
 SET found_rows=0; SET exact_rows=0;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=348 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=348 AND `unknown`=1 AND BINARY name=BINARY 'Clay Flinger''s Loop' AND BINARY descr=BINARY 'documented effect/zero-cost; reconstructed benefit binding; explicit level1 server policy' AND isguild=0;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=348 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=348 AND level=1 AND cost=0 AND item_id=57759;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=348 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=348 AND placed_item_id=95049 AND BINARY source_sha256=BINARY 'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1' AND BINARY source_description=BINARY 'Compatibility20261018: published effects/costs, reconstructed bindings; explicit level1 policy';
 SET exact_rows=exact_rows+references_found;
 IF found_rows<>0 AND (found_rows<>3 OR exact_rows<>3) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy 348 differs from original seed or is partially present; no changes made';
 END IF;
 -- Benefit 355: Blacksmith's Adamantine Hammer.
 SET found_rows=0; SET exact_rows=0;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=355 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=355 AND `unknown`=1 AND BINARY name=BINARY 'Blacksmith''s Adamantine Hammer' AND BINARY descr=BINARY 'documented effect/zero-cost; reconstructed benefit binding; explicit level1 server policy' AND isguild=0;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=355 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=355 AND level=1 AND cost=0 AND item_id=57766;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=355 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=355 AND placed_item_id=95056 AND BINARY source_sha256=BINARY 'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1' AND BINARY source_description=BINARY 'Compatibility20261018: published effects/costs, reconstructed bindings; explicit level1 policy';
 SET exact_rows=exact_rows+references_found;
 IF found_rows<>0 AND (found_rows<>3 OR exact_rows<>3) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy 355 differs from original seed or is partially present; no changes made';
 END IF;
 -- Benefit 362: Ethereal Quill.
 SET found_rows=0; SET exact_rows=0;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=362 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=362 AND `unknown`=1 AND BINARY name=BINARY 'Ethereal Quill' AND BINARY descr=BINARY 'documented effect/zero-cost; reconstructed benefit binding; explicit level1 server policy' AND isguild=0;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=362 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=362 AND level=1 AND cost=0 AND item_id=57773;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=362 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=362 AND placed_item_id=95063 AND BINARY source_sha256=BINARY 'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1' AND BINARY source_description=BINARY 'Compatibility20261018: published effects/costs, reconstructed bindings; explicit level1 policy';
 SET exact_rows=exact_rows+references_found;
 IF found_rows<>0 AND (found_rows<>3 OR exact_rows<>3) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy 362 differs from original seed or is partially present; no changes made';
 END IF;
 -- Benefit 369: Mystical Bolt.
 SET found_rows=0; SET exact_rows=0;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=369 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=369 AND `unknown`=1 AND BINARY name=BINARY 'Mystical Bolt' AND BINARY descr=BINARY 'documented effect/zero-cost; reconstructed benefit binding; explicit level1 server policy' AND isguild=0;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=369 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=369 AND level=1 AND cost=0 AND item_id=57780;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=369 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=369 AND placed_item_id=95070 AND BINARY source_sha256=BINARY 'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1' AND BINARY source_description=BINARY 'Compatibility20261018: published effects/costs, reconstructed bindings; explicit level1 policy';
 SET exact_rows=exact_rows+references_found;
 IF found_rows<>0 AND (found_rows<>3 OR exact_rows<>3) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy 369 differs from original seed or is partially present; no changes made';
 END IF;
 -- Benefit 376: Hovering Contraption.
 SET found_rows=0; SET exact_rows=0;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=376 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tributes WHERE id=376 AND `unknown`=1 AND BINARY name=BINARY 'Hovering Contraption' AND BINARY descr=BINARY 'documented effect/zero-cost; reconstructed benefit binding; explicit level1 server policy' AND isguild=0;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=376 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM tribute_levels WHERE tribute_id=376 AND level=1 AND cost=0 AND item_id=57787;
 SET exact_rows=exact_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=376 FOR UPDATE;
 IF references_found>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Duplicate trophy seed rows; no changes made'; END IF;
 SET found_rows=found_rows+references_found;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_sources WHERE benefit_id=376 AND placed_item_id=95077 AND BINARY source_sha256=BINARY 'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1' AND BINARY source_description=BINARY 'Compatibility20261018: published effects/costs, reconstructed bindings; explicit level1 policy';
 SET exact_rows=exact_rows+references_found;
 IF found_rows<>0 AND (found_rows<>3 OR exact_rows<>3) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Trophy 376 differs from original seed or is partially present; no changes made';
 END IF;
 SELECT COUNT(*) INTO references_found FROM housing_trophy_benefit_selections WHERE benefit_id IN (313,320,327,334,341,348,355,362,369,376) FOR UPDATE;
 IF references_found<>0 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Affected trophy selection exists in housing_trophy_benefit_selections; stop and review';
 END IF;
 SELECT COUNT(*) INTO references_found FROM housing_guild_trophy_selections WHERE benefit_id IN (313,320,327,334,341,348,355,362,369,376) FOR UPDATE;
 IF references_found<>0 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Affected trophy selection exists in housing_guild_trophy_selections; stop and review';
 END IF;
 SELECT COUNT(*) INTO references_found FROM character_tribute WHERE tribute IN (313,320,327,334,341,348,355,362,369,376) FOR UPDATE;
 IF references_found<>0 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Affected trophy selection exists in character_tribute; stop and review';
 END IF;
 SELECT COUNT(*) INTO references_found FROM guild_tributes WHERE tribute_id_1 IN (313,320,327,334,341,348,355,362,369,376) OR tribute_id_2 IN (313,320,327,334,341,348,355,362,369,376) FOR UPDATE;
 IF references_found<>0 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Affected ordinary guild tribute selection exists; stop and review';
 END IF;
 DELETE FROM housing_trophy_benefit_sources WHERE benefit_id IN (313,320,327,334,341,348,355,362,369,376);
 DELETE FROM tribute_levels WHERE tribute_id IN (313,320,327,334,341,348,355,362,369,376);
 DELETE FROM tributes WHERE id IN (313,320,327,334,341,348,355,362,369,376);
 COMMIT;
END//
CALL housing_withdraw_unverified_trophy_levels_20261020()//
DROP PROCEDURE housing_withdraw_unverified_trophy_levels_20261020//
DELIMITER ;
