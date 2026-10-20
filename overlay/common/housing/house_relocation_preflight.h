#ifndef EQEMU_HOUSING_HOUSE_RELOCATION_PREFLIGHT_H
#define EQEMU_HOUSING_HOUSE_RELOCATION_PREFLIGHT_H
#include "house_binding_service.h"
#include "inventory_row_db.h"
#include "crate_binding.h"
#include "crate_member_scope_db.h"
#include "vault_storage_db.h"
#include "item_storage_db.h"
#include <set>
#include <map>
#include <algorithm>
namespace Housing::HouseRelocationPreflight {
// Structural conservation only. The owning caller still proves the real
// creator, current guild/personal ACL, original content and destination. This
// read-only guard works while the retained token is carried or crate-contained;
// a presentation/attached-yard reader cannot authorize that detached source.
namespace Detail {
inline LiveItems::Error StructuralUnderTransaction(Database& db,const HouseBinding::Binding& expected,
 const LiveItems::Lease* lease,LiveItems::InventorySessionFence* fence){
 using Error=LiveItems::Error;
 if(!expected.house_id||!expected.revision||!expected.pending_key.empty())return Error::Conflict;
 if(!expected.interior)return Error::None;
 try{
  auto q=[&](const std::string&sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::invalid_argument("House relocation query");return r;};
  auto n=[](const char*s)->uint64_t{if(!s||!*s)throw std::invalid_argument("House relocation number");std::string v=s;size_t end=0;auto x=std::stoull(v,&end);if(end!=v.size()||v.find_first_not_of("0123456789")!=std::string::npos)throw std::invalid_argument("House relocation number");return x;};
  auto tx=q("SELECT @@in_transaction");if(tx.RowCount()!=1||n(tx.begin()[0])!=1)return Error::InvalidArgument;
  auto engines=q("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_house_instances','housing_house_interiors','housing_house_item_scopes','housing_house_room_profiles','housing_inventory_sessions','housing_live_item_custody','housing_live_placed_items','housing_live_placement_reservations','housing_interior_placement_reservations','housing_live_item_leases')");if(engines.RowCount()!=1||n(engines.begin()[0])!=10)return Error::Disabled;
  const auto&i=*expected.interior;const auto house=std::to_string(expected.house_id),native=std::to_string(i.native_estate_id);
  auto h=q("SELECT h.revision,h.item_template_id,h.pending_request_key,i.native_estate_id,i.definition_id,i.zone_id,i.instance_id,i.zone_version,i.catalog_key,i.catalog_revision,p.revision FROM housing_house_instances h JOIN housing_house_interiors i ON i.house_id=h.house_id JOIN housing_house_room_profiles p ON p.house_id=h.house_id WHERE h.house_id="+house+" FOR UPDATE");
  if(h.RowCount()!=1)return Error::Conflict;auto row=h.begin();
  if(row[2])return Error::PendingApplication;
  if(n(row[0])!=expected.revision||n(row[1])!=expected.item_template_id||n(row[3])!=i.native_estate_id||n(row[4])!=i.definition_id||n(row[5])!=i.zone_id||n(row[6])!=i.instance_id||n(row[7])!=i.zone_version||!row[8]||std::string(row[8])!=i.catalog_key||n(row[9])!=i.catalog_revision||!n(row[10]))return Error::Conflict;
  const auto profile_revision=n(row[10]);
  auto occupants=q("SELECT character_id FROM housing_inventory_sessions WHERE zone_id="+std::to_string(i.zone_id)+" AND instance_id="+std::to_string(i.instance_id)+" AND active=1 FOR UPDATE");if(occupants.RowCount())return Error::Conflict;
  // Includes committed foreign returns whose source custody/scope was already
  // removed. It never interprets a shortened presentation as complete.
  if(lease&&fence){auto pending=LiveItems::Service::NoPendingInteriorPublicationUnderTransaction(db,*lease,i.native_estate_id,expected.house_id,*fence);if(!pending)return pending.error;if(!pending.value)return Error::PendingApplication;}
  else if(q("SELECT character_id FROM housing_live_item_leases WHERE pending_key IS NOT NULL LIMIT 1 FOR UPDATE").RowCount())return Error::PendingApplication;
  std::set<uint64_t> scopes;
  auto scope=q("SELECT custody_id,revision FROM housing_house_item_scopes WHERE house_id="+house+" ORDER BY custody_id LIMIT 16385 FOR UPDATE");if(scope.RowCount()>16384)return Error::UnsupportedItem;
  for(auto r=scope.begin();r!=scope.end();++r)if(!n(r[0])||!n(r[1])||n(r[1])==UINT64_MAX||!scopes.insert(n(r[0])).second)return Error::Conflict;
  // Independent active native-room/tag/vault census detects both a lost scope
  // and a scope whose physical or vault record disappeared. Historical retired
  // reservations are not current actors and never enter this set.
  auto candidates=q("SELECT r.custody_id FROM housing_live_placement_reservations r LEFT JOIN housing_interior_placement_reservations t ON t.native_item_id=r.native_item_id WHERE r.state=1 AND (r.native_estate_id="+native+" OR t.house_id="+house+") ORDER BY r.custody_id LIMIT 16385 FOR UPDATE");if(candidates.RowCount()>16384)return Error::UnsupportedItem;
  std::set<uint64_t> candidate_ids;for(auto r=candidates.begin();r!=candidates.end();++r)if(!n(r[0])||!candidate_ids.insert(n(r[0])).second)return Error::Conflict;
  auto tagged=q("SELECT p.custody_id FROM housing_live_placed_items p JOIN housing_interior_placement_reservations t ON t.native_item_id=p.native_item_id WHERE t.house_id="+house+" ORDER BY p.custody_id LIMIT 16385 FOR UPDATE");if(tagged.RowCount()>16384)return Error::UnsupportedItem;for(auto r=tagged.begin();r!=tagged.end();++r){if(!n(r[0]))return Error::Conflict;candidate_ids.insert(n(r[0]));}if(candidate_ids.size()>16384)return Error::UnsupportedItem;
  if(CrateMemberScope::Installed(db)){auto origins=q("SELECT custody_id FROM housing_crate_members WHERE origin_house_id="+house+" ORDER BY custody_id LIMIT 16385 FOR UPDATE");if(origins.RowCount()>16384)return Error::UnsupportedItem;for(auto r=origins.begin();r!=origins.end();++r)candidate_ids.insert(n(r[0]));}
  if(VaultStorage::CrateMembersInstalled(db)){auto origins=q("SELECT custody_id FROM housing_crate_vault_members WHERE origin_house_id="+house+" ORDER BY custody_id LIMIT 16385 FOR UPDATE");if(origins.RowCount()>16384)return Error::UnsupportedItem;for(auto r=origins.begin();r!=origins.end();++r)candidate_ids.insert(n(r[0]));}if(candidate_ids.size()>16384)return Error::UnsupportedItem;
  // Storage and empty placed crates retain independent room provenance even
  // when every normal scope/reservation/tag link has been lost.
  if(ItemStorage::Installed(db)){auto markers=q("SELECT custody_id FROM housing_live_item_storage WHERE native_estate_id="+native+" ORDER BY custody_id LIMIT 16385 FOR UPDATE");if(markers.RowCount()>16384)return Error::UnsupportedItem;for(auto r=markers.begin();r!=markers.end();++r){if(!n(r[0]))return Error::Conflict;candidate_ids.insert(n(r[0]));}}
  std::map<uint64_t,uint64_t> placed_crates;
  auto crate_tables=q("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_crate_estates','housing_crate_locations')");if(crate_tables.RowCount()&&crate_tables.RowCount()!=2)return Error::Disabled;for(auto r=crate_tables.begin();r!=crate_tables.end();++r)if(!r[0]||std::string(r[0])!="InnoDB")return Error::Disabled;
  if(crate_tables.RowCount()){
   auto crates=q("SELECT e.crate_id,l.location_kind,l.custody_id,l.character_id,l.slot_id FROM housing_crate_estates e LEFT JOIN housing_crate_locations l ON l.crate_id=e.crate_id WHERE e.parent_native_estate_id="+native+" ORDER BY e.crate_id LIMIT 16385 FOR UPDATE");if(crates.RowCount()>16384)return Error::UnsupportedItem;
   for(auto r=crates.begin();r!=crates.end();++r){if(!n(r[0])||!r[1])return Error::Conflict;const auto kind=n(r[1]);if(kind==1){if(r[2]||!r[3]||!n(r[3])||!r[4]||n(r[4])<23||n(r[4])>33)return Error::Conflict;continue;}if(kind!=2||!r[2]||!n(r[2])||r[3]||r[4]||!placed_crates.emplace(n(r[2]),n(r[0])).second)return Error::Conflict;candidate_ids.insert(n(r[2]));}
  }
  if(candidate_ids.size()>16384)return Error::UnsupportedItem;
  const auto all_scopes=scopes;std::set<uint64_t> seen,carried_checked,placed_checked;
  const bool have_vault=VaultStorage::Installed(db);
  auto vaults=have_vault?VaultStorage::Items(db,0,i.native_estate_id,expected.house_id,{true,true,false}):std::vector<VaultStorage::Item>{};
  auto detached=[&](uint64_t custody,uint64_t crate)->bool{
   if(!crate||scopes.count(custody))return false;
   if(carried_checked.count(crate))return true;
   auto location=q("SELECT character_id,slot_id FROM housing_crate_locations WHERE crate_id="+std::to_string(crate)+" AND location_kind=1 AND custody_id IS NULL FOR UPDATE");if(location.RowCount()!=1)return false;auto l=location.begin();
   auto inventory=q("SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(n(l[0]))+" AND slot_id="+std::to_string(n(l[1]))+" FOR UPDATE");if(inventory.RowCount()!=1)return false;
   auto carried=CrateBinding::ReadCarriedUnderTransaction(db,InventoryRowDB::Decode(inventory.begin()));if(!carried||carried.value.binding.crate_id!=crate||carried.value.binding.parent_native_estate_id!=i.native_estate_id||carried.value.binding.catalog_key!=i.catalog_key||carried.value.binding.catalog_revision!=i.catalog_revision||carried.value.packed)return false;
   auto members=CrateBinding::ValidateMembersUnderTransaction(db,carried.value);if(!members)return false;carried_checked.insert(crate);return true;
  };
  for(const auto custody:candidate_ids){
   if(!seen.insert(custody).second)return Error::Conflict;
   auto stored=LiveItems::Service::InspectCustodyUnderTransaction(db,custody,true);if(!stored)return stored.error;
   const auto storage=ItemStorage::Locked(db,custody);if(storage.revision&&(!stored.value.placement||storage.native_estate_id!=i.native_estate_id))return Error::Conflict;
   auto crate=placed_crates.find(custody);if(crate!=placed_crates.end()){auto allocation=CrateBinding::ReadPlacedUnderTransaction(db,stored.value);if(!allocation||allocation.value.binding.crate_id!=crate->second||allocation.value.binding.parent_native_estate_id!=i.native_estate_id)return Error::Conflict;auto members=CrateBinding::ValidateMembersUnderTransaction(db,allocation.value);if(!members)return members.error;placed_checked.insert(crate->second);}
   auto creator=q("SELECT pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(stored.value.row.values.character_id)+" FOR UPDATE");if(creator.RowCount()>1)return Error::Conflict;if(creator.RowCount()&&creator.begin()[0])return Error::PendingApplication;
   auto vault=std::find_if(vaults.begin(),vaults.end(),[&](const auto&v){return v.stored.custody_id==custody;});
   if(vault!=vaults.end()){
    if(stored.value.placement)return Error::Conflict;
    if(!scopes.erase(custody)&&!detached(custody,vault->contained_in_crate))return Error::Conflict;
    continue;
   }
   if(!stored.value.placement||stored.value.placement->reservation.native_estate_id!=i.native_estate_id)return Error::Conflict;
   const auto&p=*stored.value.placement;
   auto tag=q("SELECT house_id,house_revision,profile_revision,source_zone,source_version,source_instance,content_version,OCTET_LENGTH(content_digest) FROM housing_interior_placement_reservations WHERE native_item_id="+std::to_string(p.reservation.native_item_id)+" FOR UPDATE");if(tag.RowCount()!=1)return Error::Conflict;auto t=tag.begin();
   if(n(t[0])!=expected.house_id||!n(t[1])||n(t[1])>expected.revision||n(t[2])!=profile_revision||n(t[3])!=i.zone_id||n(t[4])!=i.zone_version||n(t[5])!=i.instance_id||n(t[6])!=InteriorContent::Version||n(t[7])!=32)return Error::Conflict;
   auto membership=CrateMemberScope::Locked(db,custody,true);
   if(membership&&(membership->origin_house_id!=expected.house_id||membership->origin_native_estate_id!=i.native_estate_id||!stored.value.stowed))return Error::Conflict;
   if(membership&&all_scopes.count(custody)&&!placed_checked.count(membership->crate_id)){
    auto location=q("SELECT custody_id FROM housing_crate_locations WHERE crate_id="+std::to_string(membership->crate_id)+" AND location_kind=2 AND character_id IS NULL AND slot_id IS NULL FOR UPDATE");if(location.RowCount()!=1||!location.begin()[0]||!all_scopes.count(n(location.begin()[0])))return Error::Conflict;
    auto container=LiveItems::Service::InspectCustodyUnderTransaction(db,n(location.begin()[0]));if(!container)return container.error;auto allocation=CrateBinding::ReadPlacedUnderTransaction(db,container.value);if(!allocation||allocation.value.binding.crate_id!=membership->crate_id||allocation.value.binding.parent_native_estate_id!=i.native_estate_id)return Error::Conflict;auto members=CrateBinding::ValidateMembersUnderTransaction(db,allocation.value);if(!members)return members.error;placed_checked.insert(membership->crate_id);
   }
   if(!scopes.erase(custody)&&(!membership||!detached(custody,membership->crate_id)))return Error::Conflict;
  }
  // VaultStorage's validated reader intentionally joins its reservation and
  // custody. Independently enumerate raw room markers too, so a marker with a
  // missing reservation/custody/scope cannot disappear through that inner join.
  if(have_vault){auto markers=q("SELECT custody_id FROM housing_live_vault_items WHERE house_id="+house+" ORDER BY custody_id LIMIT 16385 FOR UPDATE");if(markers.RowCount()>16384)return Error::UnsupportedItem;for(auto v=markers.begin();v!=markers.end();++v)if(!seen.count(n(v[0]))||std::none_of(vaults.begin(),vaults.end(),[&](const auto&item){return item.stored.custody_id==n(v[0]);}))return Error::Conflict;}
  return scopes.empty()?Error::None:Error::Conflict;
 }catch(...){return Error::DatabaseFailure;}
}
} // namespace Detail
inline LiveItems::Error UnderTransaction(Database& db,const HouseBinding::Binding& expected,
 const LiveItems::Lease& lease,LiveItems::InventorySessionFence& fence){
 if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))return LiveItems::Error::Conflict;
 return Detail::StructuralUnderTransaction(db,expected,&lease,&fence);
}
// Offline callers must have locked/revalidated real owner/account, offline
// sessions and pending journals, and exact carried typed inventory binding.
// This entry grants no actor ACL or lease and never fabricates an online fence.
inline LiveItems::Error OfflineStructuralUnderTransaction(Database& db,const HouseBinding::Binding& expected){
 return Detail::StructuralUnderTransaction(db,expected,nullptr,nullptr);
}

}
#endif
