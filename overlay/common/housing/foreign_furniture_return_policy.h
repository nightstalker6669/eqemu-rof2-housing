#ifndef EQEMU_HOUSING_FOREIGN_FURNITURE_RETURN_POLICY_H
#define EQEMU_HOUSING_FOREIGN_FURNITURE_RETURN_POLICY_H
#include "live_item_service.h"
#include "custody_codec.h"
#include "item_payload_db.h"
#include "static_furniture_registry.h"
#include "../rulesys.h"
namespace Housing::ForeignFurnitureReturn {
// Supported ordinary transfer subset. Current ACL, canonical scene, pending
// creator fence, full raw custody equality and live recipient inventory proof
// are separately mandatory. No house/crate/vault transfer is authorized here.
inline bool Shape(const LiveItems::InventoryRow& row,const LiveItems::Catalog& catalog){
 const auto& v=row.values;
 return catalog.item_class==0&&catalog.bag_slots==0&&catalog.no_drop>0&&catalog.lore_group==0&&!v.instnodrop&&!v.ornament_icon&&!v.ornament_idfile&&!v.augment_one&&!v.augment_two&&!v.augment_three&&!v.augment_four&&!v.augment_five&&!v.augment_six;
}
// Exact normal wrapper mode: native LogServer globals select a different
// predicate for enable_FV or mode DWORD6. Flag-changing loader overrides also
// lack a faithful raw-row adapter here. NoTransfer is separately checked under SQL.
inline bool NormalMode(){return RuleI(World,FVNoDropFlag)==0&&RuleI(World,PVPSettings)!=6&&!RuleB(Items,DisableNoDrop)&&!RuleB(Items,DisableNoTransfer)&&!RuleB(Items,DisableAttuneable);}
inline bool Eligible(const LiveItems::InventoryRow& row,const LiveItems::Catalog& catalog){return NormalMode()&&StaticFurniture::Lookup(row.values.item_id)&&Shape(row,catalog);}
// Immutable replay checks committed row/recipient structure, not a mutable
// future admission registry or current ACL. New mutations must use Eligible.
inline bool Outcome(const LiveItems::Receipt& receipt,uint32_t recipient){
 if(receipt.action!=LiveItems::Action::Return||!receipt.stored.placement||receipt.house_effect_required||receipt.house_effect||receipt.crate_before||receipt.crate_allocation||!Shape(receipt.stored.row,receipt.stored.catalog))return false;
 auto expected=receipt.stored.row;ItemPayload::Owner(expected,recipient);expected.values.slot_id=receipt.inventory_after.values.slot_id;
 return CustodyCodec::Encode(expected)==CustodyCodec::Encode(receipt.inventory_after);
}
}
#endif
