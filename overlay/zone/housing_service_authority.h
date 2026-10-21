#ifndef EQEMU_ZONE_HOUSING_SERVICE_AUTHORITY_H
#define EQEMU_ZONE_HOUSING_SERVICE_AUTHORITY_H
#include "housing_service_npcs.h"
#include <functional>
#include "housing_guild_portal.h"
#include "../common/housing/scene_service.h"
namespace Housing::ServiceAuthority {
// Requires a freshly authorized committed room scene. Preview/list requests
// cannot manufacture services; stowed and packed contents are excluded.
bool Reconcile(Client&,const Scene::InteriorSnapshot&) noexcept;
// Caller owns the SQL transaction. No nested bridge calls, lifecycle changes,
// sends, scripts, or mutation; optional Client adds current session/Enter checks.
bool Validate(Database&,const ServiceNpcs::Binding&,Client* = nullptr) noexcept;
bool ValidatePortalSource(Database&,Client&,const GuildPortalRuntime::Source&) noexcept;
std::vector<GuildPortalRuntime::Source> Sources(Client&,bool pads) noexcept;
bool PublishBindings(Client&,uint32_t native_estate_id,const std::function<std::string(uint32_t)>& get_guid) noexcept;
void Clear() noexcept;
}
#endif
