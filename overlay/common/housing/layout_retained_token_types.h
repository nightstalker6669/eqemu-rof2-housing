#ifndef EQEMU_HOUSING_LAYOUT_RETAINED_TOKEN_TYPES_H
#define EQEMU_HOUSING_LAYOUT_RETAINED_TOKEN_TYPES_H
#include "live_item_service.h"
#include "../item_instance.h"
namespace Housing::LayoutRetained {
// Borrowed original under the same exclusive inventory fence. No token clone.
struct Witness {size_t operation_index=0;LiveItems::InventoryRow row;EQ::ItemInstance* original=nullptr;int32_t serial=0;uint64_t house_id=0,crate_id=0;};
inline bool Typed(const EQ::ItemInstance* i){return i&&!i->IsStackable()&&!i->IsScaling()&&bool(i->GetHousingHouseID())!=bool(i->GetHousingCrateID())&&i->GetHousingEstateID()!=UINT32_MAX;}
}
#endif
