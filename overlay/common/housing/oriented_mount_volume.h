#ifndef EQEMU_HOUSING_ORIENTED_MOUNT_VOLUME_H
#define EQEMU_HOUSING_ORIENTED_MOUNT_VOLUME_H
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
namespace Housing::OrientedMount {
using Rational=boost::multiprecision::cpp_rational;
using Integer=boost::multiprecision::cpp_int;
using Point=std::array<double,3>;
// Server-built zonotope: three independent model half-edges, up to three
// world-axis uncertainty generators and an optional complete corridor sweep.
struct Volume {Point center{};std::array<Point,7> generators{};uint32_t count=0;};
using ExactPoint=std::array<Rational,3>;
using Point2=std::array<Rational,2>;
using Polygon=std::vector<Point2>;
inline Rational Exact(double f){
 uint64_t bits;std::memcpy(&bits,&f,8);const uint64_t exponent=(bits>>52)&2047,mantissa=bits&0xfffffffffffffULL;
 if(exponent==2047)throw std::invalid_argument("Nonfinite oriented mount coordinate");
 Integer n=exponent?mantissa|0x10000000000000ULL:mantissa;const int shift=exponent?int(exponent)-1075:-1074;
 if(bits>>63)n=-n;if(shift>=0)return Rational(n<<shift);return Rational(n)/Rational(Integer(1)<<-shift);
}
inline ExactPoint Exact(const Point&p){return {Exact(p[0]),Exact(p[1]),Exact(p[2])};}
inline ExactPoint Cross(const ExactPoint&a,const ExactPoint&b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
inline Rational Dot(const ExactPoint&a,const ExactPoint&b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline Rational Abs(const Rational&v){return v<0?-v:v;}
inline bool Valid(const Volume&v){
 try{if(v.count<3||v.count>7)return false;for(double f:v.center)if(!std::isfinite(f))return false;
  for(uint32_t i=0;i<v.count;++i)for(double f:v.generators[i])if(!std::isfinite(f))return false;
  return Dot(Cross(Exact(v.generators[0]),Exact(v.generators[1])),Exact(v.generators[2]))!=0;
 }catch(...){return false;}
}
inline std::pair<std::array<float,3>,std::array<float,3>> Bounds(const Volume&v){
 if(!Valid(v))throw std::invalid_argument("Invalid oriented mount volume");
 std::array<float,3>lo{},hi{};for(unsigned axis=0;axis<3;++axis){Rational radius=0;for(uint32_t i=0;i<v.count;++i)radius+=Abs(Exact(v.generators[i][axis]));const Rational a=Exact(v.center[axis])-radius,b=Exact(v.center[axis])+radius;
  lo[axis]=a.convert_to<float>();hi[axis]=b.convert_to<float>();if(!std::isfinite(lo[axis])||!std::isfinite(hi[axis]))throw std::invalid_argument("Unbounded oriented mount volume");
  while(Exact(double(lo[axis]))>a)lo[axis]=std::nextafter(lo[axis],-INFINITY);while(Exact(double(hi[axis]))<b)hi[axis]=std::nextafter(hi[axis],INFINITY);
 }return {lo,hi};
}
inline Rational Side(const Point2&a,const Point2&b,const Point2&p){return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0]);}
inline Polygon Project(const Volume&v,unsigned axis){
 if(axis>2||!Valid(v))return {};unsigned tangent[2],n=0;for(unsigned i=0;i<3;++i)if(i!=axis)tangent[n++]=i;
 std::array<Point2,7>g;Point2 c{Exact(v.center[tangent[0]]),Exact(v.center[tangent[1]])};for(uint32_t i=0;i<v.count;++i)g[i]={Exact(v.generators[i][tangent[0]]),Exact(v.generators[i][tangent[1]])};
 Polygon points;for(uint32_t mask=0;mask<(1u<<v.count);++mask){auto p=c;for(uint32_t i=0;i<v.count;++i)for(unsigned k=0;k<2;++k)p[k]+=((mask>>i)&1)?g[i][k]:-g[i][k];points.push_back(std::move(p));}
 std::sort(points.begin(),points.end());points.erase(std::unique(points.begin(),points.end()),points.end());if(points.size()<3)return {};
 Polygon hull;for(const auto&p:points){while(hull.size()>1&&Side(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}const auto lower=hull.size();
 for(size_t i=points.size()-1;i-->0;){const auto&p=points[i];while(hull.size()>lower&&Side(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}hull.pop_back();return hull.size()>=3?hull:Polygon{};
}
// Exact separating-axis theorem for a triangle and a full-dimensional zonotope.
// Axes: triangle normal, every zonotope generator-pair normal, and every
// triangle-edge/generator cross product. Equality is collision; no gap epsilon.
inline bool TriangleOverlap(const Volume&v,const std::array<std::array<float,3>,3>&triangle){
 try{if(!Valid(v))return true;const auto c=Exact(v.center);std::array<ExactPoint,7>g;for(uint32_t i=0;i<v.count;++i)g[i]=Exact(v.generators[i]);std::array<ExactPoint,3>p,e;
  for(unsigned i=0;i<3;++i)for(unsigned k=0;k<3;++k)p[i][k]=Exact(double(triangle[i][k]))-c[k];for(unsigned i=0;i<3;++i)for(unsigned k=0;k<3;++k)e[i][k]=p[(i+1)%3][k]-p[i][k];
  const auto separated=[&](const ExactPoint&axis){if(axis==ExactPoint{})return false;Rational radius=0;for(uint32_t i=0;i<v.count;++i)radius+=Abs(Dot(axis,g[i]));Rational lo=Dot(axis,p[0]),hi=lo;for(unsigned i=1;i<3;++i){const Rational q=Dot(axis,p[i]);lo=std::min(lo,q);hi=std::max(hi,q);}return lo>radius||hi<-radius;};
  if(separated(Cross(e[0],e[1])))return false;for(uint32_t i=0;i<v.count;++i){for(uint32_t j=0;j<i;++j)if(separated(Cross(g[i],g[j])))return false;for(const auto&edge:e)if(separated(Cross(g[i],edge)))return false;}return true;
 }catch(...){return true;}
}
}
#endif
