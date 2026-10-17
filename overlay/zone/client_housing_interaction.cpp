#include "client.h"
#include "zone.h"
#include "../common/rulesys.h"
#include "../common/housing/interaction_wire.h"
#include "../common/housing/known_original_interior_zones.h"
extern Zone* zone;
void Client::Handle_OP_RealEstateInteraction(const EQApplicationPacket* app){
 if(!RuleB(Housing,EnablePlayerHousing)||!RuleB(Housing,EnableNativeFurniturePlacement)||ClientVersion()!=EQ::versions::ClientVersion::RoF2||!Connected()||!ClientDataLoaded()||!HasInventorySessionReservation()||!zone||!zone->GetInstanceID()||!app)return;
 // Room item menus use the same original messages as yard furniture. The
 // current scene below still authorizes each estate/item; zone recognition
 // alone grants no interaction with an unbound or inaccessible interior.
 if(zone->GetZoneID()!=712&&!Housing::OriginalInteriors::Matches(zone->GetZoneID(),zone->GetShortName()))return;
 try{
  if(app->GetOpcode()==OP_RealEstateItemMenu){auto query=Housing::InteractionWire::DecodeQuery(app->pBuffer,app->size);if(!query)return;
   SendHousingChairMenu(query->source.estate_id,query->source.item_id,query->mouse,query->profile==Housing::InteractionWire::QueryProfile::ActorRightClick?std::string{}:std::string(reinterpret_cast<const char*>(query->source.guid.data()),16));
  }else if(app->GetOpcode()==OP_RealEstateItemQuery){auto query=Housing::InteractionWire::DecodeActorQuery(app->pBuffer,app->size);if(!query)return;SendHousingChairMenu(query->estate_id,query->item_id,{0,0});}
 }catch(const std::exception&){LogError("Native chair interaction request unavailable");}
}
