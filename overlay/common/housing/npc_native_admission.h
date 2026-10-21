#ifndef EQEMU_HOUSING_NPC_NATIVE_ADMISSION_H
#define EQEMU_HOUSING_NPC_NATIVE_ADMISSION_H
#include "npc_runtime.h"
#include "region_wire.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
namespace Housing::NpcNativeAdmission {
// Native presentation preimage only. Never use this region for authoritative
// placement, collision, custody, catalogue identity or membership checks.
struct Projection {RegionWire::Region region;double anchor_bound{},renderer_radius{},ground_height{},padding{};};
inline bool Reviewed(const NpcRuntime::Profile& p){return NpcRuntime::Valid(p)&&p.original_model_default&&p.item_id==77408&&p.model=="HLG"&&p.race==715&&p.gender==2&&p.model_default_size_bits==0x40c00000u&&p.source_sha256==NpcReviewedBindings::SourceSha256;}
inline double Up(double x){if(!std::isfinite(x))throw std::invalid_argument("Nonfinite NPC admission bound");return std::nextafter(x,std::numeric_limits<double>::infinity());}
inline float Extent(double x){if(!(x>0)||!std::isfinite(x)||x>std::numeric_limits<float>::max())throw std::invalid_argument("NPC admission extent");return std::nextafter(float(x),std::numeric_limits<float>::infinity());}
// Constants below are generated from the authenticated model, every own-archive
// clip, original key reduction/interpolation, and the reviewed native FP paths.
// The immutable item binding and source raw actor height remain unchanged.
inline constexpr const char* ProofSha256="04765af0cbc4636b5355ddccb19fb509c4d7706270f0340b156d6994c79a7b74";
inline constexpr double RadiusIntercept=83.88973520919154;
inline constexpr double RadiusAnchorCoefficient=0.0002442745683602869;
inline Projection Build(const NpcRuntime::Profile& p,const std::vector<RegionWire::Region>& original,uint32_t native_region_id){
 if(!Reviewed(p)||original.empty())throw std::invalid_argument("Unsupported NPC native admission");EstateWire::Detail::Identity(native_region_id);
 double h=0;for(auto bits:p.allowed_heights.height_bits){const double v=NpcNativeHeight::Float(bits);if(!(v>0)||!std::isfinite(v))throw std::invalid_argument("NPC height frame");h=std::max(h,v);}h=Up(h);
 std::array<double,3> lo{INFINITY,INFINITY,INFINITY},hi{-INFINITY,-INFINITY,-INFINITY};
 for(const auto& r:original){if(r.opaque_vector!=std::array<float,3>{}||r.opaque_ids!=std::array<uint32_t,2>{0xffffffffu,0xffffffffu})throw std::invalid_argument("Unsupported NPC native region shape");RegionWire::Detail::ValidateMatrix(r.matrix);for(unsigned signs=0;signs<8;++signs)for(unsigned axis=0;axis<3;++axis){double v=r.matrix[12+axis];for(unsigned k=0;k<3;++k)v+=((signs>>k)&1?1.0:-1.0)*double(r.matrix[4*k+axis]);lo[axis]=std::min(lo[axis],std::nextafter(v,-INFINITY));hi[axis]=std::max(hi[axis],Up(v));}}
 double a2=0;for(unsigned k=0;k<3;++k){const double v=Up(std::max(std::abs(lo[k]),std::abs(hi[k])));a2=Up(a2+Up(v*v));}const double anchor=Up(std::sqrt(a2)+h);
 if(!(anchor<=65536.0))throw std::invalid_argument("NPC native coordinate proof range");
 const double radius=Up(RadiusIntercept+Up(RadiusAnchorCoefficient*anchor));
 if(!(radius>0))throw std::invalid_argument("NPC admission proof unavailable");
 const double u=0x1p-23,gamma=Up((256*u)/(1-256*u));const double need=Up(radius+h+Up(gamma*Up(anchor+radius+h+1)));
 // Twice the outward power-of-two enclosure reserves a proved error cushion
 // for native world-corner and axis-box inversion arithmetic, not item space.
 int exponent=0;std::frexp(need,&exponent);const double padding=std::ldexp(1.0,exponent+1);
 if(!std::isfinite(padding)||padding>0x1p40)throw std::invalid_argument("NPC native inverse proof range");
 RegionWire::Region result{};result.id=native_region_id;result.matrix[15]=1;
 for(unsigned k=0;k<3;++k){const double low=std::nextafter(lo[k]-padding,-INFINITY),high=Up(hi[k]+padding);const float center=float((low+high)*.5);result.matrix[12+k]=center;result.matrix[k*5]=Extent(std::max(double(center)-low,high-double(center)));}
 RegionWire::Detail::ValidateMatrix(result.matrix);return {result,anchor,radius,h,padding};
}
}
#endif
