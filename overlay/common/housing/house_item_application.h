#ifndef EQEMU_HOUSING_HOUSE_ITEM_APPLICATION_H
#define EQEMU_HOUSING_HOUSE_ITEM_APPLICATION_H
#include "house_binding_service.h"
#include "custody_codec.h"
#include "../item_instance.h"
namespace Housing::HouseMemory {
// Property application only. The zone bridge separately proves its live fence,
// exact raw17 projection and actual retained/destination pointer. This helper
// never grants template recognition, mint permission, placement or routing.
inline bool SameInterior(const std::optional<HouseBinding::Interior>& a,const std::optional<HouseBinding::Interior>& b){
 if(bool(a)!=bool(b))return false;
 if(!a)return true;
 return a->native_estate_id==b->native_estate_id&&a->definition_id==b->definition_id&&
  a->zone_id==b->zone_id&&a->instance_id==b->instance_id&&a->zone_version==b->zone_version&&
  a->catalog_key==b->catalog_key&&a->catalog_revision==b->catalog_revision;
}
inline uint32_t Estate(const HouseBinding::Binding& b){return b.interior?b.interior->native_estate_id:UINT32_MAX;}
inline bool Coherent(const HouseBinding::Effect& e)noexcept{
 try{
  using Kind=HouseBinding::Location::Kind;
  if(e.request_key.empty()||!e.before.house_id||e.before.house_id!=e.after.house_id||!e.before.revision||
   e.before.revision==UINT64_MAX||e.after.revision!=e.before.revision+1||
   e.before.item_template_id!=e.source.values.item_id||e.after.item_template_id!=e.before.item_template_id||
   !CustodyCodec::Valid(e.source)||!CustodyCodec::Valid(e.destination)||!HouseBinding::ValidInteriorTransition(e))return false;
  for(const auto* b:{&e.before,&e.after})if(b->interior&&(!b->interior->native_estate_id||
   b->interior->native_estate_id==UINT32_MAX||!b->interior->definition_id||!b->interior->instance_id||
   !b->interior->zone_id||!b->interior->catalog_revision||b->interior->catalog_key.empty()))return false;
  auto destination=e.source;
  if(e.action==LiveItems::Action::Store){
   if(e.before.location.kind!=Kind::Inventory||e.before.location.character_id!=e.source.values.character_id||
    e.before.location.slot_id!=e.source.values.slot_id||e.before.location.custody_id||
    e.before.location.slot_id<23||e.before.location.slot_id>33||e.after.location.kind!=Kind::Custody||
    !e.after.location.custody_id||e.after.location.character_id||e.after.location.slot_id||
    (e.minted&&e.before.interior))return false;
  }else if(e.action==LiveItems::Action::Return){
   if(e.minted||e.before.location.kind!=Kind::Custody||!e.before.location.custody_id||
    e.before.location.character_id||e.before.location.slot_id||
    e.after.location.kind!=Kind::Inventory||e.after.location.character_id!=e.destination.values.character_id||
    e.after.location.slot_id!=e.destination.values.slot_id||e.after.location.slot_id<23||
    e.after.location.slot_id>33||e.after.location.custody_id||
    !SameInterior(e.before.interior,e.after.interior))return false;
   destination.values.slot_id=e.destination.values.slot_id;
  }else return false;
  return CustodyCodec::Encode(destination)==CustodyCodec::Encode(e.destination);
 }catch(...){return false;}
}
inline bool MatchesParent(const LiveItems::Receipt& r)noexcept{
 try{
  if(!r.house_effect_required)return !r.house_effect;
  if(!r.house_effect||!Coherent(*r.house_effect))return false;
  const auto& e=*r.house_effect;
  return e.request_key==r.request_key&&e.action==r.action&&
   e.requires_application==r.requires_application&&CustodyCodec::Encode(e.source)==CustodyCodec::Encode(r.stored.row)&&
   (e.action==LiveItems::Action::Store?e.after.location.custody_id:e.before.location.custody_id)==r.stored.custody_id&&
   (e.action!=LiveItems::Action::Return||CustodyCodec::Encode(e.destination)==CustodyCodec::Encode(r.inventory_after));
 }catch(...){return false;}
}
inline bool Item(const HouseBinding::Effect& e,const EQ::ItemInstance& item)noexcept{
 const auto* d=item.GetItem();
 return Coherent(e)&&d&&!item.GetHousingCrateID()&&item.GetID()==e.after.item_template_id&&d->HousingDefinitionID&&
  d->HousingDefinitionID!=UINT32_MAX&&(!e.after.interior||d->HousingDefinitionID==e.after.interior->definition_id);
}
inline bool AfterMatches(const HouseBinding::Effect& e,const EQ::ItemInstance& item)noexcept{
 return Item(e,item)&&item.GetHousingHouseID()==e.after.house_id&&item.GetHousingEstateID()==Estate(e.after);
}
inline bool Apply(const HouseBinding::Effect& e,EQ::ItemInstance& item,bool reconstructed=false)noexcept{
 if(!Item(e,item))return false;
 if(AfterMatches(e,item))return true;
 const bool unbound=item.GetHousingHouseID()==0&&item.GetHousingEstateID()==UINT32_MAX;
 const bool mint=e.action==LiveItems::Action::Store&&e.minted;
 const bool rebuild=e.action==LiveItems::Action::Return&&reconstructed;
 if((mint||rebuild)?!unbound:(item.GetHousingHouseID()!=e.before.house_id||item.GetHousingEstateID()!=Estate(e.before)))return false;
 // Both assignments have been prevalidated against the same unchanged item
 // definition, so neither setter can fail after the first assignment.
 if(!item.SetHousingHouseID(e.after.house_id)||!item.SetHousingEstateID(Estate(e.after)))return false;
 return AfterMatches(e,item);
}
}
#endif
