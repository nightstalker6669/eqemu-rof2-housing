#ifndef EQEMU_ZONE_HOUSING_SERVICE_TRIBUTE_H
#define EQEMU_ZONE_HOUSING_SERVICE_TRIBUTE_H
#include <cstdint>
#include <optional>
class Client;
namespace Housing::ServiceTribute {
struct Receipt {uint32_t favor=0,guild_favor=0,member_favor=0;};
// Caller has already authenticated the real current tribute master. Debit,
// shared guild favor and member favor commit together; only then is the native
// in-memory item/currency presentation changed. No guessed trophy economy.
std::optional<Receipt> DonateItem(Client&,uint16_t slot,uint32_t quantity);
std::optional<Receipt> DonatePlatinum(Client&,uint32_t quantity,uint32_t conversion);
}
#endif
