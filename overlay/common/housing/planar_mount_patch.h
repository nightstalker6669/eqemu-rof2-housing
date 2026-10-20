#ifndef EQEMU_HOUSING_PLANAR_MOUNT_PATCH_H
#define EQEMU_HOUSING_PLANAR_MOUNT_PATCH_H
#include "mount_surface_patch.h"
#include "planar_mount_contact.h"
namespace Housing::PlanarMount {
using namespace MountSurfacePatch;
using Vector=OrientedMount::ExactPoint;
struct Frame {Vector origin{},normal{},u{},v{};};
inline Vector Vertex(const Point&p){return {Exact(p[0]),Exact(p[1]),Exact(p[2])};}
inline Vector Minus(const Vector&a,const Vector&b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
inline Frame Basis(const Triangle&t){Frame f;f.origin=Vertex(t.vertices[0]);f.normal=OrientedMount::Cross(Minus(Vertex(t.vertices[1]),f.origin),Minus(Vertex(t.vertices[2]),f.origin));unsigned axis=0;for(unsigned j=1;j<3;++j)if(OrientedMount::Abs(f.normal[j])>OrientedMount::Abs(f.normal[axis]))axis=j;if(f.normal[axis]==0)throw std::invalid_argument("Degenerate mount seed");unsigned tangent[2],n=0;for(unsigned j=0;j<3;++j)if(j!=axis)tangent[n++]=j;f.u[tangent[0]]=f.normal[axis];f.u[axis]=-f.normal[tangent[0]];f.v[tangent[1]]=f.normal[axis];f.v[axis]=-f.normal[tangent[1]];return f;}
inline Point2 Project(const Frame&f,const Vector&p){const auto d=Minus(p,f.origin);return {OrientedMount::Dot(f.u,d),OrientedMount::Dot(f.v,d)};}
inline bool SamePlane(const Frame&f,const Triangle&t){for(const auto&p:t.vertices)if(OrientedMount::Dot(f.normal,Minus(Vertex(p),f.origin))!=0)return false;const auto origin=Vertex(t.vertices[0]);const auto normal=OrientedMount::Cross(Minus(Vertex(t.vertices[1]),origin),Minus(Vertex(t.vertices[2]),origin));return OrientedMount::Dot(f.normal,normal)>0;}
inline Polygon Footprint(const Frame&f,const OrientedMount::Volume&volume){if(!OrientedMount::Valid(volume))return {};const auto center=Project(f,OrientedMount::Exact(volume.center));std::array<Point2,7>generators;for(unsigned j=0;j<volume.count;++j){const auto g=OrientedMount::Exact(volume.generators[j]);generators[j]={OrientedMount::Dot(f.u,g),OrientedMount::Dot(f.v,g)};}Polygon points;
 for(unsigned mask=0;mask<(1u<<volume.count);++mask){auto p=center;for(unsigned j=0;j<volume.count;++j)for(unsigned k=0;k<2;++k)p[k]+=((mask>>j)&1)?generators[j][k]:-generators[j][k];points.push_back(std::move(p));}std::sort(points.begin(),points.end());points.erase(std::unique(points.begin(),points.end()),points.end());if(points.size()<3)return {};Polygon hull;for(const auto&p:points){while(hull.size()>1&&Side(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}const auto lower=hull.size();for(size_t j=points.size()-1;j-->0;){const auto&p=points[j];while(hull.size()>lower&&Side(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}hull.pop_back();return hull;
}
// Exact coplanarity, winding and original positive-length source-edge overlap.
// No normal epsilon, projected seam weld or noncoplanar bend is authorized.
inline std::vector<uint32_t> Connected(const std::vector<Triangle>&source,uint32_t seed){try{
 std::map<uint32_t,const Triangle*>ids;for(const auto&t:source)if(!ids.emplace(t.id,&t).second)return {};if(!ids.count(seed))return {};const auto frame=Basis(*ids.at(seed));
 struct Edge {uint32_t id;Rational lo,hi;};using Line=std::array<Rational,6>;std::map<Line,std::vector<Edge>>lines;std::map<uint32_t,std::set<uint32_t>>neighbors;
 for(const auto&entry:ids){const auto&t=*entry.second;if(!SamePlane(frame,t))continue;neighbors[t.id];for(unsigned k=0;k<3;++k){const auto a=Vertex(t.vertices[k]),b=Vertex(t.vertices[(k+1)%3]);auto direction=Minus(b,a);unsigned pivot=0;while(pivot<3&&direction[pivot]==0)++pivot;if(pivot==3)return {};const Rational denominator=direction[pivot];for(auto&v:direction)v/=denominator;const auto moment=OrientedMount::Cross(a,direction);Line key{direction[0],direction[1],direction[2],moment[0],moment[1],moment[2]};Rational lo=a[pivot],hi=b[pivot];if(hi<lo)std::swap(lo,hi);lines[key].push_back({t.id,lo,hi});}}
 for(auto&entry:lines){auto&edges=entry.second;std::sort(edges.begin(),edges.end(),[](const Edge&a,const Edge&b){return a.lo!=b.lo?a.lo<b.lo:a.hi<b.hi;});if(edges.empty())continue;auto furthest=edges.front();for(size_t j=1;j<edges.size();++j){const auto&e=edges[j];if(e.lo<furthest.hi){neighbors[e.id].insert(furthest.id);neighbors[furthest.id].insert(e.id);if(e.hi>furthest.hi)furthest=e;}else furthest=e;}}
 std::set<uint32_t>result;std::vector<uint32_t>todo{seed};while(!todo.empty()){auto id=todo.back();todo.pop_back();if(!result.insert(id).second)continue;for(auto next:neighbors.at(id))if(!result.count(next))todo.push_back(next);}return {result.begin(),result.end()};
 }catch(...){return {};}}
inline bool Covered(const std::vector<Triangle>&source,const std::vector<uint32_t>&connected,uint32_t seed,const OrientedMount::Volume&volume){try{
 if(connected.empty()||!std::is_sorted(connected.begin(),connected.end())||std::adjacent_find(connected.begin(),connected.end())!=connected.end()||!std::binary_search(connected.begin(),connected.end(),seed))return false;const Triangle*start=nullptr;for(const auto&t:source)if(t.id==seed)start=&t;if(!start)return false;const auto frame=Basis(*start);auto footprint=Footprint(frame,volume);if(footprint.empty())return false;std::vector<Polygon>remaining{footprint};
 for(const auto&t:source){if(!std::binary_search(connected.begin(),connected.end(),t.id))continue;if(!SamePlane(frame,t))return false;std::array<Point2,3>points;for(unsigned j=0;j<3;++j)points[j]=Project(frame,Vertex(t.vertices[j]));const Rational cross=Side(points[0],points[1],points[2]);if(cross==0)return false;if(cross<0)std::swap(points[1],points[2]);std::vector<Polygon>next;for(auto polygon:remaining){for(unsigned edge=0;edge<3&&!polygon.empty();++edge){auto outside=Clip(polygon,points[edge],points[(edge+1)%3],false);if(outside.size()>=3&&Area(outside))next.push_back(std::move(outside));polygon=Clip(polygon,points[edge],points[(edge+1)%3],true);}}remaining=std::move(next);if(remaining.empty())return true;}return false;
 }catch(...){return false;}}
inline std::vector<uint32_t> Select(const std::vector<Triangle>&source,uint32_t seed,const OrientedMount::Volume&volume){auto connected=Connected(source,seed);return !connected.empty()&&Covered(source,connected,seed,volume)?connected:std::vector<uint32_t>{};}
inline std::vector<uint32_t> Select(const std::vector<Triangle>&source,const SurfaceSeed&seed,const OrientedMount::Volume&volume){if(seed.side!=1&&seed.side!=-1)return {};return Select(source,seed.triangle_id,volume);}
}
#endif
