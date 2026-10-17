#include "housing_house_preload_fence.h"
#include "housing_inventory_identity.h"
#include "client.h"
namespace Housing::HousePreload {
namespace {
bool Same(const InventorySessions::Lease& a,const InventorySessions::Lease& b) noexcept {
 return a.active&&b.active&&a.actor.character_id==b.actor.character_id&&
  a.actor.account_id==b.actor.account_id&&a.epoch==b.epoch&&a.token==b.token&&
  a.owner.process_key==b.owner.process_key&&a.owner.client_key==b.owner.client_key&&
  a.owner.zone_id==b.owner.zone_id&&a.owner.instance_id==b.owner.instance_id;
}
}
ZoneSessionFence::ZoneSessionFence(Client& c,std::shared_ptr<InventorySessions::ZoneReservation> t,InventorySessions::Lease r)
 :client_(c),ticket_(std::move(t)),reservation_(std::move(r)){}
std::unique_ptr<ZoneSessionFence> ZoneSessionFence::Create(Client& c) {
 try {
  if(c.ClientVersion()!=EQ::versions::ClientVersion::RoF2)return nullptr;
  auto ticket=c.InventorySessionReleaseTicket();
  if(!ticket)return nullptr;
  auto reservation=ticket->SnapshotFor(c);
  if(!reservation)return nullptr;
  return std::unique_ptr<ZoneSessionFence>(new ZoneSessionFence(c,std::move(ticket),std::move(*reservation)));
 }catch(...){return nullptr;}
}
bool ZoneSessionFence::HeldFor(const InventorySessions::Lease& requested)const noexcept {
 try {
  if(!Same(requested,reservation_)||client_.InventorySessionReleaseTicket()!=ticket_)return false;
  auto current=ticket_->SnapshotFor(client_);
  return current&&Same(*current,reservation_)&&client_.ClientVersion()==EQ::versions::ClientVersion::RoF2;
 }catch(...){return false;}
}
bool ZoneSessionFence::LoadedMatches(const LiveItems::InventoryRow& row,int32_t serial)const noexcept {
 try {
  if(!HeldFor(reservation_)||row.values.character_id!=reservation_.actor.character_id||serial<=0)return false;
  auto& inventory=client_.GetInv();
  if(!InventoryBridge::SlotSupported(inventory,row.values.slot_id)||!InventoryBridge::CursorCompatible(inventory,row.values.slot_id,true))return false;
  auto item=inventory.GetItem(int16_t(row.values.slot_id));
  if(!item||item->GetSerialNumber()!=serial||!InventoryBridge::MatchesProjection(row,*item))return false;
  const auto previous=loaded_pointers_.find(row.values.slot_id);
  if(previous!=loaded_pointers_.end())return previous->second==item;
  for(const auto& witnessed:loaded_pointers_)if(witnessed.second==item)return false;
  loaded_pointers_.emplace(row.values.slot_id,item);
  return true;
 }catch(...){return false;}
}
}
