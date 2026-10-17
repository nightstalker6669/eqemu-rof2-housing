#ifndef EQEMU_HOUSING_INTERIOR_NATIVE_VIEW_H
#define EQEMU_HOUSING_INTERIOR_NATIVE_VIEW_H
#include "interior_content_binding.h"
#include "interior_wall_slab_registry.h"
#include "scene_service.h"
#include "crate_binding.h"
namespace Housing::InteriorNativeView {
// Display projection only after genuine current room/house/Enter checks. Never
// rewrites persistent catalogs, house v2 identity, membership or mutation rights.
inline bool Apply(Catalog::Export& view,const InteriorContent::Profile& base_room,uint32_t current_native,const std::vector<Scene::InteriorContent>* contents=nullptr){
 auto expanded=InteriorWallSlab::Bootstrap(base_room);if(!expanded)return false;const auto& room=*expanded;
 if(!room.reviewed_room||!room.house_id||!current_native||current_native!=room.native_estate_id||!ContentBinding::Detail::RouteShape(room.route)||!OriginalInteriors::Matches(room.route.zone_id,room.zone_short_name)||room.definition.zone_id!=room.route.zone_id||(room.definition.type!=2&&room.definition.type!=3)||!room.definition.max_items||room.definition.max_non_estate_items>room.definition.max_items||!room.definition.area_id||room.definition.area_id!=room.region.id)return false;
 try{
  auto next=view;auto node=std::find_if(next.catalog.nodes.begin(),next.catalog.nodes.end(),[&](const auto& n){return n.native_id.value==current_native;});if(node==next.catalog.nodes.end()||node->definition_id!=room.definition.id)return false;
  const auto parent=*node;
  auto definition=std::find_if(next.catalog.definitions.begin(),next.catalog.definitions.end(),[&](const auto& d){return d.id==room.definition.id;});if(definition==next.catalog.definitions.end()||definition->type!=room.definition.type||definition->zone_id!=room.definition.zone_id)return false;
  // A room-specific region cannot silently replace another estate's geometry.
  for(const auto& owned:InteriorContent::Regions(room))for(const auto& d:next.catalog.definitions)if(d.id!=room.definition.id&&d.area_id==owned.id)return false;
  InteriorContent::RegionPayload(room);*definition=room.definition;
  if(!InteriorContent::ValidChildDefinitions(room))return false;
  for(const auto& child:room.child_definitions){
   auto existing=std::find_if(next.catalog.definitions.begin(),next.catalog.definitions.end(),[&](const auto& value){return value.id==child.id;});
   if(existing==next.catalog.definitions.end())next.catalog.definitions.push_back(child);
   else if(EstateWire::EncodeBootstrap({*existing},{})!=EstateWire::EncodeBootstrap({child},{}))return false;
  }
  for(const auto& owned:InteriorContent::Regions(room)){
   auto region=std::find_if(next.catalog.regions.begin(),next.catalog.regions.end(),[&](const auto& r){return r.id==owned.id;});
   if(region==next.catalog.regions.end())next.catalog.regions.push_back(owned);
   else if(RegionWire::EncodeBootstrap({*region})!=RegionWire::EncodeBootstrap({owned}))return false;
  }
  if(contents)for(const auto& entry:*contents){
   if(!entry.crate){if(CrateGeometry::TemplateDefinition(entry.stored.row.values.item_id))return false;continue;}
   const auto& allocation=*entry.crate;const auto& binding=allocation.binding;const auto& stored=entry.stored;
   CrateBinding::EncodeAllocation(allocation);
   if(!entry.room||entry.room->house_id!=room.house_id||allocation.packed||allocation.inventory_character_id||stored.stowed||!stored.placement||
      binding.custody_id!=stored.custody_id||binding.item_template_id!=stored.row.values.item_id||binding.estate_id!=stored.estate_id||
      allocation.created_by_character_id!=stored.row.values.character_id||binding.parent_native_estate_id!=current_native||
      stored.placement->reservation.native_estate_id!=current_native||stored.placement->reservation.content_binding||
      entry.room->placement.native_item_id!=stored.placement->reservation.native_item_id||entry.room->placement.native_estate_id!=current_native||
      std::find(room.definition.children.begin(),room.definition.children.end(),binding.definition_id)==room.definition.children.end())return false;
   // Display current ancestry; keep stored custody/catalog/neighborhood provenance.
   Catalog::Node child{"crate:"+std::to_string(binding.crate_id),parent.key,{binding.native_estate_id},parent.neighborhood_id,0,0,binding.definition_id,0};
   auto existing=std::find_if(next.catalog.nodes.begin(),next.catalog.nodes.end(),[&](const auto& value){return value.native_id.value==child.native_id.value||value.key==child.key;});
   if(existing==next.catalog.nodes.end())next.catalog.nodes.push_back(child);
   else if(existing->native_id.value!=child.native_id.value||existing->key!=child.key||existing->parent_key!=child.parent_key||existing->neighborhood_id!=child.neighborhood_id||existing->definition_id!=child.definition_id||existing->plot_id||existing->ownership_estate_id)return false;
  }
  if(std::count_if(next.catalog.nodes.begin(),next.catalog.nodes.end(),[&](const auto& child){return child.parent_key==parent.key;})>room.definition.max_children)return false;
  next.definition_payload=EstateWire::EncodeBootstrap(next.catalog.definitions,next.catalog.costs);next.region_payload=RegionWire::EncodeBootstrap(next.catalog.regions);
  auto decoded=Catalog::DecodePayloads(next.definition_payload,next.region_payload);if(decoded.definitions.size()!=next.catalog.definitions.size()||decoded.regions.size()!=next.catalog.regions.size())return false;view=std::move(next);return true;
 }catch(...){return false;}
}
}
#endif
