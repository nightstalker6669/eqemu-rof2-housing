#ifndef EQEMU_ZONE_HOUSING_GUILD_PORTAL_H
#define EQEMU_ZONE_HOUSING_GUILD_PORTAL_H
#include "housing_service_npcs.h"
#include "../common/housing/guild_portal_service.h"
#include <functional>
#include <vector>
class Database;
class EQApplicationPacket;
namespace Housing::GuildPortalRuntime {
// Existing basic-hall Porter proximity point (spawn76577/NPC345005). A
// Standard Guild Hall can use its authored geode only when no placed device
// exists; the source validator proves that condition under the owning TX.
inline constexpr GuildPortal::Point StandardBuiltinGeode{-131.846436f,-23.525532f,1.487914f};
struct Source { ServiceNpcs::Binding binding; GuildPortal::Scope scope; bool builtin_pad=false; GuildPortal::Point pad_position{}; };
// Both callbacks derive authority from committed service items. The validator
// executes synchronously inside the owning inventory transaction: no bridge
// reentry, scripts, sends, event pumping or mutation is permitted.
using Resolver=std::function<std::vector<Source>(Client&,bool pads)>;
using Validator=std::function<bool(Database&,Client&,const Source&)>;
void Configure(Resolver,Validator);
void Clear() noexcept;
void Forget(Client&) noexcept;
bool HandIn(Client&,Mob*) noexcept;
void Process(Client&) noexcept;
bool HandlePopup(Client&,const EQApplicationPacket*) noexcept;
}
#endif
