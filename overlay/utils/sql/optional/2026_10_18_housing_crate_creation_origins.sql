-- Separate immutable live-client and server-owned offline creation provenance.
-- Never insert a synthetic Client receipt to satisfy an offline creation FK.
CREATE TABLE IF NOT EXISTS housing_crate_creation_origins (
 request_key VARBINARY(96) NOT NULL PRIMARY KEY,
 kind TINYINT UNSIGNED NOT NULL,
 live_request_key VARBINARY(96) NULL UNIQUE,
 offline_estate_id INT UNSIGNED NULL UNIQUE,
 CONSTRAINT housing_crate_origin_live FOREIGN KEY(live_request_key) REFERENCES housing_live_item_receipts(request_key) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_origin_offline FOREIGN KEY(offline_estate_id) REFERENCES housing_estates(id) ON DELETE RESTRICT,
 CONSTRAINT housing_crate_origin_shape CHECK((kind=1 AND live_request_key IS NOT NULL AND live_request_key=request_key AND offline_estate_id IS NULL) OR (kind=2 AND live_request_key IS NULL AND offline_estate_id IS NOT NULL AND request_key=CONCAT('offline-inactivity:',offline_estate_id)))
) ENGINE=InnoDB;
INSERT IGNORE INTO housing_crate_creation_origins(request_key,kind,live_request_key,offline_estate_id)
 SELECT c.creation_request_key,1,c.creation_request_key,NULL FROM housing_crate_instances c JOIN housing_live_item_receipts r ON r.request_key=c.creation_request_key;
ALTER TABLE housing_crate_instances DROP FOREIGN KEY IF EXISTS housing_crate_creation_parent;
ALTER TABLE housing_crate_instances DROP FOREIGN KEY IF EXISTS housing_crate_creation_origin;
ALTER TABLE housing_crate_instances ADD CONSTRAINT housing_crate_creation_origin FOREIGN KEY(creation_request_key) REFERENCES housing_crate_creation_origins(request_key) ON DELETE RESTRICT;
