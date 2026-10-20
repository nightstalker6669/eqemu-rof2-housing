#ifndef EQEMU_HOUSING_ORIGINAL_REVIEWED_CACHE_H
#define EQEMU_HOUSING_ORIGINAL_REVIEWED_CACHE_H
#include "original_cached_box.h"
#include "original_actor_local_query.h"
#include "original_mesh_sidecar.h"
namespace Housing::OriginalReviewedCache {
using Point=OriginalSphereQuery::Point;
struct Record {uint32_t item_id;std::array<uint32_t,6> native_cache;};
// Reviewed plain/static, raw-flags-zero and no ActorTag box override profiles.
// These exact mesh digests are bound by OriginalMeshCatalog; cache DWORDs were
// independently read from the matching original client model definitions.
// Deliberately not a promotion of arbitrary raw EQGM meshes or registry bounds.
inline constexpr std::array<Record,2> Records{{
 {54573, {0x00000000u,0x3bc262c0u,0x3fffe331u,0x3f519954u,0x3f456469u,0x3fffe331u}},
 {66803, {0x00000000u,0x35000000u,0xc0e77e31u,0x405dd9ceu,0x3ee91486u,0x40e77cf0u}},
}};
struct Result {
 uint32_t item_id{};std::string mesh_sha256;
 std::shared_ptr<const OriginalHeightScene::LocalMesh> mesh;
 OriginalCachedBox::Bounds eligible_bounds;
 OriginalCachedBox::Box cache;
};
inline std::optional<Result> Read(uint32_t item_id,std::span<const uint8_t> bytes){
 const Record* record=nullptr;for(const auto& r:Records)if(r.item_id==item_id){if(record)return {};record=&r;}
 const OriginalHeightScene::MeshCatalogEntry* catalog=nullptr;
 for(const auto& r:OriginalHeightScene::OriginalMeshCatalog)if(r.item_id==item_id){if(catalog)return {};catalog=&r;}
 if(!record||!catalog)return {};
 OriginalSphereQuery::Detail::Rounding environment;if(!environment.good)return {};
 try{
  auto source=OriginalHeightScene::ReadMeshSidecar(bytes,*catalog);
  if(!source||source->polygons.empty()||source->polygons.size()>8000)return {};
  OriginalCachedBox::Bounds bounds;bool first=true;
  // Original definition iteration is descending, with its native ABC order.
  // This enrolled profile requires every polygon to have a nonzero original
  // cached normal; degenerate/filtered/special models need another review.
  for(auto p=source->polygons.rbegin();p!=source->polygons.rend();++p){
   if(p->flags)return {};
   auto normal=OriginalActorLocalQuery::ModelNormal(p->native_abc);
   if(!normal||*normal==Point{})return {};
   for(const auto& vertex:p->native_abc){
    if(first){bounds.low=vertex;bounds.high=vertex;first=false;continue;}
    for(unsigned k=0;k<3;++k){if(vertex[k]<bounds.low[k])bounds.low[k]=vertex[k];if(vertex[k]>bounds.high[k])bounds.high[k]=vertex[k];}
   }
  }
  auto cache=OriginalCachedBox::FromReviewedBounds(bounds);if(!cache)return {};
  for(unsigned k=0;k<3;++k)if(std::bit_cast<uint32_t>(cache->center[k])!=record->native_cache[k]||std::bit_cast<uint32_t>(cache->extent[k])!=record->native_cache[k+3])return {};
  return Result{item_id,std::string(catalog->mesh_sha256),std::make_shared<const OriginalHeightScene::LocalMesh>(std::move(*source)),bounds,*cache};
 }catch(...){return {};}
}
}
#endif
