#ifndef EQEMU_HOUSING_ORIGINAL_SIMPLE_LINEAR_QUERY_H
#define EQEMU_HOUSING_ORIGINAL_SIMPLE_LINEAR_QUERY_H
#include "original_simple_tree_query.h"
namespace Housing::OriginalSimpleLinearQuery {
using Point=OriginalSphereQuery::Point;
using LocalMesh=OriginalHeightScene::LocalMesh;
using Cache=OriginalSimpleTreeQuery::Cache;
// A reviewed source adapter supplies these ORIGINAL definition/context fields.
// No peer packet or polygon-count guess authenticates class/dispatch/context.
struct Definition {bool enabled{},disabled{};uint32_t kind{},polygon_count{};bool source64_present{};};
struct Context {bool reviewed{},backface_cull{},reject_zero_fraction{};};
struct Source {std::shared_ptr<const LocalMesh> mesh;std::vector<Cache> descriptors;Definition definition;};
inline bool ValidDefinition(const Definition& d){return d.enabled&&!d.disabled&&d.kind!=4&&d.polygon_count>0&&d.polygon_count<=200&&!d.source64_present;}
//100c53a5/af creates source64 only above200 triangles.100c5c83 then selects
// genuine linear fallback when source64 is absent. Restrict to the reviewed
// ordinary rawflags0 class, and retain invalid-normal descriptor omission.
inline std::optional<Source> Prepare(std::shared_ptr<const LocalMesh> mesh,const Definition& d){
 if(!mesh||!ValidDefinition(d)||mesh->polygons.size()!=d.polygon_count)return {};
 try {Source out{mesh,{},d};out.descriptors.reserve(d.polygon_count);
 for(unsigned k=0;k<d.polygon_count;++k){const auto&p=mesh->polygons[k];if(p.index!=k||p.flags)return {};auto c=OriginalSimpleTreeQuery::ModelCache(p.native_abc);if(!c)return {};out.descriptors.push_back(*c);}return out;
 }catch(...){return {};}
}
enum class Status {Ready,Invalid,Unknown,BudgetExceeded};
struct Enumeration {Status status=Status::Invalid;std::vector<uint32_t> polygon_ids;uint32_t visited{},invalid_descriptors{},backfaces{},distance_pruned{};};
// Conservative independent max1 source enumeration, preserving original
// descending linear order. It does not reproduce earlier-hit clipping or
// callback side effects, and does not pretend this is accelerated source64.
inline Enumeration Enumerate(const Source& source,const Context& context,const Point& start,const Point& delta,float radius,uint32_t budget=200){
 Enumeration out;if(!context.reviewed||!source.mesh||!ValidDefinition(source.definition)||source.mesh->polygons.size()!=source.definition.polygon_count||source.descriptors.size()!=source.definition.polygon_count||!budget||budget>200)return out;
 OriginalActorLocalQuery::Detail::Environment env;if(!env.good){out.status=Status::Unknown;return out;}
 auto q=OriginalSimpleTreeQuery::MakeQuery(start,delta,radius);if(!q){out.status=Status::Unknown;return out;}
 try{for(uint32_t n=source.definition.polygon_count;n>0;--n){if(out.visited==budget){out.status=Status::BudgetExceeded;return out;}++out.visited;const auto id=n-1;const auto&p=source.mesh->polygons[id];const auto&c=source.descriptors[id];if(p.index!=id||p.flags)return out;
  if(!c.valid){++out.invalid_descriptors;continue;}
  if(context.backface_cull){auto front=OriginalSimpleTreeQuery::Front(*q,c.normal);if(!front){out.status=Status::Unknown;return out;}if(!*front){++out.backfaces;continue;}}
  auto near=OriginalSimpleTreeQuery::DistanceMayReach(*q,p.native_abc[0],c.radius,1);if(!near){out.status=Status::Unknown;return out;}if(!*near){++out.distance_pruned;continue;}out.polygon_ids.push_back(id);
 }out.status=Status::Ready;return out;}catch(...){return out;}
}
enum class ContactStatus {Hit,Clear,Unknown};
struct Contact {ContactStatus status=ContactStatus::Unknown;float fraction{};Point cached_normal{};uint32_t kind{},steps{};};
//100bf860 forwards exactly descriptor/zero/sphere/delta/time/max/kind/point
// to100d5880 (already instruction-equivalent to the recovered7a6770 kernel).
// The LINEAR caller100c5eb6..ec8 publishes the descriptor cached normal even
// on edge/vertex contacts; tree callback normal aggregation is not substituted.
inline Contact Triangle(const Source& source,uint32_t id,const Context& context,const Point& start,const Point& delta,float radius,float maximum=1){
 Contact out;if(!context.reviewed||!source.mesh||!ValidDefinition(source.definition)||id>=source.mesh->polygons.size()||id>=source.descriptors.size())return out;
 const auto&p=source.mesh->polygons[id];const auto&c=source.descriptors[id];if(p.index!=id||p.flags)return out;
 if(!c.valid){out.status=ContactStatus::Clear;return out;}
 if(!OriginalSphereSource::Finite(start)||!OriginalSphereSource::Finite(delta)||delta==Point{}||!std::isfinite(radius)||radius<=0||radius>1e6f||!std::isfinite(maximum)||maximum<0||maximum>1)return out;
 if(context.backface_cull){auto q=OriginalSimpleTreeQuery::MakeQuery(start,delta,radius);if(!q)return out;auto front=OriginalSimpleTreeQuery::Front(*q,c.normal);if(!front)return out;if(!*front){out.status=ContactStatus::Clear;return out;}}
 OriginalSphereQuery::Detail::Rounding env;if(!env.good)return out;
 try{OriginalSphereQuery::Detail::Math m;constexpr uint32_t desc=0x10000,zero=0x11000,sphere=0x12000,motion=0x13000,time=0x14000,kind=0x15000,point=0x16000;
 constexpr std::array<uint32_t,3> vertices{0x10100,0x10110,0x10120};for(unsigned j=0;j<3;++j){m.put(desc+0x10+4*j,vertices[j]);for(unsigned k=0;k<3;++k)m.fpwrite(vertices[j]+4*k,p.native_abc[j][k]);}
 for(unsigned k=0;k<3;++k){m.fpwrite(desc+4*k,c.normal[k]);m.fpwrite(zero+4*k,0);m.fpwrite(sphere+4+4*k,start[k]);m.fpwrite(motion+4*k,delta[k]);}
 m.fpwrite(sphere,radius);m.fpwrite(time,maximum);m.put(kind,0);for(unsigned k=0;k<9;++k)m.fpwrite(point+4*k,0);
 m.put(m.r[7],0xeeeeeeee);const std::array<uint32_t,8>args{desc,zero,sphere,motion,time,std::bit_cast<uint32_t>(maximum),kind,point};for(unsigned j=0;j<8;++j)m.put(m.r[7]+4+4*j,args[j]);m.run(0x7a6770);out.steps=m.steps;
 if(!(m.r[0]&255)){out.status=ContactStatus::Clear;return out;}
 out.fraction=float(m.fp(time));out.kind=m.get(kind,4);if(!std::isfinite(out.fraction)||out.fraction<0||out.fraction>maximum)return out;
 if(context.reject_zero_fraction&&out.fraction==0){out.status=ContactStatus::Clear;return out;}
 out.cached_normal=c.normal;out.status=ContactStatus::Hit;return out;
 }catch(...){return out;}
}
}
#endif
