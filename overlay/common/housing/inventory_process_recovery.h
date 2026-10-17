#ifndef EQEMU_HOUSING_INVENTORY_PROCESS_RECOVERY_H
#define EQEMU_HOUSING_INVENTORY_PROCESS_RECOVERY_H
#include "inventory_session_service.h"
#include "live_item_service.h"

namespace Housing::InventoryRecovery {

// Called ONLY inside OpenLease's dedicated transaction. Optional absence and an
// unregistered process preserve the unsupervised path, which cannot be recovered.
// A registered process must be alive and the exact active preload owner must bind
// this custody lease. False rejects OpenLease; no independent COMMIT is performed.
bool LinkCustodyToReservation(Database&, const LiveItems::Lease&) noexcept;
// OpenLease acquires supervised process -> preload owner BEFORE custody locks,
// matching recovery order. A changed owner between lookup and lock rejects.
bool LockCustodyReservation(Database&, AuthenticatedPurchaser) noexcept;

// Zone Claim uses this for an opt-in supervisor-issued birth key. The supervisor
// persists the birth record BEFORE Popen and never reuses it for another child.
bool RegisteredLiveProcess(Database&, const std::string& process_key) noexcept;

// Read-only proof that a completed unapplied007 receipt was reconciled after
// exact writer death. Requires its unchanged outcome + released old history +
// immutable custody link + supervisor certificate. Never marks memory applied.
bool ReceiptDeathReconciled(Database&,uint32_t character_id,const std::string& request_key) noexcept;

enum class Error {
 None, Disabled, InvalidArgument, NoDeathProof, UnlinkedCustody,
 InvalidOutcome, Stale, DatabaseFailure, CommitUncertain
};
struct Reconciliation {
 uint32_t character_id = 0;
 uint64_t inventory_epoch = 0;
 std::string pending_key;
 bool replayed = false;
};
template<class T> struct Result {
 T value{};
 Error error = Error::None;
 explicit operator bool() const { return error == Error::None; }
};

// Exclusive connection; no transaction retry. Process death is accepted only
// from the optional trusted supervisor's persisted exact-child reaping record.
// This service cannot mint/mark death and offers no timestamp takeover API.
class Service {
public:
 explicit Service(std::unique_ptr<Database>, bool enabled = false);
 Result<Reconciliation> Reconcile(const InventorySessions::Lease& exact_old_lease);
private:
 std::unique_ptr<Database> database_;
 bool enabled_ = false;
 std::mutex mutex_;
};
}
#endif
