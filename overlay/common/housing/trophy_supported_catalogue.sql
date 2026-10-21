-- Evidence-supported reconstructed trophy profile. NOT an authenticated May2013 catalogue.
-- Unknown costs/levels are excluded, including all unverified tradeskill benefit minimum levels.
-- Apply to a reviewed server only; refuses existing keys rather than replacing data.
START TRANSACTION;
INSERT INTO tributes(id,`unknown`,name,descr,isguild) VALUES (216,1,'Trophy of the Giant','documented later cost/level/effect; reconstructed stat association',0);
INSERT INTO tribute_levels(tribute_id,level,cost,item_id) VALUES (216,95,75,68736);
INSERT INTO housing_trophy_benefit_sources(benefit_id,placed_item_id,source_sha256,source_description) VALUES (216,68730,'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1','Compatibility20261018: published effects/costs, reconstructed bindings; not authenticated2013');
INSERT INTO tributes(id,`unknown`,name,descr,isguild) VALUES (213,1,'Trophy of the Landing','documented later cost/level/effect; reconstructed stat association',0);
INSERT INTO tribute_levels(tribute_id,level,cost,item_id) VALUES (213,95,250,68733);
INSERT INTO housing_trophy_benefit_sources(benefit_id,placed_item_id,source_sha256,source_description) VALUES (213,68727,'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1','Compatibility20261018: published effects/costs, reconstructed bindings; not authenticated2013');
INSERT INTO tributes(id,`unknown`,name,descr,isguild) VALUES (111,1,'Trophy of Hardened Steel','documented later cost/level/effect; reconstructed stat association',1);
INSERT INTO tribute_levels(tribute_id,level,cost,item_id) VALUES (111,90,225,66571);
INSERT INTO housing_trophy_benefit_sources(benefit_id,placed_item_id,source_sha256,source_description) VALUES (111,66750,'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1','Compatibility20261018: published effects/costs, reconstructed bindings; not authenticated2013');
INSERT INTO housing_trophy_billing_policy(scope,period_ms,policy,source_sha256,source_description) VALUES ('personal',600000,1,'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1','Reviewed ten-minute cycle; existing emulator paid-time policy');
INSERT INTO housing_guild_trophy_billing_policy(singleton,period_ms,policy,source_sha256,source_description) VALUES (1,600000,1,'f4eee966b4ce20a590be206f1e523330b4516c7aac9d2e265436916a4075d0e1','Reviewed ten-minute cycle; existing emulator paid-time policy');
COMMIT;
