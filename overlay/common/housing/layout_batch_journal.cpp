#include "layout_batch_journal.h"
#include "layout_wire.h"
#include "custody_codec.h"
#include "../database.h"
namespace Housing::LayoutBatch {
namespace {
using Error=LiveItems::Error;using Bytes=std::vector<uint8_t>;
struct Failure {Error error;};
bool Key(const std::string& key){
 if(key.empty()||key.size()>96)return false;
 for(unsigned char c:key)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c==':'||c=='.'))return false;
 return true;
}
bool Shape(const Binding& b){
 if(!Key(b.request_key)||b.plan.empty()||b.plan.size()>MaximumPlanBytes)return false;
 const auto decoded=LayoutWire::DecodeLoad(b.wire.data(),b.wire.size());
 if(!decoded||decoded->operations.empty())return false;
 auto identity=[](uint32_t id){return id&&id!=UINT32_MAX;};
 for(const auto& operation:decoded->operations){
  if(const auto* placed=std::get_if<LayoutWire::PlacementOperation>(&operation)){
   // Format1 never covers a live inventory delta. A separate aggregate
   // application/pending/recovery contract is required before those loads.
   if(placed->source_kind!=0xff||placed->source_slots!=std::array<uint16_t,3>{0xffff,0xffff,0xffff}||
      !identity(placed->source_estate_id)||!identity(placed->native_item_id)||!identity(placed->target_estate_id))return false;
  }else{
   const auto& item=std::get<LayoutWire::ItemOperation>(operation);
   if(!identity(item.source_estate_id)||!identity(item.native_item_id)||!identity(item.target_estate_id))return false;
  }
 }
 return true;
}
std::string Hex(const Bytes& b){
 const char* alphabet="0123456789abcdef";std::string out;out.reserve(b.size()*2);
 for(auto c:b){out+=alphabet[c>>4];out+=alphabet[c&15];}return out;
}
Bytes Unhex(const char* p,size_t maximum){
 if(!p)throw Failure{Error::DatabaseFailure};std::string text=p;
 if(text.size()%2||text.size()/2>maximum)throw Failure{Error::DatabaseFailure};
 auto nibble=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw Failure{Error::DatabaseFailure};};
 Bytes out;out.reserve(text.size()/2);for(size_t i=0;i<text.size();i+=2)out.push_back((nibble(text[i])<<4)|nibble(text[i+1]));return out;
}
Bytes Canonical(const Binding& b){
 Bytes out;CustodyCodec::W(out,1,4);CustodyCodec::S(out,b.request_key);
 CustodyCodec::W(out,b.lease.actor.character_id,4);CustodyCodec::W(out,b.lease.actor.account_id,4);CustodyCodec::W(out,b.lease.epoch,8);
 CustodyCodec::S(out,b.lease.session_key);out.insert(out.end(),b.lease.token.begin(),b.lease.token.end());
 CustodyCodec::W(out,b.wire.size(),4);out.insert(out.end(),b.wire.begin(),b.wire.end());
 CustodyCodec::W(out,b.plan.size(),4);out.insert(out.end(),b.plan.begin(),b.plan.end());return out;
}
MySQLRequestResult Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw Failure{Error::DatabaseFailure};return r;}
void Schema(Database& db){
 auto s=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_layout_batches'");
 if(s.RowCount()!=1||!s.begin()[0]||std::string(s.begin()[0])!="InnoDB")throw Failure{Error::Disabled};
}
struct Current {LiveItems::Lease lease;Bytes binding;std::string key;};
Current Authenticate(Database& db,const Binding& b,LiveItems::InventorySessionFence& fence){
 auto lease=LiveItems::Service::AuthenticateUnderTransaction(db,true,b.lease,fence);if(!lease)throw Failure{lease.error};
 Schema(db);return {std::move(lease.value),Canonical(b),"x'"+Hex(Bytes(b.request_key.begin(),b.request_key.end()))+"'"};
}
LiveItems::Result<Receipt> Lookup(Database& db,const Binding& b,const Current& current){
 auto mixed_schema=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_layout_mixed_batches'");if(mixed_schema.RowCount()){if(mixed_schema.RowCount()!=1||!mixed_schema.begin()[0]||std::string(mixed_schema.begin()[0])!="InnoDB")throw Failure{Error::DatabaseFailure};auto mixed=Q(db,"SELECT request_key FROM housing_layout_mixed_batches WHERE request_key="+current.key+" FOR UPDATE");if(mixed.RowCount())throw Failure{Error::Conflict};}
 auto rows=Q(db,"SELECT format_version,character_id,account_id,lease_epoch,HEX(session_key),HEX(binding_blob),HEX(outcome_blob) FROM housing_layout_batches WHERE request_key="+current.key+" FOR UPDATE");
 if(!rows.RowCount())return {{},Error::NotFound};
 if(rows.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto row=rows.begin();for(size_t i=0;i<7;++i)if(!row[i])throw Failure{Error::DatabaseFailure};
 if(std::stoul(row[0])!=1)throw Failure{Error::DatabaseFailure};
 if(std::stoul(row[1])!=b.lease.actor.character_id||std::stoul(row[2])!=b.lease.actor.account_id)throw Failure{Error::Unauthorized};
 if(std::stoull(row[3])!=b.lease.epoch||Unhex(row[4],96)!=Bytes(b.lease.session_key.begin(),b.lease.session_key.end()))throw Failure{Error::StaleLease};
 if(Unhex(row[5],MaximumPlanBytes+NeighborhoodWire::MaximumPayload+512)!=current.binding)throw Failure{Error::Conflict};
 auto outcome=Unhex(row[6],MaximumOutcomeBytes);if(outcome.empty())throw Failure{Error::DatabaseFailure};
 return {{std::move(outcome),true},Error::None};
}
}
LiveItems::Result<Receipt> ReadUnderTransaction(Database& db,bool enabled,const Binding& b,LiveItems::InventorySessionFence& fence){
 if(!enabled)return {{},Error::Disabled};if(!Shape(b))return {{},Error::InvalidArgument};
 try{const auto current=Authenticate(db,b,fence);return Lookup(db,b,current);}
 catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Receipt> CompleteUnderTransaction(Database& db,bool enabled,const Binding& b,const Bytes& outcome,LiveItems::InventorySessionFence& fence){
 if(!enabled)return {{},Error::Disabled};if(!Shape(b)||outcome.empty()||outcome.size()>MaximumOutcomeBytes)return {{},Error::InvalidArgument};
 try{
  const auto current=Authenticate(db,b,fence);auto previous=Lookup(db,b,current);
  if(previous){if(previous.value.outcome!=outcome)return {{},Error::Conflict};return previous;}
  if(previous.error!=Error::NotFound)return previous;
  if(!current.lease.pending_key.empty())return {{},Error::PendingApplication};
  const auto& l=b.lease;
  auto inserted=Q(db,"INSERT INTO housing_layout_batches(request_key,format_version,character_id,account_id,lease_epoch,session_key,binding_blob,outcome_blob) VALUES("+current.key+",1,"+std::to_string(l.actor.character_id)+","+std::to_string(l.actor.account_id)+","+std::to_string(l.epoch)+",x'"+Hex(Bytes(l.session_key.begin(),l.session_key.end()))+"',x'"+Hex(current.binding)+"',x'"+Hex(outcome)+"')");
  if(inserted.RowsAffected()!=1)throw Failure{Error::Conflict};return {{outcome,false},Error::None};
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}
}
