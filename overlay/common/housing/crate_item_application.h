#ifndef EQEMU_HOUSING_CRATE_ITEM_APPLICATION_H
#define EQEMU_HOUSING_CRATE_ITEM_APPLICATION_H
#include "crate_binding.h"
#include "crate_placement_model.h"
namespace Housing::CrateMemory {
// Property projection only, under the bridge's actual held inventory fence.
// Receipt/raw-row/pointer proof is mandatory; this grants no ledger minting.
inline bool MatchesParent(const LiveItems::Receipt& r)noexcept{
 return CrateBinding::CoherentOutcome(r);
}
inline bool Item(const LiveItems::Receipt& r,const EQ::ItemInstance& item)noexcept{
 const auto* d=item.GetUnscaledItem();
 return r.crate_allocation&&MatchesParent(r)&&d&&!item.GetHousingHouseID()&&
  d->ID==r.stored.row.values.item_id&&d->ID==r.crate_allocation->binding.item_template_id&&
  d->HousingDefinitionID==r.crate_allocation->binding.definition_id&&CratePlacement::Model(item);
}
inline bool AfterMatches(const LiveItems::Receipt& r,const EQ::ItemInstance& item)noexcept{
 return Item(r,item)&&item.GetHousingCrateID()==r.crate_allocation->binding.crate_id&&
  item.GetHousingEstateID()==r.crate_allocation->binding.native_estate_id;
}
inline bool Apply(const LiveItems::Receipt& r,EQ::ItemInstance& item,bool reconstructed=false)noexcept{
 if(!Item(r,item))return false;
 if(AfterMatches(r,item))return true;
 const bool unbound=!item.GetHousingCrateID()&&item.GetHousingEstateID()==UINT32_MAX;
 const bool mint=r.action==LiveItems::Action::Store&&!r.crate_before;
 const bool rebuild=r.action==LiveItems::Action::Return&&reconstructed;
 if(mint||rebuild){if(!unbound)return false;}
 else if(!r.crate_before||item.GetHousingCrateID()!=r.crate_before->binding.crate_id||item.GetHousingEstateID()!=r.crate_before->binding.native_estate_id)return false;
 // Item() proves both setters' preconditions before either property changes.
 return item.SetHousingCrateID(r.crate_allocation->binding.crate_id)&&
  item.SetHousingEstateID(r.crate_allocation->binding.native_estate_id)&&AfterMatches(r,item);
}
}
#endif
