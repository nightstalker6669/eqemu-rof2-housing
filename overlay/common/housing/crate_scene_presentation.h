#ifndef EQEMU_HOUSING_CRATE_SCENE_PRESENTATION_H
#define EQEMU_HOUSING_CRATE_SCENE_PRESENTATION_H
#include "scene_service.h"
#include "crate_binding.h"
#include "crate_placement_model.h"
namespace Housing::CratePresentation {
// Presentation-cache projection only, after a fresh reviewed Scene read.
// Never use this to adopt an inventory item or to authorize custody/transfer.
inline const CrateGeometry::Record* Model(const Scene::Item& scene,const EQ::ItemInstance& item){
 if(!scene.crate||scene.house||item.GetHousingHouseID())return nullptr;
 const auto& s=scene.stored;const auto& b=scene.crate->binding;const auto* d=item.GetUnscaledItem();
 if(!d||!s.custody_id||!s.estate_id||!s.current_catalog_revision||!s.placement||!b.crate_id||!b.revision||b.custody_id!=s.custody_id||b.item_template_id!=s.row.values.item_id||b.item_template_id!=d->ID||b.estate_id!=s.estate_id||b.definition_id!=d->HousingDefinitionID||!b.native_estate_id||b.native_estate_id==UINT32_MAX||b.native_estate_id==b.parent_native_estate_id)return nullptr;
 const auto& p=*s.placement;const auto& r=p.reservation;
 if(!p.pose_revision||!r.native_item_id||r.native_item_id==UINT32_MAX||r.native_estate_id!=b.parent_native_estate_id||r.estate_id!=b.estate_id||r.catalog_key!=b.catalog_key||r.catalog_revision!=b.catalog_revision||!r.content_binding||r.content_binding->version!=ContentBinding::CrateVersion)return nullptr;
 return CratePlacement::Model(item);
}
inline bool Project(const Scene::Item& scene,EQ::ItemInstance& item){
 if(!Model(scene,item))return false;
 const auto expected=scene.crate->binding.native_estate_id;
 if(item.GetHousingCrateID()||item.GetHousingEstateID()!=UINT32_MAX)return item.GetHousingCrateID()==scene.crate->binding.crate_id&&item.GetHousingEstateID()==expected;
 return item.SetHousingCrateID(scene.crate->binding.crate_id)&&item.SetHousingEstateID(expected);
}
inline bool Matches(const Scene::Item& scene,const EQ::ItemInstance& item){
 return Model(scene,item)&&item.GetHousingCrateID()==scene.crate->binding.crate_id&&item.GetHousingEstateID()==scene.crate->binding.native_estate_id;
}
// Room reservations use a typed interior tag rather than an exterior binding.
inline bool Project(const Scene::InteriorContent& scene,uint64_t house_id,uint32_t native_room,EQ::ItemInstance& item){
 if(!scene.crate||!scene.room||!scene.stored.placement||scene.stored.stowed||item.GetHousingHouseID()||!CratePlacement::Model(item))return false;
 const auto& a=*scene.crate;const auto& b=a.binding;const auto& s=scene.stored;const auto& r=s.placement->reservation;const auto* d=item.GetUnscaledItem();
 try{CrateBinding::EncodeAllocation(a);}catch(...){return false;}
 if(a.packed||a.inventory_character_id||scene.room->house_id!=house_id||!house_id||!native_room||b.parent_native_estate_id!=native_room||
    b.custody_id!=s.custody_id||b.estate_id!=s.estate_id||b.item_template_id!=s.row.values.item_id||b.item_template_id!=d->ID||b.definition_id!=d->HousingDefinitionID||
    a.created_by_character_id!=s.row.values.character_id||!s.placement->pose_revision||r.content_binding||r.native_estate_id!=native_room||
    r.estate_id!=b.estate_id||r.catalog_key!=b.catalog_key||r.catalog_revision!=b.catalog_revision||!r.native_item_id||r.native_item_id==UINT32_MAX||
    scene.room->placement.native_estate_id!=native_room||scene.room->placement.native_item_id!=r.native_item_id)return false;
 if(item.GetHousingCrateID()||item.GetHousingEstateID()!=UINT32_MAX)return item.GetHousingCrateID()==b.crate_id&&item.GetHousingEstateID()==b.native_estate_id;
 return item.SetHousingCrateID(b.crate_id)&&item.SetHousingEstateID(b.native_estate_id);
}
inline bool SameBinding(const std::shared_ptr<const CrateBinding::Allocation>& a,const std::shared_ptr<const CrateBinding::Allocation>& b)noexcept{
 try{return (!a||!b)?!a&&!b:CrateBinding::EncodeAllocation(*a)==CrateBinding::EncodeAllocation(*b);}catch(...){return false;}
}
}
#endif
