#include "client.h"
#include "zone.h"
#include "housing_inventory_bridge.h"
#include "housing_anchor_pending.h"
#include "../common/rulesys.h"
#include <openssl/rand.h>
#include <cstring>
#include <utility>
extern Zone* zone;
namespace {
bool Ready(Client& c){return RuleB(Housing,EnablePlayerHousing)&&zone&&
 c.ClientVersion()==EQ::versions::ClientVersion::RoF2&&c.Connected()&&c.ClientDataLoaded()&&
 c.HasInventorySessionReservation()&&c.GetHP()>0;}
Housing::Route Source(){return {zone->GetZoneID(),uint8_t(zone->GetInstanceVersion()),uint16_t(zone->GetInstanceID())};}
void MoveToAnchor(Client& c,const Housing::Scene::AnchorDestination& destination){
 const auto& p=destination.pose;
 // Placement positions are native Y/X/Z. MovePC consumes server X/Y/Z.
 c.MovePC(destination.route.zone_id,destination.route.instance_id,p.position[1],p.position[0],p.position[2],p.heading);
}
void Unavailable(Client& c){c.Message(Chat::Red,"The placed anchor is unavailable or you do not have permission to enter its property.");}
}
void Client::ApplyHousingAnchorSpell(Client* caster,uint32_t item_id,bool confirmation){
 // Any new successfully processed anchor spell supersedes this Client's offer.
 m_housing_anchor_pending.reset();
 if(!caster||!Ready(*this)||!Ready(*caster))return;
 try{
  auto* bridge=GetHousingInventoryBridge();if(!bridge){Unavailable(*this);return;}
  // Pinned spell descriptions: SPA437 uses caster anchors; SPA438 uses
  // each recipient's own anchor. The caster name in7bb1 is independent.
  Client* owner=confirmation?this:caster;
  Housing::Scene::AnchorRequest request{{caster->CharacterID(),caster->AccountID()},{owner->CharacterID(),owner->AccountID()},item_id};
  auto result=bridge->ResolveAnchor(request,!confirmation);if(!result){Unavailable(*this);return;}
  if(!confirmation){MoveToAnchor(*this,result.value);return;}
  auto pending=std::make_shared<Housing::AnchorTransport::Pending>();pending->destination=std::move(result.value);
  pending->connection=Connection();pending->source=Source();pending->expires=std::chrono::steady_clock::now()+std::chrono::seconds(30);
  pending->offer.caster=pending->destination.caster_name;
  // The entire client consumer chain treats words0/2/3 solely as opaque echo
  // data. Use them as our random pending-offer identity, not guessed historical
  // spell/zone/item fields. Word1 and word4 retain their proven display roles.
  std::array<uint32_t,3> nonce{};
  if(RAND_bytes(reinterpret_cast<unsigned char*>(nonce.data()),sizeof(nonce))!=1){Unavailable(*this);return;}
  pending->offer.words={nonce[0],pending->destination.native_estate_id,nonce[1],nonce[2],pending->destination.guild_id};
  auto bytes=Housing::TrophyAnchorWire::EncodeAnchorOffer(pending->offer,pending->destination.destination_name);
  auto packet=new EQApplicationPacket(OP_RealEstateAnchorConfirm,bytes.size());std::memcpy(packet->pBuffer,bytes.data(),bytes.size());
  m_housing_anchor_pending=std::move(pending);FastQueuePacket(&packet);
 }catch(const std::exception&){m_housing_anchor_pending.reset();Unavailable(*this);}
}
void Client::Handle_OP_RealEstateAnchorConfirm(const EQApplicationPacket* app){
 // Session-local, single-use offer. Declines are local-only in the client;
 // replacement, expiry, source-route changes and Client destruction invalidate.
 auto pending=std::exchange(m_housing_anchor_pending,{});
 if(!pending||!Ready(*this)||!app)return;
 try{
  auto accepted=Housing::TrophyAnchorWire::DecodeAnchorConfirmation(app->pBuffer,app->size);
  if(!accepted||!pending->Matches(*accepted,Connection(),Source(),std::chrono::steady_clock::now()))return;
  auto* bridge=GetHousingInventoryBridge();if(!bridge){Unavailable(*this);return;}
  Housing::Scene::AnchorRequest request{pending->destination.caster,pending->destination.owner,pending->destination.item_id};
  // Re-lock owner/guild/ACL/lease, current content and exact anchor pose before
  // installing verified instance membership. Packet words are never routes.
  auto current=bridge->ResolveAnchor(request,true,&pending->destination);
  if(!current){Unavailable(*this);return;}MoveToAnchor(*this,current.value);
 }catch(const std::exception&){Unavailable(*this);}
}
