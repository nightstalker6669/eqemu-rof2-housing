#ifndef EQEMU_HOUSING_CRATE_HOUSE_RULES_H
#define EQEMU_HOUSING_CRATE_HOUSE_RULES_H
#include "house_item_scope_db.h"
#include <set>
#include <vector>
namespace Housing::CrateHouseRules {
// Original RoF2 eqstr5238/5282/5418: admission is capacity limited, at most
// one contained house may be non-empty, and a house with a placed crate
// cannot itself enter a crate. Closet items also make a house non-empty.
struct House {uint64_t id=0;bool nonempty=false,placed_crate=false;};
inline bool Allowed(const std::vector<House>& houses){
 std::set<uint64_t> ids;unsigned nonempty=0;
 for(const auto& house:houses)if(!house.id||!ids.insert(house.id).second||house.placed_crate||(house.nonempty&&++nonempty>1))return false;
 return true;
}
// Caller owns the real custody transaction and fence. Locking the stable
// HouseID serializes this check with normal room mutation and house moves.
inline House LockedHouse(Database& db,uint64_t id){
 if(!id)throw std::invalid_argument("Invalid contained house");
 auto tx=HouseItemScope::Query(db,"SELECT @@in_transaction");
 if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")throw std::invalid_argument("Contained house requires transaction");
 auto identity=HouseItemScope::Query(db,"SELECT house_id FROM housing_house_instances WHERE house_id="+std::to_string(id)+" FOR UPDATE");
 if(identity.RowCount()!=1)throw std::invalid_argument("Missing contained house");
 House out{id};if(!HouseItemScope::Installed(db))return out;
 auto items=HouseItemScope::Query(db,"SELECT s.custody_id,c.custody_id,l.crate_id,storage.stowed FROM housing_house_item_scopes s LEFT JOIN housing_live_item_custody c ON c.custody_id=s.custody_id LEFT JOIN housing_crate_locations l ON l.location_kind=2 AND l.custody_id=s.custody_id LEFT JOIN housing_live_item_storage storage ON storage.custody_id=s.custody_id WHERE s.house_id="+std::to_string(id)+" ORDER BY s.custody_id FOR UPDATE");
 for(auto row=items.begin();row.operator!=(items.end());++row){
  if(!row[0]||!row[1])throw std::invalid_argument("Invalid contained house custody");
  out.nonempty=true;
  if(row[3]&&std::string(row[3])!="0"&&std::string(row[3])!="1")throw std::invalid_argument("Invalid contained crate storage");
  if(row[2]&&(!row[3]||std::string(row[3])=="0"))out.placed_crate=true;
 }
 return out;
}
inline std::vector<House> LockedMembers(Database& db,uint64_t crate){
 if(!crate)throw std::invalid_argument("Invalid house container");
 auto rows=HouseItemScope::Query(db,"SELECT h.house_id FROM housing_crate_members m JOIN housing_house_locations h ON h.location_kind=2 AND h.custody_id=m.custody_id WHERE m.crate_id="+std::to_string(crate)+" ORDER BY h.house_id FOR UPDATE");
 std::vector<House> out;for(auto row=rows.begin();row.operator!=(rows.end());++row){if(!row[0])throw std::invalid_argument("Missing member house");out.push_back(LockedHouse(db,std::stoull(row[0])));}return out;
}
inline bool CanAdmit(Database& db,uint64_t crate,uint64_t house,const std::vector<uint64_t>& planned={}){
 auto members=LockedMembers(db,crate);for(auto id:planned)members.push_back(LockedHouse(db,id));members.push_back(LockedHouse(db,house));return Allowed(members);
}
}
#endif
