#ifndef EQEMU_HOUSING_CURRENT_ESTATE_ACCESS_H
#define EQEMU_HOUSING_CURRENT_ESTATE_ACCESS_H
#include "persistence_service.h"
#include "access_policy.h"
namespace Housing {
struct CurrentEstateAccess {
 OwnerKind owner_kind=OwnerKind::Character;
 uint32_t owner_id=0;
 PlotId plot_id=0;
 uint64_t acl_revision=0;
 AccessPolicy::EffectiveAccess effective;
};
// SERVER caller already owns an active SQL transaction and genuine held lease.
// Locks current domain/mapping/actor/membership/ACL rows. No transaction lifecycle
// or writes. This resolves ONLY exterior personal4/guild8 domain ACL semantics;
// caller must still prove canonical definition/route/context/content and operation.
// Estate ownership and inventory creator/recipient ownership remain independent.
Result<CurrentEstateAccess> LoadCurrentEstateAccessUnderTransaction(
 Database&,EstateId,uint32_t canonical_native_plot,AuthenticatedPurchaser);
// Read-only world guild-trophy billing audience check. The caller is the world
// guild service and supplies its genuine InZone ClientList account/character.
// This grants no inventory/placement/admission authority or session lease.
// Current guild membership, leader identity, account and Enter ACL are reread.
Result<CurrentEstateAccess> LoadGuildTrophyRecipientAccessUnderTransaction(Database&,EstateId,uint32_t canonical_native_plot,AuthenticatedPurchaser);
}
#endif
