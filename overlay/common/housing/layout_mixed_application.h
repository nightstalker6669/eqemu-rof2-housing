#ifndef EQEMU_HOUSING_LAYOUT_MIXED_APPLICATION_H
#define EQEMU_HOUSING_LAYOUT_MIXED_APPLICATION_H
#include "layout_mixed_journal_db.h"
#include "layout_inventory_application.h"
namespace Housing::LayoutMixedApplication {
using Error=LiveItems::Error;using Binding=LayoutMixedJournal::Binding;
using Retained=LayoutInventoryApplication::Retained;
// Trusted main-thread owner slots, not borrowed ItemInstance pointers. Native
// scene cache nodes must remain alive and unmoved until ACK is durably known.
// A slot may alias the bridge retained map's slot; duplicate ownership is never
// created. New split pieces enter retained custody before scene refresh.
using PropertyOwners=std::map<uint64_t,std::unique_ptr<EQ::ItemInstance>*>;
struct Facts {LiveItems::InventoryRow initial;std::optional<LiveItems::InventoryRow> final;bool created=false;int32_t created_serial=0;LayoutCrate::Allocation crate;LayoutHouse::Identity house;LayoutCrateMember::Proof member;};
struct Entry {
 uint64_t custody=0;Facts facts;
 std::unique_ptr<EQ::ItemInstance>* scene=nullptr;
 EQ::ItemInstance* pointer=nullptr;int32_t serial=0;
 std::unique_ptr<EQ::ItemInstance>* cache=nullptr;
 EQ::ItemInstance* cache_pointer=nullptr;int32_t cache_serial=0;
 std::unique_ptr<EQ::ItemInstance> retired_scene,retired_cache;
};
struct State {
 LayoutInventoryApplication::State inventory;
 LayoutProperty::State property;
 std::vector<Entry> entries;
 std::vector<LayoutRetained::Witness> retained_tokens;
 CustodyCodec::Bytes binding,outcome;
 bool initialized=false;
};
namespace Detail {
inline std::map<uint64_t,Facts> Fold(const LayoutMixed::Outcome& outcome){
 LayoutMixed::Detail::Shape(outcome);std::map<uint64_t,Facts> result;if(!outcome.property)return result;
 for(const auto& step:outcome.property->steps)for(const auto& d:step.changes){const auto& snapshot=d.before?*d.before:*d.after;const auto id=snapshot.placement.stored.custody_id;auto [entry,inserted]=result.emplace(id,Facts{snapshot.placement.stored.row,{},!d.before,d.before?0:snapshot.runtime_serial,snapshot.crate,snapshot.house,snapshot.member});entry->second.final=d.after?std::optional<LiveItems::InventoryRow>(d.after->placement.stored.row):std::nullopt;}
 return result;
}
inline bool Same(const Facts& a,const Facts& b){return a.created==b.created&&a.created_serial==b.created_serial&&LayoutCrate::Same(a.crate,b.crate)&&LayoutHouse::Same(a.house,b.house)&&LayoutCrateMember::Same(a.member,b.member)&&CustodyCodec::Encode(a.initial)==CustodyCodec::Encode(b.initial)&&bool(a.final)==bool(b.final)&&(!a.final||CustodyCodec::Encode(*a.final)==CustodyCodec::Encode(*b.final));}
inline bool Match(const LiveItems::InventoryRow& row,EQ::ItemInstance* pointer,int32_t serial){return pointer&&pointer->GetSerialNumber()==serial&&InventoryProjection::Matches(row,*pointer);}
inline bool Before(const Entry& e,bool cache=false){auto* slot=cache?e.cache:e.scene;auto* pointer=cache?e.cache_pointer:e.pointer;const auto serial=cache?e.cache_serial:e.serial;const auto& retired=cache?e.retired_cache:e.retired_scene;if(!slot)return cache;return !retired&&slot->get()==pointer&&Match(e.facts.initial,pointer,serial)&&LayoutCrate::Matches(e.facts.crate,pointer)&&LayoutHouse::Matches(e.facts.house,pointer);}
inline bool After(const Entry& e,bool cache=false){auto* slot=cache?e.cache:e.scene;auto* pointer=cache?e.cache_pointer:e.pointer;const auto serial=cache?e.cache_serial:e.serial;const auto& retired=cache?e.retired_cache:e.retired_scene;if(!slot)return cache;
 if(e.facts.final)return !retired&&slot->get()==pointer&&Match(*e.facts.final,pointer,serial)&&LayoutCrate::Matches(e.facts.crate,pointer)&&LayoutHouse::Matches(e.facts.house,pointer);
 return !*slot&&retired.get()==pointer&&Match(e.facts.initial,retired.get(),serial);
}
inline bool OwnersBound(const Entry& e,const PropertyOwners& owners,const Retained& retained){
 if(!e.cache&&(e.cache_pointer||e.cache_serial||e.retired_cache))return false;
 auto held=retained.find(e.custody);if(e.facts.created){if(held==retained.end()||e.scene!=&held->second||e.cache)return false;}
 else{auto owner=owners.find(e.custody);if(owner==owners.end()||owner->second!=e.scene)return false;if(e.cache){if(held==retained.end()||e.cache!=&held->second)return false;}else if(held!=retained.end()&&&held->second!=e.scene&&held->second)return false;}
 return true;
}
inline bool TokensBound(const LayoutMixed::Outcome& outcome,const std::vector<LayoutRetained::Witness>& witnesses){if(outcome.retained_tokens.size()!=witnesses.size())return false;for(size_t i=0;i<witnesses.size();++i)if(!LayoutRetained::Matches(outcome.retained_tokens[i],witnesses[i]))return false;return true;}
inline bool TokenBefore(const LayoutRetained::Witness& w,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence){return LayoutInventory::SlotSupported(inv,w.row.values.slot_id)&&inv.GetItem(int16_t(w.row.values.slot_id))==w.original&&fence.LayoutSourceMatches(w.row,w.serial);}
inline bool TokenAfter(const LayoutRetained::Outcome& t,const LayoutRetained::Witness& w,EQ::InventoryProfile& inv,const Retained& retained){auto i=retained.find(t.after.placement.stored.custody_id);return !inv.GetItem(int16_t(w.row.values.slot_id))&&(w.row.values.slot_id!=33||!inv.CursorSize())&&i!=retained.end()&&i->second.get()==w.original&&LayoutRetained::Matches(t,w);}
inline bool Bound(const Binding& binding,const LayoutMixed::Outcome& outcome,const State& state,const PropertyOwners& owners,const Retained& retained){
 if(!TokensBound(outcome,state.retained_tokens)||!state.initialized||state.binding!=LayoutMixedJournal::Detail::Canonical(binding)||state.outcome!=LayoutMixed::Encode(outcome))return false;const auto facts=Fold(outcome);if(facts.size()!=state.entries.size())return false;std::set<uint64_t> ids;
 for(const auto& e:state.entries){auto expected=facts.find(e.custody);if(!ids.insert(e.custody).second||expected==facts.end()||!Same(e.facts,expected->second)||!OwnersBound(e,owners,retained))return false;if(e.facts.created&&e.serial!=e.facts.created_serial)return false;if(!e.facts.created){bool witnessed=false;for(const auto& w:state.property.witnesses)if(w.second.custody_id==e.custody&&w.second.instance==e.pointer&&CustodyCodec::Encode(w.second.row)==CustodyCodec::Encode(e.facts.initial)&&LayoutCrate::Same(w.second.crate,e.facts.crate)&&LayoutCrate::Matches(w.second.crate,e.pointer)&&LayoutCrateMember::SameOrigin(w.second.member,e.facts.member)&&(!w.second.member||(facts.count(w.second.member->container->binding.custody_id)&&LayoutCrate::Same(w.second.member->container,facts.at(w.second.member->container->binding.custody_id).crate)))&&LayoutHouse::Same(w.second.house,e.facts.house)&&LayoutHouse::Matches(w.second.house,e.pointer)){const auto text=std::to_string(e.serial);if(w.second.guid==std::string(16-text.size(),'0')+text){witnessed=true;break;}}if(!witnessed)return false;}}return true;
}
inline bool Preflight(const State& state,const PropertyOwners& owners,const Retained& retained){for(const auto& e:state.entries){if(!OwnersBound(e,owners,retained)||(!Before(e)&&!After(e))||(!Before(e,true)&&!After(e,true)))return false;}return true;}
// Build every owner/projection record before the first inventory or property
// mutation. A trusted slot may belong to the separate native presentation map.
inline std::optional<std::vector<Entry>> Review(const LayoutMixed::Outcome& outcome,const LayoutProperty::State& property,const PropertyOwners& owners,Retained& retained){
 if(!LayoutMixedJournal::Detail::PropertyPrepared(outcome,&property))return {};auto facts=Fold(outcome);std::vector<Entry> entries;entries.reserve(facts.size());std::set<EQ::ItemInstance*> pointers;
 for(const auto& [id,fact]:facts){Entry e;e.custody=id;e.facts=fact;
  auto cached=retained.find(id);
  if(fact.created){if(cached!=retained.end()&&cached->second)return {};auto unit=property.split_units.find(id);if(unit==property.split_units.end()||!unit->second||unit->second->GetSerialNumber()!=fact.created_serial||!InventoryProjection::Matches(fact.initial,*unit->second))return {};e.pointer=unit->second.get();e.serial=e.pointer->GetSerialNumber();}
  else{auto slot=owners.find(id);if(slot==owners.end()||!slot->second||!*slot->second)return {};e.scene=slot->second;e.pointer=e.scene->get();e.serial=e.pointer->GetSerialNumber();bool witnessed=false;for(const auto& w:property.witnesses)if(w.second.custody_id==id&&w.second.instance==e.pointer&&CustodyCodec::Encode(w.second.row)==CustodyCodec::Encode(fact.initial)){witnessed=true;break;}if(!witnessed||!Match(fact.initial,e.pointer,e.serial)||!LayoutCrate::Matches(fact.crate,e.pointer)||!LayoutHouse::Matches(fact.house,e.pointer))return {};
   if(cached!=retained.end()&&cached->second&&&cached->second!=e.scene){e.cache=&cached->second;e.cache_pointer=e.cache->get();e.cache_serial=e.cache_pointer->GetSerialNumber();if(e.cache_pointer==e.pointer||!Match(fact.initial,e.cache_pointer,e.cache_serial)||!LayoutCrate::Matches(fact.crate,e.cache_pointer)||!LayoutHouse::Matches(fact.house,e.cache_pointer))return {};}
  }
  if(!pointers.insert(e.pointer).second||(e.cache_pointer&&!pointers.insert(e.cache_pointer).second))return {};
  // All supported changes preserve metadata; SetCharges is sufficient for the
  // final row, while geometry/world actors are refreshed after the known ACK.
  if(fact.final){auto projected=fact.initial;projected.values.charges=fact.final->values.charges;projected.null_fields=(projected.null_fields&~LiveItems::NullCharges)|(fact.final->null_fields&LiveItems::NullCharges);if(CustodyCodec::Encode(projected)!=CustodyCodec::Encode(*fact.final))return {};}
  entries.push_back(std::move(e));
 }
 return entries;
}
inline bool Initialize(const Binding& binding,const LayoutMixed::Outcome& outcome,LayoutProperty::State& property,State& state,const PropertyOwners& owners,Retained& retained){
 auto reviewed=Review(outcome,property,owners,retained);if(!reviewed)return false;auto entries=std::move(*reviewed);
 // Map-node allocation is completed before relinquishing prepared ownership.
 for(auto& e:entries)if(e.facts.created){auto inserted=retained.try_emplace(e.custody,nullptr);e.scene=&inserted.first->second;}
 state.entries=std::move(entries);state.binding=LayoutMixedJournal::Detail::Canonical(binding);state.outcome=LayoutMixed::Encode(outcome);state.property=std::move(property);state.initialized=true;
 // Created pieces are retained once. They were never inventory debits and are
 // never returned to a source slot or cloned again during an ACK retry.
 for(auto& e:state.entries)if(e.facts.created){auto unit=state.property.split_units.find(e.custody);if(unit==state.property.split_units.end()||!unit->second||*e.scene)return false;*e.scene=std::move(unit->second);}
 return true;
}
inline void Apply(Entry& e,bool cache=false){if(After(e,cache))return;auto* slot=cache?e.cache:e.scene;if(!slot)return;if(e.facts.final){const auto* data=(*slot)->GetItem();(*slot)->SetCharges(InventoryProjection::Charges(*e.facts.final,*data));}else{auto& retired=cache?e.retired_cache:e.retired_scene;retired=std::move(*slot);}}
}
inline bool Applied(const Binding& binding,const LayoutMixed::Outcome& outcome,const State& state,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence,const PropertyOwners& owners,const Retained& retained)noexcept{
 try{if(!fence.HeldFor(binding.lease.actor,binding.lease.epoch,binding.lease.session_key)||!Detail::Bound(binding,outcome,state,owners,retained))return false;for(size_t i=0;i<state.retained_tokens.size();++i)if(!Detail::TokenAfter(outcome.retained_tokens[i],state.retained_tokens[i],inv,retained))return false;if(outcome.inventory&&!LayoutInventoryApplication::Applied(binding,*outcome.inventory,state.inventory,inv,fence,retained))return false;for(const auto& e:state.entries)if(!Detail::After(e)||!Detail::After(e,true))return false;return true;}catch(...){return false;}
}
// Internal memory phase, called only after a fresh committed journal read below.
inline bool ApplyMemory(const Binding& binding,const LayoutMixed::Outcome& outcome,const LayoutInventory::Plan& plan,LayoutInventory::Prepared& prepared,LayoutProperty::State& property,State& state,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence,const PropertyOwners& owners,Retained& retained){
 if(!fence.HeldFor(binding.lease.actor,binding.lease.epoch,binding.lease.session_key))return false;
 if(!state.initialized){if(!Detail::TokensBound(outcome,prepared.retained_tokens))return false;for(size_t i=0;i<prepared.retained_tokens.size();++i){const auto& w=prepared.retained_tokens[i];if(!Detail::TokenBefore(w,inv,fence))return false;auto held=retained.find(outcome.retained_tokens[i].after.placement.stored.custody_id);if(held!=retained.end()&&held->second)return false;}state.retained_tokens=prepared.retained_tokens;for(const auto& t:outcome.retained_tokens)retained.try_emplace(t.after.placement.stored.custody_id,nullptr);if(outcome.inventory){if(!LayoutInventory::PreparedMatches(plan,prepared,binding.lease,inv,fence))return false;std::vector<LiveItems::LayoutUnitPlacement> placements;for(const auto& p:outcome.inventory->pieces)placements.push_back(p.placement);if(LayoutInventory::EncodeOutcome(LayoutInventory::MakeOutcome(plan,prepared,outcome.inventory->inventory,placements))!=LayoutInventory::EncodeOutcome(*outcome.inventory))return false;}if(!Detail::Initialize(binding,outcome,property,state,owners,retained))return false;}
 if(!Detail::Bound(binding,outcome,state,owners,retained)||!Detail::Preflight(state,owners,retained))return false;for(size_t i=0;i<state.retained_tokens.size();++i)if(!Detail::TokenBefore(state.retained_tokens[i],inv,fence)&&!Detail::TokenAfter(outcome.retained_tokens[i],state.retained_tokens[i],inv,retained))return false;
 if(outcome.inventory&&!LayoutInventoryApplication::ApplyMemory(binding,*outcome.inventory,plan,prepared,state.inventory,inv,fence,retained))return false;
 for(size_t i=0;i<state.retained_tokens.size();++i){const auto& w=state.retained_tokens[i];const auto& t=outcome.retained_tokens[i];if(!Detail::TokenAfter(t,w,inv,retained)){auto& owner=retained.at(t.after.placement.stored.custody_id);if(owner)return false;owner.reset(inv.PopItem(int16_t(w.row.values.slot_id)));if(owner.get()!=w.original)return false;}}
 for(auto& entry:state.entries){Detail::Apply(entry);Detail::Apply(entry,true);}return Applied(binding,outcome,state,inv,fence,owners,retained);
}
inline LiveItems::Result<bool> AcknowledgeUnderTransaction(Database& db,bool enabled,const Binding& binding,const State& state,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence,const PropertyOwners& owners,const Retained& retained){auto receipt=LayoutMixedJournal::ReadUnderTransaction(db,enabled,binding,fence);if(!receipt)return {false,receipt.error};if(receipt.value.applied)return {true,Error::None};if(!Applied(binding,receipt.value.outcome,state,inv,fence,owners,retained))return {false,Error::UnheldFence};try{for(const auto& t:receipt.value.outcome.retained_tokens){auto marked=LayoutMixedJournal::Detail::Q(db,"UPDATE housing_live_item_receipts SET applied=1 WHERE request_key="+LayoutMixedJournal::Detail::Literal(t.child_key)+" AND completed=1 AND applied=0");if(marked.RowsAffected()!=1)return {false,Error::Conflict};}LayoutRetained::ReleaseHouses(db,receipt.value.outcome.retained_tokens,binding.request_key);auto marked=LayoutMixedJournal::Detail::Q(db,"UPDATE housing_layout_mixed_batches SET applied=1 WHERE request_key="+LayoutMixedJournal::Detail::Literal(binding.request_key)+" AND applied=0");auto clear=LayoutMixedJournal::Detail::Q(db,"UPDATE housing_live_item_leases SET pending_key=NULL WHERE character_id="+std::to_string(binding.lease.actor.character_id)+" AND pending_key="+LayoutMixedJournal::Detail::Literal(binding.request_key));if(marked.RowsAffected()!=1||clear.RowsAffected()!=1)return {false,Error::Conflict};return {true,Error::None};}catch(const LayoutMixedJournal::Detail::Failure& e){return {false,e.error};}catch(...){return {false,Error::DatabaseFailure};}}
inline LiveItems::Result<LayoutMixedJournal::Receipt> ApplyCommitted(Database& db,bool enabled,const Binding& binding,const LayoutInventory::Plan& plan,LayoutInventory::Prepared& prepared,LayoutProperty::State& property,State& state,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence,const PropertyOwners& owners,Retained& retained){bool active=false,commit=false;if(!enabled)return {{},Error::Disabled};try{auto tx=LayoutMixedJournal::Detail::Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="0")return {{},Error::InvalidArgument};LayoutMixedJournal::Detail::Q(db,"START TRANSACTION");active=true;auto receipt=LayoutMixedJournal::ReadUnderTransaction(db,true,binding,fence);if(!receipt){LayoutMixedJournal::Detail::Q(db,"ROLLBACK");return receipt;}if(!receipt.value.applied){if(!ApplyMemory(binding,receipt.value.outcome,plan,prepared,property,state,inv,fence,owners,retained)){LayoutMixedJournal::Detail::Q(db,"ROLLBACK");return {{},Error::UnheldFence};}auto ack=AcknowledgeUnderTransaction(db,true,binding,state,inv,fence,owners,retained);if(!ack){LayoutMixedJournal::Detail::Q(db,"ROLLBACK");return {{},ack.error};}receipt.value.applied=true;}commit=true;LayoutMixedJournal::Detail::Q(db,"COMMIT");active=false;return receipt;}catch(const LayoutMixedJournal::Detail::Failure& e){if(commit)return {{},Error::CommitUncertain};if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},e.error};}catch(...){if(commit)return {{},Error::CommitUncertain};if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}}
}
#endif
