#ifndef EQEMU_HOUSING_CRATE_STORAGE_DESTINATION_H
#define EQEMU_HOUSING_CRATE_STORAGE_DESTINATION_H
#include "placement_policy.h"
#include <algorithm>
#include <vector>
namespace Housing::CrateStorageDestination {
// Unpack admits storage by current estate type, not a previous plot's world
// pose. Placement still requires the separate authoritative geometry service.
inline bool Allowed(const PlacementPolicy::ItemMetadata& item,uint32_t estate_type,uint32_t house_definition,const std::vector<uint32_t>& children){
 if((estate_type!=2&&estate_type!=3&&estate_type!=4&&estate_type!=8)||!item.item_id||!item.model_id||!item.placement_mask||(item.placement_mask&~uint32_t(0xff))||!(item.placement_mask&PlacementPolicy::EstateFlag(estate_type)))return false;
 if(item.house_definition_type==2||item.house_definition_type==3){
  return !item.furniture_only&&house_definition&&((item.house_definition_type==2&&estate_type==8)||(item.house_definition_type==3&&estate_type==4))&&std::find(children.begin(),children.end(),house_definition)!=children.end();
 }
 return !house_definition&&!item.house_definition_type;
}
}
#endif
