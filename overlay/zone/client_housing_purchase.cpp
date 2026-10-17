#include "client.h"
#include "zone.h"
#include "entity.h"
#include "housing_inventory_bridge.h"
#include "../common/rulesys.h"
#include <openssl/rand.h>
#include <cstring>
extern Zone* zone;
void Client::Handle_OP_RealEstatePurchase(const EQApplicationPacket*app){
 if(!RuleB(Housing,EnablePlayerHousing)||ClientVersion()!=EQ::versions::ClientVersion::RoF2||!Connected()||!ClientDataLoaded()||!HasInventorySessionReservation()||!zone||zone->GetZoneID()!=712||zone->GetInstanceVersion()||!zone->GetInstanceID()||!app||app->size!=Housing::EstateWire::PurchaseSize)return;
 bool committed=false;try{auto request=Housing::EstateWire::DecodePurchase(app->pBuffer,app->size);if(!request)return;auto*bridge=GetHousingInventoryBridge();if(!bridge)return;
  auto success=std::make_unique<EQApplicationPacket>(OP_RealEstatePurchase,1);success->pBuffer[0]=1;auto failure=std::make_unique<EQApplicationPacket>(OP_RealEstatePurchase,1);failure->pBuffer[0]=0;std::array<uint8_t,Housing::EstateWire::PurchaseSize>wire;std::memcpy(wire.data(),app->pBuffer,wire.size());unsigned char nonce[16];if(RAND_bytes(nonce,sizeof nonce)!=1)throw std::runtime_error("Purchase nonce unavailable");std::string key="plotbuy:";const char*hex="0123456789abcdef";for(auto x:nonce){key+=hex[x>>4];key+=hex[x&15];}
  auto result=bridge->BuyPlot(key,wire);if(!result){auto*packet=failure.release();FastQueuePacket(&packet);Message(Chat::Red,"This plot could not be purchased with the selected payment option.");SendHousingZoneCatalog();return;}committed=true;
  // Profile application and durable ACK completed under the synchronous fence.
  // Refresh authoritative owner/ACL/owned-list separately from native bool ACK.
  SendMoneyUpdate();SendHousingZoneCatalog();auto*packet=success.release();FastQueuePacket(&packet);
  for(const auto&entry:entity_list.GetClientList())if(entry.second&&entry.second!=this&&entry.second->ClientVersion()==EQ::versions::ClientVersion::RoF2&&entry.second->Connected()&&entry.second->ClientDataLoaded())entry.second->SendHousingZoneCatalog();
 }catch(const std::exception&){LogError("Native plot purchase {}",committed?"committed; refresh unavailable":"not completed before response");}
}
