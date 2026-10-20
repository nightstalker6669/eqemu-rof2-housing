#ifndef EQEMU_HOUSING_ORIGINAL_CACHED_BOX_H
#define EQEMU_HOUSING_ORIGINAL_CACHED_BOX_H
#include "original_sphere_query.h"
namespace Housing::OriginalCachedBox {
struct Bounds {std::array<float,3> low{},high{};};
struct Box {std::array<float,3> center{},extent{};};
// Exact final100c58e4..5933 ordinary static cache producer. Caller supplies
// authenticated original eligible-triangle extrema, not render-all-vertex bounds.
// Does not implement ActorTag overrides, triangle eligibility or animated models.
inline std::optional<Box> FromReviewedBounds(const Bounds& b) {
 OriginalSphereQuery::Detail::Rounding rounding;if(!rounding.good)return {};
 Box out;
 for(unsigned k=0;k<3;++k){
  const float lo=b.low[k],hi=b.high[k];
  auto source=[](float x){return std::isfinite(x)&&std::abs(x)<=0x1p20f&&(x==0||std::abs(x)>=0x1p-100f);};
  if(!source(lo)||!source(hi)||lo>hi)return {};
  // Keep unspilled binary64 sum/difference through power-of-two multiply.
  // Binary32 source operands + nested PC53/chop + final store32 have exactly
  // the bounded rational producer result; nearest-even stores are not valid.
  volatile double a=lo,z=hi;
  volatile double sum=a+z,difference=z-a;
  volatile double c=sum*0.5,e=difference*0.5;
  volatile float cs=static_cast<float>(c),es=static_cast<float>(e);
  out.center[k]=cs;out.extent[k]=es;
  if(!std::isfinite(cs)||!std::isfinite(es)||es<=0)return {};
 }
 return out;
}
}
#endif
