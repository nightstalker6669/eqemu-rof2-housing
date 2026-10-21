#ifndef EQEMU_HOUSING_GUILD_PORTAL_SERVICE_H
#define EQEMU_HOUSING_GUILD_PORTAL_SERVICE_H
#include <array>
#include <cstdint>
#include <cmath>
#include <optional>
#include <string>

namespace Housing::GuildPortal {
// Destination names and coordinates match the deployed basic guildhall quests.
// This is a server service contract, not an additional RoF2 housing opcode.
struct Destination { uint32_t stone; const char* name; uint32_t zone; float x,y,z; };
inline constexpr std::array<Destination,21> Destinations{{
 {10092,"The Plane of Hate",186,-393,656,3},
 {10094,"The Plane of Sky",71,539,1384,-664},
 {64191,"Dragonscale Hills",442,-1954,3916,19},
 {76000,"The Northern Plains of Karana",13,1209,-3685,-5},
 {76001,"East Commonlands",22,-140,-1520,3},
 {76002,"The Lavastorm Mountains",27,460,460,-86},
 {76003,"Toxxulia Forest",38,-916,-1510,-33},
 {76004,"The Greater Faydark",54,-441,-2023,4},
 {76005,"The Dreadlands",86,9658,3047,1052},
 {76006,"The Iceclad Ocean",110,385,5321,-17},
 {76007,"Cobalt Scar",117,-1634,-1065,299},
 {76009,"The Twilight Sea",170,-1028,1338,39},
 {76010,"Stonebrunt Mountains",100,673,-4531,0},
 {76011,"Wall of Slaughter",300,-943,13,130},
 {76012,"Barindu, Hanging Gardens",283,209,-515,-119},
 {76070,"The Undershore",362,-750,-1002,48},
 {88735,"Arcstone, Isle of Spirits",369,1630,-279,5},
 {88736,"Goru`kar Mesa",397,-85,-2050,19},
 {88738,"Katta Castrum",416,-2,-425,-19},
 {88739,"The Plane of Time",219,0,110,8},
 {88740,"Brell`s Rest",480,-23,-619,36}
}};
inline constexpr uint64_t AlignmentSeconds=86400;
inline constexpr uint64_t ConfirmationSeconds=30;
inline constexpr float ActivationRadius=10.0f;
inline const Destination* Lookup(uint32_t stone){for(const auto& d:Destinations)if(d.stone==stone)return &d;return nullptr;}
// Legacy guild-banner stone88737 has an explicit unimplemented 1,1,1,1
// placeholder. Do not charge for it or route players to that placeholder.
//76008 likewise has a hand-in branch but no destination in the basic quests.
inline bool Purchasable(uint32_t stone){return Lookup(stone)!=nullptr;}
struct Scope {uint64_t house=0;uint32_t estate=0,guild=0,zone=0,instance=0;};
inline bool Valid(const Scope& s){return s.house&&s.estate&&s.guild&&s.zone&&s.instance&&s.instance<=32000;}
inline bool Same(const Scope& a,const Scope& b){return a.house==b.house&&a.estate==b.estate&&a.guild==b.guild&&a.zone==b.zone&&a.instance==b.instance;}
inline std::string Key(const Scope& s){if(!Valid(s))return {};return "housing-guild-portal:"+std::to_string(s.house)+":"+std::to_string(s.estate)+":"+std::to_string(s.guild)+":"+std::to_string(s.zone)+":"+std::to_string(s.instance);}
struct Alignment {Scope scope;uint32_t stone=0;uint64_t expires=0,revision=0;};
inline bool Active(const Alignment& a,const Scope& current,uint64_t now){return Valid(current)&&Same(a.scope,current)&&Lookup(a.stone)&&a.revision&&now<a.expires;}
struct Point {float x=0,y=0,z=0;};
inline bool Near(const Point& a,const Point& b){if(!std::isfinite(a.x)||!std::isfinite(a.y)||!std::isfinite(a.z)||!std::isfinite(b.x)||!std::isfinite(b.y)||!std::isfinite(b.z))return false;const double x=double(a.x)-b.x,y=double(a.y)-b.y,z=double(a.z)-b.z;return x*x+y*y+z*z<=double(ActivationRadius)*ActivationRadius;}
// Per-client pending response. Never accept an arbitrary popup ID as authority:
// recheck entry permission, current placed pad, current alignment and distance.
struct Confirmation {Scope scope;uint32_t character=0,stone=0;uint64_t pad_custody=0,alignment_revision=0,expires=0;};
inline std::optional<Confirmation> Offer(const Alignment& a,const Scope& current,uint32_t character,uint64_t pad,const Point& client,const Point& position,uint64_t now){
 if(!character||!pad||!Active(a,current,now)||!Near(client,position)||now>UINT64_MAX-ConfirmationSeconds)return {};
 return Confirmation{current,character,a.stone,pad,a.revision,now+ConfirmationSeconds};
}
inline const Destination* Confirm(const Confirmation& p,const Alignment& a,const Scope& current,uint32_t character,uint64_t pad,const Point& client,const Point& position,uint32_t response,uint64_t now){
 if(!character||!pad||p.character!=character||p.pad_custody!=pad||!Same(p.scope,current)||p.stone!=response||p.alignment_revision!=a.revision||p.stone!=a.stone||now>=p.expires||!Active(a,current,now)||!Near(client,position))return nullptr;
 return Lookup(p.stone);
}
}
#endif
