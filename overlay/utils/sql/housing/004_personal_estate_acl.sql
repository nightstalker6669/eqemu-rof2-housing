-- Optional additive ACL persistence. Requires001; no estate/catalog/policy seeds.
-- Native May10,2013 defaults are supplied by service when no custom state exists.
-- Character/account relationships are checked and locked by service, not client names.
CREATE TABLE IF NOT EXISTS housing_acl_state (
    estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
    revision BIGINT UNSIGNED NOT NULL,
    CONSTRAINT housing_acl_estate_fk FOREIGN KEY (estate_id)
        REFERENCES housing_estates(id) ON DELETE CASCADE
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_acl_groups (
    estate_id INT UNSIGNED NOT NULL,
    group_id TINYINT UNSIGNED NOT NULL,
    capabilities INT UNSIGNED NOT NULL,
    PRIMARY KEY (estate_id,group_id),
    CONSTRAINT housing_acl_group_state_fk FOREIGN KEY (estate_id)
        REFERENCES housing_acl_state(estate_id) ON DELETE CASCADE,
    CONSTRAINT housing_acl_group_bounds CHECK (group_id <=6 AND capabilities <=8191 AND (group_id <>6 OR capabilities=0))
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS housing_acl_members (
    estate_id INT UNSIGNED NOT NULL,
    character_id INT UNSIGNED NOT NULL,
    group_id TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (estate_id,character_id),
    CONSTRAINT housing_acl_member_state_fk FOREIGN KEY (estate_id)
        REFERENCES housing_acl_state(estate_id) ON DELETE CASCADE,
    CONSTRAINT housing_acl_member_bounds CHECK (group_id <=5 AND character_id >0)
) ENGINE=InnoDB;
-- No foreign key to legacy character_data: its IDs/types/engines vary. Service
-- refuses non-InnoDB actor tables and detects unknown/deleted member records.
