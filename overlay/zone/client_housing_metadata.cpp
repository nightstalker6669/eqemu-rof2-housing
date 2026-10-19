#include "client.h"
#include "zone.h"
#include "entity.h"
#include "housing_inventory_bridge.h"
#include "../common/eqemu_config.h"
#include "../common/rulesys.h"
#include "../common/housing/permission_wire.h"
#include "../common/housing/known_original_interior_zones.h"
#include <openssl/rand.h>
#include <cstring>

extern Zone* zone;
bool SendHousingVotingPreferenceSnapshot(Client*,uint32_t);
namespace {
bool NativeMetadata(Client* client){
 return RuleB(Housing,EnablePlayerHousing)&&client->ClientVersion()==EQ::versions::ClientVersion::RoF2&&client->Connected()&&client->ClientDataLoaded()&&client->HasInventorySessionReservation()&&zone&&zone->GetInstanceID()!=0&&(zone->GetZoneID()==712||Housing::OriginalInteriors::Matches(zone->GetZoneID(),zone->GetShortName()));
}
std::optional<Housing::EstateId> MetadataEstate(Client* client,uint32_t& native,bool allow_interior=false){
 if(!native||native==UINT32_MAX)return {};
 if(zone->GetZoneID()!=712){
  if(!allow_interior)return {};
  auto* bridge=client->GetHousingInventoryBridge();if(!bridge)return {};
  auto room=bridge->ReadInteriorForManagement();
  if(!room||!room.value.house||!room.value.house->interior||
     (native!=room.value.context.native_plot_id&&native!=room.value.house->interior->native_estate_id))return {};
  // Candidate identity only. The access service rechecks this exact occupied
  // parent inside its own transaction under the held live Client fence.
  native=room.value.context.native_plot_id;return room.value.context.estate_id;
 }
 const auto* config=EQEmuConfig::get();if(!config)return {};
 Database db;if(!db.Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-access-route"))return {};
 // This lookup supplies identity only. The service independently locks and
 // validates current route, catalog, estate ownership, membership and Manage.
 auto rows=db.QueryDatabase("SELECT e.ownership_estate_id FROM housing_native_estates e JOIN housing_neighborhoods n ON n.id=e.neighborhood_id JOIN instance_list i ON i.id=n.instance_id WHERE e.active=1 AND e.native_id="+std::to_string(native)+" AND n.instance_id="+std::to_string(zone->GetInstanceID())+" AND n.zone_id=712 AND n.zone_version=0 AND i.zone=712 AND i.version=0 AND i.never_expires=1",false);
 if(!rows.Success()||rows.RowCount()!=1||!rows.begin()[0])return {};
 size_t end=0;std::string text=rows.begin()[0];const auto id=std::stoull(text,&end);if(end!=text.size()||!id||id>UINT32_MAX)return {};return Housing::EstateId(id);
}
std::string MetadataKey(){
 unsigned char nonce[16];if(RAND_bytes(nonce,sizeof(nonce))!=1)throw std::runtime_error("Housing access nonce unavailable");
 std::string key="acl:";const char* hex="0123456789abcdef";for(auto b:nonce){key+=hex[b>>4];key+=hex[b&15];}return key;
}
void MetadataSnapshot(Client* client,const Housing::NativeAccess::Snapshot& current){
 auto bytes=Housing::PermissionWire::EncodeSnapshot(current.wire);auto packet=std::make_unique<EQApplicationPacket>(OP_RealEstateAccess,bytes.size());
 std::memcpy(packet->pBuffer,bytes.data(),bytes.size());auto* raw=packet.release();client->FastQueuePacket(&raw);
 // Remember only the revision of a server snapshot actually queued for this client.
 client->RememberHousingAclRevision(current.native_plot_id,current.revision);
}
void MetadataRestore(Client* client,Housing::InventoryBridge::Bridge* bridge,Housing::EstateId estate,uint32_t native){
 if(bridge&&estate){auto current=bridge->ReadAccess(estate,native);if(current){MetadataSnapshot(client,current.value);if(zone&&zone->GetZoneID()!=712)client->SendHousingZoneCatalog();client->SendHousingPlacedScene();return;}}
 // The catalog display snapshot is read afresh, including revoked Manage. Never
 // echo a rejected proposal or the historical committed receipt as current ACL.
 client->SendHousingZoneCatalog();client->SendHousingPlacedScene();
}
void MetadataBroadcast() noexcept {
 try{for(const auto& entry:entity_list.GetClientList())if(entry.second&&NativeMetadata(entry.second)){
  entry.second->SendHousingZoneCatalog();entry.second->SendHousingPlacedScene();
 }}catch(const std::exception&){LogError("Native housing metadata viewer refresh unavailable");}
}
}
void Client::Handle_OP_RealEstateVotingOptOut(const EQApplicationPacket* app){
 if(!NativeMetadata(this)||!app||zone->GetZoneID()!=712)return;
 try{
  const auto request=Housing::EstateWire::DecodeVotingOptOut(app->pBuffer,app->size);if(!request)return;
  auto native=request->estate_id;const auto estate=MetadataEstate(this,native);auto* bridge=GetHousingInventoryBridge();
  if(!estate||!bridge){SendHousingVotingPreferenceSnapshot(this,request->estate_id);return;}
  const auto result=bridge->SetVotingOptOut(*estate,request->estate_id,request->opted_out);
  // Native6573 has no recovered acknowledgement. Publish the fresh63fd state;
  // never echo the proposed checkbox on failure or repeat an uncertain write.
  // Native5b09d0 already updated the initiating client's cached estate flag
  // before6573 was sent. No acknowledgement is recovered for this opcode.
  // On success, leave that matching value in place instead of rebuilding UI.
  // Plot-search inspection is independently one-shot in the original client.
  // Only rejected/uncertain writes require self restoration.
  if(result){
   LogInfo("Native housing voting preference saved actor [{}] plot [{}] opted_out [{}]",CharacterID(),request->estate_id,request->opted_out);
   for(const auto& entry:entity_list.GetClientList())if(entry.second&&entry.second!=this&&NativeMetadata(entry.second))SendHousingVotingPreferenceSnapshot(entry.second,request->estate_id);
  }else {SendHousingVotingPreferenceSnapshot(this,request->estate_id);Message(Chat::Red,"The property voting preference could not be confirmed.");}
 }catch(...){LogError("Native housing voting preference unavailable");SendHousingZoneCatalog();}
}
void Client::Handle_OP_RealEstateMetadata(const EQApplicationPacket* app){
 if(!NativeMetadata(this))return;
 Housing::EstateId estate=0;uint32_t native=0;Housing::InventoryBridge::Bridge* bridge=nullptr;bool committed=false;
 try{
  if(!app||app->size<4){SendHousingZoneCatalog();return;}
  native=Housing::NeighborhoodWire::Detail::Read32(app->pBuffer);
  auto resolved=MetadataEstate(this,native,true);if(!resolved){SendHousingZoneCatalog();SendHousingPlacedScene();return;}estate=*resolved;
  bridge=GetHousingInventoryBridge();auto proposal=Housing::PermissionWire::DecodeEdit(app->pBuffer,app->size);auto revision=HousingAclRevision(native);
  if(proposal)proposal->estate_id=native;
  if(!bridge||!proposal||!revision){MetadataRestore(this,bridge,estate,native);return;}
  Housing::NativeAccess::Request request;request.request_key=MetadataKey();request.estate_id=estate;request.expected_revision=*revision;request.proposal=std::move(*proposal);
  auto outcome=bridge->ReplaceAccess(std::move(request));
  if(!outcome){MetadataRestore(this,bridge,estate,native);Message(Chat::Red,"The housing permissions could not be saved.");return;}
  committed=true;
  // ReplaceAccess has resolved durable outcome and released the simulation fence.
  // The receipt is historical; fresh reads govern all snapshots and visibility.
  MetadataRestore(this,bridge,estate,native);MetadataBroadcast();
 }catch(const std::exception&){LogError("Native housing permission refresh unavailable");
  try{MetadataRestore(this,bridge,estate,native);}catch(const std::exception&){LogError("Native housing permission restoration unavailable");}
  if(committed)MetadataBroadcast();
 }
}
