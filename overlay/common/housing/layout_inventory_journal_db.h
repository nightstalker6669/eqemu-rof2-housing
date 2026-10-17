#ifndef EQEMU_HOUSING_LAYOUT_INVENTORY_JOURNAL_DB_H
#define EQEMU_HOUSING_LAYOUT_INVENTORY_JOURNAL_DB_H
#include "layout_inventory_outcome_db.h"
#include "layout_batch_journal.h"
namespace Housing::LayoutInventoryJournal {
using Bytes=CustodyCodec::Bytes;using Error=LiveItems::Error;using Binding=LayoutBatch::Binding;
struct Receipt {LayoutInventory::Outcome outcome;bool applied=false,replayed=false;};
namespace Detail {
struct Failure {Error error;};
inline void Require(bool yes,Error error=Error::InvalidArgument){if(!yes)throw Failure{error};}
inline std::string Hex(const Bytes& bytes){const char* d="0123456789ABCDEF";std::string text;for(auto byte:bytes){text+=d[byte>>4];text+=d[byte&15];}return text;}
inline std::string Literal(const std::string& text){return "x'"+Hex(Bytes(text.begin(),text.end()))+"'";}
inline Bytes Unhex(const char* p,size_t maximum){Require(p,Error::DatabaseFailure);std::string text=p;Require(!(text.size()%2)&&text.size()/2<=maximum,Error::DatabaseFailure);auto digit=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw Failure{Error::DatabaseFailure};};Bytes bytes;bytes.reserve(text.size()/2);for(size_t i=0;i<text.size();i+=2)bytes.push_back((digit(text[i])<<4)|digit(text[i+1]));return bytes;}
inline MySQLRequestResult Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);Require(r.Success(),Error::DatabaseFailure);return r;}
inline void Schema(Database& db){auto r=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_layout_inventory_batches'");Require(r.RowCount()==1&&r.begin()[0]&&std::string(r.begin()[0])=="InnoDB",Error::Disabled);}
inline bool Key(const std::string& s){if(s.size()<=7||s.size()>96||s.rfind("layout:",0))return false;for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==':'||c=='.'||c=='_'||c=='-'))return false;return true;}
// Initial inventory member family only. The mixed coordinator must add typed
// property member outcomes before a mixed wire can enter this pending family.
// No native handler is enabled by these internal transaction hooks.
inline std::optional<LayoutWire::Load> Wire(const Binding& b){if(!Key(b.request_key)||b.plan.empty()||b.plan.size()>LayoutBatch::MaximumPlanBytes)return {};auto w=LayoutWire::DecodeLoad(b.wire.data(),b.wire.size());if(!w||w->operations.empty())return {};for(const auto& operation:w->operations){const auto* p=std::get_if<LayoutWire::PlacementOperation>(&operation);if(!p||p->source_kind||p->source_estate_id!=UINT32_MAX||p->native_item_id!=UINT32_MAX||p->source_slots[0]<23||p->source_slots[0]>33||p->source_slots[2]!=0xffff||(p->source_slots[1]!=0xffff&&p->source_slots[1]>=EQ::invbag::SLOT_COUNT))return {};}return w;}
inline bool MatchesWire(const LayoutWire::Load& wire,const LayoutInventory::Outcome& outcome){
 if(wire.operations.size()!=outcome.pieces.size())return false;
 std::vector<const LayoutInventory::SourceChange*> sources(outcome.sources.size());for(const auto& c:outcome.inventory.changes)sources[c.source_index]=&c;
 for(const auto& p:outcome.pieces){if(p.operation_index>=wire.operations.size())return false;const auto& op=std::get<LayoutWire::PlacementOperation>(wire.operations[p.operation_index]);const auto slot=op.source_slots[1]==0xffff?int16_t(op.source_slots[0]):EQ::InventoryProfile::CalcSlotId(int16_t(op.source_slots[0]),uint8_t(op.source_slots[1]));
  if(slot<0||uint32_t(slot)!=sources[p.source_index]->before.values.slot_id||op.guid!=LayoutInventory::NativeGuid(outcome.sources[p.source_index].runtime_serial)||op.target_estate_id!=p.placement.stored.placement->reservation.native_estate_id||(op.target_estate_id!=wire.house_estate_id&&op.target_estate_id!=wire.yard_estate_id))return false;
  Bytes actual,expected;LayoutInventory::OutcomeDetail::Pose(actual,p.placement.stored.placement->transform);LayoutInventory::OutcomeDetail::Pose(expected,LayoutWire::PlacementTransform(op));if(actual!=expected)return false;
 }return true;
}
inline Bytes Canonical(const Binding& b){Bytes bytes;CustodyCodec::W(bytes,1,4);CustodyCodec::S(bytes,b.request_key);CustodyCodec::W(bytes,b.lease.actor.character_id,4);CustodyCodec::W(bytes,b.lease.actor.account_id,4);CustodyCodec::W(bytes,b.lease.epoch,8);CustodyCodec::S(bytes,b.lease.session_key);bytes.insert(bytes.end(),b.lease.token.begin(),b.lease.token.end());LayoutInventory::OutcomeDetail::Blob(bytes,b.wire);LayoutInventory::OutcomeDetail::Blob(bytes,b.plan);return bytes;}
inline LiveItems::Result<Receipt> Lookup(Database& db,const Binding& b,const LiveItems::Lease& current){
 auto mixed_schema=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_layout_mixed_batches'");if(mixed_schema.RowCount()){Require(mixed_schema.RowCount()==1&&mixed_schema.begin()[0]&&std::string(mixed_schema.begin()[0])=="InnoDB",Error::DatabaseFailure);auto mixed=Q(db,"SELECT request_key FROM housing_layout_mixed_batches WHERE request_key="+Literal(b.request_key)+" FOR UPDATE");Require(!mixed.RowCount(),Error::Conflict);}
 auto legacy=Q(db,"SELECT request_key FROM housing_live_item_receipts WHERE request_key="+Literal(b.request_key)+" FOR UPDATE");Require(!legacy.RowCount(),Error::Conflict);
 auto property_schema=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_layout_batches'");if(property_schema.RowCount()){Require(property_schema.RowCount()==1&&property_schema.begin()[0]&&std::string(property_schema.begin()[0])=="InnoDB",Error::DatabaseFailure);auto property=Q(db,"SELECT request_key FROM housing_layout_batches WHERE request_key="+Literal(b.request_key)+" FOR UPDATE");Require(!property.RowCount(),Error::Conflict);}
 auto rows=Q(db,"SELECT format_version,character_id,account_id,lease_epoch,HEX(session_key),HEX(binding_blob),HEX(outcome_blob),applied FROM housing_layout_inventory_batches WHERE request_key="+Literal(b.request_key)+" FOR UPDATE");if(!rows.RowCount())return {{},Error::NotFound};Require(rows.RowCount()==1,Error::DatabaseFailure);auto row=rows.begin();for(int i=0;i<8;++i)Require(row[i],Error::DatabaseFailure);
 Require(std::string(row[0])=="1",Error::DatabaseFailure);Require(std::stoul(row[1])==b.lease.actor.character_id&&std::stoul(row[2])==b.lease.actor.account_id,Error::Unauthorized);Require(std::stoull(row[3])==b.lease.epoch&&Unhex(row[4],96)==Bytes(b.lease.session_key.begin(),b.lease.session_key.end()),Error::StaleLease);
 Require(Unhex(row[5],LayoutBatch::MaximumPlanBytes+NeighborhoodWire::MaximumPayload+512)==Canonical(b),Error::Conflict);auto decoded=LayoutInventory::DecodeOutcome(Unhex(row[6],LayoutInventory::MaximumInventoryOutcomeBytes));Require(bool(decoded),Error::DatabaseFailure);auto wire=Wire(b);Require(wire&&MatchesWire(*wire,*decoded),Error::DatabaseFailure);Require(std::string(row[7])=="0"||std::string(row[7])=="1",Error::DatabaseFailure);bool applied=std::string(row[7])=="1";
 Require(applied?current.pending_key!=b.request_key:current.pending_key==b.request_key,Error::Conflict);if(!applied){auto valid=LayoutInventory::ValidateOutcomeUnderTransaction(db,b.lease,*decoded);if(!valid)return {{},valid.error};}return {{std::move(*decoded),applied,true},Error::None};
}
}
// Internal family hooks; caller owns TX/lifecycle. Any failure after member
// writes requires ENTIRE parent rollback. No ACK/memory/recovery release here.
inline LiveItems::Result<Receipt> ReadUnderTransaction(Database& db,bool enabled,const Binding& b,LiveItems::InventorySessionFence& fence){
 if(!enabled)return {{},Error::Disabled};if(!Detail::Wire(b))return {{},Error::InvalidArgument};
 try{auto current=LiveItems::Service::AuthenticateUnderTransaction(db,true,b.lease,fence);if(!current)return {{},current.error};Detail::Schema(db);return Detail::Lookup(db,b,current.value);}catch(const Detail::Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}
inline LiveItems::Result<Receipt> CompleteUnderTransaction(Database& db,bool enabled,const Binding& b,const LayoutInventory::Plan& plan,const LayoutInventory::Prepared& prepared,const LayoutInventory::Consumption& consumed,const std::vector<LiveItems::LayoutUnitPlacement>& placements,EQ::InventoryProfile& inv,LiveItems::InventorySessionFence& fence){
 if(!enabled)return {{},Error::Disabled};auto wire=Detail::Wire(b);if(!wire)return {{},Error::InvalidArgument};
 try{auto current=LiveItems::Service::AuthenticateUnderTransaction(db,true,b.lease,fence);if(!current)return {{},current.error};Detail::Schema(db);
  // Historical reads precede live beforeimage/ACL checks. They never repeat
  // source consumption or placement allocation after a known/uncertain commit.
  auto old=Detail::Lookup(db,b,current.value);if(old)return old;if(old.error!=Error::NotFound)return old;
  Detail::Require(current.value.pending_key.empty(),Error::PendingApplication);Detail::Require(LayoutInventory::PreparedMatches(plan,prepared,b.lease,inv,fence),Error::UnheldFence);
  auto outcome=LayoutInventory::MakeOutcome(plan,prepared,consumed,placements);Detail::Require(Detail::MatchesWire(*wire,outcome));auto valid=LayoutInventory::ValidateOutcomeUnderTransaction(db,b.lease,outcome);if(!valid)return {{},valid.error};const auto encoded=LayoutInventory::EncodeOutcome(outcome);const auto& l=b.lease;
  auto saved=Detail::Q(db,"INSERT INTO housing_layout_inventory_batches(request_key,format_version,character_id,account_id,lease_epoch,session_key,binding_blob,outcome_blob,applied) VALUES("+Detail::Literal(b.request_key)+",1,"+std::to_string(l.actor.character_id)+","+std::to_string(l.actor.account_id)+","+std::to_string(l.epoch)+","+Detail::Literal(l.session_key)+",x'"+Detail::Hex(Detail::Canonical(b))+"',x'"+Detail::Hex(encoded)+"',0)");Detail::Require(saved.RowsAffected()==1,Error::DatabaseFailure);
  auto pending=Detail::Q(db,"UPDATE housing_live_item_leases SET pending_key="+Detail::Literal(b.request_key)+" WHERE character_id="+std::to_string(l.actor.character_id)+" AND pending_key IS NULL");Detail::Require(pending.RowsAffected()==1,Error::PendingApplication);return {{std::move(outcome),false,false},Error::None};
 }catch(const Detail::Failure& e){return {{},e.error};}catch(const std::invalid_argument&){return {{},Error::InvalidArgument};}catch(...){return {{},Error::DatabaseFailure};}
}
// Read-only restart component. The supervisor-linked exact writer-death proof
// and lease locks belong to InventoryRecovery::Reconcile; this cannot release
// an actor or certify death. Optional family absence preserves legacy lookup.
inline LiveItems::Result<Receipt> ReadPendingForRestartUnderTransaction(Database& db,const LiveItems::Lease& lease){
 if(!Detail::Key(lease.pending_key))return {{},Error::NotFound};
 try{
  auto tx=Detail::Q(db,"SELECT @@in_transaction");Detail::Require(tx.RowCount()==1&&tx.begin()[0]&&std::string(tx.begin()[0])=="1");
  auto schema=Detail::Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_layout_inventory_batches'");if(!schema.RowCount()){
   auto absent=db.QueryDatabase(std::string("SELECT request_key FROM housing_layout_inventory_batches WHERE 1=0"),false);if(!absent.Success()&&absent.ErrorNumber()==1146)return {{},Error::NotFound};return {{},Error::DatabaseFailure};
  }Detail::Schema(db);
  auto rows=Detail::Q(db,"SELECT HEX(binding_blob) FROM housing_layout_inventory_batches WHERE request_key="+Detail::Literal(lease.pending_key)+" FOR UPDATE");if(!rows.RowCount())return {{},Error::NotFound};Detail::Require(rows.RowCount()==1,Error::DatabaseFailure);
  auto bytes=Detail::Unhex(rows.begin()[0],LayoutBatch::MaximumPlanBytes+NeighborhoodWire::MaximumPayload+512);CustodyCodec::Reader r{bytes};Detail::Require(r.N(4)==1,Error::DatabaseFailure);Binding b;b.request_key=r.Text(96);b.lease.actor.character_id=r.N(4);b.lease.actor.account_id=r.N(4);b.lease.epoch=r.N(8);b.lease.session_key=r.Text(96);for(auto& byte:b.lease.token)byte=r.N(1);b.wire=r.Blob(NeighborhoodWire::MaximumPayload);b.plan=r.Blob(LayoutBatch::MaximumPlanBytes);
  Detail::Require(r.at==bytes.size()&&Detail::Canonical(b)==bytes&&bool(Detail::Wire(b)),Error::DatabaseFailure);Detail::Require(b.request_key==lease.pending_key&&b.lease.actor.character_id==lease.actor.character_id&&b.lease.actor.account_id==lease.actor.account_id&&b.lease.epoch==lease.epoch&&b.lease.session_key==lease.session_key&&b.lease.token==lease.token,Error::StaleLease);
  auto result=Detail::Lookup(db,b,lease);if(result&&result.value.applied)return {{},Error::Conflict};return result;
 }catch(const Detail::Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
}

}
#endif
