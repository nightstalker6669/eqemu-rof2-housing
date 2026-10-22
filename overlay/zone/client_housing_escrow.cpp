#include "client.h"
#include "zone.h"
#include "entity.h"
#include "housing_inventory_bridge.h"
#include "housing_coin_profile.h"
#include "housing_escrow_response.h"
#include "../common/eqemu_config.h"
#include "../common/rulesys.h"
#include "../common/housing/escrow_wire.h"
#include "../common/housing/sale_wire.h"
#include <openssl/rand.h>
#include <cstring>

extern Zone* zone;
namespace {
bool NativeEscrowClientReady(Client* client){
 return RuleB(Housing,EnablePlayerHousing)&&client->ClientVersion()==EQ::versions::ClientVersion::RoF2&&client->Connected()&&client->ClientDataLoaded()&&client->HasInventorySessionReservation()&&zone&&zone->GetInstanceID()!=0&&zone->GetInstanceID()<=32000&&zone->GetInstanceVersion()<=255;
}
// Move Out keeps its original exterior-only caller contract.
bool NativeEscrowContext(Client* client){return NativeEscrowClientReady(client)&&zone->GetZoneID()==712;}
std::optional<Housing::EstateId> EscrowEstate(uint32_t native){
 if(!native||native==UINT32_MAX)return {};const auto* config=EQEmuConfig::get();if(!config)return {};
 Database db;if(!db.Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"housing-escrow-route"))return {};
 // Identity only; the live service independently locks current catalog/route,
 // session reservation, profile, cursor, ownership/WithdrawUpkeep and balance.
 const auto source=zone->GetZoneID()==712?" AND n.instance_id="+std::to_string(zone->GetInstanceID())+" AND n.zone_id=712 AND n.zone_version=0 AND i.zone=712 AND i.version=0 AND i.never_expires=1":
  " AND EXISTS(SELECT 1 FROM housing_house_interiors h JOIN housing_native_estates child ON child.native_id=h.native_estate_id AND child.active=1 AND child.parent_native_id=e.native_id JOIN housing_house_locations l ON l.house_id=h.house_id AND l.location_kind=2 JOIN housing_live_item_custody c ON c.custody_id=l.custody_id WHERE c.estate_id=e.ownership_estate_id AND h.zone_id="+std::to_string(zone->GetZoneID())+" AND h.zone_version="+std::to_string(zone->GetInstanceVersion())+" AND h.instance_id="+std::to_string(zone->GetInstanceID())+")";
 auto rows=db.QueryDatabase("SELECT e.ownership_estate_id FROM housing_native_estates e JOIN housing_neighborhoods n ON n.id=e.neighborhood_id JOIN instance_list i ON i.id=n.instance_id WHERE e.active=1 AND e.native_id="+std::to_string(native)+source,false);
 if(!rows.Success()||rows.RowCount()!=1||!rows.begin()[0])return {};std::string text=rows.begin()[0];size_t end=0;auto id=std::stoull(text,&end);if(end!=text.size()||!id||id>UINT32_MAX)return {};return Housing::EstateId(id);
}
std::string EscrowKey(){unsigned char bytes[16];if(RAND_bytes(bytes,sizeof(bytes))!=1)throw std::runtime_error("Housing escrow nonce unavailable");std::string key="coin:";const char* hex="0123456789abcdef";for(auto b:bytes){key+=hex[b>>4];key+=hex[b&15];}return key;}
std::unique_ptr<EQApplicationPacket> EscrowPacket(const std::vector<uint8_t>& bytes){
 auto packet=std::make_unique<EQApplicationPacket>(OP_RealEstateEscrow,bytes.size());std::memcpy(packet->pBuffer,bytes.data(),bytes.size());return packet;
}
void EscrowQueue(Client* client,std::unique_ptr<EQApplicationPacket> packet){auto* raw=packet.release();client->FastQueuePacket(&raw);}
// Original5ad930 consumes a result DWORD;0 is silent and1 selects the
// generic sale refusal. Do not translate unrelated service errors into its
// more specific native reasons. Original full emitted size is unresolved.
void SaleResult(Client* client,uint32_t status){
 std::vector<uint8_t> bytes;Housing::NeighborhoodWire::Detail::Write32(bytes,status);
 EQApplicationPacket packet(OP_RealEstateSaleResult,bytes.size());std::memcpy(packet.pBuffer,bytes.data(),bytes.size());client->QueuePacket(&packet);
 LogInfo("Native plot sale result actor [{}] status [{}]",client->CharacterID(),status);
}
void EscrowRejected(Client* client,const Housing::EscrowWire::Message& request,bool known_committed=false){
 const auto bytes=Housing::InventoryBridge::EscrowResponse::Rejected(request,
  Housing::InventoryBridge::CoinProfile::Snapshot(client->GetPP()).cursor,known_committed);
 if(!bytes)return;
 EscrowQueue(client,EscrowPacket(*bytes));
 client->SendHousingZoneCatalog();client->SendHousingPlacedScene();
}
void EscrowBroadcast() noexcept{
 try{for(const auto& entry:entity_list.GetClientList())if(entry.second&&RuleB(Housing,EnablePlayerHousing)&&entry.second->ClientVersion()==EQ::versions::ClientVersion::RoF2&&entry.second->Connected()&&entry.second->ClientDataLoaded()){
  entry.second->SendHousingZoneCatalog();entry.second->SendHousingPlacedScene();
 }}catch(const std::exception&){LogError("Native escrow viewer refresh unavailable");}
}
}
void Client::Handle_OP_RealEstateEscrow(const EQApplicationPacket* app){
 if(!NativeEscrowClientReady(this)||!app)return;
 std::optional<Housing::EscrowWire::Message> request;bool committed=false;
 try{
  request=Housing::EscrowWire::Decode(app->pBuffer,app->size);
  if(!request)return; // Undecodable framing cannot identify a denomination safely.
  if(request->action==uint32_t(Housing::EscrowWire::Action::SelectUpkeep)){
   if(!RuleB(Housing,EnableNativeEscrowTransfers)||!Housing::EscrowWire::IsSupportedCoinRequest(*request)){
    SendHousingZoneCatalog();return;
   }
   const auto estate=EscrowEstate(request->estate_id);auto* bridge=estate?GetHousingInventoryBridge():nullptr;
   if(!estate||!bridge){SendHousingZoneCatalog();return;}
   const auto result=bridge->SelectUpkeep(EscrowKey(),*estate,request->estate_id,request->component_id);
   // No coin cursor assignment, debit or prepaid interval is implied by action2.
   // Refresh current authoritative state, including after a historical replay.
   if(!result)Message(Chat::Red,"The upkeep payment option could not be changed.");
   SendHousingZoneCatalog();if(result)EscrowBroadcast();return;
  }
  if(!RuleB(Housing,EnableNativeEscrowTransfers)||!Housing::EscrowWire::IsSupportedCoinRequest(*request)||(request->action!=0&&request->action!=1&&request->action!=3&&request->action!=4&&request->action!=7)){
   EscrowRejected(this,*request);return;
  }
  const auto estate=EscrowEstate(request->estate_id);auto* bridge=estate?GetHousingInventoryBridge():nullptr;
  if(!estate||!bridge){EscrowRejected(this,*request);return;}
  if(request->action==3||request->action==4||request->action==7){
   // Recovered empty requests have no cursor assignment or funding selector.
   // Explicit carried-only policy, amount/rate witness and journal live in the TX.
   if(!Housing::InventoryBridge::CoinProfile::CursorItemsEmpty(GetInv())||!Housing::CoinWallet::Empty(Housing::InventoryBridge::CoinProfile::Snapshot(GetPP()).cursor)){
    Message(Chat::Red,"Put all items and coins from your cursor into inventory before using Add a Day, Add a Week, or Withdraw All.");EscrowRejected(this,*request);return;
   }
   Housing::EscrowWire::Message response;response.action=request->action;response.estate_id=request->estate_id;response.response_flag=1;
   auto success=EscrowPacket(Housing::EscrowWire::Encode(response));
   auto result=bridge->TransferCoin(EscrowKey(),*estate,*request,true);
   if(!result){LogInfo("Native bulk escrow rejected actor [{}] estate [{}] action [{}] backend [{}]",CharacterID(),request->estate_id,request->action,int(result.error));Message(Chat::Red,"The escrow request could not be completed. Check carried funds and the property's payment policy.");EscrowRejected(this,*request);return;}
   committed=true;EscrowQueue(this,std::move(success));SendMoneyUpdate();SendHousingZoneCatalog();SendHousingPlacedScene();EscrowBroadcast();return;
  }
  const auto denomination=Housing::CoinWallet::Denomination(request->values[0].subtype);
  const auto before=Housing::InventoryBridge::CoinProfile::Snapshot(GetPP());
  const auto plan=request->action==0?Housing::CoinWallet::DepositCursor(before.cursor,denomination,request->values[0].amount):Housing::CoinWallet::WithdrawCursor(before.cursor,denomination,request->values[0].amount);
  if(!plan||!Housing::InventoryBridge::CoinProfile::CursorItemsEmpty(GetInv())){EscrowRejected(this,*request);return;}
  // All success framing/packet allocation is staged before the irreversible
  // transfer. The bridge validates this request and applies/ACKs under its fence.
  Housing::EscrowWire::Message response;response.action=request->action;response.estate_id=request->estate_id;response.response_flag=1;
  if(request->action==1)response.values={{0,request->values[0].subtype,Housing::CoinWallet::Detail::Counter(plan.value.after,denomination)}};
  auto success=EscrowPacket(Housing::EscrowWire::Encode(response));const auto key=EscrowKey();
  auto result=bridge->TransferCoin(key,*estate,*request,true);
  if(!result){EscrowRejected(this,*request);Message(Chat::Red,"The escrow transfer could not be completed.");return;}
  committed=true;
  // A fresh server nonce is used per packet; there is no wire operation ID and
  // no cross-packet idempotency claim. Historical receipt state is not rendered.
  if(!(Housing::InventoryBridge::CoinProfile::Snapshot(GetPP()).cursor==plan.value.after)||!Housing::InventoryBridge::CoinProfile::CursorItemsEmpty(GetInv())){
   LogError("Committed native escrow profile requires refresh");SendHousingZoneCatalog();EscrowBroadcast();return;
  }
  EscrowQueue(this,std::move(success));SendHousingZoneCatalog();SendHousingPlacedScene();EscrowBroadcast();
 }catch(const std::exception&){LogError("Native escrow transfer refresh unavailable");
  if(committed){EscrowBroadcast();return;} // Never refund a known committed debit.
  if(request)try{EscrowRejected(this,*request,committed);}catch(const std::exception&){LogError("Native escrow cursor restoration unavailable");}
 }
}

