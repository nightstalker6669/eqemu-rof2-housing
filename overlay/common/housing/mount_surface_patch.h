#ifndef EQEMU_HOUSING_MOUNT_SURFACE_PATCH_H
#define EQEMU_HOUSING_MOUNT_SURFACE_PATCH_H
#include <boost/multiprecision/cpp_int.hpp>
#include "oriented_mount_volume.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
namespace Housing::MountSurfacePatch {
// Exact original structural triangles only. The caller supplies the immutable
// policy's allowed IDs against the already hash-verified loaded source map.
using Point=std::array<float,3>;
struct Triangle {uint32_t id;std::array<Point,3> vertices;};
using Rational=boost::multiprecision::cpp_rational;
using Integer=boost::multiprecision::cpp_int;
using Point2=std::array<Rational,2>;
using Polygon=std::vector<Point2>;
inline Rational Exact(float f){
 uint32_t bits;std::memcpy(&bits,&f,4);const uint32_t exponent=(bits>>23)&255,mantissa=bits&0x7fffff;
 if(exponent==255)throw std::invalid_argument("Nonfinite mount patch coordinate");
 Integer n=exponent?mantissa|0x800000:mantissa;int shift=exponent?int(exponent)-150:-149;
 if(bits>>31)n=-n;if(shift>=0)return Rational(n<<shift);return Rational(n)/Rational(Integer(1)<<-shift);
}
inline Rational Side(const Point2&a,const Point2&b,const Point2&p){return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0]);}
inline bool Area(const Polygon&p){Rational area=0;for(size_t i=0;i<p.size();++i){const auto&a=p[i];const auto&b=p[(i+1)%p.size()];area+=a[0]*b[1]-a[1]*b[0];}return area!=0;}
inline Polygon Clip(const Polygon& polygon,const Point2&a,const Point2&b,bool inside){
 Polygon out;if(polygon.empty())return out;auto previous=polygon.back();Rational before=Side(a,b,previous);bool previous_kept=inside?before>=0:before<=0;
 for(const auto&current:polygon){Rational after=Side(a,b,current);bool kept=inside?after>=0:after<=0;
  if(kept!=previous_kept){const Rational t=before/(before-after);out.push_back({previous[0]+t*(current[0]-previous[0]),previous[1]+t*(current[1]-previous[1])});}
  if(kept)out.push_back(current);previous=current;before=after;previous_kept=kept;
 }return out;
}
// Subtract each closed source triangle from the footprint rectangle using exact
// rational half-planes. No epsilon, bounding-rectangle fill, welded seam, hull or
// sampling. Any positive-area remainder is a real uncovered gap/hole. A finite
// closed triangle union containing the rectangle interior contains its boundary.
inline bool Covered(const std::vector<const Triangle*>& triangles,unsigned axis,const Point&minimum,const Point&maximum){
 unsigned tangent[2],count=0;for(unsigned k=0;k<3;++k)if(k!=axis)tangent[count++]=k;
 const Rational x0=Exact(minimum[tangent[0]]),x1=Exact(maximum[tangent[0]]),y0=Exact(minimum[tangent[1]]),y1=Exact(maximum[tangent[1]]);
 if(x0>=x1||y0>=y1)return false;
 const Rational rectangle_area=(x1-x0)*(y1-y0);Rational source_area=0;
 std::vector<std::array<Point2,3>> relevant;
 for(const auto*triangle:triangles){
  // Exact binary32 bounding comparison discards only zero-area/outside pieces.
  bool outside=false;for(auto k:tangent){float lo=INFINITY,hi=-INFINITY;for(const auto&v:triangle->vertices){lo=std::min(lo,v[k]);hi=std::max(hi,v[k]);}if(hi<=minimum[k]||lo>=maximum[k])outside=true;}if(outside)continue;
  std::array<Point2,3> points;for(unsigned j=0;j<3;++j)points[j]={Exact(triangle->vertices[j][tangent[0]]),Exact(triangle->vertices[j][tangent[1]])};
  auto cross=Side(points[0],points[1],points[2]);if(cross==0)continue;if(cross<0)std::swap(points[1],points[2]);
  Polygon clipped(points.begin(),points.end());const std::array<Point2,4> boundary{{{x0,y0},{x1,y0},{x1,y1},{x0,y1}}};
  for(unsigned edge=0;edge<4&&!clipped.empty();++edge)clipped=Clip(clipped,boundary[edge],boundary[(edge+1)%4],true);
  Rational area=0;for(size_t j=0;j<clipped.size();++j){const auto&v=clipped[j];const auto&w=clipped[(j+1)%clipped.size()];area+=v[0]*w[1]-v[1]*w[0];}
  if(area<0)area=-area;if(area==0)continue;source_area+=area/2;relevant.push_back(points);
 }
 // Sum of clipped triangle areas is an UPPER bound on union coverage, even
 // with duplicate/overlapping faces. This is an exact necessary condition,
 // not a size cap, sampling shortcut or permission widening.
 if(source_area<rectangle_area)return false;
 std::vector<Polygon> remainder{{{x0,y0},{x1,y0},{x1,y1},{x0,y1}}};
 for(const auto&points:relevant){
  std::vector<Polygon> next;
  for(auto polygon:remainder){
   for(unsigned edge=0;edge<3&&!polygon.empty();++edge){auto outside=Clip(polygon,points[edge],points[(edge+1)%3],false);if(outside.size()>=3&&Area(outside))next.push_back(std::move(outside));polygon=Clip(polygon,points[edge],points[(edge+1)%3],true);}
  }
  remainder=std::move(next);if(remainder.empty())return true;
 }
 return false;
}
// V4 oriented footprint: exact convex projection including trusted error and
// sweep generators; old rectangular V3 Covered/Select APIs remain unchanged.
inline bool Covered(const std::vector<const Triangle*>& triangles,unsigned axis,const OrientedMount::Volume&volume){
 try{auto footprint=OrientedMount::Project(volume,axis);if(footprint.empty())return false;unsigned tangent[2],n=0;for(unsigned i=0;i<3;++i)if(i!=axis)tangent[n++]=i;
  const auto area=[](const Polygon&p){Rational sum=0;for(size_t j=0;j<p.size();++j){const auto&a=p[j];const auto&b=p[(j+1)%p.size()];sum+=a[0]*b[1]-a[1]*b[0];}return sum<0?Rational(-sum):sum;};
  const Rational footprint_area=area(footprint);Rational source_area=0;std::vector<std::array<Point2,3>>relevant;
  for(const auto*triangle:triangles){if(!triangle)return false;std::array<Point2,3>points;for(unsigned j=0;j<3;++j)points[j]={Exact(triangle->vertices[j][tangent[0]]),Exact(triangle->vertices[j][tangent[1]])};const Rational cross=Side(points[0],points[1],points[2]);if(cross==0)continue;if(cross<0)std::swap(points[1],points[2]);
   Polygon clipped(points.begin(),points.end());for(size_t edge=0;edge<footprint.size()&&!clipped.empty();++edge)clipped=Clip(clipped,footprint[edge],footprint[(edge+1)%footprint.size()],true);const Rational covered_area=area(clipped);if(covered_area==0)continue;source_area+=covered_area;relevant.push_back(points);
  }
  // An exact necessary bound only: overlaps may count twice here. Actual union
  // subtraction below still proves every positive-area footprint piece covered.
  if(source_area<footprint_area)return false;
  std::vector<Polygon> remainder{footprint};for(const auto&points:relevant){
   std::vector<Polygon>next;for(auto polygon:remainder){for(unsigned edge=0;edge<3&&!polygon.empty();++edge){auto outside=Clip(polygon,points[edge],points[(edge+1)%3],false);if(outside.size()>=3&&Area(outside))next.push_back(std::move(outside));polygon=Clip(polygon,points[edge],points[(edge+1)%3],true);}}remainder=std::move(next);if(remainder.empty())return true;
  }return false;
 }catch(...){return false;}
}

