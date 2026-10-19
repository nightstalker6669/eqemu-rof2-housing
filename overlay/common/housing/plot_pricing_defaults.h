#ifndef EQEMU_HOUSING_PLOT_PRICING_DEFAULTS_H
#define EQEMU_HOUSING_PLOT_PRICING_DEFAULTS_H
#include "estate_wire.h"
#include <optional>
#include <string_view>
namespace Housing::PlotPricingDefaults {
// Reconstructed copper-only defaults, not an authenticated May2013 tariff dump.
// Personal address tiers: community housing table, 2022/2023:
// https://forum.gamer.com.tw/C.php?bsn=5179&snA=2008
// Guild purchase and approximate daily upkeep: firsthand contemporary guide:
// https://www.redguides.com/community/threads/so-you-want-to-buy-a-guild-hall.89350/
// No house/item rent, alternate-currency equivalence or geometric limits implied.
struct Tariff {uint64_t purchase_copper=0,daily_copper=0;};
inline std::optional<Tariff> Lookup(uint32_t type,std::string_view address){
 const auto space=address.find(' ');if(space!=3)return {};
 uint32_t number=0;for(char c:address.substr(0,space)){if(c<'0'||c>'9')return {};number=number*10+uint32_t(c-'0');}
 const auto street=address.substr(space+1);
 if(type==8){if(number!=200||(street!="Brimming Way"&&street!="Vanward Heights"&&street!="Terminus Heights"&&street!="Guild Way"))return {};return Tariff{21000,2000};}
 if(type!=4)return {};uint64_t platinum=0;
 if(street=="Market Street"&&number>=101&&number<=104)platinum=number<=102?42:21;
 else if(street=="Market Heights"&&number>=101&&number<=103)platinum=168;
 else if(street=="Betwixt Avenue"&&number>=101&&number<=107)platinum=84;
 else if(street=="Brimming Heights"&&number>=101&&number<=108)platinum=126;
 else if(street=="Brimming Way"&&number>=101&&number<=113)platinum=number<=102?168:84;
 else if(street=="Guild Way"&&number>=101&&number<=105)platinum=84;
 else if(street=="Park Circle"&&number>=101&&number<=108)platinum=126;
 else if(street=="Vanward Street"&&number>=101&&number<=116&&number!=115)platinum=84;
 else if(street=="Vanward Heights"&&number==101)platinum=168;
 else if(street=="Terminus Street"&&number>=101&&number<=107&&number!=106)platinum=126;
 else if(street=="Terminus Heights"&&number==106)platinum=168;
 if(!platinum)return {};return Tariff{platinum*1000,platinum*100};
}
// Stable emulator fallback for custom/unlabelled plots. Callers can still use
// Lookup alone when they require a documented street tier. No plot type is free
// merely because its address is missing from the reconstructed table.
inline std::optional<Tariff> LookupWithFallback(uint32_t type,std::string_view address){
 if(auto known=Lookup(type,address))return known;
 if(type==4)return Tariff{84000,8400};
 if(type==8)return Tariff{21000,2000};
 return {};
}
// Explicit administrative operation on an unpublished definition. Separate IDs
// avoid conflating purchase payment and the selected daily upkeep component.
// Never overwrite an existing price. Unknown personal/guild addresses use the
// explicit emulator fallback above; these values are not a historical claim.
inline void Apply(EstateWire::Definition& definition,std::vector<EstateWire::CostScheme>& costs,
 uint32_t purchase_scheme,uint32_t purchase_component,uint32_t upkeep_scheme,uint32_t upkeep_component){
 const auto tariff=LookupWithFallback(definition.type,definition.address);
 if(!tariff||definition.purchase_scheme||definition.upkeep_scheme)throw std::invalid_argument("Default tariff requires an unpriced personal or guild plot");
 for(auto id:{purchase_scheme,purchase_component,upkeep_scheme,upkeep_component})EstateWire::Detail::Identity(id);
 if(purchase_scheme==upkeep_scheme||purchase_component==upkeep_component)throw std::invalid_argument("Distinct tariff identities required");
 for(const auto& scheme:costs){if(scheme.id==purchase_scheme||scheme.id==upkeep_scheme)throw std::invalid_argument("Tariff scheme collision");for(const auto& component:scheme.components)if(component.id==purchase_component||component.id==upkeep_component)throw std::invalid_argument("Tariff component collision");}
 auto next=costs;next.push_back({purchase_scheme,{{purchase_component,{{0,0,int64_t(tariff->purchase_copper)}}}}});next.push_back({upkeep_scheme,{{upkeep_component,{{0,0,int64_t(tariff->daily_copper)}}}}});
 costs.swap(next);definition.purchase_scheme=purchase_scheme;definition.upkeep_scheme=upkeep_scheme;
}
}
#endif
