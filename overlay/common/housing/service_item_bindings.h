#ifndef EQEMU_HOUSING_SERVICE_ITEM_BINDINGS_H
#define EQEMU_HOUSING_SERVICE_ITEM_BINDINGS_H
#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>
namespace Housing { namespace ServiceItemBindings {
// Source item keys/flags are authentic later collector records. Appearance
// choices are explicit server equivalents, not captured Live NPC appearance.
enum class Kind { Vendor, TeleportMerchant, Banker, GuildBank, GuildTribute, PersonalTribute, TeleportPad };
struct Binding { uint32_t item_id; const char* item_key; const char* actor_prefix; uint32_t race; uint8_t gender; uint32_t respawn_interval; uint32_t default_size_bits; uint32_t npc_type_id; Kind kind; int32_t max_per_estate; };
inline constexpr const char* SourceSha256="f5b0b81d3db6a72de9e907b09b36a4e4d8b93cd193ac8c786de94ffa92fac4e5";
inline constexpr uint32_t RespawnInterval=750;
inline constexpr std::array<Binding,7> Bindings{{
 {66639,"NGH_Merchant_General","GNF",12,1,750,1077936128u,345000,Kind::Vendor,-1},
 {66640,"NGH_Teleport_Assistant","GNM",12,0,750,1077936128u,345004,Kind::TeleportMerchant,-1},
 {66641,"NGH_Banker_Personal","HUF",1,1,750,1086324736u,345002,Kind::Banker,-1},
 {66642,"NGH_Banker_Guild","HUM",1,0,750,1086324736u,345003,Kind::GuildBank,-1},
 {66643,"NGH_Tribute_Guild","HUF",1,1,750,1086324736u,345001,Kind::GuildTribute,-1},
 {66644,"NGH_Tribute_Personal","HUF",1,1,750,1086324736u,345007,Kind::PersonalTribute,-1},
 {66646,"NGH_Teleport_Device","I28",706,2,750,1086324736u,345008,Kind::TeleportPad,1},
}};
inline const Binding* Lookup(uint32_t item){for(const auto& b:Bindings)if(b.item_id==item)return &b;return nullptr;}
inline bool Matches(const Binding& b,std::string_view key,bool interactive,uint32_t respawn,std::string_view sha,float raw){uint32_t bits;std::memcpy(&bits,&raw,4);return key==b.item_key&&interactive&&respawn==b.respawn_interval&&sha==SourceSha256&&bits==0;}
}}
#endif
