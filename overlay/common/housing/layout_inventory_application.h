#ifndef EQEMU_HOUSING_LAYOUT_INVENTORY_APPLICATION_H
#define EQEMU_HOUSING_LAYOUT_INVENTORY_APPLICATION_H
#include "layout_inventory_journal_db.h"
namespace Housing::LayoutInventoryApplication {
using Error=LiveItems::Error;
using Retained=std::map<uint64_t,std::unique_ptr<EQ::ItemInstance>>;
// Owned by the real exclusive inventory bridge until ACK is known committed.
// Keeps popped originals alive through partial application/uncertain ACK.
struct State {
 LayoutInventory::Plan plan;LayoutInventory::Prepared prepared;
 std::vector<std::unique_ptr<EQ::ItemInstance>> retired;
 std::vector<EQ::ItemInstance*> pieces;
 CustodyCodec::Bytes binding,outcome;bool initialized=false;
};
namespace Detail {
inline bool PlanBound(const LayoutInventory::Outcome& outcome,const State& state){
 if(state.plan.sources.size()!=outcome.sources.size()||state.plan.uses.size()!=outcome.pieces.size())return false;
 std::vector<const LayoutInventory::SourceChange*> changes(outcome.sources.size());for(const auto& c:outcome.inventory.changes){if(c.source_index>=changes.size()||changes[c.source_index])return false;changes[c.source_index]=&c;}
 for(size_t i=0;i<state.plan.sources.size();++i){const auto& s=state.plan.sources[i];const auto& facts=outcome.sources[i];if(!changes[i]||s.serial!=facts.runtime_serial||s.available!=facts.available||s.consumed!=facts.consumed||s.stackable!=facts.stackable||CustodyCodec::Encode(s.before)!=CustodyCodec::Encode(changes[i]->before)||bool(s.after)!=bool(changes[i]->after)||(s.after&&CustodyCodec::Encode(*s.after)!=CustodyCodec::Encode(*changes[i]->after)))return false;}
 for(size_t i=0;i<state.plan.uses.size();++i)if(state.plan.uses[i].source_index!=outcome.pieces[i].source_index||state.plan.uses[i].operation_index!=outcome.pieces[i].operation_index)return false;return true;
}
inline bool After(const LayoutInventory::Source& s,const std::unique_ptr<EQ::ItemInstance>& retired,EQ::InventoryProfile& inv){
 const auto slot=s.before.values.slot_id;const auto root=s.parent?EQ::InventoryProfile::CalcSlotId(int16_t(slot)):int16_t(slot);
 if(root<0||!LayoutInventory::RootSupported(inv,uint16_t(root))||!LayoutInventory::ParentMatches(s,inv)||(s.parent&&!LayoutInventory::SlotSupported(inv,slot))||(root==33&&inv.CursorSize()!=(s.after||s.parent?1:0)))return false;
 auto* item=inv.GetItem(int16_t(slot));
 if(s.after)return !retired&&item==s.original&&item&&item->GetSerialNumber()==s.serial&&InventoryProjection::Matches(*s.after,*item);
 return !item&&retired.get()==s.original&&retired&&retired->GetSerialNumber()==s.serial&&InventoryProjection::Matches(s.before,*retired);
}
inline bool Before(const LayoutInventory::Source& s,const std::unique_ptr<EQ::ItemInstance>& retired,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence){
 auto* item=inv.GetItem(int16_t(s.before.values.slot_id));return !retired&&LayoutInventory::SlotSupported(inv,s.before.values.slot_id)&&LayoutInventory::ParentMatches(s,inv)&&item==s.original&&item&&item->GetSerialNumber()==s.serial&&InventoryProjection::Matches(s.before,*item)&&fence.LayoutSourceMatches(s.before,s.serial);
}
}
inline bool Applied(const LayoutInventoryJournal::Binding& binding,const LayoutInventory::Outcome& outcome,const State& state,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence,const Retained& retained)noexcept{
 try{
  if(!state.initialized||!Detail::PlanBound(outcome,state)||state.binding!=LayoutInventoryJournal::Detail::Canonical(binding)||state.outcome!=LayoutInventory::EncodeOutcome(outcome)||!fence.HeldFor(binding.lease.actor,binding.lease.epoch,binding.lease.session_key)||state.retired.size()!=state.plan.sources.size()||state.pieces.size()!=outcome.pieces.size())return false;
  for(size_t i=0;i<state.plan.sources.size();++i)if(!Detail::After(state.plan.sources[i],state.retired[i],inv))return false;
  for(size_t i=0;i<outcome.pieces.size();++i){const auto& piece=outcome.pieces[i];auto found=retained.find(piece.placement.stored.custody_id);if(found==retained.end()||!found->second||found->second.get()!=state.pieces[i]||found->second->GetSerialNumber()!=piece.runtime_serial||!InventoryProjection::Matches(piece.placement.stored.row,*found->second))return false;}
  return true;
 }catch(...){return false;}
}
// Internal memory phase. The enclosing ApplyCommitted below first resolves a
// journal on a connection with NO prior TX, so staged SQL can never authorize
// memory application. No source SQL writes or client packets occur here.
inline bool ApplyMemory(const LayoutInventoryJournal::Binding& binding,const LayoutInventory::Outcome& outcome,const LayoutInventory::Plan& plan,LayoutInventory::Prepared& prepared,State& state,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence,Retained& retained){
 if(!fence.HeldFor(binding.lease.actor,binding.lease.epoch,binding.lease.session_key))return false;
 const auto canonical=LayoutInventoryJournal::Detail::Canonical(binding);const auto encoded=LayoutInventory::EncodeOutcome(outcome);
 if(!state.initialized){
  if(!LayoutInventory::PreparedMatches(plan,prepared,binding.lease,inv,fence))return false;
  std::vector<LiveItems::LayoutUnitPlacement> placements;for(const auto& p:outcome.pieces)placements.push_back(p.placement);
  if(LayoutInventory::EncodeOutcome(LayoutInventory::MakeOutcome(plan,prepared,outcome.inventory,placements))!=encoded)return false;
  for(const auto& p:outcome.pieces){auto found=retained.find(p.placement.stored.custody_id);if(found!=retained.end()&&found->second)return false;}
  // Allocate all bridge bookkeeping before mutating a single inventory source.
  state.plan=plan;state.retired.resize(plan.sources.size());state.pieces.clear();state.pieces.reserve(prepared.units.size());for(const auto& unit:prepared.units)state.pieces.push_back(unit.item.get());
  for(const auto& p:outcome.pieces)retained.try_emplace(p.placement.stored.custody_id,nullptr);
  state.binding=canonical;state.outcome=encoded;state.prepared=std::move(prepared);state.initialized=true;
 }else if(state.binding!=canonical||state.outcome!=encoded)return false;
 if(!Detail::PlanBound(outcome,state)||state.prepared.units.size()!=outcome.pieces.size()||state.retired.size()!=state.plan.sources.size()||state.pieces.size()!=outcome.pieces.size())return false;
 // Check the ENTIRE mixed before/after memory state before continuing a retry.
 for(size_t i=0;i<state.plan.sources.size();++i){const auto& s=state.plan.sources[i];if(!Detail::Before(s,state.retired[i],inv,fence)&&!Detail::After(s,state.retired[i],inv))return false;}
 for(size_t i=0;i<outcome.pieces.size();++i){const auto& p=outcome.pieces[i];const auto found=retained.find(p.placement.stored.custody_id);if(found==retained.end())return false;auto* item=found->second?found->second.get():state.prepared.units[i].item.get();if(!item||item!=state.pieces[i]||item->GetSerialNumber()!=p.runtime_serial||!InventoryProjection::Matches(p.placement.stored.row,*item)||(found->second&&state.prepared.units[i].item))return false;}
 for(size_t i=0;i<state.plan.sources.size();++i){const auto& s=state.plan.sources[i];if(Detail::After(s,state.retired[i],inv))continue;
  if(s.after)s.original->SetCharges(int16_t(s.after->values.charges));else{state.retired[i].reset(inv.PopItem(int16_t(s.before.values.slot_id)));if(state.retired[i].get()!=s.original)return false;}
 }
 for(size_t i=0;i<outcome.pieces.size();++i){auto& destination=retained.at(outcome.pieces[i].placement.stored.custody_id);if(!destination)destination=std::move(state.prepared.units[i].item);}
 return Applied(binding,outcome,state,inv,fence,retained);
}
inline LiveItems::Result<bool> AcknowledgeUnderTransaction(Database& db,bool enabled,const LayoutInventoryJournal::Binding& binding,const State& state,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence,const Retained& retained){
 using namespace LayoutInventoryJournal;
 auto receipt=LayoutInventoryJournal::ReadUnderTransaction(db,enabled,binding,fence);if(!receipt)return {false,receipt.error};if(receipt.value.applied)return {true,Error::None};
 if(!Applied(binding,receipt.value.outcome,state,inv,fence,retained))return {false,Error::UnheldFence};
 try{auto marked=LayoutInventoryJournal::Detail::Q(db,"UPDATE housing_layout_inventory_batches SET applied=1 WHERE request_key="+LayoutInventoryJournal::Detail::Literal(binding.request_key)+" AND applied=0");auto cleared=LayoutInventoryJournal::Detail::Q(db,"UPDATE housing_live_item_leases SET pending_key=NULL WHERE character_id="+std::to_string(binding.lease.actor.character_id)+" AND pending_key="+LayoutInventoryJournal::Detail::Literal(binding.request_key));if(marked.RowsAffected()!=1||cleared.RowsAffected()!=1)return {false,Error::Conflict};return {true,Error::None};}catch(const LayoutInventoryJournal::Detail::Failure& e){return {false,e.error};}catch(...){return {false,Error::DatabaseFailure};}
}
// Application owner must keep the actual inventory fence and State alive on
// uncertain COMMIT and resolve using a fresh connection before any other save.
inline LiveItems::Result<LayoutInventoryJournal::Receipt> ApplyCommitted(Database& db,bool enabled,const LayoutInventoryJournal::Binding& binding,const LayoutInventory::Plan& plan,LayoutInventory::Prepared& prepared,State& state,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence,Retained& retained){
 using Error=LiveItems::Error;using namespace LayoutInventoryJournal;bool active=false,commit=false;
 if(!enabled)return {{},Error::Disabled};
 try{
  auto tx=LayoutInventoryJournal::Detail::Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="0")return {{},Error::InvalidArgument};
  LayoutInventoryJournal::Detail::Q(db,"START TRANSACTION");active=true;auto receipt=LayoutInventoryJournal::ReadUnderTransaction(db,true,binding,fence);if(!receipt){LayoutInventoryJournal::Detail::Q(db,"ROLLBACK");return receipt;}
  if(!receipt.value.applied){if(!ApplyMemory(binding,receipt.value.outcome,plan,prepared,state,inv,fence,retained)){LayoutInventoryJournal::Detail::Q(db,"ROLLBACK");return {{},Error::UnheldFence};}auto ack=AcknowledgeUnderTransaction(db,true,binding,state,inv,fence,retained);if(!ack){LayoutInventoryJournal::Detail::Q(db,"ROLLBACK");return {{},ack.error};}receipt.value.applied=true;}
  commit=true;LayoutInventoryJournal::Detail::Q(db,"COMMIT");active=false;return receipt;
 }catch(const LayoutInventoryJournal::Detail::Failure& e){if(commit)return {{},Error::CommitUncertain};if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},e.error};}catch(...){if(commit)return {{},Error::CommitUncertain};if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
}
#endif
