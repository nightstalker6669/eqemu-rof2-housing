#ifndef EQEMU_ZONE_HOUSING_HOUSE_RELOAD_H
#define EQEMU_ZONE_HOUSING_HOUSE_RELOAD_H
#include "../common/housing/house_binding_service.h"
#include "../common/housing/crate_binding.h"
#include <memory>
class Client;
class ZoneDatabase;
namespace Housing::HousePreload {
struct ZonePolicies {
 std::shared_ptr<const HouseBinding::HouseTemplatePolicy> templates;
 std::shared_ptr<const HouseBinding::InteriorRoutePolicy> routes;
 std::shared_ptr<const CrateBinding::ReloadPolicy> crates;
};
// Immutable reviewed content, installed once on the registered zone thread
// before any Client inventory admission. No packet or runtime callback setter.
bool ConfigureZonePoliciesAtSafeStartup(ZonePolicies);
ZonePolicies CurrentZonePolicies();
// Runs the real inventory loader between capture and checked hydration. No
// sends/scripts/yields; failure must reject admission before profile exposure.
bool LoadInventoryWithHouseBindings(Client&,ZoneDatabase&,bool housing_enabled);
}
#endif
