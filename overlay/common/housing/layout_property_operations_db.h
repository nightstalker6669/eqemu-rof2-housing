#ifndef EQEMU_HOUSING_LAYOUT_PROPERTY_OPERATIONS_DB_H
#define EQEMU_HOUSING_LAYOUT_PROPERTY_OPERATIONS_DB_H
#include "vault_storage_db.h"
#include "layout_wire.h"
#include "rent_state_db.h"
#include "crate_storage_service.h"
#include "interior_storage_service.h"
#include "custody_codec.h"
#include "crate_member_scope_db.h"
#include "crate_foreign_member_policy_db.h"
#include "inventory_projection.h"
#include "npc_item_metadata.h"
#include "catalog_content_db.h"
#include "layout_crate_identity.h"
#include "layout_house_identity.h"
#include "layout_crate_member_identity.h"
#include "interior_item_content_db.h"
#include <set>
#include <openssl/sha.h>
namespace Housing::LayoutProperty {
// Server-owned current scene/GUID witnesses. Incoming GUID text never creates
// these entries. Native GUID is the CURRENT serialized ItemInstance serial,
// not persisted inventory.row.guid, which may differ after reconstruction.
struct Witness {uint64_t custody_id=0;std::string guid;LiveItems::InventoryRow row;EQ::ItemInstance* instance=nullptr;LayoutCrate::Allocation crate;LayoutHouse::Identity house;LayoutCrateMember::Proof member;};
using Identity=std::pair<uint32_t,uint32_t>;
struct Context {EstateId estate_id=0;uint32_t yard_id=0,room_id=0;uint64_t house_id=0;Route physical_source;bool inside=false;};
struct State {
 std::map<Identity,Witness> witnesses;
 std::map<Identity,uint64_t> aliases;
 std::map<uint64_t,uint32_t> locations;
 // Immutable scene witnesses above; exact transaction-local count postimages below.
 std::map<uint64_t,LiveItems::InventoryRow> projected_rows;
 std::set<uint64_t> retired,released_members;
 std::map<uint64_t,LayoutCrate::Allocation> projected_crates;
 // SERVER cache/planner target preference, never parsed from a client packet.
 // The original Live server tie-break is unknown. No invented numeric-ID sort.
 std::vector<Identity> consolidation_order;
 // Own newly split pieces until parent COMMIT/application/ACK is known.
 // Rollback requires discarding this whole State, not returning pieces to inventory.
 std::map<uint64_t,std::unique_ptr<EQ::ItemInstance>> split_units;
};
struct Views {Scene::Snapshot yard;std::optional<Scene::InteriorSnapshot> room;};
struct Item {LiveItems::Stored stored;std::optional<LiveItems::InteriorMoveContext> room;LayoutCrate::Allocation crate;LayoutHouse::Identity house;LayoutCrateMember::Proof member;};
enum class Action:uint8_t {NoChange,Move,Stow,Restore,Transfer,TransferAndPlace,Consolidate,SplitAndPlace,ExtractAndPlace,ExtractAndStore,MemberConsolidate,MemberSplitAndPlace,SplitAndStore,MemberSplitAndStore,SplitToCrate};
inline bool CreatedSplit(Action a){return a==Action::SplitAndPlace||a==Action::MemberSplitAndPlace||a==Action::SplitAndStore||a==Action::MemberSplitAndStore||a==Action::SplitToCrate;}
inline bool StoredSplit(Action a){return a==Action::SplitAndStore||a==Action::MemberSplitAndStore||a==Action::SplitToCrate;}
struct CountEffect {LiveItems::Stored before;std::optional<LiveItems::Stored> after;std::optional<LiveItems::InteriorMoveContext> room;};
struct Effect {LayoutCrateMember::Proof created_member;std::optional<CountEffect> container_change;std::vector<CountEffect> counts;bool source_retired=false;Action action=Action::NoChange;LiveItems::Stored before,after;std::optional<LiveItems::InteriorMoveContext> before_room,after_room;};
namespace Detail {
using Error=LiveItems::Error;struct Failure {Error error;};
inline bool Same(Route a,Route b){return a.zone_id==b.zone_id&&a.zone_version==b.zone_version&&a.instance_id==b.instance_id;}
inline Views Read(Database& db,const LiveItems::Lease& lease,const Context& c,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* houses,const InteriorContent::ProfilePolicy* rooms,const CrateBinding::CustodyContext* crates,bool need_room=true){
 auto yard=c.inside?(houses?Scene::Service::ReadParentSceneUnderTransaction(db,lease,c.physical_source,fence,*houses,crates):LiveItems::Result<Scene::Snapshot>{{},Error::Unauthorized}):Scene::Service::ReadSceneUnderTransaction(db,lease,c.estate_id,fence,houses,crates);if(!yard)throw Failure{yard.error};
 if(!RentState::PlotActive(db,c.estate_id,c.yard_id))throw Failure{Error::Unauthorized};
 if(yard.value.estate_id!=c.estate_id||yard.value.native_plot_id!=c.yard_id||(!c.inside&&!Same(yard.value.route,c.physical_source)))throw Failure{Error::Conflict};Views out;out.yard=std::move(yard.value);
 if(c.house_id&&need_room){if(!houses||!rooms)throw Failure{Error::Unauthorized};auto room=c.inside?Scene::Service::ReadInteriorSceneUnderTransaction(db,lease,c.physical_source,fence,*houses,*rooms,crates):Scene::Service::ReadAttachedInteriorUnderTransaction(db,lease,c.estate_id,c.house_id,fence,*houses,rooms,true,crates);if(!room)throw Failure{room.error};
  if(!room.value.house||!room.value.house->interior||room.value.house->house_id!=c.house_id||room.value.house->interior->native_estate_id!=c.room_id||(!c.inside&&(!room.value.exterior_source||!Same(*room.value.exterior_source,c.physical_source))))throw Failure{Error::Conflict};out.room=std::move(room.value);
 }return out;
}
inline LiveItems::OccupiedParentContext ParentOrigin(const Views& view,const Context& c){
 if(!c.inside||!view.room||!view.room->house||!view.room->room_profile||view.room->house->house_id!=c.house_id)throw Failure{Error::Conflict};
 return {c.house_id,view.room->house->revision,view.room->profile_revision,c.physical_source};
}
inline std::optional<Item> Find(const Views& view,uint64_t custody){
 for(const auto& p:view.yard.items)if(p.stored.custody_id==custody){return Item{p.stored,{},p.crate,p.house};}
 if(view.room)for(const auto& p:view.room->contents)if(p.stored.custody_id==custody){if(!p.room||!view.room->house)throw Failure{Error::UnsupportedItem};return Item{p.stored,LiveItems::InteriorMoveContext{view.room->house->house_id,view.room->house->revision,view.room->profile_revision,p.scope_revision,view.room->context.source},p.crate};}
 for(const auto& p:view.yard.crate_contents)if(p.stored.custody_id==custody){auto container=std::find_if(view.yard.items.begin(),view.yard.items.end(),[&](const auto& c){return c.crate&&c.crate->binding.crate_id==p.contained_in_crate;});if(container==view.yard.items.end())throw Failure{Error::Conflict};return Item{p.stored,{},{},p.house,LayoutCrateMember::FromSnapshot(p.stored,p.crate_membership,container->crate)};}
 if(view.room)for(const auto& p:view.room->crate_contents)if(p.stored.custody_id==custody){if(!p.room||!view.room->house)throw Failure{Error::Conflict};auto container=std::find_if(view.room->contents.begin(),view.room->contents.end(),[&](const auto& c){return c.crate&&c.crate->binding.crate_id==p.contained_in_crate;});if(container==view.room->contents.end())throw Failure{Error::Conflict};return Item{p.stored,LiveItems::InteriorMoveContext{view.room->house->house_id,view.room->house->revision,view.room->profile_revision,p.scope_revision,view.room->context.source},{},{},LayoutCrateMember::FromSnapshot(p.stored,p.crate_membership,container->crate)};}
 return {};
}
inline Item Source(const Views& views,State& state,Identity id,const std::optional<std::string>& guid){
 auto alias=state.aliases.find(id);uint64_t custody=0;
 if(alias==state.aliases.end()){
  auto witness=state.witnesses.find(id);if(witness==state.witnesses.end()||!witness->second.custody_id||(guid&&*guid!=witness->second.guid))throw Failure{Error::Conflict};custody=witness->second.custody_id;auto current=Find(views,custody);
  if(!current||!current->stored.placement||(current->member?LayoutCrateMember::SourceEstate(*current->member):current->stored.placement->reservation.native_estate_id)!=id.first||current->stored.placement->reservation.native_item_id!=id.second||CustodyCodec::Encode(current->stored.row)!=CustodyCodec::Encode(state.projected_rows.count(custody)?state.projected_rows.at(custody):witness->second.row))throw Failure{Error::Conflict};state.aliases.emplace(id,custody);
 }else{custody=alias->second;if(guid){auto witness=state.witnesses.find(id);if(witness==state.witnesses.end()||witness->second.guid!=*guid)throw Failure{Error::Conflict};}}
 auto current=Find(views,custody);if(state.retired.count(custody)||!current||!current->stored.placement)throw Failure{Error::Conflict};auto original=state.witnesses.find(id);const auto projected=state.projected_rows.find(custody);if(original==state.witnesses.end()||original->second.custody_id!=custody||CustodyCodec::Encode(projected==state.projected_rows.end()?original->second.row:projected->second)!=CustodyCodec::Encode(current->stored.row))throw Failure{Error::Conflict};if(!LayoutHouse::Same(original->second.house,current->house)||(current->house&&(!LayoutHouse::Shape(*current->house,current->stored)||!LayoutHouse::Matches(current->house,original->second.instance))))throw Failure{Error::Conflict};if(current->member){if(!original->second.member)throw Failure{Error::Conflict};auto expected=*original->second.member;auto projection=state.projected_crates.find(expected.membership.crate_id);if(projection!=state.projected_crates.end())expected.container=projection->second;if(!LayoutCrateMember::Same(std::make_shared<const LayoutCrateMember::Record>(expected),current->member))throw Failure{Error::Conflict};}else if(original->second.member&&!state.released_members.count(custody))throw Failure{Error::Conflict};auto expected_crate=original->second.crate;if(expected_crate){auto projection=state.projected_crates.find(expected_crate->binding.crate_id);if(projection!=state.projected_crates.end())expected_crate=projection->second;}if(!LayoutCrate::Same(expected_crate,current->crate)||(current->crate&&(!LayoutCrate::Shape(*current->crate,current->stored)||!LayoutCrate::Matches(current->crate,original->second.instance))))throw Failure{Error::Conflict};state.locations[custody]=current->stored.placement->reservation.native_estate_id;return *current;
}
inline uint32_t Quantity(Database& db,const LiveItems::InventoryRow& row){auto metadata=db.QueryDatabase("SELECT stackable FROM items WHERE id="+std::to_string(row.values.item_id)+" FOR UPDATE",false);if(!metadata.Success())throw Failure{Error::DatabaseFailure};if(metadata.RowCount()!=1||!metadata.begin()[0])throw Failure{Error::UnsupportedItem};if(std::stoul(metadata.begin()[0])){auto n=row.values.charges;if(n>=32767)throw Failure{Error::UnsupportedItem};return n?n:1;}return 1;}
// Ordinary-stack semantic equality. RoF2 projects instnodrop to +80 and
// Copied=0. Preserve additional emulator payload too: its Live merge policy is
// unknown, so unlike custom/augment/ornament/color payloads cannot be discarded.
inline bool Compatible(LiveItems::InventoryRow a,LiveItems::InventoryRow b){
 if(!CustodyCodec::Valid(a)||!CustodyCodec::Valid(b))return false;
 for(auto* row:{&a,&b}){row->values.slot_id=0;row->values.guid=0;row->values.charges=0;row->null_fields&=~uint8_t(LiveItems::NullCharges|LiveItems::NullGuid);}
 return CustodyCodec::Encode(a)==CustodyCodec::Encode(b);
}
inline uint32_t StackMaximum(Database& db,const LiveItems::Stored& item){
 if(!item.row.children.empty()||!item.row.evolving_payload.empty()||item.row.values.augment_one||item.row.values.augment_two||item.row.values.augment_three||item.row.values.augment_four||item.row.values.augment_five||item.row.values.augment_six||item.catalog.item_class||item.catalog.bag_slots||item.catalog.lore_group)throw Failure{Error::UnsupportedItem};
 auto r=db.QueryDatabase("SELECT stackable,stacksize FROM items WHERE id="+std::to_string(item.row.values.item_id)+" FOR UPDATE",false);
 if(!r.Success())throw Failure{Error::DatabaseFailure};if(r.RowCount()!=1||!r.begin()[0]||!r.begin()[1])throw Failure{Error::UnsupportedItem};
 const auto maximum=std::stoll(r.begin()[1]);if(!std::stoul(r.begin()[0])||maximum<=1||maximum>=32767)throw Failure{Error::UnsupportedItem};return uint32_t(maximum);
}
inline MySQLRequestResult Query(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw Failure{Error::DatabaseFailure};return r;}
inline std::string Blob(const LiveItems::InventoryRow& row){const char* digits="0123456789abcdef";std::string result="x'";for(auto byte:CustodyCodec::Encode(row)){result+=digits[byte>>4];result+=digits[byte&15];}return result+"'";}
// Internal reviewed-item write: no independent authority or transaction lifecycle.
inline std::optional<LiveItems::Stored> WriteCount(Database& db,const Item& item,uint32_t count,LayoutCrateMember::Proof member={}){
 const auto& before=item.stored;const auto custody=std::to_string(before.custody_id),native=std::to_string(before.placement->reservation.native_item_id);
 if(count>=32767||before.storage_revision==UINT64_MAX)throw Failure{Error::UnsupportedItem};
 const auto scope=CrateMemberScope::Locked(db,before.custody_id,true);
 if(bool(scope)!=bool(member)||(member&&(!LayoutCrateMember::Same(member,item.member)||!LayoutCrateMember::Shape(*member,before)||scope->crate_id!=member->membership.crate_id||scope->revision!=member->membership.revision)))throw Failure{Error::Conflict};
 // Contained members are governed by their freshly validated current placed
 // container/host, not a historical member custody estate or origin snapshot.
 if(member){const auto& host=member->container->binding;if(!RentState::MutableItem(db,host.estate_id,0,host.custody_id))throw Failure{Error::Unauthorized};}
 else if(!RentState::MutableItem(db,before.estate_id,0,before.custody_id))throw Failure{Error::Unauthorized};
 const auto marker=ItemStorage::Locked(db,before.custody_id);if(marker.revision!=before.storage_revision||marker.stowed!=before.stowed||marker.revision==UINT64_MAX||(marker.revision&&marker.native_estate_id!=before.placement->reservation.native_estate_id))throw Failure{Error::Conflict};
 auto one=[&](const std::string& sql){if(Query(db,sql).RowsAffected()!=1)throw Failure{Error::Conflict};};
 if(!count){
  if(member)one("DELETE FROM housing_crate_members WHERE custody_id="+custody+" AND crate_id="+std::to_string(member->membership.crate_id)+" AND revision="+std::to_string(member->membership.revision));
  // Historical reservation/room fingerprint remain; retire only the live links.
  ItemStorage::Retire(db,before.custody_id);
  if(item.room)one("DELETE FROM housing_house_item_scopes WHERE custody_id="+custody+" AND house_id="+std::to_string(item.room->house_id)+" AND revision="+std::to_string(item.room->scope_revision));
  one("DELETE FROM housing_live_placed_items WHERE custody_id="+custody+" AND native_item_id="+native+" AND pose_revision="+std::to_string(before.placement->pose_revision));
  one("UPDATE housing_live_placement_reservations SET state=2,custody_id=NULL WHERE native_item_id="+native+" AND custody_id="+custody+" AND state=1 AND source_row="+Blob(before.row));
  one("DELETE FROM housing_live_item_custody WHERE custody_id="+custody+" AND row_blob="+Blob(before.row));return {};
 }
 auto after=before;after.row.values.charges=count;after.row.null_fields&=~LiveItems::NullCharges;
 if(CustodyCodec::Encode(after.row)==CustodyCodec::Encode(before.row))throw Failure{Error::Conflict};
 one("UPDATE housing_live_item_custody SET row_blob="+Blob(after.row)+" WHERE custody_id="+custody+" AND row_blob="+Blob(before.row));
 one("UPDATE housing_live_placement_reservations SET source_row="+Blob(after.row)+" WHERE native_item_id="+native+" AND custody_id="+custody+" AND state=1 AND source_row="+Blob(before.row));
 if(marker.revision)one("UPDATE housing_live_item_storage SET revision=revision+1 WHERE custody_id="+custody+" AND revision="+std::to_string(marker.revision));
 else one("INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+custody+",1,0,"+std::to_string(before.placement->reservation.native_estate_id)+")");
 after.storage_revision++;return after;
}
// Decide before writes whether the existing compatible-target operation can
// receive the complete count. A strict partial with insufficient room creates
// one actual stowed piece instead; it never partly consolidates then falls back.
inline bool ConsolidationFits(Database& db,const Views& views,State& state,const Item& source,uint32_t amount,uint32_t target){
 if(!source.member&&(target!=source.stored.placement->reservation.native_estate_id||source.stored.stowed))return false;
 const auto maximum=StackMaximum(db,source.stored),available=Quantity(db,source.stored.row);
 if(!amount||amount>available||available>maximum)throw Failure{Error::Conflict};
 std::set<uint64_t> seen;uint32_t remaining=amount;
 for(const auto& identity:state.consolidation_order){
  if(!remaining)break;if(identity.first!=target)continue;
  auto witness=state.witnesses.find(identity);if(witness==state.witnesses.end())throw Failure{Error::Conflict};
  if(witness->second.custody_id==source.stored.custody_id||state.retired.count(witness->second.custody_id)||!seen.insert(witness->second.custody_id).second)continue;
  auto candidate=Source(views,state,identity,{});if(candidate.member||!candidate.stored.stowed||(!source.member&&bool(candidate.room)!=bool(source.room))||candidate.stored.placement->reservation.native_estate_id!=target||!Compatible(source.stored.row,candidate.stored.row))continue;
  if(StackMaximum(db,candidate.stored)!=maximum)throw Failure{Error::Conflict};const auto count=Quantity(db,candidate.stored.row);if(count>maximum)throw Failure{Error::Conflict};remaining-=std::min(remaining,maximum-count);
 }
 return !remaining;
}
inline Effect Consolidate(Database& db,const Views& views,State& state,const Item& source,uint32_t amount,uint32_t target){
 if(!source.member&&(target!=source.stored.placement->reservation.native_estate_id||source.stored.stowed))throw Failure{Error::UnsupportedItem};
 const auto& access=source.room?views.room->access.effective:views.yard.access;
 if(!access.Allows(AccessPolicy::Capability::MoveItem))throw Failure{Error::Unauthorized};
 if(source.member){const auto& destination=views.room&&views.room->house&&target==views.room->house->interior->native_estate_id?views.room->access.effective:views.yard.access;
  if(!destination.Allows(AccessPolicy::Capability::AddItem))throw Failure{Error::Unauthorized};}
 const auto maximum=StackMaximum(db,source.stored),available=Quantity(db,source.stored.row);
 if(!amount||amount>available||available>maximum)throw Failure{Error::Conflict};
 struct Target {Item item;uint32_t count,receive;};std::vector<Target> targets;std::set<uint64_t> seen;uint32_t remaining=amount;
 // Preflight the whole amount before any writes. Order is an explicit server
 // scene witness. Missing/insufficient compatible storage is a whole-batch error.
 for(const auto& identity:state.consolidation_order){
  if(!remaining)break;if(identity.first!=target)continue;
  auto witness=state.witnesses.find(identity);if(witness==state.witnesses.end())throw Failure{Error::Conflict};
  if(witness->second.custody_id==source.stored.custody_id||state.retired.count(witness->second.custody_id)||!seen.insert(witness->second.custody_id).second)continue;
  auto candidate=Source(views,state,identity,{});if(candidate.member||!candidate.stored.stowed||(!source.member&&bool(candidate.room)!=bool(source.room))||candidate.stored.placement->reservation.native_estate_id!=target||!Compatible(source.stored.row,candidate.stored.row))continue;
  if(StackMaximum(db,candidate.stored)!=maximum)throw Failure{Error::Conflict};const auto count=Quantity(db,candidate.stored.row);if(count>maximum)throw Failure{Error::Conflict};const auto receive=std::min(remaining,maximum-count);if(receive){targets.push_back({candidate,count,receive});remaining-=receive;}
 }
 if(remaining)throw Failure{Error::Conflict};
 Effect effect;effect.action=source.member?Action::MemberConsolidate:Action::Consolidate;effect.before=source.stored;effect.before_room=source.room;
 auto project=[&](const Item& item,uint32_t count){auto after=WriteCount(db,item,count,item.member);effect.counts.push_back({item.stored,after,item.room});if(after)state.projected_rows[item.stored.custody_id]=after->row;else{state.retired.insert(item.stored.custody_id);state.projected_rows.erase(item.stored.custody_id);}return after;};
 auto source_after=project(source,available-amount);effect.source_retired=!source_after;if(source_after){effect.after=*source_after;effect.after_room=source.room;}
 for(const auto& candidate:targets)project(candidate.item,candidate.count+candidate.receive);
 return effect;
}
inline std::string Key(const std::string& parent,size_t ordinal,const std::string& step){const auto input=parent+":"+std::to_string(ordinal)+":"+step;std::array<uint8_t,SHA256_DIGEST_LENGTH> digest;SHA256(reinterpret_cast<const unsigned char*>(input.data()),input.size(),digest.data());const char* hex="0123456789abcdef";std::string key="layout-property:";for(auto byte:digest){key+=hex[byte>>4];key+=hex[byte&15];}return key;}
struct Split {Item piece;CountEffect source;};
struct SplitDestination {
 LiveItems::Reservation reservation;
 std::optional<LiveItems::InteriorReservation> room;
 PlacementPolicy::Transform transform;
};
inline uint64_t SplitDestinationCount(Database& db,const Views& views,bool room,uint32_t target,bool stowed){
 uint64_t vault=0;
 if(stowed){
  // One new stowed custody consumes the same total estate capacity as the
  // existing list-only vault entries. Optional absence is supported; a partial
  // or nontransactional vault schema must never become free capacity.
  auto tables=Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_live_vault_items','housing_live_vault_transfers')");
  if(tables.RowCount()){if(tables.RowCount()!=2)throw Failure{Error::DatabaseFailure};for(auto row=tables.begin();row!=tables.end();++row)if(!row[0]||std::string(row[0])!="InnoDB")throw Failure{Error::DatabaseFailure};
   auto used=Query(db,"SELECT COUNT(*) FROM housing_live_vault_items v JOIN housing_live_placement_reservations r ON r.native_item_id=v.native_item_id WHERE r.native_estate_id="+std::to_string(target)+" AND r.state=1"+VaultStorage::Uncontained(db)+" FOR UPDATE");if(used.RowCount()!=1||!used.begin()[0])throw Failure{Error::DatabaseFailure};vault=std::stoull(used.begin()[0]);
  }
 }
 return (room?views.room->contents.size():views.yard.items.size())+vault;
}
// Fresh scene authority and real destination geometry/capacity, before creating
// any unit. A full source realm does not consume an extra transient slot.
inline SplitDestination ReviewSplitDestination(Database& db,const LiveItems::Lease& lease,LiveItems::InventorySessionFence& fence,const Views& views,const Item& source,uint32_t target,const PlacementPolicy::Transform& pose,bool stowed=false,const CrateBinding::Allocation* contained=nullptr){
 const bool room=views.room&&views.room->house&&views.room->room_profile&&target==views.room->house->interior->native_estate_id;
 if(!room&&target!=views.yard.native_plot_id)throw Failure{Error::Conflict};
 const auto& from=source.room?views.room->access.effective:views.yard.access;
 const auto& access=room?views.room->access.effective:views.yard.access;
 if(!from.Allows(AccessPolicy::Capability::MoveItem)||!access.Allows(AccessPolicy::Capability::AddItem))throw Failure{Error::Unauthorized};
 const auto& definition=room?views.room->room_profile->definition:views.yard.definition;
 if(contained){
  if(!stowed||contained->packed||contained->binding.parent_native_estate_id!=target)throw Failure{Error::Conflict};
  auto actual=Find(views,contained->binding.custody_id);if(!actual||actual->stored.stowed||!LayoutCrate::Same(actual->crate,std::make_shared<const CrateBinding::Allocation>(*contained)))throw Failure{Error::Conflict};
  auto capacity=CrateBinding::ReadDefinitionUnderTransaction(db,*contained);if(!capacity)throw Failure{capacity.error};size_t count=0;for(const auto& i:views.yard.crate_contents)count+=i.contained_in_crate==contained->binding.crate_id;if(views.room)for(const auto& i:views.room->crate_contents)count+=i.contained_in_crate==contained->binding.crate_id;if(!capacity.value.max_items||count>=capacity.value.max_items)throw Failure{Error::Conflict};
 }else{const auto count=SplitDestinationCount(db,views,room,target,stowed);if(!definition.max_items||count>=definition.max_items)throw Failure{Error::Conflict};}
 auto maximum=Query(db,"SELECT UNK231,UNK226,UNK227 FROM items WHERE id="+std::to_string(source.stored.row.values.item_id)+" FOR UPDATE");
 if(maximum.RowCount()!=1||!maximum.begin()[0]||!maximum.begin()[1]||!maximum.begin()[2]||std::stoll(maximum.begin()[2]))throw Failure{Error::UnsupportedItem};
 const auto mounting=std::stoll(maximum.begin()[1]);const auto* mounted=StaticFurniture::Lookup(source.stored.row.values.item_id);
 if(mounting&&(mounting<1||mounting>2||!mounted||!StaticFurniture::Mounted(mounted->item_id)||mounted->mounting_type!=mounting))throw Failure{Error::UnsupportedItem};
 const auto limit=std::stoll(maximum.begin()[0]);if(limit<INT32_MIN||limit>INT32_MAX)throw Failure{Error::UnsupportedItem};
 uint64_t same=0;if(contained){for(const auto& i:views.yard.crate_contents)same+=i.contained_in_crate==contained->binding.crate_id&&i.stored.row.values.item_id==source.stored.row.values.item_id;if(views.room)for(const auto& i:views.room->crate_contents)same+=i.contained_in_crate==contained->binding.crate_id&&i.stored.row.values.item_id==source.stored.row.values.item_id;}else if(room){for(const auto& i:views.room->contents)if(i.stored.row.values.item_id==source.stored.row.values.item_id)++same;}else{for(const auto& i:views.yard.items)if(i.stored.row.values.item_id==source.stored.row.values.item_id)++same;}
 if(limit>0&&same>=uint64_t(limit))throw Failure{Error::Conflict};
 SplitDestination result;result.transform=pose;auto& reservation=result.reservation;
 reservation.native_estate_id=target;reservation.estate_id=source.stored.placement->reservation.native_estate_id==target?source.stored.estate_id:views.yard.estate_id;
 reservation.catalog_revision=views.yard.current_catalog_revision;
 auto parent=Query(db,"SELECT catalog_key FROM housing_native_estates WHERE native_id="+std::to_string(views.yard.native_plot_id)+" AND active=1 FOR UPDATE");
 if(parent.RowCount()!=1||!parent.begin()[0])throw Failure{Error::Conflict};reservation.catalog_key=parent.begin()[0];
 PlacementPolicy::ItemMetadata metadata;PlacementPolicy::Geometry geometry;
 if(room){
  const auto& inside=*views.room;auto input=InteriorContent::ItemInputUnderTransaction(db,*inside.room_profile,source.stored.row.values.item_id,{},true);if(!input)throw Failure{input.error};auto identity=InteriorContent::Build(input.value);if(!identity)throw Failure{Error::UnsupportedItem};
  if(inside.house->interior->catalog_key!=reservation.catalog_key||inside.context.catalog_revision!=reservation.catalog_revision)throw Failure{Error::Conflict};
  metadata=input.value.item;geometry=input.value.geometry;
  PlacementPolicy::Context checked{target,definition.type,inside.room_profile->region.id,access,true};
  checked.mount_contact_verified=stowed||!geometry.mounting_type||InteriorContent::MountForSelectedInputUnderTransaction(db,fence,lease,source.stored.row,pose,input.value,LiveItems::MountSubject::NewSource());
  if(!stowed&&!InteriorContent::CheckPlacement(input.value.room,checked,metadata,geometry,pose,PlacementPolicy::Operation::Add))throw Failure{Error::Conflict};
  result.room=LiveItems::InteriorReservation{reservation,inside.house->house_id,inside.house->revision,inside.profile_revision,inside.context.source,identity.value.digest};
 }else{
  if(source.room){auto input=InteriorContent::ItemInputForStoredUnderTransaction(db,*views.room->room_profile,source.stored);if(!input)throw Failure{input.error};metadata=input.value.item;geometry=input.value.geometry;}
  else{const Scene::Item* found=nullptr;for(const auto* rows:{&views.yard.items,&views.yard.crate_contents})for(const auto& i:*rows)if(i.stored.custody_id==source.stored.custody_id)found=&i;
   if(!found||!found->metadata||!found->geometry)throw Failure{Error::UnsupportedItem};metadata=*found->metadata;geometry=*found->geometry;}
  PlacementPolicy::Context checked{target,definition.type,views.yard.region.id,access,true};
  checked.mount_contact_verified=stowed||!geometry.mounting_type||fence.MountSurfaceWithSubjectUnderTransaction(db,lease,source.stored.row,pose,target,LiveItems::MountSubject::NewSource());
  if(!stowed&&!PlacementPolicy::CheckAny(checked,metadata,geometry,views.yard.region,views.yard.additional_regions,pose,PlacementPolicy::Operation::Add))throw Failure{Error::Conflict};
  auto catalog=Query(db,"SELECT revision,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key='"+db.Escape(reservation.catalog_key)+"' FOR UPDATE");if(catalog.RowCount()!=1||!catalog.begin()[0]||std::stoull(catalog.begin()[0])!=reservation.catalog_revision)throw Failure{Error::Conflict};
  auto unhex=[](const char* raw){if(!raw)throw Failure{Error::DatabaseFailure};std::string s=raw;CustodyCodec::Bytes out;if(s.size()%2||s.size()>2*CustodyCodec::MaximumPayload)throw Failure{Error::DatabaseFailure};auto n=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw Failure{Error::DatabaseFailure};};for(size_t i=0;i<s.size();i+=2)out.push_back((n(s[i])<<4)|n(s[i+1]));return out;};
  if(ContentBinding::ColumnsInstalled(db))reservation.content_binding=ContentBinding::Current(db,reservation.catalog_key,Catalog::DecodePayloads(unhex(catalog.begin()[1]),unhex(catalog.begin()[2])),target,views.yard.route,source.stored.row.values.item_id);
 }
 return result;
}
inline Split SplitOne(Database& db,const LiveItems::Lease& lease,LiveItems::InventorySessionFence& fence,const Views& views,State& state,const Item& source,Identity original,const std::string& parent,size_t ordinal,uint32_t target,const PlacementPolicy::Transform& pose,uint32_t amount=1,bool stowed=false,const CrateBinding::Allocation* contained=nullptr){
 const auto available=Quantity(db,source.stored.row),maximum=StackMaximum(db,source.stored);
 if(available<=1||available>maximum||!amount||amount>=available||source.crate||source.house||source.stored.placement->npc_frame)throw Failure{Error::Conflict};
 auto destination=ReviewSplitDestination(db,lease,fence,views,source,target,pose,stowed,contained);
 auto witness=state.witnesses.find(original);if(witness==state.witnesses.end()||witness->second.custody_id!=source.stored.custody_id||!witness->second.instance)throw Failure{Error::UnheldFence};
 auto& instance=*witness->second.instance;const auto* data=instance.GetItem();
 if(!InventoryProjection::Matches(witness->second.row,instance)||!data||!data->Stackable||!NpcItemMetadata::Ordinary(*data)||data->StackSize!=maximum||data->HousingDefinitionID||data->HousingPlacementType||instance.IsScaling()||instance.GetHousingHouseID()||instance.GetHousingCrateID()||instance.GetHousingEstateID()!=UINT32_MAX)throw Failure{Error::UnsupportedItem};
 const auto serial_text=std::to_string(instance.GetSerialNumber());if(witness->second.guid!=std::string(16-serial_text.size(),'0')+serial_text)throw Failure{Error::Conflict};
 for(const auto& entry:state.witnesses){const auto& w=entry.second;if(w.instance)EQ::ItemInstance::AddGUIDToMap(w.instance->GetSerialNumber());if(!(w.row.null_fields&LiveItems::NullGuid)&&w.row.values.guid)EQ::ItemInstance::AddGUIDToMap(w.row.values.guid);}
 std::unique_ptr<EQ::ItemInstance> unit(instance.CloneWithNewSerial());if(!unit||unit->GetSerialNumber()<=0||unit->GetSerialNumber()==instance.GetSerialNumber())throw Failure{Error::Conflict};unit->SetCharges(amount);
 auto row=source.stored.row;row.values.charges=amount;row.values.guid=uint32_t(unit->GetSerialNumber());row.null_fields&=~uint8_t(LiveItems::NullCharges|LiveItems::NullGuid);
 if(!InventoryProjection::Matches(row,*unit))throw Failure{Error::UnsupportedItem};
 const auto custody=std::to_string(source.stored.custody_id),native=std::to_string(source.stored.placement->reservation.native_item_id),key=Key(parent,ordinal,"split");
 if(Query(db,"SELECT native_item_id FROM housing_live_placement_reservations WHERE request_key='"+key+"' FOR UPDATE").RowCount())throw Failure{Error::Conflict};
 auto remainder=WriteCount(db,source,available-amount,source.member);if(!remainder)throw Failure{Error::Conflict};
 auto one=[&](const std::string& sql){auto r=Query(db,sql);if(r.RowsAffected()!=1)throw Failure{Error::Conflict};return r;};
 const auto& r=destination.reservation;
 auto inserted=one("INSERT INTO housing_live_item_custody(estate_id,owner_character_id,schema_descriptor,row_blob,catalog_blob) SELECT "+std::to_string(r.estate_id)+",owner_character_id,schema_descriptor,"+Blob(row)+",catalog_blob FROM housing_live_item_custody WHERE custody_id="+custody+" AND row_blob="+Blob(remainder->row));if(!inserted.LastInsertedID())throw Failure{Error::Conflict};const auto new_custody=inserted.LastInsertedID();
 auto hex=[](const auto& bytes){const char* digits="0123456789abcdef";std::string out;for(auto b:bytes){out+=digits[uint8_t(b)>>4];out+=digits[uint8_t(b)&15];}return out;};
 const bool binding=ContentBinding::ColumnsInstalled(db);const auto binding_values=r.content_binding?std::to_string(r.content_binding->version)+",x'"+hex(r.content_binding->digest)+"'":"NULL,NULL";
 auto allocated=one("INSERT INTO housing_live_placement_reservations(request_key,character_id,account_id,lease_epoch,session_key,estate_id,native_estate_id,catalog_key,catalog_revision,source_row,runtime_serial,state,custody_id"+std::string(binding?",content_binding_version,content_binding_digest":"")+") SELECT '"+key+"',character_id,account_id,lease_epoch,session_key,"+std::to_string(r.estate_id)+","+std::to_string(target)+",'"+db.Escape(r.catalog_key)+"',"+std::to_string(r.catalog_revision)+","+Blob(row)+","+std::to_string(unit->GetSerialNumber())+",1,"+std::to_string(new_custody)+(binding?","+binding_values:"")+" FROM housing_live_placement_reservations WHERE native_item_id="+native+" AND custody_id="+custody+" AND state=1 AND source_row="+Blob(remainder->row));
 if(!allocated.LastInsertedID()||allocated.LastInsertedID()>=UINT32_MAX)throw Failure{Error::Conflict};const uint32_t new_native=allocated.LastInsertedID();
 if(destination.room){const auto& room=*destination.room;one("INSERT INTO housing_interior_placement_reservations(native_item_id,house_id,house_revision,profile_revision,source_zone,source_version,source_instance,content_version,content_digest) VALUES("+std::to_string(new_native)+","+std::to_string(room.house_id)+","+std::to_string(room.house_revision)+","+std::to_string(room.profile_revision)+","+std::to_string(room.source.zone_id)+","+std::to_string(room.source.zone_version)+","+std::to_string(room.source.instance_id)+",3,x'"+hex(room.content_digest)+"')");one("INSERT INTO housing_house_item_scopes(custody_id,house_id,revision) VALUES("+std::to_string(new_custody)+","+std::to_string(room.house_id)+",1)");}
 CustodyCodec::Bytes pose_bytes;for(float v:{pose.heading,pose.pitch,pose.roll,pose.scale,pose.position[0],pose.position[1],pose.position[2]}){uint32_t bits;std::memcpy(&bits,&v,4);CustodyCodec::W(pose_bytes,bits,4);}
 one("INSERT INTO housing_live_placed_items(custody_id,native_item_id,pose_revision,pose_blob) VALUES("+std::to_string(new_custody)+","+std::to_string(new_native)+",1,x'"+hex(pose_bytes)+"')");
 one("INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+std::to_string(new_custody)+",1,"+std::to_string(stowed?1:0)+","+std::to_string(target)+")");
 Item piece=source;piece.member.reset();piece.stored.custody_id=new_custody;piece.stored.estate_id=r.estate_id;piece.stored.row=row;piece.stored.placement=LiveItems::Placed{r,pose,1};piece.stored.placement->reservation.native_item_id=new_native;piece.stored.current_catalog_revision=r.catalog_revision;piece.stored.storage_revision=1;piece.stored.stowed=stowed;piece.room.reset();if(destination.room){const auto& room=*destination.room;piece.room=LiveItems::InteriorMoveContext{room.house_id,room.house_revision,room.profile_revision,1,room.source};}
 auto* pointer=unit.get();if(!state.split_units.emplace(new_custody,std::move(unit)).second)throw Failure{Error::Conflict};state.projected_rows[source.stored.custody_id]=remainder->row;
 const Identity identity{target,new_native};const auto text=std::to_string(pointer->GetSerialNumber());
 if(!state.witnesses.emplace(identity,Witness{new_custody,std::string(16-text.size(),'0')+text,row,pointer}).second||!state.aliases.emplace(identity,new_custody).second)throw Failure{Error::Conflict};state.locations[new_custody]=target;
 return {std::move(piece),CountEffect{source.stored,remainder,source.room}};
}

}
// Transaction-scoped property members only; aggregate journal lookup precedes
// this executor. Caller rolls back the ENTIRE parent and discards State on any
// failure. Does not start/end a TX, set pending, alter inventory or send packets.
inline LiveItems::Result<Effect> ExecuteUnderTransaction(Database& db,bool enabled,const std::string& parent_key,size_t ordinal,const LiveItems::Lease& lease,const Context& context,State& state,const LayoutWire::Operation& operation,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* houses=nullptr,const InteriorContent::ProfilePolicy* rooms=nullptr,CrateBinding::CustodyContext* crates=nullptr){
 using Error=LiveItems::Error;if(!enabled)return {{},Error::Disabled};if(parent_key.empty()||parent_key.size()>96||ordinal>=UINT16_MAX)return {{},Error::InvalidArgument};
 auto authenticated=LiveItems::Service::AuthenticateUnderTransaction(db,true,lease,fence);if(!authenticated)return {{},authenticated.error};if(!authenticated.value.pending_key.empty())return {{},Error::PendingApplication};
 try{
  const auto* pose=std::get_if<LayoutWire::PlacementOperation>(&operation);const auto* storage=std::get_if<LayoutWire::ItemOperation>(&operation);Identity source;uint32_t target=0,quantity=0;
  if(pose){if(pose->source_kind!=0xff||pose->source_slots!=std::array<uint16_t,3>{0xffff,0xffff,0xffff})return {{},Error::InvalidArgument};source={pose->source_estate_id,pose->native_item_id};target=pose->target_estate_id;}else{source={storage->source_estate_id,storage->native_item_id};target=storage->target_estate_id;quantity=storage->quantity;}
  const bool crate_target=target!=context.yard_id&&target!=context.room_id;if(!target||target==UINT32_MAX||(crate_target&&(pose||!quantity)))return {{},Error::UnsupportedItem};auto source_alias=state.aliases.find(source);bool resolved_room=false;if(source_alias!=state.aliases.end()){auto location=state.locations.find(source_alias->second);resolved_room=location!=state.locations.end()&&location->second==context.room_id;}const auto witnessed_source=state.witnesses.find(source);const bool target_room_crate=crate_target&&std::any_of(state.witnesses.begin(),state.witnesses.end(),[&](const auto& w){return w.second.crate&&w.second.crate->binding.native_estate_id==target&&w.second.crate->binding.parent_native_estate_id==context.room_id;});const bool need_room=target_room_crate||context.inside||source.first==context.room_id||target==context.room_id||resolved_room||(witnessed_source!=state.witnesses.end()&&witnessed_source->second.member&&witnessed_source->second.member->membership.origin_house_id);auto views=Detail::Read(db,lease,context,fence,houses,rooms,crates,need_room);auto item=Detail::Source(views,state,source,pose?std::optional<std::string>(pose->guid):std::nullopt);const auto available=Detail::Quantity(db,item.stored.row);
  if(quantity>available)return {{},Error::Conflict};
  if(crate_target){
   if(!crates||!quantity||quantity>=available||item.member||item.house||item.crate||item.stored.row.values.character_id!=lease.actor.character_id)return {{},Error::UnsupportedItem};
   std::optional<Item> container;auto select=[&](const auto& rows){for(const auto& row:rows)if(row.crate&&row.crate->binding.native_estate_id==target){if(container)throw Detail::Failure{Error::Conflict};container=Detail::Find(views,row.stored.custody_id);}};select(views.yard.items);if(views.room)select(views.room->contents);
   if(!container||!container->crate||container->stored.stowed||container->crate->packed||container->crate->created_by_character_id!=lease.actor.character_id||container->crate->content_manifest_sha256!=crates->ManifestDigest())return {{},Error::Conflict};
   const auto host=container->crate->binding.parent_native_estate_id;const auto& access=container->room?views.room->access.effective:views.yard.access;if(!access.Allows(AccessPolicy::Capability::MoveItem)||!access.Allows(AccessPolicy::Capability::AddItem)||!access.Allows(AccessPolicy::Capability::RemoveItem))return {{},Error::Unauthorized};
   // Split only the genuine requested piece; admit that new custody through
   // the existing service in this SAME parent transaction, never inventory.
   auto split=Detail::SplitOne(db,lease,fence,views,state,item,source,parent_key,ordinal,host,{0,0,0,1,{0,0,0}},quantity,true,container->crate.get());
   CrateStorage::Request r;r.request_key=Detail::Key(parent_key,ordinal,"split-admit");r.lease=lease;r.estate_id=context.estate_id;r.native_plot_id=host;r.catalog_revision=views.yard.current_catalog_revision;r.source=context.physical_source;r.crate_id=container->crate->binding.crate_id;r.crate_revision=container->crate->binding.revision;r.custody_id=split.piece.stored.custody_id;r.pose_revision=1;r.storage_revision=1;r.to_crate=true;
   if(container->room)r.room=CrateStorage::RoomContext{context.house_id,views.room->house->revision,views.room->profile_revision,!context.inside};else if(context.inside)r.parent_yard=Detail::ParentOrigin(views,context);
   auto admitted=CrateStorage::Service::TransferUnderTransaction(db,true,r,fence,*crates,houses,rooms);if(!admitted)throw Detail::Failure{admitted.error};
   auto fresh=Detail::Read(db,lease,context,fence,houses,rooms,crates,need_room);auto piece=Detail::Find(fresh,split.piece.stored.custody_id),after_container=Detail::Find(fresh,container->stored.custody_id);if(!piece||!piece->member||!after_container||!after_container->crate)throw Detail::Failure{Error::Conflict};
   auto expected=std::make_shared<CrateBinding::Allocation>(*container->crate);if(expected->binding.revision==UINT64_MAX)throw Detail::Failure{Error::Conflict};++expected->binding.revision;if(!LayoutCrate::Same(expected,after_container->crate)||!LayoutCrate::Same(piece->member->container,after_container->crate))throw Detail::Failure{Error::Conflict};
   Effect out;out.action=Action::SplitToCrate;out.before=item.stored;out.before_room=item.room;out.after=piece->stored;out.after_room=piece->room;out.created_member=piece->member;out.counts.push_back(std::move(split.source));out.container_change=CountEffect{container->stored,after_container->stored,container->room};state.projected_crates[expected->binding.crate_id]=after_container->crate;state.locations[piece->stored.custody_id]=target;
   const Identity id{target,piece->stored.placement->reservation.native_item_id};auto owner=state.split_units.find(piece->stored.custody_id);if(owner==state.split_units.end()||!owner->second)throw Detail::Failure{Error::Conflict};auto text=std::to_string(owner->second->GetSerialNumber());state.witnesses.emplace(id,Witness{piece->stored.custody_id,std::string(16-text.size(),'0')+text,piece->stored.row,owner->second.get(),{},{},piece->member});auto alias=state.aliases.emplace(id,piece->stored.custody_id);if(!alias.second&&alias.first->second!=piece->stored.custody_id)throw Detail::Failure{Error::Conflict};return {std::move(out),Error::None};
  }
  if(item.member){
   // Direct release into the requested destination; no temporary source slot.
   // Retained houses require their own typed token path. Member counts retain custody below.
   // Zero-count Tag0 releases the whole member into reviewed storage.
   if(item.house||item.crate)return {{},Error::UnsupportedItem};
   const auto proof=item.member;auto container=Detail::Find(views,proof->container->binding.custody_id);if(!container||!container->crate||!LayoutCrate::Same(container->crate,proof->container))return {{},Error::Conflict};
   auto checked=LayoutCrateMember::CaptureUnderTransaction(db,item.stored,container->stored);if(!checked||!LayoutCrateMember::Same(proof,checked.value))return {{},Error::Conflict};
   if(quantity||(pose&&available>1)){
    if(!crates||container->stored.stowed||container->crate->packed||container->crate->content_manifest_sha256!=crates->ManifestDigest())return {{},Error::Conflict};
    const auto& access=item.room?views.room->access.effective:views.yard.access;
    if(!access.Allows(AccessPolicy::Capability::MoveItem)||!access.Allows(AccessPolicy::Capability::AddItem)||!access.Allows(AccessPolicy::Capability::RemoveItem))return {{},Error::Unauthorized};
    if((item.stored.row.values.character_id!=lease.actor.character_id||item.stored.row.values.character_id!=container->crate->created_by_character_id)&&!CrateForeignMember::EligibleUnderTransaction(db,item.stored.row))return {{},Error::UnsupportedItem};
    Effect out;
    if(quantity){
     if(quantity==available||Detail::ConsolidationFits(db,views,state,item,quantity,target))out=Detail::Consolidate(db,views,state,item,quantity,target);
     else{auto piece=Detail::SplitOne(db,lease,fence,views,state,item,source,parent_key,ordinal,target,{0,0,0,1,{0,0,0}},quantity,true);out.action=Action::MemberSplitAndStore;out.before=item.stored;out.before_room=item.room;out.after=std::move(piece.piece.stored);out.after_room=piece.piece.room;out.counts.push_back(std::move(piece.source));}
    }
    else{auto piece=Detail::SplitOne(db,lease,fence,views,state,item,source,parent_key,ordinal,target,LayoutWire::PlacementTransform(*pose));out.action=Action::MemberSplitAndPlace;out.before=item.stored;out.before_room=item.room;out.after=std::move(piece.piece.stored);out.after_room=piece.piece.room;out.counts.push_back(std::move(piece.source));}
    const auto& b=container->crate->binding;if(b.revision==UINT64_MAX)throw Detail::Failure{Error::Conflict};
    if(Detail::Query(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+std::to_string(b.crate_id)+" AND packed=0 AND revision="+std::to_string(b.revision)).RowsAffected()!=1)throw Detail::Failure{Error::Conflict};
    auto updated=LayoutCrate::ReadUnderTransaction(db,container->stored);if(!updated)throw Detail::Failure{updated.error};auto expected=std::make_shared<CrateBinding::Allocation>(*container->crate);expected->binding.revision++;
    if(!LayoutCrate::Same(expected,updated.value))throw Detail::Failure{Error::Conflict};
    out.container_change=CountEffect{container->stored,container->stored,container->room};state.projected_crates[b.crate_id]=updated.value;
    return {std::move(out),Error::None};
   }
   const auto parent=item.stored.placement->reservation.native_estate_id;const auto source_route=item.room?(context.inside?views.room->context.source:*views.room->exterior_source):views.yard.route;
   LiveItems::CrateMemberContext member{proof->membership.crate_id,proof->container->binding.revision,parent,item.room?views.room->context.catalog_revision:views.yard.current_catalog_revision,source_route};
   if(!pose&&target==parent){
    if(!crates)return {{},Error::Unauthorized};
    CrateStorage::Request r;r.request_key=Detail::Key(parent_key,ordinal,"extract-store");r.lease=lease;r.estate_id=views.yard.estate_id;r.native_plot_id=parent;r.catalog_revision=member.catalog_revision;r.source=source_route;r.crate_id=member.crate_id;r.crate_revision=member.revision;r.custody_id=item.stored.custody_id;r.pose_revision=item.stored.placement->pose_revision;r.storage_revision=item.stored.storage_revision;r.to_crate=false;r.restore_original=false;
    if(item.room)r.room=CrateStorage::RoomContext{item.room->house_id,item.room->house_revision,item.room->profile_revision,!context.inside};else if(context.inside)r.parent_yard=Detail::ParentOrigin(views,context);
    auto changed=CrateStorage::Service::TransferUnderTransaction(db,true,r,fence,*crates,houses,rooms);if(!changed)throw Detail::Failure{changed.error};
   }else if(target==parent){InteriorStorage::Request r;r.request_key=Detail::Key(parent_key,ordinal,"extract");r.lease=lease;r.custody_id=item.stored.custody_id;r.native_item_id=item.stored.placement->reservation.native_item_id;r.expected_pose_revision=item.stored.placement->pose_revision;r.expected_storage_revision=item.stored.storage_revision;r.placement=LayoutWire::PlacementTransform(*pose);r.crate_member=member;
    if(item.room){r.room=*item.room;if(!context.inside)r.room_exterior_source=context.physical_source;}else{r.exterior=InteriorStorage::ExteriorContext{context.estate_id,views.yard.native_plot_id,views.yard.current_catalog_revision,views.yard.route};if(context.inside)r.parent_yard=Detail::ParentOrigin(views,context);}
    auto changed=InteriorStorage::Service::ChangeUnderTransaction(db,true,r,fence,houses,rooms,crates);if(!changed)throw Detail::Failure{changed.error};
   }else{if(!views.room||!houses||!rooms)throw Detail::Failure{Error::Unauthorized};InteriorStorage::TransferRequest r;r.request_key=Detail::Key(parent_key,ordinal,"extract-transfer");r.lease=lease;r.occupied_room={context.house_id,views.room->house->revision,views.room->profile_revision,item.room?item.room->scope_revision:0,views.room->context.source};r.custody_id=item.stored.custody_id;r.native_item_id=item.stored.placement->reservation.native_item_id;r.source_native_estate_id=parent;r.expected_pose_revision=item.stored.placement->pose_revision;r.expected_storage_revision=item.stored.storage_revision;r.to_room=target==context.room_id;if(pose)r.placement=LayoutWire::PlacementTransform(*pose);r.crate_member=member;if(!context.inside)r.yard=InteriorStorage::ExteriorContext{context.estate_id,views.yard.native_plot_id,views.yard.current_catalog_revision,context.physical_source};auto changed=InteriorStorage::Service::TransferUnderTransaction(db,true,r,fence,*houses,*rooms,crates);if(!changed)throw Detail::Failure{changed.error};}
   auto fresh=Detail::Read(db,lease,context,fence,houses,rooms,crates,need_room);auto after=Detail::Find(fresh,item.stored.custody_id),after_container=Detail::Find(fresh,container->stored.custody_id);if(!after||after->member||!after_container||!after_container->crate)throw Detail::Failure{Error::Conflict};
   auto expected=std::make_shared<CrateBinding::Allocation>(*container->crate);if(expected->binding.revision==UINT64_MAX)throw Detail::Failure{Error::Conflict};expected->binding.revision++;if(!LayoutCrate::Same(expected,after_container->crate))throw Detail::Failure{Error::Conflict};
   Effect out;out.action=pose?Action::ExtractAndPlace:Action::ExtractAndStore;out.before=item.stored;out.before_room=item.room;out.after=after->stored;out.after_room=after->room;out.container_change=CountEffect{container->stored,after_container->stored,container->room};
   state.projected_crates[expected->binding.crate_id]=after_container->crate;state.released_members.insert(item.stored.custody_id);state.locations[item.stored.custody_id]=target;
   const Identity actual{target,out.after.placement->reservation.native_item_id};auto alias=state.aliases.emplace(actual,item.stored.custody_id);if(!alias.second&&alias.first->second!=item.stored.custody_id)throw Detail::Failure{Error::Conflict};state.witnesses.emplace(actual,state.witnesses.at(source));return {std::move(out),Error::None};
  }

  // A house keeps its unique binding, interior route and furnished contents.
  // Stored restoration uses the existing storage service under this aggregate
  // transaction; it cannot adopt a token or allocate/relocate an interior.
  if(item.house){
   if(!RentState::MutableItem(db,item.stored.estate_id,views.yard.native_plot_id,item.stored.custody_id))return {{},Error::Unauthorized};
   if(!pose||item.room||available!=1||target!=views.yard.native_plot_id||target!=item.stored.placement->reservation.native_estate_id)return {{},Error::UnsupportedItem};
   if(!views.yard.access.Allows(AccessPolicy::Capability::MoveItem))return {{},Error::Unauthorized};
   auto actor=std::find_if(views.yard.items.begin(),views.yard.items.end(),[&](const auto& p){return p.stored.custody_id==item.stored.custody_id;});
   if(actor==views.yard.items.end()||!actor->metadata||!actor->geometry||!LayoutHouse::Same(actor->house,item.house)||!LayoutHouse::Shape(*item.house,item.stored))return {{},Error::Conflict};
   auto creator=Detail::Query(db,"SELECT pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(item.stored.row.values.character_id)+" FOR UPDATE");
   if(creator.RowCount()>1)return {{},Error::DatabaseFailure};if(creator.RowCount()&&creator.begin()[0])return {{},Error::PendingApplication};
   const auto transform=LayoutWire::PlacementTransform(*pose);const auto& before=*item.stored.placement;
   if(item.stored.stowed){
    InteriorStorage::Request restore;restore.request_key=Detail::Key(parent_key,ordinal,"house-restore");restore.lease=lease;
    restore.custody_id=item.stored.custody_id;restore.native_item_id=before.reservation.native_item_id;
    restore.expected_pose_revision=before.pose_revision;restore.expected_storage_revision=item.stored.storage_revision;restore.placement=transform;
    restore.exterior=InteriorStorage::ExteriorContext{context.estate_id,views.yard.native_plot_id,views.yard.current_catalog_revision,views.yard.route};
    if(context.inside)restore.parent_yard=Detail::ParentOrigin(views,context);
    auto changed=InteriorStorage::Service::ChangeUnderTransaction(db,true,restore,fence,houses,rooms,crates);if(!changed)return {{},changed.error};
    auto current=Detail::Read(db,lease,context,fence,houses,rooms,crates,need_room);auto after=Detail::Find(current,item.stored.custody_id);
    if(!after||after->stored.stowed||!LayoutHouse::Same(after->house,item.house))return {{},Error::Conflict};
    Effect out;out.action=Action::Restore;out.before=item.stored;out.after=after->stored;state.locations[item.stored.custody_id]=target;return {std::move(out),Error::None};
   }
   PlacementPolicy::Context checked{target,views.yard.definition.type,views.yard.region.id,views.yard.access,true};
   if(!PlacementPolicy::CheckAny(checked,*actor->metadata,*actor->geometry,views.yard.region,views.yard.additional_regions,transform,PlacementPolicy::Operation::Move)||before.pose_revision==UINT64_MAX)return {{},Error::Conflict};
   CustodyCodec::Bytes bytes;for(float v:{transform.heading,transform.pitch,transform.roll,transform.scale,transform.position[0],transform.position[1],transform.position[2]}){uint32_t n;std::memcpy(&n,&v,4);CustodyCodec::W(bytes,n,4);}const char* hex="0123456789abcdef";std::string blob="x'";for(auto v:bytes){blob+=hex[v>>4];blob+=hex[v&15];}blob+="'";
   auto changed=Detail::Query(db,"UPDATE housing_live_placed_items SET pose_revision=pose_revision+1,pose_blob="+blob+" WHERE custody_id="+std::to_string(item.stored.custody_id)+" AND native_item_id="+std::to_string(before.reservation.native_item_id)+" AND pose_revision="+std::to_string(before.pose_revision));if(changed.RowsAffected()!=1)return {{},Error::Conflict};
   Effect out;out.action=Action::Move;out.before=item.stored;out.after=item.stored;out.after.placement->transform=transform;out.after.placement->pose_revision++;
   auto fresh=LayoutHouse::ReadUnderTransaction(db,out.after);if(!fresh||!LayoutHouse::Same(fresh.value,item.house))return {{},Error::Conflict};state.locations[item.stored.custody_id]=target;return {std::move(out),Error::None};
  }
  // A crate is one bound token, never an ordinary stack or newly cloned unit.
  // Actual pose movement reuses the reviewed Move service; identity, members
  // and allocation stay fixed. Packing/storage/reparenting need their typed
  // crate transitions and are not silently treated as ordinary item changes.
  if(item.crate&&(!pose||item.stored.stowed||available!=1||target!=item.stored.placement->reservation.native_estate_id))return {{},Error::UnsupportedItem};
  // Positive Tag0 is an actual count, never a destination enum.
  // Count effects need the parent mixed journal/retained-memory application.
  if(quantity){
   if(quantity==available||Detail::ConsolidationFits(db,views,state,item,quantity,target))return {Detail::Consolidate(db,views,state,item,quantity,target),Error::None};
   auto piece=Detail::SplitOne(db,lease,fence,views,state,item,source,parent_key,ordinal,target,{0,0,0,1,{0,0,0}},quantity,true);
   Effect effect;effect.action=Action::SplitAndStore;effect.before=item.stored;effect.before_room=item.room;effect.after=std::move(piece.piece.stored);effect.after_room=piece.piece.room;effect.counts.push_back(std::move(piece.source));return {std::move(effect),Error::None};
  }
  Effect out;out.before=item.stored;out.before_room=item.room;auto current=item;
  const bool split=pose&&available>1;
  if(split){auto prepared=Detail::SplitOne(db,lease,fence,views,state,item,source,parent_key,ordinal,target,LayoutWire::PlacementTransform(*pose));current=std::move(prepared.piece);out.counts.push_back(std::move(prepared.source));}
  auto change=[&](const Item& source_item,const std::optional<PlacementPolicy::Transform>& placement,const std::string& step){InteriorStorage::Request r;r.request_key=Detail::Key(parent_key,ordinal,step);r.lease=lease;r.custody_id=source_item.stored.custody_id;r.native_item_id=source_item.stored.placement->reservation.native_item_id;r.expected_pose_revision=source_item.stored.placement->pose_revision;r.expected_storage_revision=source_item.stored.storage_revision;r.placement=placement;
   if(source_item.room){r.room=*source_item.room;if(!context.inside)r.room_exterior_source=context.physical_source;}else{r.exterior=InteriorStorage::ExteriorContext{context.estate_id,views.yard.native_plot_id,views.yard.current_catalog_revision,views.yard.route};if(context.inside)r.parent_yard=Detail::ParentOrigin(views,context);}auto changed=InteriorStorage::Service::ChangeUnderTransaction(db,true,r,fence,houses,rooms,crates);if(!changed)throw Detail::Failure{changed.error};};
  const auto before_native=item.stored.placement->reservation.native_estate_id;
  if(!split&&target!=before_native){if(!views.room||!houses||!rooms)throw Detail::Failure{Error::Unauthorized};InteriorStorage::TransferRequest r;r.request_key=Detail::Key(parent_key,ordinal,"transfer");r.lease=lease;r.occupied_room={context.house_id,views.room->house->revision,views.room->profile_revision,current.room?current.room->scope_revision:0,views.room->context.source};r.custody_id=current.stored.custody_id;r.native_item_id=current.stored.placement->reservation.native_item_id;r.source_native_estate_id=before_native;r.expected_pose_revision=current.stored.placement->pose_revision;r.expected_storage_revision=current.stored.storage_revision;r.to_room=target==context.room_id;if(pose)r.placement=LayoutWire::PlacementTransform(*pose);if(!context.inside)r.yard=InteriorStorage::ExteriorContext{context.estate_id,views.yard.native_plot_id,views.yard.current_catalog_revision,context.physical_source};auto transferred=InteriorStorage::Service::TransferUnderTransaction(db,true,r,fence,*houses,*rooms,crates);if(!transferred)throw Detail::Failure{transferred.error};out.action=pose?Action::TransferAndPlace:Action::Transfer;
   views=Detail::Read(db,lease,context,fence,houses,rooms,crates,need_room);auto updated=Detail::Find(views,current.stored.custody_id);if(!updated)throw Detail::Failure{Error::Conflict};current=*updated;
  }
  if(pose&&!split&&target==before_native){const auto transform=LayoutWire::PlacementTransform(*pose);if(current.stored.stowed){change(current,transform,"restore");if(target==before_native)out.action=Action::Restore;}else{LiveItems::MoveRequest r;r.request_key=Detail::Key(parent_key,ordinal,"move");r.lease=lease;r.estate_id=current.stored.estate_id;r.custody_id=current.stored.custody_id;r.native_item_id=current.stored.placement->reservation.native_item_id;r.expected_pose_revision=current.stored.placement->pose_revision;r.expected_catalog_revision=current.stored.current_catalog_revision;r.transform=transform;r.room=current.room;if(!r.room&&context.inside)r.parent_yard=Detail::ParentOrigin(views,context);if(r.room&&!context.inside)r.room_exterior_source=context.physical_source;auto moved=LiveItems::Service::MoveUnderTransaction(db,true,r,fence,nullptr,crates,houses,rooms);if(!moved)throw Detail::Failure{moved.error};out.action=Action::Move;}}
  else if(!pose&&!current.stored.stowed){change(current,{},"stow");if(target==before_native)out.action=Action::Stow;}
  auto after=Detail::Read(db,lease,context,fence,houses,rooms,crates,need_room);auto updated=Detail::Find(after,current.stored.custody_id);if(!updated)throw Detail::Failure{Error::Conflict};out.after=updated->stored;out.after_room=updated->room;
  if((!split&&CustodyCodec::Encode(out.before.row)!=CustodyCodec::Encode(out.after.row))||out.after.placement->reservation.native_estate_id!=target)throw Detail::Failure{Error::Conflict};
  if(split){
   out.action=Action::SplitAndPlace;state.locations[out.after.custody_id]=target;
   const Identity actual{target,out.after.placement->reservation.native_item_id};
   auto unit=state.split_units.find(out.after.custody_id);if(unit==state.split_units.end())throw Detail::Failure{Error::Conflict};
   const auto text=std::to_string(unit->second->GetSerialNumber());
   auto bound=state.aliases.emplace(actual,out.after.custody_id);if(!bound.second&&bound.first->second!=out.after.custody_id)throw Detail::Failure{Error::Conflict};
   state.witnesses.emplace(actual,Witness{out.after.custody_id,std::string(16-text.size(),'0')+text,out.after.row,unit->second.get()});
   return {std::move(out),Error::None};
  }
  // Earlier members can allocate a replacement native ID for the same custody.
  // Keep the original source binding and exact new identity transaction-local.
  state.locations[out.after.custody_id]=target;const Identity new_identity{target,out.after.placement->reservation.native_item_id};auto alias=state.aliases.emplace(new_identity,out.after.custody_id);if(!alias.second&&alias.first->second!=out.after.custody_id)throw Detail::Failure{Error::Conflict};
  if(auto original=state.witnesses.find(source);original!=state.witnesses.end()){
   state.witnesses.emplace(new_identity,original->second);
   const Identity projected{target,source.second};auto bound=state.aliases.emplace(projected,out.after.custody_id);if(!bound.second&&bound.first->second!=out.after.custody_id)throw Detail::Failure{Error::Conflict};state.witnesses.emplace(projected,original->second);
  }
  return {std::move(out),Error::None};
 }catch(const Detail::Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}
}
#endif
