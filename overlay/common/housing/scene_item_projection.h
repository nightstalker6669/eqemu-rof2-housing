#ifndef EQEMU_HOUSING_SCENE_ITEM_PROJECTION_H
#define EQEMU_HOUSING_SCENE_ITEM_PROJECTION_H
#include "inventory_projection.h"
namespace Housing::SceneProjection {
// Passive scene/custody-cache refresh from an independently authorized current
// SQL row. Never use this to authorize inventory debits or replace their exact
// preimage checks. Preserve the serialized scene GUID and all other metadata.
inline bool SynchronizeCharges(const LiveItems::InventoryRow& row,EQ::ItemInstance& item){
 if(InventoryProjection::Matches(row,item))return true;
 const auto* data=item.GetItem();if(!data)return false;
 std::unique_ptr<EQ::ItemInstance> probe(item.Clone());if(!probe)return false;
 const auto charges=InventoryProjection::Charges(row,*data);probe->SetCharges(charges);
 if(!InventoryProjection::Matches(row,*probe))return false;
 item.SetCharges(charges);return true;
}
}
#endif
