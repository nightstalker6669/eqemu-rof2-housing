#include "housing_service_npcs.h"
#include "housing_pets.h"
#include "../common/housing/item_payload_db.h"
#include "housing_service_authority.h"
#include "../common/housing/centered_wall_policy.h"
#include "housing_plot_marker.h"
#include "../common/housing/plot_find_wire.h"
#include "../common/housing/exterior_exit_geometry.h"
#include "pathfinder_interface.h"
#include "../common/housing/global_myplots_db.h"
#include "../common/housing/plot_search_dispatch.h"
#include "../common/housing/stack_partition_application.h"
#include "../common/housing/npc_item_metadata.h"
#include "../common/housing/npc_runtime.h"
#include "../common/housing/npc_native_admission.h"
#include "../common/housing/upkeep_cycle_db.h"
#include "../common/housing/rent_state_db.h"
#include "../common/housing/interior_storage_service.h"
#include "client.h"
#include "zone.h"
#include "../common/housing/neighborhood_wire.h"
#include "../common/housing/neighborhood_directory_db.h"
#include "../common/rulesys.h"
#include "../common/strings.h"
#include "../common/eqemu_config.h"
#include "../common/housing/catalog_service.h"
#include "../common/housing/upkeep_selection_db.h"
#include "../common/housing/interior_native_view.h"
#include "../common/housing/custody_codec.h"
#include "../common/housing/permission_wire.h"
#include "../common/housing/placement_wire.h"
#include "../common/housing/interaction_wire.h"
#include "../common/patches/rof2.h"
#include "housing_inventory_bridge.h"
#include "../common/housing/house_scene_presentation.h"
#include "../common/housing/house_placement_preview.h"
#include "entity.h"
#include "doors.h"
#include <openssl/rand.h>
#include <map>
#include <cstring>
#include <fstream>
#include "../common/housing/current_estate_access.h"
#include "../common/housing/layout_native_intent.h"
#include "../common/housing/scene_item_projection.h"
#include "../common/housing/item_list_wire.h"
#include "../common/housing/foreign_furniture_return_policy.h"

extern Zone* zone;
extern ZoneDatabase database;
extern ZoneDatabase content_db;

namespace {
bool HandleHousingPlotSearch(Client*,const EQApplicationPacket*);
bool FurniturePlayerInRegion(Client*,const Housing::RegionWire::Region&);
bool FurniturePlayerInRegion(Client*,const Housing::Scene::Snapshot&);
bool HousingSession(Client* c) {
 return RuleB(Housing, EnablePlayerHousing) && c->ClientVersion() == EQ::versions::ClientVersion::RoF2 &&
        zone && zone->GetZoneID() == 344 && c->m_housing_gate_open;
}
void SendHousingPayload(Client* c, const std::vector<uint8_t>& payload) {
 auto packet = new EQApplicationPacket(OP_RealEstateNeighborhood, payload.size());
 std::memcpy(packet->pBuffer, payload.data(), payload.size());
 c->FastQueuePacket(&packet);
}
}

void Client::SendHousingNeighborhoods(const std::string& name_filter, const std::string& player_filter, bool owned_only, uint32_t context) {
 if (!HousingSession(this)) return;
 Housing::NeighborhoodWire::SearchRequest request;request.context_or_selected_id=context;
 request.neighborhood_name=name_filter;request.player_name=player_filter;
 try {auto result=Housing::NeighborhoodDirectory::Read(database,request,CharacterID(),owned_only);
  SendHousingPayload(this,Housing::NeighborhoodWire::EncodeList(result.rows,context));}
 catch(const std::exception&){Message(Chat::Red,"Housing neighborhood data is unavailable.");}
}

void Client::Handle_OP_RealEstateNeighborhood(const EQApplicationPacket* app) {
 if (HandleHousingPlotSearch(this,app)) return;
 if (!HousingSession(this)) return;
 auto request=Housing::NeighborhoodWire::DecodeSearch(app->pBuffer,app->size);
 if (!request) return;
 const bool mine=request->mode==0&&request->flag220==0&&request->player_name==GetName();
 try {
  auto result=Housing::NeighborhoodDirectory::Read(database,*request,CharacterID(),mine,request->mode==1?AccountID():0);
  if(request->mode==2)SendHousingPayload(this,Housing::NeighborhoodWire::EncodeOwners(result.players,result.guilds));
  else SendHousingPayload(this,Housing::NeighborhoodWire::EncodeList(result.rows,request->context_or_selected_id));
 }catch(const std::exception&){Message(Chat::Red,"Housing neighborhood data is unavailable.");}
}

void Client::Handle_OP_RealEstateGo(const EQApplicationPacket* app) {
 if(HandleHousingHouseGo(app))return;
 if (!HousingSession(this)) return;
 auto request=Housing::NeighborhoodWire::DecodeGo(app->pBuffer,app->size);
 if (!request || request->sentinel!=0xffffffff || request->reserved!=0) return;
 auto route=database.QueryDatabase("SELECT n.instance_id,i.zone,i.version FROM housing_neighborhoods n JOIN instance_list i ON i.id=n.instance_id WHERE n.id=" + std::to_string(request->selected_id) + " AND n.zone_id=712 AND n.zone_version=0 AND i.zone=n.zone_id AND i.version=n.zone_version AND i.never_expires=1 LIMIT 1");
 if (!route.Success() || route.RowCount()!=1) { Message(Chat::Red,"This neighborhood is unavailable.");return; }
 auto row=route.begin();auto instance=Strings::ToUnsignedInt(row[0]);
 if (!instance || instance>32000) return;
 auto point=content_db.QueryDatabase("SELECT safe_x,safe_y,safe_z,safe_heading FROM zone WHERE zoneidnumber=712 AND version=0 LIMIT 1");
 if (!point.Success() || point.RowCount()!=1) { Message(Chat::Red,"Neighborhood entry point is unavailable.");return; }
 // Membership admits this public neighborhood; no permission inheritance for interiors.
 if (!database.CheckInstanceByCharID(instance,CharacterID()) && !database.AddClientToInstance(instance,CharacterID())) {
  Message(Chat::Red,"Could not enter this neighborhood.");return;
 }
 auto safe=point.begin();
 m_housing_gate_open=false;
 MovePC(712,instance,Strings::ToFloat(safe[0]),Strings::ToFloat(safe[1]),Strings::ToFloat(safe[2]),Strings::ToFloat(safe[3]));
}


namespace {
// Native IDs belong to one verified catalog and never alias domain estate IDs.
// No fabricated catalog is supplied when this optional content is absent.
MySQLRequestResult QueryView(Database& db,const std::string& query){return db.QueryDatabase(query,false);}
struct HousingView {
 Housing::Catalog::Export data;
 uint32_t neighborhood=0,root=0;
 std::map<std::string,uint32_t> ids;
 std::map<uint32_t,Housing::EstateWire::EstateSnapshot> states;
 std::map<uint32_t,Housing::PermissionWire::Snapshot> permissions;
 std::map<uint32_t,uint64_t> acl_revisions;
 std::vector<uint32_t> owned;
 std::optional<Housing::Scene::InteriorSnapshot> interior;
};
bool HousingZone(Client* c) {
 return RuleB(Housing,EnablePlayerHousing) && c->ClientVersion()==EQ::versions::ClientVersion::RoF2 &&
        zone && zone->GetInstanceID()!=0 && (zone->GetZoneID()==712||Housing::OriginalInteriors::Matches(zone->GetZoneID(),zone->GetShortName()));
}
std::unique_ptr<Database> HousingConnection() {
 const auto* config=EQEmuConfig::get(); if(!config)return {};
 auto db=std::make_unique<Database>();
 if(!db->Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,
    config->DatabaseDB,config->DatabasePort,"housing-catalog-view"))return {};
 return db;
}
std::optional<HousingView> LoadHousingView(Client* c) {
 if(!HousingZone(c))return std::nullopt;
 auto db=HousingConnection();if(!db)return std::nullopt;
 std::optional<Housing::Scene::InteriorSnapshot> interior;HousingView view;std::string key;
 if(zone->GetZoneID()==712){
  auto route=QueryView(*db,"SELECT n.id,e.catalog_key FROM housing_neighborhoods n JOIN instance_list i ON i.id=n.instance_id JOIN housing_native_estates e ON e.neighborhood_id=n.id AND e.active=1 AND e.parent_native_id IS NULL WHERE n.instance_id="+std::to_string(zone->GetInstanceID())+" AND n.zone_id=712 AND n.zone_version=0 AND i.zone=n.zone_id AND i.version=n.zone_version AND i.never_expires=1");
  if(!route.Success()||route.RowCount()!=1)return std::nullopt;auto row=route.begin();view.neighborhood=Strings::ToUnsignedInt(row[0]);key=row[1];
 }else{
  auto* bridge=c->GetHousingInventoryBridge();if(!bridge)return {};auto current=bridge->ReadInterior();if(!current)current=bridge->ReadInteriorForManagement();
  if(!current||!current.value.house||!current.value.house->interior)return {};interior=std::move(current.value);key=interior->house->interior->catalog_key;
  auto route=QueryView(*db,"SELECT neighborhood_id FROM housing_native_estates WHERE native_id="+std::to_string(interior->context.native_plot_id)+" AND active=1");if(!route.Success()||route.RowCount()!=1)return {};view.neighborhood=Strings::ToUnsignedInt(route.begin()[0]);
 }
 Housing::Catalog::Service service(std::move(db),{true});
 auto loaded=service.Load(key);if(!loaded)return std::nullopt;view.data=std::move(loaded.value);
 for(const auto& n:view.data.catalog.nodes)view.ids.emplace(n.key,n.native_id.value);
 // A display-only consistent snapshot; mutation handlers must independently lock
 // and authorize their operation, never trust a previously sent owner/ACL.
 db=HousingConnection();if(!db)return std::nullopt;
 if(!QueryView(*db,"SET TRANSACTION ISOLATION LEVEL REPEATABLE READ").Success() ||
    !QueryView(*db,"START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY").Success())return std::nullopt;
 auto rollback=[&](){QueryView(*db,"ROLLBACK");};
 auto revision=QueryView(*db,"SELECT revision FROM housing_catalogs WHERE catalog_key='"+Strings::Escape(key)+"'");
 if(!revision.Success()||revision.RowCount()!=1||std::stoull(revision.begin()[0])!=view.data.catalog.revision){rollback();return std::nullopt;}
 for(const auto& n:view.data.catalog.nodes){
  if(n.neighborhood_id!=view.neighborhood)continue;
  Housing::EstateWire::EstateSnapshot state;state.estate_id=n.native_id.value;state.static_id=n.static_id;
  state.parent_id=n.parent_key.empty()?0:view.ids.at(n.parent_key);state.definition_id=n.definition_id;
  // Plot addresses are the catalog-backed display fallback for native cached
  // estate names (including transfer consent). No custom name is persisted.
  const auto display_definition=std::find_if(view.data.catalog.definitions.begin(),view.data.catalog.definitions.end(),[&](const auto& d){return d.id==n.definition_id;});
  if(display_definition==view.data.catalog.definitions.end()){rollback();return std::nullopt;}
  if(display_definition->type==4||display_definition->type==8)state.dynamic_name=display_definition->address;
  if(n.parent_key.empty()){if(view.root){rollback();return std::nullopt;}view.root=n.native_id.value;}
  for(const auto& child:view.data.catalog.nodes)if(child.parent_key==n.key)state.children.push_back(child.native_id.value);
  // A house is both an outer item and its own native estate. Native validators
  // test parent plot expiry first, then this current estate's +40 expiry.
  auto rented_house=QueryView(*db,"SELECT c.custody_id,p.native_item_id FROM housing_house_interiors i JOIN housing_house_locations l ON l.house_id=i.house_id AND l.location_kind=2 JOIN housing_live_item_custody c ON c.custody_id=l.custody_id JOIN housing_live_placed_items p ON p.custody_id=c.custody_id WHERE i.native_estate_id="+std::to_string(n.native_id.value));
  if(!rented_house.Success()||rented_house.RowCount()>1){rollback();return std::nullopt;}
  if(rented_house.RowCount())state.expiry_time=Housing::UpkeepCycle::Expired(*db,std::stoull(rented_house.begin()[0]),uint32_t(std::stoul(rented_house.begin()[1])));

  if(n.ownership_estate_id){
   auto owner=QueryView(*db,"SELECT e.owner_kind,e.owner_id,c.name,g.name FROM housing_current_estates e LEFT JOIN character_data c ON e.owner_kind=1 AND c.id=e.owner_id LEFT JOIN guilds g ON e.owner_kind=3 AND g.id=e.owner_id WHERE e.id="+std::to_string(n.ownership_estate_id));
   if(!owner.Success()||owner.RowCount()!=1){rollback();return std::nullopt;}
   auto o=owner.begin();auto kind=Strings::ToUnsignedInt(o[0]);auto id=Strings::ToUnsignedInt(o[1]);
   if(kind==1&&o[2])state.owner_name=state.owner_handle=o[2];
   else if(kind==3&&o[3]){state.guild_id=id;state.owner_name=state.owner_handle=o[3];}
   else {rollback();return std::nullopt;}
   auto preference=QueryView(*db,"SELECT voting_opted_out FROM housing_estate_preferences WHERE estate_id="+std::to_string(n.ownership_estate_id));
   if(!preference.Success()||preference.RowCount()>1){rollback();return std::nullopt;}
   if(preference.RowCount()){if(!preference.begin()[0]||Strings::ToUnsignedInt(preference.begin()[0])>1){rollback();return std::nullopt;}state.voting_opted_out=uint8_t(Strings::ToUnsignedInt(preference.begin()[0]));}
   const auto definition=std::find_if(view.data.catalog.definitions.begin(),view.data.catalog.definitions.end(),[&](const auto& d){return d.id==n.definition_id;});
   if(definition==view.data.catalog.definitions.end()){rollback();return std::nullopt;}
   state.expiry_time=Housing::RentState::Plot(*db,n.ownership_estate_id,n.native_id.value,false).expired_since;
   state.default_upkeep_component=Housing::UpkeepSelection::PublishedComponent(*db,n.ownership_estate_id,n.native_id.value,kind,id,*definition,view.data.catalog.costs);
   auto balances=QueryView(*db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+std::to_string(n.ownership_estate_id));
   if(!balances.Success()||balances.RowCount()>1){rollback();return std::nullopt;}
   if(balances.RowCount()){
    if(!balances.begin()[0]){rollback();return std::nullopt;}std::string text=balances.begin()[0];size_t end=0;const auto copper=std::stoll(text,&end);
    if(end!=text.size()||copper<0||text.empty()||!std::all_of(text.begin(),text.end(),[](char c){return c>='0'&&c<='9';})){rollback();return std::nullopt;}
    // Native63fd stores a double COPPER balance. Rounding above2^53 is a
    // display-format limitation; the ledger and debit authorization stay int64.
    const double display=static_cast<double>(copper);if(!std::isfinite(display)){rollback();return std::nullopt;}state.escrow.push_back({0,display});
   }
   if(kind==1||kind==3){
    Housing::PermissionWire::Snapshot acl;acl.estate_id=n.native_id.value;
    auto custom=QueryView(*db,"SELECT revision FROM housing_acl_state WHERE estate_id="+std::to_string(n.ownership_estate_id));
    if(!custom.Success()||custom.RowCount()>1){rollback();return std::nullopt;}
    view.acl_revisions.emplace(n.native_id.value,custom.RowCount()?std::stoull(custom.begin()[0]):0);
    if(custom.RowCount()){
     auto groups=QueryView(*db,"SELECT group_id,capabilities FROM housing_acl_groups WHERE estate_id="+std::to_string(n.ownership_estate_id)+" ORDER BY group_id");
     if(!groups.Success()||groups.RowCount()!=7){rollback();return std::nullopt;}
     uint32_t expected=0;for(auto g:groups){if(Strings::ToUnsignedInt(g[0])!=expected){rollback();return std::nullopt;}acl.masks[expected++]=Strings::ToUnsignedInt(g[1]);}
     auto members=QueryView(*db,"SELECT c.name,m.group_id FROM housing_acl_members m LEFT JOIN character_data c ON c.id=m.character_id WHERE m.estate_id="+std::to_string(n.ownership_estate_id)+" ORDER BY m.character_id");
     if(!members.Success()){rollback();return std::nullopt;}
     for(auto m:members){if(!m[0]){rollback();return std::nullopt;}acl.members.push_back({m[0],Strings::ToUnsignedInt(m[1])});}
     auto special=QueryView(*db,"SELECT subject_kind,subject_id,group_id FROM housing_acl_special_members WHERE estate_id="+std::to_string(n.ownership_estate_id)+" ORDER BY subject_kind,subject_id");
     if(!special.Success()||special.RowCount()>9){rollback();return std::nullopt;}
     for(auto m:special){const auto k=Strings::ToUnsignedInt(m[0]);if(k!=1&&k!=2){rollback();return std::nullopt;}
      Housing::AccessPolicy::Subject subject{k==1?Housing::AccessPolicy::SubjectKind::GuildRank:Housing::AccessPolicy::SubjectKind::Fellowship,Strings::ToUnsignedInt(m[1])};
      auto reserved=Housing::AccessPolicy::ReservedWireKey(subject);if(!reserved){rollback();return std::nullopt;}acl.members.push_back({*reserved,Strings::ToUnsignedInt(m[2])});
     }
    }
    view.permissions.emplace(n.native_id.value,std::move(acl));
   }
  }
  view.states.emplace(n.native_id.value,std::move(state));
 }
 // 2aa7 replaces the ENTIRE owned list, including other neighborhoods.
 auto owned=QueryView(*db,"SELECT n.native_id FROM housing_native_estates n JOIN housing_current_estates e ON e.id=n.ownership_estate_id WHERE n.active=1 AND ((e.owner_kind=1 AND e.owner_id="+std::to_string(c->CharacterID())+") OR (e.owner_kind=3 AND EXISTS (SELECT 1 FROM guild_members gm WHERE gm.guild_id=e.owner_id AND gm.char_id="+std::to_string(c->CharacterID())+" AND gm.`rank`=1))) ORDER BY n.native_id");
 if(!owned.Success()||!view.root){rollback();return std::nullopt;}
 for(auto o:owned)view.owned.push_back(Strings::ToUnsignedInt(o[0]));
 if(interior){
  // Display-only projection of the independently checked, placed house. The
  // house keeps its real parent and native identity; no second plot is minted.
  const auto& house=*interior->house;const auto native=house.interior->native_estate_id,parent=interior->context.native_plot_id;
  auto state=view.states.find(native),owner=view.states.find(parent);auto permissions=view.permissions.find(parent);
  auto current=QueryView(*db,"SELECT h.revision,h.pending_request_key,l.location_kind,l.custody_id FROM housing_house_instances h JOIN housing_house_locations l ON l.house_id=h.house_id WHERE h.house_id="+std::to_string(house.house_id));
  if(state==view.states.end()||owner==view.states.end()||permissions==view.permissions.end()||state->second.parent_id!=parent||state->second.definition_id!=house.interior->definition_id||!current.Success()||current.RowCount()!=1||std::stoull(current.begin()[0])!=house.revision||current.begin()[1]||Strings::ToUnsignedInt(current.begin()[2])!=2||std::stoull(current.begin()[3])!=house.location.custody_id){rollback();return {};}
  state->second.owner_name=owner->second.owner_name;state->second.owner_handle=owner->second.owner_handle;state->second.guild_id=owner->second.guild_id;
  auto inherited=permissions->second;inherited.estate_id=native;view.permissions.emplace(native,std::move(inherited));view.acl_revisions[native]=view.acl_revisions.at(parent);
  if(std::find(view.owned.begin(),view.owned.end(),parent)!=view.owned.end()&&std::find(view.owned.begin(),view.owned.end(),native)==view.owned.end())view.owned.push_back(native);
  view.root=native;
 }
 if(!QueryView(*db,"COMMIT").Success())return std::nullopt;
 if(interior){if(interior->room_profile&&!Housing::InteriorNativeView::Apply(view.data,*interior->room_profile,view.root,&interior->contents))return {};
  for(const auto& child:view.data.catalog.nodes)if(child.parent_key=="house:"+std::to_string(interior->house->house_id)){
   auto parent=view.states.find(view.root);auto acl=view.permissions.find(view.root);if(parent==view.states.end()||acl==view.permissions.end())return {};
   if(!view.states.count(child.native_id.value)){Housing::EstateWire::EstateSnapshot state;state.estate_id=child.native_id.value;state.parent_id=view.root;state.definition_id=child.definition_id;state.owner_name=parent->second.owner_name;state.owner_handle=parent->second.owner_handle;state.guild_id=parent->second.guild_id;view.states.emplace(state.estate_id,std::move(state));}
   if(std::find(parent->second.children.begin(),parent->second.children.end(),child.native_id.value)==parent->second.children.end())parent->second.children.push_back(child.native_id.value);
   auto inherited=acl->second;inherited.estate_id=child.native_id.value;view.permissions[child.native_id.value]=std::move(inherited);view.acl_revisions[child.native_id.value]=view.acl_revisions.at(view.root);view.ids[child.key]=child.native_id.value;
  }
  view.interior=std::move(interior);
 }
 return view;
}
void SendEstatePayload(Client* c,EmuOpcode opcode,const std::vector<uint8_t>& bytes){
 auto packet=new EQApplicationPacket(opcode,bytes.size());
 if(!bytes.empty())std::memcpy(packet->pBuffer,bytes.data(),bytes.size());c->FastQueuePacket(&packet);
}
bool HandleHousingPlotSearch(Client* c,const EQApplicationPacket* app){
 auto request=Housing::EstateWire::DecodePlotSearch(app->pBuffer,app->size);if(!request)return false;
 const auto kind=Housing::PlotSearchDispatch::Classify(*request);
 if(kind==Housing::PlotSearchDispatch::Kind::Invalid)return true;
 if(kind==Housing::PlotSearchDispatch::Kind::OwnedPlots){
  if(!RuleB(Housing,EnablePlayerHousing)||c->ClientVersion()!=EQ::versions::ClientVersion::RoF2||!c->Connected()||!c->ClientDataLoaded())return true;
  try{auto db=HousingConnection();if(!db)return true;auto rows=Housing::GlobalMyPlots::Read(*db,{c->CharacterID(),c->AccountID()});
   LogInfo("Native housing MyPlots search actor [{}] mode [{}] self-filter [{}] rows [{}]",c->CharacterID(),request->mode,uint32_t(request->source[Housing::PlotSearchDispatch::SelfFilterOffset]),rows.size());
   SendEstatePayload(c,OP_RealEstateNeighborhood,Housing::EstateWire::EncodePlotSearch(4,"My Plots",rows));
  }catch(const std::exception&){LogError("Housing global MyPlots view unavailable");}
  return true;
 }
 if(kind==Housing::PlotSearchDispatch::Kind::OwnedItems){
  if(!RuleB(Housing,EnablePlayerHousing)||c->ClientVersion()!=EQ::versions::ClientVersion::RoF2||!c->Connected()||!c->ClientDataLoaded())return true;
  try{auto db=HousingConnection();if(!db)return true;Housing::NeighborhoodWire::SearchRequest items;items.mode=1;
   auto result=Housing::NeighborhoodDirectory::Read(*db,items,c->CharacterID(),false,c->AccountID());
   LogInfo("Native housing MyItems search actor [{}] current plots [{}]",c->CharacterID(),result.item_plots.size());
   SendEstatePayload(c,OP_RealEstateNeighborhood,Housing::EstateWire::EncodePlotSearch(5,"My Items",result.item_plots));
  }catch(const std::exception&){LogError("Housing current MyItems view unavailable");}
  return true;
 }
 if(!HousingZone(c))return false;
 // Mode4 retains the authoritative current-root and explicit owner-search scope.
 try {
  auto view=LoadHousingView(c);if(!view||request->context!=view->root)return true;
  std::vector<Housing::EstateWire::PlotSearchRow> rows;
  const auto query=Housing::PermissionWire::Detail::Key(request->owner_query);
  for(const auto& state:view->states){
   auto d=std::find_if(view->data.catalog.definitions.begin(),view->data.catalog.definitions.end(),[&](const auto& def){return def.id==state.second.definition_id;});
   if(d==view->data.catalog.definitions.end()||(d->type!=4&&d->type!=8))continue;
   if(!query.empty()&&Housing::PermissionWire::Detail::Key(state.second.owner_name).find(query)==std::string::npos)continue;
   // Original XML labels native stringA's fourth column Owner, stringB's
   // second column Address. Unknown numeric/icon identity remains zero.
   rows.push_back({0,d->type,state.first,{state.second.owner_name,d->address}});
  }
  SendEstatePayload(c,OP_RealEstateNeighborhood,Housing::EstateWire::EncodePlotSearch(4,"Plot Search",rows));
 }catch(const std::exception&){LogError("Housing plot search could not be encoded");}
 return true;
}

}
namespace Housing::PlotMarker {
std::optional<uint32_t> InspectionEstate(Client* client,uint32_t static_id,float maximum,bool within_distance){
 try{
  if(!client||!client->Connected()||!client->ClientDataLoaded()||!HousingZone(client)||!zone||zone->GetZoneID()!=712||zone->GetInstanceVersion()||!zone->GetInstanceID())return std::nullopt;
  auto view=LoadHousingView(client);if(!view)return std::nullopt;
  const auto* plot=Resolve(view->data.catalog,view->neighborhood,view->root,static_id);
  if(!plot||(!within_distance&&!CatalogInRange(view->data.catalog,view->neighborhood,view->root,static_id,{client->GetX(),client->GetY(),client->GetZ()},maximum)))return std::nullopt;
  return plot->native_id.value;
 }catch(...){return std::nullopt;}
}
bool InRange(Client* client,uint32_t static_id,float maximum){
 try{
  if(!client||!client->Connected()||!client->ClientDataLoaded()||!HousingZone(client)||!zone||zone->GetZoneID()!=712||zone->GetInstanceVersion())return false;
  auto view=LoadHousingView(client);if(!view)return false;
  return CatalogInRange(view->data.catalog,view->neighborhood,view->root,static_id,{client->GetX(),client->GetY(),client->GetZ()},maximum);
 }catch(...){return false;}
}
}
void Client::SendHousingZoneCatalog(){ (void)SendHousingZoneCatalogChecked(); }
bool Client::SendHousingZoneCatalogChecked(){
 // Trusted quest handshake only; never admission or furnishing authority.
 // Clear it on every refresh before proving the currently occupied interior.
 DeleteEntityVariable("housing_verified_interior");
 try {
  auto view=LoadHousingView(this);if(!view)return false;
  if(view->interior&&view->interior->house&&view->interior->house->interior)
   SetEntityVariable("housing_verified_interior",std::to_string(zone->GetInstanceID()));
  LogInfo("Native housing catalog queued actor [{}] current estate [{}] zone [{}] instance [{}]",CharacterID(),view->root,zone->GetZoneID(),zone->GetInstanceID());
  // Encode everything before sending any cache-clearing catalog packet.
  std::vector<std::pair<EmuOpcode,std::vector<uint8_t>>> packets;
  packets.emplace_back(OP_RealEstateDefinitions,view->data.definition_payload);
  packets.emplace_back(OP_RealEstateRegions,view->data.region_payload);
  packets.emplace_back(OP_RealEstateZoneContext,Housing::EstateWire::EncodeEstateSnapshot(view->states.at(view->root)));
  for(const auto& state:view->states)if(state.first!=view->root)packets.emplace_back(OP_RealEstateSnapshot,Housing::EstateWire::EncodeEstateSnapshot(state.second));
  for(const auto& acl:view->permissions)packets.emplace_back(OP_RealEstateAccess,Housing::PermissionWire::EncodeSnapshot(acl.second));
  packets.emplace_back(OP_RealEstateOwnedList,Housing::PermissionWire::EncodeOwned(view->owned));
  ClearHousingAclRevisions();
  for(const auto& packet:packets){SendEstatePayload(this,packet.first,packet.second);
   if(packet.first==OP_RealEstateAccess){const auto id=Housing::NeighborhoodWire::Detail::Read32(packet.second.data());RememberHousingAclRevision(id,view->acl_revisions.at(id));}
  }
  return true;
 } catch(const std::exception&){LogError("Housing catalog view could not be encoded");return false;}
}
void Client::Handle_OP_RealEstateInspect(const EQApplicationPacket* app){
 if(auto* bridge=GetHousingInventoryBridge())bridge->ForgetPurchaseInspection();
 try {
  auto requested=Housing::EstateWire::DecodeInspect(app->pBuffer,app->size);if(!requested)return;
  auto view=LoadHousingView(this);if(!view)return;
  auto state=view->states.find(*requested);if(state==view->states.end())return;
  auto def=std::find_if(view->data.catalog.definitions.begin(),view->data.catalog.definitions.end(),[&](const auto& d){return d.id==state->second.definition_id;});
  if(def==view->data.catalog.definitions.end()||(def->type!=4&&def->type!=8))return;
  Housing::EstateWire::Inspection inspect;inspect.estate_id=state->first;inspect.definition_id=def->id;
  inspect.parent_id=state->second.parent_id;inspect.guild_id=state->second.guild_id;inspect.fellowship_id=state->second.fellowship_id;inspect.type=def->type;
  inspect.strings={"",def->address,state->second.owner_name,state->second.owner_handle};
  auto bytes=Housing::EstateWire::EncodeInspection(inspect);
  if(auto* bridge=GetHousingInventoryBridge()){
   Housing::EstateId generation=0;
   if(def->type==4)for(const auto& node:view->data.catalog.nodes)if(node.native_id.value==state->first)generation=node.ownership_estate_id;
   bridge->RememberRetirementInspection(generation,state->first);
   if(bridge->RememberPurchaseInspection(state->first,view->data)){
    // Publish the SAME exact bootstrap that was bound by the purchase offer.
    // Sending only an inspection could leave an older native price cache alive.
    std::vector<std::pair<EmuOpcode,std::vector<uint8_t>>> quoted;
    quoted.emplace_back(OP_RealEstateDefinitions,view->data.definition_payload);
    quoted.emplace_back(OP_RealEstateRegions,view->data.region_payload);
    quoted.emplace_back(OP_RealEstateZoneContext,Housing::EstateWire::EncodeEstateSnapshot(view->states.at(view->root)));
    for(const auto& entry:view->states)if(entry.first!=view->root)quoted.emplace_back(OP_RealEstateSnapshot,Housing::EstateWire::EncodeEstateSnapshot(entry.second));
    for(const auto& entry:view->permissions)quoted.emplace_back(OP_RealEstateAccess,Housing::PermissionWire::EncodeSnapshot(entry.second));
    quoted.emplace_back(OP_RealEstateOwnedList,Housing::PermissionWire::EncodeOwned(view->owned));
    ClearHousingAclRevisions();
    for(const auto& packet:quoted){SendEstatePayload(this,packet.first,packet.second);
     if(packet.first==OP_RealEstateAccess){const auto id=Housing::NeighborhoodWire::Detail::Read32(packet.second.data());RememberHousingAclRevision(id,view->acl_revisions.at(id));}
    }
   }
  }
  auto acl=view->permissions.find(state->first);
  if(acl!=view->permissions.end()){SendEstatePayload(this,OP_RealEstateAccess,Housing::PermissionWire::EncodeSnapshot(acl->second));RememberHousingAclRevision(acl->first,view->acl_revisions.at(acl->first));}
  SendEstatePayload(this,OP_RealEstateNeighborhood,bytes);
 }catch(const std::exception&){if(auto* bridge=GetHousingInventoryBridge())bridge->ForgetPurchaseInspection();LogError("Housing inspection view could not be encoded");}
}

