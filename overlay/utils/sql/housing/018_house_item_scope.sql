-- Durable containment of real item custody by stable HouseID, independent of
-- the exterior plot, allocated room instance, original creator or item GUID.
-- Requires007 and016. No seeds; no runtime or authority inferred from membership.
-- Insert/delete/CAS must join the real custody operation transaction and receipt.
-- House pickup/replacement leaves these links untouched. Individual item Return
-- must delete its link before releasing custody in that same checked transaction.
-- Placed/closet/vault state and capacity are separate; this table supplies neither.
-- custody.estate_id remains its existing ledger domain; current authorization
-- must resolve the actual current house parent rather than that historical field.
CREATE TABLE IF NOT EXISTS housing_house_item_scopes (
 custody_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
 house_id BIGINT UNSIGNED NOT NULL,
 revision BIGINT UNSIGNED NOT NULL DEFAULT 1,
 KEY housing_house_item_scope_container(house_id,custody_id),
 CONSTRAINT housing_house_item_scope_custody FOREIGN KEY(custody_id)
  REFERENCES housing_live_item_custody(custody_id) ON DELETE RESTRICT,
 CONSTRAINT housing_house_item_scope_house FOREIGN KEY(house_id)
  REFERENCES housing_house_instances(house_id) ON DELETE RESTRICT,
 CONSTRAINT housing_house_item_scope_shape CHECK(custody_id>0 AND house_id>0 AND revision>0)
) ENGINE=InnoDB;
