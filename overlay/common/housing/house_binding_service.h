#ifndef EQEMU_HOUSING_HOUSE_BINDING_SERVICE_H
#define EQEMU_HOUSING_HOUSE_BINDING_SERVICE_H

#include "live_item_service.h"
#include "estate_wire.h"
#include <stdexcept>

namespace Housing::HouseBinding {
// Domain IDs are independent of GUIDs, custody IDs and native placed IDs.
using HouseId = uint64_t;
enum class Error { InvalidArgument, NoTransaction, DatabaseFailure, Unauthorized,
    UnheldFence, Conflict, PendingApplication, NotFound, UnsupportedItem };
class Failure : public std::runtime_error {
public:
    explicit Failure(Error e) : std::runtime_error("Housing house binding rejected"), error(e) {}
    Error error;
};
struct Location {
    enum class Kind : uint8_t { Inventory = 1, Custody = 2 };
    Kind kind = Kind::Inventory;
    uint32_t character_id = 0, slot_id = 0;
    uint64_t custody_id = 0;
};
struct Interior {
    uint32_t native_estate_id = 0, definition_id = 0, zone_id = 0;
    uint16_t instance_id = 0;
    uint8_t zone_version = 0;
    std::string catalog_key;
    uint64_t catalog_revision = 0;
};
// Must validate a reviewed template/destination/version/spawn/content manifest.
// Canonical definition zone is necessary, not sufficient. No production witness
// is shipped; filenames/dzId/FK presence never satisfy this contract.
class InteriorRoutePolicy {
public:
    virtual ~InteriorRoutePolicy() = default;
    virtual bool VerifiedDestination(const Interior&, const EstateWire::Definition&, uint32_t item_template_id) const noexcept = 0;
};
struct Binding {
    HouseId house_id = 0;
    uint64_t revision = 0;
    uint32_t item_template_id = 0;
    Location location;
    std::optional<Interior> interior;
    std::string pending_key;
};
enum class AuthorizationPhase : uint8_t {
    StoreAfterCustodyInsert, ReturnBeforeInsert, ReturnAfterInsert, ReadStored
};
struct AuthorizationRequest {
    AuthorizationPhase phase = AuthorizationPhase::ReadStored;
    LiveItems::Lease lease;
    std::string request_key;
    Binding before;
    LiveItems::InventoryRow source;
    uint64_t custody_id = 0;
    EstateId estate_id = 0;
    uint32_t destination_slot = 0;
    int32_t runtime_serial = 0;
    std::optional<Interior> first_interior;
};
// Optional SERVER integration for fresh house-specific personal/guild authority.
// Called only after this helper's actual held 007/009/011 checks in the SAME
// transaction. A policy must independently attest exact context/content/session,
// native AddHouse/RemoveHouse and immutable creator; no permissive implementation.
// False rejects the whole caller transaction. Exceptions also require rollback.
// The policy must outlive TransactionHelpers and cannot own/commit this tx.
class CustodyAuthorization {
public:
    virtual ~CustodyAuthorization() = default;
    virtual bool AllowedUnderTransaction(Database&, LiveItems::InventorySessionFence&,
        const AuthorizationRequest&) const = 0;
};
// Server catalog/content witness; never implement from a packet flag, filename,
// UNK227 guess or item GUID. No implementation or catalog is enabled here.
class HouseTemplatePolicy {
public:
    virtual ~HouseTemplatePolicy() = default;
    virtual bool VerifiedHouse(const LiveItems::InventoryRow&) const noexcept = 0;
    // Explicit initial-adoption/mint provenance, not generic copy recognition.
    // Catalog recognition alone cannot prove an unbound row is not a copy.
    virtual bool PermitInitialBinding(AuthenticatedPurchaser,
        const LiveItems::InventoryRow&) const noexcept { return false; }
};
struct Effect {
    Binding before, after;
    LiveItems::Action action = LiveItems::Action::Store;
    LiveItems::InventoryRow source, destination;
    std::string request_key;
    bool minted = false, replayed = false, requires_application = true;
    // v1 retains the complete interior tuple. v2 records a reviewed Store
    // changing only its catalog provenance; permanent physical identity stays.
    uint32_t encoding_version = 1;
};
inline bool SamePhysicalInterior(const Interior& a, const Interior& b) {
    return a.native_estate_id == b.native_estate_id && a.definition_id == b.definition_id
        && a.zone_id == b.zone_id && a.instance_id == b.instance_id && a.zone_version == b.zone_version;
}
inline bool ValidInteriorTransition(const Effect& e) {
    if (e.encoding_version != 1 && e.encoding_version != 2) return false;
    if (!e.before.interior) return e.encoding_version == 1;
    if (!e.after.interior || !SamePhysicalInterior(*e.before.interior, *e.after.interior)) return false;
    const bool same_catalog = e.before.interior->catalog_key == e.after.interior->catalog_key
        && e.before.interior->catalog_revision == e.after.interior->catalog_revision;
    return e.encoding_version == 1 ? same_catalog
        : !same_catalog && e.action == LiveItems::Action::Store && !e.minted;
}
// Additional memory proof while the SAME inventory fence is held. This is not
// a second lease: the adapter must verify authoritative house-property changes
// (or unchanged binding for pickup), not only the parent's raw17 projection.
class HouseApplicationProof {
public:
    virtual ~HouseApplicationProof() = default;
    virtual bool AppliedHouseEffect(const Effect&) const noexcept = 0;
};
// Optional per-server custody integration. All policies/callbacks are server
// owned and must outlive Service. Allocation runs within the parent's checked
// transaction; it may not commit, pump callbacks or infer routing from wire IDs.
enum class PlacementStage { Preview, Store, Read, Move };
struct PlacementFacts {
    LiveItems::Lease lease;
    LiveItems::Reservation reservation;
    LiveItems::InventoryRow source;
    int32_t runtime_serial=0;
    std::optional<PlacementPolicy::Transform> pose;
    PlacementPolicy::Operation operation=PlacementPolicy::Operation::Add;
    PlacementStage stage=PlacementStage::Preview;
    uint64_t catalog_revision=0,placed_children=0;
    bool storage_only=false;
    uint16_t neighborhood_instance=0;
    AccessPolicy::EffectiveAccess access;
    EstateWire::Definition plot_definition,house_definition;
    RegionWire::Region region;
};
class CustodyContext {
public:
    virtual ~CustodyContext() = default;
    virtual const HouseTemplatePolicy& Templates() const = 0;
    virtual const InteriorRoutePolicy* Routes() const { return nullptr; }
    virtual const CustodyAuthorization* Authorization() const { return nullptr; }
    virtual HouseApplicationProof& Application() = 0;
    // Required server review of this exact locked context: actual item/house
    // identity and adoption provenance, feature/upkeep, native classification,
    // source/scene/pose evidence and immutable interior manifest. No writes,
    // transaction lifecycle, scripts or event-loop pumping here. Default denies.
    virtual std::optional<ContentBinding::HouseRoute> ReviewPlacementUnderTransaction(
        Database&,LiveItems::InventorySessionFence&,const PlacementFacts&) const {
        return std::nullopt;
    }
    virtual std::optional<Interior> AllocateInterior(Database&,const Binding&,
        const LiveItems::Request&) { return std::nullopt; }
    // After FinishStore has persisted the permanent interior, still inside
    // the original parent transaction. Optional registered room content must
    // commit or roll back with that house; no application or transaction pump.
    virtual bool FinalizeInteriorUnderTransaction(Database&,LiveItems::InventorySessionFence&,
        const Effect&,const LiveItems::Request&) const { return true; }
    // No inventory house application occurs here: a checked parent crate
    // Store moves a retained custody house and its permanent interior graph.
    virtual std::optional<ContentBinding::HouseRoute> ReviewCrateRelocationUnderTransaction(
        Database&,LiveItems::InventorySessionFence&,const Binding&,
        const LiveItems::Request&,const LiveItems::InventoryRow&) const{return std::nullopt;}
};
class Prepared {
public:
    const Binding& Before() const { return before_; }
private:
    friend class TransactionHelpers;
    Binding before_;
    LiveItems::Lease lease_;
    LiveItems::InventoryRow source_;
    LiveItems::Action action_ = LiveItems::Action::Store;
    std::string key_;
    bool minted_ = false;
    uint32_t destination_slot_ = 0;
    int32_t runtime_serial_ = 0;
};

// Read/lock-only validator for certified dead-owner reconciliation. Caller must
// already hold exact process/session/custody death/link locks in a checked tx.
// Does not grant live ownership, apply memory, clear pending or mark applied.
Effect ValidateRestartOutcome(Database&,const LiveItems::Lease&,const LiveItems::Receipt&);
// Internal retained-layout child only; immutable parent/component proof is
// supplied by LayoutRetained before using this alternate pending-key contract.
Effect ValidateRestartOutcome(Database&,const LiveItems::Lease&,const LiveItems::Receipt&,const std::string* aggregate_pending);

// Dedicated exclusive connection, caller-owned checked transaction. Helpers
// NEVER BEGIN/COMMIT/ROLLBACK. Any exception requires rollback of the WHOLE
// caller transaction (including parent custody writes). Do not catch and commit.
// COMMIT uncertainty/fresh-connection resolution remains the caller's duty.
class TransactionHelpers {
public:
    TransactionHelpers(Database&, LiveItems::InventorySessionFence&, const CustodyAuthorization* = nullptr);
    // A mint is part of this Store's existing incomplete parent receipt, not a
    // separately committed adoption operation. expected_house=0 requires policy.
    Prepared PrepareStore(const LiveItems::Lease&, const std::string& request_key,
        const LiveItems::InventoryRow&, int32_t runtime_serial,
        HouseId expected_house, uint64_t expected_revision, const HouseTemplatePolicy&);
    Prepared PrepareReturn(const LiveItems::Lease&, const std::string& request_key,
        HouseId, uint64_t expected_revision, uint64_t custody_id,
        const LiveItems::InventoryRow&, uint32_t destination_slot);
    // Store: custody_id must be parent's freshly inserted auto-ID, not a
    // packet/source reference. Call after custody INSERT + exact source DELETE, before parent
    // receipt completion/pending. Interior rows must already exist in THIS tx.
    Effect FinishStore(const Prepared&, uint64_t custody_id,
        const std::optional<Interior>& first_interior = std::nullopt,
        const InteriorRoutePolicy* = nullptr);
    // Return: call after exact destination INSERT, BEFORE custody DELETE (FK).
    Effect FinishReturn(const Prepared&, const LiveItems::InventoryRow& destination);
    // Parent receipt must already be decoded/validated by its original service.
    // Historical applied replay checks immutable bindings, never current location.
    Effect Resolve(const LiveItems::Lease&, const LiveItems::Receipt&);
    // Call AFTER parent's receipt-applied + lease-clear SQL, but BEFORE COMMIT.
    // Requires fence.Applied(parent) AND house-property proof for pending ACK.
    // Historical ACK does not require obsolete memory postimages. No current
    // estate permission on own ACK.
    void ClearAfterParentAcknowledgement(const LiveItems::Lease&, const LiveItems::Receipt&, HouseApplicationProof&);
    Binding ReadInventory(const LiveItems::Lease&, uint32_t slot);
    Binding ReadCustody(const LiveItems::Lease&, uint64_t custody_id);
private:
    Database& db_;
    LiveItems::InventorySessionFence& fence_;
    const CustodyAuthorization* authorization_;
    void Authorize(const AuthorizationRequest&);
};
}
#endif
