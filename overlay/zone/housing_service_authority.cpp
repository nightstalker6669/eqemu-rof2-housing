#include "housing_service_authority.h"
#include "housing_house_runtime.h"
#include "housing_item_reconstruction.h"
#include "inventory_session_owner.h"
#include "client.h"
#include "zone.h"
#include "../common/eqemu_config.h"
#include "../common/patches/rof2.h"
#include "../common/housing/guild_portal_db.h"
#include "../common/housing/custody_codec.h"
#include "../common/housing/item_storage_db.h"
#include "../common/housing/crate_member_scope_db.h"
#include "../common/housing/npc_runtime_db.h"
#include "../common/housing/npc_native_frame_db.h"
#include "../common/housing/interior_item_content_db.h"
#include "../common/housing/rent_state_db.h"
#include <map>
#include <cmath>
#include <cstdio>
extern Zone* zone;
extern ZoneDatabase database;
namespace Housing::ServiceAuthority {
namespace {
// Zone-thread-only retained committed witnesses. Each use is checked against
// current locked DB rows; these snapshots never grant inventory authority.
struct Witness {InteriorContent::Profile room;uint64_t revision=0;LiveItems::Stored stored;uint64_t scope_revision=0,house_revision=0;};
std::map<uint64_t,Witness> witnesses;
bool portal_configured=false;
using Bytes=std::vector<uint8_t>;
auto Q(Database& db,const std::string& s){return GuildPortal::Query(db,s);}
uint64_t N(const char* s){return GuildPortal::Number(s);}
void Require(bool b){if(!b)throw std::invalid_argument("Housing service source changed");}
std::string Hex(const Bytes& bytes){const char* h="0123456789ABCDEF";std::string s;for(auto b:bytes){s+=h[b>>4];s+=h[b&15];}return s;}
Bytes Unhex(const char* value,size_t maximum=131300){Require(value);std::string s=value;Require(s.size()%2==0&&s.size()<=maximum*2);Bytes bytes;auto d=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;throw std::invalid_argument("Invalid housing source hex");};for(size_t i=0;i<s.size();i+=2)bytes.push_back(uint8_t(d(s[i])*16+d(s[i+1])));return bytes;}
PlacementPolicy::Transform Pose(const Bytes& b){Require(b.size()==28);std::array<float,7> a{};for(size_t i=0;i<7;++i){uint32_t bits=uint32_t(b[4*i])|(uint32_t(b[4*i+1])<<8)|(uint32_t(b[4*i+2])<<16)|(uint32_t(b[4*i+3])<<24);std::memcpy(&a[i],&bits,4);Require(std::isfinite(a[i]));}return {a[0],a[1],a[2],a[3],{a[4],a[5],a[6]}};}
bool SamePose(const PlacementPolicy::Transform& a,const PlacementPolicy::Transform& b){return a.scale==b.scale&&a.heading==b.heading&&a.pitch==b.pitch&&a.roll==b.roll&&a.position==b.position;}
std::string Guid(uint32_t native){char s[17];std::snprintf(s,sizeof(s),"%016u",native);return s;}
ServiceNpcs::Service Kind(ServiceItemBindings::Kind k){using K=ServiceItemBindings::Kind;using S=ServiceNpcs::Service;switch(k){case K::Vendor:return S::Vendor;case K::TeleportMerchant:return S::TeleportMerchant;case K::Banker:return S::Banker;case K::GuildBank:return S::GuildBank;case K::GuildTribute:return S::GuildTribute;case K::PersonalTribute:return S::PersonalTribute;case K::TeleportPad:return S::TeleportPad;}throw std::invalid_argument("Unknown housing service");}
std::unique_ptr<Database> Connection(){const auto* c=EQEmuConfig::get();Require(c);auto db=std::make_unique<Database>();Require(db->Connect(c->DatabaseHost,c->DatabaseUsername,c->DatabasePassword,c->DatabaseDB,c->DatabasePort,"housing-service-authority"));return db;}
bool Check(const ServiceNpcs::Binding& b,Client* c){try{auto db=Connection();Q(*db,"START TRANSACTION");const bool valid=Validate(*db,b,c);Q(*db,"ROLLBACK");return valid;}catch(...){return false;}}
bool SameScope(const ServiceNpcs::Binding& a,const ServiceNpcs::Binding& b){return a.house_id==b.house_id&&a.estate_id==b.estate_id&&a.guild_id==b.guild_id&&a.native_estate_id==b.native_estate_id&&a.zone_id==b.zone_id&&a.instance_id==b.instance_id;}
bool Builtin(Database& db,const ServiceNpcs::Binding& b){
 if(b.zone_id!=345||b.item_id!=66640||b.service!=ServiceNpcs::Service::TeleportMerchant)return false;
 auto hall=Q(db,"SELECT h.item_template_id,i.zone_id,i.zone_version FROM housing_house_instances h JOIN housing_house_interiors i USING(house_id) WHERE h.house_id="+std::to_string(b.house_id)+" FOR UPDATE");if(hall.RowCount()!=1||N(hall.begin()[0])!=66618||N(hall.begin()[1])!=345||N(hall.begin()[2])!=0)return false;
 // This is the existing basic hall's actual authored geode/proximity source,
 // not an automatically created pad item or fabricated custody identity.
 auto original=Q(db,"SELECT s.id FROM spawn2 s JOIN spawnentry e ON e.spawngroupID=s.spawngroupID JOIN npc_types n ON n.id=e.npcID WHERE s.id=76577 AND s.zone='guildhall' AND s.version=1 AND e.npcID=345005 AND e.chance=100 AND n.race=240 AND n.gender=2 AND n.size=0 AND n.`class`=1 AND CAST(s.x AS DECIMAL(14,6))=-131.846436 AND CAST(s.y AS DECIMAL(14,6))=-23.525532 AND CAST(s.z AS DECIMAL(14,6))=1.487914 FOR UPDATE");if(original.RowCount()!=1)return false;
 auto items=Q(db,"SELECT c.custody_id,HEX(c.row_blob) FROM housing_live_item_custody c JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id LEFT JOIN housing_house_item_scopes scope ON scope.custody_id=c.custody_id WHERE r.state=1 AND (r.native_estate_id="+std::to_string(b.native_estate_id)+" OR scope.house_id="+std::to_string(b.house_id)+") ORDER BY c.custody_id LIMIT 10001 FOR UPDATE");Require(items.RowCount()<=10000);
 for(auto r=items.begin();r.operator!=(items.end());++r){auto row=CustodyCodec::Decode(Unhex(r[1]));if(row.values.item_id==66646&&!ItemStorage::Locked(db,N(r[0])).stowed&&!CrateMemberScope::Locked(db,N(r[0])))return false;}
 return true;
}
bool CheckPortal(Client& c,const GuildPortalRuntime::Source& source){try{auto db=Connection();Q(*db,"START TRANSACTION");const bool valid=ValidatePortalSource(*db,c,source);Q(*db,"ROLLBACK");return valid;}catch(...){return false;}}
void ClientAccess(Database& db,const ServiceNpcs::Binding& b,uint32_t native_plot,Client& c){
 Require(zone&&c.Connected()&&c.ClientDataLoaded()&&c.CharacterID()&&c.AccountID()&&c.HasInventorySessionReservation()&&zone->GetZoneID()==b.zone_id&&zone->GetInstanceID()==b.instance_id);
 auto ticket=c.InventorySessionReleaseTicket();Require(bool(ticket));auto session=ticket->SnapshotFor(c);Require(session&&ticket->HeldFor(c)&&session->active&&session->actor.character_id==c.CharacterID()&&session->actor.account_id==c.AccountID()&&session->owner.zone_id==b.zone_id&&session->owner.instance_id==b.instance_id);
 const auto id=std::to_string(c.CharacterID());auto active=Q(db,"SELECT account_id,epoch,HEX(token),zone_id,instance_id,active FROM housing_inventory_sessions WHERE character_id="+id+" FOR UPDATE");Require(active.RowCount()==1);auto a=active.begin();Require(N(a[0])==c.AccountID()&&N(a[1])==session->epoch&&Unhex(a[2],32)==Bytes(session->token.begin(),session->token.end())&&N(a[3])==b.zone_id&&N(a[4])==b.instance_id&&N(a[5])==1);
 auto pending=Q(db,"SELECT account_id,active,pending_key FROM housing_live_item_leases WHERE character_id="+id+" FOR UPDATE");Require(pending.RowCount()==1&&N(pending.begin()[0])==c.AccountID()&&N(pending.begin()[1])==1&&!pending.begin()[2]);
 auto member=Q(db,"SELECT charid FROM instance_list_player WHERE id="+std::to_string(b.instance_id)+" AND charid="+id+" FOR UPDATE");Require(member.RowCount()==1);
 if(b.service==ServiceNpcs::Service::GuildBank||b.service==ServiceNpcs::Service::GuildTribute||b.service==ServiceNpcs::Service::TeleportMerchant){auto guild_member=Q(db,"SELECT guild_id FROM guild_members WHERE char_id="+id+" FOR UPDATE");Require(guild_member.RowCount()==1&&N(guild_member.begin()[0])==b.guild_id);}
 auto access=LoadCurrentEstateAccessUnderTransaction(db,EstateId(b.estate_id),native_plot,{c.CharacterID(),c.AccountID()});Require(bool(access)&&access.value.owner_kind==OwnerKind::Guild&&access.value.owner_id==b.guild_id&&access.value.effective.Allows(AccessPolicy::Capability::Enter)&&access.value.effective.Allows(AccessPolicy::Capability::InteractItem));
}
}
void Clear()noexcept{witnesses.clear();ServiceNpcs::Clear();GuildPortalRuntime::Clear();portal_configured=false;}
bool Validate(Database& db,const ServiceNpcs::Binding& b,Client* c)noexcept{
 try{
  auto tx=Q(db,"SELECT @@in_transaction");Require(tx.RowCount()==1&&N(tx.begin()[0])==1);
  Require(zone&&b.house_id&&b.custody_id&&b.estate_id&&b.estate_id<=UINT32_MAX&&b.native_estate_id&&b.native_item_id&&b.guid==Guid(b.native_item_id)&&b.zone_id==zone->GetZoneID()&&b.instance_id==zone->GetInstanceID());
  const auto* reviewed=ServiceItemBindings::Lookup(b.item_id);Require(reviewed&&reviewed->npc_type_id==b.npc_type_id&&Kind(reviewed->kind)==b.service);
  auto current=GuildPortal::Current(db,b.zone_id,b.instance_id,true);Require(current&&current->house==b.house_id&&current->estate==b.estate_id&&current->guild==b.guild_id);
  auto w=witnesses.find(b.custody_id);Require(w!=witnesses.end());const auto& witness=w->second;const auto& stored=witness.stored;const auto& room=witness.room;Require(stored.placement&&room.house_id==b.house_id&&room.native_estate_id==b.native_estate_id&&room.route.zone_id==b.zone_id&&room.route.instance_id==b.instance_id&&room.route.zone_version==zone->GetInstanceVersion()&&room.definition.type==2&&room.reviewed_room);
  auto host=Q(db,"SELECT h.revision,h.pending_request_key,i.native_estate_id,i.definition_id,i.zone_version,l.custody_id,n.native_id FROM housing_house_instances h JOIN housing_house_interiors i USING(house_id) JOIN housing_house_locations l ON l.house_id=h.house_id AND l.location_kind=2 JOIN housing_live_item_custody hc ON hc.custody_id=l.custody_id JOIN housing_native_estates child ON child.native_id=i.native_estate_id AND child.active=1 AND child.definition_id=i.definition_id AND child.catalog_key=i.catalog_key JOIN housing_native_estates n ON n.native_id=child.parent_native_id AND n.ownership_estate_id=hc.estate_id AND n.active=1 WHERE h.house_id="+std::to_string(b.house_id)+" FOR UPDATE");
  if(!host.Success())return false;
  Require(host.RowCount()==1);auto h=host.begin();Require(N(h[0])&& !h[1]&&N(h[2])==b.native_estate_id&&N(h[3])==room.definition.id&&N(h[4])==room.route.zone_version&&RentState::PlotActive(db,uint32_t(b.estate_id),uint32_t(N(h[6])))&&RentState::HouseActive(db,N(h[5]))&&!ItemStorage::Locked(db,N(h[5])).stowed&&!CrateMemberScope::Locked(db,N(h[5])));
  auto profile=Q(db,"SELECT revision,origin,client_build,zone_short_name,HEX(client_asset_sha256),HEX(collision_map_sha256),HEX(manifest_sha256),HEX(definitions_blob),HEX(regions_blob) FROM housing_house_room_profiles WHERE house_id="+std::to_string(b.house_id)+" FOR UPDATE");Require(profile.RowCount()==1);auto pr=profile.begin();Require(N(pr[0])==witness.revision&&N(pr[1])==uint32_t(room.origin)&&N(pr[2])==room.client_build&&pr[3]&&std::string(pr[3])==room.zone_short_name&&Unhex(pr[4],32)==Unhex(room.client_asset_sha256.c_str(),32)&&Unhex(pr[5],32)==Unhex(room.collision_map_sha256.c_str(),32)&&Unhex(pr[6],32)==Unhex(room.manifest_sha256.c_str(),32)&&Unhex(pr[8],RegionWire::MaximumPayload)==InteriorContent::RegionPayload(room));
  auto decoded=Catalog::DecodePayloads(Unhex(pr[7],NeighborhoodWire::MaximumPayload),Unhex(pr[8],RegionWire::MaximumPayload));auto effective=room;effective.capacity_identity.reset();effective.capacity_policy_revision=0;effective.child_definitions.clear();bool found=false;for(const auto& d:decoded.definitions){if(d.id==room.definition.id){Require(!found);effective.definition=d;found=true;}else effective.child_definitions.push_back(d);}Require(found&&InteriorCapacity::ApplyUnderTransaction(db,effective,witness.revision)&&InteriorContent::DefinitionPayload(effective)==InteriorContent::DefinitionPayload(room)&&effective.capacity_policy_revision==room.capacity_policy_revision&&EstateWire::EncodeBootstrap(InteriorContent::Definitions(effective),{})==EstateWire::EncodeBootstrap(InteriorContent::Definitions(room),{}));
  auto rows=Q(db,"SELECT c.owner_character_id,c.schema_descriptor,HEX(c.row_blob),c.estate_id,p.native_item_id,p.pose_revision,HEX(p.pose_blob),r.native_estate_id,r.estate_id,r.state,r.custody_id,r.character_id,HEX(r.source_row),t.house_id,t.source_zone,t.source_version,t.source_instance,t.content_version,HEX(t.content_digest),t.profile_revision,s.house_id,s.revision,t.house_revision,HEX(c.catalog_blob),r.content_binding_version,r.content_binding_digest FROM housing_live_item_custody c JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id JOIN housing_interior_placement_reservations t ON t.native_item_id=p.native_item_id JOIN housing_house_item_scopes s ON s.custody_id=c.custody_id WHERE c.custody_id="+std::to_string(b.custody_id)+" FOR UPDATE");Require(rows.RowCount()==1);auto r=rows.begin();for(size_t i=0;i<24;++i)Require(r[i]);Require(!r[24]&&!r[25]);
  const auto raw=Unhex(r[2]);auto source=CustodyCodec::Decode(raw);Require(std::string(r[1])==LiveItems::Service::SchemaDescriptor()&&raw==CustodyCodec::Encode(stored.row)&&N(r[0])==source.values.character_id&&N(r[3])==stored.estate_id&&source.values.item_id==b.item_id&&N(r[4])==b.native_item_id&&N(r[5])==stored.placement->pose_revision&&N(r[7])==b.native_estate_id&&N(r[8])==stored.estate_id&&N(r[9])==1&&N(r[10])==b.custody_id&&N(r[11])==source.values.character_id&&Unhex(r[12])==raw&&N(r[13])==b.house_id&&N(r[14])==b.zone_id&&N(r[15])==room.route.zone_version&&N(r[16])==b.instance_id&&N(r[17])==InteriorContent::Version&&N(r[19])==witness.revision&&N(r[20])==b.house_id&&N(r[21])==witness.scope_revision&&N(r[22])==witness.house_revision);
  auto catalog=Unhex(r[23],16);Require(catalog.size()==16);const std::array<int32_t,4> expected_catalog{stored.catalog.item_class,stored.catalog.bag_slots,stored.catalog.lore_group,stored.catalog.no_drop};for(size_t i=0;i<4;++i){uint32_t v=uint32_t(catalog[4*i])|(uint32_t(catalog[4*i+1])<<8)|(uint32_t(catalog[4*i+2])<<16)|(uint32_t(catalog[4*i+3])<<24);Require(int32_t(v)==expected_catalog[i]);}
  auto pose=Pose(Unhex(r[6],28));Require(SamePose(pose,stored.placement->transform)&&pose.scale==1.f&&pose.pitch==0.f&&pose.roll==0.f);
  auto storage=ItemStorage::Locked(db,b.custody_id);Require(!storage.stowed&&(!storage.revision||storage.native_estate_id==b.native_estate_id)&&!CrateMemberScope::Locked(db,b.custody_id)&&RentState::MutableItem(db,stored.estate_id,uint32_t(N(h[6])),b.custody_id));
  auto pending=Q(db,"SELECT pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(source.values.character_id)+" FOR UPDATE");Require(pending.RowCount()<=1&&(!pending.RowCount()||!pending.begin()[0]));
  auto digest=Unhex(r[18],32);Require(digest.size()==32);std::array<uint8_t,32> selected{};std::copy(digest.begin(),digest.end(),selected.begin());auto input=InteriorContent::ItemInputUnderTransaction(db,room,b.item_id,selected);Require(bool(input)&&input.value.npc&&input.value.npc->service&&NpcRuntime::NativeItem(input.value.native_metadata,*input.value.npc));auto fingerprint=InteriorContent::Build(input.value);Require(bool(fingerprint)&&fingerprint.value.digest==selected);
  auto projection=InventoryBridge::Reconstruct(database,source);Require(bool(projection)&&projection.value.item);auto cached=NpcRuntime::Cached(*projection.value.item->GetUnscaledItem());Require(cached&&NpcRuntime::Same(*cached,*input.value.npc)&&RoF2::SerializeHousingItem(*projection.value.item,EQ::invslot::slotCursor));
  auto frame=NpcNativeFrame::ReadUnderTransaction(db,b.custody_id,b.native_item_id,N(r[5]),pose);auto actual=NpcRuntime::ActorPose(*input.value.npc,pose,frame);Require(actual.position[1]==b.x&&actual.position[0]==b.y&&actual.position[2]==b.z&&actual.heading==b.heading);
  if(c)ClientAccess(db,b,uint32_t(N(h[6])),*c);
  return true;
 }catch(...){return false;}
}
bool Reconcile(Client& c,const Scene::InteriorSnapshot& snapshot)noexcept{
 try{
  if(snapshot.room_profile&&snapshot.room_profile->definition.type!=2){Clear();return true;}
  Require(c.Connected()&&c.HasInventorySessionReservation()&&zone&&snapshot.house&&snapshot.house->interior&&snapshot.room_profile&&snapshot.profile_revision&&HouseRuntime::ReviewedRoomForCurrentClient(snapshot,*snapshot.room_profile,snapshot.profile_revision));
  Require(snapshot.room_profile->definition.type==2&&snapshot.house->interior->zone_id==zone->GetZoneID()&&snapshot.house->interior->instance_id==zone->GetInstanceID()&&snapshot.context.estate_id);
  std::map<uint64_t,Witness> next;std::vector<ServiceNpcs::Binding> bindings;
  for(const auto& item:snapshot.contents){const auto& stored=item.stored;const auto* entry=ServiceItemBindings::Lookup(stored.row.values.item_id);if(!entry||stored.stowed||item.contained_in_crate||!stored.placement)continue;Require(item.room&&item.room->house_id==snapshot.house->house_id&&stored.placement->reservation.native_estate_id==snapshot.house->interior->native_estate_id&&stored.placement->reservation.native_item_id);
   const auto* data=database.GetItem(stored.row.values.item_id);auto p=data?NpcRuntime::Cached(*data):std::nullopt;Require(p&&p->service);const auto pose=NpcRuntime::ActorPose(*p,stored.placement->transform,stored.placement->npc_frame);
   ServiceNpcs::Binding b;b.house_id=snapshot.house->house_id;b.estate_id=snapshot.context.estate_id;b.custody_id=stored.custody_id;b.native_estate_id=stored.placement->reservation.native_estate_id;b.native_item_id=stored.placement->reservation.native_item_id;b.guid=Guid(b.native_item_id);b.item_id=stored.row.values.item_id;b.npc_type_id=entry->npc_type_id;b.guild_id=snapshot.access.owner_id;b.zone_id=zone->GetZoneID();b.instance_id=zone->GetInstanceID();b.service=Kind(entry->kind);b.x=pose.position[1];b.y=pose.position[0];b.z=pose.position[2];b.heading=pose.heading;Require(next.emplace(b.custody_id,Witness{*snapshot.room_profile,snapshot.profile_revision,stored,item.scope_revision,item.room->house_revision}).second);bindings.push_back(b);
  }
  witnesses=std::move(next);
  Require(ServiceNpcs::Reconcile(bindings,[](const auto& b){return Check(b,nullptr);},[](Client& viewer,const auto& b){return Check(b,&viewer);}));
  if(!portal_configured){GuildPortalRuntime::Configure([](Client& viewer,bool pads){return Sources(viewer,pads);},[](Database& db,Client& viewer,const GuildPortalRuntime::Source& source){return ValidatePortalSource(db,viewer,source);});portal_configured=true;}
  return true;
 }catch(...){Clear();return false;}
}
bool ValidatePortalSource(Database& db,Client& c,const GuildPortalRuntime::Source& source)noexcept{
 // LiveBindings is a shape-only view. All source authority below stays on
 // this caller-owned connection; invoking callback-backed Bindings here would
 // open a second FOR UPDATE connection and deadlock against our own locks.
 try{const auto& b=source.binding;Require(Validate(db,b,&c)&&GuildPortal::Same(source.scope,{b.house_id,uint32_t(b.estate_id),b.guild_id,b.zone_id,b.instance_id})&&GuildPortal::SameCurrent(db,source.scope,true));
  if(source.builtin_pad){const auto& p=source.pad_position;const auto& original=GuildPortalRuntime::StandardBuiltinGeode;return p.x==original.x&&p.y==original.y&&p.z==original.z&&Builtin(db,b);}
  if(b.service==ServiceNpcs::Service::TeleportPad){for(const auto& assistant:ServiceNpcs::LiveBindings())if(SameScope(b,assistant)&&assistant.item_id==66640&&assistant.service==ServiceNpcs::Service::TeleportMerchant&&Validate(db,assistant,nullptr))return true;return false;}
  if(b.item_id!=66640||b.service!=ServiceNpcs::Service::TeleportMerchant)return false;
  for(const auto& pad:ServiceNpcs::LiveBindings())if(SameScope(b,pad)&&pad.item_id==66646&&pad.service==ServiceNpcs::Service::TeleportPad&&Validate(db,pad,nullptr))return true;
  return Builtin(db,b);
 }catch(...){return false;}
}
std::vector<GuildPortalRuntime::Source> Sources(Client& c,bool pads)noexcept{
 std::vector<GuildPortalRuntime::Source> result;try{const auto bindings=ServiceNpcs::LiveBindings();const GuildPortal::Point client{c.GetX(),c.GetY(),c.GetZ()};for(const auto& b:bindings){GuildPortalRuntime::Source candidate{b,{b.house_id,uint32_t(b.estate_id),b.guild_id,b.zone_id,b.instance_id}};
  if(!pads){if(b.service==ServiceNpcs::Service::TeleportMerchant&&CheckPortal(c,candidate))result.push_back(candidate);continue;}
  if(b.service==ServiceNpcs::Service::TeleportPad){if(GuildPortal::Near(client,{b.x,b.y,b.z})&&CheckPortal(c,candidate))result.push_back(candidate);}
  else if(b.service==ServiceNpcs::Service::TeleportMerchant&&b.item_id==66640&&b.zone_id==345){candidate.builtin_pad=true;candidate.pad_position=GuildPortalRuntime::StandardBuiltinGeode;if(GuildPortal::Near(client,candidate.pad_position)&&CheckPortal(c,candidate))result.push_back(candidate);}
 }}catch(...){result.clear();}return result;
}

bool PublishBindings(Client& c,uint32_t estate,const std::function<std::string(uint32_t)>& get_guid)noexcept{
 try{if(!get_guid||!estate)return false;std::map<uint32_t,std::string> guids;for(const auto& b:ServiceNpcs::LiveBindings())if(b.native_estate_id==estate){auto guid=get_guid(b.native_item_id);if(!guid.empty())guids.emplace(b.native_item_id,std::move(guid));}return ServiceNpcs::PublishBindings(c,guids);}catch(...){return false;}
}

}
