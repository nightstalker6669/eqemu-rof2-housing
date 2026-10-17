#include "client.h"
#include "entity.h"
#include "zone.h"
#include "worldserver.h"
#include "housing_inventory_bridge.h"
#include "../common/rulesys.h"
#include <openssl/rand.h>
extern Zone*zone;extern WorldServer worldserver;
void Client::Handle_OP_RealEstateTransfer(const EQApplicationPacket*app){
 if(!RuleB(Housing,EnablePlayerHousing)||ClientVersion()!=EQ::versions::ClientVersion::RoF2||!Connected()||!ClientDataLoaded()||!HasInventorySessionReservation()||!zone||zone->GetZoneID()!=712||zone->GetInstanceVersion()||!zone->GetInstanceID()||!app)return;
 bool committed=false;try{auto decoded=Housing::OwnershipTransfer::Decode(app->pBuffer,app->size);if(!decoded)return;auto*bridge=GetHousingInventoryBridge();if(!bridge)return;unsigned char nonce[16];if(RAND_bytes(nonce,sizeof nonce)!=1)return;const char*hex="0123456789abcdef";std::string key="transfer:";for(auto x:nonce){key+=hex[x>>4];key+=hex[x&15];}auto result=bridge->TransferPersonalPlot(key,decoded->source);if(!result){MessageString(Chat::Red,result.error==Housing::OwnershipTransfer::Error::NotPrimaryOwner?5279:5280);SendHousingZoneCatalog();return;}committed=true;
 // No507f response layout is invented. Generic original messages and existing
 // native ownership/ACL/owned-list snapshots carry the authoritative result.
 MessageString(Chat::Yellow,736,result.value.source.address.c_str(),result.value.request.recipient.name.c_str());
 if(auto*recipient=entity_list.GetClientByCharID(result.value.request.recipient.actor.character_id))recipient->MessageString(Chat::Yellow,735,GetName(),result.value.source.address.c_str());
 Client::HousingUpkeepChanged();if(worldserver.Connected()){ServerPacket refresh(ServerOP_HousingUpkeepRefresh,0);worldserver.SendPacket(&refresh);}
 }catch(const std::exception&){LogError("Native ownership transfer {}",committed?"committed; refresh unavailable":"not completed before response");}
}
