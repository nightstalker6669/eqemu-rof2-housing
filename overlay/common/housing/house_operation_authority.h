#ifndef EQEMU_HOUSING_HOUSE_OPERATION_AUTHORITY_H
#define EQEMU_HOUSING_HOUSE_OPERATION_AUTHORITY_H
#include "current_estate_access.h"
#include "estate_wire.h"
#include <optional>
namespace Housing::HouseAuthority {
enum class Operation : uint8_t { Add = 1, Remove = 2 };
struct Request {
    Operation operation = Operation::Add;
    AuthenticatedPurchaser actor;
    EstateId estate_id = 0;
    uint32_t native_plot_id = 0, item_template_id = 0;
    // Remove requires the CURRENT contained native house estate. Add of an
    // unbound token may use zero; existing bindings must be supplied by caller.
    uint32_t native_house_id = 0;
    uint64_t house_id = 0; // Durable server ledger identity, not a GUID/custody ID.
};
struct LockedContext {
    Request request;
    CurrentEstateAccess access;
    uint64_t catalog_revision = 0;
    uint32_t neighborhood_id = 0, zone_id = 0;
    uint16_t instance_id = 0;
    uint8_t zone_version = 0;
    std::string catalog_key;
    EstateWire::Definition plot_definition, house_definition;
    uint32_t item_placement_mask = 0;
};
struct TrustedItemProof {
    uint32_t creator_character_id = 0;
    // Required in addition to RemoveHouse for another creator's token. Must
    // attest actual CanDrop/lore/recipient policy, including contained items.
    bool other_creator_removal_allowed = false;
};
class TrustedWitness {
public:
    virtual ~TrustedWitness() = default;
    // REQUIRED SERVER integration, no default implementation. Under this same
    // transaction verify a genuine held session/context; exact current item,
    // house ledger/custody/GUID/creator; immutable reviewed template/geometry,
    // destination/spawn/content; feature, child relationship/capacity/upkeep
    // and all other operation constraints. Never attest from packet names,
    // declared guild/rank, UNK227 alone or a guessed template-zone pairing.
    // Return nullopt if unsupported. A successful SQL ACL check is NOT a mint,
    // transfer, lease/admission or historical-content authentication witness.
    virtual std::optional<TrustedItemProof> VerifyUnderTransaction(
        Database&, const LockedContext&) const = 0;
};
struct Decision {
    LockedContext context;
    uint32_t creator_character_id = 0;
    bool creator_removal_bypass = false;
};
// Caller-owned checked SQL transaction, same genuine inventory/session fence.
// Locks/read-only; NEVER begins/commits/rolls back or changes inventory/custody.
// On failure caller must roll back its WHOLE transaction. No activated caller.
Result<Decision> AuthorizeUnderTransaction(Database&, const Request&, const TrustedWitness&);
}
#endif
