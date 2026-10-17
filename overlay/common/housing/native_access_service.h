#ifndef EQEMU_HOUSING_NATIVE_ACCESS_SERVICE_H
#define EQEMU_HOUSING_NATIVE_ACCESS_SERVICE_H
#include "live_item_service.h"
#include "permission_wire.h"
namespace Housing::NativeAccess {
    enum class Error {
        None, Disabled, InvalidArgument, UnheldFence, Unauthorized, NotFound,
        StaleLease, PendingInventory, Conflict, DatabaseFailure, CommitUncertain
    };
    template<class T> struct Result {
        T value{};
        Error error = Error::None;
        explicit operator bool() const {
            return error == Error::None;
        }
    };
    struct Snapshot {
        EstateId estate_id = 0;
        uint32_t native_plot_id = 0;
        uint64_t revision = 0;
        PermissionWire::Snapshot wire;
        AccessPolicy::EffectiveAccess actor_access;
    };
    struct Request {
        // Server-issued key for this received request. Native3146 has no request ID or
        // ACL revision; expected_revision comes from the server's sent snapshot cache.
        std::string request_key;
        LiveItems::Lease lease;
        EstateId estate_id = 0;
        uint64_t expected_revision = 0;
        PermissionWire::Snapshot proposal;
    };
    struct Receipt {
        std::string request_key;
        Snapshot committed;
        bool replayed = false;
    };
    // Personal/guild exterior plots only. Dedicated exclusive connection, default
    // disabled; no native packet bindings. Fellowship grants are typed/persisted but
    // cannot elevate access without an authoritative membership adapter.
    class Service {
        public:
        explicit Service(std::unique_ptr<Database>, bool enabled = false);
        Result<Snapshot> Read(const LiveItems::Lease&, EstateId, uint32_t native_plot_id,
        LiveItems::InventorySessionFence&);
        Result<Receipt> Replace(const Request&, LiveItems::InventorySessionFence&);
        // Exact historical own receipt under a genuine current session fence. Does not
        // repeat a mutation or require newly revoked Manage/estate ownership. The caller
        // must reread current ACL before sending fresh state; old outcomes are historical.
        Result<Receipt> Resolve(const Request&, LiveItems::InventorySessionFence&);
        private:
        std::unique_ptr<Database> database_;
        bool enabled_ = false;
        std::mutex mutex_;
    };
}
#endif
