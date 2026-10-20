#ifndef EQEMU_HOUSING_ORIGINAL_RENDERER_BOX_H
#define EQEMU_HOUSING_ORIGINAL_RENDERER_BOX_H
#include "original_actor_query_frame.h"
namespace Housing::OriginalRendererBox {
using Point=OriginalSphereQuery::Point;
struct Result {OriginalActorQueryFrame::Frame frame;Point scaled_center{},scaled_extent{};std::array<float,9> matrix{};uint32_t steps{};};
namespace Detail {
struct Fault{};
// Original FSIN/FCOS are x87 instructions, NOT host libm approximations. Keep
// their extended results until the next original PC53 arithmetic/store boundary.
// This bounded architecture-specific helper never executes target image code.
struct Environment {
 std::fenv_t saved{};bool captured{},good{};
 Environment(){
  captured=std::fegetenv(&saved)==0;std::fenv_t ignored{};
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
  good=captured&&std::feholdexcept(&ignored)==0&&std::fesetround(FE_TOWARDZERO)==0&&std::numeric_limits<long double>::digits==64;
  if(good){uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));cw=uint16_t((cw&~0x0300u)|0x0200u);asm volatile("fldcw %0"::"m"(cw));uint16_t verify;asm volatile("fnstcw %0":"=m"(verify));good=(verify&0x0f00u)==0x0e00u;}
#endif
 }
 ~Environment(){if(captured)std::fesetenv(&saved);}
};
inline long double sum(long double a,long double b){volatile long double x=a,y=b,z=x+y;return z;}
inline long double difference(long double a,long double b){volatile long double x=a,y=b,z=x-y;return z;}
inline long double product(long double a,long double b){volatile long double x=a,y=b,z=x*y;return z;}
inline long double quotient(long double a,long double b){volatile long double x=a,y=b,z=x/y;return z;}
inline long double sine(long double a){
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
 long double v;asm volatile("fldt %1; fsin; fstpt %0":"=m"(v):"m"(a):"st");return v;
#else
 throw Fault{};
#endif
}
inline long double cosine(long double a){
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
 long double v;asm volatile("fldt %1; fcos; fstpt %0":"=m"(v):"m"(a):"st");return v;
#else
 throw Fault{};
#endif
}
struct Math {
 std::array<uint32_t,8> r{};std::array<long double,8>st{};unsigned depth{};uint32_t pc{},sw{},steps{};bool zf{},sf{},pf{},cf{};
 std::array<uint8_t,0x8000> data{};std::array<bool,0x8000> initialized{};
 std::array<uint8_t,0x2000> stack{};std::array<bool,0x2000> stack_initialized{};
 Math(){r[7]=0x700000;}
 uint8_t byte(uint32_t a)const{
  if(a>=0x10000&&a<0x18000){auto i=a-0x10000;if(!initialized[i])throw Fault{};return data[i];}
  if(a>=0x6ff000&&a<0x701000){auto i=a-0x6ff000;if(!stack_initialized[i])throw Fault{};return stack[i];}
  const auto base=a&~3u;uint32_t v;switch(base){
#include "original_box_constants.inc"
  default:throw Fault{};}return uint8_t(v>>((a&3)*8));
 }
 uint32_t get(uint32_t a,unsigned n)const{uint32_t v=0;for(unsigned j=0;j<n;++j)v|=uint32_t(byte(a+j))<<(8*j);return v;}
 void put(uint32_t a,uint32_t v,unsigned n=4){for(unsigned j=0;j<n;++j){const auto at=a+j;if(at>=0x10000&&at<0x18000){auto i=at-0x10000;data[i]=uint8_t(v>>(j*8));initialized[i]=true;}else if(at>=0x6ff000&&at<0x701000){auto i=at-0x6ff000;stack[i]=uint8_t(v>>(j*8));stack_initialized[i]=true;}else throw Fault{};}}
 long double fp(uint32_t a)const{return static_cast<long double>(std::bit_cast<float>(get(a,4)));}
 void fpwrite(uint32_t a,long double x){volatile long double value=x;volatile float stored=float(value);if(!std::isfinite(stored))throw Fault{};put(a,std::bit_cast<uint32_t>(float(stored)));}
 void pushfp(long double x){if(depth==8||!std::isfinite(x))throw Fault{};for(unsigned j=depth;j>0;--j)st[j]=st[j-1];st[0]=x;++depth;}
 void popfp(){if(!depth)throw Fault{};for(unsigned j=0;j+1<depth;++j)st[j]=st[j+1];--depth;}
 void flags(uint32_t v){zf=v==0;sf=bool(v&0x80000000u);pf=(std::popcount(v&255u)%2)==0;}
 void compare(long double a,long double b){if(!std::isfinite(a)||!std::isfinite(b))throw Fault{};sw=a<b?0x100u:a==b?0x4000u:0u;}
 void run(uint32_t entry,uint32_t stop=0){pc=entry;for(unsigned count=0;count<16000;++count){++steps;if(pc==stop&&stop)return;switch(pc){
#include "original_box_operations.inc"
 default:throw Fault{};}}throw Fault{};}
};

}
// Registry values must be reviewed renderer CACHE C/E, not raw vertex bounds,
// guessed metadata or an NPC height. Their binary32 projection is intentional.
// Orientation is the actor's original getter84 order in native512-unit angles.
inline std::optional<Result> Build(const std::array<double,3>& cache_center,const std::array<double,3>& cache_extent,float actor_scale,const Point& renderer_orientation){
 Detail::Environment environment;if(!environment.good)return {};
 if(!std::isfinite(actor_scale)||actor_scale<=0||actor_scale>16||!OriginalSphereSource::Finite(renderer_orientation))return {};
 for(unsigned k=0;k<3;++k)if(!std::isfinite(cache_center[k])||!std::isfinite(cache_extent[k])||std::abs(cache_center[k])>1e7||cache_extent[k]<=0||cache_extent[k]>1e7||std::abs(renderer_orientation[k])>512)return {};
 try{
  Detail::Math m;constexpr uint32_t box=0x10000,matrix=0x11000,angles=0x12000;Result result;
  // 4908e4..490935: cached floats multiply actual actor scale, store32.
  for(unsigned k=0;k<3;++k){m.fpwrite(box+k*4,cache_center[k]);m.fpwrite(box+12+k*4,cache_extent[k]);m.fpwrite(box+k*4,Detail::product(m.fp(box+k*4),actor_scale));m.fpwrite(box+12+k*4,Detail::product(m.fp(box+12+k*4),actor_scale));result.scaled_center[k]=float(m.fp(box+k*4));result.scaled_extent[k]=float(m.fp(box+12+k*4));}
  // 49de20 reciprocal256 + store32;490975..4909a6 multiply PI32 + store32.
  for(unsigned k=0;k<3;++k){m.fpwrite(angles+k*4,Detail::product(renderer_orientation[k],Detail::quotient(1,256)));m.fpwrite(angles+k*4,Detail::product(m.fp(angles+k*4),std::bit_cast<float>(uint32_t(0x40490fdb))));}
  m.r[2]=matrix;m.put(m.r[7],0xeeeeeeee);m.put(m.r[7]+4,angles);m.run(0x7a32b0);if(m.depth)return {};
  for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)result.matrix[row*3+col]=float(m.fp(matrix+row*16+col*4));
  m.r.fill(0);m.r[7]=0x700000;m.r[2]=box;m.put(m.r[7],0xeeeeeeee);m.put(m.r[7]+4,matrix);m.run(0x7a30d0);if(m.depth)return {};
  Point center,extent;for(unsigned k=0;k<3;++k){center[k]=float(m.fp(box+k*4));extent[k]=float(m.fp(box+12+k*4));}
  auto frame=OriginalActorQueryFrame::FromRendererBoxes(center,extent,result.scaled_extent);if(!frame)return {};result.frame=*frame;result.steps=m.steps;return result;
 }catch(const Detail::Fault&){return {};}
}
}
#endif
