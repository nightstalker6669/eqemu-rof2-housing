#ifndef EQEMU_HOUSING_ADMISSION_GUARD_H
#define EQEMU_HOUSING_ADMISSION_GUARD_H
#include "../database.h"
#include "coin_pending_guard.h"
#include <mysqld_error.h>
namespace Housing {
enum class InventoryAdmission {Allowed,ActiveLease,OwnedHousing,MaintenanceRequired,Unavailable};
namespace AdmissionDetail {
inline bool OptionalMissing(Database& db,const std::string& table,const std::string& columns){auto probe=db.QueryDatabase("SELECT "+columns+" FROM "+table+" WHERE 1=0",false);return !probe.Success()&&probe.ErrorNumber()==ER_NO_SUCH_TABLE;}
inline InventoryAdmission CheckTable(Database& db,uint32_t character,const std::string& table,bool pending){
 auto schema=db.QueryDatabase("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='"+table+"'",false);if(!schema.Success())return InventoryAdmission::Unavailable;
 auto columns=std::string("active")+(pending?",pending_key":"");
 if(!schema.RowCount())return OptionalMissing(db,table,columns)?InventoryAdmission::Allowed:InventoryAdmission::Unavailable;
 if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB")return InventoryAdmission::Unavailable;
 auto rows=db.QueryDatabase("SELECT "+columns+" FROM "+table+" WHERE character_id="+std::to_string(character),false);
 if(!rows.Success()||rows.RowCount()>1)return InventoryAdmission::Unavailable;if(!rows.RowCount())return InventoryAdmission::Allowed;
 auto row=rows.begin();if(!row[0]||(std::string(row[0])!="0"&&std::string(row[0])!="1"))return InventoryAdmission::Unavailable;
 return std::string(row[0])=="1"||(pending&&row[1])?InventoryAdmission::ActiveLease:InventoryAdmission::Allowed;
}
inline InventoryAdmission OwnedRows(Database& db,const std::string& table,const std::string& columns,const std::string& predicate){
 auto schema=db.QueryDatabase("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='"+table+"'",false);if(!schema.Success())return InventoryAdmission::Unavailable;
 if(!schema.RowCount())return OptionalMissing(db,table,columns)?InventoryAdmission::Allowed:InventoryAdmission::Unavailable;
 if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB")return InventoryAdmission::Unavailable;
 auto rows=db.QueryDatabase("SELECT "+columns+" FROM "+table+" WHERE "+predicate+" LIMIT 1",false);if(!rows.Success())return InventoryAdmission::Unavailable;return rows.RowCount()?InventoryAdmission::OwnedHousing:InventoryAdmission::Allowed;
}
}
// A zone that already owns the separate preload reservation checks only the
// custody quarantine here; its own active reservation must not reject itself.
inline InventoryAdmission CheckCustodyAdmission(Database& db,uint32_t character,bool enabled) noexcept {
 if(!enabled)return InventoryAdmission::Allowed;if(!character||db.GetStatus()!=DBcore::Connected)return InventoryAdmission::Unavailable;
 try{auto coin=CoinPending::Inspect(db,character);if(coin!=CoinPending::State::Clear)return coin==CoinPending::State::Pending?InventoryAdmission::ActiveLease:InventoryAdmission::Unavailable;return AdmissionDetail::CheckTable(db,character,"housing_live_item_leases",true);}catch(...){return InventoryAdmission::Unavailable;}
}
// Read-only quarantine; reservation Claim is the actual atomic admission gate.
// Optional absence preserves legacy behavior, only exact missing-table proves it.
inline InventoryAdmission CheckInventoryAdmission(Database& db,uint32_t character,bool enabled) noexcept {
 if(!enabled)return InventoryAdmission::Allowed;if(!character||db.GetStatus()!=DBcore::Connected)return InventoryAdmission::Unavailable;
 try{auto coin=CoinPending::Inspect(db,character);if(coin!=CoinPending::State::Clear)return coin==CoinPending::State::Pending?InventoryAdmission::ActiveLease:InventoryAdmission::Unavailable;auto custody=AdmissionDetail::CheckTable(db,character,"housing_live_item_leases",true);if(custody!=InventoryAdmission::Allowed)return custody;return AdmissionDetail::CheckTable(db,character,"housing_inventory_sessions",false);}catch(...){return InventoryAdmission::Unavailable;}
}
// Destructive offline maintenance needs an atomic writer reservation adapter.
// Snapshot checks have TOCTOU, so experimental housing mode withholds the entire
// deletion/restore operation. Rule-off preserves legacy behavior.
inline InventoryAdmission CheckInventoryMaintenance(bool enabled) noexcept {
 return enabled?InventoryAdmission::MaintenanceRequired:InventoryAdmission::Allowed;
}
inline InventoryAdmission CheckHousingCharacterDeletion(Database&,uint32_t,bool enabled) noexcept {
 return CheckInventoryMaintenance(enabled);
}
}
#endif
