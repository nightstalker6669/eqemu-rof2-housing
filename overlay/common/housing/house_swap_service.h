#ifndef EQEMU_HOUSING_HOUSE_SWAP_SERVICE_H
#define EQEMU_HOUSING_HOUSE_SWAP_SERVICE_H
#include "house_swap_preflight.h"
#include <mutex>
namespace Housing::HouseSwap {
// Explicit experimental server policy, not recovered original Live policy.
// No inventory source, fee, eviction, or deletion of either house is implied.
struct Options {bool enabled=false,old_house_to_plot_storage=false;};
struct Request {
 std::string key;LiveItems::Lease lease;EstateId estate=0;
 std::array<uint8_t,24> wire{};
};
enum class Kind:uint8_t {House=1,Furniture=2,Vault=3,Crate=4,Member=5};
struct Change {
 Kind kind=Kind::Furniture;uint64_t custody=0;
 uint32_t before_native=0,after_native=0;
 uint64_t before_storage=0,after_storage=0;
 LiveItems::InventoryRow payload;
};
struct Receipt {
 uint64_t old_house=0,new_house=0;std::vector<Change> changes;bool replayed=false;
};
// Property-only mutation: no inventory postimage/application ACK is needed.
// One journal and all property writes share ONE owned transaction. A lost
// COMMIT response returns CommitUncertain; Resolve uses a fresh connection and
// reads the immutable receipt, never reruns the swap or reevaluates old ACLs.
// No runtime mapping or policy is enabled by constructing the default service.
class Service {
 std::unique_ptr<Database> database_;Options options_;std::mutex mutex_;
public:
 explicit Service(std::unique_ptr<Database> db,Options options={}):database_(std::move(db)),options_(options){}
 LiveItems::Result<Receipt> Swap(const Request&,LiveItems::InventorySessionFence&,
  const Scene::HouseScenePolicy&,const InteriorContent::ProfilePolicy&,
  const CrateBinding::CustodyContext* =nullptr);
 LiveItems::Result<Receipt> Resolve(const Request&,LiveItems::InventorySessionFence&);
};
}
#endif
