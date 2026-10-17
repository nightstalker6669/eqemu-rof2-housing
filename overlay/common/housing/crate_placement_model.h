#ifndef EQEMU_HOUSING_CRATE_PLACEMENT_MODEL_H
#define EQEMU_HOUSING_CRATE_PLACEMENT_MODEL_H
#include "crate_geometry_registry.h"
#include "../item_instance.h"
#include "npc_item_metadata.h"
#include <optional>
namespace Housing::CratePlacement {
// Original static crate model recognition, not a content/identity grant.
inline const CrateGeometry::Record* Model(const EQ::ItemInstance& item){
 const auto* d=item.GetUnscaledItem();
 if(!d||!NpcItemMetadata::Ordinary(*d)||d->ItemClass!=EQ::item::ItemClassCommon||item.IsScaling()||item.IsStackable()||d->EvolvingItem||d->BagSlots||d->HousingPlacementType||!std::isfinite(d->HousingMinimumScale)||!std::isfinite(d->HousingMaximumScale)||!(d->HousingMinimumScale>0)||d->HousingMaximumScale<d->HousingMinimumScale)return nullptr;
 return CrateGeometry::Lookup(d->ID,d->HousingDefinitionID,std::string(d->IDFile));
}
inline std::optional<PlacementPolicy::ItemMetadata> Metadata(const EQ::ItemInstance& item,uint32_t definition,uint32_t type){
 const auto* model=Model(item);const auto* d=item.GetUnscaledItem();
 if(!model||type!=7||d->HousingDefinitionID!=definition||!d->Placeable||(d->Placeable&~uint32_t(0xff)))return {};
 return PlacementPolicy::ItemMetadata{d->ID,model->model_id,d->Placeable,d->HousingMinimumScale,d->HousingMaximumScale,false,7};
}
}
#endif
