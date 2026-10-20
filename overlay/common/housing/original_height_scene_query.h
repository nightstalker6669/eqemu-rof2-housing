#ifndef EQEMU_HOUSING_ORIGINAL_HEIGHT_SCENE_QUERY_H
#define EQEMU_HOUSING_ORIGINAL_HEIGHT_SCENE_QUERY_H
#include "original_actor_local_query.h"
#include "original_mesh_sidecar.h"
#include "original_simple_linear_query.h"
namespace Housing::OriginalZonStaticActor {struct Provider;}
namespace Housing::OriginalHeightSceneQuery {
using namespace OriginalHeightScene;
// Sealed producer proof for a reviewed ordinary authored-null-app role.
// Current scene metadata is still independently attested; constructor defaults
// cannot authorize an actor repurposed as a game switch or changed definition.
class AuthoredClassProof {
 friend struct Housing::OriginalZonStaticActor::Provider;
 Identity identity_;std::string manifest_;OriginalActorLocalQuery::Matrix matrix_;float scale_{};std::shared_ptr<const LocalMesh> mesh_;std::shared_ptr<const OriginalSimpleTreeQuery::Tree> tree_;bool all_skipped_{};
 AuthoredClassProof(Identity id,std::string m,OriginalActorLocalQuery::Matrix matrix,float scale,std::shared_ptr<const LocalMesh> mesh,std::shared_ptr<const OriginalSimpleTreeQuery::Tree> tree={},bool all_skipped=false):identity_(std::move(id)),manifest_(std::move(m)),matrix_(matrix),scale_(scale),mesh_(std::move(mesh)),tree_(std::move(tree)),all_skipped_(all_skipped){}
public:
 bool AllSkipped()const{return all_skipped_;}
 bool Accelerated()const{return bool(tree_);}
 bool Matches(const Entry&e,const OriginalActorLocalQuery::Matrix&matrix,float scale,const std::shared_ptr<const LocalMesh>&mesh,const std::shared_ptr<const OriginalSimpleTreeQuery::Tree>&tree={})const{
  if(tree!=tree_||mesh!=mesh_||identity_!=e.identity||manifest_!=e.expected_manifest_sha256||std::bit_cast<uint32_t>(scale)!=std::bit_cast<uint32_t>(scale_)||!e.actor.type||*e.actor.type!=5||!e.actor.application_identity||*e.actor.application_identity!=0||!e.actor.restriction_mask||*e.actor.restriction_mask||!e.actor.volume||*e.actor.volume)return false;
  for(unsigned k=0;k<16;++k)if(std::bit_cast<uint32_t>(matrix[k])!=std::bit_cast<uint32_t>(matrix_[k]))return false;return true;
 }
};
struct ActorSource {
 enum class Role {PlacedType9,AuthoredNullAppPlain} role=Role::PlacedType9;
 std::shared_ptr<const AuthoredClassProof> authored_class;
 std::shared_ptr<const OriginalSimpleTreeQuery::Tree> authored_tree;
 Identity identity;std::string manifest_sha256;
 OriginalActorLocalQuery::Matrix original_actor_matrix{};float scale{};
 std::shared_ptr<const LocalMesh> mesh;
 enum class Dispatch {Unknown,PlainSimpleAccelerated,PlainSimpleLinear,PlainSimpleLinearAllSkipped} dispatch=Dispatch::Unknown;
 // Native definition+18=true,+5c=false,+1c!=4, acceleration+64 nonnull;
 // actual source64 membership/order for THIS query, from a reviewed provider.
 // The raw mesh list alone is NOT this receipt. Bounds/tree traversal are not
 // silently replaced by source-index iteration. No fallbackbranch is supported.
 struct Enumeration {Point world_start{},world_delta{};float radius{};std::vector<uint32_t> ordered_polygon_ids;};
 std::optional<Enumeration> native_tree_query;
 // Distinct linear receipt: reviewed <=200 enabled ordinary definition with
 // actual source64 absence and original callback context fields, never a fake
 // accelerated tree. Definitions/descriptors are immutable provider-owned data.
 struct LinearEnumeration {Point world_start{},world_delta{};float radius{};OriginalSimpleLinearQuery::Context context;std::vector<uint32_t> ordered_polygon_ids;};
 std::shared_ptr<const OriginalSimpleLinearQuery::Source> linear_source;
 std::optional<LinearEnumeration> native_linear_query;
};
using ResolveActor=std::function<std::shared_ptr<const ActorSource>(const Entry&)>;
// Query-aware resolver builds source64 receipts for the ACTUAL constructed
// Height start/end/radius, not a caller's pre-transform request or stale probe.
using ResolveActorQuery=std::function<std::shared_ptr<const ActorSource>(const Entry&,const Query&)>;
inline ResolveActorQuery QueryAware(ResolveActor fixed){return [fixed=std::move(fixed)](const Entry&e,const Query&)->std::shared_ptr<const ActorSource>{return fixed?fixed(e):nullptr;};}
enum class Status {Hit,Clear,Invalid,Unknown,Ambiguous,BudgetExceeded,Environment};
struct Result {Status status=Status::Invalid;uint64_t source_id{},blocked_actor{};float fraction{};Point normal{},stopped{};// Aggregate solver accounting, including terrain. sources counts selected
 // terrain triangles and actor enumeration descriptors visited (even culled).
 uint32_t sources{},narrowphase{},terrain_tests{},actor_tests{};uint64_t kernel_steps{};
 // Enumeration/culling is separately bounded; kernel_steps is NOT CPU time.
 uint32_t actor_polygons{},actor_meshes{};};
inline std::optional<Point> Delta(const Query&q){OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};try{Point d;for(unsigned k=0;k<3;++k)d[k]=OriginalActorQueryFrame::Detail::Store(OriginalSphereQuery::Detail::difference(q.end[k],q.start[k]));return OriginalSphereSource::Finite(d)?std::optional<Point>(d):std::nullopt;}catch(...){return {};}}
// Receipt creation is pure geometry only. Provider must independently prove
// the original definition/context fields, source identity and complete census.
inline std::optional<ActorSource::LinearEnumeration> LinearCandidates(const OriginalSimpleLinearQuery::Source& source,const OriginalSimpleLinearQuery::Context& context,const OriginalActorLocalQuery::Matrix& matrix,float scale,const Query& query){
 auto delta=Delta(query);if(!delta)return {};auto frame=OriginalActorLocalQuery::Build(matrix,scale,query.start,*delta,query.radius);if(!frame)return {};
 auto result=OriginalSimpleLinearQuery::Enumerate(source,context,frame->start,frame->delta,frame->radius);if(result.status!=OriginalSimpleLinearQuery::Status::Ready)return {};
 return ActorSource::LinearEnumeration{query.start,*delta,query.radius,context,std::move(result.polygon_ids)};
}
// Original travel pullback runs once in WORLD space after source comparison.
inline std::optional<Point> WorldStopped(const Point& start,const Point& delta,float fraction){
 OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};if(!OriginalSphereSource::Finite(start)||!OriginalSphereSource::Finite(delta)||!std::isfinite(fraction)||fraction<0||fraction>1)return {};
 try{OriginalSphereQuery::Detail::Math m;m.r[2]=0x17000;m.fpwrite(0x17038,fraction);for(unsigned k=0;k<3;++k){m.fpwrite(0x17004+4*k,start[k]);m.fpwrite(0x1701c+4*k,delta[k]);}m.put(m.r[7],0xeeeeeeee);m.put(m.r[7]+4,0x17100);m.run(0x7a2680);Point out;for(unsigned k=0;k<3;++k)out[k]=float(m.fp(0x17100+4*k));return OriginalSphereSource::Finite(out)?std::optional<Point>(out):std::nullopt;}catch(...){return {};}
}
// Strict complete-scene comparison: every terrain primitive is queried at
// max1, just like actor primitives. Legacy OriginalSphereSource::Sweep keeps
// its existing clipped/equivalent-contact policy unchanged. Here ALL distinct
// source ties at the final minimum are refused, independent of traversal order.
inline OriginalSphereSource::Result TerrainSweep(std::span<const OriginalSphereSource::Triangle> triangles,const Query& q,OriginalSphereSource::Limits limits){
 using namespace OriginalSphereSource;OriginalSphereSource::Result out;
 if(!Parameters(q.start,q.end,q.radius,1)||!limits.sources||limits.sources>65536||!limits.narrowphase||limits.narrowphase>8192||!limits.operations||limits.operations>64000000)return out;
 OriginalSphereQuery::Detail::Rounding env;if(!env.good){out.status=OriginalSphereSource::Status::Environment;return out;}
 if(triangles.size()>limits.sources){out.status=OriginalSphereSource::Status::BudgetExceeded;return out;}
 const auto bounds=SweptBounds(q.start,q.end,q.radius);if(!bounds)return out;
 bool hit=false,ambiguous=false;float best=1;std::set<uint64_t> ids;
 for(const auto&t:triangles){
  ++out.sources;if(!ids.insert(t.id).second){out.status=OriginalSphereSource::Status::InvalidSource;return out;}
  for(const auto&p:t.vertices)if(!Finite(p)){out.status=OriginalSphereSource::Status::InvalidSource;return out;}
  if(!Intersects(*bounds,t))continue;
  if(out.narrowphase==limits.narrowphase||limits.operations-out.operations<48000){out.status=OriginalSphereSource::Status::BudgetExceeded;return out;}++out.narrowphase;
  auto contact=OriginalSphereQuery::Triangle(t.vertices,q.start,q.end,q.radius,1);
  if(!contact){out.status=OriginalSphereSource::Status::InvalidSource;return out;}out.operations+=contact->steps;
  if(out.operations>limits.operations){out.status=OriginalSphereSource::Status::BudgetExceeded;return out;}
  if(!contact->hit)continue;
  if(!hit||contact->fraction<best){hit=true;ambiguous=false;best=contact->fraction;out.source_id=t.id;out.contact=*contact;}
  else if(contact->fraction==best&&t.id!=out.source_id)ambiguous=true;
 }
 out.status=ambiguous?OriginalSphereSource::Status::Ambiguous:hit?OriginalSphereSource::Status::Hit:OriginalSphereSource::Status::Clear;return out;
}
inline Result SweepQuery(const Snapshot& scene,const Query& query,const Resolve& terrain_resolver,const ResolveActorQuery& actor_resolver,OriginalSphereSource::Limits limits={}){
 Result out;OriginalSphereQuery::Detail::Rounding environment;if(!environment.good){out.status=Status::Environment;return out;}if(!scene.complete||!scene.epoch||!scene.placed_context||!scene.encoded_zone){out.status=Status::Unknown;return out;}
 if(!limits.sources||limits.sources>65536||!limits.narrowphase||limits.narrowphase>8192||!limits.operations||limits.operations>64000000||scene.entries.size()>limits.sources){out.status=Status::BudgetExceeded;return out;}
 auto delta=Delta(query);auto bounds=OriginalSphereSource::SweptBounds(query.start,query.end,query.radius);if(!delta||!bounds)return out;
 try{std::set<uint64_t> identities;Snapshot terrain=scene;terrain.entries.clear();
 for(const auto&e:scene.entries){if(!e.identity.actor||e.identity.actor>UINT32_MAX||e.identity.epoch!=scene.epoch||!identities.insert(e.identity.actor).second)return out;if(e.terrain)terrain.entries.push_back(e);}
 // This is explicitly one subphase of the complete scene; all nonterrain
 // entries are separately checked below before any successful result escapes.
 auto selected=Collect(terrain,query,terrain_resolver,{limits.sources,1000000,limits.sources});if(selected.status!=OriginalHeightScene::Status::Ready){out.status=selected.status==OriginalHeightScene::Status::BudgetExceeded?Status::BudgetExceeded:Status::Unknown;out.blocked_actor=selected.blocked_actor;return out;}
 auto ground=TerrainSweep(selected.triangles,query,limits);out.kernel_steps=ground.operations;out.sources=ground.sources;out.narrowphase=ground.narrowphase;out.terrain_tests=ground.narrowphase;
 if(ground.status!=OriginalSphereSource::Status::Hit&&ground.status!=OriginalSphereSource::Status::Clear&&ground.status!=OriginalSphereSource::Status::Ambiguous){out.status=ground.status==OriginalSphereSource::Status::Ambiguous?Status::Ambiguous:ground.status==OriginalSphereSource::Status::BudgetExceeded?Status::BudgetExceeded:Status::Invalid;return out;}
 bool ambiguous=ground.status==OriginalSphereSource::Status::Ambiguous;bool hit=bool(ground)||ambiguous;float best=hit?ground.contact.fraction:1;if(hit){out.source_id=ground.source_id;out.normal=ground.contact.normal;}
 for(const auto&e:scene.entries){if(e.terrain)continue;out.blocked_actor=e.identity.actor;
  if(e.disabled&&*e.disabled)continue;
  if(e.conservative_world_bounds){if(!ValidBounds(*e.conservative_world_bounds))return out;if(!Overlaps(*bounds,*e.conservative_world_bounds))continue;}
  if(!e.disabled){out.status=Status::Unknown;return out;}
  auto primary=query.mode==Mode::Floor?PlacedHeightFilters::FloorPrimary(e.actor,scene.placed_context,scene.encoded_zone):PlacedHeightFilters::CeilingPrimary(e.actor,scene.placed_context);
  if(primary==PlacedHeightFilters::Decision::Exclude)continue;if(primary!=PlacedHeightFilters::Decision::Include){out.status=Status::Unknown;return out;}
  const auto secondary=PlacedHeightFilters::Secondary(e.graphics_identity,scene.excluded_graphics_global);if(secondary==PlacedHeightFilters::Decision::Exclude)continue;if(secondary!=PlacedHeightFilters::Decision::Include){out.status=Status::Unknown;return out;}
  if(!e.actor.type){out.status=Status::Unknown;return out;}
  if(!actor_resolver||!Sha(e.expected_manifest_sha256)||!Sha(e.identity.resource_sha256)||!Sha(e.identity.frame_sha256)){out.status=Status::Unknown;return out;}
  const auto source=actor_resolver(e,query);if(!source||source->identity!=e.identity||source->manifest_sha256!=e.expected_manifest_sha256||!source->mesh){out.status=Status::Unknown;return out;}
  if(source->role==ActorSource::Role::PlacedType9){if(*e.actor.type!=9||source->authored_class){out.status=Status::Unknown;return out;}}
  else if(source->role!=ActorSource::Role::AuthoredNullAppPlain||!source->authored_class||source->dispatch!=(source->authored_class->AllSkipped()?ActorSource::Dispatch::PlainSimpleLinearAllSkipped:source->authored_class->Accelerated()?ActorSource::Dispatch::PlainSimpleAccelerated:ActorSource::Dispatch::PlainSimpleLinear)||!source->authored_class->Matches(e,source->original_actor_matrix,source->scale,source->mesh,source->authored_tree)){out.status=Status::Unknown;return out;}
  // A sealed, immutable ordinary definition whose EVERY actual preserved
  // polygon has (flags&1)&&!(flags&4). Original100c5dac..5dc7 skips these
  // before cache/callback/kernel. This actor remains included in the census;
  // no query-dependent primitive receipt, disabled flag or fake tree is used.
  if(source->dispatch==ActorSource::Dispatch::PlainSimpleLinearAllSkipped){
   if(source->role!=ActorSource::Role::AuthoredNullAppPlain||!source->authored_class||!source->authored_class->AllSkipped()||source->authored_tree||source->native_tree_query||source->linear_source||source->native_linear_query){out.status=Status::Unknown;return out;}
   if(++out.actor_meshes>limits.sources||source->mesh->polygons.size()>1000000-out.actor_polygons){out.status=Status::BudgetExceeded;return out;}
   out.actor_polygons+=uint32_t(source->mesh->polygons.size());continue;
  }
  const bool linear=source->dispatch==ActorSource::Dispatch::PlainSimpleLinear;
  if(!linear&&source->dispatch!=ActorSource::Dispatch::PlainSimpleAccelerated){out.status=Status::Unknown;return out;}
  const std::vector<uint32_t>* ids=nullptr;
  if(linear){
   if(source->native_tree_query||!source->linear_source||!source->native_linear_query||source->linear_source->mesh!=source->mesh||!OriginalSimpleLinearQuery::ValidDefinition(source->linear_source->definition)||source->linear_source->definition.polygon_count!=source->mesh->polygons.size()||source->linear_source->descriptors.size()!=source->mesh->polygons.size()){out.status=Status::Unknown;return out;}
   const auto& receipt=*source->native_linear_query;
   if(!receipt.context.reviewed||!OriginalSphereSource::Bits(receipt.world_start,query.start)||!OriginalSphereSource::Bits(receipt.world_delta,*delta)||std::bit_cast<uint32_t>(receipt.radius)!=std::bit_cast<uint32_t>(query.radius)){out.status=Status::Unknown;return out;}
   ids=&receipt.ordered_polygon_ids;
   // Actual linear source visits original definition in strictly descending
   // order. A receipt from another dispatch/order is not interchangeable.
   for(size_t n=1;n<ids->size();++n)if((*ids)[n]>=(*ids)[n-1])return out;
  }else{
   if(!source->native_tree_query||source->linear_source||source->native_linear_query){out.status=Status::Unknown;return out;}
   const auto& receipt=*source->native_tree_query;
   if(!OriginalSphereSource::Bits(receipt.world_start,query.start)||!OriginalSphereSource::Bits(receipt.world_delta,*delta)||std::bit_cast<uint32_t>(receipt.radius)!=std::bit_cast<uint32_t>(query.radius)){out.status=Status::Unknown;return out;}
   ids=&receipt.ordered_polygon_ids;
  }
  if(++out.actor_meshes>limits.sources||source->mesh->polygons.size()>1000000-out.actor_polygons||ids->size()>limits.sources-out.sources){out.status=Status::BudgetExceeded;return out;}
  out.actor_polygons+=uint32_t(source->mesh->polygons.size());
  const auto frame=OriginalActorLocalQuery::Build(source->original_actor_matrix,source->scale,query.start,*delta,query.radius);if(!frame){out.status=Status::Unknown;return out;}
  // Linear100bf860 receives the original transformed delta directly; the
  // accelerated sphere constructor adds stored start+delta then subtracts start.
  const auto local_delta=linear?std::optional<Point>(frame->delta):OriginalActorLocalQuery::SphereDelta(frame->start,frame->delta);
  if(!local_delta||*local_delta==Point{}){out.status=Status::Unknown;return out;}
  std::set<uint32_t> visited;for(auto index:*ids){
   ++out.sources;
   if(index>=source->mesh->polygons.size()||!visited.insert(index).second)return out;const auto& polygon=source->mesh->polygons[index];if(polygon.index!=index)return out;
   if(polygon.flags!=0){out.status=Status::Unknown;return out;}
   std::optional<Point> tree_normal;
   if(!linear){
    auto normal=OriginalActorLocalQuery::ModelNormal(polygon.native_abc);if(!normal){out.status=Status::Unknown;return out;}if(*normal==Point{})continue;tree_normal=normal;
    const auto dot=OriginalSphereQuery::Detail::sum(OriginalSphereQuery::Detail::sum(OriginalSphereQuery::Detail::product((*local_delta)[1],(*normal)[1]),OriginalSphereQuery::Detail::product((*normal)[0],(*local_delta)[0])),OriginalSphereQuery::Detail::product((*local_delta)[2],(*normal)[2]));if(dot>0)continue;
   }
   if(out.narrowphase>=limits.narrowphase||limits.operations-out.kernel_steps<48000){out.status=Status::BudgetExceeded;return out;}++out.actor_tests;++out.narrowphase;
   float fraction{};Point local_normal{};
   if(linear){
    const auto contact=OriginalSimpleLinearQuery::Triangle(*source->linear_source,index,source->native_linear_query->context,frame->start,*local_delta,frame->radius,1);out.kernel_steps+=contact.steps;if(out.kernel_steps>limits.operations){out.status=Status::BudgetExceeded;return out;}
    if(contact.status==OriginalSimpleLinearQuery::ContactStatus::Clear)continue;if(contact.status!=OriginalSimpleLinearQuery::ContactStatus::Hit){out.status=Status::Unknown;return out;}fraction=contact.fraction;local_normal=contact.cached_normal;
   }else{
    const auto contact=OriginalActorLocalQuery::TriangleDelta(polygon.native_abc,*tree_normal,frame->start,*local_delta,frame->radius,1);out.kernel_steps+=contact.steps;if(out.kernel_steps>limits.operations){out.status=Status::BudgetExceeded;return out;}
    if(contact.status==OriginalActorLocalQuery::ContactStatus::Clear)continue;if(contact.status!=OriginalActorLocalQuery::ContactStatus::Hit){out.status=Status::Unknown;return out;}fraction=contact.fraction;local_normal=contact.normal;
   }
   const uint64_t id=(e.identity.actor<<32)|index;if(hit&&fraction==best&&out.source_id!=id)ambiguous=true;
   if(!hit||fraction<best){auto world_normal=OriginalActorLocalQuery::WorldNormal(frame->scaled_world,local_normal);if(!world_normal){out.status=Status::Unknown;return out;}hit=true;ambiguous=false;best=fraction;out.source_id=id;out.normal=*world_normal;}
  }
 }
 out.blocked_actor=0;if(ambiguous){out.status=Status::Ambiguous;return out;}if(!hit){out.status=Status::Clear;return out;}auto stopped=WorldStopped(query.start,*delta,best);if(!stopped)return out;out.fraction=best;out.stopped=*stopped;out.status=Status::Hit;return out;
 }catch(...){return out;}
}
// Compatibility API for a single query with an already query-bound receipt.
inline Result Sweep(const Snapshot& scene,const Query& query,const Resolve& terrain_resolver,const ResolveActor& actor_resolver,OriginalSphereSource::Limits limits={}){return SweepQuery(scene,query,terrain_resolver,QueryAware(actor_resolver),limits);}
struct HeightResult {Result sweep;double height{};bool valid{};};
inline HeightResult HeightQuery(const Snapshot& scene,const OriginalActorQueryFrame::HeightRequest& request,const Resolve& terrain_resolver,const ResolveActorQuery& actor_resolver,OriginalSphereSource::Limits limits={}){
 HeightResult out;using namespace OriginalActorQueryFrame;if(!ValidMode(request.mode)||!OriginalSphereSource::Finite(request.input)||!std::isfinite(request.radius)||request.radius<=0||!std::isfinite(request.extra_height)||!std::isfinite(request.display_radius)||request.display_radius<=0)return out;
 OriginalSphereQuery::Detail::Rounding env;if(!env.good){out.sweep.status=Status::Environment;return out;}
 try{Query q;q.mode=request.mode;q.start=request.input;q.end=request.input;q.radius=request.radius;if(request.mode==Mode::Floor){q.start[2]=Detail::Store(Detail::Add(Detail::Add(request.input[2],request.radius),request.extra_height));q.end[2]=-32767.f;}else{q.radius=Detail::Store(OriginalSphereQuery::Detail::product(request.display_radius,request.radius));q.end[2]=32767.f;}
 out.sweep=SweepQuery(scene,q,terrain_resolver,actor_resolver,limits);if(out.sweep.status!=Status::Hit)return out;
 out.height=request.mode==Mode::Floor?double(Detail::Store(Detail::Add(Detail::Sub(out.sweep.stopped[2],q.radius),double(std::bit_cast<float>(uint32_t(0x3a83126f)))))):Detail::Add(out.sweep.stopped[2],q.radius);out.valid=std::isfinite(out.height);return out;
 }catch(...){return out;}
}
inline HeightResult Height(const Snapshot& scene,const OriginalActorQueryFrame::HeightRequest& request,const Resolve& terrain_resolver,const ResolveActor& actor_resolver,OriginalSphereSource::Limits limits={}){return HeightQuery(scene,request,terrain_resolver,QueryAware(actor_resolver),limits);}
}
#endif
