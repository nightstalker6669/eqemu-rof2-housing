#ifndef EQEMU_ZONE_HOUSING_PETS_H
#define EQEMU_ZONE_HOUSING_PETS_H
#include <functional>
#include "../common/housing/npc_runtime.h"
#include "../common/housing/pet_commands.h"
#include "../common/housing/region_wire.h"
class Client;
namespace Housing::Pets {
struct Binding {
 uint64_t custody_id=0,pose_revision=0,estate_id=0,catalog_revision=0;
 uint32_t native_estate_id=0,native_item_id=0,owner=0;
 NpcRuntime::Profile profile;
 PlacementPolicy::Transform pose;
 std::optional<NpcNativeFrame::Frame> frame;
 std::string name;
 std::vector<RegionWire::Region> regions;
};
// Explicit generic server behavior for admitted housepets; not a claim that
// every historical item offered the same menu. Plushies/services are excluded.
bool Supported(const NpcRuntime::Profile&) noexcept;
bool Reconcile(uint32_t native_estate_id,const std::vector<Binding>&) noexcept;
bool Publish(Client&,uint32_t native_estate_id,const std::function<std::string(uint32_t)>&) noexcept;
bool Command(Client&,const PetCommands::Request&) noexcept;
std::optional<float> Distance(Client&,uint32_t estate,uint32_t item) noexcept;
void Process() noexcept;
void Clear() noexcept;
}
#endif
