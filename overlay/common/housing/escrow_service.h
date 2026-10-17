#ifndef EQEMU_HOUSING_ESCROW_SERVICE_H
#define EQEMU_HOUSING_ESCROW_SERVICE_H
#include "coin_wallet.h"
#include "persistence_service.h"
#include <memory>
#include <mutex>
#include <string>
class Database;
namespace Housing::Escrow {
struct IsolatedFixture {};
enum class Action : uint8_t { Deposit = 0, Withdraw = 1 };
enum class Error { None, Disabled, UnsupportedFunding, InvalidArgument, Unauthorized,
    NotFound, Conflict, SnapshotMismatch, InsufficientFunds, Overflow, DatabaseFailure, CommitUncertain };
template<class T> struct Result {
    T value{}; Error error = Error::None;
    explicit operator bool() const { return error == Error::None; }
};
struct Request {
    std::string request_key;
    AuthenticatedPurchaser actor; // Trusted harness/server identity, not packet fields.
    EstateId estate_id = 0;
    Action action = Action::Deposit;
    CoinWallet::Denomination denomination = CoinWallet::Denomination::Copper;
    int64_t quantity = 0; // Denomination units, never implicitly copper.
    CoinWallet::Snapshot expected_cursor;
};
struct Receipt {
    Request request; // Immutable binding, including historical BEFORE cursor.
    CoinWallet::Snapshot cursor_after;
    int64_t copper_transferred = 0;
    int64_t escrow_before = 0;
    int64_t escrow_after = 0;
    bool replayed = false;
};
class Service {
public:
    // Own an exclusive connected DB. Never alias a live zone connection.
    explicit Service(std::unique_ptr<Database> database, bool enabled = false);
    Service(std::unique_ptr<Database> database, bool enabled, IsolatedFixture);
    ~Service();
    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;
    // Production funding/session adapter is absent: always fail closed.
    Result<Receipt> TransferCoins(const Request& request);
    // Only unloginable EscrowFixture synthetic actors in the isolated DB namespace.
    Result<Receipt> TransferIsolatedCoins(const Request& request);
    // Does not create a transfer on missing key. Auth/current permissions rechecked.
    Result<Receipt> ResolveIsolatedReceipt(const Request& request);
    // Receipts contain historical AFTER snapshots; NEVER apply an old replay to
    // a live wallet. These methods have no Client/session/cursor-item authority.
private:
    Result<Receipt> Execute(const Request& request, bool lookup_only);
    std::unique_ptr<Database> database_;
    bool enabled_ = false;
    bool fixture_ = false;
    std::mutex mutex_;
};
}
#endif
