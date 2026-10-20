#ifndef EQEMU_HOUSING_ORIGINAL_HEIGHT_SCENE_COLLECTOR_H
#define EQEMU_HOUSING_ORIGINAL_HEIGHT_SCENE_COLLECTOR_H
#include "original_actor_query_frame.h"
#include "placed_height_filters.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace Housing::OriginalHeightScene {
using Point=OriginalSphereQuery::Point;
using Bounds=OriginalSphereSource::Bounds;
using Triangle=OriginalSphereSource::Triangle;
using Mode=OriginalActorQueryFrame::Mode;
// Server snapshot key, NEVER a client supplied native pointer. The bridge must
// keep the exact correspondence between this key and native appdata identity.
struct Identity {
 uint64_t epoch{},actor{},pose_revision{};
 std::string resource_sha256,frame_sha256;
 bool operator==(const Identity&)const=default;
};
struct Polygon {uint32_t index{};std::array<Point,3> native_abc{};uint16_t flags{};};
struct Source {
 Identity identity;
 std::string manifest_sha256;
 std::vector<Polygon> polygons;
 // Only a reviewed source adapter may construct this after proving this exact
 // dispatch uses 100bda70's triangle predicate. Ordinary type9 alone is NOT proof.
 enum class Dispatch {Terrain100bda70,Unsupported} dispatch=Dispatch::Unsupported;
};
struct Entry {
 Identity identity;
 // Null bounds means an unknown potentially overlapping source, not empty space.
 // Bounds must be outward and authenticated by the complete snapshot provider.
 // Missing bounds cannot be broadphase-culled.
 std::optional<Bounds> conservative_world_bounds;
 std::optional<bool> disabled;
 bool terrain{};
 PlacedHeightFilters::Actor actor;
 std::optional<uint64_t> graphics_identity;
 std::string expected_manifest_sha256;
};
struct Snapshot {
 uint64_t epoch{},placed_context{};
 uint32_t encoded_zone{};
 std::optional<uint64_t> excluded_graphics_global;
 // Receipt supplied by complete zone adapter, after terrain, alternate-height
 // field, both actor trees and disabled state have been accounted for. A partial
 // housing DB row list MUST NOT set complete=true.
 bool complete{};
 std::vector<Entry> entries;
};
struct Query {Mode mode=Mode::Floor;Point start{},end{};float radius{};};
enum class Status {Ready,InvalidQuery,IncompleteScene,UnknownSource,InvalidSource,BudgetExceeded,Environment};
struct Result {Status status=Status::InvalidQuery;std::vector<Triangle> triangles;uint32_t seen{},resolved{},excluded{};uint64_t blocked_actor{};};
struct Limits {uint32_t entries=16384,polygons=65536,selected=16384;};
// Resolver authenticates the immutable private sidecar's content against the
// expected SHA and EXACT current native frame before returning world-native ABC.
// Its immutable shared owner remains live throughout collection. No raw pointers.
using Resolve=std::function<std::shared_ptr<const Source>(const Entry&)>;
inline bool Sha(std::string_view s){if(s.size()!=64)return false;for(char c:s)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;return true;}
inline bool ValidBounds(const Bounds& b){for(unsigned k=0;k<3;++k)if(!std::isfinite(b.minimum[k])||!std::isfinite(b.maximum[k])||b.minimum[k]>b.maximum[k])return false;return true;}
inline bool Overlaps(const Bounds& a,const Bounds& b){for(unsigned k=0;k<3;++k)if(a.maximum[k]<b.minimum[k]||b.maximum[k]<a.minimum[k])return false;return true;}
// Actual original 10021339..100213f5 normal producer, not geometric epsilon.
inline std::optional<Point> Normal(const std::array<Point,3>& triangle){
 OriginalSphereQuery::Detail::Rounding environment;if(!environment.good)return {};for(const auto&p:triangle)if(!OriginalSphereSource::Finite(p))return {};
 try{OriginalSphereQuery::Detail::Math m;constexpr uint32_t d=0x10000;constexpr std::array<uint32_t,3>at{0x10100,0x10110,0x10120};
 for(unsigned j=0;j<3;++j){m.put(d+0x10+4*j,at[j]);for(unsigned k=0;k<3;++k)m.fpwrite(at[j]+4*k,triangle[j][k]);}
 m.r[2]=d;m.r[5]=at[0];m.r[3]=at[1];m.r[1]=at[2];m.run(0x10021339,0x100213f5);
 Point n;for(unsigned k=0;k<3;++k)n[k]=float(m.fp(d+4*k));if(!OriginalSphereSource::Finite(n))return {};return n;
 }catch(...){return {};}
}
// Native query stores delta as binary32 before callback. Sum instruction order
// is (deltaY*normalY + normalX*deltaX) + deltaZ*normalZ, PC53/chop.
inline std::optional<double> DeltaDot(const Point& start,const Point& end,const Point& normal){
 using namespace OriginalSphereQuery;Detail::Rounding environment;if(!environment.good)return {};try{Point delta;for(unsigned k=0;k<3;++k){volatile double d=Detail::difference(end[k],start[k]);volatile float f=float(d);delta[k]=f;if(!std::isfinite(delta[k]))return {};}
 return Detail::sum(Detail::sum(Detail::product(delta[1],normal[1]),Detail::product(normal[0],delta[0])),Detail::product(delta[2],normal[2]));}catch(...){return {};}
}
inline Result Collect(const Snapshot& scene,const Query& query,const Resolve& resolve,Limits limits={}){
 Result out;if(!OriginalActorQueryFrame::ValidMode(query.mode)||!OriginalSphereSource::Parameters(query.start,query.end,query.radius,1)||!resolve||!limits.entries||limits.entries>65536||!limits.polygons||limits.polygons>1000000||!limits.selected||limits.selected>65536)return out;
 // Scope is the actual vertical fallback, not screen/camera mount queries.
 if(!OriginalSphereSource::Bits({query.start[0],query.start[1],0},{query.end[0],query.end[1],0})||(query.mode==Mode::Floor?(query.end[2]!=-32767.f||query.start[2]<=query.end[2]):(query.end[2]!=32767.f||query.start[2]>=query.end[2])))return out;
 if(!scene.complete||!scene.epoch||!scene.placed_context||!scene.encoded_zone){out.status=Status::IncompleteScene;return out;}
 if(scene.entries.size()>limits.entries){out.status=Status::BudgetExceeded;return out;}
 OriginalSphereQuery::Detail::Rounding environment;if(!environment.good){out.status=Status::Environment;return out;}
 const auto bounds=OriginalSphereSource::SweptBounds(query.start,query.end,query.radius);if(!bounds){out.status=Status::Environment;return out;}
 try{std::set<uint64_t> actors;uint64_t polygons=0;
 for(const auto& entry:scene.entries){++out.seen;out.blocked_actor=entry.identity.actor;
  if(!entry.identity.actor||!actors.insert(entry.identity.actor).second||entry.identity.epoch!=scene.epoch){out.status=Status::InvalidSource;return out;}
  if(entry.disabled&&*entry.disabled){++out.excluded;continue;}
  if(entry.conservative_world_bounds){if(!ValidBounds(*entry.conservative_world_bounds)){out.status=Status::InvalidSource;return out;}if(!Overlaps(*bounds,*entry.conservative_world_bounds))continue;}
  if(!entry.disabled){out.status=Status::UnknownSource;return out;}
  if(!entry.terrain){
   auto primary=query.mode==Mode::Floor?PlacedHeightFilters::FloorPrimary(entry.actor,scene.placed_context,scene.encoded_zone):PlacedHeightFilters::CeilingPrimary(entry.actor,scene.placed_context);
   if(primary==PlacedHeightFilters::Decision::Exclude){++out.excluded;continue;}if(primary!=PlacedHeightFilters::Decision::Include){out.status=Status::UnknownSource;return out;}
   auto secondary=PlacedHeightFilters::Secondary(entry.graphics_identity,scene.excluded_graphics_global);
   if(secondary==PlacedHeightFilters::Decision::Exclude){++out.excluded;continue;}if(secondary!=PlacedHeightFilters::Decision::Include){out.status=Status::UnknownSource;return out;}
   // Confirmed plain CSimpleModel uses localquery100c5af0, NOT terrain100bda70.
   // Included actors remain explicit refusals until that dispatch is ported.
   out.status=Status::UnknownSource;return out;
  }
  if(!Sha(entry.expected_manifest_sha256)||!Sha(entry.identity.resource_sha256)||!Sha(entry.identity.frame_sha256)){out.status=Status::UnknownSource;return out;}
  const auto source=resolve(entry);if(!source){out.status=Status::UnknownSource;return out;}
  if(source->identity!=entry.identity||source->manifest_sha256!=entry.expected_manifest_sha256){out.status=Status::InvalidSource;return out;}
  if(source->dispatch!=Source::Dispatch::Terrain100bda70){out.status=Status::UnknownSource;return out;}
  if(polygons+source->polygons.size()>limits.polygons){out.status=Status::BudgetExceeded;return out;}polygons+=source->polygons.size();++out.resolved;
  std::set<uint32_t> ids;
  for(const auto& polygon:source->polygons){if(!ids.insert(polygon.index).second){out.status=Status::InvalidSource;return out;}
   // Enumeration assigns IDs, preserving the full polygon index in the source
   // sidecar. Reject >32bit actors rather than truncate identity/collide IDs.
   if(entry.identity.actor>UINT32_MAX){out.status=Status::InvalidSource;return out;}
   Triangle triangle{(entry.identity.actor<<32)|polygon.index,polygon.native_abc};
   for(const auto&p:triangle.vertices)if(!OriginalSphereSource::Finite(p)){out.status=Status::InvalidSource;return out;}
   if(!OriginalSphereSource::Intersects(*bounds,triangle))continue;
   auto normal=Normal(triangle.vertices);if(!normal){out.status=Status::InvalidSource;return out;}
   auto dot=DeltaDot(query.start,query.end,*normal);if(!dot){out.status=Status::InvalidSource;return out;}
   const auto decision=PlacedHeightFilters::TerrainTriangle(polygon.flags,false,false,true,true,*dot,(*normal)[2]);
   if(decision==PlacedHeightFilters::Decision::Unknown){out.status=Status::UnknownSource;return out;}if(decision==PlacedHeightFilters::Decision::Exclude)continue;
   if(out.triangles.size()==limits.selected){out.status=Status::BudgetExceeded;return out;}out.triangles.push_back(triangle);
  }
 }
 out.blocked_actor=0;out.status=Status::Ready;return out;
 }catch(...){out.status=Status::InvalidSource;return out;}
}
struct HeightResult {Result scene;OriginalActorQueryFrame::HeightResult height;};
// Bridge entry point. Never sends a partially collected/failed scene to solver.
// The caller retains the immutable transaction snapshot through the entire call.
inline HeightResult Height(const Snapshot& scene,const OriginalActorQueryFrame::HeightRequest& request,const Resolve& resolve,Limits scene_limits={},OriginalSphereSource::Limits query_limits={}){
 HeightResult out;using namespace OriginalActorQueryFrame;
 if(!ValidMode(request.mode)||!OriginalSphereSource::Finite(request.input)||!std::isfinite(request.radius)||request.radius<=0||!std::isfinite(request.extra_height)||!std::isfinite(request.display_radius)||request.display_radius<=0)return out;
 OriginalSphereQuery::Detail::Rounding environment;if(!environment.good){out.scene.status=Status::Environment;return out;}
 try{Query q;q.mode=request.mode;q.start=request.input;q.end=request.input;q.radius=request.radius;
 if(request.mode==Mode::Floor){q.start[2]=Detail::Store(Detail::Add(Detail::Add(request.input[2],request.radius),request.extra_height));q.end[2]=-32767.f;}
 else{q.radius=Detail::Store(OriginalSphereQuery::Detail::product(request.display_radius,request.radius));q.end[2]=32767.f;}
 out.scene=Collect(scene,q,resolve,scene_limits);if(out.scene.status!=Status::Ready)return out;
 out.height=OriginalActorQueryFrame::Height(out.scene.triangles,request,query_limits);return out;
 }catch(...){out.scene.status=Status::InvalidQuery;return out;}
}

}
#endif
