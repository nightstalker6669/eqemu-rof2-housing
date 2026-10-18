#ifndef EQEMU_HOUSING_LIVE_ESCROW_SERVICE_H
#define EQEMU_HOUSING_LIVE_ESCROW_SERVICE_H
#include "coin_wallet.h"
#include "escrow_wire.h"
#include "inventory_session_service.h"
#include "live_item_service.h"
namespace Housing::LiveEscrow {
    // All persisted profile currency, not a bank funding selector. Twelve coin
    // counters are signed int32; four crystal counters are uint32 in PlayerProfile.
    struct Currency {
        CoinWallet::Snapshot carried,bank,cursor;
        uint32_t radiant=0,career_radiant=0,ebon=0,career_ebon=0;
        bool operator==(const Currency& other)const noexcept {
            return carried==other.carried&&bank==other.bank&&cursor==other.cursor&&radiant==other.radiant&&
            career_radiant==other.career_radiant&&ebon==other.ebon&&career_ebon==other.career_ebon;
        }
    };
    // Server-captured exact occupied source. Native escrow still names its
    // current parent yard; no child estate substitution or packet extension.
    struct InteriorSource {uint64_t house_id=0,house_revision=0;uint32_t native_house_id=0;Route physical;};
    enum class Funding : uint8_t { Cursor=0, Carried=1 };
    struct Request {
        std::string request_key;
        // Server issued; native2aa6 has no operation ID.
        LiveItems::Lease lease;
        EstateId estate_id=0;
        EscrowWire::Message wire;
        Currency expected;
        // SERVER REVIEW ONLY: no native packet field. Requires an exact unseeded
        // generation/catalog policy; carried copper is not claimed Live behavior.
        bool occupied_refund=false;
        Funding funding=Funding::Cursor; // Server-only compatibility funding selector.
        std::optional<InteriorSource> interior_source;
    };
    struct Receipt {
        std::string request_key;
        LiveItems::Lease lease;
        InventorySessions::Lease reservation;
        EstateId estate_id=0;
        uint32_t native_plot_id=0;
        EscrowWire::Action action=EscrowWire::Action::Deposit;
        CoinWallet::Denomination denomination=CoinWallet::Denomination::Copper;
        int64_t quantity=0,copper_transferred=0,escrow_before=0,escrow_after=0;
        Currency before,after;
        bool occupied_refund=false;
        Funding funding=Funding::Cursor;
        uint64_t catalog_revision=0,daily_copper=0;
        std::array<uint8_t,32> funding_witness{};
        bool replayed=false,requires_application=true;
    };
    enum class Error {
        None,Disabled,InvalidArgument,UnheldFence,Unauthorized,NotFound,
        StaleLease,PendingApplication,SnapshotMismatch,Conflict,InsufficientFunds,Overflow,
        DatabaseFailure,CommitUncertain
    };
    template<class T>struct Result {
        T value{
        };
        Error error=Error::None;
        explicit operator bool()const{
            return error==Error::None;
        }
    };
    // SERVER-ONLY extension of the existing synchronous inventory fence. It must
    // prevent ALL currency saves/mutations/callbacks/lifecycle until application/ACK,
    // including uncertain-COMMIT resolution. No packet boolean establishes it.
    class CoinSessionFence:public LiveItems::InventorySessionFence {
        public:
        virtual bool CurrencyMatches(const Currency&)const noexcept=0;
        virtual bool CursorItemsEmpty()const noexcept=0;
        // Includes queued items/bag children.
        virtual bool CoinApplied(const Receipt&)const noexcept=0;
        virtual bool InteriorCoinSourceMatches(const InteriorSource&)const noexcept{return false;}
    };
    // Cursor transfers and explicitly reviewed carried-only bulk transfers.
    // Fresh personal owner / actual guild implicit owner
    // deposits; legacy delegated personal WithdrawUpkeep remains supported.
    // Unknown delegated deposit permission remains closed, including bit0.
    class Service {
        public:
        explicit Service(std::unique_ptr<Database>,bool enabled=false,const Scene::HouseScenePolicy* =nullptr);
        Result<Receipt> Transfer(const Request&,CoinSessionFence&);
        // Caller owns TX and must pair this key with actual item return+retirement.
        // Does not commit, acknowledge, apply profile, or bypass currency fencing.
        static Result<Receipt> RefundOccupiedUnderTransaction(Database&,const Request&,CoinSessionFence&,const Scene::HouseScenePolicy* =nullptr);
        static Result<Receipt> ValidateBulkRestartUnderTransaction(Database&,const InventorySessions::Lease&,const LiveItems::Lease&);
        Result<Receipt> Resolve(const Request&,CoinSessionFence&);
        // Requires exact certified writer death; validates actual item AND all coin
        // postconditions. No COMMIT, live memory ACK or independent replay mutation.
        static Result<Receipt> ValidateOccupiedRefundRestartUnderTransaction(Database&,const InventorySessions::Lease&,const LiveItems::Lease&);
        Result<bool> AcknowledgeApplied(const Request&,CoinSessionFence&);
        private:
        std::unique_ptr<Database> database_;
        bool enabled_=false;
        const Scene::HouseScenePolicy* houses_=nullptr;
        std::mutex mutex_;
    };
}
#endif
