#ifndef EQEMU_HOUSING_UPKEEP_SELECTION_POLICY_H
#define EQEMU_HOUSING_UPKEEP_SELECTION_POLICY_H
#include "estate_wire.h"
namespace Housing::UpkeepSelection {
// A component belongs to its definition's upkeep scheme, never its purchase
// scheme or a client-supplied scheme. No funding, duration or price inference.
inline bool Offered(const EstateWire::Definition& definition,
 const std::vector<EstateWire::CostScheme>& costs,uint32_t component){
 if((definition.type!=4&&definition.type!=8)||!definition.upkeep_scheme||
    !component||component==UINT32_MAX)return false;
 const EstateWire::CostScheme* scheme=nullptr;
 for(const auto& s:costs)if(s.id==definition.upkeep_scheme){if(scheme)return false;scheme=&s;}
 if(!scheme)return false;
 size_t found=0;for(const auto& c:scheme->components)if(c.id==component)++found;
 return found==1;
}
}
#endif
