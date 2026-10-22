#ifndef EQEMU_HOUSING_FURNITURE_ESTATE_LIMIT_H
#define EQEMU_HOUSING_FURNITURE_ESTATE_LIMIT_H
#include "custody_codec.h"
#include "house_item_scope_db.h"
#include "crate_member_scope_db.h"
#include <stdexcept>
namespace Housing::FurnitureEstateLimit {
// Caller owns the Add transaction and has reviewed the current item metadata.
// Serialize on the physical native estate, including different creators and
// custody originating on an older property generation. No inventory mutation,
// transaction lifecycle, or authority is acquired by this capacity primitive.
inline bool AvailableUnderTransaction(Database& db,uint32_t native_estate,uint32_t item,int32_t maximum){
 if(maximum<=0)return true;
 if(!native_estate||native_estate==UINT32_MAX||!item)throw std::invalid_argument("Invalid furniture estate cap identity");
 auto query=[&](const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::invalid_argument("Furniture estate cap query failed");return r;};
 auto tx=query("SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")throw std::invalid_argument("Furniture estate cap requires owning transaction");
 auto estate=query("SELECT native_id FROM housing_native_estates WHERE native_id="+std::to_string(native_estate)+" AND active=1 FOR UPDATE");
 if(estate.RowCount()!=1)throw std::invalid_argument("Furniture estate cap target absent");
 // Stowed yard furniture still consumes the estate's item allowance, matching
 // interior capacity semantics. Packed crate members and room-scoped items
 // belong to separately authenticated realms and do not consume the yard cap.
 auto rows=query("SELECT HEX(c.row_blob) FROM housing_live_placed_items placed JOIN housing_live_placement_reservations reservation ON reservation.native_item_id=placed.native_item_id AND reservation.state=1 AND reservation.custody_id=placed.custody_id LEFT JOIN housing_live_item_custody c ON c.custody_id=placed.custody_id WHERE reservation.native_estate_id="+std::to_string(native_estate)+HouseItemScope::ExteriorPredicate(db)+CrateMemberScope::ExteriorPredicate(db)+" ORDER BY placed.custody_id LIMIT 10001 FOR UPDATE");
 if(rows.RowCount()>10000)throw std::invalid_argument("Furniture estate cap census exceeds bound");
 uint64_t same=0;
 for(auto row=rows.begin();row.operator!=(rows.end());++row){
  if(!row[0])throw std::invalid_argument("Furniture estate cap custody absent");
  const std::string hex=row[0];if(hex.size()>2*CustodyCodec::MaximumPayload||hex.size()%2)throw std::invalid_argument("Invalid furniture estate cap row");
  auto digit=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw std::invalid_argument("Invalid furniture estate cap hex");};
  std::vector<uint8_t> bytes;bytes.reserve(hex.size()/2);for(size_t i=0;i<hex.size();i+=2)bytes.push_back(uint8_t((digit(hex[i])<<4)|digit(hex[i+1])));
  if(CustodyCodec::Decode(bytes).values.item_id==item)++same;
 }
 return same<uint64_t(maximum);
}
}
#endif
