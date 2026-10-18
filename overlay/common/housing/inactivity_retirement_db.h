#ifndef EQEMU_HOUSING_INACTIVITY_RETIREMENT_DB_H
#define EQEMU_HOUSING_INACTIVITY_RETIREMENT_DB_H
#include "estate_retirement_db.h"
#include "upkeep_selection_db.h"
namespace Housing::InactivityRetirement {
constexpr uint32_t InactiveSeconds=90u*24u*3600u;
enum class Status {NotDue,Retired,Deferred,Conflict,DatabaseFailure,CommitUncertain};
namespace Detail {
using EstateRetirement::Detail::Q;using EstateRetirement::Detail::N;using EstateRetirement::Detail::Quote;
struct Stop{Status status;};
inline void Empty(Database& db,const std::string& query){if(Q(db,query+" LIMIT 1 FOR UPDATE").RowCount())throw Stop{Status::Deferred};}
inline bool Installed(Database& db){auto r=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_inactivity_policy','housing_inactivity_retirements')");if(!r.RowCount())return false;if(r.RowCount()!=2)throw Stop{Status::DatabaseFailure};for(auto p=r.begin();p!=r.end();++p)if(!p[0]||std::string(p[0])!="InnoDB")throw Stop{Status::DatabaseFailure};auto core=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_estates','housing_native_estates','housing_catalogs','character_data','account','housing_inventory_sessions','housing_live_item_leases','housing_plot_definitions','housing_live_item_custody','housing_live_item_storage','housing_live_placed_items','housing_fixture_item_custody','housing_live_placement_reservations','housing_live_escrow_pending','housing_live_escrow_receipts','housing_escrow_balances')");if(core.RowCount()!=1||N(core.begin()[0])!=16)throw Stop{Status::DatabaseFailure};return true;}
}
// Server-owned empty-property disposition. No simulated Client lease, item
// delivery or wallet selection. Occupied/nonzero-escrow properties are deferred.
inline Status RetireEmpty(Database& db,uint32_t estate,uint32_t now){
 using namespace Detail;bool active=false,committing=false;
 try{if(!Installed(db)||!estate||now<InactiveSeconds)return Status::NotDue;auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0]))return Status::Conflict;Q(db,"START TRANSACTION");active=true;
 auto row=Q(db,"SELECT e.plot_id,e.owner_id,c.account_id,c.last_login,n.native_id,n.catalog_key,n.definition_id,p.native_plot_id,p.catalog_revision,cat.revision,p.inactive_seconds,p.review_reference,HEX(cat.definitions_blob),HEX(cat.regions_blob) FROM housing_current_estates e JOIN character_data c ON c.id=e.owner_id AND c.deleted_at IS NULL JOIN account a ON a.id=c.account_id JOIN housing_native_estates n ON n.ownership_estate_id=e.id AND n.plot_id=e.plot_id AND n.active=1 JOIN housing_catalogs cat ON cat.catalog_key=n.catalog_key JOIN housing_inactivity_policy p ON p.estate_id=e.id WHERE e.id="+std::to_string(estate)+" AND e.owner_kind=1 AND cat.client_build=20130510 AND cat.origin IN(1,2) FOR UPDATE");
 if(!row.RowCount())throw Stop{Status::NotDue};if(row.RowCount()!=1)throw Stop{Status::Conflict};auto r=row.begin();const auto plot=N(r[0]),owner=N(r[1]),account=N(r[2]),last=N(r[3]),native=N(r[4]);if(!plot||!owner||!account||!native||native>=UINT32_MAX||N(r[7])!=native||N(r[8])!=N(r[9])||N(r[10])!=InactiveSeconds||!r[11]||!r[11][0])throw Stop{Status::Conflict};if(!last||last>uint64_t(now)-InactiveSeconds)throw Stop{Status::NotDue};auto bundle=Catalog::DecodePayloads(UpkeepSelection::Detail::Unhex(r[12]),UpkeepSelection::Detail::Unhex(r[13]));auto definition=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==N(r[6]);});if(definition==bundle.definitions.end()||definition->type!=4)throw Stop{Status::Conflict};
 // Session acquisition locks this same character row before creating its active
 // reservation. This lock and actual session/pending checks serialize login.
 // A foreign creator may have a Return pending after its custody left this
 // property. Until receipt-to-estate indexing exists, defer on any live item
 // application, rather than incorrectly treating an empty scene as settled.
 Empty(db,"SELECT character_id FROM housing_live_item_leases WHERE pending_key IS NOT NULL");
 Empty(db,"SELECT character_id FROM housing_inventory_sessions WHERE character_id="+std::to_string(owner)+" AND active=1");
 Empty(db,"SELECT character_id FROM housing_live_item_leases WHERE character_id="+std::to_string(owner)+" AND (active=1 OR pending_key IS NOT NULL)");if(CoinPending::Inspect(db,uint32_t(owner),true)!=CoinPending::State::Clear)throw Stop{Status::Deferred};
 if(Q(db,"SELECT id FROM housing_plot_definitions WHERE id="+std::to_string(plot)+" FOR UPDATE").RowCount()!=1)throw Stop{Status::Conflict};
 Empty(db,"SELECT custody_id FROM housing_live_item_custody WHERE estate_id="+std::to_string(estate));
 Empty(db,"SELECT custody_id FROM housing_live_item_storage WHERE native_estate_id="+std::to_string(native));
 Empty(db,"SELECT p.custody_id FROM housing_live_placed_items p JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE r.native_estate_id="+std::to_string(native));
 Empty(db,"SELECT instance_key FROM housing_fixture_item_custody WHERE estate_id="+std::to_string(estate));
 Empty(db,"SELECT native_id FROM housing_native_estates WHERE parent_native_id="+std::to_string(native)+" AND active=1");
 Empty(db,"SELECT native_item_id FROM housing_live_placement_reservations WHERE native_estate_id="+std::to_string(native)+" AND state=0");
 Empty(db,"SELECT p.request_key FROM housing_live_escrow_pending p JOIN housing_live_escrow_receipts r ON r.request_key=p.request_key WHERE r.estate_id="+std::to_string(estate));
 // Historical custody may refer to a retired generation while physically held
 // in its yard. Vault records and mixed scope must also be proven empty.
 auto vault=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_vault_items'");if(vault.RowCount()){if(vault.RowCount()!=1||!vault.begin()[0]||std::string(vault.begin()[0])!="InnoDB")throw Stop{Status::DatabaseFailure};Empty(db,"SELECT custody_id FROM housing_live_vault_items WHERE realm_estate_id="+std::to_string(estate));}
 auto balance=Q(db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(balance.RowCount()>1)throw Stop{Status::Conflict};if(balance.RowCount()&&N(balance.begin()[0]))throw Stop{Status::Deferred};
 auto detached=Q(db,"UPDATE housing_native_estates SET ownership_estate_id=NULL WHERE native_id="+std::to_string(native)+" AND ownership_estate_id="+std::to_string(estate)+" AND plot_id="+std::to_string(plot)+" AND active=1");if(detached.RowsAffected()!=1)throw Stop{Status::Conflict};auto retired=Q(db,"UPDATE housing_estates SET active=0 WHERE id="+std::to_string(estate)+" AND owner_kind=1 AND owner_id="+std::to_string(owner)+" AND active=1");if(retired.RowsAffected()!=1)throw Stop{Status::Conflict};
 Q(db,"INSERT INTO housing_inactivity_retirements(estate_id,native_plot_id,plot_id,character_id,account_id,last_login,retired_epoch,catalog_revision,review_reference) VALUES("+std::to_string(estate)+","+std::to_string(native)+","+std::to_string(plot)+","+std::to_string(owner)+","+std::to_string(account)+","+std::to_string(last)+","+std::to_string(now)+","+std::to_string(N(r[9]))+","+Quote(db,r[11])+")");committing=true;auto c=db.QueryDatabase(std::string("COMMIT"),false);active=false;return c.Success()?Status::Retired:Status::CommitUncertain;
 }catch(const Stop& e){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return committing?Status::CommitUncertain:e.status;}catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return committing?Status::CommitUncertain:Status::DatabaseFailure;}
}
struct Batch{uint32_t retired=0,deferred=0;bool refresh=false;};
inline Batch ProcessDue(Database& db,uint32_t now,uint32_t& after_estate,Status (*delivery)(Database&,uint32_t,uint32_t)=nullptr){using namespace Detail;Batch out;if(!Installed(db)||now<InactiveSeconds)return out;auto read=[&](){return Q(db,"SELECT e.id FROM housing_inactivity_policy p JOIN housing_current_estates e ON e.id=p.estate_id AND e.owner_kind=1 JOIN character_data c ON c.id=e.owner_id AND c.last_login>0 AND c.last_login<="+std::to_string(now-InactiveSeconds)+" WHERE e.id>"+std::to_string(after_estate)+" ORDER BY e.id LIMIT 100");};auto due=read();if(!due.RowCount()&&after_estate){after_estate=0;due=read();}for(auto p=due.begin();p!=due.end();++p){auto id=N(p[0]);if(!id||id>UINT32_MAX)throw Stop{Status::Conflict};after_estate=uint32_t(id);auto result=RetireEmpty(db,uint32_t(id),now);if(result==Status::Deferred&&delivery)result=delivery(db,uint32_t(id),now);if(result==Status::Retired){++out.retired;out.refresh=true;}else if(result==Status::CommitUncertain)out.refresh=true;else if(result!=Status::NotDue)++out.deferred;}return out;}
}
#endif
