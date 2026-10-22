#ifndef EQEMU_HOUSING_NPC_NATIVE_HEIGHT_H
#define EQEMU_HOUSING_NPC_NATIVE_HEIGHT_H
#include <array>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <cstring>
#include <optional>
namespace Housing::NpcNativeHeight {
// Original CRT8e1258 converts main-loop9031f to x87 PC53/chop and
// temporary displaya031f to PC24/chop. Exact integer arithmetic retains
// the extended exponent until original FSTP float32; server FENV is irrelevant.
// Header-only fixed-width unsigned arithmetic is supported by MSVC as well
// as GCC/Clang. All products/aligned sums here fit below bit125.
using Wide=boost::multiprecision::uint128_t;
struct Binary {Wide significand=0;int exponent=0;};
inline uint32_t Bits(float v){uint32_t b;std::memcpy(&b,&v,4);return b;}
inline float Float(uint32_t b){float v;std::memcpy(&v,&b,4);return v;}
inline unsigned Width(Wide n){unsigned w=0;while(n){++w;n>>=1;}return w;}
inline Wide ChopRight(Wide n,unsigned shift){return shift>=128?0:n>>shift;}
inline std::optional<Binary> Decode(uint32_t bits){
 if(bits>>31||!(bits&0x7fffffff)||((bits>>23)&255)==255)return {};
 const unsigned e=(bits>>23)&255;return Binary{(bits&0x7fffff)|(e?0x800000:0),e?int(e)-150:-149};
}
inline Binary Multiply(Binary a,Binary b,unsigned precision){
 Binary out{a.significand*b.significand,a.exponent+b.exponent};const auto w=Width(out.significand);
 if(w>precision){const auto shift=w-precision;out.significand>>=shift;out.exponent+=shift;}
 return out;
}
inline std::optional<uint32_t> Float32(Binary a){
 if(!a.significand)return uint32_t(0);auto w=Width(a.significand);int top=a.exponent+int(w)-1;
 // Masked original x87 overflow with chop produces largest finite float32.
 if(top>127)return uint32_t(0x7f7fffff);
 if(top>=-126){if(w>24){a.significand>>=w-24;w=24;}const auto mantissa=uint32_t(a.significand<<(24-w));return uint32_t(top+127)<<23|(mantissa&0x7fffff);}
 const int shift=-(a.exponent+149);return uint32_t(shift>0?ChopRight(a.significand,unsigned(shift)):a.significand<<unsigned(-shift));
}
inline std::optional<uint32_t> OriginalChop(uint32_t raw,uint32_t roffset,bool race_flag,bool global_nonzero,unsigned precision){
 if(precision!=24&&precision!=53)return {};auto a=Decode(raw),base=Decode(roffset);if(!a||!base)return {};
 *a=Multiply(*a,*Decode(race_flag?0x3e2aaaab:0x3e4ccccd),precision);
 if(race_flag!=global_nonzero)*a=Multiply(*a,*Decode(race_flag?0x3fa66666:0x3f44ec4f),precision);
 return Float32(Multiply(*a,*base,precision));
}
// Exact float32 chop addition/subtraction. PC24 and PC53 give the same
// final float32 result for two float32 operands: truncation cannot double round.
inline std::optional<uint32_t> Add(uint32_t left,uint32_t right){
 if(((left>>23)&255)==255||((right>>23)&255)==255)return {};
 if(!(left&0x7fffffff)&&!(right&0x7fffffff))return (left&right)&0x80000000u;
 if(!(left&0x7fffffff))return right;if(!(right&0x7fffffff))return left;
 auto a=*Decode(left&0x7fffffff),b=*Decode(right&0x7fffffff);bool an=left>>31,bn=right>>31;
 if(a.exponent<b.exponent){auto copy=a;a=b;b=copy;bool n=an;an=bn;bn=n;auto bits=left;left=right;right=bits;}
 const unsigned difference=unsigned(a.exponent-b.exponent);
 // The small operand is below every retained bit. With opposite signs its
 // sticky subtraction makes chop step one float32 ULP toward zero.
 if(difference>100)return an==bn?left:((left&0x80000000u)|((left&0x7fffffffu)-1));
 // Use unsigned magnitudes instead of compiler-specific signed128. Equal-sign
 // addition and opposite-sign subtraction retain the exact original sign.
 const Wide av=a.significand<<difference,bv=b.significand;
 Wide magnitude;bool negative;
 if(an==bn){magnitude=av+bv;negative=an;}
 else if(av>=bv){magnitude=av-bv;negative=an;}
 else{magnitude=bv-av;negative=bn;}
 if(!magnitude)return uint32_t(0);Binary out{magnitude,b.exponent};
 auto result=Float32(out);if(!result)return {};return *result|(negative?0x80000000u:0);
}
inline std::optional<uint32_t> Subtract(uint32_t a,uint32_t b){return Add(a,b^0x80000000u);}
struct Variants {std::array<uint32_t,4> height_bits{};};
inline std::optional<Variants> OriginalControlStates(uint32_t raw,uint32_t roffset,bool race_flag){
 Variants v;unsigned index=0;for(unsigned precision:{53u,24u})for(bool mode:{false,true}){auto h=OriginalChop(raw,roffset,race_flag,mode,precision);if(!h||!Decode(*h))return {};v.height_bits[index++]=*h;}return v;
}
inline bool Contains(const Variants& v,float h){const auto b=Bits(h);for(auto x:v.height_bits)if(x==b)return true;return false;}
}
#endif
