#ifndef EQEMU_HOUSING_HOUSE_INVENTORY_PROFILE_FENCE_H
#define EQEMU_HOUSING_HOUSE_INVENTORY_PROFILE_FENCE_H
#include "house_inventory_transition.h"
#include "../inventory_profile.h"
#include <functional>
#include <map>
namespace Housing::HouseInventory {
// Actual inventory pointer/projection witness for relocation. The zone adapter
// must supply its real ZoneReservation check, keep this scope synchronous and
// retain it through commit resolution, memory application and ACK. No default
// ownership predicate, packet proof or inventory mutation is provided here.
class ProfileFence final : public SessionFence {
public:
 using HeldPredicate=std::function<bool(const InventorySessions::Lease&)>;
 ProfileFence(EQ::InventoryProfile&,Request,HeldPredicate);
 bool HeldFor(const InventorySessions::Lease&)const noexcept override;
 bool BeforeMatches(const Request&,const std::vector<HouseBinding::Binding>&)const noexcept override;
 bool BeforeMatches(const Request&,const std::vector<HouseBinding::Binding>&,const std::vector<CrateBinding::Allocation>&)const noexcept override;
 bool Applied(const Receipt&)const noexcept override;
private:
 bool SameRequest(const Request&)const;
 bool Match(const std::vector<SlotState>&,const std::vector<HouseBinding::Binding>&,const std::vector<CrateBinding::Allocation>&,bool after)const;
 EQ::InventoryProfile& inventory_;
 const Request request_;
 HeldPredicate held_;
 std::map<uint32_t,EQ::ItemInstance*> pointers_;
};
}
#endif
