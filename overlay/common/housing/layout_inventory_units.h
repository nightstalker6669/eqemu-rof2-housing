#ifndef EQEMU_HOUSING_LAYOUT_INVENTORY_UNITS_H
#define EQEMU_HOUSING_LAYOUT_INVENTORY_UNITS_H
#include "layout_inventory_plan.h"
#include "custody_codec.h"
#include "layout_retained_token_types.h"
#include "npc_item_metadata.h"
namespace Housing::LayoutInventory {
struct Unit {
 size_t operation_index=0,source_index=0;
 LiveItems::InventoryRow row; // New piece identity; original row remains in Plan.
 std::unique_ptr<EQ::ItemInstance> item;
};
struct Prepared {std::vector<Unit> units;std::vector<LayoutRetained::Witness> retained_tokens;};
inline bool Coherent(const Plan& plan){
 if(plan.uses.size()>UINT16_MAX||plan.sources.size()>UINT16_MAX)return false;
 std::vector<uint32_t> counts(plan.sources.size());
 for(size_t i=0;i<plan.uses.size();++i){const auto& use=plan.uses[i];
  if(use.source_index>=counts.size()||use.operation_index>=UINT16_MAX||(i&&plan.uses[i-1].operation_index>=use.operation_index))return false;
  ++counts[use.source_index];
 }
 for(size_t i=0;i<plan.sources.size();++i){const auto& source=plan.sources[i];const auto* data=source.original?source.original->GetItem():nullptr;
  if(!data||source.stackable!=data->Stackable||!counts[i]||counts[i]!=source.consumed||source.consumed>source.available)return false;
  if(source.stackable){if(data->StackSize<1||source.original->GetCharges()<1||source.original->GetCharges()>data->StackSize||source.available!=uint32_t(source.original->GetCharges()))return false;}
  else if(source.available!=1)return false;
  std::optional<LiveItems::InventoryRow> expected;
  if(source.consumed<source.available){expected=source.before;expected->values.charges=uint16_t(source.available-source.consumed);expected->null_fields&=~LiveItems::NullCharges;}
  if(bool(expected)!=bool(source.after)||(expected&&CustodyCodec::Encode(*expected)!=CustodyCodec::Encode(*source.after)))return false;
 }return true;
}
inline LiveItems::InventoryRow PieceRow(const Source& source,int32_t serial){
 auto row=source.before;row.values.guid=uint32_t(serial);row.null_fields&=~LiveItems::NullGuid;
 if(source.stackable){row.values.charges=1;row.null_fields&=~LiveItems::NullCharges;}return row;
}
inline bool OrdinarySource(const Source& source){
 const auto* item=source.original;const auto* data=item?item->GetItem():nullptr;
 // House/crate identity follows its existing ledger/application path; cloning
 // those properties into a second token would duplicate a unique binding.
 return data&&NpcItemMetadata::Ordinary(*data)&&!data->HousingDefinitionID&&!data->HousingPlacementType&&!item->GetHousingHouseID()&&!item->GetHousingCrateID()&&item->GetHousingEstateID()==UINT32_MAX&&!item->IsScaling();
}
inline LiveItems::Result<Prepared> PrepareOrdinary(const Plan& plan,const LiveItems::Lease& lease,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence){
 using Error=LiveItems::Error;
 if(!BeforeMatches(plan,lease,inv,fence))return {{},Error::UnheldFence};
 if(!Coherent(plan))return {{},Error::InvalidArgument};
 try{
  std::set<int32_t> serials;
  for(const auto& source:plan.sources){
   if(!OrdinarySource(source))return {{},Error::UnsupportedItem};
   serials.insert(source.serial);EQ::ItemInstance::AddGUIDToMap(source.serial);
   if(!(source.before.null_fields&LiveItems::NullGuid)&&source.before.values.guid)EQ::ItemInstance::AddGUIDToMap(source.before.values.guid);
   if(source.parent_serial>0)EQ::ItemInstance::AddGUIDToMap(source.parent_serial);
  }
  Prepared out;out.units.reserve(plan.uses.size());
  for(const auto& use:plan.uses){const auto& source=plan.sources[use.source_index];
   Unit unit;unit.operation_index=use.operation_index;unit.source_index=use.source_index;unit.item.reset(source.original->CloneWithNewSerial());
   if(!unit.item||unit.item->GetSerialNumber()<=0||!serials.insert(unit.item->GetSerialNumber()).second)return {{},Error::Conflict};
   if(source.stackable)unit.item->SetCharges(1);
   unit.row=PieceRow(source,unit.item->GetSerialNumber());
   if(!InventoryProjection::Matches(unit.row,*unit.item))return {{},Error::UnsupportedItem};
   out.units.push_back(std::move(unit));
  }return {std::move(out),Error::None};
 }catch(...){return {{},Error::DatabaseFailure};}
}
inline bool PreparedMatches(const Plan& plan,const Prepared& prepared,const LiveItems::Lease& lease,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence){
 if(!BeforeMatches(plan,lease,inv,fence)||!Coherent(plan)||prepared.units.size()!=plan.uses.size())return false;
 std::set<int32_t> serials;for(const auto& source:plan.sources)serials.insert(source.serial);
 for(size_t i=0;i<plan.uses.size();++i){const auto& use=plan.uses[i];const auto& unit=prepared.units[i];const auto& source=plan.sources[use.source_index];
  if(!unit.item||!OrdinarySource(source)||unit.operation_index!=use.operation_index||unit.source_index!=use.source_index||unit.item->GetSerialNumber()<=0||!serials.insert(unit.item->GetSerialNumber()).second)return false;
  if(CustodyCodec::Encode(unit.row)!=CustodyCodec::Encode(PieceRow(source,unit.item->GetSerialNumber()))||!InventoryProjection::Matches(unit.row,*unit.item))return false;
  if(unit.item->GetHousingHouseID()||unit.item->GetHousingCrateID()||unit.item->GetHousingEstateID()!=UINT32_MAX)return false;
 }return true;
}
// Read-only source/parent SQL preflight; locks remain with caller's transaction.
// Does NOT consume rows, create custody or mark a batch applied. Actual mutation
// must join the aggregate pending/journal/application/recovery contract.
struct SourceReview {std::vector<LiveItems::InventoryRow> locked_rows;};
LiveItems::Result<SourceReview> ReviewSourcesUnderTransaction(Database&,bool enabled,const LiveItems::Lease&,
 const Plan&,const Prepared&,EQ::InventoryProfile&,LiveItems::InventorySessionFence&);
struct SourceChange {size_t source_index=0;LiveItems::InventoryRow before;std::optional<LiveItems::InventoryRow> after;};
struct Consumption {SourceReview review;std::vector<SourceChange> changes;};
// INTERNAL mutation hook, not a callable layout endpoint or completed batch.
// Caller MUST include authorized unit custody/placements, the aggregate outcome
// and pending marker in this SAME transaction before COMMIT, then apply/ACK the
// exact memory postimage. No production caller is enabled until that exists.
// Failure may follow earlier writes: caller must roll back the ENTIRE batch.
// Repeat/uncertain-commit resolution belongs to the immutable parent journal.
LiveItems::Result<Consumption> ConsumeSourcesUnderTransaction(Database&,bool enabled,const LiveItems::Lease&,
 const Plan&,const Prepared&,EQ::InventoryProfile&,LiveItems::InventorySessionFence&);
// Inventory component of aggregate commit/restart validation ONLY. The caller
// must first authenticate the immutable batch outcome and (for restart) the
// certified writer death. This grants no lease, placement, ACK or recovery
// release authority and never starts/ends a transaction. Locks source afterrows
// and unchanged bag parents; compares every raw field, including NULL/binary.
LiveItems::Result<bool> ValidateSourcePostimagesUnderTransaction(Database&,const LiveItems::Lease&,const Consumption&);

}
#endif
