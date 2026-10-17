#ifndef EQEMU_HOUSING_ITEM_RECONSTRUCTION_H
#define EQEMU_HOUSING_ITEM_RECONSTRUCTION_H
#include "../common/housing/live_item_service.h"
#include "../common/item_instance.h"
#include "../common/shareddb.h"
namespace Housing::InventoryBridge {
// Original persistence identity remains separate from the newly allocated
// engine serial. Never overwrite the serial allocator with an old DB GUID.
struct ReconstructedItem {
 LiveItems::InventoryRow original;
 std::unique_ptr<EQ::ItemInstance> item;
 int32_t runtime_serial=0;
};
// Runtime projection, not raw-row equality. Authoritative row equality and the
// current runtime serial are checked independently by LiveItems/fence callbacks.
bool MatchesProjection(const LiveItems::InventoryRow&,EQ::ItemInstance&);
LiveItems::Result<ReconstructedItem> Reconstruct(SharedDatabase&,const LiveItems::InventoryRow&);
}
#endif
