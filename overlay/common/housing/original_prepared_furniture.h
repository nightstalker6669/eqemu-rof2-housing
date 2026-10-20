#ifndef EQEMU_HOUSING_ORIGINAL_PREPARED_FURNITURE_H
#define EQEMU_HOUSING_ORIGINAL_PREPARED_FURNITURE_H
// Include the compiled reviewed catalog before any transitive sidecar include.
#include "original_mesh_catalog.h"
#include "original_simple_tree_generator.h"
#include "original_simple_linear_query.h"
#include "original_height_scene_query.h"
#include "original_reviewed_cache.h"
#include "static_furniture_registry.h"
#include "crate_geometry_registry.h"
namespace Housing::OriginalPreparedFurniture {
using Mesh=OriginalHeightScene::LocalMesh;
using Matrix=OriginalActorLocalQuery::Matrix;
using Query=OriginalHeightScene::Query;
using ActorSource=OriginalHeightSceneQuery::ActorSource;
enum class Profile {GeneratedPlainTreeV1,OriginalPlainLinearV1};
// Immutable output of trusted preparation and THIS exact query calculation.
// It does not authenticate current actor/lease/pose/scene completeness. A zone
// resolver must bind those independently before constructing ActorSource.
struct QueryReceipt {
 uint32_t item_id{};Profile profile{};Matrix matrix{};float scale{};
 std::shared_ptr<const Mesh> mesh;
 std::shared_ptr<const OriginalSimpleTreeQuery::Tree> tree;
 std::shared_ptr<const OriginalSimpleLinearQuery::Source> linear;
 std::optional<ActorSource::Enumeration> tree_query;
 std::optional<ActorSource::LinearEnumeration> linear_query;
};
class Prepared final {
 uint32_t item_id_{};Profile profile_{};
 std::string mesh_sha256_,member_sha256_;
 std::shared_ptr<const Mesh> mesh_;
 std::shared_ptr<const OriginalSimpleTreeQuery::Tree> tree_;
 std::shared_ptr<const OriginalSimpleLinearQuery::Source> linear_;
 std::optional<OriginalCachedBox::Box> selected_cache_;
 Prepared()=default;
 static bool ReviewedClass(const OriginalHeightScene::MeshCatalogEntry& row){
  if(row.item_id==62586){const auto* c=CrateGeometry::Lookup(row.item_id,174,row.model);return c&&c->geometry.native_collision_verified&&row.archive_sha256==c->archive_sha256&&row.member_sha256==c->member_sha256;}
  if(row.item_id!=54573&&row.item_id!=52295&&row.item_id!=66803)return false;
  const auto* f=StaticFurniture::Lookup(row.item_id,std::string(row.model));return f&&row.archive_sha256==f->archive_sha256&&row.member_sha256==f->member_sha256;
 }
public:
 Prepared(const Prepared&)=delete;Prepared& operator=(const Prepared&)=delete;
 uint32_t ItemId()const{return item_id_;}
 Profile SourceProfile()const{return profile_;}
 std::string_view MeshSha256()const{return mesh_sha256_;}
 std::string_view MemberSha256()const{return member_sha256_;}
 const std::optional<OriginalCachedBox::Box>& SelectedCache()const{return selected_cache_;}
 // No caller-supplied reviewed/class/flags/model hashes. Exact bytes are checked
 // against compiled catalogs and existing immutable class evidence. Only four
 // enrolled source models are accepted. No registry C/E is used as nativecache.
 static std::shared_ptr<const Prepared> Read(uint32_t item_id,std::span<const uint8_t> bytes){
  const OriginalHeightScene::MeshCatalogEntry* row=nullptr;for(const auto& r:OriginalHeightScene::OriginalMeshCatalog)if(r.item_id==item_id){if(row)return {};row=&r;}
  if(!row||!ReviewedClass(*row))return {};
  try{auto mesh=OriginalHeightScene::ReadMeshSidecar(bytes,*row);if(!mesh)return {};const uint32_t count=uint32_t(mesh->polygons.size());
   const uint32_t expected_vertices=item_id==54573?718:item_id==52295?56:item_id==66803?355:952;
   const uint32_t expected_polygons=item_id==54573?496:item_id==52295?28:item_id==66803?212:340;
   if(mesh->vertices.size()!=expected_vertices||count!=expected_polygons)return {};
   auto out=std::shared_ptr<Prepared>(new Prepared);out->item_id_=item_id;out->mesh_sha256_=std::string(row->mesh_sha256);out->member_sha256_=std::string(row->member_sha256);out->mesh_=std::make_shared<const Mesh>(std::move(*mesh));
   if(count>200){auto tree=OriginalSimpleTreeGenerator::Build(out->mesh_,{true,true,false,1,count});if(tree.status!=OriginalSimpleTreeGenerator::Status::Ready)return {};out->profile_=Profile::GeneratedPlainTreeV1;out->tree_=std::make_shared<const OriginalSimpleTreeQuery::Tree>(std::move(tree.tree));}
   else {auto linear=OriginalSimpleLinearQuery::Prepare(out->mesh_,{true,false,1,count,false});if(!linear)return {};out->profile_=Profile::OriginalPlainLinearV1;out->linear_=std::make_shared<const OriginalSimpleLinearQuery::Source>(std::move(*linear));}
   // Keep selected exact frame caches narrow. Crate source preparation does not
   // enroll its registry bounds as an exact renderer frame cache.
   if(item_id==54573||item_id==66803){auto cache=OriginalReviewedCache::Read(item_id,bytes);if(!cache)return {};out->selected_cache_=cache->cache;}
   return out;
  }catch(...){return {};}
 }
 // Explicit ordinary selected-actor type9, original moving Floor/Ceiling query
 // family. Sourceproof:441c90 query68=true for motion;9c5868+c→476b50
 // rejects zero only for selected actor type3, therefore false for type9.
 // Current actor type9 and original query family MUST still be bound by the
 // independent zone resolver. Static null-app actors need a distinct profile.
 std::shared_ptr<const QueryReceipt> PlacedHeightCandidates(const Matrix& matrix,float scale,const Query& query)const{
  if(!OriginalActorQueryFrame::ValidMode(query.mode)||!OriginalSphereSource::Parameters(query.start,query.end,query.radius,1))return {};
  if(!OriginalSphereSource::Bits({query.start[0],query.start[1],0},{query.end[0],query.end[1],0})||(query.mode==OriginalActorQueryFrame::Mode::Floor?(query.end[2]!=-32767.f||query.start[2]<=query.end[2]):(query.end[2]!=32767.f||query.start[2]>=query.end[2])))return {};
  try{auto out=std::make_shared<QueryReceipt>();out->item_id=item_id_;out->profile=profile_;out->matrix=matrix;out->scale=scale;out->mesh=mesh_;out->tree=tree_;out->linear=linear_;
   if(tree_){auto delta=OriginalHeightSceneQuery::Delta(query);if(!delta)return {};auto frame=OriginalActorLocalQuery::Build(matrix,scale,query.start,*delta,query.radius);if(!frame)return {};auto local=OriginalActorLocalQuery::SphereDelta(frame->start,frame->delta);if(!local)return {};auto e=OriginalSimpleTreeQuery::Enumerate(*tree_,frame->start,*local,frame->radius);if(e.status!=OriginalSimpleTreeQuery::Status::Ready)return {};out->tree_query=ActorSource::Enumeration{query.start,*delta,query.radius,std::move(e.polygon_ids)};}
   else if(linear_){const OriginalSimpleLinearQuery::Context context{true,true,false};auto e=OriginalHeightSceneQuery::LinearCandidates(*linear_,context,matrix,scale,query);if(!e)return {};out->linear_query=std::move(*e);}
   else return {};return out;
  }catch(...){return {};}
 }
};
}
#endif
