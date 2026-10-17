#ifndef EQEMU_HOUSING_LAYOUT_CRATE_IDENTITY_H
#define EQEMU_HOUSING_LAYOUT_CRATE_IDENTITY_H
#include "crate_binding.h"
#include "crate_placement_model.h"
namespace Housing::LayoutCrate {
using Allocation=std::shared_ptr<const CrateBinding::Allocation>;
inline bool Same(const Allocation& a,const Allocation& b){return (!a||!b)?!a&&!b:CrateBinding::EncodeAllocation(*a)==CrateBinding::EncodeAllocation(*b);}
inline bool Shape(const CrateBinding::Allocation& a,const LiveItems::Stored& stored){
 try{CrateBinding::EncodeAllocation(a);}catch(...){return false;}
 if(!stored.placement||stored.stowed||a.packed||a.inventory_character_id||a.inventory_slot_id)return false;
 const auto& b=a.binding;const auto& r=stored.placement->reservation;
 return b.custody_id==stored.custody_id&&b.estate_id==stored.estate_id&&b.item_template_id==stored.row.values.item_id&&
  a.created_by_character_id==stored.row.values.character_id&&b.parent_native_estate_id==r.native_estate_id&&b.catalog_key==r.catalog_key&&b.catalog_revision==r.catalog_revision;
}
inline bool Matches(const Allocation& a,const EQ::ItemInstance* item){
 if(!a)return true;
 return item&&item->GetItem()&&CratePlacement::Model(*item)&&!item->IsStackable()&&!item->GetHousingHouseID()&&
  item->GetItem()->ID==a->binding.item_template_id&&item->GetHousingCrateID()==a->binding.crate_id&&item->GetHousingEstateID()==a->binding.native_estate_id;
}
// Structural allocation/member conservation under existing caller locks. The
// current Scene and Move services separately establish physical/ACL authority.
inline LiveItems::Result<Allocation> ReadUnderTransaction(Database& db,const LiveItems::Stored& stored){
 auto read=CrateBinding::ReadPlacedUnderTransaction(db,stored);if(!read)return {{},read.error};
 if(!Shape(read.value,stored))return {{},LiveItems::Error::Conflict};
 auto members=CrateBinding::ValidateMembersUnderTransaction(db,read.value);if(!members)return {{},members.error};
 return {std::make_shared<const CrateBinding::Allocation>(std::move(read.value)),LiveItems::Error::None};
}
}
#endif
