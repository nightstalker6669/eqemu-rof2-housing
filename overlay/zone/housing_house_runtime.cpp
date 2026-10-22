#include "../common/housing/runtime_activation_db.h"
#include "map.h"
#include "../common/path_manager.h"
#include "housing_house_runtime.h"
#include "housing_house_content_loader.h"
#include "../common/housing/reviewed_room_runtime.h"
#include "housing_crate_runtime.h"
#include "housing_house_reload.h"
#include "client.h"
#include "doors.h"
#include "zone.h"
#include "zonedb.h"
#include "../common/housing/known_original_interior_zones.h"
#include "../common/eqemu_config.h"
#include "../common/rulesys.h"
#include "../common/housing/house_placement_model.h"
#include "../common/housing/house_interior_allocator.h"
#include "../common/housing/house_custody_authority.h"
#include <mutex>
#include <fstream>
#include <sstream>
#include <filesystem>
extern Zone* zone;
extern ZoneDatabase content_db;
namespace Housing::HouseRuntime {
namespace {
std::mutex selection_mutex;
std::vector<std::shared_ptr<const PersonalTestSelection>> registered_selections;
std::optional<Scene::LobbyRequest> verified_lobby_door;
// Admission must not reach upstream zoning with absent destination metadata.
// This runs before the service grants instance membership; it changes no route,
// placement or inventory state. A cold restart is required after cache repair.
bool EntryDestinationAvailable(const HouseBinding::Interior& destination){
 try{
  const auto* cached=ZoneStore::Instance()->GetZone(destination.zone_id,destination.zone_version);
  if(!cached||cached->long_name.empty()||!OriginalInteriors::Matches(destination.zone_id,cached->short_name))return false;
  auto rows=content_db.QueryDatabase("SELECT short_name,long_name FROM zone WHERE zoneidnumber="+std::to_string(destination.zone_id)+" AND version="+std::to_string(destination.zone_version));
  if(!rows.Success()||rows.RowCount()!=1)return false;
  auto row=rows.begin();
  return row[0]&&row[1]&&*row[1]&&cached->short_name==row[0];
 }catch(...){return false;}
}
const HouseGeometry::Record* SelectedModel(const PersonalTestSelection& s){
 const auto* item=HouseTemplates::Lookup(s.item_template_id);
 return item?HouseGeometry::Lookup(item->item,item->model):nullptr;
}
bool CurrentClient(Client& c,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l){
 return RuleB(Housing,EnablePlayerHousing)&&InventoryBridge::IsRegisteredZoneThread()&&zone&&c.Connected()&&c.ClientDataLoaded()&&c.HasInventorySessionReservation()&&c.ClientVersion()==EQ::versions::ClientVersion::RoF2&&l.actor.character_id==c.CharacterID()&&l.actor.account_id==c.AccountID()&&f.HeldFor(l.actor,l.epoch,l.session_key);
}
using Selections=std::vector<std::shared_ptr<const PersonalTestSelection>>;
const PersonalTestSelection* ContentFor(const Selections& all,uint32_t item){const PersonalTestSelection* found=nullptr;for(const auto& s:all)if(s&&s->experimental_no_upkeep&&s->item_template_id==item&&s->content&&s->content->Route(item)){if(found)return nullptr;found=s.get();}return found;}
// Initial binding is authorized from the actual frozen item, not from a
// configured actor/slot or SQL template classification. PrepareStore also
// locks the genuine lease, exact inventory row and empty binding location.
class RuntimeTemplates final:public HouseBinding::HouseTemplatePolicy {
 Client& client_;LiveItems::InventorySessionFence& fence_;std::shared_ptr<const HouseReviewedContent::Policies> content_;
public:
 RuntimeTemplates(Client& c,LiveItems::InventorySessionFence& f,std::shared_ptr<const HouseReviewedContent::Policies> content):client_(c),fence_(f),content_(std::move(content)){}
 bool VerifiedHouse(const LiveItems::InventoryRow& row)const noexcept override{return content_->VerifiedHouse(row);}
 bool PermitInitialBinding(AuthenticatedPurchaser actor,const LiveItems::InventoryRow& row)const noexcept override{
  try{if(!InventoryBridge::IsRegisteredZoneThread()||!client_.Connected()||!client_.ClientDataLoaded()||!client_.HasInventorySessionReservation()||actor.character_id!=client_.CharacterID()||actor.account_id!=client_.AccountID()||row.values.character_id!=actor.character_id||!VerifiedHouse(row))return false;
   const auto* route=content_->Route(row.values.item_id);auto* item=client_.GetInv().GetItem(int16_t(row.values.slot_id));return route&&item&&!item->GetHousingHouseID()&&!item->GetHousingCrateID()&&item->GetHousingEstateID()==UINT32_MAX&&fence_.SourceMatches(row,item->GetSerialNumber())&&bool(HousePlacement::Metadata(*item,route->definition_id,HouseTemplates::Type(row.values.item_id)));
  }catch(...){return false;}
 }
};
class Custody final:public HouseBinding::CustodyContext,private HouseInteriors::AllocationPolicy,private HouseCustodyAuthority::ReviewedOperationPolicy,private HouseBinding::CustodyAuthorization {
 Client& client_;LiveItems::InventorySessionFence& fence_;HouseBinding::HouseApplicationProof& application_;
 Selections registered_;std::shared_ptr<const HouseReviewedContent::Policies> content_;RuntimeTemplates templates_;HouseCustodyAuthority::Authorizer authorization_;
 bool Held(LiveItems::InventorySessionFence& f,const LiveItems::Lease& l)const{return &f==&fence_&&CurrentClient(client_,f,l);}
 bool Yard(LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const LiveItems::Reservation& r)const{return Held(f,l)&&zone->GetZoneID()==712&&f.PlacementContextMatches(r.estate_id,r.native_estate_id,zone->GetInstanceID());}
public:
 Custody(Client& c,LiveItems::InventorySessionFence& f,HouseBinding::HouseApplicationProof& a,Selections all,std::shared_ptr<const HouseReviewedContent::Policies> content):client_(c),fence_(f),application_(a),registered_(std::move(all)),content_(std::move(content)),templates_(c,f,content_),authorization_(*this){}
 const HouseBinding::HouseTemplatePolicy& Templates()const override{return templates_;}
 const HouseBinding::InteriorRoutePolicy* Routes()const override{return content_.get();}
 HouseBinding::HouseApplicationProof& Application()override{return application_;}
 const HouseBinding::CustodyAuthorization* Authorization()const override{return this;}
 std::optional<ContentBinding::HouseRoute> ReviewPlacementUnderTransaction(Database&,LiveItems::InventorySessionFence& f,const HouseBinding::PlacementFacts& facts)const override{
  const auto* route=content_->Route(facts.source.values.item_id);const auto type=HouseTemplates::Type(facts.source.values.item_id);
  if(!route||!templates_.VerifiedHouse(facts.source)||!Yard(f,facts.lease,facts.reservation)||facts.source.values.character_id!=facts.lease.actor.character_id||facts.catalog_revision!=facts.reservation.catalog_revision||facts.neighborhood_instance!=zone->GetInstanceID()||facts.plot_definition.type!=(type==3?4u:8u)||!facts.plot_definition.max_children||facts.house_definition.id!=route->definition_id||facts.house_definition.type!=type||facts.house_definition.zone_id!=route->zone_id||std::find(facts.plot_definition.children.begin(),facts.plot_definition.children.end(),route->definition_id)==facts.plot_definition.children.end()){LogInfo("Native house route preview rejected actor [{}] template [{}] route [{}] yard [{}] type [{}/{}] house [{}/{}] revision [{}/{}]",client_.CharacterID(),facts.source.values.item_id,bool(route),Yard(f,facts.lease,facts.reservation),facts.plot_definition.type,type,facts.house_definition.id,route?route->definition_id:0,facts.catalog_revision,facts.reservation.catalog_revision);return {};}
  if(facts.storage_only&&(!RuleB(Housing,EnableNativeHouseSwap)||facts.pose))return {};
  const bool source=facts.stage==HouseBinding::PlacementStage::Preview||facts.stage==HouseBinding::PlacementStage::Store;
  if(source){if(!facts.access.Allows(AccessPolicy::Capability::AddHouse)||(!facts.storage_only&&facts.placed_children>=1)||!f.SourceMatches(facts.source,facts.runtime_serial)){LogInfo("Native house source rejected actor [{}] add [{}] children [{}] source_match [{}]",client_.CharacterID(),facts.access.Allows(AccessPolicy::Capability::AddHouse),facts.placed_children,f.SourceMatches(facts.source,facts.runtime_serial));return {};}auto* item=client_.GetInv().GetItem(int16_t(facts.source.values.slot_id));if(!item||!HousePlacement::Metadata(*item,route->definition_id,type)){LogInfo("Native house metadata rejected actor [{}] template [{}] runtime_definition [{}] route_definition [{}]",client_.CharacterID(),facts.source.values.item_id,item?item->GetUnscaledItem()->HousingDefinitionID:0,route->definition_id);return {};}}
  else if(!facts.access.Allows(facts.stage==HouseBinding::PlacementStage::Move?AccessPolicy::Capability::MoveItem:AccessPolicy::Capability::Enter))return {};
  return *route;
 }
 std::optional<ContentBinding::HouseRoute> ReviewCrateRelocationUnderTransaction(Database&,LiveItems::InventorySessionFence& f,const HouseBinding::Binding& before,const LiveItems::Request& request,const LiveItems::InventoryRow& source)const override{
  const auto* route=content_->Route(source.values.item_id);if(!route||!request.placement||!request.crate||!Yard(f,request.lease,request.placement->reservation)||before.location.kind!=HouseBinding::Location::Kind::Custody||!before.interior||before.item_template_id!=source.values.item_id||source.values.character_id!=request.lease.actor.character_id||!templates_.VerifiedHouse(source)||!f.SourceMatches(request.expected,request.verified_runtime_serial))return {};
  EstateWire::Definition d;d.id=route->definition_id;d.type=HouseTemplates::Type(source.values.item_id);d.zone_id=route->zone_id;if(!content_->VerifiedDestination(*before.interior,d,source.values.item_id))return {};return *route;
 }
 std::optional<HouseBinding::Interior> AllocateInterior(Database& db,const HouseBinding::Binding& before,const LiveItems::Request& r)override{
  const auto* route=content_->Route(before.item_template_id);const auto* registered=ContentFor(registered_,before.item_template_id);if(!route||!registered||!registered->room_profile)return {};
  Options options;options.enabled=true;auto allocated=HouseInteriors::AllocateUnderTransaction(db,before,r,fence_,*route,*this,options);if(!allocated)return {};return allocated.value;
 }
 bool FinalizeInteriorUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const HouseBinding::Effect& effect,const LiveItems::Request& r)const override{
  const auto* registered=ContentFor(registered_,effect.after.item_template_id);return Held(f,r.lease)&&effect.action==LiveItems::Action::Store&&effect.after.interior&&registered&&registered->room_profile&&ReviewedRoomRuntime::EnsureProfileUnderTransaction(db,effect.after,*effect.after.interior,*registered->room_profile,registered->room_profile_revision);
 }
private:
 bool AllowedUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const HouseBinding::AuthorizationRequest& r)const override{
  if(!Held(f,r.lease)||r.source.values.character_id!=r.lease.actor.character_id||!templates_.VerifiedHouse(r.source))return false;
  // Preserve the core strict personal-owner path, including retained house
  // tokens inside a crate. All delegated personal and guild operations use
  // the canonical typed authorizer and its fresh ACL/graph/source checks.
  if(HouseTemplates::Type(r.source.values.item_id)==3){auto owner=db.QueryDatabase("SELECT owner_kind,owner_id FROM housing_current_estates WHERE id="+std::to_string(r.estate_id)+" FOR UPDATE",false);if(!owner.Success()||owner.RowCount()!=1||!owner.begin()[0]||!owner.begin()[1])return false;if(std::stoull(owner.begin()[0])==1&&std::stoull(owner.begin()[1])==r.lease.actor.character_id)return true;}
  return authorization_.AllowedUnderTransaction(db,f,r);
 }
 bool VerifiedUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const HouseBinding::AuthorizationRequest& r,const HouseAuthority::LockedContext& context)const override{
  const auto* route=content_->Route(r.source.values.item_id);const auto type=HouseTemplates::Type(r.source.values.item_id);
  if(!route||!Held(f,r.lease)||!templates_.VerifiedHouse(r.source)||r.source.values.character_id!=r.lease.actor.character_id||context.zone_id!=712||context.zone_version||zone->GetZoneID()!=712||context.instance_id!=zone->GetInstanceID()||context.plot_definition.type!=(type==3?4u:8u)||context.house_definition.type!=type||context.house_definition.id!=route->definition_id||context.house_definition.zone_id!=route->zone_id)return false;
  const auto interior=r.first_interior?r.first_interior:r.before.interior;if(!interior||!content_->VerifiedDestination(*interior,context.house_definition,r.source.values.item_id)||interior->catalog_key!=context.catalog_key||interior->catalog_revision!=context.catalog_revision)return false;
  if(r.phase==HouseBinding::AuthorizationPhase::StoreAfterCustodyInsert){
   auto* item=client_.GetInv().GetItem(int16_t(r.source.values.slot_id));if(!item||!f.SourceMatches(r.source,r.runtime_serial)||!HousePlacement::Metadata(*item,route->definition_id,type))return false;
   if(r.before.interior){if(item->GetHousingHouseID()!=r.before.house_id||item->GetHousingEstateID()!=r.before.interior->native_estate_id)return false;}else if(!templates_.PermitInitialBinding(r.lease.actor,r.source))return false;
   auto count=db.QueryDatabase("SELECT COUNT(*) FROM housing_house_interiors i JOIN housing_house_locations l ON l.house_id=i.house_id AND l.location_kind=2 JOIN housing_live_placed_items p ON p.custody_id=l.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE r.native_estate_id="+std::to_string(context.request.native_plot_id)+" AND r.state=1 AND NOT EXISTS (SELECT 1 FROM housing_live_item_storage st WHERE st.custody_id=p.custody_id AND st.stowed=1) AND NOT EXISTS (SELECT 1 FROM housing_crate_members cm WHERE cm.custody_id=p.custody_id)",false);if(!count.Success()||count.RowCount()!=1||!count.begin()[0]||!context.plot_definition.max_children||std::stoull(count.begin()[0])>context.plot_definition.max_children)return false;
  }
  return true;
 }
 bool CatalogRelocationReviewedUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const HouseBinding::Binding& before,const LiveItems::Request& r,const ContentBinding::HouseRoute& route)const override{return before.interior&&ReviewedUnderTransaction(db,f,before,r,route);}
 bool ReparentReviewedUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const HouseBinding::Binding& before,const LiveItems::Request& r,const ContentBinding::HouseRoute& route,uint32_t old)const override{return old&&r.placement&&old!=r.placement->reservation.native_estate_id&&before.interior&&ReviewedUnderTransaction(db,f,before,r,route);}
 bool ReviewedUnderTransaction(Database&,LiveItems::InventorySessionFence& f,const HouseBinding::Binding& before,const LiveItems::Request& r,const ContentBinding::HouseRoute& route)const override{
  if(!r.placement||!Yard(f,r.lease,r.placement->reservation)||r.estate_id!=r.placement->reservation.estate_id||r.expected.values.character_id!=r.lease.actor.character_id||!templates_.VerifiedHouse(r.expected)||!f.SourceMatches(r.expected,r.verified_runtime_serial))return false;
  const auto* registered=content_->Route(before.item_template_id);if(!registered||registered->definition_id!=route.definition_id)return false;
  if(before.interior){EstateWire::Definition d;d.id=route.definition_id;d.type=HouseTemplates::Type(before.item_template_id);d.zone_id=route.zone_id;return content_->VerifiedDestination(*before.interior,d,before.item_template_id);}
  return templates_.PermitInitialBinding(r.lease.actor,r.expected);
 }
};
class ScenePolicy final:public Scene::HouseScenePolicy {
 Client& client_;Selections content_;std::optional<Scene::LobbyRequest> lobby_;const CrateBinding::CustodyContext* crates_;
 bool Physical(Route route)const{return zone&&route.zone_id==zone->GetZoneID()&&route.zone_version==zone->GetInstanceVersion()&&route.instance_id==zone->GetInstanceID();}
public:
 ScenePolicy(Client& c,Selections all,std::optional<Scene::LobbyRequest> lobby,const CrateBinding::CustodyContext* crates):client_(c),content_(std::move(all)),lobby_(lobby),crates_(crates){}
 const CrateBinding::CustodyContext* Crates()const override{return crates_;}
 bool LobbySourceUnderTransaction(Database&,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const Scene::LobbyRequest& request,uint32_t guild)const override{
  if(!lobby_||!CurrentClient(client_,f,l)||request.source.zone_id!=344||!Physical(request.source)||request.source.zone_id!=lobby_->source.zone_id||request.source.zone_version!=lobby_->source.zone_version||request.source.instance_id!=lobby_->source.instance_id||request.door_id!=lobby_->door_id)return false;
  const auto cached=client_.GuildID();if((AccessPolicy::PositiveIdentity(cached)?cached:0)!=guild)return false;
  auto* door=entity_list.FindDoor(request.door_id);return door&&DistanceNoZ(client_.GetPosition(),door->GetPosition())<RuleI(Range,MaxDistanceToClickDoors);
 }
 std::optional<ContentBinding::HouseRoute> ReviewedLobbyRouteUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const LiveItems::Stored& stored,const HouseBinding::Binding& binding,const EstateWire::Definition& definition,const Scene::LobbyRequest& request,uint32_t guild)const override{
  const auto* s=ContentFor(content_,binding.item_template_id);
  if(!s||!guild||!LobbySourceUnderTransaction(db,f,l,request,guild)||definition.type!=2||!binding.interior||!EntryDestinationAvailable(*binding.interior)||!stored.placement||stored.stowed||binding.location.kind!=HouseBinding::Location::Kind::Custody||binding.location.custody_id!=stored.custody_id||stored.row.values.item_id!=binding.item_template_id||!stored.row.values.character_id||!s->content->VerifiedHouse(stored.row)||!s->content->VerifiedDestination(*binding.interior,definition,binding.item_template_id))return {};
  const auto& r=stored.placement->reservation;// Fresh own-guild/plot/placed-house/ACL graph authority belongs to the
  // dedicated locked service. The configured entry supplies immutable content,
  // never an obsolete fixture guild, estate, plot or catalog ownership grant.
  if(!stored.estate_id||r.estate_id!=stored.estate_id||!r.native_estate_id||r.native_estate_id==UINT32_MAX||!r.native_item_id||r.native_item_id==UINT32_MAX||r.catalog_key!=binding.interior->catalog_key||!r.catalog_revision||r.catalog_revision>binding.interior->catalog_revision)return {};return *s->content->Route(binding.item_template_id);
 }
 std::optional<ContentBinding::HouseRoute> ReviewedAnchorHouseUnderTransaction(Database&,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const LiveItems::Stored& stored,const HouseBinding::Binding& binding,const EstateWire::Definition& d)const override{
  const auto* s=ContentFor(content_,binding.item_template_id);
  if(!s||!CurrentClient(client_,f,l)||!stored.placement||stored.stowed||!binding.interior||stored.row.values.item_id!=binding.item_template_id||!stored.row.values.character_id||binding.location.kind!=HouseBinding::Location::Kind::Custody||binding.location.custody_id!=stored.custody_id||!s->content->VerifiedHouse(stored.row)||!s->content->VerifiedDestination(*binding.interior,d,binding.item_template_id))return {};
  if(stored.placement->reservation.estate_id!=stored.estate_id||stored.placement->reservation.catalog_key!=binding.interior->catalog_key)return {};
  return *s->content->Route(binding.item_template_id);
 }
 std::optional<ContentBinding::HouseRoute> ReviewedExitUnderTransaction(Database&,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const HouseBinding::Binding& binding,const Scene::ExitContext& context)const override{
  const auto* s=ContentFor(content_,binding.item_template_id);if(!s||!CurrentClient(client_,f,l)||!binding.interior||!Physical(context.source)||!context.creator_character_id||binding.interior->catalog_key.empty()||binding.interior->catalog_revision!=context.catalog_revision||!s->content->VerifiedDestination(*binding.interior,context.house_definition,binding.item_template_id))return {};
  return *s->content->Route(binding.item_template_id);
 }
 std::optional<ExteriorExit::Destination> ReviewedExteriorPointUnderTransaction(Database&,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const HouseBinding::Binding& binding,const Scene::ExteriorExitContext& facts,const ContentBinding::HouseRoute& immutable)const override{
  const auto* s=ContentFor(content_,binding.item_template_id);
  if(!s||!CurrentClient(client_,f,l)||!Physical(facts.parent.source)||facts.parent.neighborhood.zone_id!=712||facts.parent.neighborhood.zone_version||!facts.parent.neighborhood.instance_id||!binding.interior||binding.interior->catalog_revision!=facts.parent.catalog_revision||!s->content->VerifiedDestination(*binding.interior,facts.parent.house_definition,binding.item_template_id)||immutable.definition_id!=binding.interior->definition_id)return {};
  Map map;if(!map.Load(PathManager::Instance()->GetMapsPath()+"/base/neighborhood.map"))return {};
  ExteriorExit::Point reference{facts.region.matrix[13],facts.region.matrix[12],facts.region.matrix[14]};
  if(facts.current_house){if(!facts.current_house->placement||facts.current_house->custody_id!=binding.location.custody_id||facts.current_house->row.values.item_id!=binding.item_template_id)return {};const auto& p=facts.current_house->placement->transform.position;reference={p[1],p[0],p[2]};}
  const auto trace=[&](ExteriorExit::Point from,ExteriorExit::Point to)->std::optional<ExteriorExit::Ground>{glm::vec3 normal;float distance;if(!map.DoCollisionCheck({from[0],from[1],from[2]},{to[0],to[1],to[2]},normal,distance))return {};double length=0;for(unsigned i=0;i<3;++i)length+=double(to[i]-from[i])*(to[i]-from[i]);length=std::sqrt(length);if(!std::isfinite(length)||length<=0||!std::isfinite(distance)||distance<0||distance>length)return {};ExteriorExit::Ground result;for(unsigned i=0;i<3;++i)result.point[i]=from[i]+float(distance/length)*(to[i]-from[i]);double n=std::sqrt(double(normal.x)*normal.x+double(normal.y)*normal.y+double(normal.z)*normal.z);if(!std::isfinite(n)||n<=0)return {};float sign=normal.z<0?-1:1;result.normal={float(sign*normal.x/n),float(sign*normal.y/n),float(sign*normal.z/n)};return result;};
  const auto clear=[&](const ExteriorExit::Box& body){return !map.IntersectsBox({body.minimum[0],body.minimum[1],body.minimum[2]},{body.maximum[0],body.maximum[1],body.maximum[2]});};
  return ExteriorExit::Resolve(facts.region,reference,client_.GetSize(),client_.GetZOffset(),facts.blockers,trace,clear);
 }
 std::optional<ContentBinding::HouseRoute> ReviewedInteriorUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const HouseBinding::Binding& binding,const Scene::ExitContext& context)const override{if(binding.location.kind!=HouseBinding::Location::Kind::Custody)return {};return ReviewedExitUnderTransaction(db,f,l,binding,context);}
 std::optional<ContentBinding::HouseRoute> ReviewedRouteUnderTransaction(Database&,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const LiveItems::Stored& stored,const HouseBinding::Binding& binding,const EstateWire::Definition& d)const override{
  const auto* s=ContentFor(content_,binding.item_template_id);if(!s||!CurrentClient(client_,f,l)||!stored.placement||!binding.interior||stored.row.values.item_id!=binding.item_template_id||!stored.row.values.character_id||binding.location.kind!=HouseBinding::Location::Kind::Custody||binding.location.custody_id!=stored.custody_id||!s->content->VerifiedHouse(stored.row)||!s->content->VerifiedDestination(*binding.interior,d,binding.item_template_id))return {};
  const auto& r=stored.placement->reservation;if(r.estate_id!=stored.estate_id||r.catalog_key!=binding.interior->catalog_key||!f.PlacementContextMatches(r.estate_id,r.native_estate_id,zone->GetInstanceID())||zone->GetZoneID()!=712)return {};return *s->content->Route(binding.item_template_id);
 }
 std::optional<ContentBinding::HouseRoute> ReviewedAttachedInteriorUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const Scene::Snapshot& parent,const Scene::Item& item)const override{
  if(!item.house||!parent.access.Allows(AccessPolicy::Capability::Enter)||!Physical(parent.route)||!item.stored.placement||item.stored.placement->reservation.native_estate_id!=parent.native_plot_id)return {};const auto* s=ContentFor(content_,item.house->item_template_id);if(!s)return {};const auto* route=s->content->Route(s->item_template_id);EstateWire::Definition d;d.id=route->definition_id;d.type=HouseTemplates::Type(s->item_template_id);d.zone_id=route->zone_id;return ReviewedRouteUnderTransaction(db,f,l,item.stored,*item.house,d);
 }
 std::optional<ContentBinding::HouseRoute> ReviewedEntryUnderTransaction(Database& db,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const Scene::Snapshot& scene,const Scene::Item& item)const override{
  if(!scene.access.Allows(AccessPolicy::Capability::Enter)||!Physical(scene.route)||!item.house||!item.house->interior||!item.stored.placement||!EntryDestinationAvailable(*item.house->interior))return {};const auto* s=ContentFor(content_,item.house->item_template_id);if(!s)return {};const auto* model=SelectedModel(*s);const auto* route=s->content->Route(s->item_template_id);if(!model)return {};auto distance=HousePlacement::DistanceToGeometryBounds(model->geometry,item.stored.placement->transform,{client_.GetY(),client_.GetX(),client_.GetZ()});if(!distance||*distance>20)return {};EstateWire::Definition d;d.id=route->definition_id;d.type=HouseTemplates::Type(s->item_template_id);d.zone_id=route->zone_id;return ReviewedRouteUnderTransaction(db,f,l,item.stored,*item.house,d);
 }
};
class RoomPolicy final:public InteriorContent::ProfilePolicy {
 Client& client_;Selections content_;
public:
 RoomPolicy(Client& c,Selections all):client_(c),content_(std::move(all)){}
 bool ReviewedAnchorUnderTransaction(Database&,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const InteriorContent::Snapshot& current)const override{
  if(!CurrentClient(client_,f,l))return false;const auto* s=current.context.house?ContentFor(content_,current.context.house->item_template_id):nullptr;
  if(!s||!s->room_profile)return false;
  return ReviewedRoomRuntime::Matches(current.context,current.room,current.profile_revision,s->item_template_id,*s->room_profile,s->room_profile_revision,current.context.exterior_source.value_or(current.context.context.source));
 }
 bool ReviewedUnderTransaction(Database&,LiveItems::InventorySessionFence& f,const LiveItems::Lease& l,const InteriorContent::Snapshot& current)const override{
  if(!CurrentClient(client_,f,l))return false;const auto* s=current.context.house?ContentFor(content_,current.context.house->item_template_id):nullptr;if(!s||!s->room_profile)return false;
  return ReviewedRoomRuntime::Matches(current.context,current.room,current.profile_revision,s->item_template_id,*s->room_profile,s->room_profile_revision,{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())});
 }
};

}
namespace {
bool ConfigureSelections(std::vector<PersonalTestSelection> entries){
 const auto* config=EQEmuConfig::get();
 if(!InventoryBridge::IsRegisteredZoneThread()||!config||!Housing::RuntimeActivation::AllowsReconstructedContent(database)||!ContentLoader::ValidSelections(entries))return false;
 std::vector<std::shared_ptr<const HouseReviewedContent::Policies>> parts;Selections selections,rooms;
 for(auto& entry:entries){parts.push_back(entry.content);auto selected=std::make_shared<const PersonalTestSelection>(std::move(entry));if(selected->room_profile)rooms.push_back(selected);selections.push_back(std::move(selected));}
 auto combined=std::make_shared<const HouseReviewedContent::PolicySet>(std::move(parts));
 std::lock_guard<std::mutex> guard(selection_mutex);if(!registered_selections.empty())return false;
 if(!HousePreload::ConfigureZonePoliciesAtSafeStartup({combined,combined,CrateRuntime::ReloadPolicy(rooms)}))return false;
 registered_selections=std::move(selections);return true;
}
}
bool ReviewedRoomForCurrentClient(const Scene::InteriorSnapshot& scene,const InteriorContent::Profile& p,uint64_t revision){
 if(!zone||!InventoryBridge::IsRegisteredZoneThread())return false;std::lock_guard<std::mutex> guard(selection_mutex);const auto& all=registered_selections;const auto* s=scene.house?ContentFor(all,scene.house->item_template_id):nullptr;
 return s&&s->room_profile&&ReviewedRoomRuntime::Matches(scene,p,revision,s->item_template_id,*s->room_profile,s->room_profile_revision,{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())});
}
bool ConfigureVerifiedGuildLobbyDoorAtSafeStartup(Route source,uint8_t door_id){
 if(!InventoryBridge::IsRegisteredZoneThread()||source.zone_id!=344||source.instance_id>32000||!door_id)return false;
 std::lock_guard<std::mutex> guard(selection_mutex);if(verified_lobby_door)return false;verified_lobby_door=Scene::LobbyRequest{source,door_id};return true;
}
bool HandleVerifiedGuildLobbyDoor(Client& client,uint8_t door_id,bool within){
 std::optional<Scene::LobbyRequest> configured;{std::lock_guard<std::mutex> guard(selection_mutex);configured=verified_lobby_door;}
 if(!configured||!zone||zone->GetZoneID()!=344||zone->GetInstanceVersion()!=configured->source.zone_version||zone->GetInstanceID()!=configured->source.instance_id||door_id!=configured->door_id)return false;
 if(!within)return true;
 if(!RuleB(Housing,EnablePlayerHousing)||client.ClientVersion()!=EQ::versions::ClientVersion::RoF2)return false;
 auto* bridge=client.GetHousingInventoryBridge();if(!bridge){client.Message(Chat::Red,"Guild hall entry is temporarily unavailable.");return true;}
 auto result=bridge->AdmitOwnGuildHallFromLobby(*configured);
 if(!result){client.Message(Chat::Red,"Your guild hall cannot be entered right now.");return true;}
 if(!result.value.admission){if(result.value.guild_id)client.SendToGuildHall();else client.Message(Chat::Red,"You must belong to a guild to enter the guild hall.");return true;}
 const auto& admitted=*result.value.admission;const auto& p=admitted.spawn;
 client.MovePC(admitted.destination.zone_id,admitted.destination.instance_id,p[0],p[1],p[2],p[3],0);return true;
}
bool ConfigurePersonalTestAtSafeStartup(PersonalTestSelection s){if(s.template_content_only||s.guild_id||HouseTemplates::Type(s.item_template_id)!=3)return false;return ConfigureSelections({std::move(s)});}
InventoryBridge::Bridge::HousePolicies MakeForClient(Client& c,LiveItems::InventorySessionFence& f,HouseBinding::HouseApplicationProof& a){
 Selections all;std::optional<Scene::LobbyRequest> lobby;std::vector<HouseReviewedContent::Entry> entries;{std::lock_guard<std::mutex> guard(selection_mutex);lobby=verified_lobby_door;for(const auto& configured:registered_selections)if(configured){all.push_back(configured);for(const auto& entry:configured->content->Entries())entries.push_back(entry);}}
 InventoryBridge::Bridge::HousePolicies result;result.crates=CrateRuntime::MakeForClient(c,f,{});
 if(!all.empty()){auto content=std::make_shared<const HouseReviewedContent::Policies>(std::move(entries));result.scene=std::make_unique<ScenePolicy>(c,all,lobby,result.crates.get());result.room=std::make_unique<RoomPolicy>(c,all);result.custody=std::make_unique<Custody>(c,f,a,all,std::move(content));}return result;
}
bool LoadPersonalTestAtSafeStartup(){
 if(!CrateRuntime::LoadAtSafeStartup())return false;
 std::optional<PersonalTestSelection> personal,guild;
 if(!ContentLoader::ReadManifest(".house-personal-test.json",false,personal)||!ContentLoader::ReadManifest(".house-guild-test.json",true,guild))return false;
 std::vector<PersonalTestSelection> entries;if(personal)entries.push_back(std::move(*personal));if(guild)entries.push_back(std::move(*guild));
 if(!ContentLoader::ReadRegistry(".house-personal-content-registry-test.json",entries))return false;
 if(!ContentLoader::ReadRegistry(".house-guild-content-registry-test.json",entries,true))return false;
 if(!ContentLoader::ReadRegistry("housing-personal-content-registry.json",entries))return false;
 if(!ContentLoader::ReadRegistry("housing-guild-content-registry.json",entries,true))return false;
 if(entries.empty())return !CrateRuntime::ReloadPolicy();
 return ConfigureSelections(std::move(entries));
}
}
