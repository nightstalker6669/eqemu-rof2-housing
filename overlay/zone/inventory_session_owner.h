#ifndef EQEMU_ZONE_INVENTORY_SESSION_OWNER_H
#define EQEMU_ZONE_INVENTORY_SESSION_OWNER_H
#include "../common/housing/inventory_session_service.h"
#include <optional>
class Client;
class EQStreamInterface;
namespace Housing::InventorySessions {
// A copied ticket survives full Client/Mob deletion. Only EntityList's
// post-delete boundary releases it; no wall clock/transport/CLE expiry.
class ZoneReservation {
public:
 static std::shared_ptr<ZoneReservation> ClaimForAuthenticatedClient(Client&);
 bool HeldFor(const Client&)const noexcept;
 // Server-only read view of the actual ticket. Never serialize or log its token.
 // A view does not confer ownership; users must recheck this ticket throughout.
 std::optional<Lease> SnapshotFor(const Client&)const;
 void ReleaseAfterCompleteDelete();
private:
 ZoneReservation()=default;
 std::unique_ptr<Service> Fresh()const;
 Lease lease_;EQStreamInterface* connection_=nullptr;bool released_=false;
};
}
#endif
