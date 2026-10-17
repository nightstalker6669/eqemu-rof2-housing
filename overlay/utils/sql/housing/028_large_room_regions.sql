-- Administrative schema widening only; preserve existing bytes and revisions.
-- Room profiles may retain a bounded region bootstrap over64KiB. Exact native
-- fragmented delivery still requires acceptance before enabling a large profile.
-- No content seeds, active profile changes, or global packet policy changes.
ALTER TABLE housing_house_room_profiles MODIFY regions_blob MEDIUMBLOB NOT NULL;
ALTER TABLE housing_catalogs MODIFY regions_blob MEDIUMBLOB NOT NULL;
