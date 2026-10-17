#ifndef EQEMU_HOUSING_HOUSE_INVENTORY_TRANSITION_H
#define EQEMU_HOUSING_HOUSE_INVENTORY_TRANSITION_H
#include "house_binding_service.h"
#include "inventory_session_service.h"
#include "crate_binding.h"
namespace Housing::HouseInventory {
    struct SlotState {
        uint32_t slot=0;
        std::optional<LiveItems::InventoryRow> row;
    };
    struct Move {
        uint64_t house_id=0,expected_revision=0;
        uint32_t from_slot=0,to_slot=0;
        int32_t runtime_serial=0;
        uint64_t crate_id=0; // Mutually exclusive with house_id; ordinary companions have both IDs/revision0.
    };
    struct Request {
        std::string request_key;
        // server-issued houseinv: namespace, never a wire idempotency claim
        InventorySessions::Lease reservation;
        std::vector<SlotState> before,after;
        // sorted, complete affected slots; explicit empty states
        std::vector<Move> moves;
        // sorted source slots; explicit typed house/crate identity, or both IDs/revision0 for ordinary companions
    };
    struct Receipt {
        Request request;
        std::vector<HouseBinding::Binding> before,after;
        bool replayed=false,requires_application=true;
        std::vector<CrateBinding::Allocation> crates_before,crates_after;
    };
    enum class Error {
        None,Disabled,InvalidArgument,Unauthorized,UnheldReservation,
        PendingApplication,Conflict,NotFound,UnsupportedItem,DatabaseFailure,CommitUncertain
    };
    template<class T>struct Result {
        T value{
        };
        Error error=Error::None;
        explicit operator bool()const{
            return error==Error::None;
        }
    };
    class SessionFence {
        public:
        virtual ~SessionFence()=default;
        virtual bool HeldFor(const InventorySessions::Lease&)const noexcept=0;
        // Actual m_inv pointer/serial/full supported metadata + independent ledger ID,
        // contained estate property and empty destinations/cursor queue. No wire flags.
        virtual bool BeforeMatches(const Request&,const std::vector<HouseBinding::Binding>&)const noexcept=0;
        virtual bool BeforeMatches(const Request& r,const std::vector<HouseBinding::Binding>& h,const std::vector<CrateBinding::Allocation>& c)const noexcept{return c.empty()&&BeforeMatches(r,h);}
        virtual bool Applied(const Receipt&)const noexcept=0;
    };
    // Optional guard for ALL inventory admission/release/custody/economy writers.
    // Missing017 preserves legacy; schema/query failure is never clear. Activation
    // requires these hooks + real Client lifecycle/session fencing; not wired here.
    enum class PendingState {
        Clear,Pending,Unavailable
    };
    PendingState InspectPending(Database&,uint32_t character_id,bool lock=false) noexcept;
    // Read/lock only, inside the death reconciler's checked transaction after
    // exact process-reaping and old-session proof. Validates immutable receipt,
    // lease and committed SQL postimage; never claims a live memory application.
    Result<Receipt> ValidateRestartOutcome(Database&,const InventorySessions::Lease&) noexcept;
    // Exclusive owned transaction. No partial prepared state or retries. Unknown
    // COMMIT retains the real caller fence until fresh-connection resolution/ACK.
    class Service {
        public:
        explicit Service(std::unique_ptr<Database>,bool enabled=false);
        Result<Receipt> Relocate(const Request&,SessionFence&);
        Result<Receipt> Resolve(const InventorySessions::Lease&,const std::string& key,SessionFence&);
        Result<bool> AcknowledgeApplied(const InventorySessions::Lease&,const std::string& key,SessionFence&);
        private:
        std::unique_ptr<Database> database_;
        bool enabled_;
        std::mutex mutex_;
    };
}
#endif