namespace Housing::NativePlacement {
struct Pending {
 LiveItems::Reservation reservation;
 LiveItems::InventoryRow row;
 int32_t serial=0;
 PlacementWire::Source source;
 std::unique_ptr<EQ::ItemInstance> presentation;
 std::optional<LiveItems::HouseIdentity> house_identity;
 std::optional<LiveItems::CrateIdentity> crate_identity;
 std::optional<LiveItems::CrateMemberContext> crate_member;
 std::shared_ptr<const CrateBinding::Allocation> crate_before;
 std::optional<LiveItems::InteriorReservation> room;
 std::optional<Route> room_exterior_source;
 std::shared_ptr<const HouseBinding::Binding> house_before;
 std::string reserve_key,store_key;
 std::vector<uint8_t> begin_bytes,commit_bytes;
 bool completed=false;
 bool cursor_notified=false;
};
struct Session {
 struct Swap {std::string key;bool completed=false;};
 std::map<std::vector<uint8_t>,Swap> house_swaps;
 struct VaultPending {std::string key;LiveItems::InventoryRow row;LiveItems::VaultContext context;std::optional<VaultStorage::Item> item;std::optional<LiveItems::VaultTransferRequest> transfer;int32_t serial=0;bool completed=false,partition=false;};
 std::map<std::vector<uint8_t>,VaultPending> vault_actions;
 bool geometry_preview_only=false;
 std::optional<Pending> pending;
 struct Move {
  LiveItems::Stored before;
  std::shared_ptr<const HouseBinding::Binding> house;
  std::shared_ptr<const CrateBinding::Allocation> crate;
  std::optional<LiveItems::InteriorMoveContext> room;
  std::optional<LiveItems::CrateMemberContext> crate_member;
  std::optional<InteriorStorage::TransferRequest> transfer;
  std::optional<InteriorStorage::TransferReceipt> transferred;
  PlacementWire::Source source;
  std::unique_ptr<EQ::ItemInstance> presentation;
  std::string key;
  std::vector<uint8_t> begin_bytes,commit_bytes;
  bool completed=false;
 };
 std::optional<Move> move;
 std::map<uint32_t,std::unique_ptr<EQ::ItemInstance>> presentations;
 std::map<uint32_t,Scene::InteriorContent> room_items;
 std::optional<LiveItems::InteriorMoveContext> room_context;
 std::map<std::vector<uint8_t>,Housing::InteriorStorage::Request> storage_requests;
 std::optional<Housing::InteriorStorage::StowRequest> bulk_storage_request;
 std::map<std::vector<uint8_t>,Pending> inventory_storage_requests;
 std::map<std::vector<uint8_t>,Housing::InteriorStorage::TransferRequest> transfer_requests;
 std::map<uint32_t,Scene::InteriorSnapshot> attached_rooms;
 std::map<uint32_t,CrateBinding::Binding> attached_crates;
 std::map<uint32_t,Scene::InteriorSnapshot> attached_crate_rooms;
 std::set<uint32_t> contained_items;
 struct CrateTransfer {CrateStorage::Request request;bool completed=false;};
 std::map<std::vector<uint8_t>,CrateTransfer> crate_transfers;
 std::optional<CrateStorage::BatchRequest> crate_batch;
 struct World {std::string guid;std::vector<uint8_t> packet;};
 std::map<uint32_t,std::map<uint32_t,World>> sent_worlds;
 struct Pickup {
  std::vector<uint8_t> request;
  std::string key;
  uint32_t estate=0;
  uint64_t custody=0;
  std::optional<LiveItems::HouseIdentity> house_identity;
  std::optional<LiveItems::CrateIdentity> crate_identity;
  std::optional<LiveItems::CrateMemberContext> crate_member;
  std::optional<LiveItems::Stored> crate_item;
  std::optional<Scene::InteriorContent> room_item;std::optional<LiveItems::InteriorMoveContext> room_context;std::optional<Route> room_exterior_source;
  int32_t returned_serial=0;
  bool completed=false,partition=false;
  std::unique_ptr<EQ::ItemInstance> presentation;
 };
 std::map<uint32_t,Pickup> pickups;
};
}
namespace {
using Housing::NativePlacement::Session;
using FurniturePending=Housing::NativePlacement::Pending;
using Bytes=std::vector<uint8_t>;
constexpr size_t FurnitureRequestHistoryLimit=128;
bool FurnitureTrimPickupHistory(Session& session){
 while(session.pickups.size()>=FurnitureRequestHistoryLimit){auto old=std::find_if(session.pickups.begin(),session.pickups.end(),[](const auto& e){return e.second.completed;});if(old==session.pickups.end())return false;session.pickups.erase(old);}return true;
}
bool NativeFurniture(Client* c){return HousingZone(c)&&RuleB(Housing,EnableNativeFurniturePlacement)&&c->Connected();}
std::string FurnitureKey(const char* prefix){
 unsigned char nonce[16];if(RAND_bytes(nonce,sizeof(nonce))!=1)throw std::runtime_error("Housing request nonce failed");
 const char* hex="0123456789abcdef";std::string key=prefix;
 for(auto n:nonce){key+=hex[n>>4];key+=hex[n&15];}return key;
}
std::string FurnitureGuid(const EQ::ItemInstance& item){
 if(item.GetSerialNumber()<=0)throw std::invalid_argument("Unsupported housing runtime serial");
 return fmt::format("{:016}",item.GetSerialNumber());
}
bool CursorSource(const Housing::PlacementWire::Source& source){
 // Actual ItemGlobalIndex is DWORD kind, THREE packed signed shorts, padding.
 // Bytes30/31 and GUID padding are not authorization fields.
 return Housing::PlacementWire::InventorySlotMatches(source,33);
}
bool StoredSource(const Housing::PlacementWire::Source& source){
 return source.location[0]==0xffffffff&&source.location[1]==0xffffffff&&uint16_t(source.location[2])==0xffff;
}
bool StoredCommitSource(const Housing::PlacementWire::Source& source){
 // Native4d60ff zero-extends preview byte FF,558c0c copies the decoded
 // index, and5b3fd8 copies that DWORD into Commit20. Begin retainsFFFFFFFF.
 return source.location[0]==0xff&&source.location[1]==0xffffffff&&uint16_t(source.location[2])==0xffff;
}
bool SourceGuid(const Housing::PlacementWire::Source& source,const EQ::ItemInstance& item){
 const auto guid=FurnitureGuid(item);
 return !source.guid[16]&&!source.guid[17]&&std::equal(guid.begin(),guid.end(),source.guid.begin());
}
const Housing::StaticFurniture::Record* FurnitureModel(const EQ::ItemInstance& item){
 const auto* d=item.GetUnscaledItem();if(!d||!Housing::NpcItemMetadata::Ordinary(*d))return nullptr;
 const auto* model=Housing::StaticFurniture::Lookup(d->ID,std::string(d->IDFile));
 return model&&d->ItemClass==EQ::item::ItemClassCommon&&!item.IsScaling()&&
  Housing::StaticFurniture::PlacementTypeMatches(d->ID,d->HousingPlacementType)&&Housing::StaticFurniture::NativeProfile(d->ID,d->Placeable,d->HousingIgnoreCollisions,d->HousingDefinitionID,d->HousingMinimumScale,d->HousingMaximumScale,d->HousingUpkeepID,d->HousingMaxPerEstate)&&!d->BagSlots&&(d->Placeable&142)!=0&&std::isfinite(d->HousingMinimumScale)&&
  std::isfinite(d->HousingMaximumScale)&&d->HousingMinimumScale>0&&d->HousingMinimumScale<=1&&d->HousingMaximumScale>=1?model:nullptr;
}
std::optional<Housing::NpcRuntime::Profile> FurnitureNpc(const EQ::ItemInstance& item){
 const auto* d=item.GetUnscaledItem();if(!d||d->ItemClass!=EQ::item::ItemClassCommon||item.IsScaling()||d->EvolvingItem||d->BagSlots||item.IsStackable()||Housing::StaticFurniture::Lookup(d->ID)||Housing::HouseTemplates::Type(d->ID)||Housing::CrateGeometry::TemplateDefinition(d->ID)||d->HousingPlacementType||d->HousingDefinitionID||!std::isfinite(d->HousingMinimumScale)||!std::isfinite(d->HousingMaximumScale)||d->HousingMinimumScale<=0||d->HousingMinimumScale>1||d->HousingMaximumScale<1||!(d->Placeable&142))return {};
 return Housing::NpcRuntime::Cached(*d);
}
bool KnownFurniture(const EQ::ItemInstance& item){return FurnitureModel(item)||FurnitureNpc(item).has_value();}
bool KnownChair(const EQ::ItemInstance& item){return FurnitureModel(item)!=nullptr;}
bool KnownPlacedModel(const EQ::ItemInstance& item){return KnownFurniture(item)||Housing::HousePlacement::Model(item)||Housing::CratePlacement::Model(item);}
std::optional<double> FurnitureDistance(Client* c,const EQ::ItemInstance& item,const Housing::PlacementPolicy::Transform& pose){
 if(Housing::HousePlacement::Model(item))return Housing::HousePlacement::DistanceToBounds(item,pose,{c->GetY(),c->GetX(),c->GetZ()});
 // A ceiling attachment can be far above a reachable hanging furnishing.
 // Reuse the reviewed oriented-body proximity policy already used for houses.
 // This remains a server interaction limit; ACL/custody checks are independent.
 if(const auto* model=FurnitureModel(item))return Housing::HousePlacement::DistanceToGeometryBounds(model->geometry,pose,{c->GetY(),c->GetX(),c->GetZ()});
 const auto& p=pose.position;const double distance=std::hypot(double(p[0])-c->GetY(),double(p[1])-c->GetX(),double(p[2])-c->GetZ());
 return std::isfinite(distance)?std::optional<double>(distance):std::nullopt;
}
Housing::PlacementPolicy::Geometry FurnitureGeometry(const EQ::ItemInstance& item){
 if(const auto* house=Housing::HousePlacement::Model(item))return house->geometry;
 if(const auto* crate=Housing::CratePlacement::Model(item))return crate->geometry;
 if(auto npc=FurnitureNpc(item))return Housing::NpcRuntime::Geometry(*npc);
 const auto* model=FurnitureModel(item);if(!model)throw std::invalid_argument("Unsupported furniture geometry");return model->geometry;
}
Housing::PlacementPolicy::ItemMetadata FurnitureMetadata(const EQ::ItemInstance& item,uint32_t house_definition_type=0){
 if(house_definition_type==7){const auto* d=item.GetUnscaledItem();auto m=d?Housing::CratePlacement::Metadata(item,d->HousingDefinitionID,7):std::nullopt;if(!m)throw std::invalid_argument("Unsupported crate metadata");return *m;}
 if(house_definition_type){const auto* d=item.GetUnscaledItem();auto metadata=d?Housing::HousePlacement::Metadata(item,d->HousingDefinitionID,house_definition_type):std::nullopt;if(!metadata)throw std::invalid_argument("Unsupported house metadata");return *metadata;}
 if(auto npc=FurnitureNpc(item)){const auto* d=item.GetUnscaledItem();return {d->ID,npc->model_id,d->Placeable,d->HousingMinimumScale,d->HousingMaximumScale,false};}
 const auto* model=FurnitureModel(item);if(!model)throw std::invalid_argument("Unsupported furniture metadata");
 const auto* d=item.GetUnscaledItem();return {d->ID,model->model_id,d->Placeable,d->HousingMinimumScale,d->HousingMaximumScale,true};
}
std::optional<Housing::LiveItems::InventoryRow> FurnitureInventory(Client* c,uint32_t slot=33){
 if((slot<23||slot>32)&&slot!=33)return {};
 auto db=HousingConnection();if(!db)return {};
 if(!QueryView(*db,"START TRANSACTION").Success())return {};
 struct SnapshotRollback {Database& db;~SnapshotRollback(){db.QueryDatabase(std::string("ROLLBACK"),false);}} rollback{*db};
 auto result=QueryView(*db,"SELECT character_id,slot_id,item_id,charges,color,augment_one,augment_two,augment_three,augment_four,augment_five,augment_six,instnodrop,HEX(custom_data),ornament_icon,ornament_idfile,ornament_hero_model,guid FROM inventory WHERE character_id="+std::to_string(c->CharacterID())+" AND slot_id="+std::to_string(slot));
 if(!result.Success()||result.RowCount()!=1)return {};
 auto row=result.begin();Housing::LiveItems::InventoryRow out;auto& v=out.values;
 auto number=[&](int field,uint64_t maximum){if(!row[field])return uint64_t(0);std::string text=row[field];size_t end=0;auto n=std::stoull(text,&end);if(end!=text.size()||text.empty()||text[0]=='-'||n>maximum)throw std::invalid_argument("Invalid inventory field");return uint64_t(n);};
 v.character_id=number(0,UINT32_MAX);v.slot_id=number(1,UINT32_MAX);v.item_id=number(2,UINT32_MAX);v.charges=number(3,UINT16_MAX);v.color=number(4,UINT32_MAX);
 v.augment_one=number(5,UINT32_MAX);v.augment_two=number(6,UINT32_MAX);v.augment_three=number(7,UINT32_MAX);v.augment_four=number(8,UINT32_MAX);v.augment_five=number(9,UINT32_MAX);v.augment_six=number(10,UINT32_MAX);v.instnodrop=number(11,UINT8_MAX);
 if(row[12]){std::string hex=row[12];if(hex.size()>131070||hex.size()%2)throw std::invalid_argument("Invalid inventory data");
  auto digit=[](char x)->unsigned{if(x>='0'&&x<='9')return x-'0';if(x>='A'&&x<='F')return x-'A'+10;if(x>='a'&&x<='f')return x-'a'+10;throw std::invalid_argument("Invalid inventory hex");};
  for(size_t n=0;n<hex.size();n+=2)v.custom_data+=char((digit(hex[n])<<4)|digit(hex[n+1]));}
 v.ornament_icon=number(13,UINT32_MAX);v.ornament_idfile=number(14,UINT32_MAX);
 if(!row[15])return {};size_t end=0;std::string hero=row[15];auto h=std::stoll(hero,&end);if(end!=hero.size()||h<INT32_MIN||h>INT32_MAX)return {};v.ornament_hero_model=h;
 v.guid=number(16,UINT64_MAX);if(!row[2])out.null_fields|=Housing::LiveItems::NullItemId;if(!row[3])out.null_fields|=Housing::LiveItems::NullCharges;if(!row[12])out.null_fields|=Housing::LiveItems::NullCustomData;if(!row[16])out.null_fields|=Housing::LiveItems::NullGuid;
 auto full=Housing::ItemPayload::Complete(*db,std::move(out));auto* memory=c->GetInv().GetItem(int16_t(slot));if(!memory)return {};Housing::ItemPayload::Capture(*db,full,*memory);return full;
}
struct FurnitureTarget {Housing::Catalog::Node node;Housing::EstateWire::Definition definition;Housing::RegionWire::Region region;uint32_t root=0;std::string key;uint64_t revision=0;uint32_t house_definition_type=0;std::optional<Housing::Scene::InteriorSnapshot> room;};
std::optional<Housing::InteriorContent::Profile> FurnitureAdmissionRoom(const FurnitureTarget& target,const EQ::ItemInstance& item,uint32_t native,const std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>>& reserved_digest={}){
 if(!target.room||!target.room->room_profile)return {};const auto& base=*target.room->room_profile;
 auto input=Housing::InteriorWallSlab::CachedInput(base,*item.GetUnscaledItem());if(!input)return base;
 auto digest=reserved_digest;if(!digest)for(const auto* rows:{&target.room->contents,&target.room->crate_contents})for(const auto& entry:*rows)if(entry.room&&entry.stored.placement&&entry.stored.placement->reservation.native_item_id==native){if(digest)throw std::invalid_argument("Ambiguous wall source tag");digest=entry.room->content_digest;}
 if(!digest)throw std::invalid_argument("Missing trusted wall preview binding");auto policy=Housing::InteriorWallSlab::Reviewed(base);auto selected=Housing::CenteredWall::Select(*input,policy?&*policy:nullptr,digest);if(!selected)throw std::invalid_argument("Wall preview binding/cache mismatch");return selected->input.room;
}
bool FurnitureCheckPlacement(const FurnitureTarget& target,const Housing::PlacementPolicy::Context& input_context,const Housing::PlacementPolicy::ItemMetadata& metadata,const Housing::PlacementPolicy::Geometry& geometry,const Housing::PlacementPolicy::Transform& pose,Housing::PlacementPolicy::Operation operation,const EQ::ItemInstance* item=nullptr,uint32_t native=0,const std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>>& digest={}){
 auto context=input_context;context.geometry_only=true; // Preview/preflight only; the owning TX supplies contact authority.
 if(target.room){if(!target.room->room_profile)return false;auto selected=item?FurnitureAdmissionRoom(target,*item,native,digest):std::optional<Housing::InteriorContent::Profile>{*target.room->room_profile};return selected&&bool(Housing::InteriorContent::CheckPlacement(*selected,context,metadata,geometry,pose,operation));}
 return bool(Housing::PlacementPolicy::Check(context,metadata,geometry,target.region,pose,operation));
}
std::vector<std::pair<uint32_t,uint32_t>> FurnitureRegions(const FurnitureTarget& target,const EQ::ItemInstance* item=nullptr,uint32_t native=0,const std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>>& digest={}){
 std::vector<std::pair<uint32_t,uint32_t>> result;
 if(target.room){if(!target.room->room_profile)throw std::invalid_argument("Missing room regions");auto selected=item?FurnitureAdmissionRoom(target,*item,native,digest):std::optional<Housing::InteriorContent::Profile>{*target.room->room_profile};if(!selected)throw std::invalid_argument("Missing selected room profile");for(const auto& region:Housing::InteriorContent::Regions(*selected))result.emplace_back(region.id,0);}
 else result.emplace_back(target.region.id,0);return result;
}
// The client tests the animated renderer box; persisted NPC placement uses its
// canonical ground frame. Publish only a temporary, reviewed presentation box.
// All mutation/preflight checks above continue to use the original room profile.
std::optional<std::vector<std::pair<uint32_t,uint32_t>>> FurnitureNpcNativeRegions(Client* c,const FurnitureTarget& target,const EQ::ItemInstance& item){
 if(!target.room||!target.room->room_profile)return {};auto npc=FurnitureNpc(item);
 const auto& base=*target.room->room_profile;if(!base.reviewed_room||!base.house_id||base.native_estate_id!=target.node.native_id.value)throw std::invalid_argument("NPC admission room identity");
 auto view=LoadHousingView(c);if(!view||!view->interior||!view->interior->room_profile||view->data.catalog.key!=target.key||view->data.catalog.revision!=target.revision||view->interior->room_profile->house_id!=base.house_id||view->interior->room_profile->manifest_sha256!=base.manifest_sha256||view->interior->room_profile->native_estate_id!=base.native_estate_id)throw std::invalid_argument("NPC admission view changed");
 // Restore the canonical collection for ordinary previews. This removes any
 // earlier NPC projection independently of native status-map update semantics.
 if(!npc||!Housing::NpcNativeAdmission::Reviewed(*npc)){SendEstatePayload(c,OP_RealEstateRegions,Housing::RegionWire::EncodeBootstrap(view->data.catalog.regions));return {};}
 const uint32_t id=0x7d000000u|(base.region.id&0x00ffffffu);
 if(std::any_of(view->data.catalog.regions.begin(),view->data.catalog.regions.end(),[&](const auto& r){return r.id==id;}))throw std::invalid_argument("NPC admission region collision");
 auto projection=Housing::NpcNativeAdmission::Build(*npc,Housing::InteriorContent::Regions(base),id);auto regions=view->data.catalog.regions;regions.push_back(projection.region);
 //49df replaces, rather than appends, the native collection. Preserve every
 // current base/connected/yard region and whitelist only this preview's extra ID.
 const auto bytes=Housing::RegionWire::EncodeBootstrap(regions);auto status=FurnitureRegions(target,&item);status.emplace_back(id,0);SendEstatePayload(c,OP_RealEstateRegions,bytes);return status;
}
bool FurnitureCurrentCatalog(const Housing::LiveItems::Stored& row,const FurnitureTarget& target,uint64_t witnessed=0){
 // This witness comes only from a fresh, authorized server read. Presence of
 // a stored digest alone establishes neither current content nor permission.
 if(!row.placement||(witnessed?witnessed:row.current_catalog_revision)!=target.revision)return false;
 const auto& p=row.placement->reservation;
 if(target.room){const auto matches=[&](const auto& entry){return entry.stored.custody_id==row.custody_id&&entry.room&&entry.room->house_id==target.room->house->house_id;};return (std::any_of(target.room->contents.begin(),target.room->contents.end(),matches)||std::any_of(target.room->crate_contents.begin(),target.room->crate_contents.end(),matches))&&p.native_estate_id==target.node.native_id.value&&p.estate_id==row.estate_id&&p.catalog_revision>0;}
 return p.native_estate_id==target.node.native_id.value && p.estate_id==target.node.ownership_estate_id && p.catalog_key==target.key &&
        (p.content_binding || p.catalog_revision==target.revision);
}
std::optional<FurnitureTarget> FurniturePlot(Client* c,const HousingView& view,const EQ::ItemInstance& item,uint32_t requested=0,const Housing::Scene::InteriorSnapshot* attached=nullptr){
 if(view.interior||attached){
  const auto& current=view.interior?*view.interior:*attached;if(!current.house||!current.house->interior||!current.room_profile||Housing::HousePlacement::Model(item)||(requested&&requested!=current.house->interior->native_estate_id))return {};
  auto node=std::find_if(view.data.catalog.nodes.begin(),view.data.catalog.nodes.end(),[&](const auto& n){return n.native_id.value==current.house->interior->native_estate_id;});if(node==view.data.catalog.nodes.end())return {};
  uint32_t child_type=0;if(Housing::CratePlacement::Model(item)){const auto* data=item.GetUnscaledItem();auto child=std::find_if(current.room_profile->child_definitions.begin(),current.room_profile->child_definitions.end(),[&](const auto& d){return d.id==data->HousingDefinitionID&&d.type==7;});if(child==current.room_profile->child_definitions.end()||!current.room_profile->definition.max_children||std::find(current.room_profile->definition.children.begin(),current.room_profile->definition.children.end(),child->id)==current.room_profile->definition.children.end())return {};child_type=7;}
  auto display=*node;display.ownership_estate_id=current.context.estate_id;
  return FurnitureTarget{display,current.room_profile->definition,current.room_profile->region,view.root,view.data.catalog.key,view.data.catalog.revision,child_type,current};
 }
 std::optional<FurnitureTarget> found,owned_fallback;bool ambiguous_owned=false;
 Housing::PlacementPolicy::Transform here{c->GetHeading(),0,0,Housing::HousePlacement::Model(item)?Housing::HousePlacement::DefaultScale(item):Housing::CratePlacement::Model(item)?std::clamp(1.f,item.GetUnscaledItem()->HousingMinimumScale,item.GetUnscaledItem()->HousingMaximumScale):1.f,{c->GetY(),c->GetX(),c->GetZ()}};
 for(const auto& n:view.data.catalog.nodes){
  if(n.neighborhood_id!=view.neighborhood||!n.ownership_estate_id||n.parent_key.empty()||view.ids.at(n.parent_key)!=view.root || (requested&&requested!=n.native_id.value))continue;
  auto d=std::find_if(view.data.catalog.definitions.begin(),view.data.catalog.definitions.end(),[&](const auto& x){return x.id==n.definition_id;});
  if(d==view.data.catalog.definitions.end()||(d->type!=4&&d->type!=8))continue;
  auto r=std::find_if(view.data.catalog.regions.begin(),view.data.catalog.regions.end(),[&](const auto& x){return x.id==d->area_id;});if(r==view.data.catalog.regions.end())continue;
  uint32_t house_type=0;
  if(Housing::HousePlacement::Model(item)){
   const auto* data=item.GetUnscaledItem();
   auto child=std::find_if(view.data.catalog.definitions.begin(),view.data.catalog.definitions.end(),[&](const auto& x){return x.id==data->HousingDefinitionID;});
   if(child==view.data.catalog.definitions.end()||(child->type!=2&&child->type!=3)||(child->type==2&&d->type!=8)||(child->type==3&&d->type!=4)||std::find(d->children.begin(),d->children.end(),child->id)==d->children.end())continue;
   house_type=child->type;
  }
  if(Housing::CratePlacement::Model(item)){
   const auto* data=item.GetUnscaledItem();auto child=std::find_if(view.data.catalog.definitions.begin(),view.data.catalog.definitions.end(),[&](const auto& x){return x.id==data->HousingDefinitionID;});
   if(child==view.data.catalog.definitions.end()||child->type!=7||std::find(d->children.begin(),d->children.end(),child->id)==d->children.end())continue;house_type=7;
  }
  if(house_type&&!requested&&std::find(view.owned.begin(),view.owned.end(),n.native_id.value)!=view.owned.end()){
   if(owned_fallback)ambiguous_owned=true;
   else owned_fallback=FurnitureTarget{n,*d,*r,view.root,view.data.catalog.key,view.data.catalog.revision,house_type};
  }
  Housing::PlacementPolicy::Context context{n.native_id.value,d->type,r->id,{6,0x1fff,true},true};
  // Geometry-only candidate selection: these synthetic bits confer no rights.
  // Fresh typed ACL and backend Reserve/Store/Move/Return authorize operations.
  // Stored house operations resolve their requested current plot here, then
  // prove the actual Scene binding and proximity to the building below. Testing
  // a full house placed at the player's feet would reject building-edge clicks.
  // Prefer a unique geometric candidate. A unique owned eligible house plot
  // also permits preview from beside the plot; Commit checks the submitted
  // object pose, and backend Reserve/Store freshly authorize the chosen plot.
  context.geometry_only=true; // Candidate selection, no mutation.
  if(!house_type||!requested){bool fits=false;if(auto npc=FurnitureNpc(item)){for(auto bits:npc->allowed_heights.height_bits){auto frame=Housing::NpcNativeFrame::FromSubmitted(npc->allowed_heights,here.position[2],Housing::NpcNativeHeight::Float(bits));if(frame&&Housing::PlacementPolicy::Check(context,FurnitureMetadata(item),Housing::NpcRuntime::Geometry(*npc,frame,here),*r,here,Housing::PlacementPolicy::Operation::Add))fits=true;}}else fits=bool(Housing::PlacementPolicy::Check(context,FurnitureMetadata(item,house_type),FurnitureGeometry(item),*r,here,Housing::PlacementPolicy::Operation::Add));if(!fits)continue;}
  if(found)return {};found=FurnitureTarget{n,*d,*r,view.root,view.data.catalog.key,view.data.catalog.revision,house_type};
 }
 return found?found:(!ambiguous_owned?owned_fallback:std::nullopt);
}
void FurnitureResult(Client* c,uint32_t result){Bytes bytes;Housing::NeighborhoodWire::Detail::Write32(bytes,result);SendEstatePayload(c,OP_RealEstateOperationResult,bytes);}
Housing::PlacementWire::Actor FurnitureActor(const EQ::ItemInstance& item,const Housing::LiveItems::Placed& placed,const Housing::HouseBinding::Binding* house=nullptr,bool preview=false){
 const auto* d=item.GetUnscaledItem();if(!d)throw std::invalid_argument("Housing actor template unavailable");
 auto npc=house?std::optional<Housing::NpcRuntime::Profile>{}:FurnitureNpc(item);
 const auto* model=house?nullptr:FurnitureModel(item);const auto* crate=house?nullptr:Housing::CratePlacement::Model(item);const auto* house_model=house?Housing::HousePlacement::Model(item):nullptr;
 if((!house&&!model&&!crate&&!npc)||(house&&(!house_model||!house->interior||item.GetHousingHouseID()!=house->house_id||item.GetHousingEstateID()!=house->interior->native_estate_id)))throw std::invalid_argument("Unsupported housing actor model");
 Housing::PlacementWire::Actor actor;actor.model=npc?npc->model:house_model?house_model->model:crate?crate->model:model->model;actor.is_npc=npc?1:0;actor.npc_height=npc?npc->actor_height:0;actor.guid=FurnitureGuid(item);actor.record_num=0; // Verified ordinary preview fixture; not an inferred item-ID field.
 actor.estate_id=placed.reservation.native_estate_id;actor.item_id=placed.reservation.native_item_id;const auto t=npc&&!preview?Housing::NpcRuntime::ActorPose(*npc,placed.transform,placed.npc_frame):placed.transform;
 actor.scale_heading_pitch_roll_y_x_z={t.scale,t.heading,t.pitch,t.roll,t.position[0],t.position[1],t.position[2]};
 actor.ignore_collisions=d->HousingIgnoreCollisions!=0;actor.placement_type=d->HousingPlacementType;
 actor.disable_rotation=d->HousingDisableRotation;actor.disable_free=d->HousingDisableFreePlacement;
 actor.min_max_default_scale_heading_pitch_roll={d->HousingMinimumScale,d->HousingMaximumScale,std::clamp(1.f,d->HousingMinimumScale,d->HousingMaximumScale),0,0,0};return actor;
}
Bytes FurnitureEntry(Client* c,const Housing::LiveItems::Stored& row,const EQ::ItemInstance& item,const std::string* creator=nullptr,const Housing::HouseBinding::Binding* house=nullptr,bool self_owner_display=false){
 if(!row.placement||(!creator&&row.row.values.character_id!=c->CharacterID())||(house?!Housing::HousePresentation::Model(row,*house,item):(!KnownFurniture(item)&&!Housing::CratePlacement::Model(item)))||!Housing::InventoryBridge::MatchesProjection(row.row,const_cast<EQ::ItemInstance&>(item)))throw std::invalid_argument("Unsupported current furniture row");
 if(house&&(!house->interior||item.GetHousingHouseID()!=house->house_id||item.GetHousingEstateID()!=house->interior->native_estate_id))throw std::invalid_argument("House presentation identity mismatch");
 const auto& p=*row.placement;auto embedded=RoF2::SerializeHousingItem(item,EQ::invslot::slotCursor);if(!embedded)throw std::invalid_argument("Furniture ItemPtr unavailable");
 Bytes bytes;auto w=[&](uint32_t v){Housing::NeighborhoodWire::Detail::Write32(bytes,v);};w(p.reservation.native_estate_id);w(p.reservation.native_item_id);bytes.push_back(row.stowed?0:1);w(Housing::UpkeepCycle::Expired(database,row.custody_id,p.reservation.native_item_id));
 const auto& t=p.transform;for(float v:{t.heading,t.pitch,t.roll,t.scale,t.position[1],t.position[0],t.position[2]})Housing::PlacementWire::Detail::Float(bytes,v); // ItemPosition is X,Y,Z; worldactor is Y,X,Z.
 // The native personal-trophy enumerator excludes nonempty OwnerName as
 // foreign-owned (5B4DD4). Advertise self ownership only on this viewer's personal property with an
 // empty display name; preserve every foreign creator and the owner handle.
 // This compatibility convention never changes canonical custody ownership.
 const std::string owner=creator?*creator:std::string(c->GetName());if(owner.empty()||owner.size()>63||owner.find('\0')!=std::string::npos)throw std::invalid_argument("Invalid furniture creator");
 const std::string display_owner=self_owner_display&&row.row.values.character_id==c->CharacterID()?std::string{}:owner;
 w(display_owner.size());bytes.insert(bytes.end(),display_owner.begin(),display_owner.end());
 w(owner.size());bytes.insert(bytes.end(),owner.begin(),owner.end());
 bytes.insert(bytes.end(),embedded->begin(),embedded->end());return bytes;
}
std::optional<Housing::Pets::Binding> FurniturePet(const Housing::LiveItems::Stored& row,const EQ::ItemInstance& item,const Housing::Scene::Snapshot& scene){
 auto profile=FurnitureNpc(item);if(!profile||!Housing::Pets::Supported(*profile)||row.stowed||!row.placement||row.placement->transform.pitch!=0||row.placement->transform.roll!=0||row.placement->transform.scale!=1)return {};
 Housing::Pets::Binding b;b.custody_id=row.custody_id;b.catalog_revision=scene.current_catalog_revision;b.pose_revision=row.placement->pose_revision;b.estate_id=row.estate_id;b.native_estate_id=row.placement->reservation.native_estate_id;b.native_item_id=row.placement->reservation.native_item_id;b.owner=row.row.values.character_id;b.profile=*profile;b.pose=row.placement->transform;b.frame=row.placement->npc_frame;b.name=item.GetUnscaledItem()->Name;b.regions.push_back(scene.region);b.regions.insert(b.regions.end(),scene.additional_regions.begin(),scene.additional_regions.end());return b;
}
struct FurnitureScene {uint32_t estate=0;Bytes list;std::map<uint32_t,Session::World> worlds;std::map<uint32_t,Bytes> entries;std::vector<Bytes> list_deltas;};
void FurnitureFinalizeList(FurnitureScene& scene){
 std::vector<Bytes> entries;entries.reserve(scene.entries.size());for(const auto& e:scene.entries)entries.push_back(e.second);
 auto packets=Housing::ItemListWire::Encode(scene.estate,entries);scene.list=std::move(packets.list);scene.list_deltas=std::move(packets.deltas);
}
namespace {
struct VaultRealm {Housing::LiveItems::VaultContext context;std::vector<Housing::VaultStorage::Item> items;};
Housing::Route VaultSource(){if(!zone)return {};return {zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};}
std::optional<VaultRealm> VaultRoom(const Housing::Scene::InteriorSnapshot& room){
 if(!room.house||!room.house->interior||!room.room_profile)return {};VaultRealm out;auto& c=out.context;c.estate_id=room.context.estate_id;c.native_estate_id=room.house->interior->native_estate_id;c.catalog_revision=room.context.catalog_revision;c.house_id=room.house->house_id;c.house_revision=room.house->revision;c.profile_revision=room.profile_revision;c.source=VaultSource();out.items=room.vault_items;return out;
}
template<class Entries> std::optional<VaultRealm> VaultCrateRealm(VaultRealm realm,const Entries& entries,const std::vector<Housing::VaultStorage::Item>& members,uint64_t selected=0){
 const Housing::CrateBinding::Allocation* container=nullptr;
 for(const auto& item:entries)if(item.crate&&!item.stored.stowed&&!item.crate->packed&&(!selected||item.crate->binding.crate_id==selected)){if(container)return {};container=item.crate.get();}
 if(!container||container->binding.parent_native_estate_id!=realm.context.native_estate_id)return {};
 realm.context.crate_id=container->binding.crate_id;realm.context.crate_revision=container->binding.revision;realm.items.clear();for(const auto& v:members)if(v.contained_in_crate==realm.context.crate_id)realm.items.push_back(v);return realm;
}
std::optional<VaultRealm> VaultRealmFor(Client* c,Session& session,Housing::InventoryBridge::Bridge& bridge,const HousingView& view,uint32_t action,uint32_t requested=0){
 std::optional<Housing::Scene::Snapshot> yard;
 if(requested){auto shown=session.attached_crates.find(requested);if(shown!=session.attached_crates.end()){
  auto base=VaultRealmFor(c,session,bridge,view,0,shown->second.parent_native_estate_id);if(!base)return {};
  if(base->context.house_id){auto room=view.interior?bridge.ReadInterior():bridge.ReadAttachedInterior(base->context.estate_id,base->context.house_id);if(!room)return {};return VaultCrateRealm(std::move(*base),room.value.contents,room.value.crate_vault_items,shown->second.crate_id);}
  auto read=view.interior?bridge.ReadParentScene():bridge.ReadScene(base->context.estate_id);if(!read)return {};return VaultCrateRealm(std::move(*base),read.value.items,read.value.crate_vault_items,shown->second.crate_id);
 }}
 if(view.interior){auto room=bridge.ReadInterior();if(!room)return {};if((!requested&&(action==2||action==6))||(requested&&room.value.house&&room.value.house->interior&&room.value.house->interior->native_estate_id==requested)){auto realm=VaultRoom(room.value);if(realm&&action==6)return VaultCrateRealm(std::move(*realm),room.value.contents,room.value.crate_vault_items);return realm;}auto parent=bridge.ReadParentScene();if(parent)yard=std::move(parent.value);}
 else for(const auto&node:view.data.catalog.nodes){
  if(!node.ownership_estate_id||!node.native_id.value||node.neighborhood_id!=view.neighborhood||node.parent_key.empty()||view.ids.at(node.parent_key)!=view.root)continue;
  auto definition=std::find_if(view.data.catalog.definitions.begin(),view.data.catalog.definitions.end(),[&](const auto&d){return d.id==node.definition_id;});if(definition==view.data.catalog.definitions.end()||(definition->type!=4&&definition->type!=8))continue;
  auto region=std::find_if(view.data.catalog.regions.begin(),view.data.catalog.regions.end(),[&](const auto&r){return r.id==definition->area_id;});if(region==view.data.catalog.regions.end())continue;
  bool match=requested==node.native_id.value;
  if(requested&&!match){auto child=std::find_if(view.data.catalog.nodes.begin(),view.data.catalog.nodes.end(),[&](const auto&n){return n.native_id.value==requested&&n.parent_key==node.key;});match=child!=view.data.catalog.nodes.end();}
  if(!requested)match=FurniturePlayerInRegion(c,*region);if(!match)continue;auto read=bridge.ReadScene(node.ownership_estate_id);if(!read)continue;if(yard)return {};yard=std::move(read.value);
 }
 if(!yard)return {};
 if((!requested&&(action==3||action==6))||requested==yard->native_plot_id){VaultRealm out;out.context.estate_id=yard->estate_id;out.context.native_estate_id=yard->native_plot_id;out.context.catalog_revision=yard->current_catalog_revision;out.context.source=VaultSource();out.items=yard->vault_items;if(action==6)return VaultCrateRealm(std::move(out),yard->items,yard->crate_vault_items);return out;}
 const Housing::Scene::Item* house=nullptr;for(const auto&item:yard->items)if(item.house&&item.house->interior&&!item.stored.stowed&&(!requested||item.house->interior->native_estate_id==requested)){if(house)return {};house=&item;}
 if(!house)return {};auto room=bridge.ReadAttachedInterior(yard->estate_id,house->house->house_id);if(!room)return {};session.attached_rooms[room.value.house->interior->native_estate_id]=room.value;return VaultRoom(room.value);
}
Bytes FurnitureVaultEntry(const Housing::VaultStorage::Item& row,const EQ::ItemInstance& item){
 const auto* data=item.GetUnscaledItem();if(!data||data->Placeable||!Housing::InventoryProjection::VaultCandidate(*data)||row.stored.placement||!Housing::InventoryBridge::MatchesProjection(row.stored.row,const_cast<EQ::ItemInstance&>(item)))throw std::invalid_argument("Invalid vault list-only item");
 auto embedded=RoF2::SerializeHousingItemListEntry(item,EQ::invslot::slotCursor);if(!embedded)throw std::invalid_argument("Invalid vault ItemPtr");Bytes bytes;auto word=[&](uint32_t v){Housing::NeighborhoodWire::Detail::Write32(bytes,v);};word(row.native_estate_id);word(row.native_item_id);bytes.push_back(0);word(0); // Unplaced state; NOT a world actor.
 for(float v:{0.f,0.f,0.f,1.f,0.f,0.f,0.f})Housing::PlacementWire::Detail::Float(bytes,v);
 for(unsigned i=0;i<2;++i){word(row.creator_name.size());bytes.insert(bytes.end(),row.creator_name.begin(),row.creator_name.end());}bytes.insert(bytes.end(),embedded->begin(),embedded->end());return bytes;
}
bool FurnitureVaultEntries(Session& session,FurnitureScene& scene,const std::vector<Housing::VaultStorage::Item>& entries){
 for(const auto&row:entries){if(row.native_estate_id!=scene.estate)return false;auto cached=session.presentations.find(row.native_item_id);if(cached==session.presentations.end()){auto rebuilt=Housing::InventoryBridge::Reconstruct(database,row.stored.row);if(!rebuilt)return false;cached=session.presentations.emplace(row.native_item_id,std::move(rebuilt.value.item)).first;}if(!cached->second||!Housing::SceneProjection::SynchronizeCharges(row.stored.row,*cached->second))return false;auto bytes=FurnitureVaultEntry(row,*cached->second);if(bytes.size()>65523||!scene.entries.emplace(row.native_item_id,std::move(bytes)).second)return false;}
 return true;
}
uint32_t FurniturePlacementResult(Housing::LiveItems::Error error){return error==Housing::LiveItems::Error::NpcFull?0x26:1;}
uint32_t VaultResult(Housing::LiveItems::Error error){return error==Housing::LiveItems::Error::VaultFull?12:error==Housing::LiveItems::Error::RealEstateFull?10:error==Housing::LiveItems::Error::Unauthorized?23:1;}
}
std::optional<Housing::Scene::Snapshot> FurnitureRoomCrateSnapshot(const Housing::Scene::InteriorSnapshot& room);
std::optional<Housing::Scene::Snapshot> FurnitureSnapshot(Client* c,Housing::InventoryBridge::Bridge& bridge,const FurnitureTarget& target){
 if(target.room){
  auto current=target.room->exterior_source?bridge.ReadAttachedInterior(target.room->context.estate_id,target.room->house->house_id):bridge.ReadInterior();if(!current||!current.value.house||!current.value.room_profile||current.value.house->house_id!=target.room->house->house_id||current.value.context.catalog_revision!=target.revision)return {};
  return FurnitureRoomCrateSnapshot(current.value);
 }
 auto snapshot=bridge.ReadScene(target.node.ownership_estate_id);
 if(!snapshot){LogInfo("Native viewer furniture read rejected actor [{}] estate [{}] code [{}]",c->CharacterID(),target.node.ownership_estate_id,int(snapshot.error));return {};}
 if(snapshot.value.estate_id!=target.node.ownership_estate_id||snapshot.value.native_plot_id!=target.node.native_id.value||snapshot.value.current_catalog_revision!=target.revision){LogInfo("Native viewer furniture snapshot rejected actor [{}] estate [{}] revision [{}] expected [{}]",c->CharacterID(),target.node.ownership_estate_id,snapshot.value.current_catalog_revision,target.revision);return {};}
 return std::move(snapshot.value);
}
std::optional<FurnitureScene> FurnitureReadScene(Client* c,Session& session,Housing::InventoryBridge::Bridge& bridge,const FurnitureTarget& target,const Housing::Scene::Snapshot* supplied=nullptr,bool render_world=true){
 auto snapshot=supplied?std::optional<Housing::Scene::Snapshot>(*supplied):FurnitureSnapshot(c,bridge,target);if(!snapshot)return {};
 std::vector<Housing::Pets::Binding> pets;
 FurnitureScene scene;scene.estate=target.node.native_id.value;Housing::NeighborhoodWire::Detail::Write32(scene.list,scene.estate);Housing::NeighborhoodWire::Detail::Write32(scene.list,snapshot->items.size());
 for(const auto& entry:snapshot->items){const auto& row=entry.stored;
  if(!FurnitureCurrentCatalog(row,target,snapshot->current_catalog_revision))return {};
  const auto id=row.placement->reservation.native_item_id;auto cache=session.presentations.find(id);
  if(cache==session.presentations.end()){auto rebuilt=Housing::InventoryBridge::Reconstruct(database,row.row);if(!rebuilt)return {};cache=session.presentations.emplace(id,std::move(rebuilt.value.item)).first;}
  if(!cache->second||!Housing::SceneProjection::SynchronizeCharges(row.row,*cache->second)||!Housing::HousePresentation::Project(entry,*cache->second))return {};auto bytes=FurnitureEntry(c,row,*cache->second,&entry.creator_name,entry.house.get(),snapshot->definition.type==4&&snapshot->access.implicit_owner);if(bytes.size()>65523)return {};
  if(!scene.entries.emplace(id,std::move(bytes)).second)return {};
  auto pet=FurniturePet(row,*cache->second,*snapshot);if(render_world&&pet)pets.push_back(*pet);
  if(render_world&&!row.stowed&&!pet)scene.worlds.emplace(id,Session::World{FurnitureGuid(*cache->second),Housing::PlacementWire::EncodeActor(FurnitureActor(*cache->second,*row.placement,entry.house.get()))});
 }
 if(!FurnitureVaultEntries(session,scene,snapshot->vault_items))return {};
 if(render_world&&!Housing::Pets::Reconcile(scene.estate,pets))return {};
 FurnitureFinalizeList(scene);return scene;
}
// Presentation/selection adapter from a freshly authorized typed room scene.
// Every mutation resolves the actual room again in the common service.
std::optional<Housing::Scene::Snapshot> FurnitureRoomCrateSnapshot(const Housing::Scene::InteriorSnapshot& room){
 if(!room.house||!room.house->interior||!room.room_profile||!room.profile_revision||room.room_profile->house_id!=room.house->house_id||room.room_profile->native_estate_id!=room.house->interior->native_estate_id)return {};
 Housing::Scene::Snapshot out;out.estate_id=room.context.estate_id;out.native_plot_id=room.house->interior->native_estate_id;out.current_catalog_revision=room.context.catalog_revision;out.acl_revision=room.access.acl_revision;out.access=room.access.effective;out.route=room.exterior_source.value_or(room.context.source);out.definition=room.room_profile->definition;out.region=room.room_profile->region;out.additional_regions=room.room_profile->additional_regions;out.vault_items=room.vault_items;out.crate_vault_items=room.crate_vault_items;
 for(bool contained:{false,true})for(const auto& entry:contained?room.crate_contents:room.contents){
  if(!entry.room||entry.room->house_id!=room.house->house_id||!entry.stored.placement||entry.stored.placement->reservation.native_estate_id!=out.native_plot_id||bool(entry.contained_in_crate)!=contained)return {};
  Housing::Scene::Item item;item.stored=entry.stored;item.creator_name=entry.creator_name;item.crate=entry.crate;item.contained_in_crate=entry.contained_in_crate;(contained?out.crate_contents:out.items).push_back(std::move(item));
 }
 return out;
}
std::optional<FurnitureScene> FurnitureReadCrateScene(Client* c,Session& session,const Housing::Scene::Snapshot& yard,const Housing::Scene::Item& container){
 if(!container.crate||container.stored.stowed||container.crate->binding.parent_native_estate_id!=yard.native_plot_id)return {};
 const auto& binding=container.crate->binding;FurnitureScene scene;scene.estate=binding.native_estate_id;
 for(const auto& entry:yard.crate_contents){if(entry.contained_in_crate!=binding.crate_id)continue;
  const auto& stored=entry.stored;if(entry.crate||!stored.placement||!stored.stowed||stored.placement->reservation.native_estate_id!=yard.native_plot_id)return {};
  const auto id=stored.placement->reservation.native_item_id;auto cached=session.presentations.find(id);
  if(cached==session.presentations.end()){auto rebuilt=Housing::InventoryBridge::Reconstruct(database,stored.row);if(!rebuilt)return {};cached=session.presentations.emplace(id,std::move(rebuilt.value.item)).first;}
  if(!cached->second||!Housing::SceneProjection::SynchronizeCharges(stored.row,*cached->second)||!Housing::HousePresentation::Project(entry,*cached->second))return {};
  auto display=stored;display.placement->reservation.native_estate_id=scene.estate;
  auto bytes=FurnitureEntry(c,display,*cached->second,&entry.creator_name,entry.house.get());if(bytes.size()>65523||!scene.entries.emplace(id,std::move(bytes)).second)return {};
 }
 std::vector<Housing::VaultStorage::Item> vaults;for(const auto& original:yard.crate_vault_items)if(original.contained_in_crate==binding.crate_id){auto display=original;display.native_estate_id=scene.estate;vaults.push_back(std::move(display));}if(!FurnitureVaultEntries(session,scene,vaults))return {};
 FurnitureFinalizeList(scene);
 session.attached_crates[scene.estate]=binding;return scene;
}
std::optional<FurnitureScene> FurnitureReadRoomScene(Client* c,Session& session,const Housing::Scene::InteriorSnapshot& snapshot,bool render_world=true){
 if(!snapshot.house||!snapshot.house->interior||!snapshot.room_profile||!snapshot.profile_revision||snapshot.room_profile->house_id!=snapshot.house->house_id||snapshot.room_profile->native_estate_id!=snapshot.house->interior->native_estate_id)return {};
 // Real service entities follow committed custody independently of whether
 // an unrelated furniture/vault presentation can be serialized for this view.
 if(render_world&&!Housing::ServiceAuthority::Reconcile(*c,snapshot))return {};
 std::vector<Housing::Pets::Binding> pets;auto pet_scene=FurnitureRoomCrateSnapshot(snapshot);if(!pet_scene)return {};
 FurnitureScene scene;scene.estate=snapshot.house->interior->native_estate_id;std::map<uint32_t,Housing::Scene::InteriorContent> next_cache;
 for(const auto& entry:snapshot.contents){const auto& row=entry.stored;if(!row.placement)continue;
  if(!entry.room||row.placement->reservation.native_estate_id!=scene.estate||entry.room->house_id!=snapshot.house->house_id)return {};
  const auto id=row.placement->reservation.native_item_id;auto cached=session.presentations.find(id);
  if(cached==session.presentations.end()){auto rebuilt=Housing::InventoryBridge::Reconstruct(database,row.row);if(!rebuilt)return {};cached=session.presentations.emplace(id,std::move(rebuilt.value.item)).first;}
  if(!cached->second||!Housing::SceneProjection::SynchronizeCharges(row.row,*cached->second))return {};
  if(entry.crate&&!Housing::CratePresentation::Project(entry,snapshot.house->house_id,scene.estate,*cached->second))return {};
  auto bytes=FurnitureEntry(c,row,*cached->second,&entry.creator_name,nullptr,snapshot.access.owner_kind==Housing::OwnerKind::Character&&snapshot.access.owner_id==c->CharacterID());if(bytes.size()>65523||!scene.entries.emplace(id,bytes).second)return {};
  auto pet=FurniturePet(row,*cached->second,*pet_scene);if(render_world&&pet)pets.push_back(*pet);
  if(render_world&&!row.stowed&&!pet&&!Housing::ServiceItemBindings::Lookup(row.row.values.item_id))scene.worlds.emplace(id,Session::World{FurnitureGuid(*cached->second),Housing::PlacementWire::EncodeActor(FurnitureActor(*cached->second,*row.placement))});next_cache.emplace(id,entry);
 }
 if(!FurnitureVaultEntries(session,scene,snapshot.vault_items))return {};
 if(render_world&&!Housing::Pets::Reconcile(scene.estate,pets))return {};
 FurnitureFinalizeList(scene);
 if(render_world){session.room_items=std::move(next_cache);session.room_context=Housing::LiveItems::InteriorMoveContext{snapshot.house->house_id,snapshot.house->revision,snapshot.profile_revision,0,snapshot.context.source};}else session.attached_rooms[scene.estate]=snapshot;return scene;
}
std::optional<FurnitureScene> FurnitureCurrentScene(Client* c,Session& session,Housing::InventoryBridge::Bridge& bridge,const FurnitureTarget& target){
 if(target.room){auto current=target.room->exterior_source?bridge.ReadAttachedInterior(target.room->context.estate_id,target.room->house->house_id):bridge.ReadInterior();if(!current||!current.value.house||current.value.house->house_id!=target.room->house->house_id)return {};return FurnitureReadRoomScene(c,session,current.value,!target.room->exterior_source);}
 return FurnitureReadScene(c,session,bridge,target);
}
bool FurnitureSameTransform(const Housing::PlacementPolicy::Transform& a,const Housing::PlacementPolicy::Transform& b){
 for(const auto& values:{std::pair{a.heading,b.heading},std::pair{a.pitch,b.pitch},std::pair{a.roll,b.roll},std::pair{a.scale,b.scale},std::pair{a.position[0],b.position[0]},std::pair{a.position[1],b.position[1]},std::pair{a.position[2],b.position[2]}}){
  uint32_t left,right;std::memcpy(&left,&values.first,4);std::memcpy(&right,&values.second,4);if(left!=right)return false;
 }
 return true;
}
bool FurniturePresentationMatches(const Housing::Scene::Item& entry,const FurnitureTarget& target,EQ::ItemInstance& item){
 if(target.room&&entry.crate){const auto& room=*target.room;auto found=std::find_if(room.contents.begin(),room.contents.end(),[&](const auto& value){return value.stored.custody_id==entry.stored.custody_id;});return found!=room.contents.end()&&Housing::CratePresentation::SameBinding(found->crate,entry.crate)&&Housing::CratePresentation::Project(*found,room.house->house_id,room.house->interior->native_estate_id,item);}
 return Housing::HousePresentation::Matches(entry,item);
}
Bytes FurnitureRemoval(uint32_t estate,uint32_t item,const std::string& guid){
 if(guid.size()!=16)throw std::invalid_argument("Invalid furniture removal GUID");
 // Complete native4d0998 case/556d80 forwarding consumes at most byte39.
 // 16..31 are unused. This bounded shape still awaits gated native acceptance.
 Bytes bytes(40,0);std::copy(guid.begin(),guid.end(),bytes.begin());
 for(size_t n=0;n<4;++n){bytes[32+n]=uint8_t(estate>>(8*n));bytes[36+n]=uint8_t(item>>(8*n));}return bytes;
}
void FurnitureSendScene(Client* c,Session& session,const FurnitureScene& scene,std::optional<uint32_t> changed={},bool completion=false){
 SendEstatePayload(c,OP_RealEstateItemList,scene.list);
 for(const auto& delta:scene.list_deltas)SendEstatePayload(c,OP_RealEstateItemDelta,delta);
 if(changed){auto e=scene.entries.find(*changed);if(e==scene.entries.end())throw std::invalid_argument("Missing durable placed item");Bytes delta;auto native=Housing::NeighborhoodWire::Detail::Read32(e->second.data());Housing::NeighborhoodWire::Detail::Write32(delta,native);Housing::NeighborhoodWire::Detail::Write32(delta,0);Housing::NeighborhoodWire::Detail::Write32(delta,1);delta.insert(delta.end(),e->second.begin(),e->second.end());SendEstatePayload(c,OP_RealEstateItemDelta,delta);}
 // Cache first; result0 closes the preview before the same-GUID normal worldactor.
 // Exact live ordering requires the gated native session acceptance test.
 if(completion)FurnitureResult(c,0);
 auto& sent=session.sent_worlds[scene.estate];
 for(auto prior=sent.begin();prior!=sent.end();){auto now=scene.worlds.find(prior->first);
  if(now==scene.worlds.end()||now->second.packet!=prior->second.packet){SendEstatePayload(c,OP_RealEstateWorldRemove,FurnitureRemoval(scene.estate,prior->first,prior->second.guid));if(now==scene.worlds.end()&&!scene.entries.count(prior->first)&&!session.contained_items.count(prior->first))session.presentations.erase(prior->first);prior=sent.erase(prior);}else ++prior;}
 for(const auto& world:scene.worlds)if(!sent.count(world.first)){SendEstatePayload(c,OP_RealEstateWorldItem,world.second.packet);sent.emplace(world.first,world.second);}
 Housing::Pets::Publish(*c,scene.estate,[&](uint32_t id)->std::string{auto item=session.presentations.find(id);return item!=session.presentations.end()&&item->second?FurnitureGuid(*item->second):std::string{};});
 Housing::ServiceAuthority::PublishBindings(*c,scene.estate,[&](uint32_t id)->std::string{auto item=session.presentations.find(id);return item!=session.presentations.end()&&item->second?FurnitureGuid(*item->second):std::string{};});
}
void FurniturePurgeScene(Client* c,Session& session,uint32_t estate){
 FurnitureScene empty;empty.estate=estate;Housing::NeighborhoodWire::Detail::Write32(empty.list,estate);Housing::NeighborhoodWire::Detail::Write32(empty.list,0);
 FurnitureSendScene(c,session,empty);session.sent_worlds.erase(estate);
}
void FurnitureBroadcastScenes(bool containers_changed=false) noexcept{
 // Each viewer authenticates its own bridge read and uses its own runtime GUIDs.
 // Publish new/retired container relationships without clearing definitions or
 // changing the occupied estate. Child snapshots precede their parent links.
 try{for(const auto& entry:entity_list.GetClientList())if(entry.second&&HousingZone(entry.second)&&entry.second->Connected()&&(containers_changed||NativeFurniture(entry.second))){
  auto* client=entry.second;
  // Financial snapshots are independent of the native furniture-placement rule.
  // The original63FD receiver refreshes escrow/upkeep for the current yard.
  if(containers_changed){
   auto view=LoadHousingView(client);
   if(view){
    std::vector<std::pair<EmuOpcode,Bytes>> packets;
    std::set<uint32_t> sent;
    while(sent.size()<view->states.size()){
     bool progressed=false;
     for(const auto& [id,state]:view->states){
      if(sent.count(id)||std::any_of(state.children.begin(),state.children.end(),[&](auto child){return view->states.count(child)&&!sent.count(child);}))continue;
      packets.emplace_back(OP_RealEstateSnapshot,Housing::EstateWire::EncodeEstateSnapshot(state));sent.insert(id);progressed=true;
     }
     if(!progressed)throw std::invalid_argument("Cyclic native estate refresh");
    }
    for(const auto& [id,acl]:view->permissions)packets.emplace_back(OP_RealEstateAccess,Housing::PermissionWire::EncodeSnapshot(acl));
    packets.emplace_back(OP_RealEstateOwnedList,Housing::PermissionWire::EncodeOwned(view->owned));
    for(const auto& [opcode,bytes]:packets){SendEstatePayload(client,opcode,bytes);if(opcode==OP_RealEstateAccess){const auto id=Housing::NeighborhoodWire::Detail::Read32(bytes.data());client->RememberHousingAclRevision(id,view->acl_revisions.at(id));}}
   }
  }
  if(NativeFurniture(client))client->SendHousingPlacedScene();
 }}catch(const std::exception&){LogError("Native furniture viewer refresh unavailable");}
}
void FurniturePreview(Client* c,const FurniturePending& pending,const FurnitureTarget& target,const Housing::PlacementWire::Begin& request){
 const bool house=bool(pending.house_identity);
 if(house&&(!pending.reservation.content_binding||pending.reservation.content_binding->version!=Housing::ContentBinding::HouseVersion))throw std::invalid_argument("House preview needs reviewed reservation");
 const bool crate=Housing::CratePlacement::Model(*pending.presentation);
 if(crate){
  if(target.room){const auto& current=*target.room;if(pending.reservation.content_binding||!pending.room||pending.room->house_id!=current.house->house_id||pending.room->house_revision!=current.house->revision||pending.room->profile_revision!=current.profile_revision||pending.room->placement.native_estate_id!=pending.reservation.native_estate_id||pending.room->placement.native_item_id!=pending.reservation.native_item_id||pending.room->placement.estate_id!=pending.reservation.estate_id||pending.room->placement.catalog_key!=pending.reservation.catalog_key||pending.room->placement.catalog_revision!=pending.reservation.catalog_revision)throw std::invalid_argument("Crate preview needs reviewed room reservation");}
  else if(pending.room||!pending.reservation.content_binding||pending.reservation.content_binding->version!=Housing::ContentBinding::CrateVersion)throw std::invalid_argument("Crate preview needs reviewed exterior reservation");
 }
 const float scale=house?Housing::HousePlacement::DefaultScale(*pending.presentation):crate?std::clamp(1.f,pending.presentation->GetUnscaledItem()->HousingMinimumScale,pending.presentation->GetUnscaledItem()->HousingMaximumScale):FurnitureNpc(*pending.presentation)?std::clamp(1.f,pending.presentation->GetUnscaledItem()->HousingMinimumScale,pending.presentation->GetUnscaledItem()->HousingMaximumScale):1.f;
 Housing::LiveItems::Placed placed{pending.reservation,{c->GetHeading(),0,0,scale,{c->GetY(),c->GetX(),c->GetZ()}},1};
 Housing::PlacementWire::Preview preview;
 if(!Housing::PlacementWire::InventorySlotMatches(pending.source,pending.row.values.slot_id))throw std::invalid_argument("Placement source slot changed");
 preview.source_slots={int16_t(pending.row.values.slot_id),-1,-1};
 if(house){auto actor=Housing::HousePlacement::Preview(*pending.presentation,pending.reservation,pending.presentation->GetUnscaledItem()->HousingDefinitionID,target.house_definition_type,placed.transform,pending.house_before.get());if(!actor)throw std::invalid_argument("House preview identity rejected");preview.actor=*actor;}
 else preview.actor=FurnitureActor(*pending.presentation,placed,nullptr,true);preview.flags={0,0,1};preview.words={request.trailing[0],request.trailing[1],0};
 preview.region_results=FurnitureRegions(target,pending.presentation.get(),pending.reservation.native_item_id,pending.room?std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>>{pending.room->content_digest}:std::nullopt);if(auto projection=FurnitureNpcNativeRegions(c,target,*pending.presentation))preview.region_results=std::move(*projection);preview.fallback={target.root,1};SendEstatePayload(c,OP_RealEstatePlacementBegin,Housing::PlacementWire::EncodePreview(preview));
}
bool HouseGeometryPreviewScope(Client* c){
 const auto* config=EQEmuConfig::get();
 if(!config||config->DatabaseDB!="peq_housing_test"||c->Admin()<AccountStatus::GMLeadAdmin||!c->HasInventorySessionReservation())return false;
 std::ifstream marker(".house-geometry-preview-only");std::string value;std::getline(marker,value);
 return value=="HOUSE_GEOMETRY_PREVIEW_ONLY";
}
bool HouseGeometryPreview(Client* c,const EQ::ItemInstance& source,const Housing::PlacementWire::Begin& request){
 // Diagnostic only: original item/model pairing, live cursor, no positive house
 // identity, fresh AddHouse/AddCrate on one owned plot. No adoption/custody/reservation,
 // inventory writes, world actor broadcast, or permission to commit placement.
 const auto* item=source.GetUnscaledItem();if(!item||source.GetHousingHouseID()||source.GetHousingEstateID()!=UINT32_MAX)return false;
 const char* expected=nullptr;
 if(item->ID==62586)expected="IT20028";else {const auto* known=Housing::HouseTemplates::Lookup(item->ID);if(!known)return false;expected=known->model;}
 const bool crate=item->ID==62586;
 if(crate&&item->HousingDefinitionID!=174)return false;
 if(std::string(item->IDFile)!=expected||item->ItemClass!=EQ::item::ItemClassCommon||item->BagSlots||source.IsStackable()||source.IsScaling()||item->EvolvingItem||!item->HousingDefinitionID||item->HousingPlacementType||!std::isfinite(item->HousingMinimumScale)||!std::isfinite(item->HousingMaximumScale)||!(item->HousingMinimumScale>0)||item->HousingMaximumScale<item->HousingMinimumScale)return false;
 auto row=FurnitureInventory(c);if(!row||!Housing::InventoryBridge::MatchesProjection(*row,const_cast<EQ::ItemInstance&>(source)))return false;
 auto view=LoadHousingView(c);if(!view)return false;
 const Housing::Catalog::Node* selected=nullptr;const Housing::RegionWire::Region* region=nullptr;
 const uint32_t plot_type=Housing::HouseTemplates::Type(item->ID)==3?4:8;
 for(const auto& node:view->data.catalog.nodes){
  if(node.neighborhood_id!=view->neighborhood||!node.ownership_estate_id||node.parent_key.empty()||view->ids.at(node.parent_key)!=view->root||std::find(view->owned.begin(),view->owned.end(),node.native_id.value)==view->owned.end())continue;
  auto definition=std::find_if(view->data.catalog.definitions.begin(),view->data.catalog.definitions.end(),[&](const auto& d){return d.id==node.definition_id;});
  if(definition==view->data.catalog.definitions.end()||definition->type!=plot_type||!(item->Placeable&Housing::PlacementPolicy::EstateFlag(plot_type)))continue;
  auto found=std::find_if(view->data.catalog.regions.begin(),view->data.catalog.regions.end(),[&](const auto& r){return r.id==definition->area_id;});if(found==view->data.catalog.regions.end())continue;
  if(selected)return false;selected=&node;region=&*found;
 }
 if(!selected||!region)return false;
 auto db=HousingConnection();if(!db||!QueryView(*db,"START TRANSACTION").Success())return false;
 auto access=Housing::LoadCurrentEstateAccessUnderTransaction(*db,selected->ownership_estate_id,selected->native_id.value,{c->CharacterID(),c->AccountID()});
 auto schema=QueryView(*db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_house_locations'");
 bool unbound=schema.Success()&&schema.RowCount()==1;
 if(unbound&&Strings::ToUnsignedInt(schema.begin()[0])){auto binding=QueryView(*db,"SELECT house_id FROM housing_house_locations WHERE character_id="+std::to_string(c->CharacterID())+" AND slot_id=33 FOR UPDATE");unbound=binding.Success()&&!binding.RowCount();}
 const bool allowed=bool(access)&&access.value.effective.Allows(crate?Housing::AccessPolicy::Capability::AddCrate:Housing::AccessPolicy::Capability::AddHouse)&&unbound;
 const bool rolled_back=QueryView(*db,"ROLLBACK").Success();if(!allowed||!rolled_back)return false;
 Housing::PlacementWire::Preview preview;auto& actor=preview.actor;
 actor.model=expected;actor.guid=FurnitureGuid(source);actor.record_num=0;
 actor.estate_id=selected->native_id.value;actor.item_id=UINT32_MAX-1; // ephemeral preview only; never persisted or sent as a world actor
 const float scale=std::clamp(1.f,item->HousingMinimumScale,item->HousingMaximumScale);
 actor.scale_heading_pitch_roll_y_x_z={scale,c->GetHeading(),0,0,c->GetY(),c->GetX(),c->GetZ()};
 actor.ignore_collisions=item->HousingIgnoreCollisions!=0;actor.placement_type=item->HousingPlacementType;
 actor.disable_rotation=item->HousingDisableRotation;actor.disable_free=item->HousingDisableFreePlacement;
 actor.min_max_default_scale_heading_pitch_roll={item->HousingMinimumScale,item->HousingMaximumScale,scale,0,0,0};
 preview.flags={0,0,1};preview.words={request.trailing[0],request.trailing[1],0};preview.region_results={{region->id,0}};preview.fallback={view->root,1};
 SendEstatePayload(c,OP_RealEstatePlacementBegin,Housing::PlacementWire::EncodePreview(preview));
 LogInfo("HOUSE_GEOMETRY_PREVIEW_ONLY item [{}] model [{}] plot [{}] serial [{}]; no placement authorized",item->ID,expected,selected->native_id.value,source.GetSerialNumber());return true;
}
void FurnitureMovePreview(Client* c,const Session::Move& move,const FurnitureTarget& target,const Housing::PlacementWire::Begin& request){
 Housing::PlacementWire::Preview preview;preview.actor=FurnitureActor(*move.presentation,*move.before.placement,move.house.get());
 preview.source_kind=0xff;preview.source_slots={-1,-1,-1};
 // Stored room items have no rendered actor. Initialize in front of the camera:
 // the Items Place button coordinates otherwise project through the UI edge.
 // Native ctor 0x5056b0 arg6 (third wire flag) selects screen-ray initialization.
 preview.flags=move.before.stowed?std::array<uint8_t,3>{0,0,uint8_t(move.room?0:1)}:std::array<uint8_t,3>{1,1,0};
 preview.words={request.trailing[0],request.trailing[1],0};preview.region_results=FurnitureRegions(target,move.presentation.get(),move.before.placement->reservation.native_item_id);if(auto projection=FurnitureNpcNativeRegions(c,target,*move.presentation))preview.region_results=std::move(*projection);preview.fallback={target.root,1};
 SendEstatePayload(c,OP_RealEstatePlacementBegin,Housing::PlacementWire::EncodePreview(preview));
}
std::unique_ptr<EQApplicationPacket> FurnitureCursorDeletion(uint32_t slot=EQ::invslot::slotCursor){
 auto packet=std::make_unique<EQApplicationPacket>(OP_MoveItem,sizeof(MoveItem_Struct));auto* d=reinterpret_cast<MoveItem_Struct*>(packet->pBuffer);
 d->from_slot=slot;d->to_slot=0xffffffff;d->number_in_stack=0xffffffff;return packet;
}
}

namespace {
bool VaultWholeQuantity(const Housing::PlacementWire::Source& source,const EQ::ItemInstance& item){return Housing::StackPartitionApplication::Requested(item,source.opaque)==0;}
bool VaultQuantity(const Housing::PlacementWire::Source& source,const EQ::ItemInstance& item){(void)Housing::StackPartitionApplication::Requested(item,source.opaque);return true;}
bool VaultRoomRetrySpace(Session& session){if(session.vault_actions.size()<64)return true;auto old=std::find_if(session.vault_actions.begin(),session.vault_actions.end(),[](const auto&v){return v.second.completed;});if(old==session.vault_actions.end())return false;session.vault_actions.erase(old);return true;}
bool FurnitureVaultOperation(Client* c,Session& session,Housing::InventoryBridge::Bridge& bridge,const EQApplicationPacket* app,const Housing::PlacementWire::Source& source,uint32_t action){
 const Bytes incoming(app->pBuffer,app->pBuffer+app->size);auto previous=session.vault_actions.find(incoming);
 if(source.location[0]==0){
  auto slot=Housing::PlacementWire::InventorySlot(source);auto* item=slot?c->GetInv().GetItem(int16_t(*slot)):nullptr;const auto* data=item?item->GetUnscaledItem():nullptr;
  if(previous==session.vault_actions.end()&&(!data||data->Placeable))return false;
  if(previous!=session.vault_actions.end()&&previous->second.completed&&item){session.vault_actions.erase(previous);previous=session.vault_actions.end();}
  if(action!=2&&action!=3&&action!=6){FurnitureResult(c,17);return true;}
  if(previous==session.vault_actions.end()){
   if(!slot||source.estate_id!=UINT32_MAX||source.item_id!=UINT32_MAX||!item||!data||!Housing::InventoryProjection::VaultCandidate(*data)||!SourceGuid(source,*item)||!VaultQuantity(source,*item)||(*slot==33&&c->GetInv().CursorSize()!=1)){LogInfo("Native vault source rejected actor [{}] slot [{}] item [{}] guid_match [{}] locations [{},{},{}]",c->CharacterID(),slot?*slot:UINT32_MAX,data?data->ID:0,item?SourceGuid(source,*item):false,source.location[0],source.location[1],source.location[2]);FurnitureResult(c,data&&data->NoTransfer?43:9);return true;}
   auto row=FurnitureInventory(c,*slot);if(!row||!Housing::InventoryBridge::MatchesProjection(*row,*item)||!VaultRoomRetrySpace(session)){LogInfo("Native vault inventory projection rejected actor [{}] slot [{}] row [{}]",c->CharacterID(),*slot,bool(row));FurnitureResult(c,9);return true;}
   auto view=LoadHousingView(c);auto target=view?VaultRealmFor(c,session,bridge,*view,action):std::nullopt;if(!target){FurnitureResult(c,23);return true;}
   Session::VaultPending pending;pending.key=FurnitureKey("vault-store:");pending.row=*row;pending.serial=item->GetSerialNumber();pending.context=target->context;previous=session.vault_actions.emplace(incoming,std::move(pending)).first;
  }
  auto& pending=previous->second;if(pending.completed){FurnitureResult(c,0);FurnitureBroadcastScenes();return true;}
  auto deletion=FurnitureCursorDeletion(pending.row.values.slot_id);auto result=bridge.Store(pending.key,pending.context.estate_id,pending.row,std::nullopt,std::nullopt,std::nullopt,false,std::nullopt,std::nullopt,std::nullopt,pending.context,source.opaque);
  if(!result){LogInfo("Native vault Store rejected actor [{}] item [{}] estate [{}] house [{}] code [{}]",c->CharacterID(),pending.row.values.item_id,pending.context.estate_id,pending.context.house_id,int(result.error));FurnitureResult(c,VaultResult(result.error));return true;}pending.completed=true;if(result.value.partition){if(auto* remainder=c->GetInv().GetItem(int16_t(pending.row.values.slot_id)))c->SendItemPacket(int16_t(pending.row.values.slot_id),remainder,ItemPacketTrade);}else c->QueuePacket(deletion.get());FurnitureResult(c,0);FurnitureBroadcastScenes();return true;
 }
 auto cached=session.presentations.find(source.item_id);const auto* data=cached!=session.presentations.end()&&cached->second?cached->second->GetUnscaledItem():nullptr;
 if(previous==session.vault_actions.end()&&(!data||data->Placeable))return false;
 if(action!=2&&action!=3&&action!=6){FurnitureResult(c,17);return true;}if(!StoredSource(source)){FurnitureResult(c,9);return true;}
 if(previous!=session.vault_actions.end()&&previous->second.completed&&previous->second.partition&&previous->second.transfer&&cached!=session.presentations.end()&&cached->second&&!Housing::InventoryProjection::Matches(previous->second.transfer->expected,*cached->second)){session.vault_actions.erase(previous);previous=session.vault_actions.end();}
 if(previous!=session.vault_actions.end()&&previous->second.completed){FurnitureResult(c,0);FurnitureBroadcastScenes();return true;}
 if(previous==session.vault_actions.end()){
  if(!data||!cached->second||!SourceGuid(source,*cached->second)||!VaultQuantity(source,*cached->second)||!VaultRoomRetrySpace(session)){FurnitureResult(c,9);return true;}
  auto view=LoadHousingView(c);auto from=view?VaultRealmFor(c,session,bridge,*view,0,source.estate_id):std::nullopt;auto to=view?VaultRealmFor(c,session,bridge,*view,action):std::nullopt;if(!from||!to){FurnitureResult(c,23);return true;}
  auto found=std::find_if(from->items.begin(),from->items.end(),[&](const auto&v){return v.native_item_id==source.item_id;});if(found==from->items.end()||!Housing::InventoryBridge::MatchesProjection(found->stored.row,*cached->second)){FurnitureResult(c,9);return true;}
  Session::VaultPending pending;pending.key=FurnitureKey("vault-transfer:");Housing::LiveItems::VaultTransferRequest request;request.request_key=pending.key;request.estate_id=from->context.estate_id;request.custody_id=found->stored.custody_id;request.from=from->context;request.from.expected_native_item_id=found->native_item_id;request.from.expected_revision=found->revision;request.from.member_revision=found->membership_revision;request.to=to->context;request.expected=found->stored.row;request.partition_quantity=Housing::StackPartitionApplication::Requested(*cached->second,source.opaque);pending.partition=bool(request.partition_quantity);pending.transfer=request;previous=session.vault_actions.emplace(incoming,std::move(pending)).first;
 }
 if(!previous->second.transfer){FurnitureResult(c,9);return true;}auto result=bridge.TransferVault(*previous->second.transfer);if(!result){FurnitureResult(c,VaultResult(result.error));return true;}previous->second.completed=true;FurnitureResult(c,0);FurnitureBroadcastScenes();return true;
}
bool FurnitureVaultPickup(Client* c,Session& session,const Housing::PlacementWire::Source& source,const Bytes& incoming){
 auto previous=session.vault_actions.find(incoming);if(previous!=session.vault_actions.end()&&previous->second.completed&&previous->second.partition&&!c->GetInv().GetItem(EQ::invslot::slotCursor)&&!c->GetInv().CursorSize()){session.vault_actions.erase(previous);previous=session.vault_actions.end();}auto cached=session.presentations.find(source.item_id);const auto* data=cached!=session.presentations.end()&&cached->second?cached->second->GetUnscaledItem():nullptr;
 if(previous==session.vault_actions.end()&&(!data||data->Placeable))return false;
 if(!StoredSource(source)){FurnitureResult(c,9);return true;}
 if(previous!=session.vault_actions.end()&&previous->second.completed){auto* item=c->GetInv().GetItem(EQ::invslot::slotCursor);if(item&&item->GetSerialNumber()==previous->second.serial)c->SendItemPacket(EQ::invslot::slotCursor,item,ItemPacketTrade);FurnitureResult(c,0);FurnitureBroadcastScenes();return true;}
 auto* bridge=c->GetHousingInventoryBridge();if(!bridge){FurnitureResult(c,1);return true;}
 if(previous==session.vault_actions.end()){
  if(!data||!cached->second||!SourceGuid(source,*cached->second)||!VaultQuantity(source,*cached->second)||!VaultRoomRetrySpace(session)){FurnitureResult(c,9);return true;}
  auto view=LoadHousingView(c);auto realm=view?VaultRealmFor(c,session,*bridge,*view,0,source.estate_id):std::nullopt;if(!realm){FurnitureResult(c,23);return true;}
  auto found=std::find_if(realm->items.begin(),realm->items.end(),[&](const auto&v){return v.native_item_id==source.item_id;});if(found==realm->items.end()||found->stored.row.values.character_id!=c->CharacterID()||!Housing::InventoryBridge::MatchesProjection(found->stored.row,*cached->second)){FurnitureResult(c,9);return true;}
  if(!RoF2::SerializeHousingItem(*cached->second,EQ::invslot::slotCursor)){FurnitureResult(c,1);return true;}
  Session::VaultPending pending;pending.key=FurnitureKey("vault-return:");pending.item=*found;pending.context=realm->context;pending.context.expected_native_item_id=found->native_item_id;pending.context.expected_revision=found->revision;pending.context.member_revision=found->membership_revision;previous=session.vault_actions.emplace(incoming,std::move(pending)).first;
 }
 auto& pending=previous->second;if(!pending.item||c->GetInv().CursorSize()||c->GetInv().GetItem(EQ::invslot::slotCursor)){FurnitureResult(c,1);return true;}
 auto result=bridge->ReturnVault(pending.key,*pending.item,pending.context,EQ::invslot::slotCursor,source.opaque);if(!result){FurnitureResult(c,VaultResult(result.error));return true;}pending.completed=true;pending.partition=bool(result.value.partition);auto* item=c->GetInv().GetItem(EQ::invslot::slotCursor);pending.serial=item?item->GetSerialNumber():0;if(item)c->SendItemPacket(EQ::invslot::slotCursor,item,ItemPacketTrade);if(!result.value.partition)session.presentations.erase(source.item_id);FurnitureResult(c,0);FurnitureBroadcastScenes();return true;
}
}

void Client::HousingUpkeepChanged() noexcept{
 for(const auto& entry:entity_list.GetClientList())if(entry.second){
  entry.second->InvalidateHousingTrophyBenefits();
  entry.second->InvalidateHousingGuildTrophyBenefits();
  // Existing receipt/recovery/ACK intent remains intact during invalidation.
 }
 // Include current escrow snapshots as well as storage/list/world deltas.
 FurnitureBroadcastScenes(true);
}

void Client::Handle_OP_RealEstatePlacementBegin(const EQApplicationPacket* app){
 if(!NativeFurniture(this))return;
 try{
  auto request=Housing::PlacementWire::DecodeBegin(app->pBuffer,app->size);
  if(request)LogInfo("Native housing Begin kind [{}] slots [{}/{}/{}] estate [{}] item [{}] action [{}] inventory_source [{}]",
   request->source.location[0],int16_t(request->source.location[1]),int16_t(request->source.location[1]>>16),
   int16_t(request->source.location[2]),request->source.estate_id,request->source.item_id,request->action,request->inventory_source);
  // Captured native list Place and menu action2/parameter1 share sourceflag1.
  // This field cannot prove a menu was issued; current server state authorizes.
  if(request&&StoredSource(request->source)&&request->source.estate_id!=0xffffffff&&request->source.item_id!=0xffffffff&&request->source.opaque==0&&request->action==1&&request->inventory_source==1){
   if(!m_housing_placement_session){FurnitureResult(this,9);return;}auto& session=*m_housing_placement_session;
   auto cached=session.presentations.find(request->source.item_id);
   if(cached==session.presentations.end()||!cached->second||!KnownPlacedModel(*cached->second)||!SourceGuid(request->source,*cached->second)){FurnitureResult(this,9);return;}
   auto view=LoadHousingView(this);if(!view){FurnitureResult(this,1);return;}
   auto crate_source=session.attached_crates.find(request->source.estate_id);const bool from_crate=crate_source!=session.attached_crates.end();
   auto* bridge=GetHousingInventoryBridge();if(!bridge){FurnitureResult(this,1);return;}
   auto attached=from_crate?session.attached_crate_rooms.find(request->source.estate_id):session.attached_crate_rooms.end();
   if(from_crate&&!view->interior&&attached!=session.attached_crate_rooms.end()){
    auto room=bridge->ReadAttachedInterior(attached->second.context.estate_id,attached->second.house->house_id);if(!room||!room.value.house||!room.value.exterior_source){FurnitureResult(this,23);return;}
    auto target=FurniturePlot(this,*view,*cached->second,room.value.context.native_plot_id);if(!target||target->room||!FurniturePlayerInRegion(this,target->region)){FurnitureResult(this,23);return;}
    auto yard=bridge->ReadScene(target->node.ownership_estate_id);if(!yard||!yard.value.access.Allows(Housing::AccessPolicy::Capability::AddItem)){FurnitureResult(this,73);return;}
    auto container=std::find_if(room.value.contents.begin(),room.value.contents.end(),[&](const auto& i){return i.crate&&i.crate->binding.crate_id==crate_source->second.crate_id&&i.crate->binding.native_estate_id==request->source.estate_id&&!i.stored.stowed;});
    auto member=std::find_if(room.value.crate_contents.begin(),room.value.crate_contents.end(),[&](const auto& i){return i.stored.placement&&i.stored.placement->reservation.native_item_id==request->source.item_id&&i.contained_in_crate==crate_source->second.crate_id;});
    if(container==room.value.contents.end()||container->crate->packed||member==room.value.crate_contents.end()||!member->room||member->crate||!member->stored.stowed||!Housing::InventoryBridge::MatchesProjection(member->stored.row,*cached->second)||!room.value.access.effective.Allows(Housing::AccessPolicy::Capability::MoveItem)||!room.value.access.effective.Allows(Housing::AccessPolicy::Capability::RemoveItem)){FurnitureResult(this,9);return;}
    Session::Move move;move.before=member->stored;move.presentation.reset(cached->second->Clone());move.source=request->source;move.key=FurnitureKey("crate-yard-place:");move.begin_bytes=Bytes(app->pBuffer,app->pBuffer+app->size);move.room=Housing::LiveItems::InteriorMoveContext{room.value.house->house_id,room.value.house->revision,room.value.profile_revision,member->scope_revision,room.value.context.source};
    move.crate_member=Housing::LiveItems::CrateMemberContext{container->crate->binding.crate_id,container->crate->binding.revision,room.value.house->interior->native_estate_id,room.value.context.catalog_revision,*room.value.exterior_source};
    Housing::InteriorStorage::TransferRequest transfer;transfer.request_key=move.key;transfer.occupied_room=*move.room;transfer.custody_id=member->stored.custody_id;transfer.expected_pose_revision=member->stored.placement->pose_revision;transfer.expected_storage_revision=member->stored.storage_revision;transfer.source_native_estate_id=member->stored.placement->reservation.native_estate_id;transfer.native_item_id=request->source.item_id;transfer.yard=Housing::InteriorStorage::ExteriorContext{yard.value.estate_id,yard.value.native_plot_id,yard.value.current_catalog_revision,yard.value.route};transfer.crate_member=move.crate_member;move.transfer=std::move(transfer);
    if(session.pending&&!session.pending->completed&&!bridge->CancelPlacement(session.pending->reservation.native_item_id)){FurnitureResult(this,1);return;}session.pending.reset();session.move=std::move(move);FurnitureMovePreview(this,*session.move,*target,*request);return;
   }
   auto target=FurniturePlot(this,*view,*cached->second,from_crate?crate_source->second.parent_native_estate_id:request->source.estate_id);
   if(!target||!bridge){FurnitureResult(this,23);return;}
   auto current=FurnitureSnapshot(this,*bridge,*target);if(!current){FurnitureResult(this,1);return;}
   std::optional<Housing::LiveItems::CrateMemberContext> member_context;
   if(from_crate){
    if(!FurniturePlayerInRegion(this,*current)){FurnitureResult(this,23);return;}
    auto container=std::find_if(current->items.begin(),current->items.end(),[&](const auto& item){return item.crate&&item.crate->binding.crate_id==crate_source->second.crate_id&&item.crate->binding.native_estate_id==request->source.estate_id&&!item.stored.stowed;});
    if(container==current->items.end()||container->crate->packed){FurnitureResult(this,22);return;}
    member_context=Housing::LiveItems::CrateMemberContext{container->crate->binding.crate_id,container->crate->binding.revision,current->native_plot_id,current->current_catalog_revision,current->route};
   }
   const auto& members=from_crate?current->crate_contents:current->items;
   auto row=std::find_if(members.begin(),members.end(),[&](const auto& r){return r.stored.placement&&r.stored.placement->reservation.native_estate_id==(from_crate?current->native_plot_id:request->source.estate_id)&&r.stored.placement->reservation.native_item_id==request->source.item_id&&(!from_crate||r.contained_in_crate==member_context->crate_id);});
   if(row==members.end()||(from_crate&&row->crate)||!current->access.Allows(Housing::AccessPolicy::Capability::MoveItem)||!FurnitureCurrentCatalog(row->stored,*target,current->current_catalog_revision)||!Housing::InventoryBridge::MatchesProjection(row->stored.row,*cached->second)||!FurniturePresentationMatches(*row,*target,*cached->second)){FurnitureResult(this,9);return;}
   if(session.move&&!session.move->completed&&(bool(session.move->crate_member)!=bool(member_context)||(member_context&&(session.move->crate_member->crate_id!=member_context->crate_id||session.move->crate_member->revision!=member_context->revision)))){FurnitureResult(this,9);return;}
   auto distance=FurnitureDistance(this,*cached->second,row->stored.placement->transform);
   if(!row->stored.stowed&&(!distance||*distance>20)){FurnitureResult(this,20);return;}
   const Bytes begin(app->pBuffer,app->pBuffer+app->size);
   if(session.move&&!session.move->completed&&session.move->begin_bytes==begin){
    if(session.move->before.custody_id!=row->stored.custody_id||session.move->before.placement->pose_revision!=row->stored.placement->pose_revision||session.move->before.storage_revision!=row->stored.storage_revision||session.move->before.stowed!=row->stored.stowed||!Housing::HousePresentation::SameBinding(session.move->house,row->house)||!Housing::CratePresentation::SameBinding(session.move->crate,row->crate)){FurnitureResult(this,1);return;}
    FurnitureMovePreview(this,*session.move,*target,*request);return;
   }
   Session::Move move;
   if(target->room){const auto& room=*target->room;const auto& contents=from_crate?room.crate_contents:room.contents;auto item=std::find_if(contents.begin(),contents.end(),[&](const auto& entry){return entry.stored.custody_id==row->stored.custody_id;});if(item==contents.end()||!item->room){FurnitureResult(this,9);return;}move.room=Housing::LiveItems::InteriorMoveContext{room.house->house_id,room.house->revision,room.profile_revision,item->scope_revision,room.context.source};}
   move.crate_member=member_context;move.before=row->stored;move.house=row->house;move.crate=row->crate;move.source=request->source;move.presentation.reset(cached->second->Clone());move.key=FurnitureKey("move:");move.begin_bytes=begin;
   auto scene=FurnitureCurrentScene(this,session,*bridge,*target);if(!scene){FurnitureResult(this,1);return;}
   if(session.pending&&!session.pending->completed&&!bridge->CancelPlacement(session.pending->reservation.native_item_id)){FurnitureResult(this,1);return;}
   session.pending.reset();session.move=std::move(move);
   FurnitureSendScene(this,session,*scene);FurnitureMovePreview(this,*session.move,*target,*request);return;
  }
  const auto source_slot=request?Housing::PlacementWire::InventorySlot(request->source):std::nullopt;
  if(!request||!source_slot||request->source.estate_id!=0xffffffff||request->source.item_id!=0xffffffff||request->source.opaque||request->action!=1||request->inventory_source!=1){FurnitureResult(this,1);return;}
  auto* source=GetInv().GetItem(int16_t(*source_slot));
  const auto reject=[&](uint32_t code,const char* stage,int backend=-1){
   LogInfo("Native inventory Begin rejected actor [{}] slot [{}] template [{}] stage [{}] result [{}] backend [{}]",CharacterID(),*source_slot,source&&source->GetUnscaledItem()?source->GetUnscaledItem()->ID:0,stage,code,backend);FurnitureResult(this,code);
  };
  if(*source_slot==EQ::invslot::slotCursor&&GetInv().CursorSize()!=1){
   Message(Chat::Red,"Put the queued items on your cursor into inventory, then pick up just one item to place.");reject(9,"queued-cursor");return;
  }
  const bool cursor_compatible=*source_slot!=uint32_t(EQ::invslot::slotCursor)||GetInv().CursorSize()==1;
  if(m_housing_placement_session)m_housing_placement_session->geometry_preview_only=false;
  if(source&&*source_slot==uint32_t(EQ::invslot::slotCursor)&&cursor_compatible&&SourceGuid(request->source,*source)&&!KnownFurniture(*source)&&HouseGeometryPreviewScope(this)){
   if(m_housing_placement_session&&((m_housing_placement_session->pending&&!m_housing_placement_session->pending->completed)||(m_housing_placement_session->move&&!m_housing_placement_session->move->completed))){FurnitureResult(this,1);return;}
   if(HouseGeometryPreview(this,*source,*request)){if(!m_housing_placement_session)m_housing_placement_session=std::make_shared<Session>();m_housing_placement_session->geometry_preview_only=true;return;}
   FurnitureResult(this,9);return;
  }
  const bool source_house=source&&Housing::HousePlacement::Model(*source);
  const bool source_crate=source&&Housing::CratePlacement::Model(*source);
  if(!source||!cursor_compatible||(!KnownFurniture(*source)&&!source_house&&!source_crate)||!SourceGuid(request->source,*source)){reject(9,"source-model-guid");return;}
  auto row=FurnitureInventory(this,*source_slot);if(!row||!Housing::InventoryBridge::MatchesProjection(*row,*source)){reject(9,"source-inventory-row");return;}
  auto view=LoadHousingView(this);if(!view){reject(1,"housing-view");return;}
  auto target=FurniturePlot(this,*view,*source);if(!target){reject(23,"target");return;}
  auto* bridge=GetHousingInventoryBridge();if(!bridge){reject(1,"bridge");return;}
  auto access=FurnitureSnapshot(this,*bridge,*target);if(!access||!access->access.Allows(source_house?Housing::AccessPolicy::Capability::AddHouse:source_crate?Housing::AccessPolicy::Capability::AddCrate:Housing::AccessPolicy::Capability::AddItem)){reject(71,access?"add-permission":"target-scene");return;}
  std::shared_ptr<const Housing::HouseBinding::Binding> house_before;
  if(source_house){
   if(source->GetHousingHouseID()){
    auto current_house=bridge->ReadInventoryHouse(*row);if(!current_house){reject(9,"inventory-house",int(current_house.error));return;}house_before=std::move(current_house.value);
   }else if(source->GetHousingEstateID()!=UINT32_MAX){FurnitureResult(this,9);return;}
  }
  std::shared_ptr<const Housing::CrateBinding::Allocation> crate_before;
  if(source_crate){if(source->GetHousingHouseID()){FurnitureResult(this,9);return;}if(source->GetHousingCrateID()){auto carried=bridge->ReadInventoryCrate(*row);if(!carried){reject(9,"inventory-crate",int(carried.error));return;}crate_before=std::move(carried.value);}else if(source->GetHousingEstateID()!=UINT32_MAX){FurnitureResult(this,9);return;}}
  if(!m_housing_placement_session)m_housing_placement_session=std::make_shared<Session>();auto& session=*m_housing_placement_session;
  const Bytes begin(app->pBuffer,app->pBuffer+app->size);
  if(session.pending&&!session.pending->completed){
   auto& prior=*session.pending;
   if(prior.begin_bytes==begin&&prior.serial==source->GetSerialNumber()&&prior.reservation.native_estate_id==target->node.native_id.value&&prior.reservation.catalog_key==target->key&&prior.reservation.catalog_revision==target->revision&&Housing::HousePresentation::SameBinding(prior.house_before,house_before)&&Housing::CratePresentation::SameBinding(prior.crate_before,crate_before)&&bool(prior.room)==bool(target->room)&&(!target->room||(prior.room->house_id==target->room->house->house_id&&prior.room->house_revision==target->room->house->revision&&prior.room->profile_revision==target->room->profile_revision))){FurniturePreview(this,prior,*target,*request);return;}
   auto canceled=bridge->CancelPlacement(prior.reservation.native_item_id);if(!canceled){reject(1,"prior-cancel",int(canceled.error));return;}
  }
  session.pending.reset();
  session.move.reset();
  auto current=FurnitureCurrentScene(this,session,*bridge,*target);if(!current||current->entries.size()>=target->definition.max_items){reject(10,current?"scene-capacity":"scene-presentation");return;}
  FurniturePending pending;if(source_house){pending.house_before=house_before;pending.house_identity=house_before?Housing::LiveItems::HouseIdentity{house_before->house_id,house_before->revision}:Housing::LiveItems::HouseIdentity{0,0};}if(source_crate){pending.crate_before=crate_before;if(crate_before)pending.crate_identity=Housing::LiveItems::CrateIdentity{crate_before->binding.crate_id,crate_before->binding.revision};}pending.row=*row;pending.serial=source->GetSerialNumber();pending.source=request->source;pending.presentation.reset(source->Clone());pending.begin_bytes=begin;
  pending.reserve_key=FurnitureKey("preview:");pending.store_key=FurnitureKey("place:");
  Housing::LiveItems::PreviewRequest reserve;reserve.request_key=pending.reserve_key;reserve.estate_id=target->node.ownership_estate_id;reserve.native_estate_id=target->node.native_id.value;reserve.catalog_key=target->key;reserve.catalog_revision=target->revision;reserve.expected=pending.row;reserve.verified_runtime_serial=pending.serial;
  if(target->room){
   const auto& current=*target->room;Housing::LiveItems::InteriorPreviewRequest request;request.request_key=pending.reserve_key;request.expected=pending.row;request.verified_runtime_serial=pending.serial;request.expected_house_id=current.house->house_id;request.expected_house_revision=current.house->revision;request.expected_profile_revision=current.profile_revision;
   auto allocated=bridge->ReserveInteriorPlacement(request);if(!allocated){reject(FurniturePlacementResult(allocated.error),"reserve-interior",int(allocated.error));return;}pending.room=allocated.value;pending.reservation=allocated.value.placement;
  }else{auto allocated=bridge->ReservePlacement(reserve);if(!allocated){reject(FurniturePlacementResult(allocated.error),"reserve-yard",int(allocated.error));return;}pending.reservation=allocated.value;}
  session.pending=std::move(pending);
  FurniturePreview(this,*session.pending,*target,*request);
  LogInfo("Native inventory Begin preview queued actor [{}] slot [{}] template [{}] native [{}]",CharacterID(),*source_slot,session.pending->row.values.item_id,session.pending->reservation.native_item_id);
 }catch(const std::exception& error){LogError("Native furniture begin rejected [{}]",error.what());FurnitureResult(this,1);}
}

void Client::Handle_OP_RealEstatePlacementCommit(const EQApplicationPacket* app){
 if(!NativeFurniture(this))return;
 if(m_housing_placement_session&&m_housing_placement_session->geometry_preview_only){
  auto request=app?Housing::PlacementWire::DecodeCommit(app->pBuffer,app->size):std::nullopt;
  if(request){std::string hex;const char* digits="0123456789abcdef";for(uint32_t i=0;i<app->size;++i){hex+=digits[app->pBuffer[i]>>4];hex+=digits[app->pBuffer[i]&15];}LogInfo("HOUSE_GEOMETRY_PREVIEW_ONLY rejected commit body [{}]",hex);}
  FurnitureResult(this,1);return;
 }
 bool durable_success=false;
 try{
  auto request=Housing::PlacementWire::DecodeCommit(app->pBuffer,app->size);
  if(request&&m_housing_placement_session&&m_housing_placement_session->move){
   auto& session=*m_housing_placement_session;auto& move=*session.move;const auto& frozen=*move.before.placement;const auto& reserved=frozen.reservation;
   if(!StoredCommitSource(request->source)||request->source.estate_id!=reserved.native_estate_id||request->source.item_id!=reserved.native_item_id||request->source.opaque||!SourceGuid(request->source,*move.presentation)){FurnitureResult(this,9);return;}
   if(auto npc=FurnitureNpc(*move.presentation);npc&&!Housing::NpcRuntime::SubmittedBounds(*npc,*request)){FurnitureResult(this,1);return;}
   const Bytes commit(app->pBuffer,app->pBuffer+app->size);
   if(move.completed&&move.commit_bytes!=commit){FurnitureResult(this,1);return;}
   auto view=LoadHousingView(this);if(!view){FurnitureResult(this,1);return;}
   if(move.transfer){
    auto* bridge=GetHousingInventoryBridge();const auto& expected=*move.transfer;
    if(!bridge||view->interior||!expected.yard){FurnitureResult(this,23);return;}
    auto target=FurniturePlot(this,*view,*move.presentation,expected.yard->native_estate_id);if(!target||target->room||target->node.ownership_estate_id!=expected.yard->estate_id||target->revision!=expected.yard->catalog_revision||!FurniturePlayerInRegion(this,target->region)){FurnitureResult(this,23);return;}
    if(!move.completed){
     const auto& x=request->transform;auto transfer=expected;if(auto npc=FurnitureNpc(*move.presentation)){transfer.npc_frame=Housing::NpcRuntime::SubmittedFrame(*npc,*request);if(!transfer.npc_frame){FurnitureResult(this,1);return;}}transfer.placement=Housing::PlacementPolicy::Transform{x[0],x[1],x[2],x[3],{x[4],x[5],x[6]}};
     // Save exact retry bytes before the transaction. Transfer rechecks both
     // spaces, ACLs, container revision, capacity and destination geometry.
     move.commit_bytes=commit;auto result=bridge->TransferStorage(transfer);if(!result){FurnitureResult(this,FurniturePlacementResult(result.error));return;}move.transferred=result.value;move.completed=true;durable_success=true;
    }else durable_success=true;
    if(!move.transferred){FurnitureResult(this,1);return;}
    auto scene=FurnitureCurrentScene(this,session,*bridge,*target);if(scene)FurnitureSendScene(this,session,*scene,scene->entries.count(move.transferred->after_native_item_id)?std::optional<uint32_t>(move.transferred->after_native_item_id):std::nullopt,true);else FurnitureResult(this,0);FurnitureBroadcastScenes();return;
   }
   auto target=FurniturePlot(this,*view,*move.presentation,reserved.native_estate_id);auto* bridge=GetHousingInventoryBridge();
   if(!target||!bridge||target->node.native_id.value!=reserved.native_estate_id||(!target->room&&(target->node.ownership_estate_id!=reserved.estate_id||target->key!=reserved.catalog_key))){FurnitureResult(this,23);return;}
   if(!move.completed){
    auto current=FurnitureSnapshot(this,*bridge,*target);if(!current||!current->access.Allows(Housing::AccessPolicy::Capability::MoveItem)){FurnitureResult(this,73);return;}
    if(move.crate_member&&!FurniturePlayerInRegion(this,*current)){FurnitureResult(this,23);return;}
    const auto& members=move.crate_member?current->crate_contents:current->items;
    auto row=std::find_if(members.begin(),members.end(),[&](const auto& entry){return entry.stored.placement&&entry.stored.placement->reservation.native_item_id==reserved.native_item_id&&(!move.crate_member||entry.contained_in_crate==move.crate_member->crate_id);});
    if(row==members.end()||row->stored.custody_id!=move.before.custody_id||Housing::CustodyCodec::Encode(row->stored.row)!=Housing::CustodyCodec::Encode(move.before.row)||!FurnitureCurrentCatalog(row->stored,*target,current->current_catalog_revision)||!Housing::InventoryBridge::MatchesProjection(row->stored.row,*move.presentation)||!Housing::HousePresentation::Matches(*row,*move.presentation)||!Housing::HousePresentation::SameBinding(row->house,move.house)||!Housing::CratePresentation::SameBinding(row->crate,move.crate)||row->stored.storage_revision!=move.before.storage_revision||row->stored.stowed!=move.before.stowed||row->stored.placement->pose_revision!=frozen.pose_revision||!FurnitureSameTransform(row->stored.placement->transform,frozen.transform)){FurnitureResult(this,9);return;}
    const auto& now=row->stored.placement->reservation;
    if(now.catalog_revision!=reserved.catalog_revision||bool(now.content_binding)!=bool(reserved.content_binding)||(now.content_binding&&!Housing::ContentBinding::Same(*now.content_binding,*reserved.content_binding))){FurnitureResult(this,9);return;}
    const auto& x=request->transform;Housing::PlacementPolicy::Transform pose{x[0],x[1],x[2],x[3],{x[4],x[5],x[6]}};
    std::optional<Housing::NpcNativeFrame::Frame> npc_frame;auto geometry=FurnitureGeometry(*move.presentation);if(auto npc=FurnitureNpc(*move.presentation)){npc_frame=Housing::NpcRuntime::SubmittedFrame(*npc,*request);if(!npc_frame){FurnitureResult(this,1);return;}geometry=Housing::NpcRuntime::Geometry(*npc,npc_frame,pose);}
    Housing::PlacementPolicy::Context context{reserved.native_estate_id,target->definition.type,target->region.id,current->access,true};
    if(!FurnitureCheckPlacement(*target,context,FurnitureMetadata(*move.presentation,target->house_definition_type),geometry,pose,Housing::PlacementPolicy::Operation::Move,move.presentation.get(),reserved.native_item_id)){FurnitureResult(this,1);return;}
    auto before=FurnitureCurrentScene(this,session,*bridge,*target);if(!before||(!move.crate_member&&!before->entries.count(reserved.native_item_id))){FurnitureResult(this,9);return;}
    // Stage replay bytes and local encoders before the durable pose CAS.
    move.commit_bytes=commit;auto future=move.before;future.placement->transform=pose;future.placement->npc_frame=npc_frame;
    const auto entry=FurnitureEntry(this,future,*move.presentation,&row->creator_name,row->house.get());const auto actor=Housing::PlacementWire::EncodeActor(FurnitureActor(*move.presentation,*future.placement,row->house.get()));
    Housing::LiveItems::MoveRequest update;update.request_key=move.key;update.estate_id=reserved.estate_id;update.custody_id=move.before.custody_id;update.native_item_id=reserved.native_item_id;update.expected_pose_revision=frozen.pose_revision;update.expected_catalog_revision=current->current_catalog_revision;update.transform=pose;update.room=move.room;update.npc_frame=npc_frame;
    if(move.before.stowed){
     Housing::InteriorStorage::Request restore;restore.request_key=move.key;if(move.room)restore.room=*move.room;else restore.exterior=Housing::InteriorStorage::ExteriorContext{target->node.ownership_estate_id,target->node.native_id.value,target->revision,current->route};restore.custody_id=move.before.custody_id;restore.native_item_id=reserved.native_item_id;restore.expected_pose_revision=frozen.pose_revision;restore.expected_storage_revision=move.before.storage_revision;restore.placement=pose;restore.crate_member=move.crate_member;restore.npc_frame=npc_frame;
     auto restored=bridge->ChangeStorage(restore);if(!restored){FurnitureResult(this,1);return;}
    }else{auto moved=bridge->MovePlaced(update);if(!moved){FurnitureResult(this,1);return;}}
    durable_success=true;move.completed=true;
   }else durable_success=true;
   const bool refresh_estate=move.before.stowed&&bool(move.house);
   // Journal outcomes are immutable history; only current durable rows render.
   auto scene=FurnitureCurrentScene(this,session,*bridge,*target);
   if(!scene){FurnitureResult(this,0);Message(Chat::Red,"The move completed; its current scene is unavailable.");FurnitureBroadcastScenes(refresh_estate);return;}
   FurnitureSendScene(this,session,*scene,scene->entries.count(reserved.native_item_id)?std::optional<uint32_t>(reserved.native_item_id):std::nullopt,true);FurnitureBroadcastScenes(refresh_estate);return;
  }
  if(!request||!m_housing_placement_session||!m_housing_placement_session->pending){FurnitureResult(this,1);return;}
  auto& session=*m_housing_placement_session;auto& pending=*session.pending;
  if(!Housing::PlacementWire::InventorySlotMatches(request->source,pending.row.values.slot_id)||request->source.estate_id!=pending.reservation.native_estate_id||request->source.item_id!=pending.reservation.native_item_id||request->source.opaque||!pending.presentation||!SourceGuid(request->source,*pending.presentation)){FurnitureResult(this,9);return;}
  const Bytes commit(app->pBuffer,app->pBuffer+app->size);
  if(pending.completed&&pending.commit_bytes!=commit){FurnitureResult(this,1);return;}
  auto view=LoadHousingView(this);if(!view){FurnitureResult(this,1);return;}
  auto target=FurniturePlot(this,*view,*pending.presentation,pending.reservation.native_estate_id);
  if(!target||target->node.ownership_estate_id!=pending.reservation.estate_id||target->key!=pending.reservation.catalog_key||(!pending.reservation.content_binding&&target->revision!=pending.reservation.catalog_revision)){FurnitureResult(this,23);return;}
  auto* bridge=GetHousingInventoryBridge();if(!bridge){FurnitureResult(this,1);return;}
  if(pending.completed){durable_success=true;auto scene=FurnitureCurrentScene(this,session,*bridge,*target);if(!scene){FurnitureResult(this,0);Message(Chat::Red,"The operation completed; its current scene is unavailable.");FurnitureBroadcastScenes();return;}
   if(!pending.cursor_notified&&(pending.row.values.slot_id!=uint32_t(EQ::invslot::slotCursor)||GetInv().CursorSize()==0)&&!GetInv().GetItem(int16_t(pending.row.values.slot_id))){auto deletion=FurnitureCursorDeletion(pending.row.values.slot_id);auto* packet=deletion.release();FastQueuePacket(&packet);pending.cursor_notified=true;}
   FurnitureSendScene(this,session,*scene,scene->entries.count(pending.reservation.native_item_id)?std::optional<uint32_t>(pending.reservation.native_item_id):std::nullopt,true);FurnitureBroadcastScenes();return;}
  auto* source=GetInv().GetItem(int16_t(pending.row.values.slot_id));
  if(!source||(pending.row.values.slot_id==uint32_t(EQ::invslot::slotCursor)&&GetInv().CursorSize()!=1)||source->GetSerialNumber()!=pending.serial||!Housing::InventoryBridge::MatchesProjection(pending.row,*source)){FurnitureResult(this,9);return;}
  if(pending.house_identity){
   const auto expected_estate=pending.house_before&&pending.house_before->interior?pending.house_before->interior->native_estate_id:UINT32_MAX;
   if(source->GetHousingHouseID()!=pending.house_identity->house_id||source->GetHousingEstateID()!=expected_estate){FurnitureResult(this,9);return;}
  }
  if(pending.crate_before&&(source->GetHousingCrateID()!=pending.crate_before->binding.crate_id||source->GetHousingEstateID()!=pending.crate_before->binding.native_estate_id||source->GetHousingHouseID())){FurnitureResult(this,9);return;}
  const auto& x=request->transform;Housing::PlacementPolicy::Transform pose{x[0],x[1],x[2],x[3],{x[4],x[5],x[6]}};
  auto current=FurnitureSnapshot(this,*bridge,*target);if(!current||!current->access.Allows(pending.house_identity?Housing::AccessPolicy::Capability::AddHouse:Housing::CratePlacement::Model(*pending.presentation)?Housing::AccessPolicy::Capability::AddCrate:Housing::AccessPolicy::Capability::AddItem)){FurnitureResult(this,71);return;}
  Housing::PlacementPolicy::Context context{pending.reservation.native_estate_id,target->definition.type,target->region.id,current->access,true};
  std::optional<Housing::NpcNativeFrame::Frame> npc_frame;auto geometry=FurnitureGeometry(*pending.presentation);if(auto npc=FurnitureNpc(*pending.presentation)){npc_frame=Housing::NpcRuntime::SubmittedFrame(*npc,*request);if(!npc_frame){FurnitureResult(this,1);return;}geometry=Housing::NpcRuntime::Geometry(*npc,npc_frame,pose);}
  if(!FurnitureCheckPlacement(*target,context,FurnitureMetadata(*pending.presentation,target->house_definition_type),geometry,pose,Housing::PlacementPolicy::Operation::Add,pending.presentation.get(),pending.reservation.native_item_id,pending.room?std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>>{pending.room->content_digest}:std::nullopt)){FurnitureResult(this,1);return;}
  // Client vectors72..95 are parsed only for framing; never geometry authority.
  auto before=FurnitureCurrentScene(this,session,*bridge,*target);if(!before||before->entries.size()>=target->definition.max_items){FurnitureResult(this,10);return;}
  Housing::LiveItems::Placed placed{pending.reservation,pose,1,npc_frame};
  // All local payload/copy/packet allocations that can be prepared are staged
  // before custody commits. Never leave completed=true with missing replay bytes.
  pending.commit_bytes=commit;auto deletion=FurnitureCursorDeletion(pending.row.values.slot_id);
  Housing::LiveItems::Stored future;future.row=pending.row;future.placement=placed;
  if(!pending.house_identity){
   auto future_entry=FurnitureEntry(this,future,*pending.presentation);auto future_actor=Housing::PlacementWire::EncodeActor(FurnitureActor(*pending.presentation,placed));
   if(future_entry.size()>65523||(!target->room&&future_entry.size()>65535-before->list.size())){FurnitureResult(this,10);return;}
  }
  // A new house receives its stable HouseID/interior in the real parent Store.
  // Never invent those IDs to stage a pre-commit world/item-list presentation.
  // Only the fresh post-application Scene can project and publish that identity.
  auto inserted=session.presentations.emplace(pending.reservation.native_item_id,std::unique_ptr<EQ::ItemInstance>(pending.presentation->Clone()));
  if(!inserted.second){FurnitureResult(this,1);return;}
  auto stored=bridge->Store(pending.store_key,pending.reservation.estate_id,pending.row,placed,pending.house_identity,pending.room,false,std::nullopt,pending.crate_identity);
  // No inventory pointer is read after Store: its fence has moved the instance.
  if(!stored){LogInfo("Native housing Store rejected actor [{}] estate [{}] template [{}] house [{}] code [{}]",CharacterID(),pending.reservation.estate_id,pending.row.values.item_id,bool(pending.house_identity),int(stored.error));session.presentations.erase(pending.reservation.native_item_id);FurnitureResult(this,FurniturePlacementResult(stored.error));return;}
  durable_success=true;pending.completed=true;
  auto* deleted=deletion.release();FastQueuePacket(&deleted);pending.cursor_notified=true;
  auto scene=FurnitureCurrentScene(this,session,*bridge,*target);
  if(!scene||!scene->entries.count(pending.reservation.native_item_id)){
   // Custody is committed; do not report a rollback or replay stale receipt pose.
   FurnitureResult(this,0);Message(Chat::Red,"Your furniture was stored, but its current scene could not be refreshed. Re-enter the neighborhood to refresh it.");FurnitureBroadcastScenes();return;
  }
  FurnitureSendScene(this,session,*scene,pending.reservation.native_item_id,true);FurnitureBroadcastScenes(bool(Housing::HousePlacement::Model(*pending.presentation)||Housing::CratePlacement::Model(*pending.presentation)));
 }catch(const std::exception&){LogError("Native furniture commit could not complete its current view");FurnitureResult(this,durable_success?0:1);if(durable_success)FurnitureBroadcastScenes();}
}

void Client::SendHousingPlacedScene(){
 if(!NativeFurniture(this))return;
 try{
  auto view=LoadHousingView(this);auto* bridge=GetHousingInventoryBridge();
  if(!view||!bridge){LogInfo("Native furniture bootstrap unavailable actor [{}] view [{}] bridge [{}]",CharacterID(),bool(view),bool(bridge));if(m_housing_placement_session){auto& session=*m_housing_placement_session;session.room_items.clear();session.room_context.reset();while(!session.sent_worlds.empty())FurniturePurgeScene(this,session,session.sent_worlds.begin()->first);}return;}
  if(!m_housing_placement_session)m_housing_placement_session=std::make_shared<Session>();
  auto& session=*m_housing_placement_session;std::vector<uint32_t> retained;session.attached_rooms.clear();session.attached_crates.clear();session.contained_items.clear();session.attached_crate_rooms.clear();
  auto queue_room_crates=[&](const Housing::Scene::InteriorSnapshot& room){auto projected=FurnitureRoomCrateSnapshot(room);if(!projected)return;
   for(const auto& member:projected->crate_contents)if(member.stored.placement)session.contained_items.insert(member.stored.placement->reservation.native_item_id);
   for(const auto& item:projected->items)if(item.crate){auto contents=FurnitureReadCrateScene(this,session,*projected,item);if(contents){session.attached_crate_rooms[contents->estate]=room;FurnitureSendScene(this,session,*contents);retained.push_back(contents->estate);}}
  };

  if(view->interior){
   auto room=FurnitureReadRoomScene(this,session,*view->interior);
   if(room){queue_room_crates(*view->interior);FurnitureSendScene(this,session,*room);retained.push_back(room->estate);LogInfo("Native room furniture scene queued actor [{}] estate [{}] items [{}]",CharacterID(),room->estate,room->worlds.size());}
   auto parent=bridge->ReadParentScene();
   if(parent){
    auto node=std::find_if(view->data.catalog.nodes.begin(),view->data.catalog.nodes.end(),[&](const auto& node){return node.native_id.value==parent.value.native_plot_id;});
    if(node!=view->data.catalog.nodes.end()){
     FurnitureTarget target{*node,parent.value.definition,parent.value.region,view->root,view->data.catalog.key,parent.value.current_catalog_revision};
     auto yard=FurnitureReadScene(this,session,*bridge,target,&parent.value,false);
     if(yard){
      FurnitureSendScene(this,session,*yard);retained.push_back(yard->estate);
      for(const auto& item:parent.value.items)if(item.crate){auto contents=FurnitureReadCrateScene(this,session,parent.value,item);if(contents){FurnitureSendScene(this,session,*contents);retained.push_back(contents->estate);}}
     }
    }
   }
   else{
    if(!room){session.room_items.clear();session.room_context.reset();}
    // The current room was independently authorized above. An unavailable
    // parent-yard projection must not erase its list or mutation context.
    // The retained-estate sweep below still removes every stale parent scene.
    LogInfo("Native parent furniture scene unavailable actor [{}] code [{}]; retaining authorized room",CharacterID(),int(parent.error));
   }
  }else{session.room_items.clear();session.room_context.reset();}
  // Current Enter-authorized read view; every mutation rechecks its typed capability.
  if(!view->interior)for(const auto& node:view->data.catalog.nodes){
   if(!node.ownership_estate_id||node.neighborhood_id!=view->neighborhood||node.parent_key.empty()||view->ids.at(node.parent_key)!=view->root)continue;
   auto definition=std::find_if(view->data.catalog.definitions.begin(),view->data.catalog.definitions.end(),[&](const auto& d){return d.id==node.definition_id;});
   if(definition==view->data.catalog.definitions.end()||(definition->type!=4&&definition->type!=8))continue;
   auto region=std::find_if(view->data.catalog.regions.begin(),view->data.catalog.regions.end(),[&](const auto& r){return r.id==definition->area_id;});if(region==view->data.catalog.regions.end())continue;
   FurnitureTarget target{node,*definition,*region,view->root,view->data.catalog.key,view->data.catalog.revision};
   auto snapshot=FurnitureSnapshot(this,*bridge,target);
   if(snapshot)for(const auto& member:snapshot->crate_contents)if(member.stored.placement)session.contained_items.insert(member.stored.placement->reservation.native_item_id);
   auto scene=snapshot?FurnitureReadScene(this,session,*bridge,target,&*snapshot):std::nullopt;
   if(scene){LogInfo("Native furniture scene queued actor [{}] estate [{}] items [{}]",CharacterID(),node.native_id.value,scene->worlds.size());FurnitureSendScene(this,session,*scene);retained.push_back(node.native_id.value);}
   else if(session.sent_worlds.count(node.native_id.value))FurniturePurgeScene(this,session,node.native_id.value);
   if(snapshot)for(const auto& item:snapshot->items)if(item.crate){auto contents=FurnitureReadCrateScene(this,session,*snapshot,item);if(contents){FurnitureSendScene(this,session,*contents);retained.push_back(contents->estate);}}
   if(snapshot)for(const auto& item:snapshot->items)if(item.house){
    auto attached=bridge->ReadAttachedInterior(snapshot->estate_id,item.house->house_id);if(!attached||!attached.value.room_profile)continue;
    // Capacity/definition projection only. Room geometry remains in its own
    // world; the persistent parent catalog/content identity is unchanged.
    auto projected=view->data;if(!Housing::InteriorNativeView::Apply(projected,*attached.value.room_profile,attached.value.house->interior->native_estate_id,&attached.value.contents))continue;
    auto room_state=view->states.find(attached.value.house->interior->native_estate_id);auto owner_acl=view->permissions.find(attached.value.context.native_plot_id);
    if(room_state==view->states.end()||owner_acl==view->permissions.end())continue;
    auto parent_state=room_state->second;std::vector<std::pair<EmuOpcode,Bytes>> room_packets;
    room_packets.emplace_back(OP_RealEstateDefinitions,projected.definition_payload);
    for(const auto& child:projected.catalog.nodes)if(child.parent_key=="house:"+std::to_string(attached.value.house->house_id)){
     Housing::EstateWire::EstateSnapshot state;state.estate_id=child.native_id.value;state.parent_id=parent_state.estate_id;state.definition_id=child.definition_id;state.owner_name=parent_state.owner_name;state.owner_handle=parent_state.owner_handle;state.guild_id=parent_state.guild_id;
     if(std::find(parent_state.children.begin(),parent_state.children.end(),state.estate_id)==parent_state.children.end())parent_state.children.push_back(state.estate_id);
     room_packets.emplace_back(OP_RealEstateSnapshot,Housing::EstateWire::EncodeEstateSnapshot(state));
     auto acl=owner_acl->second;acl.estate_id=state.estate_id;room_packets.emplace_back(OP_RealEstateAccess,Housing::PermissionWire::EncodeSnapshot(acl));
    }
    room_packets.emplace_back(OP_RealEstateSnapshot,Housing::EstateWire::EncodeEstateSnapshot(parent_state));
    for(const auto& packet:room_packets)SendEstatePayload(this,packet.first,packet.second);
    auto contents=FurnitureReadRoomScene(this,session,attached.value,false);if(contents){queue_room_crates(attached.value);FurnitureSendScene(this,session,*contents);retained.push_back(contents->estate);}
   }
  }
  for(auto sent=session.sent_worlds.begin();sent!=session.sent_worlds.end();){auto id=sent++->first;if(std::find(retained.begin(),retained.end(),id)==retained.end())FurniturePurgeScene(this,session,id);}
 }catch(const std::exception&){LogError("Native current furniture scene unavailable");if(m_housing_placement_session){try{auto& session=*m_housing_placement_session;while(!session.sent_worlds.empty())FurniturePurgeScene(this,session,session.sent_worlds.begin()->first);}catch(const std::exception&){LogError("Native denied furniture scene purge failed");}}}
}

bool Client::TryHousingHouseExit(bool allow_absent){
  if(!RuleB(Housing,EnablePlayerHousing)||ClientVersion()!=EQ::versions::ClientVersion::RoF2||!Connected()||!ClientDataLoaded()||!HasInventorySessionReservation())return true;
  try{
   auto* bridge=GetHousingInventoryBridge();if(!bridge)return true;
   if(m_housing_placement_session){
    auto& session=*m_housing_placement_session;
    if(session.geometry_preview_only)return true;
    // Native Escape closes its preview locally. A subsequent authenticated
    // exit request cancels the still-uncommitted server reservation first.
    // Committed inventory/application state is never discarded here.
    if(session.pending&&!session.pending->completed){
     if(!bridge->CancelPlacement(session.pending->reservation.native_item_id)){
      LogInfo("Native house exit preview cancellation unavailable actor [{}] native [{}]",CharacterID(),session.pending->reservation.native_item_id);
      Message(Chat::Red,"Your unfinished placement could not be canceled.");return true;
     }
     LogInfo("Native house exit canceled unfinished preview actor [{}] native [{}]",CharacterID(),session.pending->reservation.native_item_id);
     session.pending.reset();
    }
    if(session.move&&!session.move->completed)session.move.reset();
   }
   auto result=bridge->ExitHouse();
   if(!result){if(allow_absent&&result.error==Housing::LiveItems::Error::NotFound)return false;Message(Chat::Red,"This house's exit route is unavailable.");return true;}
   const auto& exit=result.value;
   LogInfo("Native house exit resolved actor [{}] house [{}] interior [{}] destination zone [{}] instance [{}]",CharacterID(),exit.house_id,exit.native_house_id,exit.destination.zone_id,exit.destination.instance_id);
   MovePC(exit.destination.zone_id,exit.destination.instance_id,exit.spawn[0],exit.spawn[1],exit.spawn[2],exit.spawn[3]);
  }catch(...){LogError("Native house exit unavailable");}
  return true;
}

bool Client::HandleHousingInteriorDoor(uint8_t door_id,bool within_distance){
 if(!RuleB(Housing,EnablePlayerHousing)||ClientVersion()!=EQ::versions::ClientVersion::RoF2||!zone||zone->GetInstanceVersion()!=0||!zone->GetInstanceID())return false;
 if(zone->GetZoneID()!=345&&zone->GetZoneID()!=737)return false;
 auto* door=entity_list.FindDoor(door_id);
 if(!door||door->GetOpenType()!=58||!(door->GetClientVersionMask()&ClientVersionBit()))return false;
 // Standard retains its original observed door. Palatial's front entrance
 // uses original OBJ_FrontEntrance with its leaf aligned to the collision-map
 // floor/lintel (the authored static origin is below this floor), corroborated by the
 // player's native doorway position. Other internal doors are not exits.
 const bool standard=zone->GetZoneID()==345&&door_id==4&&std::string(door->GetDoorName())=="OBJ_GUILD_DOOR";
 const auto& position=door->GetPosition();
 const bool palatial=zone->GetZoneID()==737&&door_id==1&&std::string(door->GetDoorName())=="OBJ_FRONTENTRANCE"&&
  std::abs(position.x-88.89984894f)<0.05f&&std::abs(position.y-0.68527031f)<0.05f&&std::abs(position.z-8.25539354f)<0.05f;
 if(!standard&&!palatial)return false;
 if(!Connected()||!ClientDataLoaded()||!HasInventorySessionReservation()||!within_distance)return true;
 // ExitHouse proves the current physical interior and its current parent;
 // safe egress remains available after Enter is revoked. Only absent housing
 // identity falls back to normal door handling for an ordinary basic hall.
 LogInfo("Native guild hall entrance exit request actor [{}] zone [{}] door [{}]",CharacterID(),zone->GetZoneID(),door_id);
 return TryHousingHouseExit(true);
}

bool Client::HandleHousingHouseGo(const EQApplicationPacket* app){
 auto* c=this;
 if(app->GetOpcode()==OP_RealEstateExit){
  if(!Housing::InteractionWire::DecodeHouseExit(app->pBuffer,app->size))return true;
  return TryHousingHouseExit();
 }
 auto request=Housing::InteractionWire::DecodeHouseGo(app->pBuffer,app->size);if(!request)return false;
 if(!NativeFurniture(c)||!c->ClientDataLoaded()||!c->HasInventorySessionReservation())return true;
 try{
  if(!m_housing_placement_session)return true;auto& session=*m_housing_placement_session;
  if(session.geometry_preview_only||(session.pending&&!session.pending->completed)||(session.move&&!session.move->completed))return true;
  auto cache=session.presentations.find(request->item_id);
  if(cache==session.presentations.end()||!cache->second||!Housing::HousePlacement::Model(*cache->second))return true;
  auto view=LoadHousingView(c);auto* bridge=c->GetHousingInventoryBridge();if(!view||!bridge)return true;
  auto target=FurniturePlot(c,*view,*cache->second,request->estate_id);if(!target)return true;
  auto current=FurnitureSnapshot(c,*bridge,*target);if(!current)return true;
  auto row=std::find_if(current->items.begin(),current->items.end(),[&](const auto& i){return i.stored.placement&&i.stored.placement->reservation.native_item_id==request->item_id;});
  if(row==current->items.end()||!row->house||!row->house->interior||!Housing::InventoryBridge::MatchesProjection(row->stored.row,*cache->second)||!Housing::HousePresentation::Matches(*row,*cache->second))return true;
  Housing::Scene::EntryRequest entry{request->estate_id,request->item_id,row->house->house_id,row->house->revision,row->stored.custody_id};
  auto admitted=bridge->AdmitHouse(target->node.ownership_estate_id,entry);
  if(!admitted||!admitted.value.admission){c->Message(Chat::Red,"This house cannot be entered from your current location.");return true;}
  const auto destination=*admitted.value.admission;
  LogInfo("Native house entry admitted actor [{}] plot [{}] item [{}] house [{}] interior [{}] zone [{}] instance [{}]",c->CharacterID(),request->estate_id,request->item_id,destination.house_id,destination.native_house_id,destination.destination.zone_id,destination.destination.instance_id);
  // The barrier is released and the transaction committed before zoning.
  c->MovePC(destination.destination.zone_id,destination.destination.instance_id,destination.spawn[0],destination.spawn[1],destination.spawn[2],destination.spawn[3]);
 }catch(...){LogError("Native house admission unavailable");}
 return true;
}

bool SendHousingVotingPreferenceSnapshot(Client* client,uint32_t native){
 try{
  auto view=LoadHousingView(client);if(!view)return false;
  auto state=view->states.find(native);if(state==view->states.end())return false;
  const auto definition=std::find_if(view->data.catalog.definitions.begin(),view->data.catalog.definitions.end(),[&](const auto& d){return d.id==state->second.definition_id;});
  if(definition==view->data.catalog.definitions.end()||(definition->type!=4&&definition->type!=8))return false;
  // The native purchase title is refreshed from DynamicName by63fd. Retain
  // the same default address that796c inspection displays when no custom name exists.
  if(state->second.dynamic_name.empty())state->second.dynamic_name=definition->address;
  SendEstatePayload(client,OP_RealEstateSnapshot,Housing::EstateWire::EncodeEstateSnapshot(state->second));return true;
 }catch(...){LogError("Housing voting snapshot unavailable");return false;}
}

void Client::Handle_OP_RealEstateItemInspect(const EQApplicationPacket* app){
 if(!NativeFurniture(this)||!Connected()||!ClientDataLoaded()||!HasInventorySessionReservation()||!app)return;
 try{
  auto request=Housing::InteractionWire::DecodeItemInspect(app->pBuffer,app->size);if(!request||!m_housing_placement_session)return;
  auto& session=*m_housing_placement_session;auto cached=session.presentations.find(request->item_id);
  if(cached==session.presentations.end()||!cached->second)return;
  auto view=LoadHousingView(this);auto* bridge=GetHousingInventoryBridge();if(!view||!bridge)return;
  auto target=FurniturePlot(this,*view,*cached->second,request->estate_id);if(!target)return;
  // Re-read current authorized scene/custody and synchronize presentation before
  // serializing. Cached IDs alone never grant inspection of another estate.
  auto current=FurnitureCurrentScene(this,session,*bridge,*target);
  if(!current||current->estate!=request->estate_id||!current->entries.count(request->item_id))return;
  cached=session.presentations.find(request->item_id);if(cached==session.presentations.end()||!cached->second)return;
  auto body=RoF2::SerializeHousingItemListEntry(*cached->second,EQ::invslot::slotCursor);if(!body)return;
  // Original58c7 consumes a direct FullItemPtr: no estate IDs, result or item-type prefix.
  SendEstatePayload(this,OP_RealEstateItemInspect,*body);
  LogInfo("Native housing item inspection actor [{}] estate [{}] item [{}] bytes [{}]",CharacterID(),request->estate_id,request->item_id,body->size());
 }catch(...){LogError("Native housing item inspection unavailable");}
}

void Client::Handle_OP_RealEstateHouseSwap(const EQApplicationPacket* app){
 if(!app||!NativeFurniture(this)||!RuleB(Housing,EnableNativeHouseSwap)||!ClientDataLoaded()||!HasInventorySessionReservation())return;
 auto request=Housing::InteractionWire::DecodeHouseSwap(app->pBuffer,app->size);
 if(!request){LogInfo("Native house swap invalid frame actor [{}] bytes [{}]",CharacterID(),app->size);return;}
 LogInfo("Native house swap request actor [{}] current [{}:{}] replacement [{}:{}]",CharacterID(),request->current.estate_id,request->current.item_id,request->replacement.estate_id,request->replacement.item_id);
 try{
  auto* bridge=GetHousingInventoryBridge();auto view=LoadHousingView(this);if(!bridge||!view||view->interior){Message(Chat::Red,"House swaps must be started from the neighborhood plot.");return;}
  auto node=std::find_if(view->data.catalog.nodes.begin(),view->data.catalog.nodes.end(),[&](const auto& n){return n.native_id.value==request->current.estate_id&&n.ownership_estate_id;});if(node==view->data.catalog.nodes.end())return;
  auto scene=bridge->ReadScene(node->ownership_estate_id);if(!scene){Message(Chat::Red,"The plot's housing state could not be verified.");LogInfo("Native house swap scene unavailable actor [{}] code [{}]",CharacterID(),int(scene.error));return;}if(!FurniturePlayerInRegion(this,scene.value)){Message(Chat::Red,"Stand inside the plot before swapping its houses.");return;}
  if(!m_housing_placement_session)m_housing_placement_session=std::make_shared<Session>();auto& session=*m_housing_placement_session;
  Bytes bytes(app->pBuffer,app->pBuffer+app->size);auto old=session.house_swaps.find(bytes);
  auto before=Housing::HouseSwapPreflight::Select(*request,scene.value,{CharacterID(),AccountID()});
  if(old!=session.house_swaps.end()&&old->second.completed&&before){session.house_swaps.erase(old);old=session.house_swaps.end();}
  if(old==session.house_swaps.end()){
   if(!before){Message(Chat::Red,"Select a stored house in this plot while its current house is placed.");return;}
   auto room=bridge->ReadAttachedInterior(node->ownership_estate_id,before.value.current.house->house_id);
   if(!room){Message(Chat::Red,"The current house interior could not be verified.");return;}
   if(std::any_of(room.value.contents.begin(),room.value.contents.end(),[](const auto& item){return item.crate&&!item.stored.stowed;})){MessageString(Chat::Red,5338);return;}
   if(session.house_swaps.size()>=64)session.house_swaps.clear();
   old=session.house_swaps.emplace(bytes,Session::Swap{FurnitureKey("houseswap:"),false}).first;
  }
  std::array<uint8_t,24> wire{};std::copy(bytes.begin(),bytes.end(),wire.begin());
  auto result=bridge->SwapHouse(old->second.key,node->ownership_estate_id,wire);
  if(!result){auto error=result.error;uint32_t message=error==Housing::LiveItems::Error::RealEstateFull?5331:error==Housing::LiveItems::Error::VaultFull?5332:error==Housing::LiveItems::Error::NpcFull?5333:0;if(message)MessageString(Chat::Red,message);else Message(Chat::Red,"The house swap could not be completed. Check permissions, occupants, and the plot's current state.");LogInfo("Native house swap rejected actor [{}] code [{}]",CharacterID(),int(error));return;}
  old->second.completed=true;session.attached_rooms.clear();session.attached_crates.clear();session.attached_crate_rooms.clear();session.room_items.clear();session.storage_requests.clear();session.transfer_requests.clear();session.pending.reset();session.move.reset();
  for(const auto& change:result.value.changes)if(change.before_native!=change.after_native)session.presentations.erase(change.before_native);
  FurnitureBroadcastScenes(true);
  Message(Chat::Yellow,"House swap completed. Items from the previous house are in the new house's storage.");
  LogInfo("Native house swap completed actor [{}] old house [{}] new house [{}] changes [{}]",CharacterID(),result.value.old_house,result.value.new_house,result.value.changes.size());
 }catch(...){LogError("Native house swap view unavailable");}
}

void Client::SendHousingChairMenu(uint32_t estate,uint32_t item,std::array<uint32_t,2> mouse,const std::string& claimed_guid){
 if(!NativeFurniture(this)||!ClientDataLoaded()||!HasInventorySessionReservation())return;
 try{
  if(!m_housing_placement_session){FurnitureResult(this,9);return;}auto& session=*m_housing_placement_session;
  auto cache=session.presentations.find(item);if(cache==session.presentations.end()||!cache->second||!KnownPlacedModel(*cache->second)){FurnitureResult(this,9);return;}
  const auto guid=FurnitureGuid(*cache->second);if(!claimed_guid.empty()&&claimed_guid!=guid){FurnitureResult(this,9);return;}
  auto view=LoadHousingView(this);if(!view){FurnitureResult(this,1);return;}
  auto target=FurniturePlot(this,*view,*cache->second,estate);auto* bridge=GetHousingInventoryBridge();if(!target||!bridge){FurnitureResult(this,23);return;}
  auto current=FurnitureSnapshot(this,*bridge,*target);if(!current){FurnitureResult(this,1);return;}
  auto row=std::find_if(current->items.begin(),current->items.end(),[&](const auto& r){return r.stored.placement&&r.stored.placement->reservation.native_estate_id==estate&&r.stored.placement->reservation.native_item_id==item;});
  if(row==current->items.end()||!FurnitureCurrentCatalog(row->stored,*target,current->current_catalog_revision)||!Housing::InventoryBridge::MatchesProjection(row->stored.row,*cache->second)||!Housing::HousePresentation::Matches(*row,*cache->second)){FurnitureResult(this,9);return;}
  const auto rent=Housing::RentState::Plot(database,current->estate_id,current->native_plot_id,false);
  if(!rent.Active()){FurnitureResult(this,rent.expired_since?24:1);return;}
  if(row->house&&Housing::UpkeepCycle::Expired(database,row->stored.custody_id,item)){FurnitureResult(this,25);return;}
  auto distance=Housing::Pets::Distance(*this,estate,item);if(!distance)distance=FurnitureDistance(this,*cache->second,row->stored.placement->transform);
  if(row->stored.stowed?!FurniturePlayerInRegion(this,*current):(!distance||*distance>20)){FurnitureResult(this,20);return;}
  const bool swap=RuleB(Housing,EnableNativeHouseSwap)&&row->house&&row->stored.stowed&&row->stored.row.values.character_id==CharacterID()&&std::any_of(current->items.begin(),current->items.end(),[&](const auto& placed){return placed.house&&!placed.stored.stowed&&placed.stored.row.values.character_id==CharacterID();})&&current->access.Allows(Housing::AccessPolicy::Capability::AddHouse)&&current->access.Allows(Housing::AccessPolicy::Capability::RemoveHouse);
  auto scene=FurnitureCurrentScene(this,session,*bridge,*target);if(!scene||!scene->entries.count(item)){FurnitureResult(this,9);return;}
  // Native5b74c4 item-owner match bypasses ordinary RemoveItem8/CanDrop.
  // Only the immutable creator receives Pickup; Move independently needs Move10.
  auto payload=Housing::InteractionWire::EncodeChairMenu(estate,item,guid,cache->second->GetUnscaledItem()->Name,mouse,current->access.Allows(Housing::AccessPolicy::Capability::MoveItem),row->stored.row.values.character_id==CharacterID()&&(!row->house||current->access.Allows(Housing::AccessPolicy::Capability::RemoveHouse))&&(!row->crate||current->access.Allows(Housing::AccessPolicy::Capability::RemoveCrate)),bool(row->house)&&!row->stored.stowed&&current->access.Allows(Housing::AccessPolicy::Capability::Enter),true,swap,!row->stored.stowed&&Housing::Pets::Distance(*this,estate,item).has_value()&&current->access.Allows(Housing::AccessPolicy::Capability::InteractItem));
  auto packet=std::make_unique<EQApplicationPacket>(OP_RealEstateItemMenu,payload.size());std::memcpy(packet->pBuffer,payload.data(),payload.size());
  FurnitureSendScene(this,session,*scene);auto* raw=packet.release();FastQueuePacket(&raw);
 }catch(const std::exception&){LogError("Native chair interaction menu unavailable");FurnitureResult(this,1);}
}

namespace {
bool FurniturePlayerInRegion(Client* player,const Housing::RegionWire::Region& region){
 Housing::RegionWire::Detail::ValidateMatrix(region.matrix);
 const auto& m=region.matrix;const double a=m[0],b=m[1],c=m[2],d=m[4],e=m[5],f=m[6],g=m[8],h=m[9],i=m[10];
 const std::array<double,9> adj{e*i-f*h,c*h-b*i,b*f-c*e,f*g-d*i,a*i-c*g,c*d-a*f,d*h-e*g,b*g-a*h,a*e-b*d};
 const double determinant=a*adj[0]+b*adj[3]+c*adj[6];if(!std::isfinite(determinant)||!determinant)return false;
 const std::array<double,3> point{player->GetY(),player->GetX(),player->GetZ()};
 for(size_t axis=0;axis<3;++axis){double local=0;for(size_t j=0;j<3;++j)local+=(point[j]-m[12+j])*adj[3*j+axis]/determinant;if(!std::isfinite(local)||std::abs(local)>=1)return false;}
 return true;
}
}
namespace {
bool FurniturePlayerInRegion(Client* player,const Housing::Scene::Snapshot& scene){
 if(FurniturePlayerInRegion(player,scene.region))return true;
 for(const auto& region:scene.additional_regions)if(FurniturePlayerInRegion(player,region))return true;
 return false;
}
}
void Client::Handle_OP_RealEstatePack(const EQApplicationPacket* app){
 if(!NativeFurniture(this))return;
 bool durable_success=false;
 try{
  // Original 613d is exactly one byte: 1 packs, 0 unpacks. It carries no estate.
  if(!app||app->size!=1||app->pBuffer[0]>1){FurnitureResult(this,1);return;}
  const bool packing=app->pBuffer[0]!=0;
  auto* bridge=GetHousingInventoryBridge();auto view=LoadHousingView(this);
  if(!bridge||!view){FurnitureResult(this,23);return;}
  if(!m_housing_placement_session)m_housing_placement_session=std::make_shared<Session>();
  auto& session=*m_housing_placement_session;
  if(session.crate_batch){
   // Resolve the immutable pending operation before selecting another set.
   if(session.crate_batch->packing!=packing){FurnitureResult(this,1);return;}
  }else{
   std::optional<Housing::Scene::Snapshot> selected;std::optional<Housing::CrateStorage::RoomContext> room_context;
   if(view->interior){auto fresh=bridge->ReadInterior();if(!fresh){FurnitureResult(this,23);return;}selected=FurnitureRoomCrateSnapshot(fresh.value);if(!selected||!FurniturePlayerInRegion(this,*selected)){FurnitureResult(this,23);return;}room_context=Housing::CrateStorage::RoomContext{fresh.value.house->house_id,fresh.value.house->revision,fresh.value.profile_revision,false};}
   else for(const auto& node:view->data.catalog.nodes){
    if(!node.ownership_estate_id||!node.native_id.value)continue;
    auto definition=std::find_if(view->data.catalog.definitions.begin(),view->data.catalog.definitions.end(),[&](const auto& d){return d.id==node.definition_id;});
    if(definition==view->data.catalog.definitions.end()||(definition->type!=4&&definition->type!=8))continue;
    auto region=std::find_if(view->data.catalog.regions.begin(),view->data.catalog.regions.end(),[&](const auto& r){return r.id==definition->area_id;});
    if(region==view->data.catalog.regions.end()||node.neighborhood_id!=view->neighborhood||node.parent_key.empty()||view->ids.at(node.parent_key)!=view->root||!FurniturePlayerInRegion(this,*region))continue;
    auto read=bridge->ReadScene(node.ownership_estate_id);if(!read)continue;
    if(selected){FurnitureResult(this,23);return;}selected=std::move(read.value);
   }
   if(!selected){FurnitureResult(this,23);return;}
   const Housing::Scene::Item* container=nullptr;
   for(const auto& item:selected->items)if(item.crate&&!item.stored.stowed){if(container){FurnitureResult(this,16);return;}container=&item;}
   if(!container){FurnitureResult(this,22);return;}
   Housing::CrateStorage::BatchRequest request;request.request_key=FurnitureKey("crate-batch:");request.estate_id=selected->estate_id;request.native_plot_id=selected->native_plot_id;request.catalog_revision=selected->current_catalog_revision;request.source=selected->route;request.crate_id=container->crate->binding.crate_id;request.crate_revision=container->crate->binding.revision;request.packing=packing;request.room=room_context;
   const auto& entries=packing?selected->items:selected->crate_contents;
   for(const auto& item:entries){
    if((!packing&&item.contained_in_crate!=request.crate_id)||item.crate)continue;
    if(!item.stored.placement){FurnitureResult(this,1);return;}
    request.members.push_back({item.stored.custody_id,item.stored.placement->pose_revision,item.stored.storage_revision});
   }
   for(const auto& item:packing?selected->vault_items:selected->crate_vault_items){if(!packing&&item.contained_in_crate!=request.crate_id)continue;request.members.push_back({item.stored.custody_id,0,item.revision,true});}
   if(request.members.empty()){FurnitureResult(this,11);return;}
   std::sort(request.members.begin(),request.members.end(),[](const auto& a,const auto& b){return a.custody_id<b.custody_id;});session.crate_batch=std::move(request);
  }
  auto result=bridge->PackCrate(*session.crate_batch);
  if(!result){if(result.error!=Housing::LiveItems::Error::CommitUncertain)session.crate_batch.reset();FurnitureResult(this,result.error==Housing::LiveItems::Error::NotFound?11:1);return;}
  // A successful bulk transfer supersedes canceled previews of its candidate
  // items. Failures retain both the original preview and retry request.
  if(session.move&&std::any_of(session.crate_batch->members.begin(),session.crate_batch->members.end(),[&](const auto& member){return member.custody_id==session.move->before.custody_id;}))session.move.reset();
  session.crate_batch.reset();durable_success=true;FurnitureResult(this,0);FurnitureBroadcastScenes(true);
 }catch(...){FurnitureResult(this,durable_success?0:1);}
}

void Client::Handle_OP_RealEstateStoreAll(const EQApplicationPacket* app){
 if(!NativeFurniture(this))return;
 bool durable_success=false;
 try{
  // Original6917 has no payload. Resolve the actual occupied location, never
  // an arbitrary estate ID from a cached list or the other container's tab.
  if(!app||app->size){FurnitureResult(this,1);return;}
  auto* bridge=GetHousingInventoryBridge();auto view=LoadHousingView(this);
  if(!bridge||!view){FurnitureResult(this,23);return;}
  if(!m_housing_placement_session)m_housing_placement_session=std::make_shared<Session>();
  auto& session=*m_housing_placement_session;
  // Finish an uncertain durable operation before choosing a new member set.
  if(session.bulk_storage_request){auto result=bridge->StowAll(*session.bulk_storage_request);if(!result){FurnitureResult(this,1);return;}session.bulk_storage_request.reset();durable_success=true;FurnitureResult(this,0);FurnitureBroadcastScenes();return;}
  Housing::InteriorStorage::StowRequest request;request.request_key=FurnitureKey("stow-all:");
  if(view->interior){
   auto read=bridge->ReadInterior();if(!read||!read.value.house||!read.value.room_profile){FurnitureResult(this,23);return;}
   const auto& room=read.value;request.room={room.house->house_id,room.house->revision,room.profile_revision,0,room.context.source};
   for(const auto& entry:room.contents)if(entry.stored.placement&&!entry.stored.stowed)request.items.push_back({entry.stored.custody_id,entry.stored.placement->pose_revision,entry.stored.storage_revision,entry.scope_revision,entry.stored.placement->reservation.native_item_id});
  }else{
   // The plot must contain the live player, not just appear in the Items list.
   std::optional<Housing::Scene::Snapshot> selected;
   for(const auto& node:view->data.catalog.nodes){
    if(!node.ownership_estate_id||!node.native_id.value)continue;
    auto definition=std::find_if(view->data.catalog.definitions.begin(),view->data.catalog.definitions.end(),[&](const auto& d){return d.id==node.definition_id;});
    if(definition==view->data.catalog.definitions.end()||(definition->type!=4&&definition->type!=8))continue;
    auto region=std::find_if(view->data.catalog.regions.begin(),view->data.catalog.regions.end(),[&](const auto& r){return r.id==definition->area_id;});
    if(region==view->data.catalog.regions.end())continue;
    if(node.neighborhood_id!=view->neighborhood||node.parent_key.empty()||view->ids.at(node.parent_key)!=view->root||!FurniturePlayerInRegion(this,*region))continue;
    auto read=bridge->ReadScene(node.ownership_estate_id);if(!read)continue;
    if(selected){FurnitureResult(this,23);return;}selected=std::move(read.value);
   }
   if(!selected){FurnitureResult(this,23);return;}
   request.exterior=Housing::InteriorStorage::ExteriorContext{selected->estate_id,selected->native_plot_id,selected->current_catalog_revision,selected->route};
   // Estate containers retain their bindings and are not ordinary furniture.
   // Packing/removing houses and crates uses their separate custody operations.
   for(const auto& entry:selected->items)if(!entry.house&&!entry.crate&&entry.stored.placement&&!entry.stored.stowed)request.items.push_back({entry.stored.custody_id,entry.stored.placement->pose_revision,entry.stored.storage_revision,0,entry.stored.placement->reservation.native_item_id});
  }
  if(request.items.empty()){FurnitureResult(this,11);return;}
  session.bulk_storage_request=std::move(request);
  auto result=bridge->StowAll(*session.bulk_storage_request);if(!result){if(result.error!=Housing::LiveItems::Error::CommitUncertain)session.bulk_storage_request.reset();FurnitureResult(this,1);return;}
  session.bulk_storage_request.reset();durable_success=true;FurnitureResult(this,0);FurnitureBroadcastScenes();
 }catch(...){FurnitureResult(this,durable_success?0:1);}
}

void Client::Handle_OP_RealEstateLoadLayout(const EQApplicationPacket* app){
 if(!NativeFurniture(this))return;
 bool durable_success=false;
 auto finish=[&](const char* failure=nullptr){
  // The client prints its local layout message BEFORE sending and sets busy
  // afterward. Shared native1228 DWORD0 releases that state; no echoed layout
  // ACK or new success packet is invented. Explain refusal separately.
  if(failure)Message(Chat::Red,"%s",failure);
  FurnitureResult(this,0);
 };
 try{
  auto load=app?Housing::LayoutWire::DecodeLoad(app->pBuffer,app->size):std::nullopt;
  if(!load){finish("The housing layout request is invalid.");return;}
  // Bounded decoded diagnostics distinguish real producer cases from guessed
  // crate/storage semantics without logging lease capabilities or inventory.
  for(size_t index=0;index<std::min<size_t>(load->operations.size(),16);++index){
   if(const auto* p=std::get_if<Housing::LayoutWire::PlacementOperation>(&load->operations[index]))LogInfo("Native layout member actor [{}] ordinal [{}] tag [1] source [{}:{}] target [{}] pose [{} {} {} {} {} {} {}]",CharacterID(),index,p->source_estate_id,p->native_item_id,p->target_estate_id,p->heading_pitch_roll_scale_x_y_z[0],p->heading_pitch_roll_scale_x_y_z[1],p->heading_pitch_roll_scale_x_y_z[2],p->heading_pitch_roll_scale_x_y_z[3],p->heading_pitch_roll_scale_x_y_z[4],p->heading_pitch_roll_scale_x_y_z[5],p->heading_pitch_roll_scale_x_y_z[6]);
   else{const auto& stored=std::get<Housing::LayoutWire::ItemOperation>(load->operations[index]);LogInfo("Native layout member actor [{}] ordinal [{}] tag [0] source [{}:{}] quantity [{}] target [{}]",CharacterID(),index,stored.source_estate_id,stored.native_item_id,stored.quantity,stored.target_estate_id);}
  }
  auto* bridge=GetHousingInventoryBridge();auto view=LoadHousingView(this);
  if(!bridge||!view||!m_housing_placement_session){finish("Housing layout information is unavailable.");return;}
  auto& session=*m_housing_placement_session;
  if(session.geometry_preview_only||(session.pending&&!session.pending->completed)||(session.move&&!session.move->completed)){
   finish("Finish or cancel the current item placement before loading a layout.");return;
  }
  Housing::LiveItems::Result<Housing::Scene::Snapshot> yard;
  if(view->interior)yard=bridge->ReadParentScene();
  else{
   auto node=std::find_if(view->data.catalog.nodes.begin(),view->data.catalog.nodes.end(),[&](const auto& n){return n.native_id.value==load->yard_estate_id&&n.neighborhood_id==view->neighborhood&&n.ownership_estate_id&&n.parent_key.size()&&view->ids.at(n.parent_key)==view->root;});
   if(node==view->data.catalog.nodes.end()){finish("The layout does not belong to this neighborhood.");return;}
   yard=bridge->ReadScene(node->ownership_estate_id);
  }
  if(!yard||yard.value.native_plot_id!=load->yard_estate_id){finish("The layout property is unavailable.");return;}
  std::optional<Housing::Scene::InteriorSnapshot> room;
  if(view->interior)room=*view->interior;
  else if(Housing::LayoutWire::ContextHouseEstateId(*load)){
   // Resolve the requested house ONLY among freshly authorized, actually placed
   // buildings on this yard. A client-supplied estate ID cannot attach a room.
   for(const auto& item:yard.value.items)if(item.house&&item.house->interior&&!item.stored.stowed&&item.house->interior->native_estate_id==load->house_estate_id){
    if(room){finish("The layout house is ambiguous.");return;}
    auto fresh=bridge->ReadAttachedInterior(yard.value.estate_id,item.house->house_id);
    if(!fresh){finish("The layout house is unavailable.");return;}room=std::move(fresh.value);
   }
  }
  const Housing::Route physical{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
  std::string refusal;
  auto prepared=Housing::LayoutNative::Prepare(*load,physical,bool(view->interior),yard.value,room?&*room:nullptr,session.presentations,&refusal);
  if(!prepared){LogInfo("Native layout scene rejected actor [{}] yard [{}] house [{}] operations [{}] reason [{}]",CharacterID(),load->yard_estate_id,load->house_estate_id,load->operations.size(),refusal);finish("The layout items have changed or are not supported yet. Refresh the property and try again.");return;}
  if(load->operations.empty()){finish();return;}
  const bool restores_house=std::any_of(load->operations.begin(),load->operations.end(),[&](const auto& operation){
   const auto* placement=std::get_if<Housing::LayoutWire::PlacementOperation>(&operation);if(!placement)return false;
   return std::any_of(yard.value.items.begin(),yard.value.items.end(),[&](const auto& item){return item.house&&item.stored.stowed&&item.stored.placement&&placement->source_estate_id==yard.value.native_plot_id&&placement->target_estate_id==yard.value.native_plot_id&&placement->native_item_id==item.stored.placement->reservation.native_item_id;});
  });
  Bytes wire(app->pBuffer,app->pBuffer+app->size);
  auto result=bridge->LoadLayoutMixed(FurnitureKey("layout:"),wire,prepared.value.intent,prepared.value.owners,true);
  if(!result){LogInfo("Native layout rejected actor [{}] yard [{}] operations [{}] code [{}]",CharacterID(),load->yard_estate_id,load->operations.size(),int(result.error));finish("The housing layout could not be loaded. No layout changes were committed.");return;}
  durable_success=true;
  // Owner slots are held through Bridge's known durable ACK. Retired owner
  // nodes may now be removed; old actor GUIDs remain in sent_worlds for removal.
  for(auto i=session.presentations.begin();i!=session.presentations.end();)if(!i->second)i=session.presentations.erase(i);else ++i;
  // Retained inventory tokens also create live house/crate estate links.
  // Publish those links before their item lists so the original client can
  // select a newly placed crate and enable Unpack without re-entering the zone.
  const bool containers_changed=restores_house||!result.value.outcome.retained_tokens.empty();
  FurnitureBroadcastScenes(containers_changed);finish();
 }catch(...){finish(durable_success?nullptr:"The housing layout could not be loaded.");}
}

void Client::Handle_OP_RealEstateItemOperation(const EQApplicationPacket* app){
 if(!NativeFurniture(this))return;
 bool durable_success=false;
 try{
  // Native3424 is Source44 plus destination actionDWORD44.
  // Closet2 and Shed3 select the destination supertype; Crate6 has separate routing.
  if(!app||app->size!=48||(Housing::NeighborhoodWire::Detail::Read32(app->pBuffer+44)!=2&&Housing::NeighborhoodWire::Detail::Read32(app->pBuffer+44)!=3&&Housing::NeighborhoodWire::Detail::Read32(app->pBuffer+44)!=6)){FurnitureResult(this,1);return;}
  const auto source=Housing::PlacementWire::Detail::ReadSource(app->pBuffer);
  const auto action=Housing::NeighborhoodWire::Detail::Read32(app->pBuffer+44);
  auto* bridge=GetHousingInventoryBridge();if(!bridge){FurnitureResult(this,1);return;}
  if(!m_housing_placement_session)m_housing_placement_session=std::make_shared<Session>();
  auto& session=*m_housing_placement_session;Bytes incoming(app->pBuffer,app->pBuffer+app->size);
  if(FurnitureVaultOperation(this,session,*bridge,app,source,action))return;
  if(source.location[0]==0){
   const auto slot=uint16_t(source.location[1]);
   if(((slot<23||slot>32)&&slot!=33)||uint16_t(source.location[1]>>16)!=0xffff||uint16_t(source.location[2])!=0xffff||source.estate_id!=UINT32_MAX||source.item_id!=UINT32_MAX){FurnitureResult(this,9);return;}
   auto* item=GetInv().GetItem(int16_t(slot));
   auto prior=session.inventory_storage_requests.find(incoming);
   // A returned item may legitimately produce the same body again. Its actual
   // presence creates a new operation; a duplicate after removal reuses receipt.
   if(prior!=session.inventory_storage_requests.end()&&prior->second.completed&&item){session.inventory_storage_requests.erase(prior);prior=session.inventory_storage_requests.end();}
   if(prior==session.inventory_storage_requests.end()){
    if(!item||(!KnownFurniture(*item)&&!((action==6||(action==3&&RuleB(Housing,EnableNativeHouseSwap)))&&Housing::HousePlacement::Model(*item)))||!SourceGuid(source,*item)||(slot==33&&GetInv().CursorSize()!=1)){FurnitureResult(this,9);return;}
    auto row=FurnitureInventory(this,slot);if(!row||!Housing::InventoryBridge::MatchesProjection(*row,*item)){FurnitureResult(this,9);return;}
    const auto quantity=Housing::StackPartitionApplication::Requested(*item,source.opaque);if(quantity&&((action!=2&&action!=3&&action!=6)||item->GetHousingHouseID()||item->GetHousingCrateID())){FurnitureResult(this,9);return;}
    if(session.pending&&!session.pending->completed&&!bridge->CancelPlacement(session.pending->reservation.native_item_id)){FurnitureResult(this,1);return;}session.pending.reset();session.move.reset();
    if(session.inventory_storage_requests.size()>=64){auto old=std::find_if(session.inventory_storage_requests.begin(),session.inventory_storage_requests.end(),[](const auto& entry){return entry.second.completed;});if(old==session.inventory_storage_requests.end()){FurnitureResult(this,1);return;}session.inventory_storage_requests.erase(old);}
    auto view=LoadHousingView(this);auto target=view?FurniturePlot(this,*view,*item):std::nullopt;
    if(target&&action==2&&!target->room){
     const Housing::Scene::InteriorSnapshot* selected=nullptr;bool ambiguous=false;for(const auto& entry:session.attached_rooms)if(entry.second.context.native_plot_id==target->node.native_id.value){if(selected)ambiguous=true;selected=&entry.second;}
     if(selected&&!ambiguous){auto fresh=bridge->ReadAttachedInterior(target->node.ownership_estate_id,selected->house->house_id);if(fresh)target=FurniturePlot(this,*view,*item,fresh.value.house->interior->native_estate_id,&fresh.value);else target.reset();}else target.reset();
    }
    if(!target||(action!=6&&(action==2)!=bool(target->room)) ){FurnitureResult(this,23);return;}
    FurniturePending pending;pending.row=*row;pending.serial=item->GetSerialNumber();pending.source=source;pending.reserve_key=FurnitureKey("store-preview:");pending.store_key=FurnitureKey("inventory-store:");
     if(Housing::HousePlacement::Model(*item)){
      if(item->GetHousingHouseID()){auto bound=bridge->ReadInventoryHouse(*row);if(!bound){FurnitureResult(this,9);return;}pending.house_before=bound.value;pending.house_identity=Housing::LiveItems::HouseIdentity{bound.value->house_id,bound.value->revision};}
      else {if(item->GetHousingEstateID()!=UINT32_MAX){FurnitureResult(this,9);return;}pending.house_identity=Housing::LiveItems::HouseIdentity{0,0};}
     }
    if(action==6){
     Housing::LiveItems::Result<Housing::Scene::Snapshot> fresh;
     if(target->room){if(!view->interior){FurnitureResult(this,23);return;}auto read=bridge->ReadInterior();if(!read){FurnitureResult(this,23);return;}auto projected=FurnitureRoomCrateSnapshot(read.value);if(!projected){FurnitureResult(this,23);return;}fresh.value=std::move(*projected);}
     else fresh=bridge->ReadScene(target->node.ownership_estate_id);
     if(!fresh||!FurniturePlayerInRegion(this,fresh.value)){FurnitureResult(this,23);return;}
     const Housing::Scene::Item* container=nullptr;for(const auto& entry:fresh.value.items)if(entry.crate&&!entry.stored.stowed){if(container){FurnitureResult(this,16);return;}container=&entry;}
     if(!container&&!target->room&&!view->interior){
      // A property has one placed crate, whether in its yard or inside its
      // attached house. Resolve only through current authorized house rows.
      std::optional<Housing::Scene::InteriorSnapshot> selected;
      for(const auto& entry:fresh.value.items)if(entry.house&&entry.house->interior){
       auto room=bridge->ReadAttachedInterior(fresh.value.estate_id,entry.house->house_id);if(!room||!room.value.exterior_source)continue;
       for(const auto& inside:room.value.contents)if(inside.crate&&!inside.stored.stowed){if(selected){FurnitureResult(this,16);return;}selected=room.value;}
      }
      if(selected){
       auto destination=FurniturePlot(this,*view,*item,selected->house->interior->native_estate_id,&*selected);auto projected=FurnitureRoomCrateSnapshot(*selected);if(!destination||!projected){FurnitureResult(this,23);return;}target=std::move(destination);fresh.value=std::move(*projected);
       for(const auto& entry:fresh.value.items)if(entry.crate&&!entry.stored.stowed){if(container){FurnitureResult(this,16);return;}container=&entry;}
      }
     }
     if(!container||container->crate->packed){FurnitureResult(this,22);return;}
     pending.crate_member=Housing::LiveItems::CrateMemberContext{container->crate->binding.crate_id,container->crate->binding.revision,fresh.value.native_plot_id,fresh.value.current_catalog_revision,fresh.value.route};
    }
    if(target->room){
     const auto& room=*target->room;Housing::LiveItems::InteriorPreviewRequest request;request.request_key=pending.reserve_key;request.expected=pending.row;request.verified_runtime_serial=pending.serial;request.expected_house_id=room.house->house_id;request.expected_house_revision=room.house->revision;request.expected_profile_revision=room.profile_revision;
     if(room.exterior_source){request.current_source=room.context.source;request.exterior_source=room.exterior_source;request.exterior_estate_id=room.context.estate_id;pending.room_exterior_source=room.exterior_source;}
     auto allocated=bridge->ReserveInteriorPlacement(request,pending.crate_member);if(!allocated){FurnitureResult(this,FurniturePlacementResult(allocated.error));return;}pending.room=allocated.value;pending.reservation=allocated.value.placement;
    }else{
     Housing::LiveItems::PreviewRequest request;request.request_key=pending.reserve_key;request.estate_id=target->node.ownership_estate_id;request.native_estate_id=target->node.native_id.value;request.catalog_key=target->key;request.catalog_revision=target->revision;request.expected=pending.row;request.verified_runtime_serial=pending.serial;request.house_storage_only=bool(pending.house_identity)&&!pending.crate_member;
     auto allocated=pending.crate_member?bridge->ReserveCratePlacement(request,*pending.crate_member):bridge->ReservePlacement(request);if(!allocated){FurnitureResult(this,FurniturePlacementResult(allocated.error));return;}pending.reservation=allocated.value;
    }
    prior=session.inventory_storage_requests.emplace(incoming,std::move(pending)).first;
   }
   auto& pending=prior->second;
   auto deletion=pending.completed?std::unique_ptr<EQApplicationPacket>{}:FurnitureCursorDeletion(slot);
   auto result=bridge->Store(pending.store_key,pending.reservation.estate_id,pending.row,Housing::LiveItems::Placed{pending.reservation,{0,0,0,1,{0,0,0}},1},pending.house_identity,pending.room,true,pending.room_exterior_source,std::nullopt,pending.crate_member,std::nullopt,source.opaque);
   if(!result){FurnitureResult(this,FurniturePlacementResult(result.error));return;}pending.completed=true;if(result.value.partition){if(auto* remainder=GetInv().GetItem(int16_t(slot)))SendItemPacket(int16_t(slot),remainder,ItemPacketTrade);}else if(deletion){auto* packet=deletion.release();FastQueuePacket(&packet);}FurnitureResult(this,0);FurnitureBroadcastScenes(bool(pending.house_identity));return;
  }
  if(!StoredSource(source)){FurnitureResult(this,1);return;}
  if(source.opaque){
   auto shown=session.presentations.find(source.item_id);if(shown==session.presentations.end()||!shown->second||!SourceGuid(source,*shown->second)){FurnitureResult(this,9);return;}
   if(Housing::StackPartitionApplication::Requested(*shown->second,source.opaque)){
    // Native3424 count operations use the same authoritative mixed coordinator
    // in the yard and occupied/attached room. Real scenes retain both realms;
    // no packet estate ID supplies a house or permission witness.
    auto view=LoadHousingView(this);if(!view){FurnitureResult(this,23);return;}
    std::optional<Housing::Scene::InteriorSnapshot> room;Housing::LiveItems::Result<Housing::Scene::Snapshot> yard;
    if(view->interior){auto inside=bridge->ReadInterior();if(!inside){FurnitureResult(this,23);return;}room=std::move(inside.value);yard=bridge->ReadParentScene();}
    else{
     auto child=session.attached_crates.find(source.estate_id);const auto source_realm=child==session.attached_crates.end()?source.estate_id:child->second.parent_native_estate_id;
     auto target=FurniturePlot(this,*view,*shown->second,source_realm);if(!target){FurnitureResult(this,23);return;}
     yard=bridge->ReadScene(target->node.ownership_estate_id);
     if(target->room){auto fresh=bridge->ReadAttachedInterior(target->node.ownership_estate_id,target->room->house->house_id);if(!fresh){FurnitureResult(this,23);return;}room=std::move(fresh.value);}
     if(action==2&&!room){const Housing::Scene::InteriorSnapshot* selected=nullptr;for(const auto& entry:session.attached_rooms)if(entry.second.context.native_plot_id==target->node.native_id.value){if(selected){FurnitureResult(this,16);return;}selected=&entry.second;}if(!selected){FurnitureResult(this,23);return;}auto fresh=bridge->ReadAttachedInterior(target->node.ownership_estate_id,selected->house->house_id);if(!fresh){FurnitureResult(this,23);return;}room=std::move(fresh.value);}
    }
    if(!yard||(action!=2&&action!=3&&action!=6)||(action==2&&!room)){FurnitureResult(this,23);return;}
    uint32_t target_native=action==2?room->house->interior->native_estate_id:yard.value.native_plot_id;
    if(action==6){
     if(!room&&!view->interior){for(const auto& entry:yard.value.items)if(entry.house&&entry.house->interior){auto fresh=bridge->ReadAttachedInterior(yard.value.estate_id,entry.house->house_id);if(!fresh)continue;bool contains=std::any_of(fresh.value.contents.begin(),fresh.value.contents.end(),[](const auto& i){return i.crate&&!i.stored.stowed&&!i.crate->packed;});if(contains){if(room){FurnitureResult(this,16);return;}room=std::move(fresh.value);}}}
     const Housing::CrateBinding::Allocation* destination=nullptr;auto select=[&](const auto& rows){for(const auto& i:rows)if(i.crate&&!i.stored.stowed&&!i.crate->packed){if(destination)throw std::invalid_argument("Ambiguous count destination crate");destination=i.crate.get();}};select(yard.value.items);if(room)select(room->contents);if(!destination){FurnitureResult(this,22);return;}target_native=destination->binding.native_estate_id;
    }
    const Housing::LiveItems::Stored* actual=nullptr;bool contained=false;uint32_t actual_source=0;
    auto bind=[&](const Housing::LiveItems::Stored& item,uint32_t estate,bool member){if(!item.placement||item.placement->reservation.native_item_id!=source.item_id)return;if(actual)throw std::invalid_argument("Ambiguous native count source");actual=&item;actual_source=estate;contained=member;};
    for(const auto& entry:yard.value.items)if(entry.stored.placement&&!entry.house&&!entry.crate)bind(entry.stored,entry.stored.placement->reservation.native_estate_id,false);
    if(room)for(const auto& entry:room->contents)if(entry.stored.placement&&!entry.crate)bind(entry.stored,entry.stored.placement->reservation.native_estate_id,false);
    auto members=[&](const auto& rows,const auto& containers){for(const auto& entry:rows){if(!entry.stored.placement||entry.stored.placement->reservation.native_item_id!=source.item_id)continue;auto container=std::find_if(containers.begin(),containers.end(),[&](const auto& c){return c.crate&&c.crate->binding.crate_id==entry.contained_in_crate;});if(container==containers.end())throw std::invalid_argument("Missing native count container");auto proof=Housing::LayoutCrateMember::FromSnapshot(entry.stored,entry.crate_membership,container->crate);bind(entry.stored,Housing::LayoutCrateMember::SourceEstate(*proof),true);}};
    members(yard.value.crate_contents,yard.value.items);if(room)members(room->crate_contents,room->contents);
    if(!actual||(!contained&&actual->row.values.character_id!=CharacterID())||actual_source!=source.estate_id||!Housing::InventoryBridge::MatchesProjection(actual->row,*shown->second)){FurnitureResult(this,9);return;}
    Housing::LayoutWire::Load count;count.yard_estate_id=yard.value.native_plot_id;if(room)count.house_estate_id=room->house->interior->native_estate_id;count.operations.emplace_back(Housing::LayoutWire::ItemOperation{source.estate_id,source.item_id,source.opaque,target_native});
    const Housing::Route physical{zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};
    auto prepared=Housing::LayoutNative::PrepareStorageCount(count,physical,bool(view->interior),yard.value,room?&*room:nullptr,session.presentations,nullptr,action==6);if(!prepared){FurnitureResult(this,9);return;}
    Bytes wire;Housing::CustodyCodec::W(wire,count.yard_estate_id,4);Housing::CustodyCodec::W(wire,count.house_estate_id,4);Housing::CustodyCodec::W(wire,1,2);Housing::CustodyCodec::W(wire,0,1);for(auto v:{source.estate_id,source.item_id,source.opaque,target_native})Housing::CustodyCodec::W(wire,v,4);
    auto result=bridge->LoadLayoutMixed(FurnitureKey("layout:storage-count:"),wire,prepared.value.intent,prepared.value.owners,true);if(!result){FurnitureResult(this,1);return;}
    durable_success=true;for(auto i=session.presentations.begin();i!=session.presentations.end();)if(!i->second)i=session.presentations.erase(i);else ++i;
    FurnitureResult(this,0);FurnitureBroadcastScenes();return;
   }
  }
  auto view=LoadHousingView(this);std::optional<Housing::Scene::InteriorSnapshot> yard_room;
  auto crate_source=session.attached_crates.find(source.estate_id);
  if(action==6||(action==3&&crate_source!=session.attached_crates.end())){
   if(!view){FurnitureResult(this,23);return;}
   auto prior=session.crate_transfers.find(incoming);auto shown=session.presentations.find(source.item_id);
   std::optional<Housing::Scene::Snapshot> yard;std::optional<Housing::Scene::InteriorSnapshot> room_source;
   if(view->interior){auto fresh=bridge->ReadInterior();if(fresh)room_source=std::move(fresh.value);}
   else{
    auto cached=session.attached_crate_rooms.find(source.estate_id);
    if(cached!=session.attached_crate_rooms.end()){auto fresh=bridge->ReadAttachedInterior(cached->second.context.estate_id,cached->second.house->house_id);if(fresh)room_source=std::move(fresh.value);}
    else if(action==6){auto room=session.attached_rooms.find(source.estate_id);if(room!=session.attached_rooms.end()){auto fresh=bridge->ReadAttachedInterior(room->second.context.estate_id,room->second.house->house_id);if(fresh)room_source=std::move(fresh.value);}}
   }
   if(room_source)yard=FurnitureRoomCrateSnapshot(*room_source);
   else if(action==6){
    // Houses use the same fully reviewed current yard and crate transaction.
    // Recognize their native selection here; later custody/ACL/room checks remain mandatory.
    if(shown==session.presentations.end()||!shown->second||(!KnownFurniture(*shown->second)&&!Housing::HousePlacement::Model(*shown->second))||!SourceGuid(source,*shown->second)){FurnitureResult(this,9);return;}
    auto target=FurniturePlot(this,*view,*shown->second,source.estate_id);if(!target||target->room){FurnitureResult(this,23);return;}
    auto read=bridge->ReadScene(target->node.ownership_estate_id);if(read)yard=std::move(read.value);
   }else{auto read=bridge->ReadScene(crate_source->second.estate_id);if(read)yard=std::move(read.value);}
   if(!yard){FurnitureResult(this,1);return;}
   const auto& entries=action==6?yard->items:yard->crate_contents;
   auto member=std::find_if(entries.begin(),entries.end(),[&](const auto& item){return item.stored.placement&&item.stored.placement->reservation.native_item_id==source.item_id&&!item.crate;});
   if(prior!=session.crate_transfers.end()&&prior->second.completed&&member!=entries.end()){session.crate_transfers.erase(prior);prior=session.crate_transfers.end();}
   if(prior==session.crate_transfers.end()){
    if(member==entries.end()||shown==session.presentations.end()||!shown->second||!SourceGuid(source,*shown->second)||!Housing::InventoryBridge::MatchesProjection(member->stored.row,*shown->second)){FurnitureResult(this,9);return;}
    const Housing::Scene::Item* container=nullptr;
    for(const auto& item:yard->items)if(item.crate&&!item.stored.stowed&&(action==6||item.crate->binding.crate_id==member->contained_in_crate)){if(container){FurnitureResult(this,16);return;}container=&item;}
    if(!container||container->crate->packed||(action==6?source.estate_id!=yard->native_plot_id:source.estate_id!=container->crate->binding.native_estate_id)){FurnitureResult(this,22);return;}
    if(session.crate_transfers.size()>=64){auto old=std::find_if(session.crate_transfers.begin(),session.crate_transfers.end(),[](const auto& item){return item.second.completed;});if(old==session.crate_transfers.end()){FurnitureResult(this,1);return;}session.crate_transfers.erase(old);}
    Housing::CrateStorage::Request request;request.request_key=FurnitureKey("crate-member:");request.estate_id=yard->estate_id;request.native_plot_id=yard->native_plot_id;request.catalog_revision=yard->current_catalog_revision;request.source=yard->route;request.crate_id=container->crate->binding.crate_id;request.crate_revision=container->crate->binding.revision;request.custody_id=member->stored.custody_id;request.pose_revision=member->stored.placement->pose_revision;request.storage_revision=member->stored.storage_revision;request.to_crate=action==6;request.restore_original=false;
    if(room_source)request.room=Housing::CrateStorage::RoomContext{room_source->house->house_id,room_source->house->revision,room_source->profile_revision,bool(room_source->exterior_source)};
    prior=session.crate_transfers.emplace(incoming,Session::CrateTransfer{std::move(request),false}).first;
   }
   auto result=bridge->TransferCrate(prior->second.request);
   if(!result){LogInfo("Native crate member transfer rejected actor [{}] action [{}] code [{}]",CharacterID(),action,int(result.error));FurnitureResult(this,1);return;}
   durable_success=true;prior->second.completed=true;
   // Escape ends the native preview locally. Moving that item between crate
   // and yard supersedes its old source, including an uncommitted preview.
   // Invalidate only after durable transfer; a refusal keeps its retry state.
   if(session.move&&session.move->before.custody_id==prior->second.request.custody_id)session.move.reset();
   FurnitureResult(this,0);FurnitureBroadcastScenes(true);return;
  }
  if(view&&!view->interior){
   const Housing::Scene::InteriorSnapshot* selected=nullptr;bool ambiguous=false;
   auto same_room=session.attached_rooms.find(source.estate_id);
   if(same_room!=session.attached_rooms.end())selected=&same_room->second;
   else if(action==2)for(const auto& entry:session.attached_rooms)if(entry.second.context.native_plot_id==source.estate_id){if(selected)ambiguous=true;selected=&entry.second;}
   auto shown=session.presentations.find(source.item_id);
   if(selected&&!ambiguous&&shown!=session.presentations.end()&&shown->second&&SourceGuid(source,*shown->second)&&FurniturePlot(this,*view,*shown->second,selected->context.native_plot_id))yard_room=*selected;
  }
  const auto* room_view=view&&view->interior?&*view->interior:(yard_room?&*yard_room:nullptr);
  if(room_view){
   const auto& room=*room_view;
   const auto native_room=room.house&&room.house->interior?room.house->interior->native_estate_id:0;
   const bool crosses=(action==3&&source.estate_id==native_room)||(action==2&&source.estate_id==room.context.native_plot_id);
   if(crosses){
    auto prior=session.transfer_requests.find(incoming);std::unique_ptr<EQ::ItemInstance> presentation;
    auto shown=session.presentations.find(source.item_id);
    if(shown!=session.presentations.end()&&shown->second&&SourceGuid(source,*shown->second))presentation.reset(shown->second->Clone());
    if(prior==session.transfer_requests.end()){
     if(!presentation){FurnitureResult(this,9);return;}
     Housing::InteriorStorage::TransferRequest request;request.request_key=FurnitureKey("transfer:");request.occupied_room={room.house->house_id,room.house->revision,room.profile_revision,0,room.context.source};request.to_room=action==2;request.native_item_id=source.item_id;request.source_native_estate_id=source.estate_id;if(room.exterior_source)request.yard=Housing::InteriorStorage::ExteriorContext{room.context.estate_id,room.context.native_plot_id,room.context.catalog_revision,*room.exterior_source};
     const Housing::LiveItems::Stored* item=nullptr;std::optional<Housing::Scene::Snapshot> parent;
     if(request.to_room){auto read=request.yard?bridge->ReadScene(request.yard->estate_id):bridge->ReadParentScene();if(!read){FurnitureResult(this,1);return;}parent=std::move(read.value);auto found=std::find_if(parent->items.begin(),parent->items.end(),[&](const auto& entry){return entry.stored.placement&&entry.stored.placement->reservation.native_item_id==source.item_id;});if(found!=parent->items.end()&&!found->house)item=&found->stored;}
     else{auto found=std::find_if(room.contents.begin(),room.contents.end(),[&](const auto& entry){return entry.stored.placement&&entry.stored.placement->reservation.native_item_id==source.item_id;});if(found!=room.contents.end()){item=&found->stored;request.occupied_room.scope_revision=found->scope_revision;}}
     if(!item||!Housing::InventoryBridge::MatchesProjection(item->row,*presentation)){FurnitureResult(this,9);return;}
     request.custody_id=item->custody_id;request.expected_pose_revision=item->placement->pose_revision;request.expected_storage_revision=item->storage_revision;
     if(session.transfer_requests.size()>=64)session.transfer_requests.erase(session.transfer_requests.begin());
     prior=session.transfer_requests.emplace(incoming,std::move(request)).first;
    }
    auto result=bridge->TransferStorage(prior->second);if(!result){FurnitureResult(this,1);return;}durable_success=true;
    if(presentation&&!session.presentations.count(result.value.after_native_item_id))session.presentations.emplace(result.value.after_native_item_id,std::move(presentation));
    FurnitureResult(this,0);FurnitureBroadcastScenes();return;
   }
  }

  auto shown=session.presentations.find(source.item_id);if(shown==session.presentations.end()||!shown->second||!SourceGuid(source,*shown->second)){FurnitureResult(this,9);return;}
  if(Housing::NeighborhoodWire::Detail::Read32(app->pBuffer+44)==3){
   auto view=LoadHousingView(this);if(!view){FurnitureResult(this,23);return;}
   Housing::LiveItems::Result<Housing::Scene::Snapshot> read;
   std::optional<Housing::LiveItems::OccupiedParentContext> parent;
   if(view->interior){
    const auto& room=*view->interior;if(!room.house||!room.room_profile||room.context.native_plot_id!=source.estate_id){FurnitureResult(this,23);return;}
    parent=Housing::LiveItems::OccupiedParentContext{room.house->house_id,room.house->revision,room.profile_revision,room.context.source};read=bridge->ReadParentScene();
   }else{auto target=FurniturePlot(this,*view,*shown->second,source.estate_id);if(!target){FurnitureResult(this,23);return;}read=bridge->ReadScene(target->node.ownership_estate_id);}
   if(!read||read.value.native_plot_id!=source.estate_id){FurnitureResult(this,23);return;}
   auto item=std::find_if(read.value.items.begin(),read.value.items.end(),[&](const auto& i){return i.stored.placement&&i.stored.placement->reservation.native_item_id==source.item_id;});
   if(item==read.value.items.end()||!Housing::InventoryBridge::MatchesProjection(item->stored.row,*shown->second)){FurnitureResult(this,9);return;}
   auto prior=session.storage_requests.find(incoming);
   if(prior!=session.storage_requests.end()&&(!prior->second.exterior||prior->second.exterior->catalog_revision!=read.value.current_catalog_revision||prior->second.expected_pose_revision!=item->stored.placement->pose_revision||(!item->stored.stowed&&prior->second.expected_storage_revision!=item->stored.storage_revision))){session.storage_requests.erase(prior);prior=session.storage_requests.end();}
   if(item->stored.stowed&&prior==session.storage_requests.end()){FurnitureResult(this,0);FurnitureBroadcastScenes(bool(item->house));return;}
   if(prior==session.storage_requests.end()){
    Housing::InteriorStorage::Request request;request.request_key=FurnitureKey("shed:");request.parent_yard=parent;request.exterior=Housing::InteriorStorage::ExteriorContext{read.value.estate_id,read.value.native_plot_id,read.value.current_catalog_revision,read.value.route};request.custody_id=item->stored.custody_id;request.native_item_id=source.item_id;request.expected_pose_revision=item->stored.placement->pose_revision;request.expected_storage_revision=item->stored.storage_revision;prior=session.storage_requests.emplace(std::move(incoming),std::move(request)).first;
   }
   auto result=bridge->ChangeStorage(prior->second);if(!result){FurnitureResult(this,1);return;}
   // Native Escape cancels locally; a successful Shed transition supersedes
   // any retained preview for this exact item. Failed writes retain retries.
   if(session.move&&session.move->before.custody_id==item->stored.custody_id)session.move.reset();
   FurnitureResult(this,0);FurnitureBroadcastScenes(bool(item->house));return;
  }
  auto fresh=yard_room?bridge->ReadAttachedInterior(yard_room->context.estate_id,yard_room->house->house_id):bridge->ReadInterior();if(!fresh||!fresh.value.house||!fresh.value.house->interior||fresh.value.house->interior->native_estate_id!=source.estate_id){FurnitureResult(this,23);return;}
  auto item=std::find_if(fresh.value.contents.begin(),fresh.value.contents.end(),[&](const auto& i){return i.stored.placement&&i.stored.placement->reservation.native_item_id==source.item_id;});
  if(item==fresh.value.contents.end()||!Housing::InventoryBridge::MatchesProjection(item->stored.row,*shown->second)){FurnitureResult(this,9);return;}
  auto prior=session.storage_requests.find(incoming);
  if(prior!=session.storage_requests.end()&&(prior->second.room.house_revision!=fresh.value.house->revision||prior->second.room.profile_revision!=fresh.value.profile_revision||prior->second.expected_pose_revision!=item->stored.placement->pose_revision||(!item->stored.stowed&&prior->second.expected_storage_revision!=item->stored.storage_revision))){session.storage_requests.erase(prior);prior=session.storage_requests.end();}
  if(item->stored.stowed&&prior==session.storage_requests.end()){FurnitureResult(this,0);FurnitureBroadcastScenes();return;}
  if(prior==session.storage_requests.end()){
   Housing::InteriorStorage::Request request;request.request_key=FurnitureKey("stow:");request.room={fresh.value.house->house_id,fresh.value.house->revision,fresh.value.profile_revision,item->scope_revision,fresh.value.context.source};request.room_exterior_source=fresh.value.exterior_source;request.custody_id=item->stored.custody_id;request.native_item_id=source.item_id;request.expected_pose_revision=item->stored.placement->pose_revision;request.expected_storage_revision=item->stored.storage_revision;
   prior=session.storage_requests.emplace(std::move(incoming),std::move(request)).first;
  }
  auto changed=bridge->ChangeStorage(prior->second);if(!changed){FurnitureResult(this,1);return;}
  FurnitureResult(this,0);FurnitureBroadcastScenes();
 }catch(const std::exception& error){LogError("Native item storage operation rejected [{}]",error.what());FurnitureResult(this,durable_success?0:1);}catch(...){FurnitureResult(this,durable_success?0:1);}
}

void Client::Handle_OP_RealEstatePickup(const EQApplicationPacket* app){
 if(!NativeFurniture(this))return;
 bool durable_success=false;
 try{
  // Exact2036 constructor542440: GUID18/pad2, packedsource12,IDs8,quantity4.
  if(app->size!=44||!m_housing_placement_session){FurnitureResult(this,1);return;}
  auto request=Housing::PlacementWire::Detail::ReadSource(app->pBuffer);
  if(FurnitureVaultPickup(this,*m_housing_placement_session,request,Bytes(app->pBuffer,app->pBuffer+app->size)))return;
  // Quantity0 is legacy whole; positive whole and strict partial are checked against the actual current stack below.
  const auto kind=request.location[0];
  if(kind!=0xffffffff&&kind!=14&&!(kind==0&&CursorSource(request))){FurnitureResult(this,1);return;}
  auto& session=*m_housing_placement_session;const Bytes incoming(app->pBuffer,app->pBuffer+app->size);
  auto prior=session.pickups.find(request.item_id);
  if(prior!=session.pickups.end()&&prior->second.completed&&prior->second.partition&&!GetInv().GetItem(EQ::invslot::slotCursor)&&GetInv().CursorSize()==0){session.pickups.erase(prior);prior=session.pickups.end();}
  EQ::ItemInstance* presentation=nullptr;
  if(prior!=session.pickups.end()){
   if(prior->second.request!=incoming||!prior->second.presentation){FurnitureResult(this,1);return;}
   presentation=prior->second.presentation.get();
  }else{
   auto cached=session.presentations.find(request.item_id);if(cached==session.presentations.end()||!cached->second){FurnitureResult(this,9);return;}presentation=cached->second.get();
  }
  if(!KnownPlacedModel(*presentation)||!SourceGuid(request,*presentation)){FurnitureResult(this,9);return;}
  const auto partition=Housing::StackPartitionApplication::Requested(*presentation,request.opaque);
  auto view=LoadHousingView(this);if(!view){FurnitureResult(this,1);return;}
  auto* crate_bridge=GetHousingInventoryBridge();if(!crate_bridge){FurnitureResult(this,1);return;}
  auto crate_source=session.attached_crates.find(request.estate_id);
  if((prior!=session.pickups.end()&&prior->second.crate_member)||crate_source!=session.attached_crates.end()){
   if(prior!=session.pickups.end()&&prior->second.completed){auto* item=GetInv().GetItem(EQ::invslot::slotCursor);if(item&&item->GetSerialNumber()==prior->second.returned_serial)SendItemPacket(EQ::invslot::slotCursor,item,ItemPacketTrade);FurnitureResult(this,0);FurnitureBroadcastScenes();return;}
   if(prior==session.pickups.end()){
    std::optional<Housing::Scene::InteriorSnapshot> room;
    Housing::LiveItems::Result<Housing::Scene::Snapshot> read;
    if(view->interior){auto fresh=crate_bridge->ReadInterior();if(!fresh){FurnitureResult(this,23);return;}room=std::move(fresh.value);auto projected=FurnitureRoomCrateSnapshot(*room);if(!projected){FurnitureResult(this,23);return;}read.value=std::move(*projected);}
    else{
     auto attached=session.attached_crate_rooms.find(request.estate_id);
     if(attached!=session.attached_crate_rooms.end()){
      auto fresh=crate_bridge->ReadAttachedInterior(attached->second.context.estate_id,attached->second.house->house_id);if(!fresh||!fresh.value.exterior_source){FurnitureResult(this,23);return;}
      auto parent=FurniturePlot(this,*view,*presentation,fresh.value.context.native_plot_id);if(!parent||!FurniturePlayerInRegion(this,parent->region)){FurnitureResult(this,23);return;}
      room=std::move(fresh.value);auto projected=FurnitureRoomCrateSnapshot(*room);if(!projected){FurnitureResult(this,23);return;}read.value=std::move(*projected);
     }else read=crate_bridge->ReadScene(crate_source->second.estate_id);
    }
    // Attached-room coordinates belong to another zone; parent-yard proximity
    // was checked above, and the fresh typed read proves the actual host route.
    if(!read||(!(room&&room->exterior_source)&&!FurniturePlayerInRegion(this,read.value))){FurnitureResult(this,23);return;}
    auto member=std::find_if(read.value.crate_contents.begin(),read.value.crate_contents.end(),[&](const auto& entry){return entry.stored.placement&&entry.stored.placement->reservation.native_item_id==request.item_id&&entry.contained_in_crate==crate_source->second.crate_id;});
    auto container=std::find_if(read.value.items.begin(),read.value.items.end(),[&](const auto& entry){return entry.crate&&entry.crate->binding.crate_id==crate_source->second.crate_id&&entry.crate->binding.native_estate_id==request.estate_id&&!entry.stored.stowed;});
    if(member==read.value.crate_contents.end()||container==read.value.items.end()||member->crate||(member->stored.row.values.character_id!=CharacterID()&&(!read.value.access.Allows(Housing::AccessPolicy::Capability::RemoveItem)||!Housing::ForeignFurnitureReturn::Eligible(member->stored.row,member->stored.catalog)||!presentation->IsDroppable()||presentation->GetItem()->NoTransfer))||!Housing::InventoryBridge::MatchesProjection(member->stored.row,*presentation)){FurnitureResult(this,9);return;}
    if(!FurnitureTrimPickupHistory(session)){FurnitureResult(this,1);return;}
    if(member->house&&!read.value.access.Allows(Housing::AccessPolicy::Capability::RemoveHouse)){FurnitureResult(this,71);return;}
    Session::Pickup pickup;pickup.partition=bool(partition);if(room){auto original=std::find_if(room->crate_contents.begin(),room->crate_contents.end(),[&](const auto& i){return i.stored.custody_id==member->stored.custody_id;});if(original==room->crate_contents.end()||!original->room){FurnitureResult(this,9);return;}pickup.room_item=*original;pickup.room_context=Housing::LiveItems::InteriorMoveContext{room->house->house_id,room->house->revision,room->profile_revision,original->scope_revision,room->context.source};pickup.room_exterior_source=room->exterior_source;}if(member->house)pickup.house_identity=Housing::LiveItems::HouseIdentity{member->house->house_id,member->house->revision};pickup.request=incoming;pickup.key=FurnitureKey("crate-pickup:");pickup.estate=read.value.estate_id;pickup.custody=member->stored.custody_id;pickup.presentation.reset(presentation->Clone());pickup.crate_item=member->stored;pickup.crate_member=Housing::LiveItems::CrateMemberContext{container->crate->binding.crate_id,container->crate->binding.revision,read.value.native_plot_id,read.value.current_catalog_revision,read.value.route};prior=session.pickups.emplace(request.item_id,std::move(pickup)).first;
   }
   if(!prior->second.crate_member||!prior->second.crate_item||GetInv().CursorSize()!=0||GetInv().GetItem(EQ::invslot::slotCursor)){FurnitureResult(this,1);return;}
   auto result=prior->second.room_item&&prior->second.room_context?crate_bridge->ReturnInterior(prior->second.key,*prior->second.room_item,*prior->second.room_context,EQ::invslot::slotCursor,prior->second.room_exterior_source,prior->second.crate_member,request.opaque):crate_bridge->ReturnCrateMember(prior->second.key,*prior->second.crate_item,*prior->second.crate_member,EQ::invslot::slotCursor,prior->second.house_identity,request.opaque);if(!result){FurnitureResult(this,1);return;}
   durable_success=true;prior->second.completed=true;if(!result.value.partition)Housing::ServiceNpcs::Revoke(prior->second.custody);auto* item=GetInv().GetItem(EQ::invslot::slotCursor);prior->second.returned_serial=item?item->GetSerialNumber():0;if(item)SendItemPacket(EQ::invslot::slotCursor,item,ItemPacketTrade);if(!prior->second.partition)session.presentations.erase(request.item_id);FurnitureResult(this,0);FurnitureBroadcastScenes();return;
  }
  const Housing::Scene::InteriorSnapshot* attached=nullptr;
  if(!view->interior){auto found=session.attached_rooms.find(request.estate_id);if(found!=session.attached_rooms.end()&&FurniturePlot(this,*view,*presentation,found->second.context.native_plot_id))attached=&found->second;}
  auto target=FurniturePlot(this,*view,*presentation,request.estate_id,attached);if(!target){FurnitureResult(this,23);return;}
  auto* bridge=GetHousingInventoryBridge();if(!bridge){FurnitureResult(this,1);return;}
  auto current=FurnitureSnapshot(this,*bridge,*target);
  if(prior!=session.pickups.end()&&prior->second.completed){
   durable_success=true;
   // A denied current Enter view cannot hide a known completed Return. It
   // permits no new inventory action and never replays historical world state.
   if(!prior->second.partition&&current&&std::any_of(current->items.begin(),current->items.end(),[&](const auto& entry){return entry.stored.placement&&entry.stored.placement->reservation.native_item_id==request.item_id;})){FurnitureResult(this,1);return;}
   if(!prior->second.partition){Housing::ServiceNpcs::Revoke(prior->second.custody);SendEstatePayload(this,OP_RealEstateWorldRemove,FurnitureRemoval(request.estate_id,request.item_id,FurnitureGuid(*presentation)));
   session.sent_worlds[request.estate_id].erase(request.item_id);}
   // Replays never move custody again or overwrite a later cursor item.
   auto* returned=GetInv().GetItem(EQ::invslot::slotCursor);
   if(returned&&GetInv().CursorSize()==1&&returned->GetSerialNumber()==prior->second.returned_serial)SendItemPacket(EQ::invslot::slotCursor,returned,ItemPacketTrade);
   auto scene=FurnitureCurrentScene(this,session,*bridge,*target);if(scene)FurnitureSendScene(this,session,*scene,{},true);else FurnitureResult(this,0);FurnitureBroadcastScenes();return;
  }
  if(!current){FurnitureResult(this,1);return;}
  auto row=std::find_if(current->items.begin(),current->items.end(),[&](const auto& r){return r.stored.placement&&r.stored.placement->reservation.native_item_id==request.item_id&&r.stored.placement->reservation.native_estate_id==request.estate_id;});
  if(row==current->items.end()||(row->stored.row.values.character_id!=CharacterID()&&(!current->access.Allows(Housing::AccessPolicy::Capability::RemoveItem)||!Housing::ForeignFurnitureReturn::Eligible(row->stored.row,row->stored.catalog)||!presentation->IsDroppable()||presentation->GetItem()->NoTransfer))||!FurnitureCurrentCatalog(row->stored,*target,current->current_catalog_revision)||!Housing::InventoryBridge::MatchesProjection(row->stored.row,*presentation)||!FurniturePresentationMatches(*row,*target,*presentation)){FurnitureResult(this,9);return;}
  if((row->house&&!current->access.Allows(Housing::AccessPolicy::Capability::RemoveHouse))||(row->crate&&!current->access.Allows(Housing::AccessPolicy::Capability::RemoveCrate))){FurnitureResult(this,71);return;}
  const auto distance=FurnitureDistance(this,*presentation,row->stored.placement->transform);
  if(!row->stored.stowed&&!(target->room&&target->room->exterior_source)&&(!distance||*distance>20)){FurnitureResult(this,20);return;} // Coordinates in an attached room are not yard-world distances.
  if(GetInv().CursorSize()!=0||GetInv().GetItem(EQ::invslot::slotCursor)){FurnitureResult(this,1);return;}
  if(prior==session.pickups.end()){
   if(!FurnitureTrimPickupHistory(session)){FurnitureResult(this,1);return;}
   Session::Pickup pickup;pickup.request=incoming;pickup.key=FurnitureKey("pickup:");pickup.estate=target->node.ownership_estate_id;pickup.custody=row->stored.custody_id;if(row->house)pickup.house_identity=Housing::LiveItems::HouseIdentity{row->house->house_id,row->house->revision};if(row->crate)pickup.crate_identity=Housing::LiveItems::CrateIdentity{row->crate->binding.crate_id,row->crate->binding.revision};pickup.presentation.reset(presentation->Clone());
   if(target->room){const auto& room=*target->room;auto item=std::find_if(room.contents.begin(),room.contents.end(),[&](const auto& entry){return entry.stored.custody_id==row->stored.custody_id;});if(item==room.contents.end()||!item->room){FurnitureResult(this,9);return;}pickup.room_item=*item;pickup.room_context=Housing::LiveItems::InteriorMoveContext{room.house->house_id,room.house->revision,room.profile_revision,item->scope_revision,room.context.source};pickup.estate=item->stored.estate_id;pickup.room_exterior_source=room.exterior_source;}
   prior=session.pickups.emplace(request.item_id,std::move(pickup)).first;presentation=prior->second.presentation.get();
  }else if(prior->second.estate!=(target->room?row->stored.estate_id:target->node.ownership_estate_id)||prior->second.custody!=row->stored.custody_id){FurnitureResult(this,9);return;}
  if(bool(prior->second.house_identity)!=bool(row->house)||(row->house&&(prior->second.house_identity->house_id!=row->house->house_id||prior->second.house_identity->expected_revision!=row->house->revision))){FurnitureResult(this,9);return;}
  if(bool(prior->second.crate_identity)!=bool(row->crate)||(row->crate&&(prior->second.crate_identity->crate_id!=row->crate->binding.crate_id||prior->second.crate_identity->expected_revision!=row->crate->binding.revision))){FurnitureResult(this,9);return;}
  // Prepare retirement and ordinary inventory ItemPtr validation before Return.
  const auto removal=FurnitureRemoval(request.estate_id,request.item_id,FurnitureGuid(*presentation));
  if(!RoF2::SerializeHousingItem(*presentation,EQ::invslot::slotCursor)){FurnitureResult(this,1);return;}
  session.sent_worlds.try_emplace(request.estate_id);
  Housing::LiveItems::Result<Housing::LiveItems::Receipt> returned;
  if(target->room){if(!prior->second.room_item||!prior->second.room_context){FurnitureResult(this,9);return;}returned=bridge->ReturnInterior(prior->second.key,*prior->second.room_item,*prior->second.room_context,EQ::invslot::slotCursor,prior->second.room_exterior_source,std::nullopt,request.opaque);}
  else returned=bridge->Return(prior->second.key,prior->second.estate,prior->second.custody,EQ::invslot::slotCursor,prior->second.house_identity,prior->second.crate_identity,request.opaque);
  if(!returned){FurnitureResult(this,1);return;}
  durable_success=true;prior->second.completed=true;prior->second.partition=bool(returned.value.partition);
  if(!prior->second.partition)Housing::ServiceNpcs::Revoke(prior->second.custody);
  auto* inventory_item=GetInv().GetItem(EQ::invslot::slotCursor);
  prior->second.returned_serial=inventory_item?inventory_item->GetSerialNumber():0;
  if(!returned.value.partition){SendEstatePayload(this,OP_RealEstateWorldRemove,removal);session.sent_worlds.at(request.estate_id).erase(request.item_id);session.presentations.erase(request.item_id);}
  if(inventory_item)SendItemPacket(EQ::invslot::slotCursor,inventory_item,ItemPacketTrade);
  auto scene=FurnitureCurrentScene(this,session,*bridge,*target);
  if(scene)FurnitureSendScene(this,session,*scene,{},true);
  else {FurnitureResult(this,0);Message(Chat::Red,"Your furniture was returned; its current plot view is unavailable.");}
  FurnitureBroadcastScenes(bool(prior->second.house_identity||prior->second.crate_identity));
 }catch(const std::exception&){LogError("Native furniture pickup view unavailable");FurnitureResult(this,durable_success?0:1);if(durable_success)FurnitureBroadcastScenes();}
}

// Find is visual navigation only: never membership installation, ACL, or MovePC.
bool HandleHousingPlotFind(Client*c,const EQApplicationPacket* app){
 if(!c||c->ClientVersion()!=EQ::versions::ClientVersion::RoF2||!app||!Housing::PlotFindWire::IsPlot(app->pBuffer,app->size))return false;
 if(!RuleB(Housing,EnablePlayerHousing)||!c->Connected()||!c->ClientDataLoaded())return true;
 try{
  const auto request=Housing::PlotFindWire::Decode(app->pBuffer,app->size);if(!request){c->SendPath(nullptr);return true;}
  auto db=HousingConnection();if(!db){c->SendPath(nullptr);return true;}auto entries=Housing::GlobalMyPlots::ReadEntries(*db,{c->CharacterID(),c->AccountID()});
  if(std::none_of(entries.begin(),entries.end(),[&](const auto& x){return x.row.estate_id==request->estate_id;})){
   Housing::NeighborhoodWire::SearchRequest items;items.mode=1;
   auto current=Housing::NeighborhoodDirectory::Read(*db,items,c->CharacterID(),false,c->AccountID());
   for(const auto& row:current.item_plots){const auto& destination=current.item_destinations.at(row.estate_id);entries.push_back({row,destination.first,destination.second});}
  }
  auto selected=std::find_if(entries.begin(),entries.end(),[&](const auto&x){return x.row.estate_id==request->estate_id;});
  if(selected==entries.end()||!zone||selected->route.zone_id!=zone->GetZoneID()||selected->route.zone_version!=zone->GetInstanceVersion()||selected->route.instance_id!=zone->GetInstanceID()){
   c->SendPath(nullptr);return true;
  }
  // Technical guidance target: actual zone-map contact at the plot-region
  // center, then a complete existing navmesh path. This is no claim of the
  // original server's chosen endpoint. No guessed floor/door offset or client
  // position grants destination authority.
  if(!RuleB(Pathing,Find)||!zone->pathing||!zone->HasMap()||!zone->zonemap){c->SendPath(nullptr);return true;}
  const auto& region=selected->region;const glm::vec3 start{c->GetX(),c->GetY(),c->GetZ()};glm::vec3 query{region.matrix[13],region.matrix[12],region.matrix[14]},contact{0,0,0};
  const float vertical=std::abs(region.matrix[2])+std::abs(region.matrix[6])+std::abs(region.matrix[10]);
  glm::vec3 top=query,bottom=query;top.z+=vertical;bottom.z-=vertical;
  if(!std::isfinite(vertical)||vertical<=0||!zone->zonemap->LineIntersectsZone(top,bottom,1.0f,&contact)){c->SendPath(nullptr);return true;}
  std::array<float,3> map_point{contact.x,contact.y,contact.z};
  if(!Housing::ExteriorExit::Inside(region,{map_point,map_point})){c->SendPath(nullptr);return true;}query=contact;
  bool partial=false,stuck=false;auto route=zone->pathing->FindRoute(start,query,partial,stuck,PathingNotDisabled&~(PathingZoneLine|PathingPortal));
  if(route.empty()||partial||stuck||route.size()>1000){c->SendPath(nullptr);return true;}
  for(const auto& node:route)if(node.teleport||!std::isfinite(node.pos.x)||!std::isfinite(node.pos.y)||!std::isfinite(node.pos.z)){c->SendPath(nullptr);return true;}
  const auto end=route.back().pos;std::array<float,3> endpoint{end.x,end.y,end.z};
  if(!Housing::ExteriorExit::Inside(region,{endpoint,endpoint})){c->SendPath(nullptr);return true;}
  std::vector<FindPerson_Point> points;auto append=[&](const glm::vec3&v){FindPerson_Point p;p.x=v.x;p.y=v.y;p.z=v.z;points.push_back(p);};append(end);append(start);for(const auto&node:route)append(node.pos);
  c->SendPathPacket(points);
 }catch(const std::exception&){LogError("Housing plot navigation unavailable");c->SendPath(nullptr);}
 return true;
}

// Each request re-reads durable custody, lease/catalogue/ACL and rent through
// the ordinary furniture bridge. Runtime spawn IDs confer no item authority.
bool Client::ValidateHousingPet(const Housing::Pets::Binding& expected,bool command){
 try{
  if(!NativeFurniture(this)||!Connected()||!ClientDataLoaded()||!HasInventorySessionReservation()||!m_housing_placement_session)return false;
  auto& session=*m_housing_placement_session;auto cached=session.presentations.find(expected.native_item_id);if(cached==session.presentations.end()||!cached->second)return false;
  auto view=LoadHousingView(this);auto* bridge=GetHousingInventoryBridge();if(!view||!bridge)return false;auto target=FurniturePlot(this,*view,*cached->second,expected.native_estate_id);if(!target)return false;
  auto fresh=FurnitureSnapshot(this,*bridge,*target);if(!fresh||fresh->current_catalog_revision!=expected.catalog_revision)return false;
  auto regions=fresh->additional_regions;regions.insert(regions.begin(),fresh->region);if(regions.size()!=expected.regions.size())return false;for(size_t i=0;i<regions.size();++i)if(regions[i].id!=expected.regions[i].id||regions[i].matrix!=expected.regions[i].matrix)return false;
  const auto rent=Housing::RentState::Plot(database,fresh->estate_id,fresh->native_plot_id,false);if(!rent.Active())return false;
  for(const auto& entry:fresh->items){const auto& row=entry.stored;if(row.custody_id!=expected.custody_id||!row.placement||row.stowed||entry.contained_in_crate||row.estate_id!=expected.estate_id||row.row.values.character_id!=expected.owner||row.placement->pose_revision!=expected.pose_revision||row.placement->reservation.native_estate_id!=expected.native_estate_id||row.placement->reservation.native_item_id!=expected.native_item_id)continue;
   if(!FurnitureCurrentCatalog(row,*target,fresh->current_catalog_revision)||!Housing::InventoryBridge::MatchesProjection(row.row,*cached->second)||!FurnitureSameTransform(row.placement->transform,expected.pose))return false;
   auto p=FurnitureNpc(*cached->second);if(!p||!Housing::Pets::Supported(*p)||!Housing::NpcRuntime::Same(*p,expected.profile))return false;
   return !command||fresh->access.Allows(Housing::AccessPolicy::Capability::InteractItem);
  }return false;
 }catch(...){return false;}
}
void Client::Handle_OP_RealEstateCustomInteraction(const EQApplicationPacket* app){
 if(!app||!NativeFurniture(this)||!Connected()||!ClientDataLoaded()||!HasInventorySessionReservation())return;
 auto request=Housing::PetCommands::Decode(app->pBuffer,app->size);if(!request){FurnitureResult(this,1);return;}
 if(!Housing::Pets::Command(*this,*request)){FurnitureResult(this,23);return;}FurnitureResult(this,0);
}
