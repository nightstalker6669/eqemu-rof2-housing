-- Optional typed native reserved subjects. Existing character grants remain004.
-- No grants, memberships, ownership or Live content are seeded.
CREATE TABLE IF NOT EXISTS housing_acl_special_members (
 estate_id INT UNSIGNED NOT NULL,
 subject_kind TINYINT UNSIGNED NOT NULL,
 subject_id INT UNSIGNED NOT NULL,
 group_id TINYINT UNSIGNED NOT NULL,
 PRIMARY KEY(estate_id,subject_kind,subject_id),
 CONSTRAINT housing_acl_special_state FOREIGN KEY(estate_id) REFERENCES housing_acl_state(estate_id) ON DELETE CASCADE,
 CONSTRAINT housing_acl_special_valid CHECK(group_id<=5 AND ((subject_kind=1 AND subject_id BETWEEN 1 AND 8) OR (subject_kind=2 AND subject_id=0)))
) ENGINE=InnoDB;
-- subject_kind1=GuildRank;2=Fellowship. Fellowship membership has no authoritative
-- source in this server slice and therefore never matches in Scene::Service.
