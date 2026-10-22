#include "layout_inventory_units.h"
#include "inventory_row_db.h"
#include "item_payload_db.h"
#include "../database.h"
namespace Housing::LayoutInventory {
namespace {
using Error=LiveItems::Error;struct Failure{Error error;};
MySQLRequestResult Q(Database& db,const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())throw Failure{Error::DatabaseFailure};return result;}
void Metadata(Database& db,EQ::ItemInstance& item,bool parent){
 const auto* data=item.GetItem();if(!data)throw Failure{Error::UnsupportedItem};
 auto rows=Q(db,"SELECT itemclass,bagslots,stackable,stacksize,UNK226,UNK227 FROM items WHERE id="+std::to_string(data->ID)+" FOR UPDATE");
 if(rows.RowCount()!=1)throw Failure{Error::UnsupportedItem};auto row=rows.begin();for(size_t i=0;i<6;++i)if(!row[i])throw Failure{Error::UnsupportedItem};
 if(std::stoll(row[0])!=data->ItemClass||std::stoll(row[1])!=data->BagSlots||std::stoll(row[2])!=int(data->Stackable)||(data->Stackable&&std::stoll(row[3])!=data->StackSize)||std::stoll(row[4])!=data->HousingPlacementType||std::stoll(row[5])!=data->HousingDefinitionID)throw Failure{Error::Conflict};
 if(parent){if(data->ItemClass!=EQ::item::ItemClassBag||!data->BagSlots)throw Failure{Error::UnsupportedItem};}

}
}
LiveItems::Result<SourceReview> ReviewSourcesUnderTransaction(Database& db,bool enabled,const LiveItems::Lease& lease,
 const Plan& plan,const Prepared& prepared,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence){
 if(!enabled)return {{},Error::Disabled};
 if(!PreparedMatches(plan,prepared,lease,inv,fence))return {{},Error::UnheldFence};
 if(plan.sources.empty())return {{},Error::InvalidArgument};
 auto authenticated=LiveItems::Service::AuthenticateUnderTransaction(db,true,lease,fence);if(!authenticated)return {{},authenticated.error};
 if(!authenticated.value.pending_key.empty())return {{},Error::PendingApplication};
 try{
  std::map<uint32_t,const Source*> source_slots;std::map<uint32_t,EQ::ItemInstance*> parent_slots;std::set<uint32_t> slots;
  for(const auto& source:plan.sources){if(!source_slots.emplace(source.before.values.slot_id,&source).second)throw Failure{Error::Conflict};slots.insert(source.before.values.slot_id);
   if(source.parent){const auto root=EQ::InventoryProfile::CalcSlotId(int16_t(source.before.values.slot_id));if(root<0)throw Failure{Error::Conflict};auto parent=parent_slots.emplace(uint32_t(root),source.parent);if(!parent.second&&parent.first->second!=source.parent)throw Failure{Error::Conflict};slots.insert(uint32_t(root));}
  }
  SourceReview review;review.locked_rows.reserve(slots.size());
  // One canonical physical-slot order, including bags before their contents.
  for(auto slot:slots){
   auto rows=Q(db,"SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(lease.actor.character_id)+" AND slot_id="+std::to_string(slot)+" FOR UPDATE");
   if(rows.RowCount()!=1)throw Failure{Error::Conflict};auto current=InventoryRowDB::Decode(rows.begin());
   const auto source=source_slots.find(slot);
   if(source!=source_slots.end()){
    current=ItemPayload::Complete(db,std::move(current),&source->second->before);ItemPayload::ValidateInventory(db,current);ItemPayload::Metadata(db,current);
    if(CustodyCodec::Encode(current)!=CustodyCodec::Encode(source->second->before))throw Failure{Error::Conflict};
    Metadata(db,*source->second->original,false);
    // A top-level ordinary source must not leave SQL-only children behind.
    // Bag-contained sources have no supported nested container address space.
    if(RootSupported(inv,uint16_t(slot))){
     const auto first=EQ::InventoryProfile::CalcSlotId(int16_t(slot),0);
     const auto last=EQ::InventoryProfile::CalcSlotId(int16_t(slot),EQ::invbag::SLOT_COUNT-1);
     if(first<0||last<first)throw Failure{Error::UnsupportedItem};
     auto children=Q(db,"SELECT slot_id FROM inventory WHERE character_id="+std::to_string(lease.actor.character_id)+" AND slot_id BETWEEN "+std::to_string(first)+" AND "+std::to_string(last)+" FOR UPDATE");
     if(children.RowCount())throw Failure{Error::UnsupportedItem};
    }

   }else{
    const auto parent=parent_slots.find(slot);if(parent==parent_slots.end()||!parent->second->GetItem()||current.values.item_id!=parent->second->GetItem()->ID)throw Failure{Error::Conflict};
    Metadata(db,*parent->second,true);
   }
   review.locked_rows.push_back(std::move(current));
  }return {std::move(review),Error::None};
 }catch(const Failure& failure){return {{},failure.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Consumption> ConsumeSourcesUnderTransaction(Database& db,bool enabled,const LiveItems::Lease& lease,
 const Plan& plan,const Prepared& prepared,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence){
 auto review=ReviewSourcesUnderTransaction(db,enabled,lease,plan,prepared,inv,fence);if(!review)return {{},review.error};
 try{
  std::map<uint32_t,size_t> ordered;for(size_t i=0;i<plan.sources.size();++i)if(!ordered.emplace(plan.sources[i].before.values.slot_id,i).second)throw Failure{Error::Conflict};
  Consumption out;out.review=std::move(review.value);out.changes.reserve(ordered.size());
  for(const auto& [slot,index]:ordered){const auto& source=plan.sources[index];
   const auto where="character_id="+std::to_string(lease.actor.character_id)+" AND slot_id="+std::to_string(slot);
   // Preflight holds every source and parent lock. Recheck the complete raw
   // preimage immediately before its CAS, preserving nullable/binary fields.
   auto rows=Q(db,"SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE "+where+" FOR UPDATE");
   if(rows.RowCount()!=1||CustodyCodec::Encode(ItemPayload::Complete(db,InventoryRowDB::Decode(rows.begin()),&source.before))!=CustodyCodec::Encode(source.before))throw Failure{Error::Conflict};
   const auto charge_guard=(source.before.null_fields&LiveItems::NullCharges)?" AND charges IS NULL":" AND charges="+std::to_string(source.before.values.charges);
   auto changed=Q(db,source.after?"UPDATE inventory SET charges="+std::to_string(source.after->values.charges)+" WHERE "+where+charge_guard:"DELETE FROM inventory WHERE "+where+charge_guard);
   if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};
   out.changes.push_back({index,source.before,source.after});
  }return {std::move(out),Error::None};
 }catch(const Failure& failure){return {{},failure.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<bool> ValidateSourcePostimagesUnderTransaction(Database& db,const LiveItems::Lease& lease,const Consumption& consumed){
 if(!lease.actor.character_id||!lease.actor.account_id||!lease.epoch||lease.session_key.empty()||consumed.changes.empty()||consumed.changes.size()>UINT16_MAX||consumed.review.locked_rows.size()>2*UINT16_MAX)return {false,Error::InvalidArgument};
 try{
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return {false,Error::InvalidArgument};
  std::map<uint32_t,const SourceChange*> changes;std::set<size_t> indices;std::map<uint32_t,const LiveItems::InventoryRow*> reviewed;
  for(const auto& row:consumed.review.locked_rows){if(row.values.character_id!=lease.actor.character_id||!CustodyCodec::Valid(row)||!reviewed.emplace(row.values.slot_id,&row).second)return {false,Error::InvalidArgument};}
  for(const auto& change:consumed.changes){const auto& before=change.before;
   if(change.source_index>=consumed.changes.size()||before.values.character_id!=lease.actor.character_id||!CustodyCodec::Valid(before)||!indices.insert(change.source_index).second||!changes.emplace(before.values.slot_id,&change).second)return {false,Error::InvalidArgument};
   const auto prior=reviewed.find(before.values.slot_id);if(prior==reviewed.end()||CustodyCodec::Encode(*prior->second)!=CustodyCodec::Encode(before))return {false,Error::InvalidArgument};
   if(change.after){auto expected=before;const auto count=change.after->values.charges;const auto original=(before.null_fields&LiveItems::NullCharges)||!before.values.charges?1:before.values.charges;
    if(!count||count>=original)return {false,Error::InvalidArgument};expected.values.charges=count;expected.null_fields&=~LiveItems::NullCharges;
    if(CustodyCodec::Encode(expected)!=CustodyCodec::Encode(*change.after))return {false,Error::InvalidArgument};
   }
  }
  // Same canonical source+parent order as source review/consumption. Parent
  // rows remain exact; a matching child does not excuse a replaced bag.
  for(const auto& [slot,before]:reviewed){
   const auto changed=changes.find(slot);const auto* expected=before;
   if(changed!=changes.end())expected=changed->second->after?&*changed->second->after:nullptr;
   auto rows=Q(db,"SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(lease.actor.character_id)+" AND slot_id="+std::to_string(slot)+" FOR UPDATE");
   if(!expected){if(rows.RowCount())return {false,Error::Conflict};}
   else if(rows.RowCount()!=1||CustodyCodec::Encode(changed==changes.end()?InventoryRowDB::Decode(rows.begin()):ItemPayload::Complete(db,InventoryRowDB::Decode(rows.begin()),expected))!=CustodyCodec::Encode(*expected))return {false,Error::Conflict};
  }
  return {true,Error::None};
 }catch(const Failure& failure){return {false,failure.error};}catch(...){return {false,Error::DatabaseFailure};}
}

}
