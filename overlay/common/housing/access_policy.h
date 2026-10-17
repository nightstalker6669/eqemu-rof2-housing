#ifndef EQEMU_HOUSING_ACCESS_POLICY_H
#define EQEMU_HOUSING_ACCESS_POLICY_H
#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Housing::AccessPolicy {
inline constexpr uint32_t NoGuild=0xffffffff;
inline constexpr uint32_t KnownCapabilities=0x1fff;
inline constexpr std::array<uint32_t,7> NativeDefaults{3,3,0x803,0xe03,0x1f87,0x1fff,0};
enum class PlotType:uint8_t { Personal=4, Guild=8 };
enum class SubjectKind:uint8_t { Character, GuildRank, Fellowship };
enum class Capability:uint8_t {
 ReceiveBuffs, Enter, WithdrawUpkeep, RemoveCrate, AddCrate, RemoveHouse,
 AddHouse, MailItem, RemoveItem, AddItem, MoveItem, InteractItem, Manage
};
struct Subject {
 SubjectKind kind=SubjectKind::Character;
 // Character: authoritative character ID. GuildRank: rank1..8. Fellowship:0.
 uint32_t identity=0;
};
struct Assignment { Subject subject; uint8_t group=0; };
struct Acl {
 std::array<uint32_t,7> masks=NativeDefaults;
 std::vector<Assignment> assignments;
};
// This function accepts an ALREADY RESOLVED plot only. Native houses/crates
// use their plot ancestor. The caller must resolve/validate that chain and may
// not substitute Neighborhood/Town or an unrelated plot after a failed lookup.
struct ResolvedPlot {
 uint32_t estate_id=0;
 PlotType type=PlotType::Personal;
 uint32_t owner_character_id=0;
 uint32_t guild_id=NoGuild;
 uint32_t fellowship_id=0;
};
// Populate from authenticated, current server state, never packet-declared IDs.
// These booleans are preconditions, not a database/session authentication check.
struct TrustedActor {
 uint32_t character_id=0;
 uint32_t guild_id=NoGuild;
 uint8_t guild_rank=0;
 uint32_t fellowship_id=0;
 bool authenticated=false;
 bool feature_allowed=false;
};
struct EffectiveAccess {
 uint8_t group=0;
 uint32_t capabilities=0;
 bool implicit_owner=false;
 bool Allows(Capability capability) const {
  const auto bit=static_cast<uint8_t>(capability);
  return bit<13 && (capabilities & (uint32_t{1}<<bit));
 }
};
inline bool PositiveIdentity(uint32_t id){return id && id!=NoGuild;}
// Pinned RoF2 serializer3012 and4866 send server guildrank unchanged. The old
// Titanium mapping is commented out. Validate today's stored RoF2 ranks1..8.
inline std::optional<uint8_t> NativeGuildRankFromServer(uint32_t rank){
 if(rank<1 || rank>8)return std::nullopt;return static_cast<uint8_t>(rank);
}
inline bool ValidSubject(const Subject& s){
 switch(s.kind){
 case SubjectKind::Character:return PositiveIdentity(s.identity);
 case SubjectKind::GuildRank:return NativeGuildRankFromServer(s.identity).has_value();
 case SubjectKind::Fellowship:return s.identity==0;
 }
 return false;
}
// For reserved keys only: normal character names must be resolved independently
// from character ID by the integration layer, and cannot impersonate these keys.
inline std::optional<std::string> ReservedWireKey(const Subject& s){
 if(!ValidSubject(s))return std::nullopt;
 if(s.kind==SubjectKind::Fellowship)return "_Fellowship";
 if(s.kind==SubjectKind::GuildRank)return "_Guild"+std::to_string(s.identity);
 return std::nullopt;
}
inline bool ValidAcl(const Acl& acl){
 for(uint32_t mask:acl.masks)if(mask&~KnownCapabilities)return false;
 if(acl.masks[6])return false; // Native implicit owner, never a named grant.
 std::set<std::pair<uint8_t,uint32_t>> unique;
 for(const auto& a:acl.assignments){
  if(a.group>5 || !ValidSubject(a.subject) ||
   !unique.emplace(static_cast<uint8_t>(a.subject.kind),a.subject.identity).second)return false;
 }
 return true;
}
inline std::optional<EffectiveAccess> Resolve(const ResolvedPlot& plot,
 const TrustedActor& actor,const Acl* acl){
 if(!PositiveIdentity(plot.estate_id) || !PositiveIdentity(actor.character_id) ||
  !actor.authenticated || !actor.feature_allowed ||
  (plot.type!=PlotType::Personal && plot.type!=PlotType::Guild))return std::nullopt;
 // No-guild actors use the explicit sentinel and rank0. Invalid/inconsistent
 // caller state fails closed rather than resolving elevated guild subjects.
 if((actor.guild_id==NoGuild && actor.guild_rank!=0) ||
  (actor.guild_id!=NoGuild && (!PositiveIdentity(actor.guild_id) ||
   !NativeGuildRankFromServer(actor.guild_rank))))return std::nullopt;
 if((plot.guild_id!=NoGuild && !PositiveIdentity(plot.guild_id)) ||
  plot.fellowship_id==NoGuild || actor.fellowship_id==NoGuild ||
  plot.owner_character_id==NoGuild)return std::nullopt;
 const bool same_guild=PositiveIdentity(plot.guild_id) && actor.guild_id==plot.guild_id;
 const bool owner=(plot.type==PlotType::Personal &&
  PositiveIdentity(plot.owner_character_id) && actor.character_id==plot.owner_character_id) ||
  (plot.type==PlotType::Guild && same_guild && actor.guild_rank==1);
 // Native4ff650 returns owner access before fetching the access object.
 if(owner)return EffectiveAccess{6,KnownCapabilities,true};
 if(!acl || !ValidAcl(*acl))return std::nullopt;
 uint8_t character_group=0, fellowship_group=0, guild_group=0;
 for(const auto& a:acl->assignments){
  switch(a.subject.kind){
  case SubjectKind::Character:
   if(a.subject.identity==actor.character_id)character_group=a.group;break;
  case SubjectKind::Fellowship:
   if(actor.fellowship_id && actor.fellowship_id==plot.fellowship_id)fellowship_group=a.group;break;
  case SubjectKind::GuildRank:
   if(same_guild && a.subject.identity==actor.guild_rank)guild_group=a.group;break;
  }
 }
 // Explicit nonzero assignment wins even over a stronger special assignment.
 // Named group0 falls through; matching special groups choose the larger rank.
 const uint8_t group=character_group?character_group:
  (guild_group>fellowship_group?guild_group:fellowship_group);
 return EffectiveAccess{group,acl->masks[group],false};
}
} // namespace Housing::AccessPolicy
#endif
