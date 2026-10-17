#ifndef EQEMU_HOUSING_RENT_STATE_DB_H
#define EQEMU_HOUSING_RENT_STATE_DB_H
#include "house_item_scope_db.h"
#include "access_policy.h"
#include <mysqld_error.h>
namespace Housing::RentState {
struct State {uint32_t expired_since=0;bool assessment_pending=false;bool Active()const{return !expired_since&&!assessment_pending;}};
inline uint64_t Number(const char* text){if(!text)throw std::invalid_argument("Incomplete rent state");std::string s=text;size_t end=0;auto n=std::stoull(s,&end);if(end!=s.size()||s.empty()||s[0]=='-')throw std::invalid_argument("Invalid rent number");return n;}
inline MySQLRequestResult Query(Database& db,const std::string& sql){return HouseItemScope::Query(db,sql);}
// Optional absence preserves unenrolled existing properties. A partial schema,
// a stale generation/native identity, and SQL errors never prove paid access.
inline State Plot(Database& db,uint32_t estate,uint32_t native=0,bool lock=true){
 auto tables=Query(db,"SELECT table_name,engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_upkeep_contracts','housing_upkeep_property_state')");
 if(!tables.RowCount())return {};if(tables.RowCount()!=2)throw std::invalid_argument("Partial rent schema");for(auto p=tables.begin();p!=tables.end();++p)if(!p[1]||std::string(p[1])!="InnoDB")throw std::invalid_argument("Invalid rent engine");
 auto rows=Query(db,"SELECT c.native_plot_id,c.next_due_epoch,UNIX_TIMESTAMP(),s.native_plot_id,s.expired_since FROM housing_upkeep_contracts c LEFT JOIN housing_upkeep_property_state s ON s.estate_id=c.estate_id WHERE c.estate_id="+std::to_string(estate)+(lock?" FOR UPDATE":""));
 if(!rows.RowCount())return {};if(rows.RowCount()!=1)throw std::invalid_argument("Ambiguous rent contract");auto r=rows.begin();auto source=Number(r[0]);if(!source||source>=UINT32_MAX||(native&&native!=source)||(r[3]&&Number(r[3])!=source)||(r[4]&&Number(r[4])>UINT32_MAX))throw std::invalid_argument("Rent native identity mismatch");
 return {r[4]?uint32_t(Number(r[4])):0,Number(r[1])<=Number(r[2])};
}
inline uint32_t ItemExpired(Database& db,uint64_t custody,bool lock=true){
 auto rows=db.QueryDatabase("SELECT expired_since FROM housing_upkeep_item_state WHERE custody_id="+std::to_string(custody)+(lock?" FOR UPDATE":""),false);
 if(!rows.Success()){if(rows.ErrorNumber()==ER_NO_SUCH_TABLE)return 0;throw std::invalid_argument("Item rent query failed");}if(rows.RowCount()>1||(rows.RowCount()&&Number(rows.begin()[0])>UINT32_MAX))throw std::invalid_argument("Invalid item expiry");return rows.RowCount()?uint32_t(Number(rows.begin()[0])):0;
}
inline bool PlotActive(Database& db,uint32_t estate,uint32_t native=0){return Plot(db,estate,native).Active();}
// An expired house stays placed. Ordinary delinquent storage may still be
// returned to its actual creator; only placement requires its funding guard.
inline bool HouseActive(Database& db,uint64_t custody){
 auto tables=Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_house_locations'");if(!tables.RowCount())return true;if(tables.RowCount()!=1||!tables.begin()[0]||std::string(tables.begin()[0])!="InnoDB")throw std::invalid_argument("Invalid house rent schema");
 auto house=Query(db,"SELECT house_id FROM housing_house_locations WHERE location_kind=2 AND custody_id="+std::to_string(custody)+" FOR UPDATE");if(house.RowCount()>1)throw std::invalid_argument("Ambiguous rented house");return !house.RowCount()||!ItemExpired(db,custody);
}
// Furnished rooms retain their original custody estate when their house moves.
// Billing/permissions follow the actual current placed host, never that history.
inline uint32_t BillingEstate(Database& db,uint64_t custody,uint32_t fallback){
 auto room=HouseItemScope::Locked(db,custody);if(!room)return fallback;
 auto host=Query(db,"SELECT c.estate_id FROM housing_house_locations l JOIN housing_live_item_custody c ON c.custody_id=l.custody_id JOIN housing_live_placed_items p ON p.custody_id=c.custody_id WHERE l.location_kind=2 AND l.house_id="+std::to_string(room->house_id)+" FOR UPDATE");if(host.RowCount()!=1||!Number(host.begin()[0])||Number(host.begin()[0])>UINT32_MAX)throw std::invalid_argument("Invalid current room rent host");return uint32_t(Number(host.begin()[0]));
}
inline bool MutableItem(Database& db,uint32_t estate,uint32_t native,uint64_t custody){
 auto billing=BillingEstate(db,custody,estate);if(!PlotActive(db,billing,billing==estate?native:0)||!HouseActive(db,custody))return false;
 auto room=HouseItemScope::Locked(db,custody);if(!room)return true;auto host=Query(db,"SELECT custody_id FROM housing_house_locations WHERE location_kind=2 AND house_id="+std::to_string(room->house_id)+" FOR UPDATE");if(host.RowCount()!=1)throw std::invalid_argument("Invalid current room house");return HouseActive(db,Number(host.begin()[0]));
}
// This is operation eligibility, not a persisted ACL edit. Keep Enter for a
// read-only yard/management view; actual house/anchor entry is checked separately.
// Escrow authorization remains with its existing service.
inline constexpr uint32_t ManagementCapabilities=(1u<<uint8_t(AccessPolicy::Capability::Enter))|(1u<<uint8_t(AccessPolicy::Capability::WithdrawUpkeep))|(1u<<uint8_t(AccessPolicy::Capability::Manage));
inline void Restrict(AccessPolicy::EffectiveAccess& access,const State& state){if(!state.Active())access.capabilities&=ManagementCapabilities;}
inline void Restrict(Database& db,uint32_t estate,uint32_t native,AccessPolicy::EffectiveAccess& access){Restrict(access,Plot(db,estate,native));}
}
#endif
