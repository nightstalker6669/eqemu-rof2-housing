#ifndef EQEMU_HOUSING_SELECTED_WALL_CACHE_H
#define EQEMU_HOUSING_SELECTED_WALL_CACHE_H
#include "selected_wall_cache_data.h"
#include "original_mesh_sidecar.h"
#include "original_cached_box.h"
#include "original_actor_local_query.h"
#include "static_furniture_registry.h"
namespace Housing::SelectedWallCache {
inline const OriginalHeightScene::MeshCatalogEntry* Source(uint32_t item){
 const OriginalHeightScene::MeshCatalogEntry* out=nullptr;for(const auto& r:Sources)if(r.item_id==item){if(out)return nullptr;out=&r;}return out;
}
// Original eligible-triangle cache producer, authenticated by compiled ordinary
// resource-class membership. No per-item native capture is inferred/required.
// This is distinct from old registry nearest-rounded placement geometry.
class Prepared final {
 uint32_t item_id_{};std::string member_sha256_;OriginalCachedBox::Box cache_{};std::shared_ptr<const OriginalHeightScene::LocalMesh> mesh_;
 Prepared()=default;
public:
 Prepared(const Prepared&)=delete;Prepared& operator=(const Prepared&)=delete;
 uint32_t ItemId()const{return item_id_;}
 std::string_view MemberSha256()const{return member_sha256_;}
 const OriginalCachedBox::Box& Cache()const{return cache_;}
 const std::shared_ptr<const OriginalHeightScene::LocalMesh>& MeshOwner()const{return mesh_;}
 static std::shared_ptr<const Prepared> Read(uint32_t item,std::span<const uint8_t> bytes){
  const auto* source=Source(item);const auto* model=StaticFurniture::Lookup(item);if(!source||!model||!StaticFurniture::Mounted(item)||model->mounting_type!=1||source->model!=model->model||source->archive_sha256!=model->archive_sha256||source->member_sha256!=model->member_sha256)return {};
  OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};
  try{auto mesh=OriginalHeightScene::ReadMeshSidecar(bytes,*source);if(!mesh||mesh->polygons.empty()||mesh->polygons.size()>8000)return {};
   OriginalCachedBox::Bounds bounds;bool first=true;
   for(auto p=mesh->polygons.rbegin();p!=mesh->polygons.rend();++p){if(p->flags)return {};auto normal=OriginalActorLocalQuery::ModelNormal(p->native_abc);if(!normal)return {};if(*normal==OriginalSphereQuery::Point{})continue;
    for(const auto& v:p->native_abc){if(first){bounds.low=v;bounds.high=v;first=false;continue;}for(unsigned k=0;k<3;++k){if(v[k]<bounds.low[k])bounds.low[k]=v[k];if(v[k]>bounds.high[k])bounds.high[k]=v[k];}}
   }if(first)return {};auto cache=OriginalCachedBox::FromReviewedBounds(bounds);if(!cache)return {};
   auto out=std::shared_ptr<Prepared>(new Prepared);out->item_id_=item;out->member_sha256_=std::string(source->member_sha256);out->cache_=*cache;out->mesh_=std::make_shared<const OriginalHeightScene::LocalMesh>(std::move(*mesh));return out;
  }catch(...){return {};}
 }
};
}
#endif
