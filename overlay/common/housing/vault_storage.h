#ifndef EQEMU_HOUSING_VAULT_STORAGE_H
#define EQEMU_HOUSING_VAULT_STORAGE_H
#include "live_item_service.h"
namespace Housing::VaultStorage {
struct Item {LiveItems::Stored stored;uint32_t native_estate_id=0,native_item_id=0;uint64_t revision=0,house_id=0;std::string creator_name;};
}
#endif
