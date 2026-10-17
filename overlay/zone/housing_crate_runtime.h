#ifndef EQEMU_ZONE_HOUSING_CRATE_RUNTIME_H
#define EQEMU_ZONE_HOUSING_CRATE_RUNTIME_H
#include "../common/housing/crate_binding.h"
class Client;
namespace Housing::HouseRuntime {struct PersonalTestSelection;}
namespace Housing::CrateRuntime {
bool LoadAtSafeStartup();
std::shared_ptr<const CrateBinding::ReloadPolicy> ReloadPolicy(const std::vector<std::shared_ptr<const HouseRuntime::PersonalTestSelection>>& rooms={});
std::unique_ptr<CrateBinding::CustodyContext> MakeForClient(Client&,LiveItems::InventorySessionFence&,std::shared_ptr<const HouseRuntime::PersonalTestSelection> room=nullptr);
}
#endif
