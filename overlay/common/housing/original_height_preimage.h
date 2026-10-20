#ifndef EQEMU_HOUSING_ORIGINAL_HEIGHT_PREIMAGE_H
#define EQEMU_HOUSING_ORIGINAL_HEIGHT_PREIMAGE_H
#include "original_actor_query_frame.h"
#include <functional>
#include <vector>
namespace Housing::OriginalHeightPreimage {
namespace F=OriginalActorQueryFrame;
using Point=OriginalSphereQuery::Point;
using Digest=std::array<uint8_t,32>;
inline bool Nonzero(const Digest& d){return std::any_of(d.begin(),d.end(),[](auto b){return b!=0;});}
// Ordered finite binary32 key: preserves numeric order and distinguishes zeros.
inline uint32_t Key(float f){const auto b=std::bit_cast<uint32_t>(f);return b&0x80000000u?~b:b^0x80000000u;}
inline float Value(uint32_t k){return std::bit_cast<float>(k&0x80000000u?k^0x80000000u:~k);}
inline std::optional<float> ForwardCoordinate(float query,float center){
 OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};try{return F::Detail::Store(F::Detail::Sub(query,center));}catch(...){return {};}
}
// A complete preimage INTERVAL; potentially billions of floats, never an
// unbounded materialized list. Probe scheduling below may sample this interval.
struct Interval {uint32_t first{},last{};uint64_t size()const{return uint64_t(last)-first+1;}bool Contains(float f)const{return Key(f)>=first&&Key(f)<=last;}};
inline std::optional<Interval> Preimages(float actor,float center,float minimum,float maximum){
 OriginalSphereQuery::Detail::Rounding env;if(!env.good)return {};
 for(float v:{actor,center,minimum,maximum})if(!std::isfinite(v)||std::abs(v)>1e7f)return {};
 if(Key(minimum)>Key(maximum))return {};const auto target=Key(actor);const uint64_t begin=Key(minimum),end=uint64_t(Key(maximum))+1;
 auto output=[&](uint64_t k){return Key(F::Detail::Store(F::Detail::Sub(Value(uint32_t(k)),center)));};
 try{
  uint64_t lo=begin,hi=end;while(lo<hi){const auto mid=lo+(hi-lo)/2;if(output(mid)<target)lo=mid+1;else hi=mid;}const auto first=lo;
  if(first==end||output(first)!=target)return {};
  lo=first;hi=end;while(lo<hi){const auto mid=lo+(hi-lo)/2;if(output(mid)<=target)lo=mid+1;else hi=mid;}
  return Interval{uint32_t(first),uint32_t(lo-1)};
 }catch(...){return {};}
}
struct Snapshot {Digest digest{};uint64_t generation{};};
inline bool Same(const Snapshot& a,const Snapshot& b){return a.generation==b.generation&&a.digest==b.digest;}
// Callback contract: frozen authenticated source snapshot, complete native
// filtered nearest sweep, no post-hoc omission of an inconvenient early hit.
// Reduced witness type: scene actor contacts do not expose the full WORLD
// feature point, so never fabricate OriginalSphereQuery::Result.kind/point.
struct Contact {bool hit{};float fraction{};Point normal{},stopped{};};
struct Sweep {
 OriginalSphereSource::Status status=OriginalSphereSource::Status::InvalidInput;
 uint64_t source_id{};Contact contact{};uint32_t sources{},narrowphase{};uint64_t operations{};
 explicit operator bool()const{return status==OriginalSphereSource::Status::Hit;}
};
struct Height {
 Sweep sweep;double height{};bool valid{};
 Height()=default;
 // Keeps existing terrain-only callback aggregate initialization compatible.
 Height(const F::HeightResult& h):height(h.height),valid(h.valid){
  const auto&s=h.sweep;sweep.status=s.status;sweep.source_id=s.source_id;
  sweep.contact={s.contact.hit,s.contact.fraction,s.contact.normal,s.contact.stopped};
  sweep.sources=s.sources;sweep.narrowphase=s.narrowphase;sweep.operations=s.operations;
 }
};
struct Reply {Snapshot snapshot;bool complete{};Height height;};
using Query=std::function<Reply(const F::HeightRequest&,OriginalSphereSource::Limits)>;
// Must use recovered original normal/mode predicate AFTER nearest selection.
using Normal=std::function<bool(F::Mode,const Point&)>;
struct Policy {
 uint32_t version=1;Digest review{};Snapshot snapshot;
 Point minimum{},maximum{}; // independent reviewed QUERY-INPUT envelope in YXZ
 uint32_t queries=128,xy_probes=4,z_levels=7,source_limit=16384,narrowphase=8192;uint64_t operations=16000000;
};
enum class Status {Witness,Inconclusive,InvalidInput,Unavailable,SourceChanged};
struct Result {
 Status status=Status::InvalidInput;std::array<Interval,2> xy{};uint32_t queries{},narrowphase{};uint64_t operations{};
 F::HeightRequest request;Height height;Snapshot snapshot;
 explicit operator bool()const{return status==Status::Witness;}
};
inline bool ValidFields(const Policy& p){
 if(p.version!=1||!Nonzero(p.review)||!p.queries||p.queries>256||!p.xy_probes||p.xy_probes>4||p.z_levels>8||!p.source_limit||p.source_limit>65536||!p.narrowphase||p.narrowphase>8192||p.operations<48000||p.operations>64000000||!OriginalSphereSource::Finite(p.minimum)||!OriginalSphereSource::Finite(p.maximum))return false;
 for(unsigned k=0;k<3;++k)if(Key(p.minimum[k])>Key(p.maximum[k]))return false;return true;
}
inline bool Valid(const Policy& p){return ValidFields(p)&&Nonzero(p.snapshot.digest)&&p.snapshot.generation;}
// Static semantics bytes for a NEW typed policy digest. Ephemeral scene
// identity/generation belongs to each witness, not the persistent content hash.
inline std::vector<uint8_t> PolicyBytes(const Policy& p,F::Mode mode){
 if(!ValidFields(p)||!F::ValidMode(mode))return {};std::vector<uint8_t> b;const char tag[]="OriginalHeightPreimageV1";b.insert(b.end(),tag,tag+sizeof(tag));
 auto word=[&](uint32_t n){for(unsigned i=0;i<4;++i)b.push_back(uint8_t(n>>(8*i)));};word(p.version);word(mode==F::Mode::Floor?0:2);b.insert(b.end(),p.review.begin(),p.review.end());
 for(const auto& v:{p.minimum,p.maximum})for(float f:v)word(std::bit_cast<uint32_t>(f));for(auto n:{p.queries,p.xy_probes,p.z_levels,p.source_limit,p.narrowphase})word(n);word(uint32_t(p.operations));word(uint32_t(p.operations>>32));word(1); // nonoverlap required
 return b;
}
inline std::vector<float> Probes(const Interval& interval,uint32_t maximum){
 std::vector<float> out;auto add=[&](uint32_t k){if(k<interval.first||k>interval.last||out.size()==maximum)return;for(float x:out)if(Key(x)==k)return;out.push_back(Value(k));};
 if(interval.size()<=maximum){for(uint64_t k=interval.first;k<=interval.last;++k)add(uint32_t(k));}
 else {add(interval.first);add(interval.last);add(interval.first+uint32_t((interval.size()-1)/2));add(interval.first+uint32_t((interval.size()-1)/4));}return out;
}
// Exact forward equality; never require inverse QueryCenter(actor)==input XY.
inline bool Matches(const Point& actor,const F::Frame& frame,const F::HeightRequest& request,const Height& height,const std::array<Interval,2>& xy){
 if(!height.valid||!height.sweep||!F::ValidMode(request.mode)||std::bit_cast<uint32_t>(request.radius)!=std::bit_cast<uint32_t>(frame.radius)||!xy[0].Contains(request.input[0])||!xy[1].Contains(request.input[1]))return false;
 auto generated=F::ActorFromHeight(request.input,height.height,frame,request.mode);return generated&&OriginalSphereSource::Bits(*generated,actor);
}
inline Result Find(const Point& actor,const F::Frame& frame,F::Mode mode,const Policy& policy,const Query& query,const Normal& normal){
 Result out;if(!Valid(policy)||!F::Valid(frame)||!F::ValidMode(mode)||!OriginalSphereSource::Finite(actor)||!query||!normal)return out;
 OriginalSphereQuery::Detail::Rounding env;if(!env.good){out.status=Status::Unavailable;return out;}
 try{
  for(unsigned k=0;k<2;++k){auto interval=Preimages(actor[k],frame.center[k],policy.minimum[k],policy.maximum[k]);if(!interval){out.status=Status::Inconclusive;return out;}out.xy[k]=*interval;}
  auto x=Probes(out.xy[0],policy.xy_probes),y=Probes(out.xy[1],policy.xy_probes);std::set<uint32_t> seen_z;
  const auto try_z=[&](float z)->bool{
   if(z<policy.minimum[2]||z>policy.maximum[2]||!seen_z.insert(std::bit_cast<uint32_t>(z)).second)return false;
   for(float qx:x){for(float qy:y){
    if(out.queries==policy.queries||out.narrowphase==policy.narrowphase||policy.operations-out.operations<48000){out.status=Status::Inconclusive;return true;}
    // Recovered FINAL ordinary branch: arg8=0; normal initialized display1.
    F::HeightRequest request{mode,{qx,qy,z},frame.radius,0,1};++out.queries;OriginalSphereSource::Limits remaining{policy.source_limit,policy.narrowphase-out.narrowphase,policy.operations-out.operations};auto reply=query(request,remaining);
    // Account all completed bounded work even when a later unknown source or
    // freshness failure makes this probe unavailable. Never spend another probe
    // after those failures, but do not report discarded terrain/actor work as 0.
    const auto& sweep=reply.height.sweep;const auto status=sweep.status;
    if(sweep.narrowphase>remaining.narrowphase||sweep.operations>remaining.operations||sweep.sources>remaining.sources){out.status=Status::Unavailable;return true;}out.narrowphase+=sweep.narrowphase;out.operations+=sweep.operations;
    if(Nonzero(reply.snapshot.digest)&&!Same(reply.snapshot,policy.snapshot)){out.status=Status::SourceChanged;return true;}
    if(!reply.complete||!Same(reply.snapshot,policy.snapshot)){out.status=Status::Unavailable;return true;}
    if(status==OriginalSphereSource::Status::BudgetExceeded){out.status=Status::Inconclusive;return true;}
    if(status==OriginalSphereSource::Status::Clear){if(reply.height.valid||sweep.contact.hit){out.status=Status::Unavailable;return true;}continue;}
    if(status!=OriginalSphereSource::Status::Hit||!reply.height.valid||!sweep.contact.hit||!std::isfinite(sweep.contact.fraction)||!OriginalSphereSource::Finite(sweep.contact.normal)||!OriginalSphereSource::Finite(sweep.contact.stopped)){out.status=Status::Unavailable;return true;}
    if(sweep.contact.fraction<=0||sweep.contact.fraction>1||!normal(mode,sweep.contact.normal))continue;
    if(Matches(actor,frame,request,reply.height,out.xy)){out.status=Status::Witness;out.request=request;out.height=reply.height;out.snapshot=reply.snapshot;return true;}
   }}
   return false;
  };
  if(try_z(policy.minimum[2])||try_z(policy.maximum[2]))return out;
  // Versioned dyadic schedule, NOT a claim to enumerate all transient heights.
  // Never binary-search output Z: float fractions/pullback are non-monotone.
  for(unsigned level=1;level<=policy.z_levels;++level){const uint32_t denominator=1u<<level;for(uint32_t n=1;n<denominator;n+=2){const auto width=F::Detail::Sub(policy.maximum[2],policy.minimum[2]);const auto t=OriginalSphereQuery::Detail::quotient(n,denominator);const float z=F::Detail::Store(F::Detail::Add(policy.minimum[2],OriginalSphereQuery::Detail::product(width,t)));if(try_z(z))return out;}}
  out.status=Status::Inconclusive;return out;
 }catch(...){out.status=Status::Unavailable;return out;}
}
}
#endif
