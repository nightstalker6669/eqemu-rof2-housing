#ifndef EQEMU_HOUSING_ORIGINAL_HEIGHT_SCENE_QUERY_H
#define EQEMU_HOUSING_ORIGINAL_HEIGHT_SCENE_QUERY_H
#include "original_actor_local_query.h"
#include "original_mesh_sidecar.h"
namespace Housing::OriginalHeightSceneQuery {
using namespace OriginalHeightScene;
struct ActorSource {
 Identity identity;std::string manifest_sha256;
 OriginalActorLocalQuery::Matrix original_actor_matrix{};float scale{};
 std::shared_ptr<const LocalMesh> mesh;
 enum class Dispatch {Unknown,PlainSimpleAccelerated} dispatch=Dispatch::Unknown;
 // Native definition+18=true,+5c=false,+1c!=4, acceleration+64 nonnull;
 // actual source64 membership/order for THIS query, from a reviewed provider.
 // The raw mesh list alone is NOT this receipt. Bounds/tree traversal are not
 // silently replaced by source-index iteration. No fallbackbranch is supported.
 struct Enumeration {Point world_start{},world_delta{};float radius{};std::vector<uint32_t> ordered_polygon_ids;};
 std::optional<Enumeration> native_tree_query;
};
using ResolveActor=std::function<std::shared_ptr<const ActorSource>(const Entry&)>;
enum class Status {Hit,Clear,Invalid,Unknown,Ambiguous,BudgetExceeded,Environment};
struct Result {Status status=Status::Invalid;uint64_t source_id{},blocked_actor{};float fraction{};Point normal{},stopped{};uint32_t actor_tests{};uint64_t kernel_steps{};};
inline std::optional<Point> Delta(const Query&q){OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};try{Point d;for(unsigned k=0;k<3;++k)d[k]=OriginalActorQueryFrame::Detail::Store(OriginalSphereQuery::Detail::difference(q.end[k],q.start[k]));return OriginalSphereSource::Finite(d)?std::optional<Point>(d):std::nullopt;}catch(...){return {};}}
// Original travel pullback runs once in WORLD space after source comparison.
inline std::optional<Point> WorldStopped(const Point& start,const Point& delta,float fraction){
 OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};if(!OriginalSphereSource::Finite(start)||!OriginalSphereSource::Finite(delta)||!std::isfinite(fraction)||fraction<0||fraction>1)return {};
 try{OriginalSphereQuery::Detail::Math m;m.r[2]=0x17000;m.fpwrite(0x17038,fraction);for(unsigned k=0;k<3;++k){m.fpwrite(0x17004+4*k,start[k]);m.fpwrite(0x1701c+4*k,delta[k]);}m.put(m.r[7],0xeeeeeeee);m.put(m.r[7]+4,0x17100);m.run(0x7a2680);Point out;for(unsigned k=0;k<3;++k)out[k]=float(m.fp(0x17100+4*k));return OriginalSphereSource::Finite(out)?std::optional<Point>(out):std::nullopt;}catch(...){return {};}
}
inline Result Sweep(const Snapshot& scene,const Query& query,const Resolve& terrain_resolver,const ResolveActor& actor_resolver,OriginalSphereSource::Limits limits={}){
 Result out;OriginalSphereQuery::Detail::Rounding environment;if(!environment.good){out.status=Status::Environment;return out;}if(!scene.complete||!scene.epoch||!scene.placed_context||!scene.encoded_zone){out.status=Status::Unknown;return out;}
 if(!limits.sources||limits.sources>65536||!limits.narrowphase||limits.narrowphase>8192||!limits.operations||limits.operations>64000000||scene.entries.size()>limits.sources){out.status=Status::BudgetExceeded;return out;}
 auto delta=Delta(query);auto bounds=OriginalSphereSource::SweptBounds(query.start,query.end,query.radius);if(!delta||!bounds)return out;
 try{std::set<uint64_t> identities;Snapshot terrain=scene;terrain.entries.clear();
 for(const auto&e:scene.entries){if(!e.identity.actor||e.identity.actor>UINT32_MAX||e.identity.epoch!=scene.epoch||!identities.insert(e.identity.actor).second)return out;if(e.terrain)terrain.entries.push_back(e);}
 // This is explicitly one subphase of the complete scene; all nonterrain
 // entries are separately checked below before any successful result escapes.
 auto selected=Collect(terrain,query,terrain_resolver,{limits.sources,1000000,limits.sources});if(selected.status!=OriginalHeightScene::Status::Ready){out.status=selected.status==OriginalHeightScene::Status::BudgetExceeded?Status::BudgetExceeded:Status::Unknown;out.blocked_actor=selected.blocked_actor;return out;}
 auto ground=OriginalSphereSource::Sweep(selected.triangles,query.start,query.end,query.radius,1,limits);out.kernel_steps=ground.operations;
 if(ground.status!=OriginalSphereSource::Status::Hit&&ground.status!=OriginalSphereSource::Status::Clear){out.status=ground.status==OriginalSphereSource::Status::Ambiguous?Status::Ambiguous:ground.status==OriginalSphereSource::Status::BudgetExceeded?Status::BudgetExceeded:Status::Invalid;return out;}
 bool hit=bool(ground);float best=hit?ground.contact.fraction:1;if(hit){out.source_id=ground.source_id;out.normal=ground.contact.normal;}
 for(const auto&e:scene.entries){if(e.terrain)continue;out.blocked_actor=e.identity.actor;
  if(e.disabled&&*e.disabled)continue;
  if(e.conservative_world_bounds){if(!ValidBounds(*e.conservative_world_bounds))return out;if(!Overlaps(*bounds,*e.conservative_world_bounds))continue;}
  if(!e.disabled){out.status=Status::Unknown;return out;}
  auto primary=query.mode==Mode::Floor?PlacedHeightFilters::FloorPrimary(e.actor,scene.placed_context,scene.encoded_zone):PlacedHeightFilters::CeilingPrimary(e.actor,scene.placed_context);
  if(primary==PlacedHeightFilters::Decision::Exclude)continue;if(primary!=PlacedHeightFilters::Decision::Include){out.status=Status::Unknown;return out;}
  const auto secondary=PlacedHeightFilters::Secondary(e.graphics_identity,scene.excluded_graphics_global);if(secondary==PlacedHeightFilters::Decision::Exclude)continue;if(secondary!=PlacedHeightFilters::Decision::Include){out.status=Status::Unknown;return out;}
  if(!e.actor.type||*e.actor.type!=9){out.status=Status::Unknown;return out;}
  if(!actor_resolver||!Sha(e.expected_manifest_sha256)||!Sha(e.identity.resource_sha256)||!Sha(e.identity.frame_sha256)){out.status=Status::Unknown;return out;}
  const auto source=actor_resolver(e);if(!source||source->identity!=e.identity||source->manifest_sha256!=e.expected_manifest_sha256||!source->mesh||source->dispatch!=ActorSource::Dispatch::PlainSimpleAccelerated||!source->native_tree_query){out.status=Status::Unknown;return out;}
  const auto& enumeration=*source->native_tree_query;if(!OriginalSphereSource::Bits(enumeration.world_start,query.start)||!OriginalSphereSource::Bits(enumeration.world_delta,*delta)||std::bit_cast<uint32_t>(enumeration.radius)!=std::bit_cast<uint32_t>(query.radius)){out.status=Status::Unknown;return out;}
  if(source->mesh->polygons.size()>1000000||enumeration.ordered_polygon_ids.size()>limits.sources){out.status=Status::BudgetExceeded;return out;}
  const auto frame=OriginalActorLocalQuery::Build(source->original_actor_matrix,source->scale,query.start,*delta,query.radius);if(!frame){out.status=Status::Unknown;return out;}
  const auto sphere_delta=OriginalActorLocalQuery::SphereDelta(frame->start,frame->delta);if(!sphere_delta||*sphere_delta==Point{}){out.status=Status::Unknown;return out;}
  std::set<uint32_t> visited;for(auto index:enumeration.ordered_polygon_ids){
   if(index>=source->mesh->polygons.size()||!visited.insert(index).second)return out;const auto& polygon=source->mesh->polygons[index];if(polygon.index!=index)return out;
   // This bounded ordinary profile has raw flags0; special mutation and
   // flag1/flag4 fallback/tree divergence require another reviewed dispatch.
   if(polygon.flags!=0){out.status=Status::Unknown;return out;}
   auto normal=OriginalActorLocalQuery::ModelNormal(polygon.native_abc);if(!normal){out.status=Status::Unknown;return out;}if(*normal==Point{})continue;
   // 100d1ed2..ee4: valid-motion front-face rule, NO terrain option1 test.
   const auto dot=OriginalSphereQuery::Detail::sum(OriginalSphereQuery::Detail::sum(OriginalSphereQuery::Detail::product((*sphere_delta)[1],(*normal)[1]),OriginalSphereQuery::Detail::product((*normal)[0],(*sphere_delta)[0])),OriginalSphereQuery::Detail::product((*sphere_delta)[2],(*normal)[2]));if(dot>0)continue;
   if(uint64_t(ground.narrowphase)+out.actor_tests>=limits.narrowphase||out.kernel_steps>=limits.operations){out.status=Status::BudgetExceeded;return out;}++out.actor_tests;
   // Strict scene policy: independentlyquery max1 so storedfraction ties
   // cannot be hidden by the kernel clipping against an earlier roundedtime.
   // This refuses ambiguous source order instead of claiming complete native
   // traversal parity from a DB snapshot.
   const auto contact=OriginalActorLocalQuery::TriangleDelta(polygon.native_abc,*normal,frame->start,*sphere_delta,frame->radius,1);out.kernel_steps+=contact.steps;if(out.kernel_steps>limits.operations){out.status=Status::BudgetExceeded;return out;}
   if(contact.status==OriginalActorLocalQuery::ContactStatus::Clear)continue;if(contact.status!=OriginalActorLocalQuery::ContactStatus::Hit){out.status=Status::Unknown;return out;}
   const uint64_t id=(e.identity.actor<<32)|index;if(hit&&contact.fraction==best&&out.source_id!=id){out.status=Status::Ambiguous;return out;}
   if(!hit||contact.fraction<best){auto world_normal=OriginalActorLocalQuery::WorldNormal(frame->scaled_world,contact.normal);if(!world_normal){out.status=Status::Unknown;return out;}hit=true;best=contact.fraction;out.source_id=id;out.normal=*world_normal;}
  }
 }
 out.blocked_actor=0;if(!hit){out.status=Status::Clear;return out;}auto stopped=WorldStopped(query.start,*delta,best);if(!stopped)return out;out.fraction=best;out.stopped=*stopped;out.status=Status::Hit;return out;
 }catch(...){return out;}
}
struct HeightResult {Result sweep;double height{};bool valid{};};
inline HeightResult Height(const Snapshot& scene,const OriginalActorQueryFrame::HeightRequest& request,const Resolve& terrain_resolver,const ResolveActor& actor_resolver,OriginalSphereSource::Limits limits={}){
 HeightResult out;using namespace OriginalActorQueryFrame;if(!ValidMode(request.mode)||!OriginalSphereSource::Finite(request.input)||!std::isfinite(request.radius)||request.radius<=0||!std::isfinite(request.extra_height)||!std::isfinite(request.display_radius)||request.display_radius<=0)return out;
 OriginalSphereQuery::Detail::Rounding env;if(!env.good){out.sweep.status=Status::Environment;return out;}
 try{Query q;q.mode=request.mode;q.start=request.input;q.end=request.input;q.radius=request.radius;if(request.mode==Mode::Floor){q.start[2]=Detail::Store(Detail::Add(Detail::Add(request.input[2],request.radius),request.extra_height));q.end[2]=-32767.f;}else{q.radius=Detail::Store(OriginalSphereQuery::Detail::product(request.display_radius,request.radius));q.end[2]=32767.f;}
 out.sweep=Sweep(scene,q,terrain_resolver,actor_resolver,limits);if(out.sweep.status!=Status::Hit)return out;
 out.height=request.mode==Mode::Floor?double(Detail::Store(Detail::Add(Detail::Sub(out.sweep.stopped[2],q.radius),double(std::bit_cast<float>(uint32_t(0x3a83126f)))))):Detail::Add(out.sweep.stopped[2],q.radius);out.valid=std::isfinite(out.height);return out;
 }catch(...){return out;}
}
}
#endif
