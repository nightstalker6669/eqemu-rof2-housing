#include "house_inventory_profile_fence.h"
#include "inventory_projection.h"
#include "crate_placement_model.h"
#include "custody_codec.h"
#include <set>
namespace Housing::HouseInventory {
namespace {
bool SameLease(const InventorySessions::Lease& a,const InventorySessions::Lease& b){
 return a.active&&b.active&&a.actor.character_id==b.actor.character_id&&a.actor.account_id==b.actor.account_id&&a.epoch==b.epoch&&a.token==b.token&&a.owner.process_key==b.owner.process_key&&a.owner.client_key==b.owner.client_key&&a.owner.zone_id==b.owner.zone_id&&a.owner.instance_id==b.owner.instance_id;
}
bool SameStates(const std::vector<SlotState>& a,const std::vector<SlotState>& b){
 if(a.size()!=b.size())return false;
 for(size_t i=0;i<a.size();++i)if(a[i].slot!=b[i].slot||bool(a[i].row)!=bool(b[i].row)||(a[i].row&&CustodyCodec::Encode(*a[i].row)!=CustodyCodec::Encode(*b[i].row)))return false;
 return true;
}
}
ProfileFence::ProfileFence(EQ::InventoryProfile& inv,Request request,HeldPredicate held)
 :inventory_(inv),request_(std::move(request)),held_(std::move(held)){
 if(!held_||request_.before.empty()||request_.before.size()>11)throw std::invalid_argument("Invalid housing inventory fence scope");
 std::set<EQ::ItemInstance*> unique;
 for(const auto& state:request_.before){
  if(state.slot<23||state.slot>33||!pointers_.emplace(state.slot,inventory_.GetItem(int16_t(state.slot))).second)throw std::invalid_argument("Invalid housing inventory fence slots");
  auto item=pointers_.at(state.slot);
  if(item&&!unique.insert(item).second)throw std::invalid_argument("Duplicate housing inventory pointer");
 }
}
bool ProfileFence::HeldFor(const InventorySessions::Lease& lease)const noexcept {
 try{return SameLease(lease,request_.reservation)&&held_(lease);}catch(...){return false;}
}
bool ProfileFence::SameRequest(const Request& other)const {
 if(other.request_key!=request_.request_key||!SameLease(other.reservation,request_.reservation)||!SameStates(other.before,request_.before)||!SameStates(other.after,request_.after)||other.moves.size()!=request_.moves.size())return false;
 for(size_t i=0;i<other.moves.size();++i){const auto& a=other.moves[i];const auto& b=request_.moves[i];if(a.house_id!=b.house_id||a.crate_id!=b.crate_id||a.expected_revision!=b.expected_revision||a.from_slot!=b.from_slot||a.to_slot!=b.to_slot||a.runtime_serial!=b.runtime_serial)return false;}
 return true;
}
bool ProfileFence::Match(const std::vector<SlotState>& states,const std::vector<HouseBinding::Binding>& houses,const std::vector<CrateBinding::Allocation>& crates,bool after)const {
 if(inventory_.InventoryVersion()!=EQ::versions::ConvertClientVersionToMobVersion(EQ::versions::ClientVersion::RoF2)||!inventory_.GetLookup())return false;
 if(states.size()!=request_.before.size())return false;
 size_t occupied=0;for(const auto& state:states)if(state.row)++occupied;
 if(occupied!=request_.moves.size())return false;
 std::set<uint64_t> expected_houses;
 for(const auto& move:request_.moves)if(move.house_id&&!expected_houses.insert(move.house_id).second)return false;
 std::set<uint64_t> expected_crates;for(const auto& m:request_.moves)if(m.crate_id&&!expected_crates.insert(m.crate_id).second)return false;
 if((expected_houses.empty()&&expected_crates.empty())||expected_houses.size()!=houses.size()||expected_crates.size()!=crates.size())return false;
 std::set<uint32_t> slots;std::set<EQ::ItemInstance*> unique;std::set<uint64_t> matched_houses,matched_crates;
 for(const auto& state:states){
  if(state.slot<23||state.slot>33||!slots.insert(state.slot).second||!((inventory_.GetLookup()->PossessionsBitmask>>state.slot)&1))return false;
  auto item=inventory_.GetItem(int16_t(state.slot));
  if(state.slot==33&&inventory_.CursorSize()!=(state.row?1:0))return false;
  if(!state.row){if(item)return false;continue;}
  if(!item||!unique.insert(item).second||state.row->values.character_id!=request_.reservation.actor.character_id||state.row->values.slot_id!=state.slot||item->IsStackable()||!InventoryProjection::Matches(*state.row,*item))return false;
  const Move* move=nullptr;
  for(const auto& candidate:request_.moves)if((after?candidate.to_slot:candidate.from_slot)==state.slot){if(move)return false;move=&candidate;}
  if(!move||(move->house_id&&move->crate_id)||move->runtime_serial!=item->GetSerialNumber()||item->GetHousingHouseID()!=move->house_id||item->GetHousingCrateID()!=move->crate_id)return false;
  auto pinned=pointers_.find(move->from_slot);if(pinned==pointers_.end()||pinned->second!=item)return false;
  uint32_t estate=UINT32_MAX;
  if(move->house_id){
   const HouseBinding::Binding* house=nullptr;
   for(const auto& candidate:houses)if(candidate.house_id==move->house_id){if(house)return false;house=&candidate;}
   if(!house||!matched_houses.insert(house->house_id).second||house->item_template_id!=item->GetID()||house->location.kind!=HouseBinding::Location::Kind::Inventory||house->location.character_id!=request_.reservation.actor.character_id||house->location.slot_id!=state.slot||house->location.custody_id||!item->GetItem()->HousingDefinitionID||item->GetItem()->HousingDefinitionID==UINT32_MAX)return false;
   if(!move->expected_revision||move->expected_revision==UINT64_MAX||house->revision!=move->expected_revision+(after?1:0))return false;
   // Immutable receipt decoding intentionally omits the mutable SQL pending
   // key. The service's Postimage validates that key under its transaction.
   // Fresh results may carry it; an unrelated key is never acceptable here.
   if((!after&&!house->pending_key.empty())||(after&&!house->pending_key.empty()&&house->pending_key!=request_.request_key))return false;
   if(house->interior){if(!house->interior->native_estate_id||house->interior->native_estate_id==UINT32_MAX||house->interior->definition_id!=item->GetItem()->HousingDefinitionID)return false;estate=house->interior->native_estate_id;}
  }else if(move->crate_id){
   const CrateBinding::Allocation* crate=nullptr;for(const auto& c:crates)if(c.binding.crate_id==move->crate_id){if(crate)return false;crate=&c;}
   if(!crate||!matched_crates.insert(move->crate_id).second||!CratePlacement::Model(*item)||crate->binding.item_template_id!=item->GetID()||crate->binding.definition_id!=item->GetUnscaledItem()->HousingDefinitionID||crate->binding.custody_id||crate->inventory_character_id!=request_.reservation.actor.character_id||crate->inventory_slot_id!=state.slot||!move->expected_revision||move->expected_revision==UINT64_MAX||crate->binding.revision!=move->expected_revision+(after?1:0))return false;
   CrateBinding::EncodeAllocation(*crate);estate=crate->binding.native_estate_id;
  }else if(item->GetItem()->HousingDefinitionID)return false;
  if(item->GetHousingEstateID()!=estate)return false;
 }
 return matched_houses.size()==houses.size()&&matched_crates.size()==crates.size();
}
bool ProfileFence::BeforeMatches(const Request& request,const std::vector<HouseBinding::Binding>& houses)const noexcept {
 try{return HeldFor(request.reservation)&&SameRequest(request)&&Match(request.before,houses,{},false);}catch(...){return false;}
}
bool ProfileFence::BeforeMatches(const Request& request,const std::vector<HouseBinding::Binding>& houses,const std::vector<CrateBinding::Allocation>& crates)const noexcept {
 try{return HeldFor(request.reservation)&&SameRequest(request)&&Match(request.before,houses,crates,false);}catch(...){return false;}
}
bool ProfileFence::Applied(const Receipt& receipt)const noexcept {
 try{return HeldFor(receipt.request.reservation)&&SameRequest(receipt.request)&&Match(receipt.request.after,receipt.after,receipt.crates_after,true);}catch(...){return false;}
}
}
