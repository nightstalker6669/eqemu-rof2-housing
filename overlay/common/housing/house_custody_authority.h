#ifndef EQEMU_HOUSING_HOUSE_CUSTODY_AUTHORITY_H
#define EQEMU_HOUSING_HOUSE_CUSTODY_AUTHORITY_H
#include "house_binding_service.h"
#include "house_operation_authority.h"
namespace Housing::HouseCustodyAuthority {
// REQUIRED reviewed server policy. No implementation/default is shipped. SQL
// identity and ACL do not prove original content, geometry, native metadata,
// destination/spawn, account feature/upkeep/capacity, or current ItemInstance
// HouseBindingID. This policy must attest them, including the phase-specific
// actual main-thread item/cache/wire context under the supplied held fence.
class ReviewedOperationPolicy {
public:
    virtual ~ReviewedOperationPolicy() = default;
    virtual bool VerifiedUnderTransaction(Database&, LiveItems::InventorySessionFence&,
        const HouseBinding::AuthorizationRequest&, const HouseAuthority::LockedContext&) const = 0;
};
class Authorizer final : public HouseBinding::CustodyAuthorization {
public:
    explicit Authorizer(const ReviewedOperationPolicy& policy) : policy_(policy) {}
    // Only own-creator Store/Return/Read. Called by TransactionHelpers AFTER its
    // genuine007/009/optional011 LockLease/NewParent checks in this SAME tx.
    // Parent original receipt binding/action must already be validated by its
    // service. This adapter does not interpret an invented binding_blob grammar.
    // No writes/transaction lifecycle, mint, transfer, admission or replay gate.
    bool AllowedUnderTransaction(Database&, LiveItems::InventorySessionFence&,
        const HouseBinding::AuthorizationRequest&) const override;
private:
    const ReviewedOperationPolicy& policy_;
};
}
#endif
