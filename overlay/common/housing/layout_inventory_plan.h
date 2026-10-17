#ifndef EQEMU_HOUSING_LAYOUT_INVENTORY_PLAN_H
#define EQEMU_HOUSING_LAYOUT_INVENTORY_PLAN_H
#include "layout_wire.h"
#include "inventory_projection.h"
#include "../inventory_profile.h"
#include <map>
#include <set>
namespace Housing::LayoutInventory {
// Inventory unit accounting only. No SQL, custody/serial allocation, mutation,
// packet send or source/destination property permission is supplied here.
// Original73a610 emits one unit per tag1, retaining the source GUID/location.
inline std::string NativeGuid(int32_t serial){
 if(serial<=0)return {};auto value=std::to_string(serial);return std::string(16-value.size(),'0')+value;
}
inline bool RootSupported(const EQ::InventoryProfile& inv,uint16_t root){
 return root>=23&&root<=33&&inv.GetLookup()&&((inv.GetLookup()->PossessionsBitmask>>root)&1);
}
inline std::optional<uint32_t> ResolveSlot(EQ::InventoryProfile& inv,uint8_t kind,const std::array<uint16_t,3>& slots){
 const auto root=slots[0];
 if(kind||!RootSupported(inv,root)||slots[2]!=0xffff||(root==33&&inv.CursorSize()!=1))return {};
 if(slots[1]==0xffff)return root;
 auto* parent=inv.GetItem(int16_t(root));const auto* data=parent?parent->GetItem():nullptr;
 if(!data||data->ItemClass!=EQ::item::ItemClassBag||slots[1]>=uint16_t(data->BagSlots)||slots[1]>=EQ::invbag::SLOT_COUNT)return {};
 const auto slot=EQ::InventoryProfile::CalcSlotId(int16_t(root),uint8_t(slots[1]));
 if(slot<0||EQ::InventoryProfile::CalcSlotId(slot)!=root||EQ::InventoryProfile::CalcBagIdx(slot)!=slots[1])return {};
 return uint32_t(slot);
}
inline bool SlotSupported(EQ::InventoryProfile& inv,uint32_t slot){
 if(slot>INT16_MAX)return false;
 if(RootSupported(inv,uint16_t(slot)))return ResolveSlot(inv,0,{uint16_t(slot),0xffff,0xffff})==slot;
 const auto root=EQ::InventoryProfile::CalcSlotId(int16_t(slot));if(root<0)return false;
 return ResolveSlot(inv,0,{uint16_t(root),EQ::InventoryProfile::CalcBagIdx(int16_t(slot)),0xffff})==slot;
}
struct Source {
 LiveItems::InventoryRow before;
 std::optional<LiveItems::InventoryRow> after;
 EQ::ItemInstance* original=nullptr; // Valid only under the same live fence.
 EQ::ItemInstance* parent=nullptr;
 int32_t serial=0,parent_serial=0;
 uint32_t available=0,consumed=0;
 bool stackable=false;
};
struct Use {size_t operation_index=0,source_index=0;};
struct Plan {std::vector<Source> sources;std::vector<Use> uses;};
inline LiveItems::Result<Plan> Build(const LayoutWire::Load& load,const LiveItems::Lease& lease,
 const std::vector<LiveItems::InventoryRow>& current_rows,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence,const std::set<size_t>& retained_ordinals={}){
 using Error=LiveItems::Error;
 if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))return {{},Error::UnheldFence};
 if(!lease.actor.character_id||!lease.actor.account_id||!lease.epoch||load.operations.size()>UINT16_MAX)return {{},Error::InvalidArgument};
 std::map<uint32_t,const LiveItems::InventoryRow*> rows;
 for(const auto& row:current_rows)if(row.values.character_id!=lease.actor.character_id||!rows.emplace(row.values.slot_id,&row).second)return {{},Error::Conflict};
 Plan plan;std::map<uint32_t,size_t> selected;std::set<int32_t> serials;
 for(size_t ordinal=0;ordinal<load.operations.size();++ordinal){
  const auto* op=std::get_if<LayoutWire::PlacementOperation>(&load.operations[ordinal]);if(!op||retained_ordinals.count(ordinal))continue;
  if(!op->target_estate_id||op->target_estate_id==UINT32_MAX||
     (op->target_estate_id!=load.yard_estate_id&&op->target_estate_id!=load.house_estate_id))return {{},Error::InvalidArgument};
  if(op->source_estate_id!=UINT32_MAX||op->native_item_id!=UINT32_MAX){
   if(!op->source_estate_id||!op->native_item_id||op->source_estate_id==UINT32_MAX||op->native_item_id==UINT32_MAX||
      op->source_kind!=0xff||op->source_slots!=std::array<uint16_t,3>{0xffff,0xffff,0xffff})return {{},Error::InvalidArgument};
   continue; // Property source authority belongs to the mixed coordinator.
  }
  auto slot=ResolveSlot(inv,op->source_kind,op->source_slots);if(!slot)return {{},Error::UnsupportedItem};
  const auto found=rows.find(*slot);auto* item=inv.GetItem(int16_t(*slot));
  if(found==rows.end()||!item||op->guid!=NativeGuid(item->GetSerialNumber()))return {{},Error::Conflict};
  const auto& row=*found->second;
  if(!InventoryProjection::Matches(row,*item)||!fence.LayoutSourceMatches(row,item->GetSerialNumber()))return {{},Error::UnheldFence};
  auto prior=selected.find(*slot);size_t index;
  if(prior==selected.end()){
   const auto* data=item->GetItem();Source source;source.before=row;source.original=item;source.serial=item->GetSerialNumber();source.stackable=data->Stackable;
   if(!serials.insert(source.serial).second)return {{},Error::Conflict};
   if(source.stackable){
    if(data->StackSize<1||item->GetCharges()<1||item->GetCharges()>data->StackSize)return {{},Error::UnsupportedItem};
    source.available=uint32_t(item->GetCharges());
   }else source.available=1; // Charged nonstackable items are one instance.
   if(op->source_slots[1]!=0xffff){source.parent=inv.GetItem(int16_t(op->source_slots[0]));source.parent_serial=source.parent->GetSerialNumber();if(source.parent_serial<=0)return {{},Error::UnsupportedItem};}
   index=plan.sources.size();selected.emplace(*slot,index);plan.sources.push_back(std::move(source));
  }else index=prior->second;
  auto& source=plan.sources[index];if(source.original!=item||source.serial!=item->GetSerialNumber()||source.consumed>=source.available)return {{},Error::Conflict};
  ++source.consumed;plan.uses.push_back({ordinal,index});
 }
 for(auto& source:plan.sources){
  if(source.consumed<source.available){
   source.after=source.before;source.after->values.charges=uint16_t(source.available-source.consumed);
   source.after->null_fields&=~LiveItems::NullCharges;
  }
 }
 return {std::move(plan),Error::None};
}
inline bool ParentMatches(const Source& source,EQ::InventoryProfile& inv){
 if(!source.parent)return true;const auto root=EQ::InventoryProfile::CalcSlotId(int16_t(source.before.values.slot_id));
 const auto* parent=root<0?nullptr:inv.GetItem(root);return parent==source.parent&&parent&&parent->GetSerialNumber()==source.parent_serial;
}
inline bool BeforeMatches(const Plan& plan,const LiveItems::Lease& lease,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence){
 if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))return false;
 for(const auto& source:plan.sources){auto* item=inv.GetItem(int16_t(source.before.values.slot_id));
  if(source.before.values.character_id!=lease.actor.character_id||!SlotSupported(inv,source.before.values.slot_id)||!ParentMatches(source,inv)||item!=source.original||!item||item->GetSerialNumber()!=source.serial||
     !InventoryProjection::Matches(source.before,*item)||!fence.LayoutSourceMatches(source.before,source.serial))return false;
 }return true;
}
inline bool AfterMatches(const Plan& plan,const LiveItems::Lease& lease,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence){
 if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))return false;
 for(const auto& source:plan.sources){
  // Deleting a cursor item requires an empty queue, not advancement to another
  // item which would silently change the selected source during the batch.
  const auto root=source.parent?EQ::InventoryProfile::CalcSlotId(int16_t(source.before.values.slot_id)):int16_t(source.before.values.slot_id);
  if(source.before.values.character_id!=lease.actor.character_id||root<0||!RootSupported(inv,uint16_t(root)))return false;
  if(source.parent&&!SlotSupported(inv,source.before.values.slot_id))return false;
  if(root==33&&inv.CursorSize()!=(source.after||source.parent?1:0))return false;
  if(source.parent&&!ParentMatches(source,inv))return false;
  auto* item=inv.GetItem(int16_t(source.before.values.slot_id));
  if(source.after){if(!item||item!=source.original||item->GetSerialNumber()!=source.serial||!InventoryProjection::Matches(*source.after,*item))return false;}
  else if(item)return false;
 }return true;
}
}
#endif
