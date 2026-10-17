-- Mechanisms only: no original trophy definitions, tier assignments or policy seeds.
ALTER TABLE guild_members ADD COLUMN IF NOT EXISTS trophy_enable TINYINT UNSIGNED NOT NULL DEFAULT 0;
CREATE TABLE IF NOT EXISTS housing_guild_trophy_state (
 guild_id INT UNSIGNED PRIMARY KEY,enabled TINYINT UNSIGNED NOT NULL DEFAULT 0,
 revision BIGINT UNSIGNED NOT NULL DEFAULT 1,remaining_ms INT UNSIGNED NOT NULL DEFAULT 0,
 paid_until_ms BIGINT UNSIGNED NOT NULL DEFAULT 0,
 selection_hash CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL DEFAULT '',
 CHECK(enabled IN(0,1))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_guild_trophy_selections (
 guild_id INT UNSIGNED NOT NULL,slot TINYINT UNSIGNED NOT NULL,benefit_id INT UNSIGNED NOT NULL,
 tier TINYINT UNSIGNED NOT NULL,custody_id BIGINT UNSIGNED NOT NULL,house_id BIGINT UNSIGNED NOT NULL,
 current_host_estate_id INT UNSIGNED NOT NULL,
 source_digest CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 PRIMARY KEY(guild_id,slot),UNIQUE KEY(guild_id,benefit_id),CHECK(slot<4),CHECK(tier<10)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_guild_trophy_billing_policy (
 singleton TINYINT UNSIGNED PRIMARY KEY,period_ms INT UNSIGNED NOT NULL,policy INT UNSIGNED NOT NULL,
 source_sha256 CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,source_description VARCHAR(255) NOT NULL,
 CHECK(singleton=1),CHECK(period_ms=600000),CHECK(policy=1)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_guild_trophy_favor_charges (
 guild_id INT UNSIGNED NOT NULL,state_revision BIGINT UNSIGNED NOT NULL,
 favor_before INT UNSIGNED NOT NULL,favor_after INT UNSIGNED NOT NULL,cost INT UNSIGNED NOT NULL,
 period_ms INT UNSIGNED NOT NULL,paid_until_ms BIGINT UNSIGNED NOT NULL,
 selection_hash CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 PRIMARY KEY(guild_id,state_revision),CHECK(favor_before>=favor_after),CHECK(cost=favor_before-favor_after)
) ENGINE=InnoDB;
