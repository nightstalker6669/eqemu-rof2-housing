#ifndef EQEMU_HOUSING_HOUSE_SCENE_PRESENTATION_H
#define EQEMU_HOUSING_HOUSE_SCENE_PRESENTATION_H
#include "scene_service.h"
#include "crate_scene_presentation.h"
#include "house_binding_service.h"
#include "house_geometry_registry.h"
#include "house_placement_model.h"
#include "../item_instance.h"
namespace Housing::HousePresentation {
// Shape/property projection ONLY, immediately after a fresh reviewed Scene read.
// Use only on server-owned reconstructed/cloned presentation-cache items, never
// m_inv, packet items or as adoption/application/entry authority. The caller must
// additionally verify the real raw-row projection and cached template metadata.
inline const HouseGeometry::Record* Model(const LiveItems::Stored& stored,
 const HouseBinding::Binding& house,const EQ::ItemInstance& item){
 const auto* d=item.GetUnscaledItem();
 if(item.GetHousingCrateID()||!d||!stored.custody_id||!stored.estate_id||!stored.placement||!stored.current_catalog_revision||!house.house_id||!house.revision||!house.pending_key.empty()||!house.interior||house.item_template_id!=stored.row.values.item_id||house.item_template_id!=d->ID||house.location.kind!=HouseBinding::Location::Kind::Custody||house.location.custody_id!=stored.custody_id||house.location.character_id||house.location.slot_id)return nullptr;
 const auto& interior=*house.interior;const auto& placement=*stored.placement;
 if(!placement.reservation.native_item_id||placement.reservation.native_item_id==UINT32_MAX||!placement.reservation.native_estate_id||placement.reservation.native_estate_id==UINT32_MAX||placement.reservation.estate_id!=stored.estate_id||!placement.pose_revision||!placement.reservation.content_binding||placement.reservation.content_binding->version!=ContentBinding::HouseVersion||!interior.native_estate_id||interior.native_estate_id==UINT32_MAX||!interior.instance_id||interior.instance_id>32000||interior.definition_id!=d->HousingDefinitionID||interior.catalog_key!=placement.reservation.catalog_key||interior.catalog_revision!=stored.current_catalog_revision)return nullptr;
 return HousePlacement::Model(item);
}
inline bool Project(const Scene::Item& scene,EQ::ItemInstance& presentation){
 if(scene.crate)return CratePresentation::Project(scene,presentation);
 if(CrateGeometry::TemplateDefinition(scene.stored.row.values.item_id))return false;
 if(!scene.house)return !presentation.GetHousingHouseID()&&!presentation.GetHousingCrateID()&&presentation.GetHousingEstateID()==UINT32_MAX;
 const auto& house=*scene.house;if(!Model(scene.stored,house,presentation))return false;
 const auto estate=house.interior->native_estate_id;
 if(presentation.GetHousingHouseID())return presentation.GetHousingHouseID()==house.house_id&&presentation.GetHousingEstateID()==estate;
 if(presentation.GetHousingEstateID()!=UINT32_MAX)return false;
 // Model checks both setter preconditions before either assignment.
 return presentation.SetHousingHouseID(house.house_id)&&presentation.SetHousingEstateID(estate);
}
// Mutation ingress must already have a correctly projected presentation from
// the fresh scene. Never repair an identity while authorizing Move or Pickup.
inline bool Matches(const Scene::Item& scene,const EQ::ItemInstance& presentation){
 if(scene.crate)return CratePresentation::Matches(scene,presentation);
 if(CrateGeometry::TemplateDefinition(scene.stored.row.values.item_id))return false;
 if(!scene.house)return !presentation.GetHousingHouseID()&&!presentation.GetHousingCrateID()&&presentation.GetHousingEstateID()==UINT32_MAX;
 return Model(scene.stored,*scene.house,presentation)&&presentation.GetHousingHouseID()==scene.house->house_id&&presentation.GetHousingEstateID()==scene.house->interior->native_estate_id;
}
inline bool SameBinding(const std::shared_ptr<const HouseBinding::Binding>& a,const std::shared_ptr<const HouseBinding::Binding>& b){
 if(!a||!b)return !a&&!b;
 if(a->house_id!=b->house_id||a->revision!=b->revision||a->item_template_id!=b->item_template_id||a->pending_key!=b->pending_key||a->location.kind!=b->location.kind||a->location.character_id!=b->location.character_id||a->location.slot_id!=b->location.slot_id||a->location.custody_id!=b->location.custody_id||bool(a->interior)!=bool(b->interior))return false;
 if(!a->interior)return true;
 const auto& x=*a->interior;const auto& y=*b->interior;
 return x.native_estate_id==y.native_estate_id&&x.definition_id==y.definition_id&&x.zone_id==y.zone_id&&x.instance_id==y.instance_id&&x.zone_version==y.zone_version&&x.catalog_key==y.catalog_key&&x.catalog_revision==y.catalog_revision;
}
}
#endif
