#include "housing_height_scene.h"
#include "client.h"
#include "entity.h"
#include "doors.h"
#include "object.h"
#include "corpse.h"
#include "zone.h"
#include "housing_actor_publication.h"
#include "housing_service_npcs.h"
#include "housing_service_authority.h"
#include "../common/housing/house_binding_service.h"
#include "../common/housing/interior_content_binding.h"
#include "../common/housing/original_terrain_source.h"
#include "../common/housing/original_zon_static_actor.h"
#include "../common/housing/original_prepared_furniture.h"
#include "../common/housing/original_furniture_source_provider.h"
#include "../common/housing/original_placement_frame.h"
#include "../common/housing/original_height_preimage_scene_adapter.h"
#include "../common/housing/placement_contact.h"
#include "../common/eqemu_logsys.h"
#include "../common/housing/placed_npc_height_filters.h"
#include "../common/housing/npc_item_metadata_db.h"
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
namespace Housing::HeightScene {
namespace H=OriginalHeightScene;namespace S=OriginalHeightSceneQuery;
namespace P=OriginalHeightPreimage;namespace Z=OriginalZonStaticActor;
namespace A=OriginalHeightPreimageSceneAdapter;
namespace {
std::vector<uint8_t> Read(std::string_view digest){
 if(!H::Sha(digest))return {};const char* configured=std::getenv("EQEMU_HOUSING_ORIGINAL_ASSETS");
 const std::string root=configured&&*configured?configured:"housing-original-meshes";
 std::ifstream in(root+"/"+std::string(digest)+".mesh",std::ios::binary|std::ios::ate);if(!in)return {};auto length=in.tellg();if(length<=0||length>64*1024*1024)return {};std::vector<uint8_t> bytes(static_cast<size_t>(length));in.seekg(0);if(!in.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))return {};return bytes;
}
// Cache only authenticated immutable geometry. Current actor/SQL/publication
// identity is freshly bound below on every synchronous admission.
struct PlacedResource {
 std::shared_ptr<const OriginalPreparedFurniture::Prepared> legacy;
 std::shared_ptr<const OriginalFurnitureSource::Prepared> ordinary;
 std::shared_ptr<const OriginalSimpleTreeQuery::Tree> tree;
 std::shared_ptr<const OriginalSimpleLinearQuery::Source> linear;
 const OriginalCachedBox::Box* Cache()const{if(ordinary)return &ordinary->Cache();if(legacy&&legacy->SelectedCache())return &*legacy->SelectedCache();return nullptr;}
 std::shared_ptr<const OriginalPreparedFurniture::QueryReceipt> Candidates(const OriginalActorLocalQuery::Matrix&matrix,float scale,const H::Query&q)const{
  if(legacy)return legacy->PlacedHeightCandidates(matrix,scale,q);if(!ordinary)return {};
  auto out=std::make_shared<OriginalPreparedFurniture::QueryReceipt>();out->item_id=ordinary->ItemId();out->matrix=matrix;out->scale=scale;out->mesh=ordinary->MeshOwner();out->tree=tree;out->linear=linear;
  if(tree){auto delta=S::Delta(q);if(!delta)return {};auto frame=OriginalActorLocalQuery::Build(matrix,scale,q.start,*delta,q.radius);if(!frame)return {};auto local=OriginalActorLocalQuery::SphereDelta(frame->start,frame->delta);if(!local)return {};auto candidates=OriginalSimpleTreeQuery::Enumerate(*tree,frame->start,*local,frame->radius);if(candidates.status!=OriginalSimpleTreeQuery::Status::Ready)return {};out->profile=OriginalPreparedFurniture::Profile::GeneratedPlainTreeV1;out->tree_query=S::ActorSource::Enumeration{q.start,*delta,q.radius,std::move(candidates.polygon_ids)};}
  else if(linear){auto receipt=S::LinearCandidates(*linear,{true,true,false},matrix,scale,q);if(!receipt)return {};out->profile=OriginalPreparedFurniture::Profile::OriginalPlainLinearV1;out->linear_query=std::move(receipt);}else return {};return out;
 }
};
std::mutex geometry_mutex;
std::map<uint32_t,std::shared_ptr<const PlacedResource>> furniture_cache;
std::map<std::string,std::shared_ptr<const Z::Resource>> authored_cache;
const H::MeshCatalogEntry* MeshEntry(uint32_t item){if(auto wall=OriginalFurnitureSource::Source(item))return wall;for(const auto&m:H::OriginalMeshCatalog)if(m.item_id==item)return &m;return nullptr;}
std::shared_ptr<const PlacedResource> Furniture(uint32_t item){
 std::lock_guard lock(geometry_mutex);auto found=furniture_cache.find(item);if(found!=furniture_cache.end())return found->second;auto catalog=MeshEntry(item);if(!catalog)return {};auto bytes=Read(catalog->mesh_sha256);auto prepared=std::make_shared<PlacedResource>();
 if(OriginalFurnitureSource::Source(item)){
  prepared->ordinary=OriginalFurnitureSource::Prepared::Read(item,bytes);if(!prepared->ordinary)return {};prepared->tree=prepared->ordinary->TreeOwner();prepared->linear=prepared->ordinary->LinearOwner();if(bool(prepared->tree)==bool(prepared->linear))return {};
 }else{prepared->legacy=OriginalPreparedFurniture::Prepared::Read(item,bytes);if(!prepared->legacy)return {};}
 furniture_cache[item]=prepared;return prepared;
}
std::shared_ptr<const Z::Resource> Authored(const Z::Profile&p){std::lock_guard lock(geometry_mutex);auto found=authored_cache.find(std::string(p.mesh.mesh_sha256));if(found!=authored_cache.end())return found->second;auto prepared=Z::Provider::ReadResource(p.zone,p.placement_index,Read(p.mesh.mesh_sha256));if(prepared)authored_cache[std::string(p.mesh.mesh_sha256)]=prepared;return prepared;}
P::Digest Hash(std::span<const uint8_t> bytes){P::Digest digest{};unsigned n=0;if(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&n,EVP_sha256(),nullptr)!=1||n!=32)return {};return digest;}
std::string PoseSha(const PlacementPolicy::Transform&p){std::array<float,7> words{p.position[0],p.position[1],p.position[2],p.heading,p.pitch,p.roll,p.scale};return Z::Hex(Hash({reinterpret_cast<const uint8_t*>(words.data()),sizeof(words)}));}
// This fingerprint is current server-owned publication/census identity, not a
// native process memory assertion. No callback yields or mutates the world.
std::string PublishedDoorName(const Door_Struct&door){return std::string(door.name,strnlen(door.name,sizeof(door.name)));}
// Bind to the actual recipient publication, not a DBID or EntityList key. A
// stale, duplicated, moved or partially known publication remains represented
// as an Unknown actor and cannot inherit a filter or resource from a new row.
Doors* CurrentPublishedDoor(Client&client,uint32_t id,const ActorPublication::DoorRecord&record){
 if(!record.closed_resource||id!=record.door.doorId)return nullptr;
 Doors*found=nullptr;for(const auto&[entity_id,door]:entity_list.GetDoorsList()){(void)entity_id;if(!door)return nullptr;if(door->GetDoorID()!=id)continue;if(found)return nullptr;found=door;}
 if(!found||!(found->GetClientVersionMask()&client.ClientVersionBit()))return nullptr;
 const auto&wire=record.door;const auto&position=found->GetPosition();
 if(PublishedDoorName(wire)!=found->GetDoorName()||PublishedDoorName(wire).size()<=3||wire.opentype!=found->GetOpenType()||wire.size!=found->GetSize()||wire.incline!=uint32_t(found->GetIncline())||wire.invert_state!=found->GetInvertState()||wire.door_param!=uint32_t(found->GetDoorParam())||wire.state_at_spawn!=uint8_t(found->GetInvertState()?!found->IsDoorOpen():found->IsDoorOpen()))return nullptr;
 if(!OriginalSphereSource::Bits({wire.yPos,wire.xPos,wire.zPos},{position.y,position.x,position.z})||std::bit_cast<uint32_t>(wire.heading)!=std::bit_cast<uint32_t>(position.w))return nullptr;
 return found;
}
std::string Census(Client&client){std::ostringstream out;out<<zone->GetZoneID()<<':'<<zone->GetInstanceID()<<':'<<ActorPublication::Revision(client)<<';';
 std::map<uint16_t,Mob*> mobs(entity_list.GetMobList().begin(),entity_list.GetMobList().end());for(const auto&[id,m]:mobs){if(!m)return {};out<<'m'<<id<<':'<<m->GetRace()<<':'<<unsigned(m->GetGender())<<':'<<unsigned(m->GetBodyType())<<':'<<std::bit_cast<uint32_t>(m->GetSize())<<':'<<m->GetHousingEstateId()<<':'<<m->GetHousingItemId()<<':'<<m->GetHousingItemGuid()<<';';}
 for(const auto&[id,c]:entity_list.GetCorpseList()){if(!c)return {};out<<'c'<<id<<';';}
 const auto&publications=ActorPublication::Publication(client);if(!publications.DoorsPublished())return {};out<<'D'<<publications.Revision()<<';';for(const auto&[id,record]:publications.PublishedDoors()){const auto&d=record.door;out<<'p'<<id<<':'<<record.generation<<':'<<record.closed_resource<<':'<<PublishedDoorName(d)<<':'<<unsigned(d.doorId)<<':'<<unsigned(d.opentype)<<':'<<d.size<<':'<<d.incline<<':'<<unsigned(d.invert_state)<<':'<<unsigned(d.state_at_spawn)<<':'<<d.door_param<<':'<<std::bit_cast<uint32_t>(d.xPos)<<':'<<std::bit_cast<uint32_t>(d.yPos)<<':'<<std::bit_cast<uint32_t>(d.zPos)<<':'<<std::bit_cast<uint32_t>(d.heading)<<';';}
 std::map<uint16_t,Doors*> doors(entity_list.GetDoorsList().begin(),entity_list.GetDoorsList().end());for(const auto&[id,d]:doors){if(!d)return {};out<<'d'<<id<<':'<<d->GetDoorDBID()<<':'<<d->GetDoorName()<<':'<<unsigned(d->GetOpenType())<<':'<<d->GetSize()<<':'<<d->GetIncline()<<':'<<d->GetInvertState()<<':'<<d->IsDoorOpen()<<':'<<d->GetDoorParam()<<':'<<d->GetClientVersionMask()<<':'<<std::bit_cast<uint32_t>(d->GetPosition().x)<<':'<<std::bit_cast<uint32_t>(d->GetPosition().y)<<':'<<std::bit_cast<uint32_t>(d->GetPosition().z)<<':'<<std::bit_cast<uint32_t>(d->GetPosition().w)<<';';}
 std::map<uint16_t,Object*> objects(entity_list.GetObjectList().begin(),entity_list.GetObjectList().end());for(const auto&[id,o]:objects){if(!o)return {};out<<'o'<<id<<':'<<o->GetType()<<':'<<o->GetModelName()<<';';}return out.str();
}
std::optional<OriginalActorLocalQuery::Matrix> Matrix(const PlacementPolicy::Transform&p){auto tables=OriginalActorLocalQuery::OriginalRendererTables();if(!tables)return {};return OriginalActorLocalQuery::MatrixFromRendererAngles(p.position,{p.heading,p.pitch,p.roll},*tables);}
// Exact reviewed original Standard-Hall closed GuildDoor publication profile.
// Server MakeDoorSpawnPacket transmits these position/heading/size/incline and
// closed/invert fields directly. GetDoorParam is signed32: the exact DB -1
// sentinel becomes uint32 UINT32_MAX in Door_Struct and RoF2 copies it intact.
// Passive original class evidence binds this
// profile to OBJ_GUILD_DOOR plain static geometry, type3/mask2/volume1 and
// native scale0.99999994 (wire100 * original float.01, <=1). This is a source
// bound, not an actor exclusion or a type3 collision solver. Changed/open or
// nearby sources deliberately retain the ordinary Unknown refusal.
std::optional<H::Bounds> ClosedGuildDoorBounds(Client&client,Doors&door){
 if(!zone||zone->GetZoneID()!=345||zone->GetInstanceVersion()!=0||!(door.GetClientVersionMask()&client.ClientVersionBit())||door.GetOpenType()!=58||door.GetSize()!=100||door.GetIncline()!=0||door.GetInvertState()!=0||door.IsDoorOpen()||door.GetDoorParam()!=-1)return {};
 std::string name=door.GetDoorName();if(name!="OBJ_GUILD_DOOR"&&name!="GUILD_DOOR")return {};
 const auto&position=door.GetPosition();const H::Point anchor{position.y,position.x,position.z};
 if(!OriginalSphereSource::Bits(anchor,{-162.9f,-3.4f,6.9f})||std::bit_cast<uint32_t>(position.w)!=std::bit_cast<uint32_t>(128.f))return {};
 const auto*profile=Z::Provider::Find("guildhall",26);if(!profile||profile->mesh.model!="OBJ_GUILD_DOOR"||profile->vertices!=192||profile->polygons!=246||!profile->raw_low16_zero)return {};
 // Validate the original ordinary class and exact immutable mesh identity;
 // no server/item/packet bounds or unverified raw render AABB is trusted.
 if(!Authored(*profile))return {};auto mesh=Z::Provider::Geometry(profile->zone,profile->placement_index,Read(profile->mesh.mesh_sha256));if(!mesh)return {};
 double maximum=0;for(const auto&vertex:mesh->vertices){double sum=0;for(float value:vertex){if(!std::isfinite(value))return {};sum=std::nextafter(sum+std::abs(double(value)),INFINITY);}maximum=std::max(maximum,sum);}
 if(!std::isfinite(maximum)||maximum<=0||maximum>4096)return {};
 // L1 bounds every component after any rigid orientation at this anchor.
 // The pinned size100 profile has scale<=1. Integer outward rounding plus a
 // full world-unit allowance covers original stored transform arithmetic;
 // vertices and this fixed anchor are bounded well inside binary32 range.
 const double radius=std::ceil(maximum)+1;H::Bounds bounds;for(unsigned k=0;k<3;++k){bounds.minimum[k]=std::nextafter(double(anchor[k])-radius,-INFINITY);bounds.maximum[k]=std::nextafter(double(anchor[k])+radius,INFINITY);}return bounds;
}
struct Owned {
 std::shared_ptr<H::Snapshot> scene=std::make_shared<H::Snapshot>();
 std::map<uint64_t,std::shared_ptr<const H::Source>> terrain;
 std::map<uint64_t,std::shared_ptr<const S::ActorSource>> authored;
 struct Furniture {std::shared_ptr<const PlacedResource> source;OriginalActorLocalQuery::Matrix matrix;float scale{};};
 std::map<uint64_t,Furniture> furniture;
};
}
Result Admit(Client& client,Database&db,const LiveItems::Lease&lease,const Scene::InteriorSnapshot&room,const LiveItems::MountSubject&subject,const LiveItems::InventoryRow&selected,const PlacementPolicy::Transform&proposed,const MountFlags::Flags&flags,LiveItems::InventorySessionFence&fence)noexcept{
 Result out;try{
  if(!zone||client.ClientVersion()!=EQ::versions::ClientVersion::RoF2||!fence.HeldFor(lease.actor,lease.epoch,lease.session_key)||!subject.Valid()||!room.house||!room.house->interior||!room.room_profile||!room.profile_revision)return out;
  auto transaction=db.QueryDatabase(std::string("SELECT @@in_transaction"),false);if(!transaction.Success()||transaction.RowCount()!=1||!transaction.begin()[0]||std::string(transaction.begin()[0])!="1")return out;
  out.stage=1;const auto&profile=*room.room_profile;if(profile.route.zone_id!=zone->GetZoneID()||profile.route.instance_id!=zone->GetInstanceID()||profile.route.zone_version!=zone->GetInstanceVersion()||profile.zone_short_name!=zone->GetShortName()||!profile.reviewed_room)return out;
  const H::TerrainCatalogEntry* tc=nullptr;for(const auto&t:H::OriginalTerrainCatalog)if(t.zone_id==zone->GetZoneID()&&t.zone==profile.zone_short_name)tc=&t;if(!tc||profile.client_asset_sha256!=tc->mesh.archive_sha256||!ActorPublication::Revision(client))return out;
  const auto*selected_model=StaticFurniture::Lookup(selected.values.item_id);if(!selected_model)return out;
  if(flags.known&&!flags.disable_free){out.status=Status::NotApplicable;return out;}
  if(selected_model->mounting_type!=1&&selected_model->mounting_type!=2)return out;
  if(selected_model->mounting_type==1&&(!flags.known||flags.disable_rotation)){out.status=Status::NotApplicable;return out;}
  out.stage=2;auto selected_source=Furniture(selected.values.item_id);if(!selected_source||!selected_source->Cache())return out;
  auto selected_frame=OriginalPlacementFrame::Build(*selected_source->Cache(),{proposed.scale,{proposed.heading,proposed.pitch,proposed.roll},proposed.position});if(!selected_frame)return out;
  out.stage=3;const auto census=Census(client);if(census.empty())return out;const auto thread=std::this_thread::get_id();
  auto own=std::make_shared<Owned>();auto&scene=*own->scene;scene.epoch=lease.epoch;scene.encoded_zone=zone->GetZoneID();scene.excluded_graphics_global=0;
  // Abstract app identity mirrors codec native_item_id. Add has no published
  // self actor; Move excludes exactly its locked native source, never itemtype.
  scene.placed_context=subject.kind==LiveItems::MountSubject::Kind::Move?subject.native_item_id:(uint64_t(1)<<60);
  uint64_t next=1;auto entry=[&](std::string_view resource,std::string_view frame,std::string_view manifest,uint64_t rev){H::Entry e;e.identity={scene.epoch,next++,rev,std::string(resource),std::string(frame)};e.expected_manifest_sha256=manifest;e.disabled=false;e.graphics_identity=e.identity.actor;return e;};
  auto terrain_entry=entry(tc->mesh.member_sha256,tc->world_frame_sha256,tc->mesh.mesh_sha256,1);terrain_entry.terrain=true;auto terrain=H::ReadTerrainSource(tc->zone_id,tc->zon_sha256,terrain_entry,Read(tc->mesh.mesh_sha256));if(!terrain)return out;own->terrain[terrain_entry.identity.actor]=terrain;scene.entries.push_back(terrain_entry);
  const auto add_authored=[&](const Z::Profile&p)->bool{if(p.zone!=tc->zone)return true;auto e=entry(p.mesh.member_sha256,Z::FrameSha(p),p.mesh.mesh_sha256,1);e.actor.type=5;e.actor.application_identity=0;e.actor.restriction_mask=0;e.actor.volume=0;auto source=Z::Provider::BindAuthored(p.zone,p.placement_index,p.zon_sha256,Z::RendererSha,e,Authored(p));if(!source)return false;own->authored[e.identity.actor]=source;scene.entries.push_back(std::move(e));return true;};
  out.stage=4;for(const auto&p:Z::PersonalDoors)if(!add_authored(p))return out;for(const auto&p:Z::GuildAuthoredPlacements)if(!add_authored(p))return out;
  out.stage=5;bool matched_subject=subject.kind==LiveItems::MountSubject::Kind::Add;
  // A transfer/unstow source may be outside this destination's visible census.
  // Bind its exact existing SQL identity without inventing a destination actor.
  if(subject.kind==LiveItems::MountSubject::Kind::Move){auto source=db.QueryDatabase("SELECT native_item_id,pose_revision FROM housing_live_placed_items WHERE custody_id="+std::to_string(subject.custody_id)+" FOR UPDATE",false);if(!source.Success()||source.RowCount()!=1||!source.begin()[0]||!source.begin()[1]||std::stoul(source.begin()[0])!=subject.native_item_id||std::stoull(source.begin()[1])!=subject.pose_revision)return out;matched_subject=true;}
  std::set<uint32_t> native_ids;std::map<uint32_t,std::optional<NpcItemMetadata::Source>> npc_metadata;
  std::map<uint32_t,PlacedHeightFilters::Actor> linked_service_actors;
  for(const auto&content:room.contents){const auto&row=content.stored;if(row.stowed||content.contained_in_crate||!row.placement)continue;const auto&p=*row.placement;const auto native=p.reservation.native_item_id;
   if(!native||!native_ids.insert(native).second||!content.room||!p.pose_revision)return out;
   if(subject.kind==LiveItems::MountSubject::Kind::Move&&native==subject.native_item_id){if(row.custody_id!=subject.custody_id||p.pose_revision!=subject.pose_revision||row.row.values.item_id!=selected.values.item_id)return out;matched_subject=true;}
   const auto item=row.row.values.item_id;if(!npc_metadata.count(item))npc_metadata[item]=NpcItemMetadata::ReadUnderTransaction(db,item);const auto&npc_source=npc_metadata[item];
   if(npc_source&&!NpcItemMetadata::Ordinary(*npc_source)){
    auto npc=NpcRuntime::Review(*npc_source);
    if(npc&&npc->service){
     // Interactive contracts have ONE actual published network NPC. They do
     // not create the received noninteractive CPlacedNPC's local graphics.
     // A missing/duplicate/stale binding remains Unknown, never omission.
     size_t matches=0;uint32_t spawn=0;std::optional<PlacedHeightFilters::Actor> actor;
     for(const auto&[id,record]:ActorPublication::Publication(client).Records()){
      if(record.spawn.housing_estate_id!=p.reservation.native_estate_id||record.spawn.housing_item_id!=native)continue;
      ++matches;spawn=id;if(matches>1)break;
      auto*mob=id<=UINT16_MAX?entity_list.GetMob(uint16_t(id)):nullptr;auto bound=ServiceNpcs::LiveBinding(mob);
      if(!bound||bound->custody_id!=row.custody_id||bound->estate_id!=row.estate_id||bound->house_id!=room.house->house_id||bound->item_id!=item||bound->native_item_id!=native||bound->native_estate_id!=p.reservation.native_estate_id||!fence.NpcProfileMatchesUnderTransaction(db,lease,*npc)||!ServiceAuthority::Validate(db,*bound,nullptr))continue;
      // Validate uses THIS custody transaction. Find/Bindings would open a
      // second locking connection and can deadlock on our own source rows.
      actor=ActorPublication::ProjectServiceHeightActor(client,uint16_t(id),*npc,row.custody_id,p.reservation.native_estate_id,native);
     }
     if(matches==1&&actor&&linked_service_actors.emplace(spawn,*actor).second)continue;
     scene.entries.push_back(entry({},PoseSha(p.transform),{},p.pose_revision));continue;
    }
    auto e=entry({},PoseSha(p.transform),{},p.pose_revision);if(npc&&fence.NpcProfileMatchesUnderTransaction(db,lease,*npc)){auto projected=PlacedNpcHeightFilters::Project(*npc,p.transform,p.npc_frame,native);if(projected)e.actor=*projected;}scene.entries.push_back(std::move(e));continue;
   }
   const H::MeshCatalogEntry* mesh=MeshEntry(item);
   auto e=entry(mesh?mesh->member_sha256:std::string_view{},PoseSha(p.transform),mesh?mesh->mesh_sha256:std::string_view{},p.pose_revision);e.actor.type=9;e.actor.application_identity=native;e.actor.restriction_mask=2;e.actor.volume=1;
   if(mesh){auto prepared=Furniture(row.row.values.item_id);auto matrix=Matrix(p.transform);if(prepared&&matrix)own->furniture[e.identity.actor]={std::move(prepared),*matrix,p.transform.scale};}
   scene.entries.push_back(std::move(e));
  }
  if(!matched_subject)return out;
  // Per-recipient outgoing publications define dynamic actor presence.
  // Current server entities bind their state but cannot invent unpublished actors.
  out.stage=6;std::set<uint32_t> dynamic_ids;for(const auto&[id,mob]:entity_list.GetMobList()){if(!mob)return out;dynamic_ids.insert(id);}for(const auto&[id,corpse]:entity_list.GetCorpseList()){if(!corpse)return out;dynamic_ids.insert(id);}for(const auto&[id,record]:ActorPublication::Publication(client).Records()){(void)record;dynamic_ids.insert(id);}
  for(auto id:dynamic_ids){const auto*published=ActorPublication::Publication(client).Find(id);if(!published)continue;auto e=entry({}, {}, {},published->generation);
   auto linked=linked_service_actors.find(id);
   if(linked!=linked_service_actors.end())e.actor=linked->second;
   else if(id<=UINT16_MAX){
    auto*mob=entity_list.GetMob(uint16_t(id));const auto&spawn=published->spawn;
    const bool bound=spawn.housing_estate_id&&spawn.housing_estate_id!=UINT32_MAX&&spawn.housing_item_id&&spawn.housing_item_id!=UINT32_MAX;
    // Unsupported housing-bound network actors never borrow ordinary player
    // projection and never manufacture a second local placed-NPC actor.
    if(!bound&&!ServiceNpcs::Managed(mob)){auto projected=ActorPublication::ProjectHeightActor(client,uint16_t(id));if(projected)e.actor=*projected;}
   }scene.entries.push_back(std::move(e));}
  const auto&door_publications=ActorPublication::Publication(client);if(!door_publications.DoorsPublished())return out;
  for(const auto&[id,record]:door_publications.PublishedDoors()){auto e=entry({}, {}, {},record.generation);auto*door=CurrentPublishedDoor(client,id,record);if(door){e.actor.type=3;e.actor.application_identity=uint64_t(id)+1;e.actor.restriction_mask=2;e.actor.volume=1;e.actor.switch_data=PlacedHeightFilters::Switch{record.door.opentype,PublishedDoorName(record.door)};e.conservative_world_bounds=ClosedGuildDoorBounds(client,*door);}scene.entries.push_back(std::move(e));}
  for(const auto&[id,object]:entity_list.GetObjectList()){(void)id;if(!object)return out;scene.entries.push_back(entry({}, {}, {},1));}
  if(scene.entries.size()>16384||Census(client)!=census)return out;
  // Completion is set only here, after the trusted code has enumerated static
  // original resources, every published committed room row and live entities.
  // Unsupported represented actors remain Unknown in the normal filter/solver.
  out.stage=7;scene.complete=true;std::vector<uint8_t> canonical(census.begin(),census.end());for(const auto&e:scene.entries){const auto text=std::to_string(e.identity.actor)+":"+std::to_string(e.identity.pose_revision)+":"+e.identity.resource_sha256+":"+e.identity.frame_sha256+":"+e.expected_manifest_sha256+";";canonical.insert(canonical.end(),text.begin(),text.end());}auto digest=Hash(canonical);if(!P::Nonzero(digest))return out;
  A::Frozen frozen;frozen.scene=own->scene;frozen.expected={digest,scene.epoch};frozen.terrain=[own](const H::Entry&e){auto it=own->terrain.find(e.identity.actor);return it==own->terrain.end()?nullptr:it->second;};
  frozen.actors=[own](const H::Entry&e,const H::Query&q)->std::shared_ptr<const S::ActorSource>{auto a=own->authored.find(e.identity.actor);if(a!=own->authored.end())return Z::Provider::ForQuery(a->second,e,q);auto f=own->furniture.find(e.identity.actor);if(f==own->furniture.end())return {};auto receipt=f->second.source->Candidates(f->second.matrix,f->second.scale,q);if(!receipt)return {};auto source=std::make_shared<S::ActorSource>();source->identity=e.identity;source->manifest_sha256=e.expected_manifest_sha256;source->original_actor_matrix=receipt->matrix;source->scale=receipt->scale;source->mesh=receipt->mesh;source->dispatch=receipt->tree?S::ActorSource::Dispatch::PlainSimpleAccelerated:S::ActorSource::Dispatch::PlainSimpleLinear;source->linear_source=receipt->linear;source->native_tree_query=receipt->tree_query;source->native_linear_query=receipt->linear_query;return source;};
  frozen.attest=[&,thread,expected=frozen.expected,scene_owner=own->scene](const H::Snapshot&s)->std::optional<P::Snapshot>{if(&s!=scene_owner.get()||std::this_thread::get_id()!=thread||!fence.HeldFor(lease.actor,lease.epoch,lease.session_key)||Census(client)!=census)return {};return expected;};
  P::Policy policy;policy.snapshot=frozen.expected;policy.review=Hash(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(tc->zon_sha256.data()),tc->zon_sha256.size()));policy.minimum={-32766,-32766,-32766};policy.maximum={32766,32766,32766};policy.queries=16;policy.xy_probes=2;policy.z_levels=2;policy.source_limit=65536;policy.narrowphase=8192;policy.operations=4000000;
  // Search a bounded server-derived envelope around the exact submitted anchor.
  // It is a scheduling envelope, never an alternative collision tolerance.
  auto anchor=OriginalActorQueryFrame::ScreenAnchor(proposed.position,selected_frame->boxes.frame,selected_model->mounting_type==2?H::Mode::Ceiling:H::Mode::Floor);if(!anchor)return out;policy.minimum[2]=std::max(-32766.f,(*anchor)[2]-64);policy.maximum[2]=std::min(32766.f,(*anchor)[2]+64);
  PlacementContact::Profile contact{selected_model->mounting_type,flags.disable_rotation,flags.disable_free,false,true};
  const auto mode=selected_model->mounting_type==2?H::Mode::Ceiling:H::Mode::Floor;
  const auto normal=[&](const H::Point&n){return PlacementContact::NormalAllowed(contact,PlacementContact::Mode::Normal,n);};
  // Same before/after attestation contract as the pure adapter, retaining the
  // actual blocked-source diagnostic before reducing the scene result.
  P::Query complete=[&](const OriginalActorQueryFrame::HeightRequest&request,OriginalSphereSource::Limits limits)->P::Reply{
   P::Reply reply;const auto before=frozen.attest(*frozen.scene);if(!before)return reply;reply.snapshot=*before;if(!P::Same(*before,frozen.expected))return reply;
   auto height=S::HeightQuery(*frozen.scene,request,frozen.terrain,frozen.actors,limits);reply.height=A::Reduce(height);
   if(height.sweep.status==S::Status::Unknown||height.sweep.status==S::Status::Invalid){out.blocked_source=height.sweep.blocked_actor;for(const auto&e:scene.entries)if(e.identity.actor==out.blocked_source){LogInfo("Housing original height blocked source [{}] type [{}] app [{}] volume [{}] restrictions [{}]",out.blocked_source,e.actor.type?int(*e.actor.type):-1,e.actor.application_identity?std::to_string(*e.actor.application_identity):"unknown",e.actor.volume?int(*e.actor.volume):-1,e.actor.restriction_mask?int(*e.actor.restriction_mask):-1);break;}}
   const auto after=frozen.attest(*frozen.scene);if(!after){LogInfo("Housing original height source census changed during query");return reply;}reply.snapshot=*after;if(!P::Same(*after,frozen.expected))return reply;
   reply.complete=height.sweep.status==S::Status::Hit||height.sweep.status==S::Status::Clear||height.sweep.status==S::Status::Ambiguous||height.sweep.status==S::Status::BudgetExceeded;return reply;
  };
  out.stage=8;uint32_t spent_queries=0,spent_tests=0;uint64_t spent_steps=0;
  const auto save=[&](){out.queries=spent_queries;out.narrowphase=spent_tests;out.kernel_steps=spent_steps;};
  const auto remaining=[&]()->std::optional<OriginalSphereSource::Limits>{if(spent_queries>=policy.queries||spent_tests>=policy.narrowphase||spent_steps>policy.operations||policy.operations-spent_steps<48000)return {};return OriginalSphereSource::Limits{policy.source_limit,policy.narrowphase-spent_tests,policy.operations-spent_steps};};
  std::array<P::Interval,2> xy;for(unsigned k=0;k<2;++k){auto interval=P::Preimages(proposed.position[k],selected_frame->boxes.frame.center[k],policy.minimum[k],policy.maximum[k]);if(!interval)return out;xy[k]=*interval;}
  const auto exact=[&](const OriginalActorQueryFrame::HeightRequest&request,const P::Height&height){return height.valid&&height.sweep&&height.sweep.contact.fraction>0&&height.sweep.contact.fraction<=1&&normal(height.sweep.contact.normal)&&P::Matches(proposed.position,selected_frame->boxes.frame,request,height,xy);};
  const auto charge=[&](const P::Reply&reply,const OriginalSphereSource::Limits&limit)->bool{++spent_queries;const auto&w=reply.height.sweep;if(w.narrowphase>limit.narrowphase||w.operations>limit.operations||w.sources>limit.sources)return false;spent_tests+=w.narrowphase;spent_steps+=w.operations;save();if(P::Nonzero(reply.snapshot.digest)&&!P::Same(reply.snapshot,policy.snapshot)){out.status=Status::SourceChanged;return false;}return reply.complete&&P::Same(reply.snapshot,policy.snapshot);};
  OriginalSphereQuery::Detail::Rounding env;if(!env.good)return out;
  // Near-anchor first, instead of spending full-room kernels on a remote Clear
  // query below the floor. These are QUERY INPUTS, not placement tolerances.
  OriginalActorQueryFrame::HeightRequest seed{mode,{P::Value(xy[0].first),P::Value(xy[1].first),OriginalActorQueryFrame::Detail::Store(OriginalActorQueryFrame::Detail::Add((*anchor)[2],mode==H::Mode::Floor?1.:-1.))},selected_frame->boxes.frame.radius,0,1};
  if(seed.input[2]<policy.minimum[2]||seed.input[2]>policy.maximum[2]){out.status=Status::Inconclusive;return out;}
  const auto seed_limit=remaining();if(!seed_limit)return out;auto seed_reply=complete(seed,*seed_limit);if(!charge(seed_reply,*seed_limit))return out;
  if(exact(seed,seed_reply.height)){out.status=Status::Witness;return out;}
  // Candidate generation only: when the actual nearest source is terrain,
  // cheaply explore that one immutable primitive's floating-point startZ
  // preimages. Never admit from this single-primitive speculation. Every
  // candidate must pass a NEW complete nearest/filter/normal/exact-forward
  // query, including every original authored and published dynamic source.
  if(seed_reply.height.valid&&seed_reply.height.sweep){
   const auto id=seed_reply.height.sweep.source_id;auto terrain_owner=own->terrain.find(id>>32);
   if(terrain_owner!=own->terrain.end()){
    const auto index=uint32_t(id);const auto&polygons=terrain_owner->second->polygons;auto polygon=std::find_if(polygons.begin(),polygons.end(),[&](const auto&v){return v.index==index;});
    if(polygon!=polygons.end()){
     const OriginalSphereSource::Triangle triangle{id,polygon->native_abc};
     for(unsigned probe=1;probe<=128;++probe){
      if(spent_tests>=policy.narrowphase||policy.operations-spent_steps<48000)break;
      auto candidate=seed;const double offset=double(probe)*.125;candidate.input[2]=OriginalActorQueryFrame::Detail::Store(OriginalActorQueryFrame::Detail::Add((*anchor)[2],mode==H::Mode::Floor?offset:-offset));
      if(candidate.input[2]<policy.minimum[2]||candidate.input[2]>policy.maximum[2])continue;
      const auto speculation=OriginalActorQueryFrame::Height(std::span<const OriginalSphereSource::Triangle>(&triangle,1),candidate,{1,1,policy.operations-spent_steps});
      if(speculation.sweep.narrowphase>1||speculation.sweep.operations>policy.operations-spent_steps)return out;
      spent_tests+=speculation.sweep.narrowphase;spent_steps+=speculation.sweep.operations;save();
      if(!exact(candidate,P::Height(speculation)))continue;
      const auto limit=remaining();if(!limit)break;const auto verified=complete(candidate,*limit);if(!charge(verified,*limit))return out;
      if(exact(candidate,verified.height)){out.status=Status::Witness;return out;}
     }
    }
   }
  }
  // Preserve the bounded general schedule for sources whose exact preimage was
  // not found above, charging the seed/speculation against the SAME total caps.
  const auto left=remaining();if(!left){out.status=Status::Inconclusive;return out;}
  policy.queries-=spent_queries;policy.narrowphase-=spent_tests;policy.operations-=spent_steps;
  const auto result=P::Find(proposed.position,selected_frame->boxes.frame,mode,policy,complete,[&](H::Mode,const H::Point&n){return normal(n);});
  out.queries=spent_queries+result.queries;out.narrowphase=spent_tests+result.narrowphase;out.kernel_steps=spent_steps+result.operations;out.status=result.status==P::Status::Witness?Status::Witness:result.status==P::Status::SourceChanged?Status::SourceChanged:result.status==P::Status::Inconclusive?Status::Inconclusive:Status::Unavailable;return out;
 }catch(...){return out;}
}
}
