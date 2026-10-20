#ifndef EQEMU_HOUSING_MOUNT_ADMISSION_H
#define EQEMU_HOUSING_MOUNT_ADMISSION_H
#include "placement_contact.h"
#include "placement_policy.h"
#include "mount_flags_db.h"
#include <functional>
namespace Housing::MountAdmission {
using Trace=std::function<std::optional<PlacementContact::Contact>(const PlacementContact::Vector&,const PlacementContact::Vector&)>;
// Original557704..55772e/5043e0: radius uses the unrotated horizontal
// renderer extents and DEFAULT actor heading, not submitted current heading.
// Current FurnitureActor emits default_heading0. A future actor default must be
// passed explicitly; do not replace this selector with a pose-facing guess.
inline std::optional<float> WallRadius(const PlacementPolicy::Geometry& g,float scale,float default_heading){
 if(!std::isfinite(scale)||scale<=0||!std::isfinite(default_heading)||default_heading<0||default_heading>512)return {};
 if(!std::all_of(g.center.begin(),g.center.end(),[](double v){return std::isfinite(v);})||!std::isfinite(g.extents[0])||!std::isfinite(g.extents[1])||g.extents[0]<=0||g.extents[1]<=0)return {};
 bool y=g.extents[1]<g.extents[0];const auto quadrant=(uint32_t(default_heading)+64)/128;
 if(quadrant&1)y=!y;const float radius=float(g.extents[y?1:0]*scale);if(!std::isfinite(radius)||radius<=0)return {};return radius;
}
// Necessary/existential planar-face witness for the native radius query and
//7a2680's exact.001 travel pullback. The commit contains no camera ray.
// A second actual ray along the trusted face normal proves orthogonal face
// contact: an oblique axis ray alone can hit a plane beyond its triangle edge.
// Legacy actor-position anchor, corroborated for the zero-centre painting.
// Source55869c..6bf actually forms Q=P+rotated/scaled C before557630;
// centre-aware nonzero-C admission needs a separately versioned successor.
// This legacy witness does not emulate swept query edges/curves.
inline bool WallPlane(const PlacementPolicy::Geometry& g,const PlacementPolicy::Transform& p,const PlacementContact::Profile& profile,float tolerance,float default_heading,const Trace& trace){
 const auto radius=WallRadius(g,p.scale,default_heading);if(!radius||!trace||!std::isfinite(tolerance)||tolerance<=0||!PlacementContact::ModeAllowed(profile,PlacementContact::Mode::Wall))return false;
 constexpr float pullback=0.0010000000474974513f;const PlacementContact::Vector centre{p.position[1],p.position[0],p.position[2]};
 for(unsigned axis=0;axis<2;++axis)for(float sign:{-1.f,1.f}){
  const float length=2*(*radius+pullback+4*tolerance);auto end=centre;end[axis]+=sign*length;auto candidate=trace(centre,end);
  if(!candidate||!PlacementContact::NormalAllowed(profile,PlacementContact::Mode::Wall,candidate->normal)||std::abs(PlacementContact::Dot(candidate->normal,candidate->normal)-1)>1e-5f)continue;
  for(unsigned i=0;i<3;++i)end[i]=centre[i]-candidate->normal[i]*length;
  auto hit=trace(centre,end);if(!hit||!PlacementContact::NormalAllowed(profile,PlacementContact::Mode::Wall,hit->normal)||PlacementContact::Dot(candidate->normal,hit->normal)<1-1e-6f)continue;
  PlacementContact::Vector separation{};for(unsigned i=0;i<3;++i)separation[i]=centre[i]-hit->point[i];const float clearance=PlacementContact::Dot(separation,hit->normal);
  if(std::isfinite(clearance)&&clearance>=*radius-tolerance&&clearance<=*radius+pullback+tolerance)return true;
 }return false;
}
// No selected mode is present in711a. Accept a geometrically witnessed member
// of the authoritative permitted-mode union, never a guessed packet field.
inline bool Check(const PlacementPolicy::Geometry& g,const PlacementPolicy::Transform& p,const MountFlags::Flags& flags,const Trace& trace,float default_heading=0){
 if(!g.native_collision_verified||(g.mounting_type!=1&&g.mounting_type!=2)||!std::isfinite(p.scale)||p.scale<=0)return false;
 for(float f:{p.heading,p.pitch,p.roll})if(!std::isfinite(f)||f<0||f>512)return false;
 if(!PlacementContact::Finite(p.position))return false;
 // Unknown historical bytes grant no Free capability. Static/non-NPC is bound
 // by the independently reviewed resource class, not an incoming item flag.
 if(flags.known&&!flags.disable_free)return true;
 PlacementContact::Profile profile{g.mounting_type,flags.disable_rotation,flags.disable_free,false,true};
 const auto rotation=PlacementPolicy::Rotation(p.heading,p.pitch,p.roll);
 double extent=0;for(unsigned i=0;i<3;++i){if(!std::isfinite(g.extents[i])||g.extents[i]<=0)return false;extent+=std::abs(rotation[3*i+2])*float(g.extents[i]*p.scale);}
 if(!std::isfinite(extent)||extent<0)return false;
 // Conservative supported float correspondence envelope, not a claimed Live
 // contact epsilon. Query geometry remains exact; this only bounds cache/pose
 // float rounding after transformed original source vertices.
 float ulp=0;for(float f:p.position)ulp=std::max(ulp,std::max(std::abs(std::nextafter(f,INFINITY)-f),std::abs(f-std::nextafter(f,-INFINITY))));
 const float tolerance=std::max(8*ulp,float(4*g.measured_local_error*p.scale)+8*std::numeric_limits<float>::epsilon());
 if(!std::isfinite(tolerance)||tolerance<=0||tolerance>.125f)return false;
 const auto test=[&](PlacementContact::Mode mode){
  if(!PlacementContact::ModeAllowed(profile,mode))return false;
  PlacementContact::Vector expected{p.position[1],p.position[0],p.position[2]}; // nativeYX -> serverXY
  if(mode!=PlacementContact::Mode::Wall)expected[2]+=g.mounting_type==2?float(extent):-float(extent);
  for(unsigned axis=0;axis<3;++axis)for(float sign:{-1.f,1.f}){
   auto start=expected,end=expected;start[axis]+=sign*4*tolerance;end[axis]-=sign*4*tolerance;
   auto hit=trace(start,end);if(!hit||!PlacementContact::NormalAllowed(profile,mode,hit->normal))continue;
   auto projected=PlacementContact::NativeContactPosition(profile,mode,*hit,float(extent));if(!projected)continue;
   PlacementContact::Vector actual{p.position[1],p.position[0],p.position[2]};bool same=true;for(unsigned i=0;i<3;++i)if(std::abs((*projected)[i]-actual[i])>tolerance)same=false;if(same)return true;
  }return false;
 };
 // Both branches are independent of the missing bytes. A type1 floor-mode
 // witness additionally requires the known DisableRotation=false predicate.
 if(g.mounting_type==1&&WallPlane(g,p,profile,tolerance,default_heading,trace))return true;
 if(g.mounting_type==2&&test(PlacementContact::Mode::Normal))return true;
 return g.mounting_type==1&&flags.known&&!flags.disable_rotation&&test(PlacementContact::Mode::Normal);
}
}
#endif
