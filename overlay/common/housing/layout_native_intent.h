#ifndef EQEMU_HOUSING_LAYOUT_NATIVE_INTENT_H
#define EQEMU_HOUSING_LAYOUT_NATIVE_INTENT_H
#include "layout_mixed_coordinator_db.h"
namespace Housing::LayoutNative {
struct Prepared {
 LayoutMixedCoordinator::Intent intent;
 LayoutMixedApplication::PropertyOwners owners;
};
// Adapt freshly authorized server scenes and the actual serialized scene
// owners. Header IDs select candidates; they never establish a route, house
// relationship, custody identity, item GUID or mutation permission.
inline LiveItems::Result<Prepared> Prepare(const LayoutWire::Load& load,Route physical,bool inside,
 const Scene::Snapshot& yard,const Scene::InteriorSnapshot* room,
 std::map<uint32_t,std::unique_ptr<EQ::ItemInstance>>& presentations,std::string* refusal=nullptr,bool crate_destination=false){
 using Error=LiveItems::Error;
 auto fail=[&](const std::string& reason)->LiveItems::Result<Prepared>{if(refusal)*refusal=reason;return {{},Error::Conflict};};
 try{
  if(!yard.estate_id||!yard.native_plot_id||load.yard_estate_id!=yard.native_plot_id||
     !physical.zone_id||!physical.instance_id||physical.instance_id>32000||
     bool(LayoutWire::ContextHouseEstateId(load))!=bool(room)||(!inside&&!LayoutProperty::Detail::Same(physical,yard.route)))return fail("yard/header/physical context");
  Prepared out;auto& c=out.intent.context;c={yard.estate_id,yard.native_plot_id,0,0,physical,inside};
  if(room){
   if(!room->house||!room->house->interior||!room->room_profile||!room->profile_revision||
      room->house->interior->native_estate_id!=load.house_estate_id||
      room->context.estate_id!=yard.estate_id||room->context.native_plot_id!=yard.native_plot_id||
      !LayoutProperty::Detail::Same(room->context.neighborhood,yard.route)||
      (inside?(!LayoutProperty::Detail::Same(room->context.source,physical)||bool(room->exterior_source)):
              (!room->exterior_source||!LayoutProperty::Detail::Same(*room->exterior_source,physical))))return fail("attached house/room relationship");
   c.room_id=load.house_estate_id;c.house_id=room->house->house_id;
  }else if(inside)return fail("occupied house has no room context");
  auto add=[&](const LiveItems::Stored& stored,uint32_t estate,LayoutCrate::Allocation crate={},LayoutHouse::Identity house={},LayoutCrateMember::Proof member={}){
   if(!stored.custody_id||!stored.placement||(member?LayoutCrateMember::SourceEstate(*member):stored.placement->reservation.native_estate_id)!=estate)throw std::invalid_argument("Invalid native layout scene row");
   if(member&&(!LayoutCrateMember::Shape(*member,stored)||crate||house))throw std::invalid_argument("Invalid native layout crate member");const auto id=stored.placement->reservation.native_item_id;auto shown=presentations.find(id);
   if(shown==presentations.end()||!shown->second||!InventoryProjection::Matches(stored.row,*shown->second)||(crate&&(!LayoutCrate::Shape(*crate,stored)||!LayoutCrate::Matches(crate,shown->second.get())))||(house&&(!LayoutHouse::Shape(*house,stored)||!LayoutHouse::Matches(house,shown->second.get()))))throw std::invalid_argument("Native layout presentation changed");
   LayoutProperty::Identity identity{estate,id};
   if(!out.intent.sources.emplace(identity,LayoutProperty::Witness{stored.custody_id,LayoutInventory::NativeGuid(shown->second->GetSerialNumber()),stored.row,shown->second.get(),crate,house,member}).second||
      !out.owners.emplace(stored.custody_id,&shown->second).second)throw std::invalid_argument("Ambiguous native layout owner");
   if(!crate&&!house&&!member)out.intent.consolidation_order.push_back(identity);
  };
  // Explicit server scene order, reversed for target preference. The original
  // Live server tie-break is unknown; this is not claimed to reproduce its
  // untransmitted target selection or the client's historical cache slots.
  for(const auto& item:yard.items)if(!item.contained_in_crate)add(item.stored,c.yard_id,item.crate,item.house);
  if(room)for(const auto& item:room->contents)if(!item.contained_in_crate){
   if(!item.room||item.room->house_id!=c.house_id)throw std::invalid_argument("Invalid native layout room member");add(item.stored,c.room_id,item.crate);
  }
  for(const auto& item:yard.crate_contents)if(!item.house){auto container=std::find_if(yard.items.begin(),yard.items.end(),[&](const auto& c){return c.crate&&c.crate->binding.crate_id==item.contained_in_crate;});if(container==yard.items.end())throw std::invalid_argument("Missing layout crate container");auto proof=LayoutCrateMember::FromSnapshot(item.stored,item.crate_membership,container->crate);add(item.stored,LayoutCrateMember::SourceEstate(*proof),{},{},proof);}
  if(room)for(const auto& item:room->crate_contents){if(!item.room||item.room->house_id!=c.house_id)throw std::invalid_argument("Invalid native layout room crate member");auto container=std::find_if(room->contents.begin(),room->contents.end(),[&](const auto& p){return p.crate&&p.crate->binding.crate_id==item.contained_in_crate;});if(container==room->contents.end())throw std::invalid_argument("Missing layout room crate container");auto proof=LayoutCrateMember::FromSnapshot(item.stored,item.crate_membership,container->crate);add(item.stored,LayoutCrateMember::SourceEstate(*proof),{},{},proof);}
  std::reverse(out.intent.consolidation_order.begin(),out.intent.consolidation_order.end());
  // Refuse unadvertised/unsupported sources before any transaction work. GUID
  // equality for property Tag1 is still checked by the ordered coordinator.
  for(const auto& op:load.operations){
   uint32_t target=0;std::optional<LayoutProperty::Identity> source;
   if(const auto* p=std::get_if<LayoutWire::PlacementOperation>(&op)){target=p->target_estate_id;if(p->source_kind==0xff)source={{p->source_estate_id,p->native_item_id}};else if(p->source_kind!=0)return {{},Error::UnsupportedItem};}
   else{const auto& item=std::get<LayoutWire::ItemOperation>(op);target=item.target_estate_id;source={{item.source_estate_id,item.native_item_id}};}
   bool typed_crate=false;if(crate_destination)if(const auto* count=std::get_if<LayoutWire::ItemOperation>(&op);count&&count->quantity){auto check=[&](const auto& rows){for(const auto& row:rows)typed_crate|=row.crate&&!row.stored.stowed&&!row.crate->packed&&row.crate->binding.native_estate_id==target;};check(yard.items);if(room)check(room->contents);}
   if(target!=c.yard_id&&(!c.room_id||target!=c.room_id)&&!typed_crate)return fail("unsupported target "+std::to_string(target));
   if(source&&!out.intent.sources.count(*source))return fail("unsupported property source "+std::to_string(source->first)+":"+std::to_string(source->second));
  }
  return {std::move(out),Error::None};
 }catch(const std::invalid_argument& e){return fail(e.what());}catch(...){return fail("native scene projection exception");}
}
// Native3424 is a single stored count intent. Unrelated yard actors need not
// have room-local presentation owners. Select only its freshly read source and
// cached compatible destination stacks for MEMORY witnesses. The owning
// coordinator still rereads complete actual scenes/capacity/authority under TX.
// This grants no permission and is never used for arbitrary file layouts.
inline LiveItems::Result<Prepared> PrepareStorageCount(const LayoutWire::Load& load,Route physical,bool inside,
 const Scene::Snapshot& yard,const Scene::InteriorSnapshot* room,
 std::map<uint32_t,std::unique_ptr<EQ::ItemInstance>>& presentations,std::string* refusal=nullptr,bool crate_destination=false){
 using Error=LiveItems::Error;
 if(load.operations.size()!=1)return {{},Error::InvalidArgument};const auto* op=std::get_if<LayoutWire::ItemOperation>(&load.operations[0]);if(!op||!op->quantity)return {{},Error::InvalidArgument};
 const LiveItems::Stored* source=nullptr;uint64_t container_custody=0;
 auto bind=[&](const LiveItems::Stored& stored,uint32_t estate,uint64_t container=0){if(!stored.placement||stored.placement->reservation.native_item_id!=op->native_item_id||estate!=op->source_estate_id)return;if(source)throw std::invalid_argument("Ambiguous count source");source=&stored;container_custody=container;};
 try{
  for(const auto& i:yard.items)if(i.stored.placement)bind(i.stored,i.stored.placement->reservation.native_estate_id);
  if(room)for(const auto& i:room->contents)if(i.stored.placement)bind(i.stored,i.stored.placement->reservation.native_estate_id);
  auto member=[&](const auto& rows,const auto& containers){for(const auto& i:rows){if(!i.stored.placement||i.stored.placement->reservation.native_item_id!=op->native_item_id)continue;auto container=std::find_if(containers.begin(),containers.end(),[&](const auto& c){return c.crate&&c.crate->binding.crate_id==i.contained_in_crate;});if(container==containers.end())throw std::invalid_argument("Missing count container");auto proof=LayoutCrateMember::FromSnapshot(i.stored,i.crate_membership,container->crate);bind(i.stored,LayoutCrateMember::SourceEstate(*proof),container->stored.custody_id);}};
  member(yard.crate_contents,yard.items);if(room)member(room->crate_contents,room->contents);if(!source)return {{},Error::NotFound};const auto item=source->row.values.item_id;const auto custody=source->custody_id;
  uint64_t destination_container=0;if(crate_destination){auto find=[&](const auto& rows){for(const auto& row:rows)if(row.crate&&row.crate->binding.native_estate_id==op->target_estate_id){if(destination_container)throw std::invalid_argument("Ambiguous count destination crate");destination_container=row.stored.custody_id;}};find(yard.items);if(room)find(room->contents);if(!destination_container||container_custody)return {{},Error::UnsupportedItem};}
  auto selected=[&](const auto& i){const auto& s=i.stored;if(s.custody_id==custody||s.custody_id==container_custody||s.custody_id==destination_container)return true;if(!s.placement||!s.stowed||s.placement->reservation.native_estate_id!=op->target_estate_id||s.row.values.item_id!=item)return false;auto shown=presentations.find(s.placement->reservation.native_item_id);return shown!=presentations.end()&&bool(shown->second);};
  auto y=yard;y.items.erase(std::remove_if(y.items.begin(),y.items.end(),[&](const auto& i){return !selected(i);}),y.items.end());y.crate_contents.erase(std::remove_if(y.crate_contents.begin(),y.crate_contents.end(),[&](const auto& i){return i.stored.custody_id!=custody;}),y.crate_contents.end());
  std::optional<Scene::InteriorSnapshot> r;if(room){r=*room;r->contents.erase(std::remove_if(r->contents.begin(),r->contents.end(),[&](const auto& i){return !selected(i);}),r->contents.end());r->crate_contents.erase(std::remove_if(r->crate_contents.begin(),r->crate_contents.end(),[&](const auto& i){return i.stored.custody_id!=custody;}),r->crate_contents.end());}
  return Prepare(load,physical,inside,y,r?&*r:nullptr,presentations,refusal,crate_destination);
 }catch(const std::invalid_argument& e){if(refusal)*refusal=e.what();return {{},Error::Conflict};}
}

}
#endif
