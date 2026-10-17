#ifndef EQEMU_ZONE_HOUSING_HOUSE_PRELOAD_FENCE_H
#define EQEMU_ZONE_HOUSING_HOUSE_PRELOAD_FENCE_H
#include "../common/housing/house_binding_preload.h"
#include "inventory_session_owner.h"
#include <map>
class Client;
namespace Housing::HousePreload {
// Admission-only adapter. It must remain alive across capture, inventory load,
// recheck and immediate hydration, without yielding or calling scripts/sends.
// The character reload pipeline uses this fence with exact loader witnesses.
// This grants no binding creation or permission to use unsupported writers.
// Construction does not claim a new session.
class ZoneSessionFence final : public SessionFence {
public:
 static std::unique_ptr<ZoneSessionFence> Create(Client&);
 const InventorySessions::Lease& Reservation()const noexcept { return reservation_; }
 bool HeldFor(const InventorySessions::Lease&)const noexcept override;
 bool LoadedMatches(const LiveItems::InventoryRow&,int32_t serial)const noexcept override;
private:
 ZoneSessionFence(Client&,std::shared_ptr<InventorySessions::ZoneReservation>,InventorySessions::Lease);
 Client& client_;
 std::shared_ptr<InventorySessions::ZoneReservation> ticket_;
 InventorySessions::Lease reservation_;
 // Once witnessed, retain the exact loaded pointer as well as its native serial.
 mutable std::map<uint32_t,const void*> loaded_pointers_;
};
}
#endif
