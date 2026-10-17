#ifndef EQEMU_HOUSING_HOUSE_ITEM_SCOPE_DB_H
#define EQEMU_HOUSING_HOUSE_ITEM_SCOPE_DB_H
#include "../database.h"
#include <optional>
#include <stdexcept>
namespace Housing::HouseItemScope {
struct Membership {uint64_t custody_id=0,house_id=0,revision=0;};
// Caller owns the custody transaction and its real lease/session fence.
// Membership is containment, never an ACL or permission witness.
inline MySQLRequestResult Query(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::invalid_argument("House item scope query failed");return r;}
inline bool Installed(Database& db){
 auto tables=Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_house_item_scopes'");
 if(!tables.RowCount())return false;
 if(tables.RowCount()!=1||!tables.begin()[0]||std::string(tables.begin()[0])!="InnoDB")throw std::invalid_argument("Invalid house item scope schema");
 auto columns=Query(db,"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_house_item_scopes' AND column_name IN ('custody_id','house_id','revision')");
 if(columns.RowCount()!=1||std::stoul(columns.begin()[0])!=3)throw std::invalid_argument("Partial house item scope schema");return true;
}
inline std::optional<Membership> Locked(Database& db,uint64_t custody){
 if(!custody)throw std::invalid_argument("Invalid house item custody");if(!Installed(db))return {};
 auto rows=Query(db,"SELECT house_id,revision FROM housing_house_item_scopes WHERE custody_id="+std::to_string(custody)+" FOR UPDATE");
 if(!rows.RowCount())return {};if(rows.RowCount()!=1||!rows.begin()[0]||!rows.begin()[1])throw std::invalid_argument("Invalid house item scope");
 Membership m{custody,std::stoull(rows.begin()[0]),std::stoull(rows.begin()[1])};if(!m.house_id||!m.revision)throw std::invalid_argument("Invalid house item identity");return m;
}
inline std::string ExteriorPredicate(Database& db){return Installed(db)?" AND NOT EXISTS (SELECT 1 FROM housing_house_item_scopes room_scope WHERE room_scope.custody_id=c.custody_id)":"";}
}
#endif
