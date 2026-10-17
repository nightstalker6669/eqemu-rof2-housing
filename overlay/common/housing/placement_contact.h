#ifndef EQEMU_HOUSING_PLACEMENT_CONTACT_H
#define EQEMU_HOUSING_PLACEMENT_CONTACT_H
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

namespace Housing::PlacementContact {
using Vector = std::array<float,3>; // One consistent world coordinate basis.
enum class Mode : uint32_t { Normal=0, Wall=1, Alternate=2, Free=3 };
struct Profile { uint32_t type{}; bool disable_rotation{},disable_free{},npc{},known{}; };
inline bool Finite(const Vector& v){return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
inline float Dot(const Vector& a,const Vector& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
// Native 0x5ae160. Unknown is a server authority fence, not a native field.
inline bool ModeAllowed(const Profile& p,Mode m){
 if(!p.known||p.type>2)return false;
 switch(m){
 case Mode::Normal:case Mode::Alternate:return !(p.type==1&&p.disable_rotation);
 case Mode::Wall:return p.type==1;
 case Mode::Free:return !p.disable_free&&!p.npc;
 }return false;
}
// Native 0x5ae010 after region checks, before object-blocked validation.
// This predicate alone does NOT authorize placement or establish contact.
inline bool NormalAllowed(const Profile& p,Mode m,const Vector& n){
 if(!ModeAllowed(p,m)||!Finite(n))return false;
 if(m==Mode::Free)return true;
 constexpr float threshold=.2f;
 if(p.type==2)return std::abs(n[0])<threshold&&std::abs(n[1])<threshold&&n[2]<0;
 if(m==Mode::Wall)return std::abs(n[2])<threshold;
 return std::abs(n[0])<threshold&&std::abs(n[1])<threshold&&n[2]>0;
}
struct Contact { Vector point{},normal{}; };
// Map::DoCollisionCheck uses a double-sided triangle raycaster with raw
// winding normals. Face the normal toward this trusted query's origin.
// Do not use client packet bounds/normals or an untrusted chosen ray here.
inline std::optional<Contact> FromMapRay(const Vector& start,const Vector& end,const Vector& raw_normal,float distance){
 if(!Finite(start)||!Finite(end)||!Finite(raw_normal)||!std::isfinite(distance)||distance<0)return {};
 Vector direction{};for(unsigned i=0;i<3;++i)direction[i]=end[i]-start[i];
 const float length=std::sqrt(Dot(direction,direction));
 const float normal_length=std::sqrt(Dot(raw_normal,raw_normal));
 if(!std::isfinite(length)||!std::isfinite(normal_length)||length<=0||normal_length<=0||distance>length)return {};
 Contact c;for(unsigned i=0;i<3;++i){direction[i]/=length;c.normal[i]=raw_normal[i]/normal_length;c.point[i]=start[i]+direction[i]*distance;}
 const float facing=Dot(c.normal,direction);
 if(!std::isfinite(facing)||std::abs(facing)<1e-6f)return {}; // No source-facing side for a tangent.
 if(facing>0)for(auto& v:c.normal)v=-v;
 return Finite(c.point)&&Finite(c.normal)?std::optional<Contact>(c):std::nullopt;
}
// Recovered final projection only: 0x557937..0x55798c. Its caller supplies
// the reviewed renderer box's already transformed/scaled Z half-extent
// (first box at function entry stack+0x5c; final stack+0x7c).
// Full camera ray construction/retry and actor-box provider are not supplied.
// Mode::Free has a different caller path and is intentionally unsupported.
inline std::optional<Vector> NativeContactPosition(const Profile& p,Mode m,const Contact& c,float renderer_z_extent){
 if(m==Mode::Free||!NormalAllowed(p,m,c.normal)||!Finite(c.point)||!std::isfinite(renderer_z_extent)||renderer_z_extent<0)return {};
 Vector position=c.point;
 if(m!=Mode::Wall)position[2]+=p.type==2?-renderer_z_extent:renderer_z_extent;
 return Finite(position)?std::optional<Vector>(position):std::nullopt;
}
} // namespace Housing::PlacementContact
#endif
