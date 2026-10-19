#ifndef EQEMU_HOUSING_UPKEEP_ENROLLMENT_DB_H
#define EQEMU_HOUSING_UPKEEP_ENROLLMENT_DB_H
#include "../database.h"
#include "upkeep_clock.h"
#include <optional>
#include <stdexcept>
#include <openssl/sha.h>
namespace Housing::UpkeepEnrollment {
struct Policy {std::string review;bool allow_bulk_carried_copper=false;uint32_t max_catchup_hours=0;};
inline MySQLRequestResult Q(Database& db,const std::string& text){auto r=db.QueryDatabase(text,false);if(!r.Success())throw std::runtime_error("Upkeep enrollment query failed");return r;}
inline std::string Quote(Database& db,const std::string& s){return "'"+db.Escape(s)+"'";}
// This optional server policy authorizes copper-only purchase+rent enrollment.
// It supplies neither tariffs nor a grant to purchase. The existing authenticated
// NativePlotPurchase transaction remains responsible for the acquisition fence.
inline std::optional<Policy> Read(Database& db,const std::string& key,uint32_t definition,uint64_t revision,const std::string& definitions,const std::string& regions){
 auto table=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_upkeep_enrollment_policy'");if(!table.RowCount())return {};if(table.RowCount()!=1||!table.begin()[0]||std::string(table.begin()[0])!="InnoDB")throw std::runtime_error("Invalid enrollment engine");
 auto column=Q(db,"SELECT data_type FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_upkeep_enrollment_policy' AND column_name='allow_bulk_carried_copper'");if(column.RowCount()>1||(column.RowCount()&&(!column.begin()[0]||std::string(column.begin()[0])!="tinyint")))throw std::runtime_error("Invalid bulk enrollment column");const bool has_bulk=column.RowCount()==1;
 auto catchup_column=Q(db,"SELECT data_type FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_upkeep_enrollment_policy' AND column_name='max_catchup_hours'");if(catchup_column.RowCount()>1||(catchup_column.RowCount()&&(!catchup_column.begin()[0]||std::string(catchup_column.begin()[0])!="int")))throw std::runtime_error("Invalid catch-up enrollment column");const bool has_catchup=catchup_column.RowCount()==1;
 auto r=Q(db,"SELECT catalog_revision,definitions_sha256,regions_sha256,mode,review_reference"+std::string(has_bulk?",allow_bulk_carried_copper":"")+std::string(has_catchup?",max_catchup_hours":"")+" FROM housing_upkeep_enrollment_policy WHERE catalog_key="+Quote(db,key)+" AND definition_id="+std::to_string(definition)+" FOR UPDATE");if(!r.RowCount())return {};auto p=r.begin();if(r.RowCount()!=1||!p[0]||std::stoull(p[0])!=revision||!p[1]||definitions!=p[1]||!p[2]||regions!=p[2]||!p[3]||std::string(p[3])!="1"||!p[4]||!p[4][0])throw std::runtime_error("Stale upkeep enrollment review");
 auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_upkeep_contracts','housing_upkeep_selection','housing_upkeep_rounding_policy','housing_upkeep_cycles','housing_upkeep_cycle_items','housing_upkeep_item_state','housing_upkeep_property_state','housing_upkeep_property_cycles','housing_upkeep_cycle_scopes','housing_upkeep_reactivation_policy')");if(engines.RowCount()!=1||!engines.begin()[0]||std::string(engines.begin()[0])!="10")throw std::runtime_error("Missing enrollment schema");if(has_bulk&&(!p[5]||(std::string(p[5])!="0"&&std::string(p[5])!="1")))throw std::runtime_error("Invalid bulk enrollment flag");uint32_t max_catchup_hours=0;if(has_catchup){const auto index=has_bulk?6u:5u;if(!p[index])throw std::runtime_error("Missing catch-up enrollment horizon");const auto hours=std::stoull(p[index]);if(hours>2160)throw std::runtime_error("Invalid catch-up enrollment horizon");max_catchup_hours=uint32_t(hours);}return Policy{p[4],has_bulk&&std::string(p[5])=="1",max_catchup_hours};
}
// Caller already holds the purchase TX and current-generation/native/catalog
// locks. Ordinary INSERTs refuse all collisions; rollback includes acquisition,
// currency, selection, initial due clock, rounding and funded recovery authority
// together. Recovery applies only to this acquired generation and catalogue;
// it never enables unwitnessed historical arrears or enrolls legacy properties.
inline void Enroll(Database& db,uint32_t estate,uint32_t native,uint32_t owner_kind,uint32_t owner,uint32_t definition,uint32_t component,uint64_t revision,const Policy& policy){
 auto tx=Q(db,"SELECT @@in_transaction,UNIX_TIMESTAMP()");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1"||!tx.begin()[1])throw std::runtime_error("Enrollment requires purchase transaction");const auto now=std::stoull(tx.begin()[1]);if(!now||now>UINT32_MAX-3600||policy.review.empty()||policy.review.size()>255||!estate||!native||!owner||!revision||!component||policy.max_catchup_hours>2160||(owner_kind!=1&&owner_kind!=3))throw std::runtime_error("Invalid enrollment identity");
 auto current=Q(db,"SELECT e.id,c.catalog_key,SHA2(c.definitions_blob,256),SHA2(c.regions_blob,256) FROM housing_current_estates e JOIN housing_native_estates n ON n.ownership_estate_id=e.id AND n.plot_id=e.plot_id AND n.active=1 JOIN housing_catalogs c ON c.catalog_key=n.catalog_key WHERE e.id="+std::to_string(estate)+" AND e.owner_kind="+std::to_string(owner_kind)+" AND e.owner_id="+std::to_string(owner)+" AND n.native_id="+std::to_string(native)+" AND n.definition_id="+std::to_string(definition)+" AND c.revision="+std::to_string(revision)+" FOR UPDATE");if(current.RowCount()!=1)throw std::runtime_error("Enrollment current owner mismatch");
 // Opt-in review is re-read against the actual locked catalogue, not caller
 // supplied hashes. Absence/default zero never creates catch-up authority.
 if(policy.max_catchup_hours){
  auto actual=current.begin();if(!actual[1]||!actual[2]||!actual[3])throw std::runtime_error("Missing catch-up enrollment catalogue");
  auto reviewed=Read(db,actual[1],definition,revision,actual[2],actual[3]);
  if(!reviewed||reviewed->review!=policy.review||reviewed->allow_bulk_carried_copper!=policy.allow_bulk_carried_copper||reviewed->max_catchup_hours!=policy.max_catchup_hours)throw std::runtime_error("Changed catch-up enrollment review");
  auto tables=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_upkeep_catchup_policy','housing_upkeep_catchup_checkpoints','housing_upkeep_catchup_audits')");if(tables.RowCount()!=1||!tables.begin()[0]||std::string(tables.begin()[0])!="3")throw std::runtime_error("Missing catch-up enrollment schema");
  Q(db,"INSERT INTO housing_upkeep_catchup_policy(estate_id,native_plot_id,owner_kind,owner_id,catalog_revision,definitions_sha256,regions_sha256,max_overdue_hours,enabled,review_reference) VALUES("+std::to_string(estate)+","+std::to_string(native)+","+std::to_string(owner_kind)+","+std::to_string(owner)+","+std::to_string(revision)+","+Quote(db,actual[2])+","+Quote(db,actual[3])+","+std::to_string(policy.max_catchup_hours)+",1,"+Quote(db,policy.review)+")");
  // No checkpoint is manufactured. Only a subsequent successful real on-time
  // assessment can record the state required for witnessed catch-up.
 }
 Q(db,"INSERT INTO housing_upkeep_contracts(estate_id,native_plot_id,owner_kind,owner_id,catalog_revision,next_due_epoch,revision,review_reference) VALUES("+std::to_string(estate)+","+std::to_string(native)+","+std::to_string(owner_kind)+","+std::to_string(owner)+","+std::to_string(revision)+","+std::to_string(now+3600)+",1,"+Quote(db,policy.review)+")");
 Q(db,"INSERT INTO housing_upkeep_selection VALUES("+std::to_string(estate)+","+std::to_string(native)+","+std::to_string(owner_kind)+","+std::to_string(owner)+","+std::to_string(definition)+","+std::to_string(component)+",1)");
 Q(db,"INSERT INTO housing_upkeep_rounding_policy VALUES("+std::to_string(estate)+","+std::to_string(native)+","+std::to_string(revision)+",1,"+Quote(db,policy.review)+")");
 Q(db,"INSERT INTO housing_upkeep_reactivation_policy(estate_id,native_plot_id,catalog_revision,mode,review_reference) VALUES("+std::to_string(estate)+","+std::to_string(native)+","+std::to_string(revision)+",1,"+Quote(db,policy.review)+")");
 if(policy.allow_bulk_carried_copper){
  auto table=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_escrow_bulk_policy'");if(table.RowCount()!=1||!table.begin()[0]||std::string(table.begin()[0])!="InnoDB")throw std::runtime_error("Missing bulk enrollment schema");
  std::string reference="enrollment:"+policy.review;
  if(reference.size()>96){unsigned char digest[32];SHA256(reinterpret_cast<const unsigned char*>(policy.review.data()),policy.review.size(),digest);reference="enrollment:sha256:";const char* hex="0123456789abcdef";for(auto byte:digest){reference+=hex[byte>>4];reference+=hex[byte&15];}}
  Q(db,"INSERT INTO housing_escrow_bulk_policy(estate_id,native_plot_id,catalog_revision,mode,enabled,review_reference) VALUES("+std::to_string(estate)+","+std::to_string(native)+","+std::to_string(revision)+",1,1,"+Quote(db,reference)+")");
 }
}
}
#endif
