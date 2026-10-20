#ifndef EQEMU_HOUSING_CURVED_MOUNT_SUPPORT_V6_H
#define EQEMU_HOUSING_CURVED_MOUNT_SUPPORT_V6_H
#include "planar_mount_patch.h"

#include "curved_mount_contact.h"
#include <functional>
namespace Housing::CurvedMount {
using Triangle=MountSurfacePatch::Triangle;
using R=OrientedMount::Rational;
using Vector=OrientedMount::ExactPoint;
using Polygon=MountSurfacePatch::Polygon;
using Point2=MountSurfacePatch::Point2;
inline constexpr const char* PolicyTag="EQEmuReviewedFiniteGraphCurvedMountRegionsV6";
// Explicit reviewed native-mode metadata is external source evidence. These
// bits label the original Wall/Ceiling predicates, not new packet mode fields.
enum class Mode:uint8_t {Wall=1,Ceiling=2};
struct Source {Triangle geometry;uint8_t side_mask=0,native_mode_mask=0;};
inline R Area(const Polygon&p){R sum=0;for(size_t j=0;j<p.size();++j){const auto&a=p[j];const auto&b=p[(j+1)%p.size()];sum+=a[0]*b[1]-a[1]*b[0];}return sum<0?R(-sum):sum;}
inline Polygon Intersection(Polygon p,const Polygon&clip){for(size_t j=0;j<clip.size()&&!p.empty();++j)p=MountSurfacePatch::Clip(p,clip[j],clip[(j+1)%clip.size()],true);return p;}
// A connected original sheet, consistently facing the trusted seed projection.
// Exact source-line edge overlaps support T junctions. No plane flattening,
// distance welding, arbitrary neighboring cluster or guessed angular epsilon.
inline std::vector<uint32_t> Connected(const std::vector<Triangle>&source,uint32_t seed,const PlanarMount::Frame&frame){
 struct Edge {uint32_t id;R lo,hi;};using Line=std::array<R,6>;std::map<Line,std::vector<Edge>>lines;std::map<uint32_t,std::set<uint32_t>>neighbors;
 for(const auto&t:source){const auto own=PlanarMount::Basis(t);if(OrientedMount::Dot(frame.normal,own.normal)<=0)continue;neighbors[t.id];for(unsigned j=0;j<3;++j){const auto a=PlanarMount::Vertex(t.vertices[j]),b=PlanarMount::Vertex(t.vertices[(j+1)%3]);auto d=PlanarMount::Minus(b,a);unsigned pivot=0;while(pivot<3&&d[pivot]==0)++pivot;if(pivot==3)return {};const R divisor=d[pivot];for(auto&v:d)v/=divisor;const auto m=OrientedMount::Cross(a,d);Line key{d[0],d[1],d[2],m[0],m[1],m[2]};R lo=a[pivot],hi=b[pivot];if(hi<lo)std::swap(lo,hi);lines[key].push_back({t.id,lo,hi});}}
 for(auto&entry:lines){auto&edges=entry.second;std::sort(edges.begin(),edges.end(),[](const Edge&a,const Edge&b){return a.lo!=b.lo?a.lo<b.lo:a.hi<b.hi;});if(edges.empty())continue;auto furthest=edges.front();for(size_t j=1;j<edges.size();++j){const auto&e=edges[j];if(e.lo<furthest.hi){neighbors[e.id].insert(furthest.id);neighbors[furthest.id].insert(e.id);if(e.hi>furthest.hi)furthest=e;}else furthest=e;}}
 if(!neighbors.count(seed))return {};std::set<uint32_t>done;std::vector<uint32_t>todo{seed};while(!todo.empty()){auto id=todo.back();todo.pop_back();if(!done.insert(id).second)continue;for(auto next:neighbors.at(id))if(!done.count(next))todo.push_back(next);}return {done.begin(),done.end()};
}
// Prove a finite one-to-one graph over the entire real oriented footprint.
// Every projected triangle is clipped exactly to that convex footprint. Pairwise
// positive-area overlap means a fold/duplicate/layer and is refused. With disjoint
// interiors, exact sum-of-clipped-areas==footprint-area proves finite closed union
// coverage; missing edges/holes cannot be filled by a hull or bounding rectangle.
inline Error ProjectedGraph(const std::vector<Triangle>&component,const PlanarMount::Frame&frame,const OrientedMount::Volume&body,uint32_t maximum,std::vector<uint32_t>&support){
 auto footprint=PlanarMount::Footprint(frame,body);if(footprint.empty()||!maximum)return Error::Invalid;std::vector<Polygon>parts;R area=0;bool curved=false;
 for(const auto&t:component){Polygon triangle;for(const auto&p:t.vertices)triangle.push_back(PlanarMount::Project(frame,PlanarMount::Vertex(p)));const R cross=MountSurfacePatch::Side(triangle[0],triangle[1],triangle[2]);if(cross==0)return Error::Invalid;if(cross<0)std::swap(triangle[1],triangle[2]);auto clipped=Intersection(std::move(triangle),footprint);const R measured=Area(clipped);if(measured==0)continue;if(parts.size()==maximum)return Error::TooLarge;
  for(const auto&prior:parts)if(Area(Intersection(clipped,prior))!=0)return Error::Fold;
  area+=measured;parts.push_back(std::move(clipped));support.push_back(t.id);if(!PlanarMount::SamePlane(frame,t))curved=true;
 }
 if(area!=Area(footprint))return Error::Coverage;return curved?Error::None:Error::NotCurved;
}
inline Proof Select(const std::vector<Source>&reviewed,const PlanarMount::SurfaceSeed&seed,Mode mode,const OrientedMount::Volume&body,Limits limits={}){
 Proof out;try{if(!OrientedMount::Valid(body)||(seed.side!=1&&seed.side!=-1)||(mode!=Mode::Wall&&mode!=Mode::Ceiling)||!std::isfinite(limits.search_margin)||limits.search_margin<=0||limits.search_margin>8||!limits.source||limits.source>65536||!limits.component||limits.component>4096||!limits.relevant||limits.relevant>128)return out;if(reviewed.size()>limits.source){out.error=Error::TooLarge;return out;}
  uint32_t previous=0;bool first=true;std::vector<Triangle>eligible;const Triangle*seed_triangle=nullptr;
  for(const auto&s:reviewed){if((!first&&s.geometry.id<=previous)||!s.side_mask||s.side_mask>3||!s.native_mode_mask||(s.native_mode_mask&~3)){out.error=Error::Source;return out;}first=false;previous=s.geometry.id;for(const auto&p:s.geometry.vertices)for(float f:p)if(!std::isfinite(f)){out.error=Error::Source;return out;}if(!(s.side_mask&(seed.side>0?1u:2u))||!(s.native_mode_mask&uint8_t(mode)))continue;eligible.push_back(s.geometry);if(s.geometry.id==seed.triangle_id)seed_triangle=&s.geometry;
  }
  if(!seed_triangle){out.error=Error::MissingSeed;return out;}const auto frame=PlanarMount::Basis(*seed_triangle);auto connected=Connected(eligible,seed.triangle_id,frame);if(connected.empty()){out.error=Error::Source;return out;}if(connected.size()>limits.component){out.error=Error::TooLarge;return out;}
  std::vector<Triangle>component;for(const auto&t:eligible)if(std::binary_search(connected.begin(),connected.end(),t.id))component.push_back(t);out.error=ProjectedGraph(component,frame,body,limits.relevant,out.support_ids);if(out.error!=Error::None)return out;
  // ONLY the exact seed-plane component may retain V5's collision exemption.
  // Noncoplanar support facets NEVER become exemptions. Every bend must be
  // physically clear of the full body AND the complete inward swept corridor.
  out.collision_exempt_ids=PlanarMount::Connected(component,seed.triangle_id);if(out.collision_exempt_ids.empty()){out.error=Error::Source;return out;}return out;
 }catch(...){out.error=Error::Invalid;return out;}}

}
#endif
