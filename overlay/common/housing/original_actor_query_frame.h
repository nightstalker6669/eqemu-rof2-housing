#ifndef EQEMU_HOUSING_ORIGINAL_ACTOR_QUERY_FRAME_H
#define EQEMU_HOUSING_ORIGINAL_ACTOR_QUERY_FRAME_H
#include "original_sphere_source.h"
namespace Housing::OriginalActorQueryFrame {
using Point=OriginalSphereQuery::Point;
enum class Mode {Floor,Ceiling};
inline bool ValidMode(Mode m){return m==Mode::Floor||m==Mode::Ceiling;}
// Provider supplies the original renderer's already transformed, scaled box.
// No Euler convention, geometric epsilon, source normal flip or actor filter is
// inferred here. C and E are native YXZ. Radius uses the SECOND/unrotated box.
struct Frame {Point center{},extent{};float radius{};};
inline std::optional<Frame> FromRendererBoxes(const Point& rotated_center,const Point& rotated_extent,const Point& scaled_unrotated_extent){
 if(!OriginalSphereSource::Finite(rotated_center)||!OriginalSphereSource::Finite(rotated_extent)||!OriginalSphereSource::Finite(scaled_unrotated_extent))return {};
 for(unsigned k=0;k<3;++k)if(rotated_extent[k]<0||scaled_unrotated_extent[k]<=0)return {};
 const auto r=std::min({scaled_unrotated_extent[0],scaled_unrotated_extent[1],scaled_unrotated_extent[2]});if(r>1e6f)return {};return Frame{rotated_center,rotated_extent,r};
}
inline bool Valid(const Frame& f){return bool(FromRendererBoxes(f.center,f.extent,{f.radius,f.radius,f.radius}));}
namespace Detail {
inline float Store(double x){volatile double d=x;volatile float f=float(d);if(!std::isfinite(f))throw OriginalSphereQuery::Detail::Fault{};return f;}
inline double Add(double a,double b){return OriginalSphereQuery::Detail::sum(a,b);}
inline double Sub(double a,double b){return OriginalSphereQuery::Detail::difference(a,b);}
}
// 55869c..5586bf. This is a stored centre, not a physical contact point.
inline std::optional<Point> QueryCenter(const Point& actor,const Frame& f){
 if(!Valid(f)||!OriginalSphereSource::Finite(actor))return {};OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};try{Point q;for(unsigned k=0;k<3;++k)q[k]=Detail::Store(Detail::Add(actor[k],f.center[k]));return q;}catch(...){return {};}
}
// Algebraic inverse anchor only; float subtraction is not globally invertible.
// Authority must compare an ACTUAL stopped query through ActorFromStopped.
inline std::optional<Point> ScreenAnchor(const Point& actor,const Frame& f,Mode mode){
 if(!ValidMode(mode))return {};auto q=QueryCenter(actor,f);if(!q)return {};OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};try{(*q)[2]=Detail::Store(mode==Mode::Floor?Detail::Sub((*q)[2],f.extent[2]):Detail::Add((*q)[2],f.extent[2]));return q;}catch(...){return {};}
}
// 557937 final box-centre projection, THEN 5587c6..5587ee subtraction.
inline std::optional<Point> ActorFromStopped(const Point& stopped,const Frame& f,Mode mode){
 if(!ValidMode(mode)||!Valid(f)||!OriginalSphereSource::Finite(stopped))return {};OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};try{Point box=stopped;box[2]=Detail::Store(mode==Mode::Floor?Detail::Add(stopped[2],f.extent[2]):Detail::Sub(stopped[2],f.extent[2]));Point actor;for(unsigned k=0;k<3;++k)actor[k]=Detail::Store(Detail::Sub(box[k],f.center[k]));return actor;}catch(...){return {};}
}
// Explicit original wrapper inputs: caller must prove ordinary-class radius,
// selected source filters, extra height and display radius; no implicit defaults.
struct HeightRequest {Mode mode=Mode::Floor;Point input{};float radius{},extra_height{},display_radius{};};
struct HeightResult {OriginalSphereSource::Result sweep;double height{};bool valid{};};
inline HeightResult Height(std::span<const OriginalSphereSource::Triangle> selected,const HeightRequest& request,OriginalSphereSource::Limits limits={}){
 HeightResult out;if(!ValidMode(request.mode)||!OriginalSphereSource::Finite(request.input)||!std::isfinite(request.radius)||request.radius<=0||!std::isfinite(request.extra_height)||!std::isfinite(request.display_radius)||request.display_radius<=0)return out;
 OriginalSphereQuery::Detail::Rounding env;if(!env.good){out.sweep.status=OriginalSphereSource::Status::Environment;return out;}
 try{
  Point start=request.input,end=request.input;float radius=request.radius;
  if(request.mode==Mode::Floor){start[2]=Detail::Store(Detail::Add(Detail::Add(request.input[2],radius),request.extra_height));end[2]=-32767.f;}
  else {radius=Detail::Store(OriginalSphereQuery::Detail::product(request.display_radius,request.radius));end[2]=32767.f;}
  out.sweep=OriginalSphereSource::Sweep(selected,start,end,radius,1,limits);if(!out.sweep)return out;
  // Floor48c72c..750 stores its adjusted result to binary32. Ceiling48c867
  // leaves stopZ+radius on the PC53 stack. Neither is a raw triangle height.
  out.height=request.mode==Mode::Floor?double(Detail::Store(Detail::Add(Detail::Sub(out.sweep.contact.stopped[2],radius),double(std::bit_cast<float>(uint32_t(0x3a83126f)))))):Detail::Add(out.sweep.contact.stopped[2],radius);
  out.valid=std::isfinite(out.height);return out;
 }catch(...){return out;}
}
// Literal fallback store chain55837a/558419→558457→55852d→5587ea.
// This preserves cancellation/store boundaries omitted by H±E−C algebra.
inline std::optional<Point> ActorFromHeight(const Point& resolved_center_xy,double height,const Frame& f,Mode mode){
 if(!ValidMode(mode)||!Valid(f)||!OriginalSphereSource::Finite(resolved_center_xy)||!std::isfinite(height))return {};OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};try{
  const auto offset=Detail::Store(mode==Mode::Floor?Detail::Sub(f.center[2],f.extent[2]):Detail::Add(f.center[2],f.extent[2]));
  Point box=resolved_center_xy;box[2]=Detail::Store(Detail::Add(Detail::Sub(height,offset),f.center[2]));Point actor;for(unsigned k=0;k<3;++k)actor[k]=Detail::Store(Detail::Sub(box[k],f.center[k]));return actor;
 }catch(...){return {};}
}
// Camera-free NECESSARY fallback witness for a trusted explicit height request.
// This does not prove input-height/extra-height policy, source coverage/filters,
// mode, region, body clearance, or that the client chose this fallback branch.
inline bool MatchesHeightPose(const Point& actor,const Frame& f,const HeightRequest& request,const HeightResult& result){
 if(!ValidMode(request.mode)||!result.valid||!result.sweep||!Valid(f)||std::bit_cast<uint32_t>(request.radius)!=std::bit_cast<uint32_t>(f.radius))return false;
 auto expected=ActorFromHeight(request.input,result.height,f,request.mode);return expected&&OriginalSphereSource::Bits(*expected,actor);
}
}
#endif
