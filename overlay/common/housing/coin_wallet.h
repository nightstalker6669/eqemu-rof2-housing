#ifndef EQEMU_HOUSING_COIN_WALLET_H
#define EQEMU_HOUSING_COIN_WALLET_H

#include <cstdint>
#include <limits>

namespace Housing::CoinWallet {
// Deliberately no bank/shared-bank, Client, database, permission or escrow state.
// Counters match PlayerProfile's signed int32 representation. Aggregate order is
// copper, silver, gold, platinum; denomination IDs match recovered coin subtypes.
struct Snapshot {
    int32_t copper = 0;
    int32_t silver = 0;
    int32_t gold = 0;
    int32_t platinum = 0;
};
inline bool operator==(const Snapshot& a, const Snapshot& b) {
    return a.copper == b.copper && a.silver == b.silver &&
        a.gold == b.gold && a.platinum == b.platinum;
}
enum class Denomination : uint8_t { Copper = 0, Silver = 1, Gold = 2, Platinum = 3 };
enum class Error {
    None, InvalidSnapshot, InvalidAmount, InvalidDenomination, InsufficientFunds,
    Overflow, EmptyCursor, CursorNotEmpty, MixedCursor, PartialCursor
};
template<class T> struct Result {
    T value{};
    Error error = Error::None;
    explicit operator bool() const { return error == Error::None; }
};
template<class T> inline Result<T> Fail(Error error) { return {{}, error}; }
struct Plan {
    Snapshot before;
    Snapshot after;
    int64_t copper_amount = 0; // Positive magnitude; operation determines direction.
};
inline bool Valid(const Snapshot& value) {
    return value.copper >= 0 && value.silver >= 0 && value.gold >= 0 && value.platinum >= 0;
}
inline bool Empty(const Snapshot& value) { return value == Snapshot{}; }
inline Result<int64_t> CopperValue(Denomination denomination, int64_t quantity) {
    int64_t multiplier = 0;
    switch (denomination) {
        case Denomination::Copper: multiplier = 1; break;
        case Denomination::Silver: multiplier = 10; break;
        case Denomination::Gold: multiplier = 100; break;
        case Denomination::Platinum: multiplier = 1000; break;
        default: return Fail<int64_t>(Error::InvalidDenomination);
    }
    if (quantity <= 0) return Fail<int64_t>(Error::InvalidAmount);
    if (quantity > std::numeric_limits<int64_t>::max() / multiplier)
        return Fail<int64_t>(Error::Overflow);
    return {quantity * multiplier, Error::None};
}
inline Result<int64_t> TotalCopper(const Snapshot& value) {
    if (!Valid(value)) return Fail<int64_t>(Error::InvalidSnapshot);
    // Maximum is INT32_MAX*1111, safely inside int64; promote before multiplying.
    return {int64_t(value.copper) + int64_t(value.silver) * 10 +
        int64_t(value.gold) * 100 + int64_t(value.platinum) * 1000, Error::None};
}
inline Result<Plan> DebitCarried(const Snapshot& before, int64_t copper_amount) {
    auto total = TotalCopper(before);
    if (!total) return Fail<Plan>(total.error);
    if (copper_amount <= 0) return Fail<Plan>(Error::InvalidAmount);
    if (copper_amount > total.value) return Fail<Plan>(Error::InsufficientFunds);
    const int64_t remaining = total.value - copper_amount;
    if (remaining / 1000 > std::numeric_limits<int32_t>::max())
        return Fail<Plan>(Error::Overflow);
    // Explicit deterministic normalization policy: remainder change is base10,
    // lower denominations0..9. This is not a claim about Live's debit selection.
    Snapshot after{int32_t(remaining % 10), int32_t(remaining / 10 % 10),
        int32_t(remaining / 100 % 10), int32_t(remaining / 1000)};
    return {{before, after, copper_amount}, Error::None};
}
namespace Detail {
inline int32_t Counter(const Snapshot& value, Denomination denomination) {
    switch (denomination) {
        case Denomination::Copper: return value.copper;
        case Denomination::Silver: return value.silver;
        case Denomination::Gold: return value.gold;
        case Denomination::Platinum: return value.platinum;
        default: return 0; // Caller validates denomination through CopperValue.
    }
}
inline Snapshot OneDenomination(Denomination denomination, int32_t quantity) {
    Snapshot value;
    switch (denomination) {
        case Denomination::Copper: value.copper = quantity; break;
        case Denomination::Silver: value.silver = quantity; break;
        case Denomination::Gold: value.gold = quantity; break;
        case Denomination::Platinum: value.platinum = quantity; break;
        default: break;
    }
    return value;
}
}
inline Result<Plan> DepositCursor(const Snapshot& before, Denomination denomination, int64_t quantity) {
    if (!Valid(before)) return Fail<Plan>(Error::InvalidSnapshot);
    auto copper = CopperValue(denomination, quantity);
    if (!copper) return Fail<Plan>(copper.error);
    if (Empty(before)) return Fail<Plan>(Error::EmptyCursor);
    const int32_t selected = Detail::Counter(before, denomination);
    if (!(before == Detail::OneDenomination(denomination, selected)))
        return Fail<Plan>(Error::MixedCursor);
    if (quantity > selected) return Fail<Plan>(Error::InsufficientFunds);
    // Native observed coin deposit consumes the whole selected cursor bucket.
    // Partial quantity is not enabled by this narrowly scoped planning contract.
    if (quantity != selected) return Fail<Plan>(Error::PartialCursor);
    return {{before, Snapshot{}, copper.value}, Error::None};
}
inline Result<Plan> WithdrawCursor(const Snapshot& before, Denomination denomination, int64_t quantity) {
    if (!Valid(before)) return Fail<Plan>(Error::InvalidSnapshot);
    auto copper = CopperValue(denomination, quantity);
    if (!copper) return Fail<Plan>(copper.error);
    if (!Empty(before)) return Fail<Plan>(Error::CursorNotEmpty);
    if (quantity > std::numeric_limits<int32_t>::max()) return Fail<Plan>(Error::Overflow);
    return {{before, Detail::OneDenomination(denomination, int32_t(quantity)), copper.value}, Error::None};
}
// Plans never change inputs or prove authorization/funds on the escrow side.
// The caller must also reject a cursor item, coordinate transaction/session fences,
// and establish the appropriate native acknowledgement/cursor synchronization.
}
#endif
