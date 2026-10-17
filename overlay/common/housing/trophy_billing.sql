-- Original help fixes duration at 600000ms. NO policy, cost or definition seeds.
-- Explicit policy1 is documented server behavior, not a recovered Live enum.
CREATE TABLE IF NOT EXISTS housing_trophy_billing_policy (
 scope VARCHAR(16) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,
 period_ms INT UNSIGNED NOT NULL,
 policy INT UNSIGNED NOT NULL,
 source_sha256 CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 source_description VARCHAR(255) NOT NULL,
 CHECK(scope='personal'), CHECK(period_ms=600000), CHECK(policy=1)
) ENGINE=InnoDB;
ALTER TABLE housing_trophy_benefit_state ADD COLUMN IF NOT EXISTS remaining_ms INT UNSIGNED NOT NULL DEFAULT 0;
ALTER TABLE housing_trophy_benefit_state ADD COLUMN IF NOT EXISTS paid_until_ms BIGINT UNSIGNED NOT NULL DEFAULT 0;
ALTER TABLE housing_trophy_benefit_state ADD COLUMN IF NOT EXISTS selection_hash CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL DEFAULT '';
CREATE TABLE IF NOT EXISTS housing_trophy_favor_charges (
 character_id INT UNSIGNED NOT NULL,
 state_revision BIGINT UNSIGNED NOT NULL,
 lease_epoch BIGINT UNSIGNED NOT NULL,
 session_key VARBINARY(96) NOT NULL,
 favor_before INT UNSIGNED NOT NULL,
 favor_after INT UNSIGNED NOT NULL,
 cost INT UNSIGNED NOT NULL,
 period_ms INT UNSIGNED NOT NULL,
 paid_until_ms BIGINT UNSIGNED NOT NULL,
 selection_hash CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 PRIMARY KEY(character_id,state_revision),
 CHECK(favor_before>=favor_after), CHECK(cost=favor_before-favor_after)
) ENGINE=InnoDB;
