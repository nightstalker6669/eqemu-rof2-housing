#ifndef EQEMU_ZONE_HOUSING_HOUSE_INVENTORY_SWAP_H
#define EQEMU_ZONE_HOUSING_HOUSE_INVENTORY_SWAP_H
#include <cstdint>
class Client;
namespace Housing::HouseInventory {
enum class SwapDisposition { NotApplicable, Moved, Rejected };
// Synchronous main-thread operation; no scripts/sends/event pumping until ACK.
// Unsupported bound-house operations reject before the legacy writer can mutate.
SwapDisposition TryZoneSwap(Client&,uint32_t from,uint32_t to,uint32_t stack_count);
}
#endif
