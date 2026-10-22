#include "../common/housing/vault_partition_application.h"
#include "../common/housing/interior_wall_slab_registry.h"
#include "../common/housing/stack_partition_application.h"
#include "../common/housing/scene_item_projection.h"
#include "../common/housing/crate_scene_presentation.h"
#include "../common/housing/interior_storage_service.h"
#include "../common/housing/layout_inventory_plan.h"
#include "../common/housing/layout_inventory_coordinator_db.h"
#include "../common/housing/layout_mixed_coordinator_db.h"
#include "housing_inventory_bridge.h"
#include "../common/housing/guild_portal_db.h"
#include "../common/housing/inventory_row_db.h"
#include "../common/housing/return_parcel_db.h"
#include "../common/housing/return_settlement_db.h"
#include "housing_house_surface.h"
#include "housing_height_scene.h"
#include "../common/housing/centered_wall_policy.h"
#include "map.h"
#include "../common/path_manager.h"
#include "../common/housing/mount_admission.h"
#include <fstream>
#include <cstdlib>
#include <memory>
#include "../common/housing/estate_retirement_db.h"
#include "../common/housing/occupied_move_out_db.h"
#include "../common/patches/rof2.h"
#include "../common/housing/upkeep_selection_db.h"
#include "../common/housing/custody_codec.h"
#include "housing_coin_profile.h"
#include "../common/housing/house_item_application.h"
#include "../common/housing/crate_item_application.h"
#include "client.h"
#include "../common/housing/npc_runtime_db.h"
#include "../common/eqemu_config.h"
#include "../common/rulesys.h"
#include <openssl/rand.h>
#include <utility>
#include <set>
#include <chrono>
#include <cstring>
#include <thread>
#include <mutex>
#include <stdexcept>
namespace Housing::InventoryBridge {
    namespace {
        std::mutex thread_mutex;
        std::thread::id zone_thread;
        bool registered=false;
        thread_local bool barrier=false;
        void Wait(){
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        bool ZoneThread(){
            std::lock_guard<std::mutex> lock(thread_mutex);
            return registered&&std::this_thread::get_id()==zone_thread;
        }
    }
    bool IsRegisteredZoneThread() noexcept { return ZoneThread(); }
    void RegisterZoneThreadAtSafeStartup(){
        std::lock_guard<std::mutex> lock(thread_mutex);
        if(registered&&zone_thread!=std::this_thread::get_id())throw std::logic_error("Housing zone thread already bound");
        zone_thread=std::this_thread::get_id();
        registered=true;
    }
    bool Bridge::CanCreateForClient(Client& c) noexcept {
        return ZoneThread()&&!barrier&&c.HasInventorySessionReservation()&&c.ClientDataLoaded()&&c.Connected()&&c.Connection()&&c.CharacterID()>0&&c.AccountID()>0;
    }
    Bridge::Bridge(Client& c,ConnectionFactory factory,HousePolicyFactory houses):client_(c),factory_(std::move(factory)),actor_{
        c.CharacterID(),c.AccountID()
    },connection_(c.Connection()){
        if(!CanCreateForClient(c))throw std::logic_error("Housing bridge requires active zone-thread Client");
        unsigned char bytes[32];
        if(RAND_bytes(bytes,32)!=1)throw std::runtime_error("Housing session nonce failed");
        const char* hex="0123456789abcdef";
        session_key_="zone:";
        for(auto b:bytes){
            session_key_+=hex[b>>4];
            session_key_+=hex[b&15];
        }
        if(houses){
            house_policies_=houses(c,*this,*this);
            // Visitors may read registered house scenes and use their own
            // crate/furniture without permission to adopt a new house token.
            // A custody grant still requires its corresponding scene policy.
            if(house_policies_.custody&&!house_policies_.scene)throw std::logic_error("House custody requires Client scene policy");
            if(house_policies_.custody && &house_policies_.custody->Application()!=static_cast<HouseBinding::HouseApplicationProof*>(this))throw std::logic_error("House context must use this Client's real application proof");
        }
    }
    Bridge::~Bridge(){
        if(held_||lease_.epoch)FreezeInvariantFailure();
    }
    bool Bridge::Enter(bool closing){
        if(!ZoneThread()||barrier||held_||(!closing&&!client_.Connected())||client_.Connection()!=connection_||client_.CharacterID()!=actor_.character_id||client_.AccountID()!=actor_.account_id)return false;
        barrier=true;
        held_=true;
        return true;
    }
    void Bridge::Leave(){
        held_=false;
        barrier=false;
        if(std::exchange(trophy_sources_changed_,false))Client::HousingTrophySourcesChanged();
    }
    std::unique_ptr<LiveItems::Service> Bridge::Fresh()const{
        return factory_?factory_(house_policies_.custody.get(),house_policies_.crates.get()):nullptr;
    }
    bool Bridge::HeldFor(AuthenticatedPurchaser a,uint64_t epoch,std::string_view session)const noexcept{
        return held_&&barrier&&ZoneThread()&&client_.HasInventorySessionReservation()&&a.character_id==actor_.character_id&&a.account_id==actor_.account_id&&client_.CharacterID()==actor_.character_id&&client_.AccountID()==actor_.account_id&&client_.Connection()==connection_&&session==session_key_&&(epoch==0||epoch==lease_.epoch);
    }
    bool Bridge::SourceMatches(const LiveItems::InventoryRow& r,int32_t serial)const noexcept{
        try{
            auto& inv=client_.GetInv();
            auto item=inv.GetItem(int16_t(r.values.slot_id));
            return held_&&item&&SlotSupported(inv,r.values.slot_id)&&CursorCompatible(inv,r.values.slot_id,true)&&item->GetSerialNumber()==serial&&Matches(r,*item);
        }
        catch(...){
            return false;
        }
    }
    bool Bridge::LayoutSourceMatches(const LiveItems::InventoryRow& row,int32_t serial)const noexcept{
        try{
            auto& inv=client_.GetInv();auto* item=inv.GetItem(int16_t(row.values.slot_id));
            return HeldFor(actor_,lease_.epoch,session_key_)&&item&&
                LayoutInventory::SlotSupported(inv,row.values.slot_id)&&item->GetSerialNumber()==serial&&Matches(row,*item);
        }catch(...){return false;}
    }
    bool Bridge::PlacementContextMatches(EstateId estate,uint32_t native_estate,uint16_t instance)const noexcept{
        // The service resolves these estate IDs through its locked canonical
        // catalog. Here we prove the live simulation context, never DB location.
        return estate && native_estate && instance && HeldFor(actor_,lease_.epoch,session_key_)
            && client_.Connected() && zone && zone->GetZoneID()==712
            && zone->GetInstanceID()==instance;
    }
    bool Bridge::ParentMatchesUnderTransaction(Database& db,const LiveItems::Lease& lease,LiveItems::InventorySessionFence& fence,EstateId estate,uint32_t native,uint16_t instance)const noexcept{
        try{
            if(&fence!=static_cast<const LiveItems::InventorySessionFence*>(this)||
                !HeldFor(lease.actor,lease.epoch,lease.session_key)||!client_.Connected()||
                !zone||zone->GetZoneID()==712||!zone->GetInstanceID()||!house_policies_.scene)return false;
            const Route source{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
            auto room=Scene::Service::ReadInteriorManagementUnderTransaction(db,lease,source,fence,*house_policies_.scene);
            return room&&room.value.house&&room.value.context.estate_id==estate&&
                room.value.context.native_plot_id==native&&room.value.context.neighborhood.instance_id==instance;
        }catch(...){return false;}
    }
    bool Bridge::InteriorCoinSourceMatches(const LiveEscrow::InteriorSource& source)const noexcept{
        return HeldFor(actor_,lease_.epoch,session_key_)&&client_.Connected()&&zone&&zone->GetZoneID()==source.physical.zone_id&&zone->GetInstanceVersion()==source.physical.zone_version&&zone->GetInstanceID()==source.physical.instance_id;
    }
namespace {
std::string MountFileHash(const std::string& path){std::ifstream file(path,std::ios::binary);if(!file)return {};SHA256_CTX ctx;SHA256_Init(&ctx);char block[65536];while(file){file.read(block,sizeof(block));auto n=file.gcount();if(n>0)SHA256_Update(&ctx,block,size_t(n));}if(!file.eof())return {};unsigned char bytes[32];SHA256_Final(bytes,&ctx);std::string out;const char* h="0123456789abcdef";for(auto v:bytes){out+=h[v>>4];out+=h[v&15];}return out;}
std::shared_ptr<const Map> ReviewedMountMap(const InteriorContent::Profile& p){
#ifdef USE_MAP_MMFS
 // An unbound MMF cache is not authority for the requested .map digest.
 return {};
#else
 if(!p.reviewed_room||!OriginalInteriors::Matches(p.route.zone_id,p.zone_short_name)||!ContentBinding::Detail::Hash(p.collision_map_sha256))return {};
 // One retained revision per original registered family, not one permanent
 // entry per hash. Matches above bounds the keys to OriginalInteriors::Known,
 // including every enrolled family; there is no arbitrary eight-map ceiling.
 struct Entry {std::string hash;std::shared_ptr<const Map> map;};
 static std::mutex mutex;static std::map<uint32_t,Entry> maps;
 std::lock_guard<std::mutex> lock(mutex);auto old=maps.find(p.route.zone_id);
 if(old!=maps.end()){
  if(old->second.hash==p.collision_map_sha256)return old->second.map;
  // A synchronous mount operation holds its own shared owner. Never replace
  // an in-use revision or accumulate stale revisions behind active readers.
  if(old->second.map.use_count()!=1)return {};
 }
 const auto path=PathManager::Instance()->GetMapsPath()+"/base/"+p.zone_short_name+".map";
 if(MountFileHash(path)!=p.collision_map_sha256)return {};
 auto result=std::make_shared<Map>();if(!result->Load(path)||MountFileHash(path)!=p.collision_map_sha256)return {};
 Entry replacement{p.collision_map_sha256,std::move(result)};
 if(old!=maps.end()){old->second=std::move(replacement);return old->second.map;}
 return maps.emplace(p.route.zone_id,std::move(replacement)).first->second.map;
#endif
}
}
bool Bridge::NpcProfileMatchesUnderTransaction(Database& db,const LiveItems::Lease& lease,const NpcRuntime::Profile& profile)const noexcept{
 try{if(!HeldFor(lease.actor,lease.epoch,lease.session_key))return false;auto tx=db.QueryDatabase(std::string("SELECT @@in_transaction"),false);if(!tx.Success()||tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return false;const auto* data=database.GetItem(profile.item_id);auto cached=data?NpcRuntime::Cached(*data):std::nullopt;return cached&&NpcRuntime::Same(*cached,profile);}catch(...){return false;}
}
bool Bridge::MountSurfaceUnderTransaction(Database& db,const LiveItems::Lease& lease,const LiveItems::InventoryRow& row,const PlacementPolicy::Transform& pose,uint32_t native)const noexcept{return MountSurfaceSelectedUnderTransaction(db,lease,row,pose,native,nullptr);}
bool Bridge::MountSurfaceWithSubjectUnderTransaction(Database& db,const LiveItems::Lease& lease,const LiveItems::InventoryRow& row,const PlacementPolicy::Transform& pose,uint32_t native,const LiveItems::MountSubject& subject)const noexcept{return MountSurfaceSelectedUnderTransaction(db,lease,row,pose,native,nullptr,subject);}
bool Bridge::MountInteriorSurfaceUnderTransaction(Database& db,const LiveItems::Lease& lease,const LiveItems::InventoryRow& row,const PlacementPolicy::Transform& pose,uint32_t native,const LiveItems::InteriorMountContext& selected)const noexcept{return MountSurfaceSelectedUnderTransaction(db,lease,row,pose,native,&selected,selected.subject);}
bool Bridge::MountSurfaceSelectedUnderTransaction(Database& db,const LiveItems::Lease& lease,const LiveItems::InventoryRow& row,const PlacementPolicy::Transform& pose,uint32_t native,const LiveItems::InteriorMountContext* selected,const LiveItems::MountSubject& subject)const noexcept{
 try{
  if(!zone||!HeldFor(lease.actor,lease.epoch,lease.session_key)||!house_policies_.scene||!house_policies_.room)return false;
  auto tx=db.QueryDatabase(std::string("SELECT @@in_transaction"),false);if(!tx.Success()||tx.RowCount()!=1||!tx.begin()[0]||std::stoul(tx.begin()[0])!=1)return false;
  const auto* model=StaticFurniture::Lookup(row.values.item_id);if(!model||!StaticFurniture::Mounted(row.values.item_id))return false;
  auto metadata=db.QueryDatabase("SELECT idfile,UNK226 FROM items WHERE id="+std::to_string(row.values.item_id)+" FOR UPDATE",false);if(!metadata.Success()||metadata.RowCount()!=1||!metadata.begin()[0]||!metadata.begin()[1]||std::string(metadata.begin()[0])!=model->model||std::stoul(metadata.begin()[1])!=model->mounting_type)return false;
  const auto flags=MountFlags::ReadUnderTransaction(db,row.values.item_id);const bool free_permitted=flags.known&&!flags.disable_free;
  const Route actual{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
  // Resolve the actual current realm again on this SAME connection/TX. Packet
  // source, cached presentations and previously selected house poses confer no
  // surface/ACL authority. Existing scene code locks creator pending state too.
  std::shared_ptr<const Map> map;std::optional<Scene::Snapshot> yard;std::optional<Scene::InteriorSnapshot> room;std::optional<InteriorWallSlab::Selection> wall_selection;
  bool base_free_allowed=false;
  const auto base_free=[&](){if(base_free_allowed)LogInfo("Housing effective Free item [{}] subject [{}] original-room bounds accepted",row.values.item_id,subject.native_item_id);return base_free_allowed;};
  auto target=db.QueryDatabase("SELECT house_id FROM housing_house_interiors WHERE native_estate_id="+std::to_string(native)+" FOR UPDATE",false);if(!target.Success()||target.RowCount()>1)return false;
  if(target.RowCount()){
   const auto house=std::stoull(target.begin()[0]);if(!selected||selected->house_id!=house||!subject.Valid())return false;
   auto read=actual.zone_id==712?Scene::Service::ReadPublishedAttachedInteriorCollisionUnderTransaction(db,lease,house,const_cast<Bridge&>(*this),*house_policies_.scene,*house_policies_.room,house_policies_.crates.get()):Scene::Service::ReadPublishedInteriorCollisionUnderTransaction(db,lease,actual,const_cast<Bridge&>(*this),*house_policies_.scene,*house_policies_.room,house_policies_.crates.get());
   if(!read||!read.value.house||!read.value.house->interior||read.value.house->house_id!=house||read.value.house->interior->native_estate_id!=native||!read.value.room_profile)return false;
   room=std::move(read.value);
   // Preserve the mutation's exact stored/new content identity before trying
   // any member of the mode union. Free must never reinterpret a V7 room as
   // the original room or rewrite that immutable selected fingerprint.
   auto exact=InteriorContent::MountSelectionUnderTransaction(db,*room->room_profile,row.values.item_id,*selected);if(!exact)return false;
   auto base=InteriorContent::ItemInputUnderTransaction(db,*room->room_profile,row.values.item_id);if(!base||base.value.npc||!base.value.item.furniture_only)return false;
   const auto*runtime=database.GetItem(row.values.item_id);const auto&wire=base.value.native_metadata;
   if(!runtime||runtime->ID!=row.values.item_id||std::string(runtime->IDFile)!=model->model||
      runtime->Placeable!=wire.placeable||bool(runtime->HousingIgnoreCollisions)!=wire.ignore_collisions||
      runtime->HousingPlacementType!=wire.placement_type||runtime->HousingDefinitionID!=wire.definition_id||
      runtime->HousingMinimumScale!=wire.scale_min||runtime->HousingMaximumScale!=wire.scale_max||
      runtime->HousingUpkeepID!=wire.upkeep_id||runtime->HousingMaxPerEstate!=wire.max_per_estate||
      !NpcItemMetadata::Ordinary(*runtime))return false;
   const bool effective_flags=flags.known?
    (runtime->HousingPlacementFlagsKnown&&bool(runtime->HousingDisableRotation)==flags.disable_rotation&&bool(runtime->HousingDisableFreePlacement)==flags.disable_free):
    (!runtime->HousingPlacementFlagsKnown&&!runtime->HousingDisableRotation&&!runtime->HousingDisableFreePlacement);
   if(!effective_flags)return false;
   // Current RoF2 item and placed-actor serializers emit these cached bytes.
   // Unknown SQL columns therefore describe effective outgoing Free0 here,
   // not recovered historical Live metadata. Base-only containment and the
   // actual current ACL are required; no selected/expanded V7 regions enter.
   PlacementPolicy::Context free_context{native,room->room_profile->definition.type,room->room_profile->region.id,room->access.effective,true,false,true};
   const auto operation=subject.kind==LiveItems::MountSubject::Kind::Move?PlacementPolicy::Operation::Move:PlacementPolicy::Operation::Add;
   base_free_allowed=!runtime->HousingDisableFreePlacement&&bool(InteriorContent::CheckPlacement(*room->room_profile,free_context,base.value.item,base.value.geometry,pose,operation));
   if(actual.zone_id!=712){
    // Try original Normal/Floor/Ceiling first, keeping Free out of this query.
    // This operational profile is never stored or included in fingerprints.
    MountFlags::Flags query_flags{true,runtime->HousingDisableRotation!=0,true};
    const auto height=HeightScene::Admit(client_,db,lease,*room,subject,row,pose,query_flags,const_cast<Bridge&>(*this));
    LogInfo("Housing original height item [{}] subject [{}] result [{}] stage [{}] queries [{}] narrowphase [{}] operations [{}]",row.values.item_id,subject.native_item_id,unsigned(height.status),height.stage,height.queries,height.narrowphase,height.kernel_steps);
    if(height)return true;
    if(height.status==HeightScene::Status::SourceChanged)return false;
    // Ceiling has no independent Wall mode. A missing/changed source must not
    // fall back to the predecessor's approximate local map-ray admission.
    if(model->mounting_type==2)return base_free();
   }
   // Only the separate reviewed typed region path requires this stronger proof.
   // Base Free/floor/ceiling semantics stay exact. Scene reads below are pure
   // committed geometry validation and never recurse into this surface method.
   if(InteriorWallSlab::Eligible(model->geometry)){
    PlacementPolicy::Context context{native,room->room_profile->definition.type,room->room_profile->region.id,{6,AccessPolicy::KnownCapabilities,true},true,false,true};
    if(exact.value.policy_revision==CenteredWall::Revision||exact.value.policy_revision==CenteredWall::ContactRevision||!InteriorContent::CheckPlacement(*room->room_profile,context,exact.value.input.item,exact.value.input.geometry,pose,PlacementPolicy::Operation::Add)){
     wall_selection=std::move(exact.value);if(!wall_selection||!wall_selection->wall_policy)return false;
    }
   }
   if(model->mounting_type==1){map=ReviewedMountMap(*room->room_profile);if(!map)return false;}
  }else{
   if(selected)return false;
   auto estate=db.QueryDatabase("SELECT ownership_estate_id FROM housing_native_estates WHERE native_id="+std::to_string(native)+" AND active=1 FOR UPDATE",false);if(!estate.Success()||estate.RowCount()!=1||!estate.begin()[0])return false;const auto id=uint32_t(std::stoul(estate.begin()[0]));
   auto read=actual.zone_id==712?Scene::Service::ReadSceneUnderTransaction(db,lease,id,const_cast<Bridge&>(*this),house_policies_.scene.get(),house_policies_.crates.get()):Scene::Service::ReadParentSceneUnderTransaction(db,lease,actual,const_cast<Bridge&>(*this),*house_policies_.scene,house_policies_.crates.get());if(!read||read.value.native_plot_id!=native)return false;yard=std::move(read.value);
  }
  struct House {const HouseBinding::Binding* binding;const LiveItems::Stored* row;std::unique_ptr<HouseSurface::Provider> provider;};std::vector<House> houses;
  if(yard&&!free_permitted)for(const auto& item:yard->items)if(item.house&&!item.stored.stowed){auto surface=HouseSurface::Provider::Build(*item.house,item.stored);if(!surface)return false;houses.push_back({item.house.get(),&item.stored,std::move(surface)});}
  // Neighborhood .map data alone cannot supply a placed building. Yard
  // mounting uses these exact original transformed house triangles. No broad
  // neighborhood surface trust is inferred from the zone number.
  const auto trace=[&](const PlacementContact::Vector& start,const PlacementContact::Vector& end)->std::optional<PlacementContact::Contact>{
   std::optional<PlacementContact::Contact> nearest;float distance=INFINITY;
   const auto consider=[&](const std::optional<PlacementContact::Contact>& hit){if(!hit)return;float d=0;for(unsigned i=0;i<3;++i){auto n=hit->point[i]-start[i];d+=n*n;}if(d<distance){distance=d;nearest=hit;}};
   if(map){glm::vec3 normal;float hit_distance;if(map->DoCollisionCheck({start[0],start[1],start[2]},{end[0],end[1],end[2]},normal,hit_distance))consider(PlacementContact::FromMapRay(start,end,{normal.x,normal.y,normal.z},hit_distance));}
   for(const auto& house:houses)consider(house.provider->Trace(*house.binding,*house.row,start,end));return nearest;
  };
  if(wall_selection){
   if(!room||!map)return false;PlacementPolicy::Context context{native,room->room_profile->definition.type,room->room_profile->region.id,{6,AccessPolicy::KnownCapabilities,true},true,true,true};
   if(wall_selection->policy_revision==CenteredWall::Revision||wall_selection->policy_revision==CenteredWall::ContactRevision){
    const auto*source=SelectedWallCache::Source(row.values.item_id);if(!source)return false;
    const char*configured=std::getenv("EQEMU_HOUSING_ORIGINAL_ASSETS");const std::string root=configured&&*configured?configured:"housing-original-meshes";
    std::ifstream input(root+"/"+std::string(source->mesh_sha256)+".mesh",std::ios::binary|std::ios::ate);if(!input)return false;auto length=input.tellg();if(length<=0||length>64*1024*1024)return false;std::vector<uint8_t>bytes(static_cast<size_t>(length));input.seekg(0);if(!input.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))return false;
    auto cache=SelectedWallCache::Prepared::Read(row.values.item_id,bytes);if(!cache)return false;
    if(wall_selection->policy_revision==CenteredWall::ContactRevision){
     const bool wall=bool(CenteredWall::CheckContact(*room->room_profile,*wall_selection,context,pose,PlacementPolicy::Operation::Add,flags,*cache,
      [&](const auto&start,const auto&end){return map->TracePlanarMount({start[0],start[1],start[2]},{end[0],end[1],end[2]});}
     ));
     return wall||base_free();
    }
    const bool wall=bool(CenteredWall::Check(*room->room_profile,*wall_selection,context,pose,PlacementPolicy::Operation::Add,flags,*cache,
     [&](const auto&start,const auto&end){return map->TracePlanarMount({start[0],start[1],start[2]},{end[0],end[1],end[2]});},
     [&](const auto&volume,const auto&seed,const auto&candidates){if(!wall_selection->source_policy||map->SourceTriangleCount()!=wall_selection->source_policy->planar_source_triangle_count)return std::vector<uint32_t>{};return map->SourcePlanarMountPatch(volume,seed,candidates);},
     [&](const auto&volume,const auto&triangles){return !map->IntersectsVolumeExceptTriangleList(volume,triangles);},0,
     [&](const auto&volume,const auto&seed,const auto&entries,const auto&limits){if(!wall_selection->source_policy||map->SourceTriangleCount()!=wall_selection->source_policy->planar_source_triangle_count)return CurvedMount::Proof{};return map->SourceCurvedMountPatch(volume,seed,entries,limits);}
    ));
    return wall||base_free();
   }
   const bool wall=bool(InteriorWallSlab::Check(*room->room_profile,*wall_selection,context,pose,PlacementPolicy::Operation::Add,flags,trace,[&](const auto& minimum,const auto& maximum){return !map->IntersectsBox({minimum[0],minimum[1],minimum[2]},{maximum[0],maximum[1],maximum[2]});},0,[&](const auto& minimum,const auto& maximum,const auto& triangles){return !map->IntersectsBoxExceptTriangles({minimum[0],minimum[1],minimum[2]},{maximum[0],maximum[1],maximum[2]},triangles[0],triangles[1]);},[&](const auto& minimum,const auto& maximum,uint32_t axis,const auto& seed,const auto& candidates){return map->SourceMountPatch({minimum[0],minimum[1],minimum[2]},{maximum[0],maximum[1],maximum[2]},axis,seed[0],seed[1],candidates);},[&](const auto& minimum,const auto& maximum,const auto& triangles){return !map->IntersectsBoxExceptTriangleList({minimum[0],minimum[1],minimum[2]},{maximum[0],maximum[1],maximum[2]},triangles);},[&](const auto& volume,uint32_t axis,const auto& seed,const auto& candidates){return map->SourceMountPatch(volume,axis,seed[0],seed[1],candidates);},[&](const auto& volume,const auto& triangles){return !map->IntersectsVolumeExceptTriangleList(volume,triangles);},[&](const auto& start,const auto& end){return map->TracePlanarMount({start[0],start[1],start[2]},{end[0],end[1],end[2]});},[&](const auto& volume,const auto& seed,const auto& candidates){
    // The fresh same-TX selection and independently hash-checked map must agree
    // on the complete source index space before a V5 exemption is considered.
    if(!wall_selection->source_policy||map->SourceTriangleCount()!=wall_selection->source_policy->planar_source_triangle_count)return std::vector<uint32_t>{};
    return map->SourcePlanarMountPatch(volume,seed,candidates);
   },[&](const auto& volume,const auto& seed,const auto& entries,const auto& limits){
    // New curved support retains only the reviewed zero-centre anchor profile.
    // Nonzero-centre Wall needs its separately recovered frame-aware successor.
    if(std::any_of(model->geometry.center.begin(),model->geometry.center.end(),[](double value){return value!=0;}))return CurvedMount::Proof{};
    if(!wall_selection->source_policy||map->SourceTriangleCount()!=wall_selection->source_policy->planar_source_triangle_count)return CurvedMount::Proof{};
    return map->SourceCurvedMountPatch(volume,seed,entries,limits);
   }));
   return wall||base_free();
  }
  // Interior Floor/Ceiling now require the complete original scene above.
  // The independent legacy Wall alternative remains available for its saved
  // predecessor profiles; it cannot silently retry approximate floor rays.
  if(room){if(model->mounting_type==2)return base_free();auto wall_flags=flags;wall_flags.known=true;wall_flags.disable_rotation=true;wall_flags.disable_free=true;return MountAdmission::Check(model->geometry,pose,wall_flags,trace)||base_free();}
  return MountAdmission::Check(model->geometry,pose,flags,trace);
 }catch(...){return false;}
}

    bool Bridge::ReturnAllowed(const LiveItems::InventoryRow& r)const noexcept{
        try{
            auto& inv=client_.GetInv();
            if(!held_||!SlotSupported(inv,r.values.slot_id)||inv.GetItem(int16_t(r.values.slot_id))||!CursorCompatible(inv,r.values.slot_id,false))return false;
            // Native lore restrictions apply to every physical member, including
            // socketed augments and bag children, and within this one bundle.
            std::set<std::pair<int32_t,uint32_t>> lore;
            const auto allowed=[&](auto&& self,EQ::ItemInstance& item)->bool{
                const auto* data=item.GetItem();if(!data||client_.CheckLoreConflict(data))return false;
                if(data->LoreFlag&&data->LoreGroup){const auto key=std::make_pair(data->LoreGroup,data->LoreGroup==-1?data->ID:0u);if(!lore.insert(key).second)return false;}
                for(const auto& [slot,child]:*item.GetContents())if(!child||!self(self,*child))return false;
                return true;
            };
            for(const auto& it:retained_)if(it.second&&Matches(r,*it.second))return allowed(allowed,*it.second);
            auto rebuilt=Reconstruct(database,r);
            return rebuilt && allowed(allowed,*rebuilt.value.item);
        }
        catch(...){
            return false;
        }
    }
    bool Bridge::Applied(const LiveItems::Receipt& r)const noexcept{
        try{
            if(!held_||!HouseMemory::MatchesParent(r)||!CrateMemory::MatchesParent(r))return false;
            if(r.house_effect_required&&!AppliedHouseEffect(*r.house_effect))return false;
            auto& inv=client_.GetInv();
            if(r.partition)return StackPartitionApplication::Applied(r,inv,retained_);
            if(r.action==LiveItems::Action::Store){
                auto i=retained_.find(r.stored.custody_id);
                return i!=retained_.end()&&i->second&&Matches(r.stored.row,*i->second)&&(!r.crate_allocation||CrateMemory::AfterMatches(r,*i->second))&&!inv.GetItem(int16_t(r.stored.row.values.slot_id));
            }
            auto item=inv.GetItem(int16_t(r.inventory_after.values.slot_id));
            return item&&Matches(r.inventory_after,*item)&&(!r.crate_allocation||CrateMemory::AfterMatches(r,*item))&&retained_.find(r.stored.custody_id)==retained_.end();
        }
        catch(...){
            return false;
        }
    }
    bool Bridge::AppliedHouseEffect(const HouseBinding::Effect& e)const noexcept{
        try{
            if(!HeldFor(actor_,lease_.epoch,session_key_)||!HouseMemory::Coherent(e))return false;
            auto& inv=client_.GetInv();
            if(e.action==LiveItems::Action::Store){
                const auto found=retained_.find(e.after.location.custody_id);
                return found!=retained_.end()&&found->second&&!inv.GetItem(int16_t(e.source.values.slot_id))&&
                    Matches(e.destination,*found->second)&&HouseMemory::AfterMatches(e,*found->second);
            }
            auto* item=inv.GetItem(int16_t(e.destination.values.slot_id));
            return item&&retained_.find(e.before.location.custody_id)==retained_.end()&&
                Matches(e.destination,*item)&&HouseMemory::AfterMatches(e,*item);
        }catch(...){return false;}
    }
    bool Bridge::Apply(const LiveItems::Receipt& r){
        if(!HouseMemory::MatchesParent(r)||!CrateMemory::MatchesParent(r))return false;
        if(Applied(r))return true;
        auto& inv=client_.GetInv();
        if(r.partition){if(r.action==LiveItems::Action::Return&&!StackPartitionApplication::Applied(r,inv,retained_)&&!ReturnAllowed(r.inventory_after))return false;return StackPartitionApplication::Apply(r,inv,retained_,partition_pieces_);}
        if(r.action==LiveItems::Action::Store){
            auto found=retained_.find(r.stored.custody_id);
            if(found==retained_.end())found=retained_.emplace(r.stored.custody_id,nullptr).first;
            auto source=inv.GetItem(int16_t(r.stored.row.values.slot_id));
            if(found->second||!source||!SourceMatches(r.stored.row,source->GetSerialNumber()))return false;
            found->second.reset(inv.PopItem(int16_t(r.stored.row.values.slot_id)));
            if(r.house_effect_required&&(!found->second||!HouseMemory::Apply(*r.house_effect,*found->second)))return false;
            if(r.crate_allocation&&(!found->second||!CrateMemory::Apply(r,*found->second)))return false;
            reconstructed_custody_.erase(r.stored.custody_id);
            return Applied(r);
        }
        auto found=retained_.find(r.stored.custody_id);
        if(found==retained_.end()||!found->second){
            auto rebuilt=Reconstruct(database,r.inventory_after);
            if(!rebuilt)return false;
            if(found==retained_.end())found=retained_.emplace(r.stored.custody_id,std::move(rebuilt.value.item)).first;
            else found->second=std::move(rebuilt.value.item);
            reconstructed_custody_.insert(r.stored.custody_id);
        }
        if(!InventoryProjection::RestoreEvolution(r.inventory_after,*found->second)||!Matches(r.inventory_after,*found->second)||!ReturnAllowed(r.inventory_after))return false;
        if(r.house_effect_required&&!HouseMemory::Apply(*r.house_effect,*found->second,reconstructed_custody_.count(r.stored.custody_id)!=0))return false;
        if(r.crate_allocation&&!CrateMemory::Apply(r,*found->second,reconstructed_custody_.count(r.stored.custody_id)!=0))return false;
        auto pointer=found->second.get();
        if(inv.PushItem(int16_t(r.inventory_after.values.slot_id),pointer)!=int16_t(r.inventory_after.values.slot_id))return false;
        found->second.release();
        retained_.erase(found);
        reconstructed_custody_.erase(r.stored.custody_id);
        return Applied(r);
    }
    [[noreturn]]void Bridge::FreezeInvariantFailure()const{
        for(;;)Wait();
    }
    bool Bridge::Open(){
        if(lease_.epoch)return true;
        bool uncertain=false;
        for(;;){
            try{
                auto service=Fresh();
                if(service){
                    auto opened=service->OpenLease(actor_,session_key_,*this);
                    if(opened){
                        lease_=opened.value;
                        if(!lease_.pending_key.empty())FreezeInvariantFailure();
                        return true;
                    }
                    if(opened.error==LiveItems::Error::CommitUncertain)uncertain=true;
                    else if(!uncertain)return false;
                }
            }
            catch(...){
                if(!uncertain)return false;
            }
            if(!uncertain)return false;
            Wait();
        }
    }
    LiveItems::Result<LiveItems::Receipt> Bridge::Execute(LiveItems::Request request){
        request.lease=lease_;
        std::unique_ptr<LiveItems::Service> service;
        LiveItems::Result<LiveItems::Receipt> result;
        bool uncertain=false;
        try{
            service=Fresh();
            if(!service)return {
                {
                },LiveItems::Error::DatabaseFailure
            };
            if(request.vault){result=service->OperateVault(request,*this,house_policies_.scene.get(),house_policies_.room.get());}else if(request.room){if(!house_policies_.scene||!house_policies_.room)return {{},LiveItems::Error::Unauthorized};result=service->OperateInterior(request,*this,*house_policies_.scene,*house_policies_.room);}else if(request.crate_member)result=service->OperateCrate(request,*this,house_policies_.scene.get());else result=service->Operate(request,*this);
            uncertain=result.error==LiveItems::Error::CommitUncertain||result.error==LiveItems::Error::DatabaseFailure;
        }
        catch(...){
            uncertain=true;
        }
        if(uncertain){
            service.reset();
            for(;;){
                try{
                    service=Fresh();
                    if(service){
                        result=service->Resolve(lease_,request.request_key,*this);
                        if(result)break;
                        if(result.error==LiveItems::Error::NotFound)return {
                            {
                            },LiveItems::Error::DatabaseFailure
                        };
                    }
                }
                catch(...){
                }
                service.reset();
                Wait();
            }
        }
        if(!result)return result;
        trophy_sources_changed_=true; // known committed custody mutation; flush only after Leave
        if(!result.value.requires_application)return result;
        // From known COMMIT onward no exception or invariant failure may release the
        // barrier. Keeping the process blocked is deliberate; never kick/save/refund.
        try{
            if(!Apply(result.value))FreezeInvariantFailure();
        }
        catch(...){
            FreezeInvariantFailure();
        }
        for(;;){
            try{
                if(!service)service=Fresh();
                if(service){
                    auto ack=service->AcknowledgeApplied(lease_,request.request_key,*this);
                    if(ack)return result;
                }
            }
            catch(...){
            }
            service.reset();
            Wait();
        }
    }
    namespace {
        std::unique_ptr<NativePlotPurchase::Service> FreshPurchase(){const auto*c=EQEmuConfig::get();if(!c)return {};auto d=std::make_unique<Database>();if(!d->Connect(c->DatabaseHost,c->DatabaseUsername,c->DatabasePassword,c->DatabaseDB,c->DatabasePort,"housing-native-plot-purchase"))return {};return std::make_unique<NativePlotPurchase::Service>(std::move(d),true);}
    }
    bool Bridge::GuildExpansionAllowed()const noexcept{
        return HeldFor(actor_,lease_.epoch,session_key_)&&client_.Connected()&&client_.ClientDataLoaded()&&(client_.GetPP().expansions&EQ::expansions::bitVoA)!=0;
    }
    bool Bridge::InspectedOfferMatches(const NativePlotPurchase::Offer&o)const noexcept{try{return purchase_inspection_&&HeldFor(actor_,lease_.epoch,session_key_)&&zone&&zone->GetZoneID()==712&&!zone->GetInstanceVersion()&&zone->GetInstanceID()==o.source.instance_id&&NativePlotPurchase::OfferBytes(*purchase_inspection_)==NativePlotPurchase::OfferBytes(o);}catch(...){return false;}}
    bool Bridge::PurchaseApplied(const NativePlotPurchase::Receipt&r)const noexcept{return HeldFor(r.request.lease.actor,r.request.lease.epoch,r.request.lease.session_key)&&r.request.lease.token==lease_.token&&CurrencyMatches(r.after)&&CursorItemsEmpty();}
    bool Bridge::ApplyPurchase(const NativePlotPurchase::Receipt&r){if(!r.requires_application||!HeldFor(r.request.lease.actor,r.request.lease.epoch,r.request.lease.session_key)||r.request.lease.token!=lease_.token||!CursorItemsEmpty())return false;auto expected=r.request.expected;if(r.charged_copper){auto plan=CoinWallet::DebitCarried(expected.carried,r.charged_copper);if(!plan)return false;expected.carried=plan.value.after;}if(!(expected==r.after))return false;if(CurrencyMatches(r.after))return true;if(!CurrencyMatches(r.request.expected))return false;auto&pp=client_.GetPP();pp.copper=r.after.carried.copper;pp.silver=r.after.carried.silver;pp.gold=r.after.carried.gold;pp.platinum=r.after.carried.platinum;return PurchaseApplied(r);}
    bool Bridge::RememberPurchaseInspection(uint32_t native,const Catalog::Export& published){purchase_inspection_.reset();if(!Enter())return false;try{if(!Open()||!zone||zone->GetZoneID()!=712||zone->GetInstanceVersion()){Leave();return false;}auto service=FreshPurchase();if(!service){Leave();return false;}auto offer=service->Inspect(lease_,native,{712,0,uint16_t(zone->GetInstanceID())},*this);if(offer&&NativePlotPurchase::MatchesPublished(offer.value,published)){purchase_inspection_=std::move(offer.value);Leave();return true;}Leave();return false;}catch(...){Leave();return false;}}
    NativePlotPurchase::Result<NativePlotPurchase::Receipt> Bridge::BuyPlot(const std::string&key,const std::array<uint8_t,EstateWire::PurchaseSize>&wire){using Error=NativePlotPurchase::Error;if(!Enter())return {{},Error::UnheldFence};bool issued=false;try{if(!Open()||!purchase_inspection_||!InspectedOfferMatches(*purchase_inspection_)){Leave();return {{},Error::Unauthorized};}NativePlotPurchase::Request request;request.request_key=key;request.lease=lease_;request.offer=*purchase_inspection_;request.wire=wire;request.expected=CoinProfile::Snapshot(client_.GetPP());auto service=FreshPurchase();if(!service){Leave();return {{},Error::DatabaseFailure};}issued=true;NativePlotPurchase::Result<NativePlotPurchase::Receipt> result;try{result=service->Buy(request,*this);}catch(...){result.error=Error::CommitUncertain;}bool uncertain=result.error==Error::CommitUncertain;if(uncertain){service.reset();for(;;){try{service=FreshPurchase();if(service){result=service->Resolve(request,*this);if(result)break;if(result.error==Error::NotFound){Leave();return {{},Error::DatabaseFailure};}}}catch(...){}service.reset();Wait();}}
        if(!result){purchase_inspection_.reset();Leave();return result;}if(result.value.requires_application){if(!ApplyPurchase(result.value))FreezeInvariantFailure();for(;;){try{if(!service)service=FreshPurchase();if(service&&service->AcknowledgeApplied(request,*this))break;}catch(...){}service.reset();Wait();}}purchase_inspection_.reset();Leave();return result;
      }catch(...){if(issued)FreezeInvariantFailure();Leave();return {{},Error::DatabaseFailure};}}
    namespace {
        std::unique_ptr<OwnershipTransfer::Service> FreshOwnershipTransfer(){const auto*c=EQEmuConfig::get();if(!c)return {};auto db=std::make_unique<Database>();if(!db->Connect(c->DatabaseHost,c->DatabaseUsername,c->DatabasePassword,c->DatabaseDB,c->DatabasePort,"housing-ownership-transfer"))return {};return std::make_unique<OwnershipTransfer::Service>(std::move(db),true,uint32_t(RuleI(Housing, ReviewedPersonalTransferLimit)));}
    }
    bool Bridge::TransferFrameMatches(const OwnershipTransfer::Request&r,const OwnershipTransfer::Source&s)const noexcept{
        try{if(!HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key)||r.lease.token!=lease_.token||!zone||zone->GetZoneID()!=712||zone->GetInstanceVersion()||zone->GetInstanceID()!=r.source.instance_id||!client_.Connected()||!client_.ClientDataLoaded()||s.route.instance_id!=r.source.instance_id||r.position!=std::array<float,3>{client_.GetX(),client_.GetY(),client_.GetZ()})return false;
        if(transfer_expected_source_){const auto&e=*transfer_expected_source_;if(e.estate_id!=s.estate_id||e.native_plot_id!=s.native_plot_id||e.acl_revision!=s.acl_revision||e.catalog_revision!=s.catalog_revision||e.catalog_key!=s.catalog_key||e.definitions_sha256!=s.definitions_sha256||e.regions_sha256!=s.regions_sha256||e.contents_sha256!=s.contents_sha256)return false;}
        auto*mob=client_.GetTarget();if(!mob||!mob->IsClient())return false;auto*target=mob->CastToClient();return target!=&client_&&target->Connected()&&target->ClientDataLoaded()&&target->HasInventorySessionReservation()&&target->GetID()==r.recipient.runtime_entity_id&&target->CharacterID()==r.recipient.actor.character_id&&target->AccountID()==r.recipient.actor.account_id&&std::string(target->GetName())==r.recipient.name;
        }catch(...){return false;}
    }
    bool Bridge::TransferContentsReviewedUnderTransaction(Database& db,const OwnershipTransfer::Request& r,const OwnershipTransfer::Source& source,LiveItems::InventorySessionFence& fence)const{
        try{
            if(!HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key)||!house_policies_.scene||!house_policies_.room)return false;
            auto yard=Scene::Service::ReadSceneUnderTransaction(db,r.lease,source.estate_id,fence,house_policies_.scene.get(),house_policies_.crates.get());
            if(!yard||yard.value.native_plot_id!=source.native_plot_id||yard.value.acl_revision!=source.acl_revision||yard.value.current_catalog_revision!=source.catalog_revision)return false;
            std::map<uint64_t,LiveItems::InventoryRow> seen;
            auto add=[&](const LiveItems::Stored& item){return item.estate_id==source.estate_id&&item.custody_id&&seen.emplace(item.custody_id,item.row).second;};
            std::set<uint64_t> houses;
            for(const auto& item:yard.value.items){if(!add(item.stored))return false;if(item.house&&!houses.insert(item.house->house_id).second)return false;}
            // Packed houses retain rooms: include them in the complete transfer
            // graph, using conservation reads that cannot admit or unpack them.
            for(const auto& item:yard.value.crate_contents){if(!add(item.stored))return false;if(item.house&&!houses.insert(item.house->house_id).second)return false;}
            for(const auto& item:yard.value.vault_items)if(!add(item.stored))return false;
            for(auto house:houses){
                auto room=Scene::Service::ReadAttachedInteriorUnderTransaction(db,r.lease,source.estate_id,house,fence,*house_policies_.scene,house_policies_.room.get(),true,house_policies_.crates.get(),Scene::AttachedRead::TransferConservation);
                if(!room||!room.value.house||room.value.house->house_id!=house||!room.value.room_profile)return false;
                for(const auto& item:room.value.contents)if(!add(item.stored))return false;
                for(const auto& item:room.value.crate_contents)if(!add(item.stored))return false;
                for(const auto& item:room.value.vault_items)if(!add(item.stored))return false;
            }
            auto rows=db.QueryDatabase("SELECT custody_id,owner_character_id,schema_descriptor,HEX(row_blob) FROM housing_live_item_custody WHERE estate_id="+std::to_string(source.estate_id)+" ORDER BY custody_id FOR UPDATE",false);
            if(!rows.Success()||rows.RowCount()!=seen.size())return false;
            for(auto row=rows.begin();row!=rows.end();++row){
                if(!row[0]||!row[1]||!row[2]||!row[3]||std::string(row[2])!=LiveItems::Service::SchemaDescriptor())return false;
                auto item=seen.find(std::stoull(row[0]));if(item==seen.end()||item->second.values.character_id!=std::stoul(row[1]))return false;
                const auto raw=CustodyCodec::Encode(item->second);static constexpr char hex[]="0123456789ABCDEF";std::string encoded;for(auto b:raw){encoded+=hex[b>>4];encoded+=hex[b&15];}if(encoded!=row[3])return false;
                seen.erase(item);
            }
            return seen.empty();
        }catch(...){return false;}
    }
    OwnershipTransfer::Result<OwnershipTransfer::Receipt> Bridge::InspectPersonalPlotTransfer(const std::string&key,const std::array<uint8_t,EstateWire::PurchaseSize>&wire){return ExecutePersonalPlotTransfer(key,wire,true,nullptr);}
    OwnershipTransfer::Result<OwnershipTransfer::Receipt> Bridge::TransferPersonalPlot(const std::string&key,const std::array<uint8_t,EstateWire::PurchaseSize>&wire,const OwnershipTransfer::Source&expected){return ExecutePersonalPlotTransfer(key,wire,false,&expected);}
    OwnershipTransfer::Result<OwnershipTransfer::Receipt> Bridge::ExecutePersonalPlotTransfer(const std::string&key,const std::array<uint8_t,EstateWire::PurchaseSize>&wire,bool inspect,const OwnershipTransfer::Source*expected){
    struct ExpectedScope{const OwnershipTransfer::Source*&slot;ExpectedScope(const OwnershipTransfer::Source*&s,const OwnershipTransfer::Source*e):slot(s){slot=e;}~ExpectedScope(){slot=nullptr;}} scope(transfer_expected_source_,expected);
    using Error=OwnershipTransfer::Error;if(!Enter())return {{},Error::UnheldFence};bool issued=false;try{if(!Open()||!zone||zone->GetZoneID()!=712||zone->GetInstanceVersion()){Leave();return {{},Error::Unauthorized};}auto*mob=client_.GetTarget();if(!mob||!mob->IsClient()){Leave();return {{},Error::Unauthorized};}auto*target=mob->CastToClient();OwnershipTransfer::Request request;request.key=key;request.lease=lease_;request.wire=wire;request.source={712,0,uint16_t(zone->GetInstanceID())};request.position={client_.GetX(),client_.GetY(),client_.GetZ()};request.recipient={{target->CharacterID(),target->AccountID()},target->GetID(),target->GetName()};auto service=FreshOwnershipTransfer();if(!service){Leave();return {{},Error::DatabaseFailure};}issued=true;OwnershipTransfer::Result<OwnershipTransfer::Receipt> result;try{result=inspect?service->Inspect(request,*this,*this):service->Transfer(request,*this,*this);}catch(...){result.error=Error::CommitUncertain;}
        if(result.error==Error::CommitUncertain){service.reset();for(;;){try{service=FreshOwnershipTransfer();if(service){result=service->Resolve(request,*this);if(result)break;if(result.error==Error::NotFound){Leave();return {{},Error::DatabaseFailure};}}}catch(...){}service.reset();Wait();}}
        // Metadata is already authoritative SQL. No item/coin memory application
        // or invented507f acknowledgement is needed; fresh views follow release.
        Leave();return result;
        }catch(...){if(issued)FreezeInvariantFailure();Leave();return {{},Error::DatabaseFailure};}}
    bool Bridge::CurrencyMatches(const LiveEscrow::Currency& currency)const noexcept{
        return HeldFor(actor_,lease_.epoch,session_key_)&&CoinProfile::Snapshot(client_.GetPP())==currency;
    }
    bool Bridge::CursorItemsEmpty()const noexcept{
        return HeldFor(actor_,lease_.epoch,session_key_)&&CoinProfile::CursorItemsEmpty(client_.GetInv());
    }
    bool Bridge::CoinApplied(const LiveEscrow::Receipt& receipt)const noexcept{
        return receipt.lease.actor.character_id==actor_.character_id
            &&receipt.lease.actor.account_id==actor_.account_id
            &&receipt.lease.epoch==lease_.epoch&&receipt.lease.token==lease_.token
            &&receipt.lease.session_key==session_key_&&CurrencyMatches(receipt.after)&&CursorItemsEmpty();
    }
    bool Bridge::ApplyCoin(const LiveEscrow::Receipt& receipt){
        if(!HeldFor(receipt.lease.actor,receipt.lease.epoch,receipt.lease.session_key)
            ||receipt.lease.token!=lease_.token||!CursorItemsEmpty())return false;
        return CoinProfile::ApplyPending(client_.GetPP(),receipt)&&CoinApplied(receipt);
    }
    namespace {
        std::unique_ptr<LiveEscrow::Service> FreshCoin(const Scene::HouseScenePolicy* houses){
            const auto* config=EQEmuConfig::get();if(!config)return nullptr;
            auto db=std::make_unique<Database>();
            if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,
                config->DatabaseDB,config->DatabasePort,"housing-live-coin"))return nullptr;
            return std::make_unique<LiveEscrow::Service>(std::move(db),true,houses);
        }
    }
    LiveEscrow::Result<LiveEscrow::Receipt> Bridge::ExecuteCoin(LiveEscrow::Request request){
        std::unique_ptr<LiveEscrow::Service> service;
        LiveEscrow::Result<LiveEscrow::Receipt> result;
        bool uncertain=false;
        try{
            service=FreshCoin(house_policies_.scene.get());if(!service)return {{},LiveEscrow::Error::DatabaseFailure};
            result=service->Transfer(request,*this);
            uncertain=result.error==LiveEscrow::Error::CommitUncertain||result.error==LiveEscrow::Error::DatabaseFailure;
        }catch(...){uncertain=true;}
        if(uncertain){
            service.reset();
            for(;;){
                try{
                    service=FreshCoin(house_policies_.scene.get());if(service){
                        result=service->Resolve(request,*this);
                        if(result)break;
                        if(result.error==LiveEscrow::Error::NotFound)return {{},LiveEscrow::Error::DatabaseFailure};
                    }
                }catch(...){}
                service.reset();Wait();
            }
        }
        if(!result||!result.value.requires_application)return result;
        // From known COMMIT onward hold the existing simulation barrier through
        // profile application and durable ACK. Never send, save, kick or refund.
        try{if(!ApplyCoin(result.value))FreezeInvariantFailure();}
        catch(...){FreezeInvariantFailure();}
        for(;;){
            try{
                if(!service)service=FreshCoin(house_policies_.scene.get());
                if(service&&service->AcknowledgeApplied(request,*this))return result;
            }catch(...){}
            service.reset();Wait();
        }
    }
    LiveEscrow::Result<LiveEscrow::Receipt> Bridge::TransferCoin(const std::string& key,
        EstateId estate,const EscrowWire::Message& wire,bool enabled){
        if(!enabled)return {{},LiveEscrow::Error::Disabled};
        if(!Enter())return {{},LiveEscrow::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveEscrow::Error::StaleLease};}
            LiveEscrow::Request request;request.request_key=key;request.lease=lease_;
            request.estate_id=estate;request.wire=wire;request.expected=CoinProfile::Snapshot(client_.GetPP());
            if(wire.action==3||wire.action==4||wire.action==7)request.funding=LiveEscrow::Funding::Carried;
            if(zone&&zone->GetZoneID()!=712){
                // Capture only from the actual occupied simulation, never the
                // native body. The coin TX revalidates this exact placed host.
                const auto* config=EQEmuConfig::get();auto db=std::make_unique<Database>();
                if(!config||!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-interior-coin-source")){Leave();return {{},LiveEscrow::Error::DatabaseFailure};}
                Scene::Service source(std::move(db),true,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get());
                Route physical{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
                auto current=source.ReadInteriorForManagement(lease_,physical,*this);
                if(!current||!current.value.house||!current.value.house->interior||current.value.context.estate_id!=estate||current.value.context.native_plot_id!=wire.estate_id){Leave();return {{},LiveEscrow::Error::Unauthorized};}
                request.interior_source=LiveEscrow::InteriorSource{current.value.house->house_id,current.value.house->revision,current.value.house->interior->native_estate_id,physical};
            }
            auto result=ExecuteCoin(std::move(request));Leave();return result;
        }catch(...){Leave();return {{},LiveEscrow::Error::DatabaseFailure};}
    }
    LiveItems::Result<LayoutInventoryJournal::Receipt> Bridge::LoadLayoutInventory(const std::string& key,const std::vector<uint8_t>& wire,const std::vector<LiveItems::LayoutUnitDestination>& targets,bool enabled){
        using Error=LiveItems::Error;if(!enabled)return {{},Error::Disabled};if(client_.ClientVersion()!=EQ::versions::ClientVersion::RoF2)return {{},Error::UnsupportedItem};if(!Enter())return {{},Error::UnheldFence};
        auto connect=[]()->std::unique_ptr<Database>{const auto* c=EQEmuConfig::get();if(!c)return {};auto db=std::make_unique<Database>();if(!db->Connect(c->DatabaseHost,c->DatabaseUsername,c->DatabasePassword,c->DatabaseDB,c->DatabasePort,"housing-layout-exclusive"))return {};return db;};
        std::unique_ptr<Database> db;LayoutInventoryJournal::Binding binding;LayoutInventoryCoordinator::Staged staged;bool tx=false,attempted_commit=false,known_commit=false;
        try{
            if(!Open()){Leave();return {{},Error::StaleLease};}binding=LayoutInventoryCoordinator::Bind(key,lease_,wire,targets);db=connect();if(!db){Leave();return {{},Error::DatabaseFailure};}
            LayoutInventoryJournal::Detail::Q(*db,"START TRANSACTION");tx=true;auto entry=LiveItems::Service::AuthenticateUnderTransaction(*db,true,lease_,*this);if(!entry){LayoutInventoryJournal::Detail::Q(*db,"ROLLBACK");tx=false;Leave();return {{},entry.error};}if(!entry.value.pending_key.empty())FreezeInvariantFailure();auto result=LayoutInventoryCoordinator::StageUnderTransaction(*db,true,binding,targets,client_.GetInv(),*this,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get());
            if(!result){LayoutInventoryJournal::Detail::Q(*db,"ROLLBACK");tx=false;Leave();return {{},result.error};}staged=std::move(result.value);
            // An unapplied prior batch cannot lose its in-process ownership
            // state: synchronous callers retain the barrier until known ACK.
            if(staged.receipt.replayed&&!staged.receipt.applied)FreezeInvariantFailure();
            attempted_commit=true;LayoutInventoryJournal::Detail::Q(*db,"COMMIT");tx=false;known_commit=true;
        }catch(const std::invalid_argument&){if(attempted_commit)FreezeInvariantFailure();if(tx&&db)db->QueryDatabase(std::string("ROLLBACK"),false);Leave();return {{},Error::InvalidArgument};}
        catch(...){if(!attempted_commit){if(tx&&db)db->QueryDatabase(std::string("ROLLBACK"),false);Leave();return {{},Error::DatabaseFailure};}}
        if(!known_commit){
            // Discard the uncertain connection before observing the same key.
            db.reset();for(;;){try{db=connect();if(db){LayoutInventoryJournal::Detail::Q(*db,"START TRANSACTION");auto found=LayoutInventoryJournal::ReadUnderTransaction(*db,true,binding,*this);LayoutInventoryJournal::Detail::Q(*db,"ROLLBACK");if(found){staged.receipt=std::move(found.value);known_commit=true;break;}if(found.error==Error::NotFound){Leave();return {{},Error::DatabaseFailure};}if(found.error!=Error::DatabaseFailure)FreezeInvariantFailure();}}catch(...){}db.reset();Wait();}
        }
        const bool historical=staged.receipt.applied;LayoutInventoryApplication::State application;
        if(!historical){
            for(;;){try{db=connect();if(db){auto applied=LayoutInventoryApplication::ApplyCommitted(*db,true,binding,staged.plan,staged.prepared,application,client_.GetInv(),*this,retained_);if(applied){staged.receipt=std::move(applied.value);break;}if(applied.error!=Error::DatabaseFailure&&applied.error!=Error::CommitUncertain)FreezeInvariantFailure();}}catch(...){FreezeInvariantFailure();}db.reset();Wait();}
            for(const auto& piece:staged.receipt.outcome.pieces)reconstructed_custody_.erase(piece.placement.stored.custody_id);
        }
        lease_.pending_key.clear();staged.receipt.replayed=historical;trophy_sources_changed_=!historical;Leave();
        // All durable/memory work is ACKed and the barrier is released before
        // queuing native packets. These sends never save or debit inventory.
        if(!historical){try{for(const auto& source:staged.receipt.outcome.inventory.changes){const auto slot=int16_t(source.before.values.slot_id);if(source.after){auto* item=client_.GetInv().GetItem(slot);if(!item)FreezeInvariantFailure();client_.SendItemPacket(slot,item,ItemPacketTrade);}else{EQApplicationPacket packet(OP_MoveItem,sizeof(MoveItem_Struct));auto* deleted=reinterpret_cast<MoveItem_Struct*>(packet.pBuffer);deleted->from_slot=source.before.values.slot_id;deleted->to_slot=UINT32_MAX;deleted->number_in_stack=UINT32_MAX;client_.QueuePacket(&packet);}}}catch(...){FreezeInvariantFailure();}}
        return {std::move(staged.receipt),Error::None};
    }
    LiveItems::Result<LayoutMixedJournal::Receipt> Bridge::LoadLayoutMixed(const std::string& key,const std::vector<uint8_t>& wire,const LayoutMixedCoordinator::Intent& intent,const std::map<uint64_t,std::unique_ptr<EQ::ItemInstance>*>& owners,bool enabled){
        using Error=LiveItems::Error;if(!enabled)return {{},Error::Disabled};if(client_.ClientVersion()!=EQ::versions::ClientVersion::RoF2)return {{},Error::UnsupportedItem};if(!Enter())return {{},Error::UnheldFence};
        auto connect=[]()->std::unique_ptr<Database>{const auto* c=EQEmuConfig::get();if(!c)return {};auto db=std::make_unique<Database>();if(!db->Connect(c->DatabaseHost,c->DatabaseUsername,c->DatabasePassword,c->DatabaseDB,c->DatabasePort,"housing-layout-exclusive"))return {};return db;};
        std::unique_ptr<Database> db;LayoutMixedJournal::Binding binding;LayoutMixedCoordinator::Staged staged;bool tx=false,attempted_commit=false,known_commit=false;
        try{
            if(!Open()){Leave();return {{},Error::StaleLease};}binding=LayoutMixedCoordinator::Bind(key,lease_,wire,intent);db=connect();if(!db){Leave();return {{},Error::DatabaseFailure};}
            LayoutMixedJournal::Detail::Q(*db,"START TRANSACTION");tx=true;auto entry=LiveItems::Service::AuthenticateUnderTransaction(*db,true,lease_,*this);if(!entry){LayoutMixedJournal::Detail::Q(*db,"ROLLBACK");tx=false;Leave();return {{},entry.error};}if(!entry.value.pending_key.empty())FreezeInvariantFailure();auto result=LayoutMixedCoordinator::StageUnderTransaction(*db,true,binding,intent,client_.GetInv(),*this,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get(),house_policies_.custody.get());
            if(!result){LayoutMixedJournal::Detail::Q(*db,"ROLLBACK");tx=false;Leave();return {{},result.error};}staged=std::move(result.value);
            // An unapplied prior batch cannot lose its in-process ownership
            // state: synchronous callers retain the barrier until known ACK.
            if(staged.receipt.replayed&&!staged.receipt.applied)FreezeInvariantFailure();
            // A different viewer may have changed a retained passive cache's
            // count. The coordinator just proved these exact initial rows under
            // the transaction locks. Refresh count-only copies before ownership
            // review; scene/inventory debit witnesses remain exact preimages.
            bool cache_current=true;
            if(!staged.receipt.replayed)for(const auto& [id,fact]:LayoutMixedApplication::Detail::Fold(staged.receipt.outcome))if(!fact.created){auto cached=retained_.find(id);if(cached!=retained_.end()&&cached->second&&!SceneProjection::SynchronizeCharges(fact.initial,*cached->second))cache_current=false;}
            if(!staged.receipt.replayed&&(!cache_current||!LayoutMixedApplication::Detail::Review(staged.receipt.outcome,staged.property,owners,retained_))){LayoutMixedJournal::Detail::Q(*db,"ROLLBACK");tx=false;Leave();return {{},Error::Conflict};}
            attempted_commit=true;LayoutMixedJournal::Detail::Q(*db,"COMMIT");tx=false;known_commit=true;
        }catch(const std::invalid_argument&){if(attempted_commit)FreezeInvariantFailure();if(tx&&db)db->QueryDatabase(std::string("ROLLBACK"),false);Leave();return {{},Error::InvalidArgument};}
        catch(...){if(!attempted_commit){if(tx&&db)db->QueryDatabase(std::string("ROLLBACK"),false);Leave();return {{},Error::DatabaseFailure};}}
        if(!known_commit){
            // Discard the uncertain connection before observing the same key.
            db.reset();for(;;){try{db=connect();if(db){LayoutMixedJournal::Detail::Q(*db,"START TRANSACTION");auto found=LayoutMixedJournal::ReadUnderTransaction(*db,true,binding,*this);LayoutMixedJournal::Detail::Q(*db,"ROLLBACK");if(found){staged.receipt=std::move(found.value);known_commit=true;break;}if(found.error==Error::NotFound){Leave();return {{},Error::DatabaseFailure};}if(found.error!=Error::DatabaseFailure)FreezeInvariantFailure();}}catch(...){}db.reset();Wait();}
        }
        const bool historical=staged.receipt.applied;LayoutMixedApplication::State application;
        if(!historical){
            for(;;){try{db=connect();if(db){auto applied=LayoutMixedApplication::ApplyCommitted(*db,true,binding,staged.plan,staged.prepared,staged.property,application,client_.GetInv(),*this,owners,retained_);if(applied){staged.receipt=std::move(applied.value);break;}if(applied.error!=Error::DatabaseFailure&&applied.error!=Error::CommitUncertain)FreezeInvariantFailure();}}catch(...){FreezeInvariantFailure();}db.reset();Wait();}
            for(const auto& token:staged.receipt.outcome.retained_tokens)reconstructed_custody_.erase(token.after.placement.stored.custody_id);
            if(staged.receipt.outcome.inventory)for(const auto& piece:staged.receipt.outcome.inventory->pieces)reconstructed_custody_.erase(piece.placement.stored.custody_id);
            if(staged.receipt.outcome.property)for(const auto& step:staged.receipt.outcome.property->steps)for(const auto& delta:step.changes)reconstructed_custody_.erase((delta.before?delta.before:delta.after)->placement.stored.custody_id);
        }
        lease_.pending_key.clear();staged.receipt.replayed=historical;trophy_sources_changed_=!historical;Leave();
        // All durable/memory work is ACKed and the barrier is released before
        // queuing native packets. These sends never save or debit inventory.
        if(!historical&&staged.receipt.outcome.inventory){try{for(const auto& source:staged.receipt.outcome.inventory->inventory.changes){const auto slot=int16_t(source.before.values.slot_id);if(source.after){auto* item=client_.GetInv().GetItem(slot);if(!item)FreezeInvariantFailure();client_.SendItemPacket(slot,item,ItemPacketTrade);}else{EQApplicationPacket packet(OP_MoveItem,sizeof(MoveItem_Struct));auto* deleted=reinterpret_cast<MoveItem_Struct*>(packet.pBuffer);deleted->from_slot=source.before.values.slot_id;deleted->to_slot=UINT32_MAX;deleted->number_in_stack=UINT32_MAX;client_.QueuePacket(&packet);}}}catch(...){FreezeInvariantFailure();}}
        if(!historical){try{for(const auto& token:staged.receipt.outcome.retained_tokens){EQApplicationPacket packet(OP_MoveItem,sizeof(MoveItem_Struct));auto* deleted=reinterpret_cast<MoveItem_Struct*>(packet.pBuffer);deleted->from_slot=token.source.values.slot_id;deleted->to_slot=UINT32_MAX;deleted->number_in_stack=UINT32_MAX;client_.QueuePacket(&packet);}}catch(...){FreezeInvariantFailure();}}
        return {std::move(staged.receipt),Error::None};
    }
    LiveItems::Result<LiveItems::Receipt> Bridge::Store(const std::string& key,EstateId estate,const LiveItems::InventoryRow& row,std::optional<LiveItems::Placed> placement,std::optional<LiveItems::HouseIdentity> house,std::optional<LiveItems::InteriorReservation> room,bool initial_stowed,std::optional<Route> room_exterior_source,std::optional<LiveItems::CrateIdentity> crate,std::optional<LiveItems::CrateMemberContext> crate_member,std::optional<LiveItems::VaultContext> vault,uint32_t quantity){
        if(!Enter())return {
            {
            },LiveItems::Error::UnheldFence
        };
        LiveItems::Result<LiveItems::Receipt> out;
        try{
            if(!Open()){
                Leave();
                return {
                    {
                    },LiveItems::Error::StaleLease
                };
            }
            auto service=Fresh();
            if(!service){
                Leave();
                return {
                    {
                    },LiveItems::Error::DatabaseFailure
                };
            }
            auto prior=service->Resolve(lease_,key,*this);
            auto item=client_.GetInv().GetItem(int16_t(row.values.slot_id));
            if(!prior&&prior.error!=LiveItems::Error::NotFound){
                Leave();
                return prior;
            }
            if(!prior&&(!item||!SourceMatches(row,item->GetSerialNumber()))){
                Leave();
                return {
                    {
                    },LiveItems::Error::UnsupportedItem
                };
            }
            if(!prior&&((house&&house->house_id!=item->GetHousingHouseID())||
                (house&&!house->house_id&&item->GetHousingEstateID()!=UINT32_MAX)||
                (house&&item->GetHousingCrateID())||
                (crate&&(house||item->GetHousingHouseID()||item->GetHousingCrateID()!=crate->crate_id||item->GetHousingEstateID()==UINT32_MAX))||
                (!house&&!crate&&(item->GetHousingHouseID()||item->GetHousingCrateID()||item->GetHousingEstateID()!=UINT32_MAX)))){
                Leave();return {{},LiveItems::Error::Conflict};
            }
            LiveItems::Request r;
            r.request_key=key;
            r.estate_id=estate;
            r.expected=row;
            r.placement=std::move(placement);
            r.house=house;r.room=room;r.initial_stowed=initial_stowed;r.room_exterior_source=room_exterior_source;r.crate=crate;r.crate_member=crate_member;r.vault=vault;
            if(vault&&(!zone||vault->source.zone_id!=zone->GetZoneID()||vault->source.zone_version!=zone->GetInstanceVersion()||vault->source.instance_id!=zone->GetInstanceID())){Leave();return {{},LiveItems::Error::Conflict};}
            r.verified_runtime_serial=item?item->GetSerialNumber():0;
            if(!item&&prior){auto retained=retained_.find(prior.value.stored.custody_id);if(retained!=retained_.end()&&retained->second)r.verified_runtime_serial=retained->second->GetSerialNumber();}
            if(prior&&prior.value.partition){r.partition_quantity=prior.value.partition->quantity;r.partition_serial=prior.value.partition->serial;if(quantity!=r.partition_quantity){Leave();return {{},LiveItems::Error::Conflict};}}
            else if(!prior&&item){r.partition_quantity=StackPartitionApplication::Requested(*item,quantity);if(r.partition_quantity){auto piece=StackPartitionApplication::Prepare(row,*item,r.partition_quantity);if(!piece){Leave();return {{},LiveItems::Error::UnsupportedItem};}r.partition_serial=piece->GetSerialNumber();partition_pieces_[key]=std::move(piece);}}
            out=Execute(r);
        }
        catch(...){
            Leave();
            return {
                {
                },LiveItems::Error::DatabaseFailure
            };
        }
        Leave();
        return out;
    }
    LiveItems::Result<LiveItems::Receipt> Bridge::ReturnVault(const std::string& key,const VaultStorage::Item& cached,const LiveItems::VaultContext& context,uint32_t destination,uint32_t quantity){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()||!zone||context.source.zone_id!=zone->GetZoneID()||context.source.zone_version!=zone->GetInstanceVersion()||context.source.instance_id!=zone->GetInstanceID()){Leave();return {{},LiveItems::Error::StaleLease};}
            auto service=Fresh();if(!service){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            auto prior=service->Resolve(lease_,key,*this);if(!prior&&prior.error!=LiveItems::Error::NotFound){Leave();return prior;}
            const auto& row=prior?prior.value.stored.row:cached.stored.row;const auto id=prior?prior.value.stored.custody_id:cached.stored.custody_id;
            if(row.values.character_id!=actor_.character_id){Leave();return {{},LiveItems::Error::Unauthorized};}
            if(!prior||prior.value.requires_application){auto local=retained_.find(id);if(local==retained_.end()||!local->second){auto rebuilt=Reconstruct(database,row);if(!rebuilt){Leave();return {{},rebuilt.error};}if(local==retained_.end())local=retained_.emplace(id,std::move(rebuilt.value.item)).first;else local->second=std::move(rebuilt.value.item);reconstructed_custody_.insert(id);}if(!Matches(row,*local->second)){Leave();return {{},LiveItems::Error::Conflict};}}
            LiveItems::Request request;request.request_key=key;request.action=LiveItems::Action::Return;request.estate_id=cached.stored.estate_id;request.custody_id=cached.stored.custody_id;request.destination_slot=destination;request.expected=row;request.vault=context;
            if(prior&&prior.value.partition){request.partition_quantity=prior.value.partition->quantity;request.partition_serial=prior.value.partition->serial;if(quantity!=request.partition_quantity){Leave();return {{},LiveItems::Error::Conflict};}}
            else if(!prior){auto source=retained_.find(id);if(source==retained_.end()||!source->second){Leave();return {{},LiveItems::Error::Conflict};}request.partition_quantity=StackPartitionApplication::Requested(*source->second,quantity);if(request.partition_quantity){if(row.values.character_id!=actor_.character_id){Leave();return {{},LiveItems::Error::UnsupportedItem};}auto piece=StackPartitionApplication::Prepare(row,*source->second,request.partition_quantity);if(!piece){Leave();return {{},LiveItems::Error::UnsupportedItem};}request.partition_serial=piece->GetSerialNumber();partition_pieces_[key]=std::move(piece);}}
            auto result=Execute(request);Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<LiveItems::VaultTransferReceipt> Bridge::TransferVault(LiveItems::VaultTransferRequest& request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        if(!zone||request.from.source.zone_id!=zone->GetZoneID()||request.from.source.zone_version!=zone->GetInstanceVersion()||request.from.source.instance_id!=zone->GetInstanceID()){Leave();return {{},LiveItems::Error::Conflict};}
        try{if(request.partition_quantity){
            auto source=retained_.find(request.custody_id);if(source==retained_.end()||!source->second){auto rebuilt=Reconstruct(database,request.expected);if(!rebuilt){Leave();return {{},rebuilt.error};}retained_[request.custody_id]=std::move(rebuilt.value.item);source=retained_.find(request.custody_id);}
            auto after=request.expected;if(!after.values.charges||request.partition_quantity>=after.values.charges){Leave();return {{},LiveItems::Error::Conflict};}after.values.charges-=request.partition_quantity;after.null_fields&=~LiveItems::NullCharges;
            if(request.expected.values.character_id!=actor_.character_id||(!Matches(request.expected,*source->second)&&!(request.partition_serial&&Matches(after,*source->second)))){Leave();return {{},LiveItems::Error::Conflict};}
            auto piece=partition_pieces_.find(request.request_key);
            if(!request.partition_serial){if(StackPartitionApplication::Requested(*source->second,request.partition_quantity)!=request.partition_quantity){Leave();return {{},LiveItems::Error::Conflict};}if(piece==partition_pieces_.end()){auto unit=StackPartitionApplication::Prepare(request.expected,*source->second,request.partition_quantity);if(!unit){Leave();return {{},LiveItems::Error::UnsupportedItem};}piece=partition_pieces_.emplace(request.request_key,std::move(unit)).first;}if(!piece->second){Leave();return {{},LiveItems::Error::Conflict};}request.partition_serial=piece->second->GetSerialNumber();}
            else if(piece!=partition_pieces_.end()){if(!piece->second||uint32_t(piece->second->GetSerialNumber())!=request.partition_serial){Leave();return {{},LiveItems::Error::Conflict};}}
            else if(!Matches(after,*source->second)){Leave();return {{},LiveItems::Error::UnheldFence};}
        }}catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
        bool uncertain=false;
        for(;;){try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}request.lease=lease_;auto service=Fresh();
            if(service){auto result=service->TransferVault(request,*this,house_policies_.scene.get(),house_policies_.room.get());if(result){if(result.value.partition){if(!VaultPartitionApplication::Apply(request,result.value,retained_,partition_pieces_)){Leave();return {{},LiveItems::Error::Conflict};}}Leave();return result;}if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;else if(!uncertain||result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}}
            else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
        }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}Wait();}
    }
    LiveItems::Result<LiveItems::Reservation> Bridge::ReservePlacement(LiveItems::PreviewRequest request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        bool uncertain=false;
        for(;;){
            try{
                if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
                request.lease=lease_;
                auto service=Fresh();
                if(service){
                    auto result=service->ReservePlacement(request,*this);
                    if(result){Leave();return result;}
                    if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                    else if(!uncertain){Leave();return result;}
                    // Repeating this exact immutable key resolves an uncertain
                    // allocation without touching inventory or allocating twice.
                    else if(result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
                }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}
            Wait();
        }
    }
    LiveItems::Result<LiveItems::Reservation> Bridge::ReserveCratePlacement(LiveItems::PreviewRequest request,const LiveItems::CrateMemberContext& destination){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        if(!zone||destination.source.zone_id!=zone->GetZoneID()||destination.source.zone_version!=zone->GetInstanceVersion()||destination.source.instance_id!=zone->GetInstanceID()){Leave();return {{},LiveItems::Error::Conflict};}
        bool uncertain=false;
        for(;;){
            try{
                if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
                request.lease=lease_;
                auto service=Fresh();
                if(service){
                    auto result=service->ReserveCratePlacement(request,destination,*this,house_policies_.scene.get());
                    if(result){Leave();return result;}
                    if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                    else if(!uncertain){Leave();return result;}
                    // Repeating this exact immutable key resolves an uncertain
                    // allocation without touching inventory or allocating twice.
                    else if(result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
                }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}
            Wait();
        }
    }
    LiveItems::Result<LiveItems::InteriorReservation> Bridge::ReserveInteriorPlacement(LiveItems::InteriorPreviewRequest request,std::optional<LiveItems::CrateMemberContext> destination){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        if(!house_policies_.scene||!house_policies_.room||!zone){Leave();return {{},LiveItems::Error::Unauthorized};}
        const Route physical{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};if(request.exterior_source)request.exterior_source=physical;else request.current_source=physical;
        bool uncertain=false;
        for(;;){
            try{
                if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
                request.lease=lease_;
                auto service=Fresh();
                if(service){
                    auto result=service->ReserveInteriorPlacement(request,*this,*house_policies_.scene,*house_policies_.room,destination?&*destination:nullptr);
                    if(result){Leave();return result;}
                    if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                    else if(!uncertain){Leave();return result;}
                    // Repeating this exact immutable key resolves an uncertain
                    // allocation without touching inventory or allocating twice.
                    else if(result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
                }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}
            Wait();
        }
    }
    LiveItems::Result<bool> Bridge::CancelPlacement(uint32_t id){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        bool uncertain=false;
        for(;;){
            try{
                if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
                auto service=Fresh();
                if(service){
                    auto result=service->CancelPlacement(lease_,id,*this);
                    if(result){Leave();return result;}
                    if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                    else if(!uncertain || result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
                }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}
            Wait();
        }
    }
    LiveItems::Result<std::vector<LiveItems::Stored>> Bridge::ReadPlaced(EstateId estate){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            auto service=Fresh();
            if(!service){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            auto result=service->ReadPlaced(lease_,estate,*this);
            Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<HouseSwap::Receipt> Bridge::SwapHouse(const std::string& key,EstateId estate,const std::array<uint8_t,24>& wire){
        using Error=LiveItems::Error;if(!RuleB(Housing,EnableNativeHouseSwap))return {{},Error::Disabled};
        if(!Enter())return {{},Error::UnheldFence};
        if(!zone||zone->GetZoneID()!=712||zone->GetInstanceVersion()||!house_policies_.scene||!house_policies_.room){Leave();return {{},Error::Unauthorized};}
        HouseSwap::Request request;request.key=key;request.estate=estate;request.wire=wire;bool uncertain=false;
        for(;;){try{
            if(!Open()){Leave();return {{},Error::StaleLease};}request.lease=lease_;
            const auto* config=EQEmuConfig::get();auto db=std::make_unique<Database>();
            if(config&&db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-house-swap-exclusive")){
                HouseSwap::Service service(std::move(db),{true,true});auto result=uncertain?service.Resolve(request,*this):service.Swap(request,*this,*house_policies_.scene,*house_policies_.room,house_policies_.crates.get());
                if(result){trophy_sources_changed_=true;Leave();return result;}
                if(result.error==Error::CommitUncertain)uncertain=true;
                else if(!uncertain||result.error==Error::NotFound){Leave();return result;}
            }else if(!uncertain){Leave();return {{},Error::DatabaseFailure};}
        }catch(...){uncertain=true;}Wait();}
    }
    LiveItems::Result<InteriorStorage::Receipt> Bridge::ChangeStorage(InteriorStorage::Request request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        if(!zone||((!request.exterior||request.parent_yard)&&(!house_policies_.scene||!house_policies_.room))){Leave();return {{},LiveItems::Error::Unauthorized};}
        const Route source{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
        const auto requested_source=request.parent_yard?request.parent_yard->source:request.exterior?request.exterior->source:request.room_exterior_source.value_or(request.room.source);
        if(requested_source.zone_id!=source.zone_id||requested_source.zone_version!=source.zone_version||requested_source.instance_id!=source.instance_id){Leave();return {{},LiveItems::Error::Conflict};}
        bool uncertain=false;
        for(;;){try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}request.lease=lease_;
            const auto* config=EQEmuConfig::get();auto db=std::make_unique<Database>();
            if(config&&db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-storage-exclusive")){
                InteriorStorage::Service service(std::move(db),true,house_policies_.crates.get());auto result=request.exterior?service.ChangeExterior(request,*this,house_policies_.scene.get(),house_policies_.room.get()):service.Change(request,*this,*house_policies_.scene,*house_policies_.room);
                if(result){trophy_sources_changed_=true;Leave();return result;}
                if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                else if(!uncertain||result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
            }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
        }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}Wait();}
    }
    LiveItems::Result<std::vector<InteriorStorage::Receipt>> Bridge::StowAll(InteriorStorage::StowRequest request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        if(!zone||(!request.exterior&&(!house_policies_.scene||!house_policies_.room))){Leave();return {{},LiveItems::Error::Unauthorized};}
        const Route source{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
        const auto requested_source=request.exterior?request.exterior->source:request.room.source;
        if(requested_source.zone_id!=source.zone_id||requested_source.zone_version!=source.zone_version||requested_source.instance_id!=source.instance_id){Leave();return {{},LiveItems::Error::Conflict};}
        bool uncertain=false;
        for(;;){try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}request.lease=lease_;
            const auto* config=EQEmuConfig::get();auto db=std::make_unique<Database>();
            if(config&&db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-storage-exclusive")){
                InteriorStorage::Service service(std::move(db),true,house_policies_.crates.get());auto result=service.StowAll(request,*this,house_policies_.scene.get(),house_policies_.room.get());
                if(result){trophy_sources_changed_=true;Leave();return result;}
                if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                else if(!uncertain||result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
            }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
        }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}Wait();}
    }
    LiveItems::Result<InteriorStorage::TransferReceipt> Bridge::TransferStorage(InteriorStorage::TransferRequest request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        if(!zone||!house_policies_.scene||!house_policies_.room){Leave();return {{},LiveItems::Error::Unauthorized};}
        const Route source{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
        const auto physical=request.yard?request.yard->source:request.occupied_room.source;
        if(physical.zone_id!=source.zone_id||physical.zone_version!=source.zone_version||physical.instance_id!=source.instance_id){Leave();return {{},LiveItems::Error::Conflict};}
        bool uncertain=false;
        for(;;){try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}request.lease=lease_;
            const auto* config=EQEmuConfig::get();auto db=std::make_unique<Database>();
            if(config&&db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-transfer-exclusive")){
                InteriorStorage::Service service(std::move(db),true,house_policies_.crates.get());auto result=service.Transfer(request,*this,*house_policies_.scene,*house_policies_.room);
                if(result){trophy_sources_changed_=true;Leave();return result;}if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                else if(!uncertain||result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
            }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
        }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}Wait();}
    }
    LiveItems::Result<uint64_t> Bridge::SelectUpkeep(const std::string& key,EstateId estate,uint32_t native,uint32_t component){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        // Physical source is captured from the actual occupied simulation,
        // never from the native escrow body. Logical receipt identity remains
        // key/lease/estate/component, so committed replay can resolve after a
        // context change without mutating the newer current selection.
        std::optional<Route> occupied_source;
        if(!zone){Leave();return {{},LiveItems::Error::Unauthorized};}
        if(zone->GetZoneID()!=712)occupied_source=Route{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
        bool uncertain=false;
        for(;;){try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            const auto* config=EQEmuConfig::get();auto db=std::make_unique<Database>();
            if(config&&db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-upkeep-selection")){
                UpkeepSelection::Request request{key,lease_,estate,native,component,occupied_source};
                auto result=UpkeepSelection::Select(*db,true,request,*this,house_policies_.scene.get(),house_policies_.crates.get());
                if(result){Leave();return result;}
                if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                else if(!uncertain||result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
            }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
        }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}Wait();}
    }
    LiveItems::Result<Bridge::MoveOutResult> Bridge::MoveOutOccupied(const std::string& key,EstateId estate,uint32_t native,bool enabled){
        using Error=LiveItems::Error;using namespace EstateRetirement::Detail;
        if(!enabled)return {{},Error::Disabled};
        if(client_.ClientVersion()!=EQ::versions::ClientVersion::RoF2||!(client_.GetPP().expansions&EQ::expansions::bitHoT))return {{},Error::UnsupportedItem};
        if(!Enter())return {{},Error::UnheldFence};
        auto connect=[]()->std::unique_ptr<Database>{const auto* c=EQEmuConfig::get();if(!c)return {};auto db=std::make_unique<Database>();if(!db->Connect(c->DatabaseHost,c->DatabaseUsername,c->DatabasePassword,c->DatabaseDB,c->DatabasePort,"housing-occupied-move-out"))return {};return db;};
        uint64_t staged_custody=0;bool staged_new=false,committed=false,may_commit=false;
        auto reject=[&](Error error)->LiveItems::Result<MoveOutResult>{
            if(committed||may_commit)FreezeInvariantFailure();
            if(staged_new){retained_.erase(staged_custody);reconstructed_custody_.erase(staged_custody);}
            Leave();return {{},error};
        };
        std::optional<LiveEscrow::Receipt> refund;
        OccupiedMoveOut::Request request;LiveItems::Result<LiveItems::Receipt> result;std::unique_ptr<EQApplicationPacket> inventory_packet;int32_t staged_serial=0;
        try{
            if(!Open())return reject(Error::StaleLease);
            if(!house_policies_.crates)return reject(Error::UnsupportedItem);
            auto db=connect();if(!db)return reject(Error::DatabaseFailure);
            Scene::Snapshot scene;
            Q(*db,"START TRANSACTION");
            try{auto read=Scene::Service::ReadSceneUnderTransaction(*db,lease_,estate,*this,house_policies_.scene.get(),house_policies_.crates.get());
                if(!read){Q(*db,"ROLLBACK");return reject(read.error);}scene=std::move(read.value);Q(*db,"ROLLBACK");
            }catch(...){db->QueryDatabase(std::string("ROLLBACK"),false);throw;}
            if(scene.native_plot_id!=native||scene.definition.type!=4||!scene.access.implicit_owner)return reject(Error::Unauthorized);
            const Scene::Item* crate=nullptr;for(const auto& item:scene.items)if(item.crate){if(crate)return reject(Error::UnsupportedItem);crate=&item;}
            if(!crate||crate->stored.stowed||!crate->stored.placement||crate->crate->packed||crate->crate->created_by_character_id!=actor_.character_id)return reject(Error::UnsupportedItem);
            auto balance=Q(*db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+std::to_string(estate));
            if(balance.RowCount()>1)return reject(Error::Conflict);
            const auto copper=balance.RowCount()?N(balance.begin()[0]):0;
            if(copper){
                if(copper>INT32_MAX)return reject(Error::UnsupportedItem);
                LiveEscrow::Request coin;coin.request_key=key;coin.lease=lease_;coin.estate_id=estate;
                coin.expected=CoinProfile::Snapshot(client_.GetPP());coin.occupied_refund=true;
                coin.wire.estate_id=native;coin.wire.action=uint32_t(EscrowWire::Action::Withdraw);coin.wire.values.push_back({0,0,int64_t(copper)});
                request.refund=std::move(coin);
            }
            staged_custody=crate->stored.custody_id;
            auto cached=retained_.find(staged_custody);
            if(cached==retained_.end()||!cached->second){
                auto rebuilt=Reconstruct(database,crate->stored.row);if(!rebuilt)return reject(rebuilt.error);
                if(cached==retained_.end())cached=retained_.emplace(staged_custody,std::move(rebuilt.value.item)).first;else cached->second=std::move(rebuilt.value.item);
                staged_new=true;reconstructed_custody_.insert(staged_custody);
                if(!cached->second->SetHousingCrateID(crate->crate->binding.crate_id)||!cached->second->SetHousingEstateID(crate->crate->binding.native_estate_id))return reject(Error::UnsupportedItem);
            }
            if(!Matches(crate->stored.row,*cached->second)||cached->second->GetHousingCrateID()!=crate->crate->binding.crate_id||cached->second->GetHousingEstateID()!=crate->crate->binding.native_estate_id)return reject(Error::Conflict);
            std::optional<uint32_t> destination;
            // Server inventory policy, not a decoded wire field or claimed Live
            // slot priority: use a supported empty general slot, then empty cursor.
            for(uint32_t slot=EQ::invslot::GENERAL_BEGIN;slot<=uint32_t(EQ::invslot::GENERAL_END);++slot){auto row=crate->stored.row;row.values.slot_id=slot;if(ReturnAllowed(row)){destination=slot;break;}}
            if(!destination&&!request.refund){auto row=crate->stored.row;row.values.slot_id=EQ::invslot::slotCursor;if(ReturnAllowed(row))destination=EQ::invslot::slotCursor;}
            if(!destination||!RoF2::SerializeHousingItem(*cached->second,int16_t(*destination)))return reject(Error::UnsupportedItem);
            // Same internal ItemPacket format as Client::SendItemPacket. It
            // points at this exact staged instance, which Apply moves into the
            // real inventory without changing its address or runtime serial.
            const auto serialized=cached->second->Serialize(int16_t(*destination));
            inventory_packet=std::make_unique<EQApplicationPacket>(OP_ItemPacket,serialized.size()+sizeof(ItemPacket_Struct));
            auto* payload=reinterpret_cast<ItemPacket_Struct*>(inventory_packet->pBuffer);payload->PacketType=ItemPacketTrade;
            std::memcpy(payload->SerializedItem,serialized.data(),serialized.size());staged_serial=cached->second->GetSerialNumber();
            request.retirement={key,lease_,estate,native};request.destination_slot=*destination;
            request.expected_crate=OccupiedMoveOut::ExpectedCrate{crate->crate->binding.crate_id,staged_custody,crate->crate->binding.revision,crate->stored.row};
            // Staged cache entry, binding, slot/lore proof and native serializer
            // are ready BEFORE any inventory/custody/ownership COMMIT.
            may_commit=true;
            result=OccupiedMoveOut::Execute(*db,true,request,*this,*house_policies_.crates,house_policies_.custody.get(),house_policies_.scene.get(),house_policies_.room.get(),this,&refund);
            if(result){committed=true;may_commit=false;}
            else if(result.error!=Error::CommitUncertain&&result.error!=Error::DatabaseFailure){may_commit=false;return reject(result.error);}
            db.reset();
        }catch(...){if(!may_commit&&!committed)return reject(Error::DatabaseFailure);}
        if(!committed){
            // Drop uncertain connections. Only immutable receipt+retirement
            // resolution on fresh connections may decide whether COMMIT happened.
            for(;;){try{auto db=connect();auto items=Fresh();if(db&&items){
                result=OccupiedMoveOut::Resolve(*db,*items,request,*this);
                if(result){committed=true;may_commit=false;break;}
                if(result.error==Error::NotFound){
                    auto marker=Q(*db,"SELECT estate_id FROM housing_estate_retirements WHERE request_key="+Quote(*db,key));
                    if(marker.RowCount())FreezeInvariantFailure();
                    may_commit=false;return reject(Error::DatabaseFailure);
                }
                if(result.error!=Error::DatabaseFailure&&result.error!=Error::CommitUncertain)FreezeInvariantFailure();
            }}catch(...){}Wait();}
        }
        if(request.refund){
            // Coin and item journal use the same key in separate tables; their
            // combined pending state cannot authorize any other operation.
            for(;;){try{auto coin=FreshCoin(house_policies_.scene.get());if(coin){auto resolved=coin->Resolve(*request.refund,*this);
                if(resolved){refund=std::move(resolved.value);break;}
                if(resolved.error!=LiveEscrow::Error::DatabaseFailure&&resolved.error!=LiveEscrow::Error::CommitUncertain)FreezeInvariantFailure();
            }}catch(...){}Wait();}
        }
        // Known commit: no exception/invariant path can release the barrier.
        // This is the SAME item application and durable ACK used by live Return.
        try{if(result.value.requires_application?!Apply(result.value):!Applied(result.value))FreezeInvariantFailure();}
        catch(...){FreezeInvariantFailure();}
        if(refund)try{if(refund->requires_application?!ApplyCoin(*refund):!CoinApplied(*refund))FreezeInvariantFailure();}catch(...){FreezeInvariantFailure();}
        if(result.value.requires_application)for(;;){try{auto items=Fresh();if(items&&items->AcknowledgeApplied(lease_,key,*this))break;}catch(...){}Wait();}
        if(refund&&refund->requires_application)for(;;){try{auto coin=FreshCoin(house_policies_.scene.get());if(coin&&coin->AcknowledgeApplied(*request.refund,*this))break;}catch(...){}Wait();}
        lease_.pending_key.clear();MoveOutResult output{std::move(result.value),std::move(inventory_packet),staged_serial,bool(refund)};trophy_sources_changed_=true;Leave();return {std::move(output),Error::None};
    }
    LiveItems::Result<Bridge::ParcelClaimResult> Bridge::ClaimHousingParcel(uint32_t parcel_id,uint32_t native_slot,uint32_t item_id){
        using Error=LiveItems::Error;
        if(client_.ClientVersion()!=EQ::versions::ClientVersion::RoF2)return {{},Error::UnsupportedItem};
        if(!Enter())return {{},Error::UnheldFence};
        auto connect=[]()->std::unique_ptr<Database>{const auto* c=EQEmuConfig::get();if(!c)return {};auto db=std::make_unique<Database>();if(!db->Connect(c->DatabaseHost,c->DatabaseUsername,c->DatabasePassword,c->DatabaseDB,c->DatabasePort,"housing-parcel-claim"))return {};return db;};
        auto query=[](Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::runtime_error("Housing parcel query failed");return r;};
        bool attempted=false,committed=false;uint32_t destination=0;
        const auto key=session_key_+":parcel:"+std::to_string(parcel_id);
        std::unique_ptr<Database> db;std::unique_ptr<EQ::ItemInstance> staged;ParcelClaimResult output;
        std::optional<ReturnParcels::Record> claimed;
        LiveItems::InventoryRow expected_row;std::vector<uint8_t> expected_allocation;
        auto reject=[&](Error error)->LiveItems::Result<ParcelClaimResult>{if(attempted||committed)FreezeInvariantFailure();if(db)db->QueryDatabase(std::string("ROLLBACK"),false);Leave();return {{},error};};
        try{
            if(!Open())return reject(Error::StaleLease);
            db=connect();if(!db)return reject(Error::DatabaseFailure);
            query(*db,"START TRANSACTION");
            auto authenticated=LiveItems::Service::AuthenticateUnderTransaction(*db,true,lease_,*this);
            if(!authenticated)return reject(authenticated.error);
            if(!authenticated.value.pending_key.empty())return reject(Error::PendingApplication);
            const auto record=ReturnParcels::Lookup(*db,parcel_id,actor_.character_id,actor_.account_id);
            if(!record||record->claimed||record->native_slot!=native_slot||record->row.values.item_id!=item_id)return reject(Error::Conflict);
            if(!ReturnSettlement::VerifyUnderTransaction(*db,record->estate_id,record->owner_kind,record->estate_owner_id))return reject(Error::Conflict);
            auto rebuilt=Reconstruct(database,record->row);if(!rebuilt)return reject(rebuilt.error);staged=std::move(rebuilt.value.item);
            if(record->allocation){const auto& b=record->allocation->binding;
                if(!staged->SetHousingCrateID(b.crate_id)||!staged->SetHousingEstateID(b.native_estate_id))return reject(Error::UnsupportedItem);
            }
            // Claim only an empty ordinary saved slot; no stacking/cursor or
            // generic parcel duplicate-deletion policy can consume this item.
            for(uint32_t slot=23;slot<=30;++slot){auto row=record->row;row.values.character_id=actor_.character_id;row.values.slot_id=slot;if(ReturnAllowed(row)){destination=slot;break;}}
            if(!destination)return reject(Error::Conflict);
            if(!RoF2::SerializeHousingItem(*staged,int16_t(destination)))return reject(Error::UnsupportedItem);
            const auto serialized=staged->Serialize(int16_t(destination));
            output.inventory_packet=std::make_unique<EQApplicationPacket>(OP_ItemPacket,serialized.size()+sizeof(ItemPacket_Struct));
            auto* payload=reinterpret_cast<ItemPacket_Struct*>(output.inventory_packet->pBuffer);payload->PacketType=ItemPacketTrade;
            std::memcpy(payload->SerializedItem,serialized.data(),serialized.size());
            expected_row=record->row;expected_row.values.character_id=actor_.character_id;expected_row.values.slot_id=destination;
            claimed=ReturnParcels::ClaimUnderTransaction(*db,*record,destination,lease_,key);
            if(!claimed->claimed||claimed->parcel_id!=parcel_id||claimed->owner!=actor_.character_id||claimed->account!=actor_.account_id||
                CustodyCodec::Encode(claimed->row)!=CustodyCodec::Encode(expected_row)||bool(claimed->allocation)!=bool(record->allocation))return reject(Error::Conflict);
            if(claimed->allocation)expected_allocation=CrateBinding::EncodeAllocation(*claimed->allocation);
            // Everything needed by the native packet and memory projection is
            // staged before COMMIT; no ordinary inventory save follows it.
            attempted=true;
            query(*db,"COMMIT");committed=true;
        }catch(...){if(!attempted)return reject(Error::DatabaseFailure);}
        db.reset();
        if(!committed){
            for(;;){try{db=connect();if(db){
                query(*db,"START TRANSACTION");
                auto resolved=ReturnParcels::ResolveClaim(*db,parcel_id,actor_.character_id,actor_.account_id,key,destination);
                query(*db,"ROLLBACK");
                if(!resolved){attempted=false;return reject(Error::DatabaseFailure);}
                claimed=std::move(resolved);committed=true;break;
            }}catch(...){}db.reset();Wait();}
        }
        try{
            if(!claimed||!claimed->claimed||!staged||!output.inventory_packet||
                claimed->parcel_id!=parcel_id||claimed->owner!=actor_.character_id||claimed->account!=actor_.account_id||
                claimed->row.values.character_id!=actor_.character_id||claimed->row.values.slot_id!=destination||
                CustodyCodec::Encode(claimed->row)!=CustodyCodec::Encode(expected_row)||
                !Matches(claimed->row,*staged)||!ReturnAllowed(claimed->row))FreezeInvariantFailure();
            if(claimed->allocation){const auto& a=*claimed->allocation;
                if(CrateBinding::EncodeAllocation(a)!=expected_allocation||a.inventory_character_id!=actor_.character_id||a.inventory_slot_id!=destination||a.binding.custody_id||
                    staged->GetHousingCrateID()!=a.binding.crate_id||staged->GetHousingEstateID()!=a.binding.native_estate_id)FreezeInvariantFailure();
            }else if(staged->GetHousingCrateID()||staged->GetHousingHouseID())FreezeInvariantFailure();
            auto* exact=staged.get();const auto serial=exact->GetSerialNumber();
            if(client_.GetInv().PushItem(int16_t(destination),exact)!=int16_t(destination))FreezeInvariantFailure();
            staged.release();
            if(client_.GetInv().GetItem(int16_t(destination))!=exact||exact->GetSerialNumber()!=serial||!Matches(claimed->row,*exact))FreezeInvariantFailure();
        }catch(...){FreezeInvariantFailure();}
        Leave();return {std::move(output),Error::None};
    }
    LiveItems::Result<uint32_t> Bridge::RetireEmptyPlot(const std::string& key,EstateId estate,uint32_t native){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        bool uncertain=false;
        for(;;){try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            const auto* config=EQEmuConfig::get();auto db=std::make_unique<Database>();
            if(config&&db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-empty-move-out")){
                EstateRetirement::Request request{key,lease_,estate,native};
                auto result=EstateRetirement::Retire(*db,true,request,*this,house_policies_.scene.get(),house_policies_.crates.get());
                if(result){trophy_sources_changed_=true;Leave();return result;}
                if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                else if(!uncertain||result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
            }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
        }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}Wait();}
    }
    LiveItems::Result<CrateStorage::Receipt> Bridge::TransferCrate(CrateStorage::Request request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        if(!zone||!house_policies_.crates){Leave();return {{},LiveItems::Error::Unauthorized};}
        const Route source{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
        const auto requested_source=request.parent_yard?request.parent_yard->source:request.source;
        if(requested_source.zone_id!=source.zone_id||requested_source.zone_version!=source.zone_version||requested_source.instance_id!=source.instance_id){Leave();return {{},LiveItems::Error::Conflict};}
        bool uncertain=false;
        for(;;){try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}request.lease=lease_;
            const auto* config=EQEmuConfig::get();auto db=std::make_unique<Database>();
            if(config&&db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-crate-transfer")){
                CrateStorage::Service service(std::move(db),true);auto result=service.Transfer(request,*this,*house_policies_.crates,house_policies_.scene.get(),house_policies_.room.get());
                if(result){trophy_sources_changed_=true;Leave();return result;}if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                else if(!uncertain||result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
            }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
        }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}Wait();}
    }
    LiveItems::Result<CrateStorage::BatchReceipt> Bridge::PackCrate(CrateStorage::BatchRequest request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        if(!zone||!house_policies_.crates){Leave();return {{},LiveItems::Error::Unauthorized};}
        const Route source{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
        if(request.source.zone_id!=source.zone_id||request.source.zone_version!=source.zone_version||request.source.instance_id!=source.instance_id){Leave();return {{},LiveItems::Error::Conflict};}
        bool uncertain=false;
        for(;;){try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}request.lease=lease_;
            const auto* config=EQEmuConfig::get();auto db=std::make_unique<Database>();
            if(config&&db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-crate-batch")){
                CrateStorage::Service service(std::move(db),true);auto result=service.Pack(request,*this,*house_policies_.crates,house_policies_.scene.get(),house_policies_.room.get());
                if(result){trophy_sources_changed_=true;Leave();return result;}if(result.error==LiveItems::Error::CommitUncertain)uncertain=true;
                else if(!uncertain||result.error!=LiveItems::Error::DatabaseFailure){Leave();return result;}
            }else if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}
        }catch(...){if(!uncertain){Leave();return {{},LiveItems::Error::DatabaseFailure};}}Wait();}
    }
    LiveItems::Result<Scene::InteriorSnapshot> Bridge::ReadAttachedInterior(EstateId estate,uint64_t house_id){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}const auto* config=EQEmuConfig::get();
            if(!config||!zone||!house_policies_.scene||!house_policies_.room){Leave();return {{},LiveItems::Error::Unauthorized};}auto db=std::make_unique<Database>();
            if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-attached-scene")){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            Scene::Service service(std::move(db),true,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get());auto result=service.ReadAttachedInterior(lease_,estate,house_id,*this);Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<Scene::Snapshot> Bridge::ReadParentScene(){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}const auto* config=EQEmuConfig::get();
            if(!config||!zone||!house_policies_.scene){Leave();return {{},LiveItems::Error::Unauthorized};}
            const Route source{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};auto db=std::make_unique<Database>();
            if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-parent-scene")){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            Scene::Service service(std::move(db),true,house_policies_.scene.get(),nullptr,house_policies_.crates.get());auto result=service.ReadParentScene(lease_,source,*this);Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<LiveItems::MoveReceipt> Bridge::MovePlaced(LiveItems::MoveRequest request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            request.lease=lease_;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
        LiveItems::Result<LiveItems::MoveReceipt> result;
        bool uncertain=false;
        try{
            auto service=Fresh();
            if(!service){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            if(request.room){if(!house_policies_.scene||!house_policies_.room){Leave();return {{},LiveItems::Error::Unauthorized};}result=service->MoveInterior(request,*this,*house_policies_.scene,*house_policies_.room);}else result=service->MovePlaced(request,*this);
            uncertain=result.error==LiveItems::Error::CommitUncertain || result.error==LiveItems::Error::DatabaseFailure;
        }catch(...){uncertain=true;}
        if(uncertain){
            // A pose commit never applies an inventory delta. Resolve its
            // immutable journal before releasing the simulation barrier; the
            // caller must rebuild actors from ReadPlaced, not receipt.after.
            for(;;){
                try{
                    auto service=Fresh();
                    if(service){
                        result=service->ResolveMove(lease_,request.request_key,*this);
                        if(result)break;
                        if(result.error==LiveItems::Error::NotFound){Leave();return {{},LiveItems::Error::DatabaseFailure};}
                    }
                }catch(...){}
                Wait();
            }
        }
        if(result)trophy_sources_changed_=true;Leave();return result;
    }
    LiveItems::Result<std::shared_ptr<const HouseBinding::Binding>> Bridge::ReadInventoryHouse(const LiveItems::InventoryRow& row){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            auto service=Fresh();auto* item=client_.GetInv().GetItem(int16_t(row.values.slot_id));
            if(!service||!item){Leave();return {{},LiveItems::Error::UnsupportedItem};}
            auto result=service->ReadInventoryHouse(lease_,row,item->GetSerialNumber(),*this);
            if(result){
                const auto& b=*result.value;const auto estate=b.interior?b.interior->native_estate_id:UINT32_MAX;
                if(item->GetHousingHouseID()!=b.house_id||item->GetHousingEstateID()!=estate){Leave();return {{},LiveItems::Error::Conflict};}
            }
            Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<std::shared_ptr<const CrateBinding::Allocation>> Bridge::ReadInventoryCrate(const LiveItems::InventoryRow& row){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}auto service=Fresh();auto* item=client_.GetInv().GetItem(int16_t(row.values.slot_id));
            if(!service||!item||!CratePlacement::Model(*item)){Leave();return {{},LiveItems::Error::UnsupportedItem};}
            auto result=service->ReadInventoryCrate(lease_,row,item->GetSerialNumber(),*this);
            if(result&&(item->GetHousingHouseID()||item->GetHousingCrateID()!=result.value->binding.crate_id||item->GetHousingEstateID()!=result.value->binding.native_estate_id)){Leave();return {{},LiveItems::Error::Conflict};}
            Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<Scene::Admission> Bridge::ExitHouse(){
        auto result=ResolveInterior(true);if(!result)return {{},result.error};if(!result.value.exit)return {{},LiveItems::Error::Conflict};return {*result.value.exit,LiveItems::Error::None};
    }
    LiveItems::Result<Scene::InteriorSnapshot> Bridge::ReadInterior(){return ResolveInterior(false);}
    LiveItems::Result<Scene::InteriorSnapshot> Bridge::ReadInteriorForManagement(){return ResolveInterior(false,true);}
    LiveItems::Result<Scene::InteriorSnapshot> Bridge::ResolveInterior(bool egress,bool management_view){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            const auto* config=EQEmuConfig::get();
            if(!config||!zone||!zone->GetInstanceID()||zone->GetInstanceID()>32000||zone->GetInstanceVersion()>255){Leave();return {{},LiveItems::Error::InvalidArgument};}
            // Source coordinates/zone are the live simulation, never packet data
            // or character_data's last saved location.
            Route source{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
            auto db=std::make_unique<Database>();
            if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-house-exit")){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            Scene::Service service(std::move(db),true,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get());
            LiveItems::Result<Scene::InteriorSnapshot> result;
            if(egress){auto exited=service.ExitHouse(lease_,source,*this);if(!exited){Leave();return {{},exited.error};}Scene::InteriorSnapshot snapshot;snapshot.exit=exited.value;Leave();return {std::move(snapshot),LiveItems::Error::None};}
            result=management_view?service.ReadInteriorForManagement(lease_,source,*this):service.ReadInterior(lease_,source,*this);Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<GuildTrophies::View> Bridge::GuildTrophiesView(const GuildTrophies::Request& request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};std::unique_ptr<Database> db;bool transaction=false;
        try{if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}const auto* config=EQEmuConfig::get();if(!config){Leave();return {{},LiveItems::Error::DatabaseFailure};}db=std::make_unique<Database>();if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-guild-trophies")){Leave();return {{},LiveItems::Error::DatabaseFailure};}if(!db->QueryDatabase(std::string("START TRANSACTION"),false).Success()){Leave();return {{},LiveItems::Error::DatabaseFailure};}transaction=true;
         auto trusted=request;trusted.level=client_.GetLevel();trusted.location.reset();if(zone)trusted.location=TrophyBenefits::SelectionLocation{{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())},{client_.GetY(),client_.GetX(),client_.GetZ()}};
         auto result=GuildTrophies::ExecuteUnderTransaction(*db,lease_,*this,trusted,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get());if(!result){db->QueryDatabase(std::string("ROLLBACK"),false);Leave();return result;}if(!db->QueryDatabase(std::string("COMMIT"),false).Success())FreezeInvariantFailure();transaction=false;Leave();return result;
        }catch(...){if(transaction&&db)db->QueryDatabase(std::string("ROLLBACK"),false);Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<GuildPortal::Alignment> Bridge::AlignGuildPortal(const GuildPortalRuntime::Source& source,uint32_t slot,int32_t serial,const GuildPortalRuntime::Validator& validate){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        std::unique_ptr<Database> db;std::unique_ptr<EQ::ItemInstance> consumed;EQ::ItemInstance* memory=nullptr;
        bool transaction=false,committing=false,changed=false;int16_t old_charges=0;
        try{
            if(!validate||slot<uint32_t(EQ::invslot::TRADE_BEGIN)||slot>uint32_t(EQ::invslot::TRADE_NPC_END)||!GuildPortal::Valid(source.scope)||source.binding.item_id!=66640||source.binding.service!=ServiceNpcs::Service::TeleportMerchant||!zone||source.scope.zone!=zone->GetZoneID()||source.scope.instance!=zone->GetInstanceID()||source.binding.house_id!=source.scope.house||source.binding.estate_id!=source.scope.estate||source.binding.guild_id!=source.scope.guild||client_.GuildID()!=source.scope.guild||!Open())throw std::invalid_argument("Invalid housing portal hand-in scope");
            memory=client_.GetInv().GetItem(int16_t(slot));if(!memory||memory->GetSerialNumber()!=serial||!GuildPortal::Purchasable(memory->GetID())||!memory->IsClassCommon()||memory->IsEvolving()||memory->IsAugmented()||!memory->GetContents()->empty()||memory->IsAttuned())throw std::invalid_argument("Unsupported housing portal stone");
            old_charges=memory->GetCharges();if((memory->IsStackable()&&old_charges<1)||(!memory->IsStackable()&&(old_charges<0||old_charges>1)))throw std::invalid_argument("Unsupported housing portal stone quantity");
            const auto* config=EQEmuConfig::get();if(!config)throw std::runtime_error("Missing DB configuration");db=std::make_unique<Database>();if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-guild-portal"))throw std::runtime_error("Portal connection failed");
            auto engines=GuildPortal::Query(*db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('inventory','housing_guild_portal_alignments')");if(engines.RowCount()!=2)throw std::invalid_argument("Portal transaction schema missing");for(auto engine:engines)if(!engine[0]||std::string(engine[0])!="InnoDB")throw std::invalid_argument("Portal transaction requires InnoDB");
            GuildPortal::Query(*db,"START TRANSACTION");transaction=true;auto authenticated=LiveItems::Service::AuthenticateUnderTransaction(*db,true,lease_,*this);if(!authenticated||!authenticated.value.pending_key.empty())throw std::invalid_argument("Portal lease not current");
            if(!validate(*db,client_,source)||!GuildPortal::SameCurrent(*db,source.scope,true))throw std::invalid_argument("Portal merchant no longer current");
            auto rows=GuildPortal::Query(*db,std::string("SELECT ")+InventoryRowDB::Columns+" FROM inventory WHERE character_id="+std::to_string(actor_.character_id)+" AND slot_id="+std::to_string(slot)+" FOR UPDATE");if(rows.RowCount()!=1)throw std::invalid_argument("Missing portal stone row");auto row=InventoryRowDB::Decode(rows.begin());
            if(row.values.character_id!=actor_.character_id||row.values.slot_id!=slot||memory!=client_.GetInv().GetItem(int16_t(slot))||memory->GetSerialNumber()!=serial||!Matches(row,*memory))throw std::invalid_argument("Portal stone identity changed");
            // The canonical row (including NULL/GUID/custom/ornament fields) is
            // matched before changing exactly one physical crystal. No source
            // packet or merchant item template alone grants inventory authority.
            const bool partial=memory->IsStackable()&&old_charges>1;
            auto removed=GuildPortal::Query(*db,partial?"UPDATE inventory SET charges="+std::to_string(old_charges-1)+" WHERE character_id="+std::to_string(actor_.character_id)+" AND slot_id="+std::to_string(slot):"DELETE FROM inventory WHERE character_id="+std::to_string(actor_.character_id)+" AND slot_id="+std::to_string(slot));if(removed.RowsAffected()!=1)throw std::runtime_error("Portal stone debit failed");
            auto alignment=GuildPortal::AlignLocked(*db,source.scope,row.values.item_id,GuildPortal::DatabaseNow(*db));
            if(partial)memory->SetCharges(old_charges-1);else{consumed.reset(client_.GetInv().PopItem(int16_t(slot)));if(consumed.get()!=memory)FreezeInvariantFailure();}changed=true;
            committing=true;if(!db->QueryDatabase(std::string("COMMIT"),false).Success())FreezeInvariantFailure();transaction=false;
            Leave();return {alignment,LiveItems::Error::None};
        }catch(...){
            if(committing)FreezeInvariantFailure();
            if(changed){if(consumed){if(client_.GetInv().PushItem(int16_t(slot),consumed.get())!=int16_t(slot))FreezeInvariantFailure();consumed.release();}else if(memory)memory->SetCharges(old_charges);}
            if(transaction&&db&&!db->QueryDatabase(std::string("ROLLBACK"),false).Success())FreezeInvariantFailure();Leave();return {{},LiveItems::Error::DatabaseFailure};
        }
    }
    std::optional<uint32_t> Bridge::CurrentFavorUnderFence(const LiveItems::Lease& lease)const noexcept{
        if(!HeldFor(lease.actor,lease.epoch,lease.session_key))return {};return client_.GetPP().tribute_points;
    }
    LiveItems::Result<TrophyBenefits::View> Bridge::Trophies(const TrophyBenefits::Request& request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        std::unique_ptr<Database> db;bool transaction=false,committing=false,memory_favor_changed=false;const uint32_t before_favor=client_.GetPP().tribute_points;
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            const auto* config=EQEmuConfig::get();if(!config){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            db=std::make_unique<Database>();
            if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-trophy-benefits")){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            if(!db->QueryDatabase(std::string("START TRANSACTION"),false).Success()){Leave();return {{},LiveItems::Error::DatabaseFailure};}transaction=true;
            auto trusted=request;trusted.selection_location.reset();
            if(zone){trusted.selection_location=TrophyBenefits::SelectionLocation{{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())},{client_.GetY(),client_.GetX(),client_.GetZ()}};}
            auto result=TrophyBenefits::ExecuteUnderTransaction(*db,lease_,*this,trusted,this,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get());
            if(!result){db->QueryDatabase(std::string("ROLLBACK"),false);Leave();return result;}
            // Apply the current profile balance BEFORE COMMIT while every save,
            // callback and send is barred. Rollback restores it; a process crash
            // observes either rolled-back DB favor or committed after-favor.
            if(result.value.favor_after){if(client_.GetPP().tribute_points!=before_favor)FreezeInvariantFailure();client_.GetPP().tribute_points=*result.value.favor_after;memory_favor_changed=true;}
            committing=true;if(!db->QueryDatabase(std::string("COMMIT"),false).Success())FreezeInvariantFailure();transaction=false;
            Leave();return result;
        }catch(...){if(committing)FreezeInvariantFailure();if(memory_favor_changed)client_.GetPP().tribute_points=before_favor;if(transaction&&db)db->QueryDatabase(std::string("ROLLBACK"),false);Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<Scene::AnchorDestination> Bridge::ResolveAnchor(const Scene::AnchorRequest& request,bool admit,const Scene::AnchorDestination* expected){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            const auto* config=EQEmuConfig::get();if(!config){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            auto db=std::make_unique<Database>();
            if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-anchor-remote")){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            Scene::Service service(std::move(db),true,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get());
            auto result=service.ResolveAnchor(lease_,request,*this,admit,expected);Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<Scene::Snapshot> Bridge::ReadScene(EstateId estate){return ReadCurrentScene(estate,nullptr);}
    LiveItems::Result<Scene::Snapshot> Bridge::AdmitHouse(EstateId estate,const Scene::EntryRequest& request){return ReadCurrentScene(estate,&request);}
    LiveItems::Result<Scene::LobbyAdmission> Bridge::AdmitOwnGuildHallFromLobby(const Scene::LobbyRequest& request){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            const auto* config=EQEmuConfig::get();if(!config){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            auto db=std::make_unique<Database>();
            if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-lobby-admission")){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            Scene::Service service(std::move(db),true,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get());
            auto result=service.AdmitOwnGuildHallFromLobby(lease_,request,*this);Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<Scene::Snapshot> Bridge::ReadCurrentScene(EstateId estate,const Scene::EntryRequest* entry){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            const auto* config=EQEmuConfig::get();
            if(!config){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            auto db=std::make_unique<Database>();
            if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,
                config->DatabaseDB,config->DatabasePort,"housing-current-scene")){
                Leave();return {{},LiveItems::Error::DatabaseFailure};
            }
            Scene::Service service(std::move(db),true,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get());
            auto result=entry?service.AdmitHouse(lease_,estate,*entry,*this):service.ReadScene(lease_,estate,*this);
            Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    namespace {
        std::unique_ptr<NativeAccess::Service> FreshAccess(const NativeAccess::InteriorSourcePolicy* interior=nullptr){
            const auto* config=EQEmuConfig::get();if(!config)return {};
            auto db=std::make_unique<Database>();
            if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-native-access"))return {};
            return std::make_unique<NativeAccess::Service>(std::move(db),true,interior);
        }
    }
    NativeAccess::Result<NativeAccess::Snapshot> Bridge::ReadAccess(EstateId estate,uint32_t native){
        if(!Enter())return {{},NativeAccess::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},NativeAccess::Error::StaleLease};}
            auto service=FreshAccess(this);if(!service){Leave();return {{},NativeAccess::Error::DatabaseFailure};}
            auto result=service->Read(lease_,estate,native,*this);Leave();return result;
        }catch(...){Leave();return {{},NativeAccess::Error::DatabaseFailure};}
    }
    NativeAccess::Result<bool> Bridge::SetVotingOptOut(EstateId estate,uint32_t native,bool opted_out){
        if(!Enter())return {{},NativeAccess::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},NativeAccess::Error::StaleLease};}
            auto service=FreshAccess();if(!service){Leave();return {{},NativeAccess::Error::DatabaseFailure};}
            auto result=service->SetVotingOptOut(lease_,estate,native,opted_out,*this);Leave();return result;
        }catch(...){Leave();return {{},NativeAccess::Error::DatabaseFailure};}
        // No retry on uncertain commit: callers restore the fresh preference.
        // There is no inventory/currency handoff or client-supplied replay token.
    }
    NativeAccess::Result<NativeAccess::Receipt> Bridge::ReplaceAccess(NativeAccess::Request request){
        if(!Enter())return {{},NativeAccess::Error::UnheldFence};
        try{if(!Open()){Leave();return {{},NativeAccess::Error::StaleLease};}request.lease=lease_;}
        catch(...){Leave();return {{},NativeAccess::Error::DatabaseFailure};}
        bool uncertain=false;
        try{
            auto service=FreshAccess(this);if(!service){Leave();return {{},NativeAccess::Error::DatabaseFailure};}
            auto result=service->Replace(request,*this);
            uncertain=result.error==NativeAccess::Error::CommitUncertain||result.error==NativeAccess::Error::DatabaseFailure;
            if(!uncertain){Leave();return result;}
        }catch(...){uncertain=true;}
        // No cache, packets, lifecycle transitions or callbacks while held.
        // Resolve this immutable request before admitting more simulation work.
        for(;;){
            try{auto service=FreshAccess(this);if(service){auto result=service->Resolve(request,*this);
                if(result.error!=NativeAccess::Error::DatabaseFailure&&result.error!=NativeAccess::Error::CommitUncertain){Leave();return result;}
            }}catch(...){}
            Wait();
        }
    }
    LiveItems::Result<LiveItems::Receipt> Bridge::ReturnCrateMember(const std::string& key,const LiveItems::Stored& cached,const LiveItems::CrateMemberContext& context,uint32_t destination,std::optional<LiveItems::HouseIdentity> house,uint32_t quantity){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            if(!house_policies_.crates||!zone||!cached.placement){Leave();return {{},LiveItems::Error::Unauthorized};}
            const Route source{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
            if(context.source.zone_id!=source.zone_id||context.source.zone_version!=source.zone_version||context.source.instance_id!=source.instance_id){Leave();return {{},LiveItems::Error::Conflict};}
            auto service=Fresh();if(!service){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            auto prior=service->Resolve(lease_,key,*this);
            if(prior){if(prior.value.action!=LiveItems::Action::Return||!prior.value.crate_member||prior.value.stored.custody_id!=cached.custody_id||prior.value.stored.estate_id!=cached.estate_id||prior.value.inventory_after.values.slot_id!=destination){Leave();return {{},LiveItems::Error::Conflict};}}
            else{
                if(prior.error!=LiveItems::Error::NotFound){Leave();return prior;}
                // The cache supplies reconstruction bytes only. OperateCrate rechecks
                // its exact current custody/member/creator under the SQL transaction.
                auto local=retained_.find(cached.custody_id);
                if(local==retained_.end()||!local->second){auto rebuilt=Reconstruct(database,cached.row);if(!rebuilt){Leave();return {{},rebuilt.error};}if(local==retained_.end())local=retained_.emplace(cached.custody_id,std::move(rebuilt.value.item)).first;else local->second=std::move(rebuilt.value.item);reconstructed_custody_.insert(cached.custody_id);}
                if(!Matches(cached.row,*local->second)){Leave();return {{},LiveItems::Error::Conflict};}
            }
            LiveItems::Request request;request.request_key=key;request.action=LiveItems::Action::Return;request.estate_id=cached.estate_id;request.custody_id=cached.custody_id;request.destination_slot=destination;request.expected=cached.row;request.crate_member=context;request.house=house;
            if(prior&&prior.value.partition){request.partition_quantity=prior.value.partition->quantity;request.partition_serial=prior.value.partition->serial;if(quantity!=request.partition_quantity){Leave();return {{},LiveItems::Error::Conflict};}}
            else if(!prior){auto source=retained_.find(cached.custody_id);if(source==retained_.end()||!source->second){Leave();return {{},LiveItems::Error::Conflict};}request.partition_quantity=StackPartitionApplication::Requested(*source->second,quantity);if(request.partition_quantity){if(cached.row.values.character_id!=actor_.character_id){Leave();return {{},LiveItems::Error::UnsupportedItem};}auto piece=StackPartitionApplication::Prepare(cached.row,*source->second,request.partition_quantity);if(!piece){Leave();return {{},LiveItems::Error::UnsupportedItem};}request.partition_serial=piece->GetSerialNumber();partition_pieces_[key]=std::move(piece);}}

            auto result=Execute(std::move(request));Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<LiveItems::Receipt> Bridge::ReturnInterior(const std::string& key,const Scene::InteriorContent& cached,const LiveItems::InteriorMoveContext& context,uint32_t destination,std::optional<Route> room_exterior_source,std::optional<LiveItems::CrateMemberContext> crate_member,uint32_t quantity){
        if(!Enter())return {{},LiveItems::Error::UnheldFence};
        try{
            if(!Open()){Leave();return {{},LiveItems::Error::StaleLease};}
            if(!house_policies_.scene||!house_policies_.room||!cached.room||!cached.stored.placement||!zone){Leave();return {{},LiveItems::Error::Unauthorized};}
            Route source{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
            const auto physical=room_exterior_source.value_or(context.source);
            if(physical.zone_id!=source.zone_id||physical.zone_version!=source.zone_version||physical.instance_id!=source.instance_id){Leave();return {{},LiveItems::Error::Conflict};}
            auto service=Fresh();if(!service){Leave();return {{},LiveItems::Error::DatabaseFailure};}
            auto prior=service->Resolve(lease_,key,*this);
            if(prior){if(prior.value.action!=LiveItems::Action::Return||!prior.value.room||prior.value.stored.custody_id!=cached.stored.custody_id||prior.value.inventory_after.values.slot_id!=destination){Leave();return {{},LiveItems::Error::Conflict};}}
            else{
                if(prior.error!=LiveItems::Error::NotFound){Leave();return prior;}
                const auto* config=EQEmuConfig::get();if(!config){Leave();return {{},LiveItems::Error::DatabaseFailure};}
                auto db=std::make_unique<Database>();if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-room-return")){Leave();return {{},LiveItems::Error::DatabaseFailure};}
                Scene::Service scene(std::move(db),true,house_policies_.scene.get(),house_policies_.room.get(),house_policies_.crates.get());auto current=room_exterior_source?scene.ReadCurrentAttachedInterior(lease_,context.house_id,*this):scene.ReadInterior(lease_,source,*this);if(!current){Leave();return {{},current.error};}
                const auto& members=crate_member?current.value.crate_contents:current.value.contents;
                auto found=std::find_if(members.begin(),members.end(),[&](const auto& item){return item.stored.custody_id==cached.stored.custody_id;});
                if(found==members.end()||!found->room||!found->stored.placement||found->scope_revision!=cached.scope_revision||found->stored.placement->pose_revision!=cached.stored.placement->pose_revision||CustodyCodec::Encode(found->stored.row)!=CustodyCodec::Encode(cached.stored.row)){Leave();return {{},LiveItems::Error::Conflict};}
                auto local=retained_.find(cached.stored.custody_id);
                if(local==retained_.end()||!local->second){auto rebuilt=Reconstruct(database,found->stored.row);if(!rebuilt){Leave();return {{},rebuilt.error};}if(local==retained_.end())local=retained_.emplace(cached.stored.custody_id,std::move(rebuilt.value.item)).first;else local->second=std::move(rebuilt.value.item);reconstructed_custody_.insert(cached.stored.custody_id);}
                if((crate_member&&(found->contained_in_crate!=crate_member->crate_id||cached.contained_in_crate!=crate_member->crate_id))||!Matches(found->stored.row,*local->second)||!CratePresentation::SameBinding(found->crate,cached.crate)){Leave();return {{},LiveItems::Error::Conflict};}
                if(found->crate&&!CratePresentation::Project(*found,current.value.house->house_id,current.value.house->interior->native_estate_id,*local->second)){Leave();return {{},LiveItems::Error::Conflict};}
            }
            LiveItems::Request request;request.request_key=key;request.action=LiveItems::Action::Return;request.estate_id=cached.stored.estate_id;request.custody_id=cached.stored.custody_id;request.destination_slot=destination;request.expected=cached.stored.row;request.room=cached.room;request.room_return_context=context;request.expected_room_pose_revision=cached.stored.placement->pose_revision;request.room_exterior_source=room_exterior_source;request.crate_member=crate_member;if(cached.crate)request.crate=LiveItems::CrateIdentity{cached.crate->binding.crate_id,cached.crate->binding.revision};
            if(prior&&prior.value.partition){request.partition_quantity=prior.value.partition->quantity;request.partition_serial=prior.value.partition->serial;if(quantity!=request.partition_quantity){Leave();return {{},LiveItems::Error::Conflict};}}
            else if(!prior){auto source=retained_.find(cached.stored.custody_id);if(source==retained_.end()||!source->second){Leave();return {{},LiveItems::Error::Conflict};}request.partition_quantity=StackPartitionApplication::Requested(*source->second,quantity);if(request.partition_quantity){if(request.expected.values.character_id!=actor_.character_id){Leave();return {{},LiveItems::Error::UnsupportedItem};}auto piece=StackPartitionApplication::Prepare(request.expected,*source->second,request.partition_quantity);if(!piece){Leave();return {{},LiveItems::Error::UnsupportedItem};}request.partition_serial=piece->GetSerialNumber();partition_pieces_[key]=std::move(piece);}}
            auto result=Execute(std::move(request));Leave();return result;
        }catch(...){Leave();return {{},LiveItems::Error::DatabaseFailure};}
    }
    LiveItems::Result<LiveItems::Receipt> Bridge::Return(const std::string& key,EstateId estate,uint64_t id,uint32_t destination,std::optional<LiveItems::HouseIdentity> house,std::optional<LiveItems::CrateIdentity> crate,uint32_t quantity){
        if(!Enter())return {
            {
            },LiveItems::Error::UnheldFence
        };
        LiveItems::Result<LiveItems::Receipt> out;
        try{
            if(!Open()){
                Leave();
                return {
                    {
                    },LiveItems::Error::StaleLease
                };
            }
            auto service=Fresh();
            if(!service){
                Leave();
                return {
                    {
                    },LiveItems::Error::DatabaseFailure
                };
            }
            auto prior=service->Resolve(lease_,key,*this);
            LiveItems::InventoryRow expected;
            if(prior){
                if(prior.value.action!=LiveItems::Action::Return||prior.value.stored.estate_id!=estate||prior.value.stored.custody_id!=id||prior.value.inventory_after.values.slot_id!=destination){
                    Leave();
                    return {
                        {
                        },LiveItems::Error::Conflict
                    };
                }
                expected=prior.value.stored.row;
            }
            else{
                if(prior.error!=LiveItems::Error::NotFound){
                    Leave();
                    return prior;
                }
                auto local=retained_.find(id);
                auto stored=service->ReadStored(lease_,id,*this);
                if(!stored){
                    Leave();
                    return {
                        {
                        },stored.error
                    };
                }
                if(stored.value.estate_id!=estate){
                    Leave();
                    return {
                        {
                        },LiveItems::Error::Conflict
                    };
                }
                if(local==retained_.end()||!local->second){
                    auto rebuilt=Reconstruct(database,stored.value.row);
                    if(!rebuilt){Leave();return {{},rebuilt.error};}
                    if(local==retained_.end())local=retained_.emplace(id,std::move(rebuilt.value.item)).first;
                    else local->second=std::move(rebuilt.value.item);
                    reconstructed_custody_.insert(id);
                }
                if(!Matches(stored.value.row,*local->second)){Leave();return {{},LiveItems::Error::Conflict};}
                expected=stored.value.row;
            }
            LiveItems::Request r;
            r.request_key=key;
            r.action=LiveItems::Action::Return;
            r.estate_id=estate;
            r.custody_id=id;
            r.destination_slot=destination;
            r.expected=expected;
            r.house=house;r.crate=crate;
            if(prior&&prior.value.partition){r.partition_quantity=prior.value.partition->quantity;r.partition_serial=prior.value.partition->serial;if(quantity!=r.partition_quantity){Leave();return {{},LiveItems::Error::Conflict};}}
            else if(!prior){auto source=retained_.find(id);if(source==retained_.end()||!source->second){Leave();return {{},LiveItems::Error::Conflict};}r.partition_quantity=StackPartitionApplication::Requested(*source->second,quantity);if(r.partition_quantity){auto piece=StackPartitionApplication::Prepare(expected,*source->second,r.partition_quantity);if(!piece){Leave();return {{},LiveItems::Error::UnsupportedItem};}r.partition_serial=piece->GetSerialNumber();partition_pieces_[key]=std::move(piece);}}
            out=Execute(r);
        }
        catch(...){
            Leave();
            return {
                {
                },LiveItems::Error::DatabaseFailure
            };
        }
        Leave();
        return out;
    }
    LiveItems::Result<bool> Bridge::CloseSession(){
        if(!Enter(true))return {
            {
            },LiveItems::Error::UnheldFence
        };
        if(!lease_.epoch){
            Leave();
            return {
                true,LiveItems::Error::None
            };
        }
        bool uncertain=false;
        for(;;){
            try{
                auto service=Fresh();
                if(service){
                    if(uncertain){
                        auto reopened=service->OpenLease(actor_,session_key_,*this);
                        if(!reopened){
                            Wait();
                            continue;
                        }
                        lease_=reopened.value;
                        if(!lease_.pending_key.empty())FreezeInvariantFailure();
                    }
                    auto closed=service->ReleaseLease(lease_,*this);
                    if(closed){
                        lease_={
                        };
                        Leave();
                        return closed;
                    }
                    if(closed.error==LiveItems::Error::PendingApplication)FreezeInvariantFailure();
                    if(closed.error==LiveItems::Error::CommitUncertain)uncertain=true;
                    else if(!uncertain){
                        Leave();
                        return closed;
                    }
                }
            }
            catch(...){
                if(!uncertain){
                    Leave();
                    return {
                        {
                        },LiveItems::Error::DatabaseFailure
                    };
                }
            }
            Wait();
        }
    }
}
