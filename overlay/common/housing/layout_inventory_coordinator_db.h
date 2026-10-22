#include "item_payload_db.h"
#ifndef EQEMU_HOUSING_LAYOUT_INVENTORY_COORDINATOR_DB_H
#define EQEMU_HOUSING_LAYOUT_INVENTORY_COORDINATOR_DB_H
#include "layout_inventory_application.h"
#include "inventory_row_db.h"
#include "occupied_parent_yard.h"
namespace Housing::LayoutInventoryCoordinator {
struct Staged {LayoutInventoryJournal::Receipt receipt;LayoutInventory::Plan plan;LayoutInventory::Prepared prepared;};
inline LayoutInventoryJournal::Binding Bind(const std::string& key,const LiveItems::Lease& lease,const std::vector<uint8_t>& wire,const std::vector<LiveItems::LayoutUnitDestination>& targets){
 LayoutInventoryJournal::Binding b{key,lease,wire,{1}};auto load=LayoutInventoryJournal::Detail::Wire(b);if(!load||load->operations.size()!=targets.size())throw std::invalid_argument("Invalid inventory layout intent");const bool parent_context=std::any_of(targets.begin(),targets.end(),[](const auto& t){return bool(t.parent_yard);});b.plan.clear();CustodyCodec::W(b.plan,parent_context?2:1,4);CustodyCodec::W(b.plan,targets.size(),2);
 auto route=[&](Route r){CustodyCodec::W(b.plan,r.zone_id,4);CustodyCodec::W(b.plan,r.zone_version,1);CustodyCodec::W(b.plan,r.instance_id,2);};
 for(size_t i=0;i<targets.size();++i){const auto& t=targets[i];const auto& op=std::get<LayoutWire::PlacementOperation>(load->operations[i]);if(t.native_estate_id!=op.target_estate_id||!t.estate_id||t.catalog_key.empty()||t.catalog_key.size()>96||!t.catalog_revision||(!t.room&&t.exterior_source)||(t.parent_yard&&(t.room||t.exterior_source||!OccupiedParentYard::Shape(*t.parent_yard))))throw std::invalid_argument("Invalid layout destination");
  CustodyCodec::W(b.plan,t.estate_id,4);CustodyCodec::W(b.plan,t.native_estate_id,4);CustodyCodec::S(b.plan,t.catalog_key);CustodyCodec::W(b.plan,t.catalog_revision,8);CustodyCodec::W(b.plan,bool(t.room),1);
  if(t.room){CustodyCodec::W(b.plan,t.room->house_id,8);CustodyCodec::W(b.plan,t.room->house_revision,8);CustodyCodec::W(b.plan,t.room->profile_revision,8);route(t.room->source);}CustodyCodec::W(b.plan,bool(t.exterior_source),1);if(t.exterior_source)route(*t.exterior_source);if(parent_context){CustodyCodec::W(b.plan,bool(t.parent_yard),1);if(t.parent_yard)LayoutInventory::OutcomeDetail::Blob(b.plan,OccupiedParentYard::Encode(*t.parent_yard));}
 }return b;
}
// One actual caller transaction: resolve first, load source SQL from the real
// fenced inventory, prepare pieces, create all authorized destinations, consume
// grouped sources, and complete journal+pending. No lifecycle/packets/memory.
inline LiveItems::Result<Staged> StageUnderTransaction(Database& db,bool enabled,const LayoutInventoryJournal::Binding& binding,const std::vector<LiveItems::LayoutUnitDestination>& targets,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* houses=nullptr,const InteriorContent::ProfilePolicy* rooms=nullptr,const CrateBinding::CustodyContext* crates=nullptr){
 using Error=LiveItems::Error;if(!enabled)return {{},Error::Disabled};
 try{
  auto expected=Bind(binding.request_key,binding.lease,binding.wire,targets);if(LayoutInventoryJournal::Detail::Canonical(expected)!=LayoutInventoryJournal::Detail::Canonical(binding))return {{},Error::InvalidArgument};
  auto prior=LayoutInventoryJournal::ReadUnderTransaction(db,true,binding,fence);if(prior){Staged out;out.receipt=std::move(prior.value);return {std::move(out),Error::None};}if(prior.error!=Error::NotFound)return {{},prior.error};
  auto load=*LayoutInventoryJournal::Detail::Wire(binding);std::set<uint32_t> slots;
  for(const auto& operation:load.operations){const auto& op=std::get<LayoutWire::PlacementOperation>(operation);auto slot=LayoutInventory::ResolveSlot(inv,op.source_kind,op.source_slots);if(!slot)return {{},Error::UnsupportedItem};slots.insert(*slot);if(op.source_slots[1]!=0xffff)slots.insert(op.source_slots[0]);}
  std::vector<LiveItems::InventoryRow> rows;for(auto slot:slots){auto selected=LayoutInventoryJournal::Detail::Q(db,"SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(binding.lease.actor.character_id)+" AND slot_id="+std::to_string(slot)+" FOR UPDATE");if(selected.RowCount()!=1)return {{},Error::Conflict};auto captured=InventoryRowDB::Decode(selected.begin());auto* memory=inv.GetItem(int16_t(slot));if(!memory)return {{},Error::Conflict};captured.evolving_payload=EvolvingItemCustody::Capture(db,captured.values.character_id,slot,*memory);rows.push_back(std::move(captured));}
  auto plan=LayoutInventory::Build(load,binding.lease,rows,inv,fence);if(!plan)return {{},plan.error};auto prepared=LayoutInventory::PrepareOrdinary(plan.value,binding.lease,inv,fence);if(!prepared)return {{},prepared.error};
  std::vector<LiveItems::LayoutUnitPlacement> placements;placements.reserve(prepared.value.units.size());
  for(size_t i=0;i<prepared.value.units.size();++i){const auto ordinal=prepared.value.units[i].operation_index;auto target=targets[ordinal];target.lease=binding.lease;target.transform=LayoutWire::PlacementTransform(std::get<LayoutWire::PlacementOperation>(load.operations[ordinal]));
   const auto member=binding.request_key+":"+std::to_string(ordinal);std::array<uint8_t,SHA256_DIGEST_LENGTH> digest;SHA256(reinterpret_cast<const unsigned char*>(member.data()),member.size(),digest.data());target.request_key="layout-unit:"+LayoutInventoryJournal::Detail::Hex(std::vector<uint8_t>(digest.begin(),digest.end()));
   auto placed=LiveItems::Service::PlaceLayoutUnitUnderTransaction(db,true,target,plan.value,prepared.value,i,inv,fence,houses,rooms,crates);if(!placed)return {{},placed.error};placements.push_back(std::move(placed.value));
  }
  auto consumed=LayoutInventory::ConsumeSourcesUnderTransaction(db,true,binding.lease,plan.value,prepared.value,inv,fence);if(!consumed)return {{},consumed.error};for(const auto& placed:placements)ItemPayload::Hold(db,placed.stored.row,placed.stored.custody_id);auto completed=LayoutInventoryJournal::CompleteUnderTransaction(db,true,binding,plan.value,prepared.value,consumed.value,placements,inv,fence);if(!completed)return {{},completed.error};
  Staged out;out.receipt=std::move(completed.value);out.plan=std::move(plan.value);out.prepared=std::move(prepared.value);return {std::move(out),Error::None};
 }catch(const LayoutInventoryJournal::Detail::Failure& e){return {{},e.error};}catch(const std::invalid_argument&){return {{},Error::InvalidArgument};}catch(...){return {{},Error::DatabaseFailure};}
}
}
#endif
