#ifndef EQEMU_ZONE_HOUSING_PLOT_MARKER_H
#define EQEMU_ZONE_HOUSING_PLOT_MARKER_H
#include "../common/housing/catalog_service.h"
#include <algorithm>
class Client;
namespace Housing::PlotMarker {
// UI inspection range only; this never authorizes ownership or placement.
inline bool GeometryInRange(const RegionWire::Region& region,std::array<float,3> server_xyz,float maximum){
 if(!std::isfinite(maximum)||maximum<0)return false;
 for(float value:server_xyz)if(!std::isfinite(value))return false;
 try{RegionWire::Detail::ValidateMatrix(region.matrix);}catch(...){return false;}
 const auto& m=region.matrix;std::array<double,3> delta{double(server_xyz[1])-m[12],double(server_xyz[0])-m[13],double(server_xyz[2])-m[14]};
 std::array<double,3> lengths{};double distance_squared=0;
 for(size_t i=0;i<3;++i){for(size_t k=0;k<3;++k)lengths[i]+=double(m[i*4+k])*m[i*4+k];if(!(lengths[i]>0)||!std::isfinite(lengths[i]))return false;}
 // These are reviewed OBBs, not arbitrary sheared affine volumes. Refuse a
 // nonorthogonal basis rather than underestimating its distance to a boundary.
 for(size_t i=0;i<3;++i)for(size_t j=i+1;j<3;++j){double dot=0;for(size_t k=0;k<3;++k)dot+=double(m[i*4+k])*m[j*4+k];if(std::abs(dot)>1e-5*std::sqrt(lengths[i]*lengths[j]))return false;}
 for(size_t i=0;i<3;++i){double projection=0;for(size_t k=0;k<3;++k)projection+=delta[k]*m[i*4+k];const double excess=std::max(0.0,std::abs(projection)/std::sqrt(lengths[i])-std::sqrt(lengths[i]));distance_squared+=excess*excess;}
 return std::isfinite(distance_squared)&&distance_squared<=double(maximum)*maximum;
}
inline const Catalog::Node* Resolve(const Catalog::Bundle& catalog,uint32_t neighborhood,uint32_t root_native,uint32_t static_id){
 if(!neighborhood||!root_native||!static_id)return nullptr;
 const Catalog::Node* root=nullptr;const Catalog::Node* plot=nullptr;
 for(const auto& node:catalog.nodes){if(node.neighborhood_id!=neighborhood)continue;if(node.native_id.value==root_native){if(root||!node.parent_key.empty())return nullptr;root=&node;}if(node.static_id==static_id){if(plot)return nullptr;plot=&node;}}
 if(!root||!plot||!plot->plot_id||plot->parent_key!=root->key)return nullptr;
 const EstateWire::Definition* definition=nullptr;bool root_valid=false;
 for(const auto& d:catalog.definitions){if(d.id==root->definition_id)root_valid=d.type==5&&d.zone_id==712;if(d.id==plot->definition_id){if(definition)return nullptr;definition=&d;}}
 if(!root_valid||!definition||(definition->type!=4&&definition->type!=8)||definition->zone_id!=712||!definition->area_id)return nullptr;
 const RegionWire::Region* region=nullptr;for(const auto& r:catalog.regions)if(r.id==definition->area_id){if(region)return nullptr;region=&r;}
 return region?plot:nullptr;
}
inline bool CatalogInRange(const Catalog::Bundle& catalog,uint32_t neighborhood,uint32_t root_native,uint32_t static_id,std::array<float,3> server_xyz,float maximum){
 const auto* plot=Resolve(catalog,neighborhood,root_native,static_id);if(!plot)return false;
 const auto definition=std::find_if(catalog.definitions.begin(),catalog.definitions.end(),[&](const auto& d){return d.id==plot->definition_id;});
 const auto region=std::find_if(catalog.regions.begin(),catalog.regions.end(),[&](const auto& r){return r.id==definition->area_id;});
 return GeometryInRange(*region,server_xyz,maximum);
}
std::optional<uint32_t> InspectionEstate(Client*,uint32_t static_id,float maximum,bool within_distance);
bool InRange(Client*,uint32_t static_id,float maximum);
}
#endif
