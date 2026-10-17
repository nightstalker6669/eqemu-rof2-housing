#ifndef EQEMU_HOUSING_REVIEWED_CRATE_RUNTIME_H
#define EQEMU_HOUSING_REVIEWED_CRATE_RUNTIME_H
#include "crate_binding.h"
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
}
#endif
