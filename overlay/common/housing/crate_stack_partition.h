#ifndef EQEMU_HOUSING_CRATE_STACK_PARTITION_H
#define EQEMU_HOUSING_CRATE_STACK_PARTITION_H
#include "layout_crate_member_identity.h"
namespace Housing::CrateStackPartition {
// Immutable paired count/container proof. This authenticates no client input;
// LiveItems captures it from fresh authorized scenes and locked SQL rows.
inline bool Coherent(const LiveItems::Receipt& r){try{
 if(!r.partition)return !r.crate_member;const auto& p=*r.partition;
 const bool any=p.member_container_before||p.member_before||p.member_after;
 if(!r.crate_member)return !any;
 if(!p.member_container_before||!p.member_after||p.vault_source||!r.stored.placement||r.house_effect_required||r.crate_allocation||r.crate_before)return false;
 if(!LayoutCrateMember::Valid(*p.member_after))return false;
 const auto& before=*p.member_container_before;const auto& after=*p.member_after->container;const auto& m=*r.crate_member;
 auto expected=before;if(!before.binding.revision||before.binding.revision>=UINT64_MAX-1)return false;++expected.binding.revision;
 if(CrateBinding::EncodeAllocation(expected)!=CrateBinding::EncodeAllocation(after)||after.binding.crate_id!=m.crate_id||after.binding.revision!=m.revision||after.binding.parent_native_estate_id!=m.native_plot_id||after.created_by_character_id!=r.stored.row.values.character_id)return false;
 auto stored=r.stored;stored.stowed=true;stored.storage_revision=r.action==LiveItems::Action::Store?1:p.storage_after;stored.row=r.action==LiveItems::Action::Store?r.stored.row:p.after;
 if(!LayoutCrateMember::Shape(*p.member_after,stored)||p.member_after->membership.origin_house_id!=(p.room_source?p.room_source->house_id:0)||!p.member_after->membership.origin_stowed)return false;
 if(r.action==LiveItems::Action::Store)return !p.member_before&&p.member_after->membership.revision==1;
 if(r.action!=LiveItems::Action::Return||!p.member_before||!p.stowed||!p.storage_before)return false;
 stored.row=p.before;stored.storage_revision=p.storage_before;
 return LayoutCrateMember::Shape(*p.member_before,stored)&&CrateBinding::EncodeAllocation(*p.member_before->container)==CrateBinding::EncodeAllocation(before)&&LayoutCrateMember::SameOrigin(p.member_before,p.member_after);
 }catch(...){return false;}}
}
#endif
