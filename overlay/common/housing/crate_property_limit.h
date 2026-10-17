#ifndef EQEMU_HOUSING_CRATE_PROPERTY_LIMIT_H
#define EQEMU_HOUSING_CRATE_PROPERTY_LIMIT_H
#include "house_item_scope_db.h"
#include <set>

namespace Housing::CratePropertyLimit {
// Original RoF2 eqstr5243 limits placed crates across the whole property,
// including its house. Room custody retains its historical estate; current
// property membership must instead follow the hosting house's native parent.
// The caller owns the custody transaction. Locking the ownership estate also
// serializes this check with ordinary placement and current house relocation.
inline uint64_t CountPlacedUnderTransaction(Database& db, uint32_t estate, uint64_t except_crate=0) {
 if(!estate)throw std::invalid_argument("Missing crate property");
 auto tx=HouseItemScope::Query(db,"SELECT @@in_transaction");
 if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")throw std::invalid_argument("Crate property requires transaction");
 auto locked=HouseItemScope::Query(db,"SELECT id FROM housing_current_estates WHERE id="+std::to_string(estate)+" FOR UPDATE");
 if(locked.RowCount()!=1)throw std::invalid_argument("Missing crate property");
 auto rows=HouseItemScope::Query(db,
  "SELECT e.crate_id,e.ownership_estate_id,e.parent_native_estate_id,n.parent_native_id,n.active,"
  "c.estate_id,scope.house_id,hosting.location_kind,hosting.custody_id,i.native_estate_id,"
  "parent.plot_id,parent.ownership_estate_id,outer_plot.plot_id,outer_plot.ownership_estate_id,"
  "storage.stowed,house_storage.stowed,h.pending_request_key "
  "FROM housing_crate_estates e JOIN housing_crate_locations l ON l.crate_id=e.crate_id AND l.location_kind=2 "
  "JOIN housing_native_estates n ON n.native_id=e.native_estate_id "
  "JOIN housing_live_item_custody c ON c.custody_id=l.custody_id "
  "JOIN housing_native_estates parent ON parent.native_id=e.parent_native_estate_id "
  "LEFT JOIN housing_live_item_storage storage ON storage.custody_id=c.custody_id "
  "LEFT JOIN housing_house_item_scopes scope ON scope.custody_id=c.custody_id "
  "LEFT JOIN housing_house_interiors i ON i.house_id=scope.house_id "
  "LEFT JOIN housing_house_instances h ON h.house_id=scope.house_id "
  "LEFT JOIN housing_house_locations hosting ON hosting.house_id=scope.house_id "
  "LEFT JOIN housing_native_estates outer_plot ON outer_plot.native_id=parent.parent_native_id "
  "LEFT JOIN housing_live_item_storage house_storage ON house_storage.custody_id=hosting.custody_id "
  "WHERE e.ownership_estate_id="+std::to_string(estate)+" OR outer_plot.ownership_estate_id="+std::to_string(estate)+
  " ORDER BY e.crate_id LIMIT 1001 FOR UPDATE");
 if(rows.RowCount()>1000)throw std::invalid_argument("Too many property crate bindings");
 std::set<uint64_t> counted;
 auto number=[](const char* value)->uint64_t {
  if(!value||!*value)throw std::invalid_argument("Invalid property crate facts");
  std::string s(value);if(s.find_first_not_of("0123456789")!=std::string::npos)throw std::invalid_argument("Invalid property crate facts");
  return std::stoull(s);
 };
 for(auto row=rows.begin();row.operator!=(rows.end());++row){
  const auto id=number(row[0]);
  if(!id||number(row[2])!=number(row[3])||number(row[4])!=1||number(row[1])!=number(row[5]))throw std::invalid_argument("Inconsistent placed crate property");
  const auto stowed=row[14]?number(row[14]):0;
  if(stowed>1)throw std::invalid_argument("Invalid crate storage state");
  uint64_t current=0;
  if(!row[6]){
   if(!row[10]||!number(row[10])||number(row[11])!=number(row[1]))throw std::invalid_argument("Invalid yard crate parent");
   current=number(row[11]);
  }else{
   if(!number(row[6])||number(row[9])!=number(row[2])||row[16])throw std::invalid_argument("Invalid interior crate parent");
   const auto location=number(row[7]);
   if(location!=1&&location!=2)throw std::invalid_argument("Invalid hosting house location");
   const auto house_stowed=row[15]?number(row[15]):0;
   if(house_stowed>1)throw std::invalid_argument("Invalid hosting house storage");
   if(location==2&&!house_stowed){
    if(!row[8]||!number(row[8])||!row[12]||!number(row[12]))throw std::invalid_argument("Missing current hosting property");
    current=number(row[13]);
   }
  }
  if(current==estate&&!stowed&&id!=except_crate)counted.insert(id);
 }
 return counted.size();
}
inline bool AvailableUnderTransaction(Database& db,uint32_t estate,uint64_t except_crate=0){
 return CountPlacedUnderTransaction(db,estate,except_crate)==0;
}
}
#endif
