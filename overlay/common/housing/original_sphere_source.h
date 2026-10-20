#ifndef EQEMU_HOUSING_ORIGINAL_SPHERE_SOURCE_H
#define EQEMU_HOUSING_ORIGINAL_SPHERE_SOURCE_H
#include "original_sphere_query.h"
#include <algorithm>
#include <set>
#include <span>
namespace Housing::OriginalSphereSource {
// Every point and normal is native Y,X,Z, including transformed actor triangles.
// IDs belong to the authenticated source provider; zero is a valid source ID.
// This adapter NEVER decides actor/terrain flags, one-sidedness, mode or ACL.
using Point=OriginalSphereQuery::Point;
struct Triangle {uint64_t id{};std::array<Point,3> vertices{};};
struct Bounds {std::array<double,3> minimum{},maximum{};};
struct Limits {uint32_t sources=16384,narrowphase=2048;uint64_t operations=16000000;};
enum class Status {Hit,Clear,InvalidInput,InvalidSource,BudgetExceeded,Ambiguous,Environment};
struct Result {
 Status status=Status::InvalidInput;uint64_t source_id{};OriginalSphereQuery::Result contact{};
 uint32_t sources{},narrowphase{};uint64_t operations{};
 explicit operator bool()const noexcept{return status==Status::Hit;}
};
inline bool Finite(const Point& p){for(float x:p)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;return true;}
inline bool Parameters(const Point& start,const Point& end,float radius,float maximum){return Finite(start)&&Finite(end)&&start!=end&&std::isfinite(radius)&&radius>0&&radius<=1e6f&&std::isfinite(maximum)&&maximum>=0&&maximum<=1;}
// Conservative source-provider broadphase, not collision/contact authority.
inline std::optional<Bounds> SweptBounds(const Point& start,const Point& end,float radius){
 if(!Parameters(start,end,radius,1))return {};OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};
 Bounds b;for(unsigned k=0;k<3;++k){b.minimum[k]=std::nextafter(OriginalSphereQuery::Detail::difference(std::min(start[k],end[k]),radius),-std::numeric_limits<double>::infinity());b.maximum[k]=std::nextafter(OriginalSphereQuery::Detail::sum(std::max(start[k],end[k]),radius),std::numeric_limits<double>::infinity());}return b;
}
inline bool Intersects(const Bounds& b,const Triangle& t){for(unsigned k=0;k<3;++k){const auto lo=std::min({t.vertices[0][k],t.vertices[1][k],t.vertices[2][k]}),hi=std::max({t.vertices[0][k],t.vertices[1][k],t.vertices[2][k]});if(double(hi)<b.minimum[k]||double(lo)>b.maximum[k])return false;}return true;}
inline bool Bits(const Point& a,const Point& b){for(unsigned k=0;k<3;++k)if(std::bit_cast<uint32_t>(a[k])!=std::bit_cast<uint32_t>(b[k]))return false;return true;}
inline Result Sweep(std::span<const Triangle> selected,const Point& start,const Point& end,float radius,float maximum=1,Limits limits={}){
 Result out;if(!Parameters(start,end,radius,maximum)||!limits.sources||limits.sources>65536||!limits.narrowphase||limits.narrowphase>8192||!limits.operations||limits.operations>64000000)return out;
 if(selected.size()>limits.sources){out.status=Status::BudgetExceeded;return out;}
 OriginalSphereQuery::Detail::Rounding env;if(!env.good){out.status=Status::Environment;return out;}
 const auto bounds=SweptBounds(start,end,radius);if(!bounds){out.status=Status::Environment;return out;}
 try{std::set<uint64_t> ids;bool found=false,ambiguous=false;float best=maximum;
 for(const auto& source:selected){
  ++out.sources;if(!ids.insert(source.id).second){out.status=Status::InvalidSource;return out;}for(const auto& p:source.vertices)if(!Finite(p)){out.status=Status::InvalidSource;return out;}
  if(!Intersects(*bounds,source))continue;
  if(out.narrowphase==limits.narrowphase||limits.operations-out.operations<48000){out.status=Status::BudgetExceeded;return out;}++out.narrowphase;
  auto hit=OriginalSphereQuery::Triangle(source.vertices,start,end,radius,best);
  if(!hit){out.status=Status::InvalidSource;return out;}out.operations+=hit->steps;
  if(out.operations>limits.operations){out.status=Status::BudgetExceeded;return out;}
  if(!hit->hit)continue;
  if(!found||hit->fraction<best){found=true;ambiguous=false;best=hit->fraction;out.source_id=source.id;out.contact=*hit;}
  else if(hit->fraction==best){
   // Original source enumeration is external. Never invent its tie ordering.
   // Equivalent contact witnesses can use a deterministic ID without changing
   // contact; incompatible normals/feature/point require stronger provenance.
   if(hit->kind!=out.contact.kind||!Bits(hit->normal,out.contact.normal)||!Bits(hit->point,out.contact.point)||!Bits(hit->stopped,out.contact.stopped))ambiguous=true;
   else if(source.id<out.source_id){out.source_id=source.id;out.contact=*hit;}
  }
 }
 out.status=ambiguous?Status::Ambiguous:found?Status::Hit:Status::Clear;return out;
 }catch(...){out.status=Status::InvalidSource;return out;}
}
}
#endif
