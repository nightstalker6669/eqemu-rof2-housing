#ifndef EQEMU_HOUSING_ORIGINAL_SPHERE_QUERY_H
#define EQEMU_HOUSING_ORIGINAL_SPHERE_QUERY_H
#include <array>
#include <bit>
#include <cfenv>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
// Narrow mechanical port of original RoF2 pure collision mathematics. No code
// image, process, OS callback or untrusted instruction is loaded/executed.
// Caller supplies reviewed triangles; source/ACL/profile authority is separate.
// Validation candidate: not yet wired to production admission.
namespace Housing::OriginalSphereQuery {
using Point=std::array<float,3>;
struct Result {bool hit{};float fraction{};uint32_t kind{};Point point{},normal{},stopped{};uint32_t steps{};};
namespace Detail {
struct Fault{};
struct Rounding {
 std::fenv_t saved{};bool captured{},good{};
 Rounding(){
  captured=std::fegetenv(&saved)==0;std::fenv_t ignored{};
  good=FLT_EVAL_METHOD==0&&std::numeric_limits<float>::is_iec559&&std::numeric_limits<float>::digits==24&&captured&&std::feholdexcept(&ignored)==0&&std::fesetround(FE_TOWARDZERO)==0&&std::fegetround()==FE_TOWARDZERO;
  if(good){
   // The port needs gradual binary32 stores/inputs. Ambient FTZ/DAZ cannot
   // silently change original x87 results; reject such an environment.
   volatile float normal=std::numeric_limits<float>::min(),half=.5f,two=2.f;
   volatile float down=normal*half,subnormal=std::bit_cast<float>(uint32_t(0x00400000)),up=subnormal*two;
   good=std::bit_cast<uint32_t>(float(down))==0x00400000u&&std::bit_cast<uint32_t>(float(up))==0x00800000u;
  }
 }
 ~Rounding(){if(captured)std::fesetenv(&saved);}
 Rounding(const Rounding&)=delete;Rounding& operator=(const Rounding&)=delete;
};
// Volatile operations preserve every original PC53 arithmetic boundary. The
// input domain is finite binary32 geometry; native extended exponent overflow
// cases are deliberately not promoted to authority by this bounded port.
inline double sum(double a,double b){volatile double x=a,y=b,z=x+y;return z;}
inline double difference(double a,double b){volatile double x=a,y=b,z=x-y;return z;}
inline double product(double a,double b){volatile double x=a,y=b,z=x*y;return z;}
inline double quotient(double a,double b){volatile double x=a,y=b,z=x/y;return z;}
inline double root(double a){volatile double x=a,z=std::sqrt(x);return z;}
struct Math {
 std::array<uint32_t,8> r{};std::array<double,8>st{};unsigned depth{};uint32_t pc{},sw{},steps{};bool zf{},sf{},pf{},cf{};
 std::array<uint8_t,0x8000> data{};std::array<bool,0x8000> initialized{};
 std::array<uint8_t,0x2000> stack{};std::array<bool,0x2000> stack_initialized{};
 Math(){r[7]=0x700000;}
 uint8_t byte(uint32_t a)const{
  if(a>=0x10000&&a<0x18000){auto i=a-0x10000;if(!initialized[i])throw Fault{};return data[i];}
  if(a>=0x6ff000&&a<0x701000){auto i=a-0x6ff000;if(!stack_initialized[i])throw Fault{};return stack[i];}
  const auto base=a&~3u;uint32_t v;switch(base){
#include "original_sphere_query_constants.inc"
  default:throw Fault{};}return uint8_t(v>>((a&3)*8));
 }
 uint32_t get(uint32_t a,unsigned n)const{uint32_t v=0;for(unsigned j=0;j<n;++j)v|=uint32_t(byte(a+j))<<(8*j);return v;}
 void put(uint32_t a,uint32_t v,unsigned n=4){for(unsigned j=0;j<n;++j){const auto at=a+j;if(at>=0x10000&&at<0x18000){auto i=at-0x10000;data[i]=uint8_t(v>>(j*8));initialized[i]=true;}else if(at>=0x6ff000&&at<0x701000){auto i=at-0x6ff000;stack[i]=uint8_t(v>>(j*8));stack_initialized[i]=true;}else throw Fault{};}}
 double fp(uint32_t a)const{return double(std::bit_cast<float>(get(a,4)));}
 void fpwrite(uint32_t a,double x){volatile double value=x;volatile float stored=float(value);if(!std::isfinite(stored))throw Fault{};put(a,std::bit_cast<uint32_t>(float(stored)));}
 void pushfp(double x){if(depth==8||!std::isfinite(x))throw Fault{};for(unsigned j=depth;j>0;--j)st[j]=st[j-1];st[0]=x;++depth;}
 void popfp(){if(!depth)throw Fault{};for(unsigned j=0;j+1<depth;++j)st[j]=st[j+1];--depth;}
 void flags(uint32_t v){zf=v==0;sf=bool(v&0x80000000u);pf=(std::popcount(v&255u)%2)==0;}
 void compare(double a,double b){if(!std::isfinite(a)||!std::isfinite(b))throw Fault{};sw=a<b?0x100u:a==b?0x4000u:0u;}
 void run(uint32_t entry,uint32_t stop=0){pc=entry;for(unsigned count=0;count<16000;++count){++steps;if(pc==stop&&stop)return;switch(pc){
#include "original_sphere_query_operations.inc"
 default:throw Fault{};}}throw Fault{};}
};
}
inline std::optional<Result> Triangle(const std::array<Point,3>&triangle,const Point&start,const Point&end,float radius,float maximum=1){
 for(const auto&p:triangle)for(float x:p)if(!std::isfinite(x)||std::abs(x)>1e7f)return {};
 for(const auto&p:{start,end})for(float x:p)if(!std::isfinite(x)||std::abs(x)>1e7f)return {};
 if(!std::isfinite(radius)||radius<=0||radius>1e6f||!std::isfinite(maximum)||maximum<0||maximum>1)return {};
 if(!std::numeric_limits<double>::is_iec559||std::numeric_limits<double>::digits!=53)return {};
 Detail::Rounding env;if(!env.good)return {};
 try{
  Detail::Math m;constexpr uint32_t desc=0x10000,zero=0x11000,sphere=0x12000,delta=0x13000,time=0x14000,kind=0x15000,point=0x16000;
  constexpr std::array<uint32_t,3> at={0x10100,0x10110,0x10120};
  for(unsigned j=0;j<3;++j){m.put(desc+0x10+j*4,at[j]);for(unsigned k=0;k<3;++k)m.fpwrite(at[j]+k*4,triangle[j][k]);}
  // Actual direct EQGT normal producer; preserves its exact operation order.
  m.r[2]=desc;m.r[5]=at[0];m.r[3]=at[1];m.r[1]=at[2];m.run(0x10021339,0x100213f5);
  Result result;for(unsigned k=0;k<3;++k)result.normal[k]=float(m.fp(desc+k*4));
  double n2=0;for(float x:result.normal)n2+=double(x)*x;if(!(n2>.5&&n2<1.5))return {};
  m.depth=0;m.r.fill(0);m.r[7]=0x700000;
  for(unsigned k=0;k<3;++k){m.fpwrite(zero+k*4,0);m.fpwrite(sphere+4+k*4,start[k]);m.fpwrite(delta+k*4,Detail::difference(end[k],start[k]));}
  m.fpwrite(sphere,radius);m.fpwrite(time,maximum);m.put(kind,0);for(unsigned k=0;k<9;++k)m.fpwrite(point+k*4,0);
  m.put(m.r[7],0xeeeeeeee);const std::array<uint32_t,8>args={desc,zero,sphere,delta,time,std::bit_cast<uint32_t>(maximum),kind,point};for(unsigned j=0;j<args.size();++j)m.put(m.r[7]+4+j*4,args[j]);
  m.run(0x7a6770);result.hit=(m.r[0]&255)!=0;result.fraction=float(m.fp(time));result.kind=m.get(kind,4);for(unsigned k=0;k<3;++k)result.point[k]=float(m.fp(point+k*4));
  if(result.hit){
   if(result.fraction<0||result.fraction>maximum||!std::isfinite(result.fraction))return {};
   // Actual7a2680 travel pullback (distinct from floor wrapper+.001).
   m.depth=0;m.r.fill(0);m.r[7]=0x700000;m.r[2]=0x17000;
   m.fpwrite(0x17038,result.fraction);for(unsigned k=0;k<3;++k){m.fpwrite(0x17004+k*4,start[k]);m.fpwrite(0x1701c+k*4,Detail::difference(end[k],start[k]));}
   m.put(m.r[7],0xeeeeeeee);m.put(m.r[7]+4,0x17100);m.run(0x7a2680);
   for(unsigned k=0;k<3;++k)result.stopped[k]=float(m.fp(0x17100+k*4));
  }
  result.steps=m.steps;return result;
 }catch(const Detail::Fault&){return {};}
}
}
#endif
