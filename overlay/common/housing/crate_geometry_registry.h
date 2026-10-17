#ifndef EQEMU_HOUSING_CRATE_GEOMETRY_REGISTRY_H
#define EQEMU_HOUSING_CRATE_GEOMETRY_REGISTRY_H
#include "placement_policy.h"
#include <string_view>
namespace Housing::CrateGeometry {
inline constexpr uint32_t GeometryPolicyRevision=1;
// Original IT20028 CSimpleModel: exact ordered asset vertices/indices and
// passive cached bounds match. Recognition grants no custody, child estate,
// capacity, definition type, placement, packing or transfer authority.
// Evidence: housing/content/crate-geometry-offline-1015/native-small-crate.json.
struct Record {
 uint32_t model_id;
 const char* model;
 PlacementPolicy::Geometry geometry;
 const char *archive_sha256,*member_sha256,*positions_sha256,*indices_sha256,*witness_sha256;
 uint32_t vertices,triangles;
};
inline constexpr Record Original={
 20028,"IT20028",{20028,{-1.1920928955078125e-7,0,1.875},{3.75,1.875,1.875},true,0},
 "d73ffa6a7aa461066079d5fc7bc171399285f7570b16350adfcb73fc3b05b884",
 "c7a3bb27f59a3e6b546e7ec2531068d3192fdd92cd9e11122633e4e29a100f36",
 "48250371afad9f1f373429b8ec874c027268380ed0f055c2dffecf9ef82e634f",
 "f81740d88beffd9538c14b0f7fa75fbd388d17913527735345a1a183cce7b6b0",
 "dd2459d3810d2e654ffd25efacb3085e9d7ddd0a72e3f38a5171dd82abedeae1",952,340
};
// Exact item/definition/model associations from the retained item snapshot.
// Definition payloads and live limits still require independent review.
inline constexpr uint32_t TemplateDefinition(uint32_t item){
 return item==52147?31:(item>=62586&&item<=62596?174+(item-62586):0);
}
inline const Record* Lookup(uint32_t item,uint32_t definition,std::string_view model){
 if(model!=Original.model)return nullptr;
 if(TemplateDefinition(item)&&definition==TemplateDefinition(item))return &Original;
 return nullptr;
}
}
#endif
