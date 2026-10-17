#ifndef EQEMU_HOUSING_HOUSE_PLACEMENT_MODEL_H
#define EQEMU_HOUSING_HOUSE_PLACEMENT_MODEL_H
#include "house_geometry_registry.h"
#include "../item_instance.h"
#include "npc_item_metadata.h"
#include <optional>
namespace Housing::HousePlacement {
// Recognition only. Canonical definition/type must come from the current
// server catalog; this supplies no ownership, adoption or interior authority.
inline const HouseGeometry::Record* Model(const EQ::ItemInstance& item){
 const auto* d=item.GetUnscaledItem();
 if(!d||!NpcItemMetadata::Ordinary(*d)||d->ItemClass!=EQ::item::ItemClassCommon||item.IsScaling()||item.IsStackable()||d->EvolvingItem||d->BagSlots||!d->HousingDefinitionID||d->HousingPlacementType||!std::isfinite(d->HousingMinimumScale)||!std::isfinite(d->HousingMaximumScale)||!(d->HousingMinimumScale>0)||d->HousingMaximumScale<d->HousingMinimumScale)return nullptr;
 return HouseGeometry::Lookup(d->ID,std::string(d->IDFile));
}
inline std::optional<PlacementPolicy::ItemMetadata> Metadata(const EQ::ItemInstance& item,uint32_t definition_id,uint32_t definition_type){
 const auto* model=Model(item);const auto* d=item.GetUnscaledItem();
 if(!model||d->HousingDefinitionID!=definition_id||(definition_type!=2&&definition_type!=3)||!d->Placeable||(d->Placeable&~uint32_t(0xff)))return {};
 return PlacementPolicy::ItemMetadata{d->ID,model->model_id,d->Placeable,d->HousingMinimumScale,d->HousingMaximumScale,false,definition_type};
}
inline float DefaultScale(const EQ::ItemInstance& item){
 if(!Model(item))throw std::invalid_argument("Unsupported house placement model");
 const auto* d=item.GetUnscaledItem();return std::clamp(1.f,d->HousingMinimumScale,d->HousingMaximumScale);
}
// Server interaction proximity to the verified oriented bounds, not to a huge
// house's origin. This is a bounded server policy, not recovered Live raycast
// behavior, exact mesh collision, ownership or permission proof.
inline std::optional<double> DistanceToGeometryBounds(const PlacementPolicy::Geometry& geometry,const PlacementPolicy::Transform& pose,const PlacementPolicy::Point& point){
 for(float v:{pose.heading,pose.pitch,pose.roll,pose.scale,pose.position[0],pose.position[1],pose.position[2]})if(!std::isfinite(v))return {};
 for(double v:point)if(!std::isfinite(v))return {};
 if(!(pose.scale>0))return {};
 for(float v:{pose.heading,pose.pitch,pose.roll})if(v<0||v>512)return {};
 const auto rotation=PlacementPolicy::Rotation(pose.heading,pose.pitch,pose.roll);
 double squared=0;
 for(size_t axis=0;axis<3;++axis){
  double local=0;for(size_t world=0;world<3;++world)local+=(point[world]-pose.position[world])*rotation[axis*3+world];
  const double outside=std::max(0.,std::abs(local-geometry.center[axis]*pose.scale)-geometry.extents[axis]*pose.scale);
  squared+=outside*outside;
 }
 return std::sqrt(squared);
}
inline std::optional<double> DistanceToBounds(const EQ::ItemInstance& item,const PlacementPolicy::Transform& pose,const PlacementPolicy::Point& point){
 const auto* model=Model(item);if(!model)return {};const auto* d=item.GetUnscaledItem();
 if(pose.scale<d->HousingMinimumScale||pose.scale>d->HousingMaximumScale)return {};
 return DistanceToGeometryBounds(model->geometry,pose,point);
}
}
#endif
