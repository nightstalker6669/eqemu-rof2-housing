#ifndef EQEMU_HOUSING_NPC_REVIEWED_ITEM_BINDINGS_H
#define EQEMU_HOUSING_NPC_REVIEWED_ITEM_BINDINGS_H
#include <cstdint>
#include <cstring>
#include <string_view>
namespace Housing::NpcReviewedBindings {
// Exact reviewed item-key/actor association. The item wire key is deliberately
// distinct from the first-three-character renderer token. Historical Live raw
// height is unknown: source0 selects an explicit override from original racedata.
// Wire6 avoids an unproved zero-scale setter path at504830.
inline constexpr std::string_view SourceSha256="4367281d3ea244b1f8eba86d5f8428535b0eb698f9cde375f822223444f74a00";
inline constexpr std::string_view RaceDataSha256="8c50fe1b68059164b6e93aa37c0c4649737b0006d3e9f2e071a714791824f49d";
struct Binding {uint32_t item_id;std::string_view item_key,actor_prefix;uint32_t race,gender,respawn_interval,default_size_bits;};
inline constexpr Binding Bindings[]={{77408,"a_holgresh","HLG",715,2,4500,0x40c00000u}};
inline constexpr const Binding* Lookup(uint32_t item){for(const auto& b:Bindings)if(b.item_id==item)return &b;return nullptr;}
inline bool Matches(const Binding& b,std::string_view key,bool interactive,uint32_t respawn,std::string_view sha,float raw){uint32_t bits;std::memcpy(&bits,&raw,4);return key==b.item_key&&!interactive&&respawn==b.respawn_interval&&sha==SourceSha256&&bits==0;}
}
#endif
