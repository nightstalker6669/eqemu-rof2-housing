#ifndef EQEMU_ZONE_HOUSING_HOUSE_RUNTIME_H
#define EQEMU_ZONE_HOUSING_HOUSE_RUNTIME_H
#include "housing_inventory_bridge.h"
#include "../common/housing/house_reviewed_content.h"
#include "housing_house_preload_fence.h"
namespace Housing::HouseRuntime {
// Explicit isolated-test selection; no default routes or adoption grants.
// A production configuration needs its own content, economy and feature review.
struct PersonalTestSelection {
 std::shared_ptr<const HouseReviewedContent::Policies> content;
 AuthenticatedPurchaser actor;
 uint32_t item_template_id=61823,guild_id=0; // Zero guild selects the personal route.
 EstateId estate_id=0;
 uint32_t native_plot_id=0;
 uint16_t neighborhood_instance=0;
 std::string catalog_key;
 bool experimental_no_upkeep=false;
 // Explicit reviewed personal content only; carries no actor, estate, source
 // adoption grant or guild identity. Live inventory/ACL authority is unchanged.
 bool template_content_only=false;
 std::shared_ptr<const InteriorContent::Profile> room_profile;uint64_t room_profile_revision=0;
};
// Explicit observed SERVER door registration; no guessed ID/default exists.
// Call at safe zone startup before creating Client bridges. Bound to this exact
// lobby route; false leaves the hook inactive. No client packet configures it.
bool ConfigureVerifiedGuildLobbyDoorAtSafeStartup(Route source,uint8_t door_id);
bool HandleVerifiedGuildLobbyDoor(Client&,uint8_t clicked_door,bool within_distance);
bool ConfigurePersonalTestAtSafeStartup(PersonalTestSelection);
// Optional legacy manifests plus .house-personal-content-registry-test.json.
// Registry entries each require their own route and room manifest.
// Absence preserves default-deny house support;
// invalid present configuration rejects startup rather than weakening policy.
bool LoadPersonalTestAtSafeStartup();
// Immutable room content recognition; grants no ACL or inventory authority.
bool ReviewedRoomForCurrentClient(const Scene::InteriorSnapshot&,const InteriorContent::Profile&,uint64_t revision);
InventoryBridge::Bridge::HousePolicies MakeForClient(Client&,LiveItems::InventorySessionFence&,HouseBinding::HouseApplicationProof&);
}
#endif
