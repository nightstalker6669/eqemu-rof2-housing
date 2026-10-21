#include "npc_native_frame_db.h"
#include "occupied_parent_yard.h"
#include "rent_state_db.h"
#include "upkeep_cycle_db.h"
#include "interior_storage_service.h"
#include "crate_binding.h"
#include "crate_member_scope_db.h"
#include "house_binding_service.h"
#include "house_shed_storage.h"
#include "interior_item_content_db.h"
#include "npc_runtime_db.h"
#include "custody_codec.h"
#include "catalog_content_db.h"
#include <limits>
namespace Housing::InteriorStorage {
namespace {
using Error=LiveItems::Error;struct Failure{Error error;};
std::string Hex(const std::vector<uint8_t>& b){std::string out;const char* h="0123456789abcdef";for(auto v:b){out+=h[v>>4];out+=h[v&15];}return out;}
std::vector<uint8_t> Binding(const Request& r){
 std::vector<uint8_t>b;auto n=[&](uint64_t v,size_t bytes){for(size_t i=0;i<bytes;++i)b.push_back(uint8_t(v>>(8*i)));};auto s=[&](const std::string& v){n(v.size(),4);b.insert(b.end(),v.begin(),v.end());};
 n(1,4);s(r.request_key);n(r.lease.actor.character_id,4);n(r.lease.actor.account_id,4);n(r.lease.epoch,8);s(r.lease.session_key);b.insert(b.end(),r.lease.token.begin(),r.lease.token.end());
 for(auto v:{r.room.house_id,r.room.house_revision,r.room.profile_revision,r.room.scope_revision,r.custody_id,r.expected_pose_revision,r.expected_storage_revision})n(v,8);
 n(r.room.source.zone_id,4);n(r.room.source.zone_version,1);n(r.room.source.instance_id,2);n(r.native_item_id,4);n(bool(r.placement),1);if(r.placement){for(float v:{r.placement->heading,r.placement->pitch,r.placement->roll,r.placement->scale,r.placement->position[0],r.placement->position[1],r.placement->position[2]}){uint32_t bits;std::memcpy(&bits,&v,4);n(bits,4);}}if(r.exterior){n(2,4);n(r.exterior->estate_id,4);n(r.exterior->native_estate_id,4);n(r.exterior->catalog_revision,8);n(r.exterior->source.zone_id,4);n(r.exterior->source.zone_version,1);n(r.exterior->source.instance_id,2);}if(r.crate_member){n(3,4);n(r.crate_member->crate_id,8);n(r.crate_member->revision,8);n(r.crate_member->native_plot_id,4);n(r.crate_member->catalog_revision,8);n(r.crate_member->source.zone_id,4);n(r.crate_member->source.zone_version,1);n(r.crate_member->source.instance_id,2);}if(r.room_exterior_source){n(4,4);n(r.room_exterior_source->zone_id,4);n(r.room_exterior_source->zone_version,1);n(r.room_exterior_source->instance_id,2);}if(r.parent_yard){n(5,4);auto parent=OccupiedParentYard::Encode(*r.parent_yard);n(parent.size(),4);b.insert(b.end(),parent.begin(),parent.end());}if(r.npc_frame){n(6,4);n(r.npc_frame->height_bits,4);n(r.npc_frame->origin_z_bits,4);}return b;
}
}
namespace {
// Caller owns transaction and inventory fence. A bulk failure rolls back every
// marker and receipt, including already processed members.
struct ReadCache {std::optional<Scene::InteriorSnapshot> room;std::optional<Scene::Snapshot> yard;};
Receipt ChangeChecked(Database& db,const Request& r,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* profile,const std::vector<uint8_t>& batch_binding={},ReadCache* shared_cache=nullptr,const CrateBinding::CustodyContext* crates=nullptr){
 auto q=[&](const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())throw Failure{Error::DatabaseFailure};return result;};
  if(!ItemStorage::Installed(db))throw Failure{Error::Disabled};if(r.npc_frame&&!r.placement)throw Failure{Error::InvalidArgument};
  ReadCache local_cache;auto& cache=shared_cache?*shared_cache:local_cache;auto& room_scene=cache.room;auto& yard_scene=cache.yard;uint32_t native=0;AccessPolicy::EffectiveAccess access;
  if(r.exterior){
   if(r.room.house_id||!r.exterior->estate_id||!r.exterior->native_estate_id||!r.exterior->catalog_revision)throw Failure{Error::InvalidArgument};
   if(r.parent_yard){auto read=OccupiedParentYard::ReadUnderTransaction(db,r.lease,*r.parent_yard,fence,house,profile,crates);if(!read)throw Failure{read.error};yard_scene=std::move(read.value);}
   else if(!yard_scene){auto read=Scene::Service::ReadSceneUnderTransaction(db,r.lease,r.exterior->estate_id,fence,house,crates);if(!read)throw Failure{read.error};yard_scene=std::move(read.value);}
   const auto& e=*r.exterior;if(yard_scene->estate_id!=e.estate_id||yard_scene->native_plot_id!=e.native_estate_id||yard_scene->current_catalog_revision!=e.catalog_revision||yard_scene->route.zone_id!=e.source.zone_id||yard_scene->route.zone_version!=e.source.zone_version||yard_scene->route.instance_id!=e.source.instance_id)throw Failure{Error::Conflict};
   native=yard_scene->native_plot_id;access=yard_scene->access;
  }else{
   if(!house||!profile)throw Failure{Error::Unauthorized};
   if(!room_scene){auto read=r.room_exterior_source?Scene::Service::ReadCurrentAttachedInteriorUnderTransaction(db,r.lease,r.room.house_id,fence,*house,profile,crates):Scene::Service::ReadInteriorSceneUnderTransaction(db,r.lease,r.room.source,fence,*house,*profile,crates);if(!read)throw Failure{read.error};room_scene=std::move(read.value);}
   if(room_scene->context.source.zone_id!=r.room.source.zone_id||room_scene->context.source.zone_version!=r.room.source.zone_version||room_scene->context.source.instance_id!=r.room.source.instance_id||(r.room_exterior_source&&(!room_scene->exterior_source||room_scene->exterior_source->zone_id!=r.room_exterior_source->zone_id||room_scene->exterior_source->zone_version!=r.room_exterior_source->zone_version||room_scene->exterior_source->instance_id!=r.room_exterior_source->instance_id)))throw Failure{Error::Conflict};
   if(!room_scene->house||room_scene->house->house_id!=r.room.house_id||room_scene->house->revision!=r.room.house_revision||room_scene->profile_revision!=r.room.profile_revision)throw Failure{Error::Conflict};
   native=room_scene->house->interior->native_estate_id;access=room_scene->access.effective;
  }
  auto binding=Binding(r);binding.insert(binding.end(),batch_binding.begin(),batch_binding.end());const auto key="x'"+Hex(std::vector<uint8_t>(r.request_key.begin(),r.request_key.end()))+"'";
  auto previous=q("SELECT character_id,account_id,HEX(binding_blob),custody_id,before_revision,after_revision FROM housing_live_storage_receipts WHERE request_key="+key+" FOR UPDATE");
  Receipt out;
  if(previous.RowCount()){
   if(previous.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto p=previous.begin();for(size_t i=0;i<6;++i)if(!p[i])throw Failure{Error::DatabaseFailure};
   auto hex=Hex(binding);std::string saved=p[2];for(auto& c:saved)if(c>='A'&&c<='F')c+=32;
   if(std::stoul(p[0])!=r.lease.actor.character_id||std::stoul(p[1])!=r.lease.actor.account_id||saved!=hex)throw Failure{Error::Conflict};
   out={std::stoull(p[3]),std::stoull(p[4]),std::stoull(p[5]),true};if(out.custody_id!=r.custody_id||out.before_revision!=r.expected_storage_revision||out.after_revision!=out.before_revision+1)throw Failure{Error::DatabaseFailure};
  }else{
   if(!access.Allows(AccessPolicy::Capability::MoveItem))throw Failure{Error::Unauthorized};
   const LiveItems::Stored* stored=nullptr;std::optional<PlacementPolicy::ItemMetadata> metadata;std::optional<PlacementPolicy::Geometry> geometry;RegionWire::Region region;uint32_t estate_type=0;std::optional<InteriorContent::Profile> selected_room;
   if(yard_scene){
    const auto& members=r.crate_member?yard_scene->crate_contents:yard_scene->items;
    auto item=std::find_if(members.begin(),members.end(),[&](const auto& i){return i.stored.custody_id==r.custody_id;});
    if(item==members.end()||item->crate||!item->metadata||!item->geometry)throw Failure{Error::UnsupportedItem};
    if(item->house&&!r.placement&&!r.crate_member){
     // Direct building Shed storage is an exterior creator operation. The
     // permanent room and every scope remain attached to the stable HouseID.
     const auto& h=*item->house;const auto& s=item->stored;
     if(r.parent_yard||!house||!profile||!h.interior||!h.pending_key.empty()||
        h.location.kind!=HouseBinding::Location::Kind::Custody||h.location.custody_id!=s.custody_id||
        h.item_template_id!=s.row.values.item_id||s.row.values.character_id!=r.lease.actor.character_id)
      throw Failure{Error::Unauthorized};
     auto attached=Scene::Service::ReadAttachedInteriorUnderTransaction(db,r.lease,yard_scene->estate_id,h.house_id,fence,*house,profile,true,crates);
     if(!attached)throw Failure{attached.error};
     if(!attached.value.house||attached.value.house->house_id!=h.house_id||attached.value.house->revision!=h.revision)
      throw Failure{Error::Conflict};
     auto quiet=HouseShedStorage::QuiescentUnderTransaction(db,attached.value);if(quiet!=Error::None)throw Failure{quiet};
    }
    if(item->house&&r.placement){
     // Scene authentication proves the retained house/route under this lease.
     // A stored house restores that identity; it does not allocate another room.
     const auto& h=*item->house;const auto& s=item->stored;
     if(!house||!s.placement||!h.interior||!h.house_id||!h.revision||!h.pending_key.empty()||
        h.location.kind!=HouseBinding::Location::Kind::Custody||h.location.custody_id!=s.custody_id||
        h.item_template_id!=s.row.values.item_id||s.row.values.character_id!=r.lease.actor.character_id||
        s.estate_id!=yard_scene->estate_id||h.interior->catalog_key!=s.placement->reservation.catalog_key||
        h.interior->catalog_revision!=yard_scene->current_catalog_revision||!yard_scene->definition.max_children||
        std::find(yard_scene->definition.children.begin(),yard_scene->definition.children.end(),h.interior->definition_id)==yard_scene->definition.children.end())throw Failure{Error::Conflict};
     // Count locked persisted siblings, not a possibly shortened projected list.
     auto occupied=q("SELECT l.house_id FROM housing_house_locations l JOIN housing_live_placed_items p ON p.custody_id=l.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id AND r.custody_id=p.custody_id AND r.state=1 LEFT JOIN housing_live_item_storage s ON s.custody_id=p.custody_id WHERE l.location_kind=2 AND r.native_estate_id="+std::to_string(native)+" AND l.house_id<>"+std::to_string(h.house_id)+" AND (s.stowed IS NULL OR s.stowed=0) AND NOT EXISTS (SELECT 1 FROM housing_crate_members m WHERE m.custody_id=p.custody_id) ORDER BY l.house_id FOR UPDATE");
     // Restoring a stored house must not use the shared house/crate child budget.
     if(occupied.RowCount()>=1)throw Failure{Error::Conflict};
    }
    if(r.crate_member){const auto& m=*r.crate_member;
     if(!yard_scene->definition.max_items||yard_scene->items.size()>=yard_scene->definition.max_items)throw Failure{Error::Conflict};
     if(!crates||!r.placement||m.native_plot_id!=native||m.catalog_revision!=yard_scene->current_catalog_revision||m.source.zone_id!=yard_scene->route.zone_id||m.source.zone_version!=yard_scene->route.zone_version||m.source.instance_id!=yard_scene->route.instance_id||item->contained_in_crate!=m.crate_id||!access.Allows(AccessPolicy::Capability::AddItem)||!access.Allows(AccessPolicy::Capability::RemoveItem))throw Failure{Error::Unauthorized};
     auto container=std::find_if(yard_scene->items.begin(),yard_scene->items.end(),[&](const auto& i){return i.crate&&i.crate->binding.crate_id==m.crate_id;});
     if(container==yard_scene->items.end()||container->stored.stowed||container->crate->packed||container->crate->binding.revision!=m.revision)throw Failure{Error::Conflict};
    }
    if(item->house&&(!house||!access.Allows(AccessPolicy::Capability::AddHouse)||!access.Allows(AccessPolicy::Capability::RemoveHouse)))throw Failure{Error::Unauthorized};
    stored=&item->stored;metadata=item->metadata;geometry=item->geometry;region=yard_scene->region;estate_type=yard_scene->definition.type;
   }else{
    const auto& members=r.crate_member?room_scene->crate_contents:room_scene->contents;
    auto item=std::find_if(members.begin(),members.end(),[&](const auto& i){return i.stored.custody_id==r.custody_id;});
    if(item==members.end()||!item->room||item->scope_revision!=r.room.scope_revision)throw Failure{Error::Conflict};
    if(r.crate_member){const auto& m=*r.crate_member;
     if(!crates||!r.placement||item->crate||m.native_plot_id!=native||m.catalog_revision!=room_scene->context.catalog_revision||m.source.zone_id!=r.room_exterior_source.value_or(r.room.source).zone_id||m.source.zone_version!=r.room_exterior_source.value_or(r.room.source).zone_version||m.source.instance_id!=r.room_exterior_source.value_or(r.room.source).instance_id||item->contained_in_crate!=m.crate_id||!access.Allows(AccessPolicy::Capability::AddItem)||!access.Allows(AccessPolicy::Capability::RemoveItem))throw Failure{Error::Unauthorized};
     if(!room_scene->room_profile||!room_scene->room_profile->definition.max_items||room_scene->contents.size()>=room_scene->room_profile->definition.max_items)throw Failure{Error::Conflict};
     auto container=std::find_if(room_scene->contents.begin(),room_scene->contents.end(),[&](const auto& i){return i.crate&&i.crate->binding.crate_id==m.crate_id;});
     if(container==room_scene->contents.end()||container->stored.stowed||container->crate->packed||container->crate->binding.revision!=m.revision)throw Failure{Error::Conflict};
    }
    stored=&item->stored;
    if(r.placement){auto input=InteriorContent::ItemInputForStoredUnderTransaction(db,*room_scene->room_profile,*stored);if(!input)throw Failure{input.error};metadata=input.value.item;geometry=input.value.geometry;selected_room=input.value.room;region=selected_room->region;estate_type=room_scene->room_profile->definition.type;}
   }
   if(stored&&!RentState::MutableItem(db,stored->estate_id,0,stored->custody_id))throw Failure{Error::Unauthorized};
   if(!stored||!stored->placement||stored->placement->reservation.native_item_id!=r.native_item_id||stored->placement->pose_revision!=r.expected_pose_revision)throw Failure{Error::Conflict};
   auto state=ItemStorage::Locked(db,r.custody_id);if(state.stowed!=bool(r.placement)||state.revision!=r.expected_storage_revision)throw Failure{Error::Conflict};
   if(state.revision&&state.native_estate_id!=native)throw Failure{Error::Conflict};
   std::optional<NpcNativeFrame::Frame> next_frame;
   if(r.placement){auto npc=NpcRuntime::Current(db,stored->row.values.item_id);if(npc){if(!fence.NpcProfileMatchesUnderTransaction(db,r.lease,*npc))throw Failure{Error::Conflict};next_frame=r.npc_frame;if(!next_frame&&stored->placement->npc_frame){next_frame=NpcNativeFrame::Reframe(*stored->placement->npc_frame,*r.placement);if(!next_frame)throw Failure{Error::Conflict};}geometry=NpcRuntime::Geometry(*npc,next_frame,*r.placement);}else if(r.npc_frame||stored->placement->npc_frame)throw Failure{Error::Conflict};}
   if(r.placement){
    if(!UpkeepCycle::RestoreAllowedUnderTransaction(db,stored->estate_id,r.custody_id))throw Failure{Error::Unauthorized};
    if(!state.revision||r.expected_pose_revision==UINT64_MAX||!metadata||!geometry)throw Failure{Error::Conflict};
    PlacementPolicy::Context context{native,estate_type,region.id,access,true};if(geometry->mounting_type){if(room_scene){auto exact=InteriorContent::ItemInputForStoredUnderTransaction(db,*room_scene->room_profile,*stored);context.mount_contact_verified=exact&&InteriorContent::MountForSelectedInputUnderTransaction(db,fence,r.lease,stored->row,*r.placement,exact.value,LiveItems::MountSubject::ExistingSource(stored->custody_id,stored->placement->reservation.native_item_id,stored->placement->pose_revision));}else context.mount_contact_verified=fence.MountSurfaceWithSubjectUnderTransaction(db,r.lease,stored->row,*r.placement,native,LiveItems::MountSubject::ExistingSource(stored->custody_id,stored->placement->reservation.native_item_id,stored->placement->pose_revision));}
    if(!(room_scene?InteriorContent::CheckPlacement(selected_room?*selected_room:*room_scene->room_profile,context,*metadata,*geometry,*r.placement,PlacementPolicy::Operation::Move):PlacementPolicy::CheckAny(context,*metadata,*geometry,region,yard_scene->additional_regions,*r.placement,PlacementPolicy::Operation::Move)))throw Failure{Error::Conflict};
    std::vector<uint8_t> pose;for(float v:{r.placement->heading,r.placement->pitch,r.placement->roll,r.placement->scale,r.placement->position[0],r.placement->position[1],r.placement->position[2]}){uint32_t bits;std::memcpy(&bits,&v,4);NeighborhoodWire::Detail::Write32(pose,bits);}
    auto changed=q("UPDATE housing_live_placed_items SET pose_revision=pose_revision+1,pose_blob=x'"+Hex(pose)+"' WHERE custody_id="+std::to_string(r.custody_id)+" AND native_item_id="+std::to_string(r.native_item_id)+" AND pose_revision="+std::to_string(r.expected_pose_revision));if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};
    NpcNativeFrame::ReplaceUnderTransaction(db,r.custody_id,r.native_item_id,r.expected_pose_revision,r.native_item_id,r.expected_pose_revision+1,*r.placement,stored->placement->npc_frame,next_frame);
   }
   if(r.crate_member){const auto& m=*r.crate_member;
    auto removed=q("DELETE FROM housing_crate_members WHERE custody_id="+std::to_string(r.custody_id)+" AND crate_id="+std::to_string(m.crate_id));if(removed.RowsAffected()!=1)throw Failure{Error::Conflict};
    auto changed=q("UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+std::to_string(m.crate_id)+" AND revision="+std::to_string(m.revision)+" AND packed=0");if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};
   }
   out={r.custody_id,state.revision,state.revision+1,false};
   if(!state.revision){auto changed=q("INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+std::to_string(r.custody_id)+",1,1,"+std::to_string(native)+")");if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};}
   else{auto changed=q("UPDATE housing_live_item_storage SET stowed="+std::to_string(r.placement?0:1)+",revision=revision+1 WHERE custody_id="+std::to_string(r.custody_id)+" AND revision="+std::to_string(state.revision)+" AND stowed="+std::to_string(r.placement?1:0));if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};}
   q("INSERT INTO housing_live_storage_receipts(request_key,character_id,account_id,binding_blob,custody_id,before_revision,after_revision) VALUES("+key+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+",x'"+Hex(binding)+"',"+std::to_string(out.custody_id)+","+std::to_string(out.before_revision)+","+std::to_string(out.after_revision)+")");
  }
  return out;
}
bool ValidRequest(const Request& r){return (!r.parent_yard||(r.exterior&&!r.room.house_id&&!r.room_exterior_source&&OccupiedParentYard::Shape(*r.parent_yard)))&&(!r.room_exterior_source||(!r.exterior&&r.room.house_id&&r.room_exterior_source->zone_id&&r.room_exterior_source->instance_id))&&!r.request_key.empty()&&r.request_key.size()<=96&&r.request_key.find('\0')==std::string::npos&&r.custody_id&&r.native_item_id&&r.native_item_id!=UINT32_MAX&&r.expected_pose_revision&&r.expected_storage_revision!=UINT64_MAX;}
}
LiveItems::Result<Receipt> Service::ChangeUnderTransaction(Database& db,bool enabled,const Request& r,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* profile,const CrateBinding::CustodyContext* crates){
 if(!enabled)return {{},Error::Disabled};
 if(!ValidRequest(r)||(r.crate_member&&((!r.exterior&&!r.room.house_id)||!r.placement||!r.crate_member->crate_id||!r.crate_member->revision||r.crate_member->revision>=UINT64_MAX-1)))return {{},Error::InvalidArgument};
 if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};
 try{
  auto tx=db.QueryDatabase(std::string("SELECT @@in_transaction"),false);
  if(!tx.Success())return {{},Error::DatabaseFailure};
  if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return {{},Error::InvalidArgument};
  return {ChangeChecked(db,r,fence,house,profile,{},nullptr,crates),Error::None};
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Receipt> Service::ChangeCurrent(const Request& r,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* profile){
 if(!enabled_||!database_)return {{},Error::Disabled};
 if(!ValidRequest(r)||(r.crate_member&&((!r.exterior&&!r.room.house_id)||!r.placement||!r.crate_member->crate_id||!r.crate_member->revision||r.crate_member->revision>=UINT64_MAX-1)))return {{},Error::InvalidArgument};
 if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};
 std::lock_guard<std::mutex> lock(mutex_);auto& db=*database_;bool transaction=false;
 try{
  if(!db.QueryDatabase(std::string("START TRANSACTION"),false).Success())throw Failure{Error::DatabaseFailure};transaction=true;
  auto out=ChangeChecked(db,r,fence,house,profile,{},nullptr,crates_);
  auto committed=db.QueryDatabase(std::string("COMMIT"),false);transaction=false;if(!committed.Success())return {{},Error::CommitUncertain};return {out,Error::None};
 }catch(const Failure& f){if(transaction)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},f.error};}catch(...){if(transaction)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
LiveItems::Result<std::vector<Receipt>> Service::StowAll(const StowRequest& input,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* profile){
 if(!enabled_||!database_)return {{},Error::Disabled};
 if(input.request_key.empty()||input.request_key.size()>64||input.request_key.find('\0')!=std::string::npos||input.items.empty()||input.items.size()>4096)return {{},Error::InvalidArgument};
 if(!fence.HeldFor(input.lease.actor,input.lease.epoch,input.lease.session_key))return {{},Error::UnheldFence};
 auto members=input.items;std::sort(members.begin(),members.end(),[](const auto& a,const auto& b){return a.custody_id<b.custody_id;});
 std::vector<Request> requests;std::vector<uint8_t> binding;
 NeighborhoodWire::Detail::Write32(binding,4);NeighborhoodWire::Detail::Write32(binding,members.size());
 for(const auto& member:members){
  if(!requests.empty()&&requests.back().custody_id==member.custody_id)return {{},Error::InvalidArgument};
  Request r;r.request_key=input.request_key+":"+std::to_string(member.custody_id);r.lease=input.lease;r.room=input.room;r.room.scope_revision=member.scope_revision;r.exterior=input.exterior;r.custody_id=member.custody_id;r.native_item_id=member.native_item_id;r.expected_pose_revision=member.pose_revision;r.expected_storage_revision=member.storage_revision;
  if(!ValidRequest(r))return {{},Error::InvalidArgument};
  const auto bytes=Binding(r);NeighborhoodWire::Detail::Write32(binding,bytes.size());binding.insert(binding.end(),bytes.begin(),bytes.end());requests.push_back(std::move(r));
 }
 // Bind every member to the exact canonical set without quadratic journal
 // growth or exceeding the existing BLOB column for large estates.
 std::array<uint8_t,SHA256_DIGEST_LENGTH> digest{};if(!SHA256(binding.data(),binding.size(),digest.data()))return {{},Error::DatabaseFailure};
 binding.clear();NeighborhoodWire::Detail::Write32(binding,4);binding.insert(binding.end(),digest.begin(),digest.end());
 std::lock_guard<std::mutex> lock(mutex_);auto& db=*database_;bool transaction=false;
 try{
  if(!db.QueryDatabase(std::string("START TRANSACTION"),false).Success())throw Failure{Error::DatabaseFailure};transaction=true;
  ReadCache cache;std::vector<Receipt> out;for(const auto& r:requests)out.push_back(ChangeChecked(db,r,fence,house,profile,binding,&cache,crates_));
  // Mixed replay/new membership means the caller reused a bulk key for a
  // different set. Never turn a historical retry into additional mutations.
  if(std::any_of(out.begin(),out.end(),[](const auto& x){return x.replayed;})&&!std::all_of(out.begin(),out.end(),[](const auto& x){return x.replayed;}))throw Failure{Error::Conflict};
  auto committed=db.QueryDatabase(std::string("COMMIT"),false);transaction=false;if(!committed.Success())return {{},Error::CommitUncertain};return {out,Error::None};
 }catch(const Failure& f){if(transaction)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},f.error};}catch(...){if(transaction)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
static LiveItems::Result<TransferReceipt> TransferCurrent(Database* database_,bool enabled_,const TransferRequest& r,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy& house,const InteriorContent::ProfilePolicy& profile,const CrateBinding::CustodyContext* crates_,bool externally_owned=false){
 if(!enabled_||!database_)return {{},Error::Disabled};
 const auto& occupied=r.occupied_room;
 if((r.npc_frame&&!r.placement)||r.request_key.empty()||r.request_key.size()>96||r.request_key.find('\0')!=std::string::npos||!occupied.house_id||!occupied.house_revision||!occupied.profile_revision||!r.custody_id||!r.expected_pose_revision||r.expected_storage_revision==UINT64_MAX||!r.native_item_id||r.native_item_id==UINT32_MAX||!r.source_native_estate_id||r.source_native_estate_id==UINT32_MAX)return {{},Error::InvalidArgument};
 // Direct placement may target either freshly reviewed realm. Attached-room
 // or occupied-room parent reads below establish the actual physical route;
 // source Move, destination Add, capacity and geometry remain mandatory.
 if((r.crate_member&&(!crates_||!r.crate_member->crate_id||!r.crate_member->revision||r.crate_member->revision>=UINT64_MAX-1)))return {{},Error::InvalidArgument};
 if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};
 auto& db=*database_;bool transaction=false;
 auto q=[&](const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())throw Failure{Error::DatabaseFailure};return result;};
 auto bytes=[](const std::string& value){return std::vector<uint8_t>(value.begin(),value.end());};
 const auto key="x'"+Hex(bytes(r.request_key))+"'";
 // Reuse the storage canonical encoder, preserving every actor/token/CAS field;
 // destination and source-estate words form a distinct transfer extension.
 Request canonical;canonical.request_key=r.request_key;canonical.lease=r.lease;canonical.room=occupied;canonical.custody_id=r.custody_id;canonical.native_item_id=r.native_item_id;canonical.expected_pose_revision=r.expected_pose_revision;canonical.expected_storage_revision=r.expected_storage_revision;canonical.exterior=r.yard;canonical.crate_member=r.crate_member;canonical.placement=r.placement;canonical.npc_frame=r.npc_frame;
 auto binding=Binding(canonical);NeighborhoodWire::Detail::Write32(binding,3);NeighborhoodWire::Detail::Write32(binding,r.source_native_estate_id);NeighborhoodWire::Detail::Write32(binding,r.to_room?1:0);
 try{
  if(!externally_owned){q("START TRANSACTION");transaction=true;}
  else{auto tx=q("SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")throw Failure{Error::InvalidArgument};}
  auto schema=q("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_transfer_receipts'");if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB"||!ItemStorage::Installed(db))throw Failure{Error::Disabled};
  auto room=r.yard?Scene::Service::ReadAttachedInteriorUnderTransaction(db,r.lease,r.yard->estate_id,occupied.house_id,fence,house,&profile,true,crates_):Scene::Service::ReadInteriorSceneUnderTransaction(db,r.lease,occupied.source,fence,house,profile,crates_);if(!room)throw Failure{room.error};
  auto old=q("SELECT character_id,account_id,HEX(binding_blob),custody_id,before_native_item_id,after_native_item_id,target_native_estate_id,storage_revision FROM housing_live_transfer_receipts WHERE request_key="+key+" FOR UPDATE");
  TransferReceipt out;
  if(old.RowCount()){
   if(old.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto saved=old.begin();for(size_t n=0;n<8;++n)if(!saved[n])throw Failure{Error::DatabaseFailure};
   std::string hex=saved[2];for(auto& c:hex)if(c>='A'&&c<='F')c+=32;
   if(std::stoul(saved[0])!=r.lease.actor.character_id||std::stoul(saved[1])!=r.lease.actor.account_id||hex!=Hex(binding))throw Failure{Error::Conflict};
   out={std::stoull(saved[3]),std::stoull(saved[7]),uint32_t(std::stoul(saved[4])),uint32_t(std::stoul(saved[5])),uint32_t(std::stoul(saved[6])),true};
   if(out.custody_id!=r.custody_id||out.before_native_item_id!=r.native_item_id||out.after_native_item_id==out.before_native_item_id||out.storage_revision!=r.expected_storage_revision+1)throw Failure{Error::DatabaseFailure};
  }else{
   const auto& inside=room.value;
   if(!inside.house||!inside.house->interior||!inside.room_profile||inside.house->house_id!=occupied.house_id||inside.house->revision!=occupied.house_revision||inside.profile_revision!=occupied.profile_revision)throw Failure{Error::Conflict};
   auto parent=r.yard?Scene::Service::ReadSceneUnderTransaction(db,r.lease,r.yard->estate_id,fence,&house,crates_):Scene::Service::ReadParentSceneUnderTransaction(db,r.lease,occupied.source,fence,house,crates_);if(!parent)throw Failure{parent.error};const auto& yard=parent.value;
   if(inside.context.source.zone_id!=occupied.source.zone_id||inside.context.source.zone_version!=occupied.source.zone_version||inside.context.source.instance_id!=occupied.source.instance_id)throw Failure{Error::Conflict};
   if(r.yard&&(yard.native_plot_id!=r.yard->native_estate_id||yard.current_catalog_revision!=r.yard->catalog_revision||yard.route.zone_id!=r.yard->source.zone_id||yard.route.zone_version!=r.yard->source.zone_version||yard.route.instance_id!=r.yard->source.instance_id))throw Failure{Error::Conflict};
   const auto native_room=inside.house->interior->native_estate_id;
   if(inside.context.estate_id!=yard.estate_id||inside.context.native_plot_id!=yard.native_plot_id||(r.to_room?r.source_native_estate_id!=yard.native_plot_id:r.source_native_estate_id!=native_room))throw Failure{Error::Conflict};
   const auto& source_access=r.to_room?yard.access:inside.access.effective;
   const auto& target_access=r.to_room?inside.access.effective:yard.access;
   if(!source_access.Allows(AccessPolicy::Capability::MoveItem)||!target_access.Allows(AccessPolicy::Capability::AddItem))throw Failure{Error::Unauthorized};
   const LiveItems::Stored* stored=nullptr;
   if(r.to_room){
    if(occupied.scope_revision)throw Failure{Error::Conflict};
    const auto& members=r.crate_member?yard.crate_contents:yard.items;auto item=std::find_if(members.begin(),members.end(),[&](const auto& item){return item.stored.custody_id==r.custody_id;});if(item==members.end()||item->house||(r.crate_member&&item->crate))throw Failure{Error::UnsupportedItem};stored=&item->stored;
   }else{
    const auto& members=r.crate_member?inside.crate_contents:inside.contents;auto item=std::find_if(members.begin(),members.end(),[&](const auto& item){return item.stored.custody_id==r.custody_id;});if(item==members.end()||!item->room||item->scope_revision!=occupied.scope_revision||(r.crate_member&&item->crate))throw Failure{Error::Conflict};stored=&item->stored;
   }
   if(!stored||!stored->placement||(r.to_room&&stored->estate_id!=yard.estate_id)||stored->placement->reservation.native_estate_id!=r.source_native_estate_id||stored->placement->reservation.native_item_id!=r.native_item_id||stored->placement->pose_revision!=r.expected_pose_revision||stored->storage_revision!=r.expected_storage_revision)throw Failure{Error::Conflict};
   if(!RentState::MutableItem(db,stored->estate_id,0,stored->custody_id))throw Failure{Error::Unauthorized};
   const auto state=ItemStorage::Locked(db,r.custody_id);if(state.revision!=r.expected_storage_revision||state.stowed!=stored->stowed||(state.revision&&state.native_estate_id!=r.source_native_estate_id))throw Failure{Error::Conflict};
   if(r.crate_member){const auto& m=*r.crate_member;
    const auto physical=r.to_room?yard.route:inside.exterior_source.value_or(inside.context.source);
    if(!source_access.Allows(AccessPolicy::Capability::RemoveItem)||!stored->stowed||m.native_plot_id!=r.source_native_estate_id||m.catalog_revision!=yard.current_catalog_revision||m.source.zone_id!=physical.zone_id||m.source.zone_version!=physical.zone_version||m.source.instance_id!=physical.instance_id)throw Failure{Error::Unauthorized};
    auto membership=CrateMemberScope::Locked(db,r.custody_id,!r.to_room);if(!membership||membership->crate_id!=m.crate_id)throw Failure{Error::Conflict};
    std::shared_ptr<const CrateBinding::Allocation> container;
    if(r.to_room){for(const auto& i:yard.items)if(i.crate&&i.crate->binding.crate_id==m.crate_id&&!i.stored.stowed)container=i.crate;}
    else{for(const auto& i:inside.contents)if(i.crate&&i.crate->binding.crate_id==m.crate_id&&!i.stored.stowed)container=i.crate;}
    if(!container||container->packed||container->binding.revision!=m.revision||container->binding.parent_native_estate_id!=r.source_native_estate_id)throw Failure{Error::Conflict};
   }
   const auto& definition=r.to_room?inside.room_profile->definition:yard.definition;
   const auto count=r.to_room?inside.contents.size():yard.items.size();
   if(!definition.max_items||count>=definition.max_items)throw Failure{Error::Conflict};
   auto maximum=q("SELECT UNK231 FROM items WHERE id="+std::to_string(stored->row.values.item_id)+" FOR UPDATE");if(maximum.RowCount()!=1||!maximum.begin()[0])throw Failure{Error::UnsupportedItem};auto limit=std::stoll(maximum.begin()[0]);if(limit<INT32_MIN||limit>INT32_MAX)throw Failure{Error::UnsupportedItem};
   uint64_t same=0;if(r.to_room){for(const auto& item:inside.contents)if(item.stored.row.values.item_id==stored->row.values.item_id)++same;}else{for(const auto& item:yard.items)if(item.stored.row.values.item_id==stored->row.values.item_id)++same;}
   if(limit>0&&same>=uint64_t(limit))throw Failure{Error::Conflict};
   const auto target=r.to_room?native_room:yard.native_plot_id;
   const auto target_transform=r.placement.value_or(PlacementPolicy::Transform{0,0,0,1,{0,0,0}});std::optional<NpcNativeFrame::Frame> next_frame=r.npc_frame;
   auto npc=NpcRuntime::Current(db,stored->row.values.item_id);
   if(npc){if(!next_frame&&stored->placement->npc_frame){next_frame=NpcNativeFrame::Reframe(*stored->placement->npc_frame,target_transform);if(!next_frame)throw Failure{Error::Conflict};}(void)NpcRuntime::Geometry(*npc,next_frame,target_transform);}else if(next_frame||stored->placement->npc_frame)throw Failure{Error::Conflict};
   if(npc){if(!fence.NpcProfileMatchesUnderTransaction(db,r.lease,*npc)||r.crate_member)throw Failure{Error::Conflict};if(!npc->service&&!NpcRuntime::Capacity(definition.max_npc_items,NpcRuntime::Count(db,target,r.to_room?inside.house->house_id:0)))throw Failure{Error::NpcFull};}
   std::optional<InteriorContent::Fingerprint> room_identity;std::optional<ContentBinding::Identity> yard_identity;
   if(r.to_room){auto input=InteriorContent::ItemInputUnderTransaction(db,*inside.room_profile,stored->row.values.item_id,{},r.to_room);if(!input)throw Failure{input.error};auto identity=InteriorContent::Build(input.value);if(!identity)throw Failure{Error::UnsupportedItem};room_identity=identity.value;}
   else{
    const auto& catalog_key=inside.house->interior->catalog_key;
    auto catalog=q("SELECT revision,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key=x'"+Hex(bytes(catalog_key))+"' FOR UPDATE");if(catalog.RowCount()!=1||std::stoull(catalog.begin()[0])!=yard.current_catalog_revision)throw Failure{Error::Conflict};
    auto unhex=[](const char* p){std::string value=p?p:"";if(value.size()%2||value.size()>262140)throw Failure{Error::DatabaseFailure};std::vector<uint8_t> result;auto nibble=[](char c){if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw Failure{Error::DatabaseFailure};};for(size_t i=0;i<value.size();i+=2)result.push_back(uint8_t(nibble(value[i])*16+nibble(value[i+1])));return result;};
    const auto bundle=Catalog::DecodePayloads(unhex(catalog.begin()[1]),unhex(catalog.begin()[2]));yard_identity=ContentBinding::Current(db,catalog_key,bundle,target,yard.route,stored->row.values.item_id);
   }
   if(r.placement){
    if(!UpkeepCycle::RestoreAllowedUnderTransaction(db,stored->estate_id,r.custody_id))throw Failure{Error::Unauthorized};
    auto input=InteriorContent::ItemInputUnderTransaction(db,*inside.room_profile,stored->row.values.item_id,{},r.to_room);if(!input)throw Failure{input.error};
    if(input.value.npc)input.value.geometry=NpcRuntime::Geometry(*input.value.npc,next_frame,*r.placement);
    const auto& region=r.to_room?inside.room_profile->region:yard.region;
    PlacementPolicy::Context checked{target,definition.type,region.id,target_access,true};if(input.value.geometry.mounting_type)checked.mount_contact_verified=r.to_room?InteriorContent::MountForSelectedInputUnderTransaction(db,fence,r.lease,stored->row,*r.placement,input.value,LiveItems::MountSubject::ExistingSource(stored->custody_id,stored->placement->reservation.native_item_id,stored->placement->pose_revision)):fence.MountSurfaceWithSubjectUnderTransaction(db,r.lease,stored->row,*r.placement,target,LiveItems::MountSubject::ExistingSource(stored->custody_id,stored->placement->reservation.native_item_id,stored->placement->pose_revision));
    if(!(r.to_room?InteriorContent::CheckPlacement(input.value.room,checked,input.value.item,input.value.geometry,*r.placement,PlacementPolicy::Operation::Move):PlacementPolicy::CheckAny(checked,input.value.item,input.value.geometry,region,yard.additional_regions,*r.placement,PlacementPolicy::Operation::Move)))throw Failure{Error::Conflict};
   }
   auto origin=q("SELECT character_id,account_id,lease_epoch,HEX(session_key),HEX(source_row),runtime_serial FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(r.native_item_id)+" AND state=1 AND custody_id="+std::to_string(r.custody_id)+" FOR UPDATE");if(origin.RowCount()!=1)throw Failure{Error::Conflict};auto src=origin.begin();for(size_t i=0;i<6;++i)if(!src[i])throw Failure{Error::DatabaseFailure};
   std::string source_hex=src[4];for(auto& c:source_hex)if(c>='A'&&c<='F')c+=32;
   if(std::stoul(src[0])!=stored->row.values.character_id||source_hex!=Hex(CustodyCodec::Encode(stored->row))||std::stoll(src[5])<=0)throw Failure{Error::Conflict};
   // Preserve the creator/source provenance. The transfer journal independently
   // binds this operation to the currently authenticated actor and held lease.
   const auto catalog_key=inside.house->interior->catalog_key;
   // Release the unique active-custody slot before allocating its replacement.
   auto retired=q("UPDATE housing_live_placement_reservations SET state=2,custody_id=NULL WHERE native_item_id="+std::to_string(r.native_item_id)+" AND state=1 AND custody_id="+std::to_string(r.custody_id));if(retired.RowsAffected()!=1)throw Failure{Error::Conflict};
   auto created=q("INSERT INTO housing_live_placement_reservations(request_key,character_id,account_id,lease_epoch,session_key,estate_id,native_estate_id,catalog_key,catalog_revision,source_row,runtime_serial,state,custody_id"+std::string(yard_identity?",content_binding_version,content_binding_digest":"")+") VALUES("+key+","+src[0]+","+src[1]+","+src[2]+",x'"+src[3]+"',"+std::to_string(yard.estate_id)+","+std::to_string(target)+",x'"+Hex(bytes(catalog_key))+"',"+std::to_string(yard.current_catalog_revision)+",x'"+src[4]+"',"+src[5]+",1,"+std::to_string(r.custody_id)+(yard_identity?","+std::to_string(yard_identity->version)+",x'"+Hex(std::vector<uint8_t>(yard_identity->digest.begin(),yard_identity->digest.end()))+"'":"")+")");
   const auto id=created.LastInsertedID();if(!id||id>=UINT32_MAX)throw Failure{Error::Conflict};
   if(stored->estate_id!=yard.estate_id){auto moved=q("UPDATE housing_live_item_custody SET estate_id="+std::to_string(yard.estate_id)+" WHERE custody_id="+std::to_string(r.custody_id)+" AND estate_id="+std::to_string(stored->estate_id));if(moved.RowsAffected()!=1)throw Failure{Error::Conflict};}
   if(room_identity){auto tag=q("INSERT INTO housing_interior_placement_reservations(native_item_id,house_id,house_revision,profile_revision,source_zone,source_version,source_instance,content_version,content_digest) VALUES("+std::to_string(id)+","+std::to_string(occupied.house_id)+","+std::to_string(occupied.house_revision)+","+std::to_string(occupied.profile_revision)+","+std::to_string(occupied.source.zone_id)+","+std::to_string(occupied.source.zone_version)+","+std::to_string(occupied.source.instance_id)+",3,x'"+Hex(std::vector<uint8_t>(room_identity->digest.begin(),room_identity->digest.end()))+"')");if(tag.RowsAffected()!=1)throw Failure{Error::Conflict};}
   if(r.to_room){if(HouseItemScope::Locked(db,r.custody_id))throw Failure{Error::Conflict};q("INSERT INTO housing_house_item_scopes(custody_id,house_id,revision) VALUES("+std::to_string(r.custody_id)+","+std::to_string(occupied.house_id)+",1)");}
   else{auto scope=q("DELETE FROM housing_house_item_scopes WHERE custody_id="+std::to_string(r.custody_id)+" AND house_id="+std::to_string(occupied.house_id)+" AND revision="+std::to_string(occupied.scope_revision));if(scope.RowsAffected()!=1)throw Failure{Error::Conflict};}
   // The previous pose belongs to a historical reservation. The new identity is
   // placed or stored in the destination; never mutate an old receipt's pose/tag.
   std::vector<uint8_t> target_pose;const auto placed=r.placement.value_or(PlacementPolicy::Transform{0,0,0,1,{0,0,0}});for(float value:{placed.heading,placed.pitch,placed.roll,placed.scale,placed.position[0],placed.position[1],placed.position[2]}){uint32_t bits;std::memcpy(&bits,&value,4);NeighborhoodWire::Detail::Write32(target_pose,bits);}
   auto pose=q("UPDATE housing_live_placed_items SET native_item_id="+std::to_string(id)+",pose_revision=1,pose_blob=x'"+Hex(target_pose)+"' WHERE custody_id="+std::to_string(r.custody_id)+" AND native_item_id="+std::to_string(r.native_item_id)+" AND pose_revision="+std::to_string(r.expected_pose_revision));if(pose.RowsAffected()!=1)throw Failure{Error::Conflict};
   NpcNativeFrame::ReplaceUnderTransaction(db,r.custody_id,r.native_item_id,r.expected_pose_revision,uint32_t(id),1,placed,stored->placement->npc_frame,next_frame);
   if(!state.revision)q("INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+std::to_string(r.custody_id)+",1,"+std::to_string(r.placement?0:1)+","+std::to_string(target)+")");
   else{auto marker=q("UPDATE housing_live_item_storage SET revision=revision+1,stowed="+std::to_string(r.placement?0:1)+",native_estate_id="+std::to_string(target)+" WHERE custody_id="+std::to_string(r.custody_id)+" AND revision="+std::to_string(state.revision)+" AND stowed="+std::to_string(state.stowed?1:0)+" AND native_estate_id="+std::to_string(r.source_native_estate_id));if(marker.RowsAffected()!=1)throw Failure{Error::Conflict};}
   if(r.crate_member){const auto& m=*r.crate_member;
    auto removed=q("DELETE FROM housing_crate_members WHERE custody_id="+std::to_string(r.custody_id)+" AND crate_id="+std::to_string(m.crate_id));if(removed.RowsAffected()!=1)throw Failure{Error::Conflict};
    auto changed=q("UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+std::to_string(m.crate_id)+" AND revision="+std::to_string(m.revision)+" AND packed=0");if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};
   }
   out={r.custody_id,state.revision+1,r.native_item_id,uint32_t(id),target,false};
   q("INSERT INTO housing_live_transfer_receipts(request_key,character_id,account_id,binding_blob,custody_id,before_native_item_id,after_native_item_id,target_native_estate_id,storage_revision) VALUES("+key+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+",x'"+Hex(binding)+"',"+std::to_string(out.custody_id)+","+std::to_string(out.before_native_item_id)+","+std::to_string(out.after_native_item_id)+","+std::to_string(out.target_native_estate_id)+","+std::to_string(out.storage_revision)+")");
  }
  if(!externally_owned){auto committed=db.QueryDatabase(std::string("COMMIT"),false);transaction=false;if(!committed.Success())return {{},Error::CommitUncertain};}return {out,Error::None};
 }catch(const Failure& f){if(transaction)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},f.error};}catch(...){if(transaction)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
LiveItems::Result<TransferReceipt> Service::Transfer(const TransferRequest& r,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy& house,const InteriorContent::ProfilePolicy& profile){
 std::lock_guard<std::mutex> lock(mutex_);return TransferCurrent(database_.get(),enabled_,r,fence,house,profile,crates_);
}
LiveItems::Result<TransferReceipt> Service::TransferUnderTransaction(Database& db,bool enabled,const TransferRequest& r,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy& house,const InteriorContent::ProfilePolicy& profile,const CrateBinding::CustodyContext* crates){
 return TransferCurrent(&db,enabled,r,fence,house,profile,crates,true);
}
}
