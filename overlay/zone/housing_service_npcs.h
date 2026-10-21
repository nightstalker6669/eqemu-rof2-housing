#ifndef EQEMU_ZONE_HOUSING_SERVICE_NPCS_H
#define EQEMU_ZONE_HOUSING_SERVICE_NPCS_H
#include <cstdint>
#include <functional>
#include <optional>
#include <map>
#include <string>
#include <vector>
class Client;
class Mob;
namespace Housing::ServiceNpcs {
enum class Service : uint8_t { Vendor, Banker, PersonalTribute, GuildTribute, GuildBank, TeleportMerchant, TeleportPad };
// Produced from a committed placed item and reviewed item-to-NPC binding, never
// from a client-supplied spawn ID or an uncommitted furniture preview.
struct Binding {
 uint64_t house_id=0, estate_id=0, custody_id=0;
 std::string guid;
 uint32_t native_estate_id=0,native_item_id=0,item_id=0,npc_type_id=0,guild_id=0,zone_id=0;
 uint16_t instance_id=0;
 Service service=Service::Vendor;
 float x=0,y=0,z=0,heading=0;
};
using Revalidator=std::function<bool(const Binding&)>;
using AccessValidator=std::function<bool(Client&,const Binding&)>;
// Complete snapshot for THIS zone instance. Validate must reread current
// authority/custody/pose/house ownership (including active placement) on every
// call. False, an exception or a malformed snapshot revokes all managed NPCs.
// Call after placement/move/pickup/retirement commits and before zone spawn sync.
bool Reconcile(const std::vector<Binding>&,Revalidator,AccessValidator);
void Clear() noexcept;
// Called only after a known durable removal/storage commit. Revokes the exact
// existing runtime actor without requiring another scene read or source SQL.
void Revoke(uint64_t custody_id) noexcept;
bool Managed(const Mob*) noexcept;
// Runtime entity signature only, with NO source SQL or authority grant. A
// caller inside a custody TX must use ServiceAuthority::Validate on that same
// connection before consuming this witness; Find remains the action API.
std::optional<Binding> LiveBinding(const Mob*) noexcept;
std::vector<Binding> LiveBindings() noexcept;
std::optional<Binding> Find(const Mob*) noexcept;
std::vector<Binding> Bindings() noexcept;
// Per-recipient native item GUIDs come from the furniture list just published
// to that client; a local Actor binding is not authority for another client.
bool PublishBindings(Client&,const std::map<uint32_t,std::string>& native_item_guids) noexcept;
// Untagged existing NPCs retain upstream semantics. A managed NPC must still be
// the exact owned entity, at its committed pose, in this instance, with fresh
// authority and matching service. Guild-only services require the owning guild.
bool Allows(Client&,const Mob*,Service) noexcept;
bool HasNearby(Client&,Service) noexcept;
bool AllowsMerchant(Client&,const Mob*) noexcept;
// Send the native bank snapshot on authenticated zone entry, as the basic
// hall does; viewing needs owning-guild membership, not proximity.
bool PublishGuildBank(Client&) noexcept;
// GuildBank packets contain no NPC ID. Call before the existing GuildBank
// handler in housing instances; its normal rank/item restrictions remain.
bool EnsureGuildBank(Client&) noexcept;
}
#endif
