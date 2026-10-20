#ifndef EQEMU_HOUSING_ORIGINAL_ACTOR_LOCAL_QUERY_H
#define EQEMU_HOUSING_ORIGINAL_ACTOR_LOCAL_QUERY_H
#include "original_height_scene_collector.h"
namespace Housing::OriginalActorLocalQuery {
using Point=OriginalSphereQuery::Point;
using Matrix=std::array<float,16>;
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
inline long double root(long double a){
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
 long double v;asm volatile("fldt %1; fsqrt; fstpt %0":"=m"(v):"m"(a):"st");return v;
#else
 throw Fault{};
#endif
}
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
#include "original_actor_local_constants.inc"
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
#include "original_actor_local_operations.inc"
 default:throw Fault{};}}throw Fault{};}
};

}

// Exact renderer startup table image: cos[512] followed by sin[512]. Provider
// must authenticate its original construction/configuration; no hostlibm table.
using TrigTables=std::array<float,1024>;
inline std::optional<Matrix> MatrixFromRendererAngles(const Point& position,const Point& angles,const TrigTables& tables){
 Detail::Environment environment;if(!environment.good||!OriginalSphereSource::Finite(position)||!OriginalSphereSource::Finite(angles))return {};
 for(float x:angles)if(std::abs(x)>1e6f)return {};for(float x:tables)if(!std::isfinite(x)||std::abs(x)>1.001f)return {};
 try{
  auto Trig=[&](float angle,bool sine){long double value=sine?static_cast<long double>(angle):std::abs(static_cast<long double>(angle));int32_t integer=int32_t(value);if(sine&&integer<0)--integer;const auto fraction=Detail::difference(value,integer);const auto index=uint32_t(integer)&511u;const unsigned offset=sine?512:0;const long double first=tables[offset+index],next=tables[offset+((index+1)&511)];return Detail::sum(Detail::product(fraction,Detail::difference(next,first)),first);};
  Detail::Math m;constexpr uint32_t data=0x10000;
  // 10052af0 sets both additional100c4200 angle offsets to exactzero.
  // Preserve five FSTP32 boundaries; finalcos(angle2) stays on x87 stack.
  m.fpwrite(m.r[7]+12,Trig(angles[0],true));m.fpwrite(m.r[7]+8,Trig(angles[0],false));m.fpwrite(m.r[7]+20,Trig(-angles[1],true));m.fpwrite(m.r[7]+4,Trig(-angles[1],false));m.fpwrite(m.r[7]+24,Trig(angles[2],true));m.pushfp(Trig(angles[2],false));
  for(unsigned k=0;k<16;++k)m.fpwrite(data+4+4*k,k%5==0?1:0);for(unsigned k=0;k<3;++k)m.fpwrite(data+0x34+4*k,position[k]);m.r[4]=data;m.run(0x100c42b8,0x100c4335);if(m.depth)return {};Matrix out;for(unsigned k=0;k<16;++k)out[k]=float(m.fp(data+4+4*k));return out;
 }catch(...){return {};}
}
struct Frame {Matrix scaled_world{},inverse{};Point start{},delta{};float radius{};uint32_t steps{};};
inline bool Finite(const Matrix& matrix){for(float f:matrix)if(!std::isfinite(f)||std::abs(f)>1e7f)return false;return true;}
// Exact input is original actorData+4 matrix, not azone ZON or EQGM raw pose.
// Native matrix producer100c4200 uses renderer trigonometry interface; do not
// substitute OriginalRendererBox's game-side matrix or a generic Euler helper.
inline std::optional<Frame> Build(const Matrix& actor_matrix,float scale,const Point& world_start,const Point& world_delta,float world_radius){
 Detail::Environment environment;if(!environment.good||!Finite(actor_matrix)||!OriginalSphereSource::Finite(world_start)||!OriginalSphereSource::Finite(world_delta)||!std::isfinite(scale)||scale<=0||scale>16||!std::isfinite(world_radius)||world_radius<=0||world_radius>1e6f)return {};
 if(actor_matrix[3]!=0||actor_matrix[7]!=0||actor_matrix[11]!=0||actor_matrix[15]!=1)return {};
 try{Detail::Math m;constexpr uint32_t forward=0x10000,inverse=0x10100,input=0x10200,output=0x10300;Frame out;
 for(unsigned k=0;k<16;++k)m.fpwrite(forward+4*k,actor_matrix[k]);
 auto Run=[&](uint32_t entry,uint32_t self,std::initializer_list<uint32_t>args){if(m.depth)throw Detail::Fault{};m.r.fill(0);m.r[7]=0x700000;m.r[2]=self;m.put(m.r[7],0xeeeeeeee);unsigned i=0;for(auto arg:args)m.put(m.r[7]+4*(++i),arg);m.run(entry);if(m.depth)throw Detail::Fault{};};
 Run(0x10045320,forward,{std::bit_cast<uint32_t>(scale)});Run(0x10021a60,forward,{inverse});
 for(unsigned k=0;k<16;++k){out.scaled_world[k]=float(m.fp(forward+4*k));out.inverse[k]=float(m.fp(inverse+4*k));}
 if(!Finite(out.scaled_world)||!Finite(out.inverse))return {};
 for(unsigned k=0;k<3;++k)m.fpwrite(input+4*k,world_start[k]);Run(0x1000b210,inverse,{output,input});for(unsigned k=0;k<3;++k)out.start[k]=float(m.fp(output+4*k));
 for(unsigned k=0;k<3;++k)m.fpwrite(input+4*k,world_delta[k]);Run(0x1002fb20,inverse,{output,input});for(unsigned k=0;k<3;++k)out.delta[k]=float(m.fp(output+4*k));
 volatile long double r=Detail::quotient(world_radius,scale);volatile float stored=float(r);out.radius=stored;
 if(!OriginalSphereSource::Finite(out.start)||!OriginalSphereSource::Finite(out.delta)||!std::isfinite(out.radius)||out.radius<=0||out.radius>1e6f)return {};out.steps=m.steps;return out;
 }catch(...){return {};}
}
inline std::optional<Point> ModelNormal(const std::array<Point,3>& native_abc){
 Detail::Environment environment;if(!environment.good)return {};for(const auto&p:native_abc)if(!OriginalSphereSource::Finite(p))return {};
 try{Detail::Math m;constexpr uint32_t d=0x10000;constexpr std::array<uint32_t,3>v{0x10100,0x10110,0x10120};for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)m.fpwrite(v[j]+4*k,native_abc[j][k]);
 m.r[2]=d;m.r[5]=v[0];m.r[4]=v[1];m.r[6]=v[2];m.run(0x100c559d,0x100c565a);Point out;for(unsigned k=0;k<3;++k)out[k]=float(m.fp(d+4*k));return OriginalSphereSource::Finite(out)?std::optional<Point>(out):std::nullopt;
 }catch(...){return {};}
}
// Exact renderer post-contact normal rotation+normalization10045bde..10045bf5.
inline std::optional<Point> WorldNormal(const Matrix& scaled_world,const Point& local_normal){
 Detail::Environment environment;if(!environment.good||!Finite(scaled_world)||!OriginalSphereSource::Finite(local_normal))return {};
 try{Detail::Math m;constexpr uint32_t matrix=0x10000,input=0x10100,output=0x10200;for(unsigned k=0;k<16;++k)m.fpwrite(matrix+4*k,scaled_world[k]);for(unsigned k=0;k<3;++k)m.fpwrite(input+4*k,local_normal[k]);m.r[2]=matrix;m.put(m.r[7],0xeeeeeeee);m.put(m.r[7]+4,output);m.put(m.r[7]+8,input);m.run(0x1002fb20);m.r.fill(0);m.r[7]=0x700000;m.r[2]=output;m.put(m.r[7],0xeeeeeeee);m.run(0x100076d0);Point out;for(unsigned k=0;k<3;++k)out[k]=float(m.fp(output+4*k));return OriginalSphereSource::Finite(out)?std::optional<Point>(out):std::nullopt;
 }catch(...){return {};}
}
// Original100ba6c0 builds storedend, then1003ad80 storesend-start.
// Keep this deliberate native cancellation stage after matrix delta transform.
inline std::optional<Point> SphereDelta(const Point& start,const Point& transformed_delta){
 OriginalSphereQuery::Detail::Rounding env;if(!env.good||!OriginalSphereSource::Finite(start)||!OriginalSphereSource::Finite(transformed_delta))return {};
 try{Point out;for(unsigned k=0;k<3;++k){const float end=OriginalActorQueryFrame::Detail::Store(OriginalSphereQuery::Detail::sum(start[k],transformed_delta[k]));out[k]=OriginalActorQueryFrame::Detail::Store(OriginalSphereQuery::Detail::difference(end,start[k]));}return out;}catch(...){return {};}
}
enum class ContactStatus {Hit,Clear,Invalid,UnsupportedFeature};
struct Contact {ContactStatus status=ContactStatus::Invalid;float fraction{};Point normal{},point{};uint32_t kind{},steps{};};
// Reuses the existing exact kernel. sphere-equivalence.json proves renderer
// 100d5880 and game7a6770 complete reachable instruction/callee/constant parity.
// LOCAL delta is supplied directly, never reconstructed from rounded endpoints.
inline Contact TriangleDelta(const std::array<Point,3>& native_abc,const Point& cached_normal,const Point& start,const Point& delta,float radius,float maximum=1){
 Contact out;for(const auto&p:native_abc)if(!OriginalSphereSource::Finite(p))return out;for(const auto&p:{cached_normal,start,delta})if(!OriginalSphereSource::Finite(p))return out;
 if(!std::isfinite(radius)||radius<=0||radius>1e6f||!std::isfinite(maximum)||maximum<0||maximum>1||delta==Point{})return out;
 OriginalSphereQuery::Detail::Rounding environment;if(!environment.good)return out;
 try{OriginalSphereQuery::Detail::Math m;constexpr uint32_t desc=0x10000,zero=0x11000,sphere=0x12000,motion=0x13000,time=0x14000,kind=0x15000,point=0x16000;
 constexpr std::array<uint32_t,3> vertices{0x10100,0x10110,0x10120};for(unsigned j=0;j<3;++j){m.put(desc+0x10+4*j,vertices[j]);for(unsigned k=0;k<3;++k)m.fpwrite(vertices[j]+4*k,native_abc[j][k]);}
 for(unsigned k=0;k<3;++k){m.fpwrite(desc+4*k,cached_normal[k]);m.fpwrite(zero+4*k,0);m.fpwrite(sphere+4+4*k,start[k]);m.fpwrite(motion+4*k,delta[k]);}
 m.fpwrite(sphere,radius);m.fpwrite(time,maximum);m.put(kind,0);for(unsigned k=0;k<9;++k)m.fpwrite(point+4*k,0);
 m.put(m.r[7],0xeeeeeeee);const std::array<uint32_t,8>args{desc,zero,sphere,motion,time,std::bit_cast<uint32_t>(maximum),kind,point};for(unsigned j=0;j<8;++j)m.put(m.r[7]+4+4*j,args[j]);m.run(0x7a6770);
 out.steps=m.steps;if(!(m.r[0]&255)){out.status=ContactStatus::Clear;return out;}out.fraction=float(m.fp(time));out.kind=m.get(kind,4);for(unsigned k=0;k<3;++k)out.point[k]=float(m.fp(point+4*k));
 if(!std::isfinite(out.fraction)||out.fraction<0||out.fraction>maximum)return out;
 // The kind>1 CCollisionInfoSphere path needs its complete retained output
 // vector aggregation. Do not invent a normal from one of those witnesses.
 if(out.kind!=1){out.status=ContactStatus::UnsupportedFeature;return out;}
 Point gradient;for(unsigned k=0;k<3;++k){volatile double value=OriginalSphereQuery::Detail::difference(OriginalSphereQuery::Detail::sum(OriginalSphereQuery::Detail::product(delta[k],out.fraction),start[k]),out.point[k]);volatile float stored=float(value);gradient[k]=stored;}
 Detail::Environment normal_environment;if(!normal_environment.good)return out;Detail::Math normal_math;for(unsigned k=0;k<3;++k)normal_math.fpwrite(0x10000+4*k,gradient[k]);normal_math.r[2]=0x10000;normal_math.put(normal_math.r[7],0xeeeeeeee);normal_math.run(0x100076d0);for(unsigned k=0;k<3;++k)out.normal[k]=float(normal_math.fp(0x10000+4*k));out.steps+=normal_math.steps;
 if(!OriginalSphereSource::Finite(out.normal))return out;out.status=ContactStatus::Hit;return out;
 }catch(...){return out;}
}

}
#endif
