#ifndef EQEMU_HOUSING_HOUSE_SWAP_PREFLIGHT_H
#define EQEMU_HOUSING_HOUSE_SWAP_PREFLIGHT_H
#include "interaction_wire.h"
#include "scene_service.h"
#include "house_binding_service.h"
#include <set>

namespace Housing::HouseSwapPreflight {
// Read-only preparation for the proven same-plot stored estate-item profile.
// This is NOT a swap implementation or a reusable write authorization. No
// mapping/caller is activated: token transitions, complete
// contents migration and an immutable aggregate outcome are still required.
struct Selection {Scene::Item current,replacement;};
inline LiveItems::Result<Selection> Select(const InteractionWire::HouseSwap& wire,
 const Scene::Snapshot& scene,AuthenticatedPurchaser actor){
 using Error=LiveItems::Error;
 if(!actor.character_id||!actor.account_id||!scene.estate_id||
  !InteractionWire::Identity(wire.current.estate_id)||!InteractionWire::Identity(wire.current.item_id)||
  !InteractionWire::Identity(wire.replacement.estate_id)||!InteractionWire::Identity(wire.replacement.item_id)||
  (wire.current.estate_id==wire.replacement.estate_id&&wire.current.item_id==wire.replacement.item_id))return {{},Error::InvalidArgument};
 if(wire.current.estate_id!=scene.native_plot_id)return {{},Error::Conflict};
 // Other plots and crate members require distinct source-graph review. Never
 // fall back to inventory, an arbitrary owned plot, or a matching template.
 if(wire.replacement.estate_id!=scene.native_plot_id)return {{},Error::UnsupportedItem};
 for(auto capability:{AccessPolicy::Capability::AddHouse,AccessPolicy::Capability::RemoveHouse,
  AccessPolicy::Capability::MoveItem,AccessPolicy::Capability::RemoveItem,AccessPolicy::Capability::AddItem})
  if(!scene.access.Allows(capability))return {{},Error::Unauthorized};
 const Scene::Item* current=nullptr;const Scene::Item* replacement=nullptr;
 for(const auto& item:scene.items){
  if(!item.stored.placement)continue;const auto& p=*item.stored.placement;
  if(p.reservation.native_item_id==wire.current.item_id){if(current)return {{},Error::Conflict};current=&item;}
  if(p.reservation.native_item_id==wire.replacement.item_id){if(replacement)return {{},Error::Conflict};replacement=&item;}
 }
 if(!current||!replacement)return {{},Error::NotFound};
 for(const auto* item:{current,replacement}){
  const auto& s=item->stored;const auto& p=*s.placement;
  if(!item->house||!item->house->interior||item->crate||item->contained_in_crate||item->crate_membership)return {{},Error::UnsupportedItem};
  const auto& h=*item->house;
  if(!s.custody_id||s.estate_id!=scene.estate_id||p.reservation.estate_id!=scene.estate_id||
   p.reservation.native_estate_id!=scene.native_plot_id||!p.pose_revision||p.pose_revision==UINT64_MAX||
   !h.house_id||!h.revision||h.revision==UINT64_MAX||!h.pending_key.empty()||
   h.location.kind!=HouseBinding::Location::Kind::Custody||h.location.custody_id!=s.custody_id||
   h.item_template_id!=s.row.values.item_id||!InteractionWire::Identity(h.interior->native_estate_id)||
   !h.interior->zone_id||!h.interior->definition_id||!h.interior->instance_id||
   h.interior->catalog_key.empty()||h.interior->catalog_key!=p.reservation.catalog_key||
   h.interior->catalog_revision!=scene.current_catalog_revision||
   !s.current_catalog_revision||s.current_catalog_revision!=scene.current_catalog_revision||
   s.storage_revision==UINT64_MAX||(s.stowed&&!s.storage_revision))return {{},Error::Conflict};
  // Deliberately narrow server policy; this is not recovered Live eligibility.
  // Delegated or different-creator custody requires an explicit removal policy.
  if(s.row.values.character_id!=actor.character_id)return {{},Error::Unauthorized};
 }
 if(current->stored.stowed||!replacement->stored.stowed||
  current->stored.custody_id==replacement->stored.custody_id||current->house->house_id==replacement->house->house_id||
  current->house->interior->native_estate_id==replacement->house->interior->native_estate_id||
  (current->house->interior->zone_id==replacement->house->interior->zone_id&&
   current->house->interior->instance_id==replacement->house->interior->instance_id))return {{},Error::Conflict};
 return {{*current,*replacement},Error::None};
}
struct Snapshot {Scene::Snapshot yard;Selection houses;Scene::InteriorSnapshot current_room,replacement_room;};
// Cross-check independently enumerated scopes against ALL scene categories.
// A creator's pending item application can suppress a scene entry: treating
// the shortened scene as the complete graph would lose that item on migration.
inline LiveItems::Error CompleteGraphUnderTransaction(Database& db,const Scene::InteriorSnapshot& room){
 using Error=LiveItems::Error;if(!room.house||!room.room_profile)return Error::Conflict;
 std::set<uint64_t> seen;
 auto add=[&](const LiveItems::Stored& s){return s.custody_id&&s.storage_revision!=UINT64_MAX&&seen.insert(s.custody_id).second;};
 for(const auto& i:room.contents)if(!add(i.stored))return Error::Conflict;
 for(const auto& i:room.crate_contents)if(!add(i.stored))return Error::Conflict;
 for(const auto& i:room.vault_items)if(!add(i.stored))return Error::Conflict;
 auto rows=db.QueryDatabase("SELECT scope.custody_id,scope.revision,leases.pending_key FROM housing_house_item_scopes scope JOIN housing_live_item_custody custody ON custody.custody_id=scope.custody_id LEFT JOIN housing_live_item_leases leases ON leases.character_id=custody.owner_character_id WHERE scope.house_id="+std::to_string(room.house->house_id)+" ORDER BY scope.custody_id FOR UPDATE",false);
 if(!rows.Success())return Error::DatabaseFailure;
 for(auto row=rows.begin();row!=rows.end();++row){
  if(!row[0]||!row[1])return Error::DatabaseFailure;
  if(row[2])return Error::PendingApplication;
  const auto revision=std::stoull(row[1]);if(!revision||revision==UINT64_MAX||!seen.erase(std::stoull(row[0])))return Error::Conflict;
 }
 return seen.empty()?Error::None:Error::Conflict;
}
// Caller owns an active transaction and genuine main-thread inventory fence.
// SELECTs/current scene policies only; never BEGIN/COMMIT/ROLLBACK or mutate.
// Successful output captures both reviewed room graphs and checks current
// occupants. It does NOT check destination migration fingerprints/capacities,
// decide the old token's destination, or authorize a write. Re-read for a write.
inline LiveItems::Result<Snapshot> ReadUnderTransaction(Database& db,bool enabled,
 const LiveItems::Lease& lease,EstateId estate,const InteractionWire::HouseSwap& wire,
 LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy& houses,const InteriorContent::ProfilePolicy& rooms,
 const CrateBinding::CustodyContext* crates=nullptr){
 using Error=LiveItems::Error;
 if(!enabled)return {{},Error::Disabled};
 if(!estate)return {{},Error::InvalidArgument};
 if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))return {{},Error::UnheldFence};
 try{
  auto tx=db.QueryDatabase(std::string("SELECT @@in_transaction"),false);
  if(!tx.Success())return {{},Error::DatabaseFailure};
  if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return {{},Error::InvalidArgument};
  auto authenticated=LiveItems::Service::AuthenticateUnderTransaction(db,true,lease,fence);
  if(!authenticated)return {{},authenticated.error};
  if(!authenticated.value.pending_key.empty())return {{},Error::PendingApplication};
  auto engines=db.QueryDatabase(std::string("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_inventory_sessions','housing_house_item_scopes','housing_live_item_custody','housing_live_item_leases')"),false);
  if(!engines.Success())return {{},Error::DatabaseFailure};
  if(engines.RowCount()!=1||!engines.begin()[0]||std::string(engines.begin()[0])!="4")return {{},Error::Disabled};
  auto yard=Scene::Service::ReadSceneUnderTransaction(db,authenticated.value,estate,fence,&houses,crates);
  if(!yard)return {{},yard.error};
  auto selected=Select(wire,yard.value,lease.actor);if(!selected)return {{},selected.error};
  auto current=Scene::Service::ReadAttachedInteriorUnderTransaction(db,authenticated.value,estate,selected.value.current.house->house_id,fence,houses,&rooms,true,crates);
  if(!current)return {{},current.error};
  auto replacement=Scene::Service::ReadAttachedInteriorUnderTransaction(db,authenticated.value,estate,selected.value.replacement.house->house_id,fence,houses,&rooms,true,crates);
  if(!replacement)return {{},replacement.error};
  for(const auto* room:{&current.value,&replacement.value}){
   auto complete=CompleteGraphUnderTransaction(db,*room);if(complete!=Error::None)return {{},complete};
   const auto& interior=*room->house->interior;
   auto occupants=db.QueryDatabase("SELECT character_id FROM housing_inventory_sessions WHERE zone_id="+std::to_string(interior.zone_id)+" AND instance_id="+std::to_string(interior.instance_id)+" AND active=1 FOR UPDATE",false);
   if(!occupants.Success())return {{},Error::DatabaseFailure};
   if(occupants.RowCount())return {{},Error::Conflict};
  }
  return {{std::move(yard.value),std::move(selected.value),std::move(current.value),std::move(replacement.value)},Error::None};
 }catch(...){return {{},Error::DatabaseFailure};}
}
}
#endif
