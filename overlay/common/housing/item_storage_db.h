#ifndef EQEMU_HOUSING_ITEM_STORAGE_DB_H
#define EQEMU_HOUSING_ITEM_STORAGE_DB_H
#include "house_item_scope_db.h"
namespace Housing::ItemStorage {
struct State {uint64_t revision=0;bool stowed=false;uint32_t native_estate_id=0;};
inline bool Installed(Database& db){
 auto rows=HouseItemScope::Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_item_storage'");
 if(!rows.RowCount())return false;
 if(rows.RowCount()!=1||!rows.begin()[0]||std::string(rows.begin()[0])!="InnoDB")throw std::invalid_argument("Invalid item storage schema");
 return true;
}
// Caller owns the transaction. A storage flag is a disposition, never authority.
inline State Locked(Database& db,uint64_t custody){
 if(!custody)throw std::invalid_argument("Invalid storage custody");if(!Installed(db))return {};
 auto rows=HouseItemScope::Query(db,"SELECT revision,stowed,native_estate_id FROM housing_live_item_storage WHERE custody_id="+std::to_string(custody)+" FOR UPDATE");
 if(!rows.RowCount())return {};if(rows.RowCount()!=1||!rows.begin()[0]||!rows.begin()[1]||!rows.begin()[2])throw std::invalid_argument("Invalid storage state");
 State out{std::stoull(rows.begin()[0]),false,uint32_t(std::stoul(rows.begin()[2]))};auto flag=std::stoul(rows.begin()[1]);
 if(!out.revision||flag>1||!out.native_estate_id||out.native_estate_id==UINT32_MAX)throw std::invalid_argument("Invalid storage identity");out.stowed=flag==1;return out;
}
inline void Retire(Database& db,uint64_t custody){if(Installed(db))HouseItemScope::Query(db,"DELETE FROM housing_live_item_storage WHERE custody_id="+std::to_string(custody));}
}
#endif
