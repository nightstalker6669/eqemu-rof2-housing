#ifndef EQEMU_HOUSING_CRATE_FOREIGN_MEMBER_POLICY_DB_H
#define EQEMU_HOUSING_CRATE_FOREIGN_MEMBER_POLICY_DB_H
#include "live_item_service.h"
#include "../database.h"
#include "foreign_furniture_return_policy.h"
namespace Housing::CrateForeignMember {
// Eligibility only. Existing callers still prove fresh ACL, creator/source
// equality, current custody/route and whole transaction. Packing cannot bypass
// the same supported normal CanDrop subset as ordinary foreign inventory return.
inline bool EligibleUnderTransaction(Database& db,const LiveItems::InventoryRow& row){
 if(!row.values.character_id||!ForeignFurnitureReturn::NormalMode())return false;
 auto tx=db.QueryDatabase(std::string("SELECT @@in_transaction"),false);if(!tx.Success()||tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return false;
 auto metadata=db.QueryDatabase("SELECT idfile,nodrop,loregroup,itemclass,bagslots,notransfer FROM items WHERE id="+std::to_string(row.values.item_id)+" FOR UPDATE",false);
 if(!metadata.Success()||metadata.RowCount()!=1)return false;auto item=metadata.begin();for(unsigned i=0;i<6;++i)if(!item[i])return false;
 try{const LiveItems::Catalog catalog{std::stoi(item[3]),std::stoi(item[4]),std::stoi(item[2]),std::stoi(item[1])};
 return std::string(item[5])=="0"&&StaticFurniture::Lookup(row.values.item_id,item[0])&&ForeignFurnitureReturn::Eligible(row,catalog);
 }catch(...){return false;}
}
}
#endif
