#ifndef EQEMU_HOUSING_UPKEEP_SELECTION_DB_H
#define EQEMU_HOUSING_UPKEEP_SELECTION_DB_H
#include "upkeep_selection_policy.h"
#include "scene_service.h"
#include "catalog_service.h"
#include "coin_pending_guard.h"
namespace Housing::UpkeepSelection {
struct Request {std::string key;LiveItems::Lease lease;EstateId estate_id=0;uint32_t native_plot_id=0,component_id=0;std::optional<Route> occupied_source;};
namespace Detail {
using Error=LiveItems::Error;
struct Failure{Error error;};
inline MySQLRequestResult Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw Failure{Error::DatabaseFailure};return r;}
inline std::string Quote(Database& db,const std::string& s){return "'"+db.Escape(s)+"'";}
inline uint64_t N(const char* p){if(!p)throw Failure{Error::DatabaseFailure};std::string s=p;if(s.empty()||!std::all_of(s.begin(),s.end(),[](char c){return c>='0'&&c<='9';}))throw Failure{Error::DatabaseFailure};size_t end=0;auto n=std::stoull(s,&end);if(end!=s.size())throw Failure{Error::DatabaseFailure};return n;}
inline std::string Hex(const std::array<uint8_t,32>& bytes){std::string s;const char* d="0123456789ABCDEF";for(auto b:bytes){s+=d[b>>4];s+=d[b&15];}return s;}
inline std::vector<uint8_t> Unhex(const char* p){if(!p)throw Failure{Error::DatabaseFailure};std::string s=p;if(s.size()%2||s.size()>2*RegionWire::MaximumPayload)throw Failure{Error::DatabaseFailure};auto n=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw Failure{Error::DatabaseFailure};};std::vector<uint8_t> b;for(size_t i=0;i<s.size();i+=2)b.push_back(uint8_t(n(s[i])*16+n(s[i+1])));return b;}
inline bool Key(const std::string& s){return !s.empty()&&s.size()<=96&&std::all_of(s.begin(),s.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==':'||c=='-'||c=='_'||c=='.';});}
inline bool Installed(Database& db){auto r=Q(db,"SELECT table_name,engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_upkeep_selection','housing_upkeep_selection_receipts')");if(!r.RowCount())return false;if(r.RowCount()!=2)throw Failure{Error::DatabaseFailure};for(auto row=r.begin();row.operator!=(r.end());++row)if(!row[1]||std::string(row[1])!="InnoDB")throw Failure{Error::DatabaseFailure};return true;}
}
// Display-only read on caller's consistent snapshot. Optional absence preserves
// older deployments. Old-owner/replaced-definition/removed-component settings
// are not projected onto a different property. Never used for authorization.
inline uint32_t PublishedComponent(Database& db,EstateId estate,uint32_t native,
 uint32_t owner_kind,uint32_t owner,const EstateWire::Definition& definition,
 const std::vector<EstateWire::CostScheme>& costs){
 using namespace Detail;if(!Installed(db))return 0;
 auto r=Q(db,"SELECT component_id,revision FROM housing_upkeep_selection WHERE estate_id="+std::to_string(estate)+" AND native_plot_id="+std::to_string(native)+" AND owner_kind="+std::to_string(owner_kind)+" AND owner_id="+std::to_string(owner)+" AND definition_id="+std::to_string(definition.id));
 if(r.RowCount()>1)throw Failure{Error::DatabaseFailure};if(!r.RowCount())return 0;
 auto c=N(r.begin()[0]);if(!N(r.begin()[1])||c>UINT32_MAX)throw Failure{Error::DatabaseFailure};return Offered(definition,costs,uint32_t(c))?uint32_t(c):0;
}
// Owns one transaction. Actual caller fence stays held across uncertain COMMIT
// and key resolution. Assignment has no currency/inventory application phase.
// Historical receipts resolve without reapplying over a newer selection.
inline LiveItems::Result<uint64_t> Select(Database& db,bool enabled,const Request& request,
 LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* houses=nullptr,
 const CrateBinding::CustodyContext* crates=nullptr){
 using namespace Detail;if(!enabled)return {{},Error::Disabled};
 if(!Key(request.key)||!request.estate_id||!request.native_plot_id||request.native_plot_id==UINT32_MAX||!request.component_id||request.component_id==UINT32_MAX)return {{},Error::InvalidArgument};
 bool active=false,committing=false;
 try{
  if(!Installed(db))return {{},Error::Disabled};
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0]))return {{},Error::InvalidArgument};
  Q(db,"START TRANSACTION");active=true;
  auto actor=LiveItems::Service::AuthenticateUnderTransaction(db,true,request.lease,fence);if(!actor)throw Failure{actor.error};
  if(!actor.value.pending_key.empty()||CoinPending::Inspect(db,request.lease.actor.character_id,true)!=CoinPending::State::Clear)throw Failure{Error::PendingApplication};
  const auto key=Quote(db,request.key);
  auto prior=Q(db,"SELECT character_id,account_id,lease_epoch,session_key,HEX(lease_token),estate_id,native_plot_id,component_id,selection_revision FROM housing_upkeep_selection_receipts WHERE request_key="+key+" FOR UPDATE");
  uint64_t revision=0;
  if(prior.RowCount()){
   if(prior.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto r=prior.begin();
   if(N(r[0])!=request.lease.actor.character_id||N(r[1])!=request.lease.actor.account_id||N(r[2])!=request.lease.epoch||!r[3]||std::string(r[3])!=request.lease.session_key||!r[4]||std::string(r[4])!=Hex(request.lease.token)||N(r[5])!=request.estate_id||N(r[6])!=request.native_plot_id||N(r[7])!=request.component_id||!(revision=N(r[8])))throw Failure{Error::Conflict};
  }else{
   uint32_t definition_id=0;uint64_t catalog_revision=0;
   if(request.occupied_source){
    // Native action2 still names the parent plot. Only the actual server
    // simulation supplies this source; strict management metadata revalidates
    // current placed house, canonical custody, native parent and Enter access.
    // It exposes no contents and deliberately works while rent has expired.
    const auto source=*request.occupied_source;
    if(!houses||source.zone_id==712||!source.zone_id||!source.instance_id||source.instance_id>32000)throw Failure{Error::Unauthorized};
    auto current=Scene::Service::ReadInteriorManagementUnderTransaction(db,request.lease,source,fence,*houses);if(!current)throw Failure{current.error};
    if(!current.value.house||current.value.context.estate_id!=request.estate_id||current.value.context.native_plot_id!=request.native_plot_id||!current.value.access.effective.Allows(AccessPolicy::Capability::Manage))throw Failure{Error::Unauthorized};
    auto parent=Q(db,"SELECT definition_id FROM housing_native_estates WHERE native_id="+std::to_string(request.native_plot_id)+" AND ownership_estate_id="+std::to_string(request.estate_id)+" AND active=1 FOR UPDATE");if(parent.RowCount()!=1||!N(parent.begin()[0])||N(parent.begin()[0])>=UINT32_MAX)throw Failure{Error::Conflict};
    definition_id=uint32_t(N(parent.begin()[0]));catalog_revision=current.value.context.catalog_revision;
   }else{
    auto scene=Scene::Service::ReadSceneUnderTransaction(db,request.lease,request.estate_id,fence,houses,crates);if(!scene)throw Failure{scene.error};
    if(scene.value.native_plot_id!=request.native_plot_id||!scene.value.access.Allows(AccessPolicy::Capability::Manage))throw Failure{Error::Unauthorized};
    definition_id=scene.value.definition.id;catalog_revision=scene.value.current_catalog_revision;
   }
   auto catalog=Q(db,"SELECT c.revision,HEX(c.definitions_blob),HEX(c.regions_blob) FROM housing_native_estates n JOIN housing_catalogs c ON c.catalog_key=n.catalog_key WHERE n.native_id="+std::to_string(request.native_plot_id)+" AND n.ownership_estate_id="+std::to_string(request.estate_id)+" AND n.definition_id="+std::to_string(definition_id)+" AND n.active=1 FOR UPDATE");
   if(catalog.RowCount()!=1||N(catalog.begin()[0])!=catalog_revision)throw Failure{Error::Conflict};
   auto bundle=Catalog::DecodePayloads(Unhex(catalog.begin()[1]),Unhex(catalog.begin()[2]));
   auto definition=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==definition_id;});
   if(definition==bundle.definitions.end()||(definition->type!=4&&definition->type!=8)||!Offered(*definition,bundle.costs,request.component_id))throw Failure{Error::InvalidArgument};
   auto owners=Q(db,"SELECT owner_kind,owner_id FROM housing_current_estates WHERE id="+std::to_string(request.estate_id)+" FOR UPDATE");if(owners.RowCount()!=1)throw Failure{Error::NotFound};auto owner=owners.begin();const auto kind=N(owner[0]),id=N(owner[1]);
   auto state=Q(db,"SELECT revision FROM housing_upkeep_selection WHERE estate_id="+std::to_string(request.estate_id)+" FOR UPDATE");if(state.RowCount()>1)throw Failure{Error::DatabaseFailure};revision=state.RowCount()?N(state.begin()[0]):0;if(revision==UINT64_MAX||(state.RowCount()&&!revision))throw Failure{Error::Conflict};++revision;
   Q(db,"INSERT INTO housing_upkeep_selection(estate_id,native_plot_id,owner_kind,owner_id,definition_id,component_id,revision) VALUES ("+std::to_string(request.estate_id)+","+std::to_string(request.native_plot_id)+","+std::to_string(kind)+","+std::to_string(id)+","+std::to_string(definition_id)+","+std::to_string(request.component_id)+","+std::to_string(revision)+") ON DUPLICATE KEY UPDATE native_plot_id=VALUES(native_plot_id),owner_kind=VALUES(owner_kind),owner_id=VALUES(owner_id),definition_id=VALUES(definition_id),component_id=VALUES(component_id),revision=VALUES(revision)");
   Q(db,"INSERT INTO housing_upkeep_selection_receipts(request_key,character_id,account_id,lease_epoch,session_key,lease_token,estate_id,native_plot_id,component_id,selection_revision) VALUES ("+key+","+std::to_string(request.lease.actor.character_id)+","+std::to_string(request.lease.actor.account_id)+","+std::to_string(request.lease.epoch)+","+Quote(db,request.lease.session_key)+",x'"+Hex(request.lease.token)+"',"+std::to_string(request.estate_id)+","+std::to_string(request.native_plot_id)+","+std::to_string(request.component_id)+","+std::to_string(revision)+")");
  }
  committing=true;auto committed=db.QueryDatabase(std::string("COMMIT"),false);active=false;if(!committed.Success())return {{},Error::CommitUncertain};return {revision,Error::None};
 }catch(const Failure& e){if(active)try{db.QueryDatabase(std::string("ROLLBACK"),false);}catch(...){}return {{},committing?Error::CommitUncertain:e.error};}
 catch(...){if(active)try{db.QueryDatabase(std::string("ROLLBACK"),false);}catch(...){}return {{},committing?Error::CommitUncertain:Error::DatabaseFailure};}
}
}
#endif
