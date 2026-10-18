#ifndef EQEMU_HOUSING_REVIEWED_CRATE_RUNTIME_H
#define EQEMU_HOUSING_REVIEWED_CRATE_RUNTIME_H
#include "crate_binding.h"
#include "scene_service.h"
#include "house_binding_service.h"
namespace Housing::ReviewedCrateRuntime {
// The service resolves and locks canonical catalog/ACL facts. The runtime must
// additionally prove the real client, held fence, asset metadata and inventory
// source. Content registration is independent of creator and current plot.
inline bool ExteriorFacts(const CrateBinding::ContentFacts& f,AuthenticatedPurchaser actor,Route physical){
 if(!actor.character_id||!actor.account_id||f.lease.actor.character_id!=actor.character_id||f.lease.actor.account_id!=actor.account_id||physical.zone_id!=712||!physical.instance_id||physical.instance_id!=f.neighborhood_instance||!f.reservation.estate_id||!f.reservation.native_estate_id||f.reservation.native_estate_id==UINT32_MAX||f.reservation.catalog_key.empty()||!f.reservation.catalog_revision||!f.catalog_revision||f.source.values.item_id!=62586||!f.source.values.character_id||f.crate.id!=174||f.crate.type!=7||!f.access.owner_id||f.region.id!=f.plot.area_id)return false;
 if(!((f.plot.type==4&&f.access.owner_kind==OwnerKind::Character)||(f.plot.type==8&&f.access.owner_kind==OwnerKind::Guild)))return false;
 AccessPolicy::Capability capability;
 switch(f.stage){
 case CrateBinding::Stage::Read:capability=AccessPolicy::Capability::Enter;break;
 case CrateBinding::Stage::Move:capability=AccessPolicy::Capability::MoveItem;break;
 case CrateBinding::Stage::Preview:case CrateBinding::Stage::Store:
  if(f.source.values.character_id!=actor.character_id||f.reservation.catalog_revision!=f.catalog_revision)return false;
  capability=AccessPolicy::Capability::AddCrate;break;
 default:return false;
 }
 return f.access.effective.Allows(capability);
}
// Called only after the scene service independently locks and authenticates the
// occupied house and its parent. A matching view grants no exterior mutation.
inline bool ParentReadFacts(const CrateBinding::ContentFacts& f,const Scene::InteriorSnapshot& room,AuthenticatedPurchaser actor,Route physical){
 const auto same=[](Route a,Route b){return a.zone_id==b.zone_id&&a.zone_version==b.zone_version&&a.instance_id==b.instance_id;};
 if(f.stage!=CrateBinding::Stage::Read||!room.house||!room.house->interior||room.exterior_source||
    !same(physical,room.context.source)||!physical.zone_id||physical.zone_id==712||!physical.instance_id||
    !room.house->house_id||!room.house->revision||!room.house->pending_key.empty()||
    room.house->location.kind!=HouseBinding::Location::Kind::Custody||!room.house->location.custody_id||
    !room.access.effective.Allows(AccessPolicy::Capability::Enter)||
    room.access.owner_kind!=f.access.owner_kind||room.access.owner_id!=f.access.owner_id||
    room.access.plot_id!=f.access.plot_id||room.access.acl_revision!=f.access.acl_revision||
    room.context.estate_id!=f.reservation.estate_id||room.context.native_plot_id!=f.reservation.native_estate_id||
    room.context.catalog_revision!=f.catalog_revision||
    room.house->interior->catalog_key!=f.reservation.catalog_key||
    room.house->interior->catalog_revision!=f.catalog_revision||
    !same(physical,{room.house->interior->zone_id,room.house->interior->zone_version,room.house->interior->instance_id})||
    room.context.neighborhood.zone_id!=712||room.context.neighborhood.instance_id!=f.neighborhood_instance)return false;
 return ExteriorFacts(f,actor,room.context.neighborhood);
}

}
#endif
