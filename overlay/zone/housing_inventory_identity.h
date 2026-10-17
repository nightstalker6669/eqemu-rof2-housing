#ifndef EQEMU_HOUSING_INVENTORY_IDENTITY_H
#define EQEMU_HOUSING_INVENTORY_IDENTITY_H
#include "../common/housing/live_item_service.h"
#include "../common/inventory_profile.h"
#include "../common/item_instance.h"
#include "housing_item_reconstruction.h"
#include <limits>
namespace Housing::InventoryBridge {
// Persisted GUID/null/raw fields are DB identity, not the runtime serial.
inline bool Matches(const LiveItems::InventoryRow& row, EQ::ItemInstance& item) {return MatchesProjection(row,item);}
inline bool SlotSupported(const EQ::InventoryProfile& inv,uint32_t slot){
 return slot<=uint32_t(EQ::invslot::slotCursor)&&slot>=uint32_t(EQ::invslot::GENERAL_BEGIN)&&inv.GetLookup()&&((inv.GetLookup()->PossessionsBitmask>>slot)&1);
}
inline bool CursorCompatible(EQ::InventoryProfile& inv,uint32_t slot,bool source){return slot!=uint32_t(EQ::invslot::slotCursor)||inv.CursorSize()==(source?1:0);}
}
#endif
