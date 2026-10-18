#ifndef EQEMU_HOUSING_GUILD_TROPHY_WIRE_H
#define EQEMU_HOUSING_GUILD_TROPHY_WIRE_H
#include "trophy_anchor_wire.h"
#include <string>
#include <set>
namespace Housing::GuildTrophyWire {
using TrophyAnchorWire::Detail::Read;using TrophyAnchorWire::Detail::Write;
struct Bank {std::array<uint32_t,4> ids{UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};std::array<int32_t,4> tiers{};};
inline std::optional<bool> Toggle(const uint8_t* p,size_t size){if(!p||size!=12)return {};auto v=Read(p);if(v>1)return {};return bool(v);} // tail is uninitialized native stack, never identity.
inline std::optional<bool> Modify(const uint8_t* p,size_t size){if(!p||size!=80||p[0]>1)return {};return bool(p[0]);} // only byte0 is initialized/proved.
inline std::optional<Bank> Save(const uint8_t* p,size_t size){if(!p||size!=68)return {};Bank b;for(size_t i=0;i<4;++i){b.ids[i]=Read(p+20+4*i);b.tiers[i]=int32_t(Read(p+36+4*i));}return b;} // other bytes remain opaque.
struct OptIn {bool enabled=false;std::string name;};
inline std::optional<OptIn> Member(const uint8_t* p,size_t size){if(!p||size!=80||Read(p+4)>1)return {};size_t n=0;while(n<64&&p[8+n])++n;if(!n||n==64)return {};std::string name(reinterpret_cast<const char*>(p+8),n);for(auto c:name)if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')))return {};return OptIn{bool(Read(p+4)),name};} // guild/reference and tail never confer authority.
inline std::vector<uint8_t> Available(uint32_t guild,const std::set<uint32_t>& ids){std::vector<uint8_t> p;Write(p,guild);Write(p,uint32_t(ids.size()));for(auto id:ids)Write(p,id);return p;}
inline std::vector<uint8_t> Active(uint32_t guild,uint32_t favor,uint32_t remaining_ms,bool enabled,const Bank& b){std::vector<uint8_t> p;Write(p,guild);Write(p,favor);Write(p,remaining_ms);Write(p,uint32_t(enabled));Write(p,0);for(auto id:b.ids)Write(p,id);for(auto tier:b.tiers)Write(p,uint32_t(tier));return p;} // consumer through byte51, enabled is byte12.
}
#endif
