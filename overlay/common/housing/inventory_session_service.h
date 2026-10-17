#ifndef EQEMU_HOUSING_INVENTORY_SESSION_SERVICE_H
#define EQEMU_HOUSING_INVENTORY_SESSION_SERVICE_H
#include "persistence_service.h"
#include <array>
namespace Housing::InventorySessions {
struct Owner {std::string process_key,client_key;uint32_t zone_id=0;uint16_t instance_id=0;};
struct Lease {AuthenticatedPurchaser actor;Owner owner;uint64_t epoch=0;std::array<uint8_t,32> token{};bool active=false;};
enum class Error {None,Disabled,InvalidArgument,Unauthorized,Busy,AlreadyReleased,Stale,DatabaseFailure,CommitUncertain};
template<class T>struct Result{T value{};Error error=Error::None;explicit operator bool()const{return error==Error::None;}};
// Separate from housing custody: every enabled zone inventory admission claims
// BEFORE loading. Dedicated exclusive Database, no retry inside a transaction,
// no timeout/takeover. Claim nonce and Release token are server-only values.
class Service {
public:
 explicit Service(std::unique_ptr<Database>,bool enabled=false,bool require_supervised_process=false);~Service();
 Result<Lease> Claim(AuthenticatedPurchaser,const Owner&);
 Result<bool> InspectHeld(const Lease&);
 Result<bool> Release(const Lease&); // Only AFTER full writer teardown.
 Result<bool> ResolveReleased(const Lease&); // Immutable own history, no mutation.
private:std::unique_ptr<Database> database_;bool enabled_;bool require_supervised_process_;std::mutex mutex_;
};
}
#endif
