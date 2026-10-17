#ifndef EQEMU_HOUSING_HOUSE_PLACEMENT_PREVIEW_H
#define EQEMU_HOUSING_HOUSE_PLACEMENT_PREVIEW_H
#include "house_placement_model.h"
#include "live_item_service.h"
#include "house_binding_service.h"
#include "placement_wire.h"
namespace Housing::HousePlacement {
// For a checked cursor item AFTER the real backend reservation. Replacement
// additionally requires its fresh server inventory-binding snapshot.
// A reservation is not an allocated HouseID or permission to publish a placed
// house. The normal world scene still requires its checked durable binding.
inline std::optional<PlacementWire::Actor> Preview(const EQ::ItemInstance& item,
 const LiveItems::Reservation& reservation,uint32_t definition_id,uint32_t definition_type,
 const PlacementPolicy::Transform& pose,const HouseBinding::Binding* retained=nullptr){
 const auto metadata=Metadata(item,definition_id,definition_type);const auto* model=Model(item);
 if(!metadata||!model||item.GetSerialNumber()<=0||!reservation.native_item_id||reservation.native_item_id==UINT32_MAX||!reservation.native_estate_id||reservation.native_estate_id==UINT32_MAX||!reservation.estate_id||!reservation.content_binding||reservation.content_binding->version!=ContentBinding::HouseVersion)return {};
 if(retained){
  if(!retained->house_id||!retained->revision||!retained->pending_key.empty()||retained->item_template_id!=item.GetID()||retained->location.kind!=HouseBinding::Location::Kind::Inventory||!retained->location.character_id||!((retained->location.slot_id>=23&&retained->location.slot_id<=32)||retained->location.slot_id==33)||retained->location.custody_id||item.GetHousingHouseID()!=retained->house_id||item.GetHousingEstateID()!=(retained->interior?retained->interior->native_estate_id:UINT32_MAX))return {};
  if(retained->interior&&retained->interior->definition_id!=definition_id)return {};
 }else if(item.GetHousingHouseID()||item.GetHousingEstateID()!=UINT32_MAX)return {};
 for(float value:{pose.heading,pose.pitch,pose.roll,pose.scale,pose.position[0],pose.position[1],pose.position[2]})if(!std::isfinite(value))return {};
 if(pose.scale<metadata->minimum_scale||pose.scale>metadata->maximum_scale)return {};
 PlacementWire::Actor actor;actor.model=model->model;
 actor.guid=std::to_string(item.GetSerialNumber());actor.guid.insert(0,16-actor.guid.size(),'0');
 actor.estate_id=reservation.native_estate_id;actor.item_id=reservation.native_item_id;
 actor.scale_heading_pitch_roll_y_x_z={pose.scale,pose.heading,pose.pitch,pose.roll,pose.position[0],pose.position[1],pose.position[2]};
 const auto* d=item.GetUnscaledItem();actor.ignore_collisions=d->HousingIgnoreCollisions!=0;actor.placement_type=d->HousingPlacementType;
 actor.min_max_default_scale_heading_pitch_roll={metadata->minimum_scale,metadata->maximum_scale,DefaultScale(item),0,0,0};
 return actor;
}
}
#endif