inline bool OrientedAxisFace(const Triangle&t,unsigned axis,int sign){
 std::array<Rational,3>a,b,n;for(unsigned k=0;k<3;++k){a[k]=Exact(t.vertices[1][k])-Exact(t.vertices[0][k]);b[k]=Exact(t.vertices[2][k])-Exact(t.vertices[0][k]);}
 n={a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
 if(n[axis]*sign<=0)return false;Rational norm=0;for(const auto&v:n)norm+=v*v;
 // Same .999999 axis-normal criterion as the existing native-contact adapter,
 // evaluated exactly over binary source coordinates rather than rounded normals.
 return n[axis]*n[axis]*Integer(1000000000000ULL)>=norm*Integer(999998000001ULL);
}
inline std::vector<uint32_t> Connected(const std::vector<Triangle>& source,unsigned axis,uint32_t seed_a,uint32_t seed_b){
 try{
  if(axis>2||seed_a==seed_b)return {};
  std::map<uint32_t,const Triangle*> ids;for(const auto&t:source){if(!ids.emplace(t.id,&t).second)return {};for(const auto&v:t.vertices)for(float f:v)if(!std::isfinite(f))return {};}
  if(!ids.count(seed_a)||!ids.count(seed_b))return {};
  float low=INFINITY,high=-INFINITY;for(auto seed:{seed_a,seed_b})for(const auto&v:ids.at(seed)->vertices){low=std::min(low,v[axis]);high=std::max(high,v[axis]);}
  if(high-low>.001f)return {};int sign=OrientedAxisFace(*ids.at(seed_a),axis,1)?1:OrientedAxisFace(*ids.at(seed_a),axis,-1)?-1:0;if(!sign)return {};
  // Connect exact positive-length collinear source-edge overlaps, including
  // T-junction subdivisions. A shared endpoint alone does not join a patch.
  // Exact rational line keys never weld a near seam or bridge a real crack.
  struct EdgeUse {uint32_t id;Rational low,high;};
  using Line=std::array<Rational,6>;std::map<Line,std::vector<EdgeUse>> lines;
  std::map<uint32_t,std::set<uint32_t>> neighbors;
  for(const auto&entry:ids){const auto&t=*entry.second;bool within=true;for(const auto&v:t.vertices)if(v[axis]<low||v[axis]>high)within=false;if(!within||!OrientedAxisFace(t,axis,sign))continue;
   neighbors[t.id];
   for(unsigned k=0;k<3;++k){const auto&a=t.vertices[k];const auto&b=t.vertices[(k+1)%3];std::array<Rational,3> origin,direction;
    unsigned pivot=3;for(unsigned j=0;j<3;++j){origin[j]=Exact(a[j]);direction[j]=Exact(b[j])-origin[j];if(pivot==3&&direction[j]!=0)pivot=j;}if(pivot==3)return {};
    const Rational denominator=direction[pivot];for(auto&v:direction)v/=denominator;
    Line key{direction[0],direction[1],direction[2],origin[1]*direction[2]-origin[2]*direction[1],origin[2]*direction[0]-origin[0]*direction[2],origin[0]*direction[1]-origin[1]*direction[0]};
    Rational first=origin[pivot],last=Exact(b[pivot]);if(last<first)std::swap(first,last);lines[key].push_back({t.id,first,last});
   }
  }
  if(!neighbors.count(seed_a)||!neighbors.count(seed_b))return {};
  for(auto&entry:lines){auto&edges=entry.second;std::sort(edges.begin(),edges.end(),[](const EdgeUse&a,const EdgeUse&b){return a.low!=b.low?a.low<b.low:a.high<b.high;});
   if(edges.empty())continue;auto furthest=edges.front();
   for(size_t i=1;i<edges.size();++i){const auto&edge=edges[i];if(edge.low<furthest.high){neighbors[edge.id].insert(furthest.id);neighbors[furthest.id].insert(edge.id);if(edge.high>furthest.high)furthest=edge;}else furthest=edge;}
  }
  std::set<uint32_t> connected;std::vector<uint32_t> todo{seed_a};while(!todo.empty()){auto id=todo.back();todo.pop_back();if(!connected.insert(id).second)continue;for(auto neighbor:neighbors.at(id))if(!connected.count(neighbor))todo.push_back(neighbor);}
  if(!connected.count(seed_b))return {};return {connected.begin(),connected.end()};
 }catch(...){return {};}
}
inline std::vector<uint32_t> Select(const std::vector<Triangle>& source,unsigned axis,uint32_t seed_a,uint32_t seed_b,const Point&minimum,const Point&maximum){
 try{for(unsigned k=0;k<3;++k)if(!std::isfinite(minimum[k])||!std::isfinite(maximum[k])||minimum[k]>=maximum[k])return {};
 auto connected=Connected(source,axis,seed_a,seed_b);if(connected.empty())return {};std::vector<const Triangle*> patch;for(const auto&t:source)if(std::binary_search(connected.begin(),connected.end(),t.id))patch.push_back(&t);return Covered(patch,axis,minimum,maximum)?connected:std::vector<uint32_t>{};
 }catch(...){return {};}
}

}
#endif
