#ifndef EQEMU_HOUSING_HOUSE_SHED_STORAGE_H
#define EQEMU_HOUSING_HOUSE_SHED_STORAGE_H
#include "house_swap_preflight.h"
namespace Housing::HouseShedStorage {
// Caller owns the custody transaction, authenticated scene and stable house
// lock. No room items are moved, rewritten or flattened into yard storage.
inline LiveItems::Error QuiescentUnderTransaction(Database& db,const Scene::InteriorSnapshot& room){
 using Error=LiveItems::Error;
 if(!room.house||!room.house->interior||!room.house->pending_key.empty())return Error::PendingApplication;
 auto tx=db.QueryDatabase(std::string("SELECT @@in_transaction"),false);
 if(!tx.Success())return Error::DatabaseFailure;
 if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return Error::InvalidArgument;
 auto engines=db.QueryDatabase(std::string("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_inventory_sessions','housing_house_item_scopes','housing_live_item_custody','housing_live_item_leases','housing_live_placement_reservations','housing_native_estates')"),false);
 if(!engines.Success())return Error::DatabaseFailure;
 if(engines.RowCount()!=1||!engines.begin()[0]||std::string(engines.begin()[0])!="6")return Error::Disabled;
 auto complete=HouseSwapPreflight::CompleteGraphUnderTransaction(db,room);if(complete!=Error::None)return complete;
 const auto& interior=*room.house->interior;
 if(!interior.zone_id||!interior.instance_id||!interior.native_estate_id)return Error::Conflict;
 auto occupants=db.QueryDatabase("SELECT character_id FROM housing_inventory_sessions WHERE zone_id="+std::to_string(interior.zone_id)+" AND instance_id="+std::to_string(interior.instance_id)+" AND active=1 FOR UPDATE",false);
 if(!occupants.Success())return Error::DatabaseFailure;
 if(occupants.RowCount())return Error::Conflict;
 auto pending=db.QueryDatabase("SELECT l.character_id FROM housing_live_item_leases l JOIN housing_live_placement_reservations r ON r.character_id=l.character_id LEFT JOIN housing_native_estates n ON n.native_id=r.native_estate_id WHERE l.pending_key IS NOT NULL AND (r.native_estate_id="+std::to_string(interior.native_estate_id)+" OR n.parent_native_id="+std::to_string(interior.native_estate_id)+") ORDER BY l.character_id FOR UPDATE",false);
 if(!pending.Success())return Error::DatabaseFailure;
 return pending.RowCount()?Error::PendingApplication:Error::None;
}
}
#endif
