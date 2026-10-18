-- Per-ownership generation: retired/new plots do not inherit voting preferences.
CREATE TABLE IF NOT EXISTS housing_estate_preferences (
 estate_id INT UNSIGNED NOT NULL PRIMARY KEY,
 voting_opted_out TINYINT UNSIGNED NOT NULL DEFAULT 0,
 CONSTRAINT housing_estate_preferences_owner FOREIGN KEY(estate_id) REFERENCES housing_estates(id),
 CONSTRAINT housing_estate_preferences_boolean CHECK(voting_opted_out<=1)
) ENGINE=InnoDB;