// Confirmation100 uses792c;101/507f routes to the separate ownership
// transfer handler. Never treat its target character claim as Move Out.
void Client::Handle_OP_RealEstateSale(const EQApplicationPacket* app){
 if(!NativeEscrowContext(this)||!app)return;
 bool completed=false;
 try{
  auto request=Housing::SaleWire::Decode(app->pBuffer,app->size);if(!request)return;
  auto* bridge=GetHousingInventoryBridge();const auto estate=bridge?bridge->ConsumeRetirementInspection(request->estate_id):std::nullopt;
  if(!estate||!bridge){LogInfo("Housing Move Out requires a fresh inspection: character [{}] native [{}]",CharacterID(),request->estate_id);SaleResult(this,1);SendHousingZoneCatalog();return;}
  const auto key=EscrowKey();auto empty=bridge->RetireEmptyPlot(key,*estate,request->estate_id);completed=bool(empty);
  if(!completed)LogInfo("Housing empty Move Out declined: character [{}] estate [{}] native [{}] error [{}]",CharacterID(),*estate,request->estate_id,int(empty.error));
  if(!completed&&empty.error==Housing::LiveItems::Error::UnsupportedItem&&RuleB(Housing,EnableNativeFurniturePlacement)){
   auto occupied=bridge->MoveOutOccupied(key,*estate,request->estate_id,true);
   if(occupied){
    completed=true;
    // The bridge has resolved COMMIT, applied the exact staged token, ACKed
    // the existing item receipt and released its barrier before any packets.
    auto* returned=GetInv().GetItem(int16_t(occupied.value.receipt.inventory_after.values.slot_id));
    if(returned&&returned->GetSerialNumber()==occupied.value.runtime_serial&&Housing::InventoryBridge::Matches(occupied.value.receipt.inventory_after,*returned)&&occupied.value.inventory_packet){auto* packet=occupied.value.inventory_packet.release();FastQueuePacket(&packet);}
    else LogError("Committed Move Out inventory presentation unavailable");
    if(occupied.value.refunded)SendMoneyUpdate();
   }
  }
  SaleResult(this,completed?0:1);
  // Native result0 is silent.63fd ownership invalidation and the complete
  //2aa7 owned list remain the authoritative cache paths after known commit.
  SendHousingZoneCatalog();SendHousingPlacedScene();if(completed)EscrowBroadcast();
 }catch(const std::exception&){LogError("Native Move Out refresh unavailable");EscrowBroadcast();}
}
