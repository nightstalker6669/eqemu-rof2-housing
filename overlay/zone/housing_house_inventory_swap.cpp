#include "housing_house_inventory_swap.h"
#include "housing_house_preload_fence.h"
#include "client.h"
#include "../common/eqemu_config.h"
#include "../common/housing/house_inventory_profile_fence.h"
#include <openssl/rand.h>
#include <charconv>
#include <chrono>
#include <thread>
namespace Housing::HouseInventory {
namespace {
void Wait(){std::this_thread::sleep_for(std::chrono::milliseconds(250));}
[[noreturn]] void Freeze(){for(;;)Wait();}
bool ContainsBinding(EQ::ItemInstance* item,unsigned depth=0){
 if(!item)return false;
 if(depth>4||item->GetHousingHouseID()||item->GetHousingCrateID()||item->GetHousingEstateID()!=UINT32_MAX)return true;
 for(const auto& child:*item->GetContents())if(ContainsBinding(child.second,depth+1))return true;
 return false;
}
EQ::ItemInstance* At(EQ::InventoryProfile& inv,uint32_t slot){return slot<=INT16_MAX?inv.GetItem(int16_t(slot)):nullptr;}
std::unique_ptr<Database> Connection(){
 const auto* cfg=EQEmuConfig::get();if(!cfg)return nullptr;
 auto db=std::make_unique<Database>();
 if(!db->Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"house-relocation-exclusive"))return nullptr;
 return db;
}
std::unique_ptr<Service> Fresh(){auto db=Connection();return db?std::make_unique<Service>(std::move(db),true):nullptr;}
std::string Key(){
 uint8_t bytes[32];if(RAND_bytes(bytes,32)!=1)throw std::runtime_error("House movement key failed");
 std::string key="houseinv:";const char* hex="0123456789abcdef";
 for(auto b:bytes){key+=hex[b>>4];key+=hex[b&15];}return key;
}
LiveItems::InventoryRow Raw(const InventoryRepository::LoadRow& row){
 LiveItems::InventoryRow out;out.values=row.values;
 if(!row.fields[2])out.null_fields|=LiveItems::NullItemId;
 if(!row.fields[3])out.null_fields|=LiveItems::NullCharges;
 if(!row.fields[12])out.null_fields|=LiveItems::NullCustomData;
 if(!row.fields[16])out.null_fields|=LiveItems::NullGuid;
 return out;
}
uint64_t Revision(Database& db,uint64_t house,uint32_t character,uint32_t slot,uint32_t item){
 auto r=db.QueryDatabase("SELECT h.revision FROM housing_house_instances h JOIN housing_house_locations l ON l.house_id=h.house_id WHERE h.house_id="+std::to_string(house)+" AND h.item_template_id="+std::to_string(item)+" AND h.pending_request_key IS NULL AND l.location_kind=1 AND l.character_id="+std::to_string(character)+" AND l.slot_id="+std::to_string(slot),false);
 if(!r.Success()||r.RowCount()!=1||!r.begin()[0])throw std::runtime_error("House movement preimage unavailable");
 const std::string text=r.begin()[0];uint64_t value=0;auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
 if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||!value||value==UINT64_MAX)throw std::runtime_error("House movement revision invalid");
 return value;
}
}
SwapDisposition TryZoneSwap(Client& client,uint32_t from,uint32_t to,uint32_t stack_count){
 bool write_started=false;
 try{
  auto& inventory=client.GetInv();
  if(!ContainsBinding(At(inventory,from))&&!ContainsBinding(At(inventory,to)))return SwapDisposition::NotApplicable;
  if(!RuleB(Housing,EnablePlayerHousing)||!client.ClientDataLoaded())return SwapDisposition::Rejected;
  auto local=HousePreload::ZoneSessionFence::Create(client);if(!local)return SwapDisposition::Rejected;
  if(from==to)return SwapDisposition::Moved;
  // This is the existing relocation domain's first real caller. Other slot
  // classes and stack/trade/deletion semantics still require dedicated adapters.
  if(stack_count||from<23||from>33||to<23||to>33||!At(inventory,from))return SwapDisposition::Rejected;
  auto db=Connection();if(!db)return SwapDisposition::Rejected;
  auto snapshot=InventoryRepository::GetLoadSnapshot(*db,client.CharacterID());if(!snapshot.success)return SwapDisposition::Rejected;
  Request request;request.request_key=Key();request.reservation=local->Reservation();
  for(auto slot:{std::min(from,to),std::max(from,to)}){
   SlotState state;state.slot=slot;auto item=At(inventory,slot);
   const InventoryRepository::LoadRow* persisted=nullptr;
   for(const auto& row:snapshot.rows)if(row.values.slot_id==slot)persisted=&row;
   if(bool(item)!=bool(persisted)){LogError("Housing inventory relocation occupancy mismatch actor [{}] slot [{}] memory [{}] persisted [{}]",client.CharacterID(),slot,bool(item),bool(persisted));return SwapDisposition::Rejected;}
   if(item){
    state.row=Raw(*persisted);const auto house=item->GetHousingHouseID();
    const auto crate=item->GetHousingCrateID();if(house&&crate)return SwapDisposition::Rejected;
    uint64_t revision=house?Revision(*db,house,client.CharacterID(),slot,item->GetID()):0;
    if(crate){auto current=db->QueryDatabase("SELECT h.revision,e.native_estate_id FROM housing_crate_instances h JOIN housing_crate_locations l ON l.crate_id=h.crate_id JOIN housing_crate_estates e ON e.crate_id=h.crate_id WHERE h.crate_id="+std::to_string(crate)+" AND h.item_template_id="+std::to_string(item->GetID())+" AND l.location_kind=1 AND l.character_id="+std::to_string(client.CharacterID())+" AND l.slot_id="+std::to_string(slot),false);if(!current.Success()||current.RowCount()!=1||!current.begin()[0]||!current.begin()[1]||std::stoull(current.begin()[1])!=item->GetHousingEstateID())return SwapDisposition::Rejected;revision=std::stoull(current.begin()[0]);if(!revision||revision==UINT64_MAX)return SwapDisposition::Rejected;}
    request.moves.push_back({house,revision,slot,slot==from?to:from,item->GetSerialNumber(),crate});
   }
   request.before.push_back(std::move(state));
  }
  for(const auto& destination:request.before){
   SlotState state;state.slot=destination.slot;
   for(const auto& source:request.before)if(source.slot!=destination.slot&&source.row){state.row=*source.row;state.row->values.slot_id=state.slot;}
   request.after.push_back(std::move(state));
  }
  ProfileFence fence(inventory,request,[&](const auto& lease){return local->HeldFor(lease);});
  auto service=std::make_unique<Service>(std::move(db),true);
  bool uncertain=false;Receipt receipt;
  for(;;){
   if(!service){service=Fresh();if(!service){if(!uncertain)return SwapDisposition::Rejected;Wait();continue;}}
   write_started=true;
   auto result=uncertain?service->Resolve(request.reservation,request.request_key,fence):service->Relocate(request,fence);
   if(result){receipt=std::move(result.value);break;}
   if(uncertain&&result.error==Error::NotFound){uncertain=false;service.reset();continue;}
   if(result.error==Error::CommitUncertain)uncertain=true;
   else if(!uncertain){LogError("Housing inventory relocation rejected actor [{}] from [{}] to [{}] error [{}]",client.CharacterID(),from,to,int(result.error));write_started=false;return SwapDisposition::Rejected;}
   service.reset();Wait();
  }
  // A historical applied key is never a reason to perform another permutation.
  if(receipt.requires_application){
   EQ::InventoryProfile::SwapItemFailState failure;
   if(!inventory.SwapItem(int16_t(from),int16_t(to),failure)||!fence.Applied(receipt))Freeze();
  }else if(!fence.Applied(receipt))Freeze();
  for(;;){
   auto fresh=Fresh();if(!fresh){Wait();continue;}
   auto acknowledged=fresh->AcknowledgeApplied(request.reservation,request.request_key,fence);
   if(acknowledged)return SwapDisposition::Moved;
   // Memory is already applied. Keep the actual reservation/pointers alive and
   // retry own idempotent ACK on fresh connections; never fall into legacy saves.
   Wait();
  }
 }catch(...){if(write_started)Freeze();return SwapDisposition::Rejected;}
}
}
