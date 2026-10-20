#ifndef EQEMU_HOUSING_CRATE_VAULT_MEMBERS_DB_H
#define EQEMU_HOUSING_CRATE_VAULT_MEMBERS_DB_H
#include "vault_storage_db.h"
#include "crate_binding.h"
#include "interior_placement_review.h"
namespace Housing::CrateVault {
using VaultStorage::Query;
inline uint64_t Count(Database& db,uint64_t crate){if(!VaultStorage::CrateMembersInstalled(db))return 0;auto r=Query(db,"SELECT custody_id FROM housing_crate_vault_members WHERE crate_id="+std::to_string(crate)+" ORDER BY custody_id FOR UPDATE");return r.RowCount();}
// Allocation was independently authenticated by the owning scene/carry path.
// Revalidate every original non-placeable row and source marker. No dummy pose,
// physical actor, creator rewrite or unreviewed geometry is accepted.
inline std::vector<VaultStorage::Item> Read(Database& db,const CrateBinding::Allocation& a,bool pending=false){
 std::vector<VaultStorage::Item> out;if(!VaultStorage::CrateMembersInstalled(db))return out;
 auto rows=Query(db,"SELECT custody_id,revision,origin_native_estate_id,origin_house_id,origin_realm_estate_id,vault_revision,HEX(source_row),HEX(catalog_blob),schema_descriptor FROM housing_crate_vault_members WHERE crate_id="+std::to_string(a.binding.crate_id)+" ORDER BY custody_id LIMIT 1001 FOR UPDATE");
 if(rows.RowCount()>1000)throw std::invalid_argument("Too many crate vault members");
 for(auto r=rows.begin();r!=rows.end();++r){for(unsigned i=0;i<9;++i)if(i!=3&&!r[i])throw std::invalid_argument("Incomplete crate vault witness");
  const uint64_t custody=std::stoull(r[0]),revision=std::stoull(r[1]),house=r[3]?std::stoull(r[3]):0,vault_revision=std::stoull(r[5]);const uint32_t native=std::stoul(r[2]),estate=std::stoul(r[4]);
  if(!custody||!revision||!vault_revision||native!=a.binding.parent_native_estate_id||(!house&&estate!=a.binding.estate_id)||std::string(r[8])!=LiveItems::Service::SchemaDescriptor())throw std::invalid_argument("Changed crate vault origin");
  auto items=VaultStorage::Items(db,estate,native,house,{true,true,bool(house&&(a.inventory_character_id||a.parcel_id))});auto i=std::find_if(items.begin(),items.end(),[&](const auto& v){return v.stored.custody_id==custody;});
  if(i==items.end()||i->realm_estate_id!=estate||i->revision!=vault_revision||i->stored.row.values.character_id!=a.created_by_character_id||VaultStorage::Unhex(r[6])!=CustodyCodec::Encode(i->stored.row))throw std::invalid_argument("Changed crate vault payload");
  std::vector<uint8_t> cat;for(auto n:{i->stored.catalog.item_class,i->stored.catalog.bag_slots,i->stored.catalog.lore_group,i->stored.catalog.no_drop})CustodyCodec::W(cat,uint32_t(n),4);if(cat!=VaultStorage::Unhex(r[7]))throw std::invalid_argument("Changed crate vault catalog");
  auto invalid=Query(db,"SELECT (SELECT COUNT(*) FROM housing_live_placed_items WHERE custody_id="+std::to_string(custody)+")+(SELECT COUNT(*) FROM housing_live_item_storage WHERE custody_id="+std::to_string(custody)+")+(SELECT COUNT(*) FROM housing_crate_members WHERE custody_id="+std::to_string(custody)+")");if(invalid.RowCount()!=1||std::stoull(invalid.begin()[0]))throw std::invalid_argument("Vault cannot acquire physical custody");
  auto lease=Query(db,"SELECT pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(i->stored.row.values.character_id)+" FOR UPDATE");if(!pending&&lease.RowCount()&&lease.begin()[0])throw std::invalid_argument("Pending crate vault creator");
  i->contained_in_crate=a.binding.crate_id;i->membership_revision=revision;out.push_back(*i);
 }return out;
}
inline void Relocate(Database& db,const CrateBinding::Allocation& carried,const LiveItems::Request& request,const InteriorContent::PlacementReview* room){
 auto vaults=Read(db,carried,true);const auto& target=request.placement->reservation;
 for(const auto& item:vaults){
  if(item.revision==UINT64_MAX||item.membership_revision==UINT64_MAX)throw std::invalid_argument("Vault relocation overflow");
  auto r=Query(db,"SELECT character_id,account_id,lease_epoch,HEX(session_key),runtime_serial FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(item.native_item_id)+" AND state=1 AND custody_id="+std::to_string(item.stored.custody_id)+" FOR UPDATE");if(r.RowCount()!=1)throw std::invalid_argument("Vault relocation reservation");auto old=r.begin();for(unsigned i=0;i<5;++i)if(!old[i])throw std::invalid_argument("Vault relocation source");
  const auto seed=request.request_key+":vault:"+std::to_string(item.stored.custody_id)+":"+std::to_string(item.native_item_id);std::array<uint8_t,32> digest{};if(!SHA256(reinterpret_cast<const unsigned char*>(seed.data()),seed.size(),digest.data()))throw std::invalid_argument("Vault relocation key");const auto key="crate-vault-relocate:"+VaultStorage::Hex({digest.begin(),digest.end()});
  auto old_reservation=Query(db,"UPDATE housing_live_placement_reservations SET state=2,custody_id=NULL WHERE native_item_id="+std::to_string(item.native_item_id)+" AND state=1 AND custody_id="+std::to_string(item.stored.custody_id));if(old_reservation.RowsAffected()!=1)throw std::invalid_argument("Vault relocation CAS");
  auto made=Query(db,"INSERT INTO housing_live_placement_reservations(request_key,character_id,account_id,lease_epoch,session_key,estate_id,native_estate_id,catalog_key,catalog_revision,source_row,runtime_serial,state,custody_id) VALUES('"+db.Escape(key)+"',"+old[0]+","+old[1]+","+old[2]+",UNHEX('"+old[3]+"'),"+std::to_string(request.estate_id)+","+std::to_string(target.native_estate_id)+",'"+db.Escape(target.catalog_key)+"',"+std::to_string(target.catalog_revision)+",UNHEX('"+VaultStorage::Hex(CustodyCodec::Encode(item.stored.row))+"'),"+old[4]+",1,"+std::to_string(item.stored.custody_id)+")");if(made.RowsAffected()!=1||!made.LastInsertedID()||made.LastInsertedID()>=UINT32_MAX)throw std::invalid_argument("Vault relocation allocation");
  auto scopes=Query(db,"SELECT house_id FROM housing_house_item_scopes WHERE custody_id="+std::to_string(item.stored.custody_id)+" FOR UPDATE");if(scopes.RowCount())throw std::invalid_argument("Carried vault scope not detached");
  if(item.stored.estate_id!=request.estate_id){auto moved=Query(db,"UPDATE housing_live_item_custody SET estate_id="+std::to_string(request.estate_id)+" WHERE custody_id="+std::to_string(item.stored.custody_id)+" AND estate_id="+std::to_string(item.stored.estate_id));if(moved.RowsAffected()!=1)throw std::invalid_argument("Vault relocation custody");}
  const auto house=room?room->profile.room.house_id:0;const auto h=house?std::to_string(house):"NULL";
  auto marker=Query(db,"UPDATE housing_live_vault_items SET native_item_id="+std::to_string(made.LastInsertedID())+",realm_estate_id="+std::to_string(request.estate_id)+",house_id="+h+",revision=revision+1 WHERE custody_id="+std::to_string(item.stored.custody_id)+" AND revision="+std::to_string(item.revision));if(marker.RowsAffected()!=1)throw std::invalid_argument("Vault relocation marker");
  if(house){auto scope=Query(db,"INSERT INTO housing_house_item_scopes(custody_id,house_id,revision) VALUES("+std::to_string(item.stored.custody_id)+","+h+",1)");if(scope.RowsAffected()!=1)throw std::invalid_argument("Vault relocation room");}
  auto membership=Query(db,"UPDATE housing_crate_vault_members SET revision=revision+1,origin_native_estate_id="+std::to_string(target.native_estate_id)+",origin_house_id="+h+",origin_realm_estate_id="+std::to_string(request.estate_id)+",vault_revision=vault_revision+1 WHERE custody_id="+std::to_string(item.stored.custody_id)+" AND crate_id="+std::to_string(carried.binding.crate_id)+" AND revision="+std::to_string(item.membership_revision));if(membership.RowsAffected()!=1)throw std::invalid_argument("Vault relocation membership");
 }
}

}
#endif
