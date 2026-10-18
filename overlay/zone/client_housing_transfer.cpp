#include "client.h"
#include "entity.h"
#include "zone.h"
#include "worldserver.h"
#include "housing_inventory_bridge.h"
#include "housing_transfer_pending.h"
#include "../common/rulesys.h"
#include <openssl/rand.h>
#include <cstring>
extern Zone*zone;extern WorldServer worldserver;
namespace {
bool Ready(Client& c){return RuleB(Housing,EnablePlayerHousing)&&zone&&zone->GetZoneID()==712&&!zone->GetInstanceVersion()&&zone->GetInstanceID()&&c.ClientVersion()==EQ::versions::ClientVersion::RoF2&&c.Connected()&&c.ClientDataLoaded()&&c.HasInventorySessionReservation();}
// Original5adad0 consumes one result DWORD:0 silent,2 not owner,
//3 NO TRADE house/crate,other nonzero generic failure. Four bytes are our
// bounded response profile; an original exclusive full length is unproved.
void TransferResult(Client& client,uint32_t status){
 std::vector<uint8_t> bytes;Housing::NeighborhoodWire::Detail::Write32(bytes,status);
 EQApplicationPacket packet(OP_RealEstateTransferResult,bytes.size());std::memcpy(packet.pBuffer,bytes.data(),bytes.size());client.QueuePacket(&packet);
 LogInfo("Native plot transfer result actor [{}] status [{}]",client.CharacterID(),status);
}
uint32_t FailureStatus(Housing::OwnershipTransfer::Error error){
 using Error=Housing::OwnershipTransfer::Error;
 return error==Error::NotPrimaryOwner?2:error==Error::NoTradeHouseOrCrate?3:1;
}
void ExplainContentPolicy(Client& client,Housing::OwnershipTransfer::Error error){
 if(error==Housing::OwnershipTransfer::Error::UnsupportedContent)client.Message(Chat::Red,"This plot contains items whose complete transfer profile is not supported. Its ownership has not changed.");
}
std::string Key(const std::array<uint8_t,20>& nonce){const char*hex="0123456789abcdef";std::string key="transfer:";for(auto x:nonce){key+=hex[x>>4];key+=hex[x&15];}return key;}
}
void Client::Handle_OP_RealEstateTransfer(const EQApplicationPacket*app){
 if(!Ready(*this)||!app)return;
 try{
  auto decoded=Housing::OwnershipTransfer::Decode(app->pBuffer,app->size);if(!decoded)return;
  auto*target=GetTarget();Client*recipient=target&&target->IsClient()?target->CastToClient():nullptr;
  if(!recipient||recipient==this||!Ready(*recipient)){TransferResult(*this,1);return;}
  if(m_housing_transfer_outgoing&&m_housing_transfer_outgoing->Expired())m_housing_transfer_outgoing.reset();
  if(recipient->m_housing_transfer_incoming&&recipient->m_housing_transfer_incoming->Expired())recipient->m_housing_transfer_incoming.reset();
  if(m_housing_transfer_outgoing||recipient->m_housing_transfer_incoming){TransferResult(*this,1);return;}
  auto*bridge=GetHousingInventoryBridge();if(!bridge){TransferResult(*this,1);return;}
  std::array<uint8_t,20> nonce{};if(RAND_bytes(nonce.data(),nonce.size())!=1){TransferResult(*this,1);return;}
  auto inspected=bridge->InspectPersonalPlotTransfer(Key(nonce),decoded->source);
  if(!inspected){ExplainContentPolicy(*this,inspected.error);TransferResult(*this,FailureStatus(inspected.error));SendHousingZoneCatalog();return;}
  auto body=Housing::TransferConsentWire::Offer(decoded->source,GetName(),nonce);if(!body)return;
  auto pending=std::make_shared<Housing::TransferConsent::Pending>();pending->inspected=std::move(inspected.value);pending->offer=*body;
  pending->sender_connection=Connection();pending->recipient_connection=recipient->Connection();
  pending->expires=std::chrono::steady_clock::now()+std::chrono::minutes(5);
  EQApplicationPacket packet(OP_RealEstateTransferConsent,body->size());std::memcpy(packet.pBuffer,body->data(),body->size());
  // Shared offer identity is session-local on BOTH Client objects. A reconnect,
  // zoning, replaced offer, decline or successful reply cannot reauthorize it.
  // Native consent reads the name from the recipient estate cache. Refresh it
  // before the offer so a newly acquired plot has its catalog-backed address.
  recipient->SendHousingZoneCatalog();
  m_housing_transfer_outgoing=pending;recipient->m_housing_transfer_incoming=pending;recipient->QueuePacket(&packet);
  LogInfo("Native plot transfer offer actor [{}] recipient [{}] plot [{}]",CharacterID(),recipient->CharacterID(),decoded->native_plot_id);
 }catch(const std::exception&){LogError("Native ownership transfer offer unavailable");TransferResult(*this,1);}
}
void Client::Handle_OP_RealEstateTransferConsent(const EQApplicationPacket*app){
 if(!Ready(*this)||!app)return;
 auto pending=m_housing_transfer_incoming;if(!pending)return;
 auto reply=Housing::TransferConsentWire::Reply(pending->offer,app->pBuffer,app->size);if(!reply)return;
 auto*sender=entity_list.GetClientByCharID(pending->inspected.request.lease.actor.character_id);
 const bool current=sender&&Ready(*sender)&&sender->m_housing_transfer_outgoing==pending&&sender->Connection()==pending->sender_connection&&Connection()==pending->recipient_connection&&CharacterID()==pending->inspected.request.recipient.actor.character_id&&AccountID()==pending->inspected.request.recipient.actor.account_id&&GetID()==pending->inspected.request.recipient.runtime_entity_id&&!pending->Expired();
 // Consume before any commit/refresh. An unsolicited or repeated reply never
 // reaches TransferPersonalPlot. Opaque packet fields supply no actor authority.
 m_housing_transfer_incoming.reset();if(sender&&sender->m_housing_transfer_outgoing==pending)sender->m_housing_transfer_outgoing.reset();
 if(!current){if(sender&&Ready(*sender))TransferResult(*sender,1);return;}
 if(!*reply){sender->MessageString(Chat::Yellow,7638);LogInfo("Native plot transfer declined actor [{}] recipient [{}]",sender->CharacterID(),CharacterID());return;}
 bool committed=false;
 try{
  auto*bridge=sender->GetHousingInventoryBridge();if(!bridge){TransferResult(*sender,1);return;}
  // Fresh current-owner/recipient/plot-limit/route/position/target/ACL/catalog
  // checks and receipt recovery remain under the existing sender inventory fence.
  auto result=bridge->TransferPersonalPlot(pending->inspected.request.key,pending->inspected.request.wire,pending->inspected.source);
  if(!result){ExplainContentPolicy(*sender,result.error);TransferResult(*sender,FailureStatus(result.error));sender->SendHousingZoneCatalog();return;}
  committed=true;TransferResult(*sender,0);
  sender->MessageString(Chat::Yellow,736,result.value.source.address.c_str(),GetName());
  MessageString(Chat::Yellow,735,sender->GetName(),result.value.source.address.c_str());
  LogInfo("Native plot transfer accepted actor [{}] recipient [{}] plot [{}]",sender->CharacterID(),CharacterID(),result.value.source.native_plot_id);
  Client::HousingUpkeepChanged();if(worldserver.Connected()){ServerPacket refresh(ServerOP_HousingUpkeepRefresh,1);refresh.pBuffer[0]=0;worldserver.SendPacket(&refresh);}
 }catch(const std::exception&){LogError("Native ownership transfer {}",committed?"committed; refresh unavailable":"not completed before response");if(!committed)TransferResult(*sender,1);}
}
