#ifndef EQEMU_HOUSING_LAYOUT_BATCH_JOURNAL_H
#define EQEMU_HOUSING_LAYOUT_BATCH_JOURNAL_H
#include "live_item_service.h"
#include <vector>
namespace Housing::LayoutBatch {
inline constexpr size_t MaximumPlanBytes=1048576,MaximumOutcomeBytes=1048576;
// SERVER-ONLY canonical plan/outcome codecs belong to the layout coordinator.
// Wire bytes bind the received ordered request; they grant no source/target
// authority. Plan/outcome bytes must never be accepted from a client verbatim.
struct Binding {
 std::string request_key;
 LiveItems::Lease lease;
 std::vector<uint8_t> wire,plan;
};
struct Receipt {std::vector<uint8_t> outcome;bool replayed=false;};
// Property-only immutable journal. No inventory pending/application family is
// enabled here. Reads authenticate the current held lease, then exact immutable
// binding, without reevaluating estate ACLs or reapplying historical effects.
// Both APIs require the caller's active TX; never BEGIN/COMMIT/ROLLBACK/send.
// Complete MUST share the actual mutation transaction. Any failure requires
// aborting that whole parent transaction, including already executed members.
LiveItems::Result<Receipt> ReadUnderTransaction(Database&,bool enabled,const Binding&,LiveItems::InventorySessionFence&);
LiveItems::Result<Receipt> CompleteUnderTransaction(Database&,bool enabled,const Binding&,
 const std::vector<uint8_t>& trusted_outcome,LiveItems::InventorySessionFence&);
}
#endif
