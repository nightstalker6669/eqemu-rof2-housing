#ifndef EQEMU_HOUSING_ESTATE_RETIREMENT_DB_H
#define EQEMU_HOUSING_ESTATE_RETIREMENT_DB_H
#include "scene_service.h"
#include "coin_pending_guard.h"
#include "sale_wire.h"
#include <algorithm>
namespace Housing::EstateRetirement {
struct Request{std::string key;LiveItems::Lease lease;EstateId estate_id=0;uint32_t native_plot_id=0;};
namespace Detail {
using Error=LiveItems::Error;struct Failure{Error error;};
inline MySQLRequestResult Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw Failure{Error::DatabaseFailure};return r;}
inline uint64_t N(const char* p){if(!p)throw Failure{Error::DatabaseFailure};std::string s=p;if(s.empty()||!std::all_of(s.begin(),s.end(),[](char c){return c>='0'&&c<='9';}))throw Failure{Error::DatabaseFailure};return std::stoull(s);}
inline std::string Quote(Database& db,const std::string& s){return "'"+db.Escape(s)+"'";}
inline std::string Hex(const std::array<uint8_t,32>& bytes){std::string s;const char* d="0123456789ABCDEF";for(auto b:bytes){s+=d[b>>4];s+=d[b&15];}return s;}
inline bool Key(const std::string& s){return !s.empty()&&s.size()<=96&&std::all_of(s.begin(),s.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==':'||c=='-'||c=='_'||c=='.';});}
inline void Empty(Database& db,const std::string& sql){if(Q(db,sql+" LIMIT 1 FOR UPDATE").RowCount())throw Failure{Error::UnsupportedItem};}
}
// Real optional empty-personal-plot Move Out. No currency/item delivery is
// enabled. Retired generations, their keys, ACL, receipts, and FK history remain.
// Caller keeps its genuine inventory fence through uncertain-COMMIT resolution.
inline LiveItems::Result<uint32_t> Retire(Database& db,bool enabled,const Request& request,
 LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* houses=nullptr,
 const CrateBinding::CustodyContext* crates=nullptr){
 using namespace Detail;if(!enabled)return {{},Error::Disabled};
 if(!Key(request.key)||!request.estate_id||!request.native_plot_id||request.native_plot_id==UINT32_MAX)return {{},Error::InvalidArgument};
 bool active=false,committing=false;
 try{
  auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_estates','housing_estate_retirements')");
  if(engines.RowCount()!=1||N(engines.begin()[0])!=2)return {{},Error::Disabled};
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0]))return {{},Error::InvalidArgument};
  Q(db,"START TRANSACTION");active=true;
  auto authenticated=LiveItems::Service::AuthenticateUnderTransaction(db,true,request.lease,fence);if(!authenticated)throw Failure{authenticated.error};
  if(!authenticated.value.pending_key.empty()||CoinPending::Inspect(db,request.lease.actor.character_id,true)!=CoinPending::State::Clear)throw Failure{Error::PendingApplication};
  auto key=Quote(db,request.key);
  auto prior=Q(db,"SELECT estate_id,character_id,account_id,lease_epoch,session_key,HEX(lease_token),native_plot_id FROM housing_estate_retirements WHERE request_key="+key+" FOR UPDATE");
  if(prior.RowCount()){
   if(prior.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto r=prior.begin();
   if(N(r[0])!=request.estate_id||N(r[1])!=request.lease.actor.character_id||N(r[2])!=request.lease.actor.account_id||N(r[3])!=request.lease.epoch||!r[4]||std::string(r[4])!=request.lease.session_key||!r[5]||std::string(r[5])!=Hex(request.lease.token)||N(r[6])!=request.native_plot_id)throw Failure{Error::Conflict};
  }else{
   auto scene=Scene::Service::ReadSceneUnderTransaction(db,request.lease,request.estate_id,fence,houses,crates);if(!scene)throw Failure{scene.error};
   if(scene.value.native_plot_id!=request.native_plot_id||scene.value.definition.type!=4||!scene.value.access.implicit_owner)throw Failure{Error::Unauthorized};
   auto generation=Q(db,"SELECT plot_id,owner_kind,owner_id,active FROM housing_estates WHERE id="+std::to_string(request.estate_id)+" FOR UPDATE");
   if(generation.RowCount()!=1||N(generation.begin()[1])!=1||N(generation.begin()[2])!=request.lease.actor.character_id||N(generation.begin()[3])!=1)throw Failure{Error::Unauthorized};
   const auto plot=N(generation.begin()[0]);if(!plot||plot>UINT32_MAX)throw Failure{Error::DatabaseFailure};
   auto plot_lock=Q(db,"SELECT id FROM housing_plot_definitions WHERE id="+std::to_string(plot)+" FOR UPDATE");if(plot_lock.RowCount()!=1)throw Failure{Error::Conflict};
   if(!scene.value.items.empty()||!scene.value.crate_contents.empty())throw Failure{Error::UnsupportedItem};
   // Stowed objects are not in the rendered scene. Never mistake their absence
   // from a visible scene for an empty property. Historical closed previews and
   // completed receipts remain untouched. Bound child estates (even carried)
   // need their separate return/egress cutover before this path can support them.
   // Includes legacy unscoped storage, and deliberately denies historical
   // origin custody that has moved elsewhere until its return/authority cutover
   // is implemented. An omitted actor is not evidence of item conservation.
   Empty(db,"SELECT custody_id FROM housing_live_item_custody WHERE estate_id="+std::to_string(request.estate_id));
   Empty(db,"SELECT custody_id FROM housing_live_item_storage WHERE native_estate_id="+std::to_string(request.native_plot_id));
   Empty(db,"SELECT p.custody_id FROM housing_live_placed_items p JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE r.native_estate_id="+std::to_string(request.native_plot_id));
   Empty(db,"SELECT instance_key FROM housing_fixture_item_custody WHERE estate_id="+std::to_string(request.estate_id));
   Empty(db,"SELECT native_id FROM housing_native_estates WHERE parent_native_id="+std::to_string(request.native_plot_id)+" AND active=1");
   Empty(db,"SELECT native_item_id FROM housing_live_placement_reservations WHERE native_estate_id="+std::to_string(request.native_plot_id)+" AND state=0");
   Empty(db,"SELECT p.request_key FROM housing_live_escrow_pending p JOIN housing_live_escrow_receipts r ON r.request_key=p.request_key WHERE r.estate_id="+std::to_string(request.estate_id));
   auto balances=Q(db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+std::to_string(request.estate_id)+" FOR UPDATE");
   if(balances.RowCount()>1)throw Failure{Error::DatabaseFailure};if(balances.RowCount()&&N(balances.begin()[0])!=0)throw Failure{Error::UnsupportedItem};
   // Locking the current domain generation serializes all current-authority
   // mutations. Creation locks plot identity and unique active_plot_id provides
   // independent cross-connection one-current-owner enforcement.
   auto detached=Q(db,"UPDATE housing_native_estates SET ownership_estate_id=NULL WHERE native_id="+std::to_string(request.native_plot_id)+" AND ownership_estate_id="+std::to_string(request.estate_id)+" AND plot_id="+std::to_string(plot)+" AND active=1");if(detached.RowsAffected()!=1)throw Failure{Error::Conflict};
   auto retired=Q(db,"UPDATE housing_estates SET active=0 WHERE id="+std::to_string(request.estate_id)+" AND active=1");if(retired.RowsAffected()!=1)throw Failure{Error::Conflict};
   Q(db,"INSERT INTO housing_estate_retirements(estate_id,request_key,character_id,account_id,lease_epoch,session_key,lease_token,native_plot_id,plot_id,catalog_revision) VALUES ("+std::to_string(request.estate_id)+","+key+","+std::to_string(request.lease.actor.character_id)+","+std::to_string(request.lease.actor.account_id)+","+std::to_string(request.lease.epoch)+","+Quote(db,request.lease.session_key)+",x'"+Hex(request.lease.token)+"',"+std::to_string(request.native_plot_id)+","+std::to_string(plot)+","+std::to_string(scene.value.current_catalog_revision)+")");
  }
  committing=true;auto result=db.QueryDatabase(std::string("COMMIT"),false);active=false;if(!result.Success())return {{},Error::CommitUncertain};return {request.estate_id,Error::None};
 }catch(const Failure& e){if(active)try{db.QueryDatabase(std::string("ROLLBACK"),false);}catch(...){}return {{},committing?Error::CommitUncertain:e.error};}
 catch(...){if(active)try{db.QueryDatabase(std::string("ROLLBACK"),false);}catch(...){}return {{},committing?Error::CommitUncertain:Error::DatabaseFailure};}
}
}
#endif
