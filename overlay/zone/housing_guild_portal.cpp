#include "housing_guild_portal.h"
#include "housing_inventory_bridge.h"
#include "client.h"
#include "mob.h"
#include "zone.h"
#include "../common/eqemu_config.h"
#include "../common/eq_packet_structs.h"
#include "../common/housing/guild_portal_db.h"
#include <chrono>
#include <map>
#include <memory>
extern Zone* zone;
namespace Housing::GuildPortalRuntime {
namespace {
Resolver resolve;
Validator validate;
struct State {
 uint32_t character=0,account=0,zone=0,instance=0;
 uint64_t last_pad=0;
 std::optional<GuildPortal::Confirmation> pending;
 std::chrono::steady_clock::time_point next{};
};
std::map<Client*,State> states;
bool Ready(Client& c){return zone&&zone->GetInstanceID()&&c.Connected()&&c.ClientDataLoaded()&&c.CharacterID()&&c.HasInventorySessionReservation();}
GuildPortal::Point Position(Client& c){return {c.GetX(),c.GetY(),c.GetZ()};}
bool Builtin(const Source& s){return s.builtin_pad&&s.scope.zone==345&&s.binding.item_id==66640&&s.binding.service==ServiceNpcs::Service::TeleportMerchant&&s.pad_position.x==StandardBuiltinGeode.x&&s.pad_position.y==StandardBuiltinGeode.y&&s.pad_position.z==StandardBuiltinGeode.z;}
GuildPortal::Point Position(const Source& s){if(s.builtin_pad){if(!Builtin(s))throw std::invalid_argument("Invalid builtin guild portal point");return s.pad_position;}return {s.binding.x,s.binding.y,s.binding.z};}
bool Shape(const Source& s,bool pad){const auto& b=s.binding;const bool kind=pad&&s.builtin_pad?Builtin(s):!s.builtin_pad&&b.item_id==(pad?66646u:66640u)&&b.service==(pad?ServiceNpcs::Service::TeleportPad:ServiceNpcs::Service::TeleportMerchant);return kind&&GuildPortal::Valid(s.scope)&&b.house_id==s.scope.house&&b.estate_id==s.scope.estate&&b.guild_id==s.scope.guild&&b.zone_id==s.scope.zone&&b.instance_id==s.scope.instance&&b.custody_id&&b.native_item_id&&b.native_estate_id&&b.guid.size()==16&&zone&&b.zone_id==zone->GetZoneID()&&b.instance_id==zone->GetInstanceID();}
bool SameBinding(const ServiceNpcs::Binding& a,const ServiceNpcs::Binding& b){return a.house_id==b.house_id&&a.estate_id==b.estate_id&&a.custody_id==b.custody_id&&a.guid==b.guid&&a.native_estate_id==b.native_estate_id&&a.native_item_id==b.native_item_id&&a.item_id==b.item_id&&a.npc_type_id==b.npc_type_id&&a.guild_id==b.guild_id&&a.zone_id==b.zone_id&&a.instance_id==b.instance_id&&a.service==b.service&&a.x==b.x&&a.y==b.y&&a.z==b.z&&a.heading==b.heading;}
std::unique_ptr<Database> Connection(){const auto* c=EQEmuConfig::get();if(!c)throw std::runtime_error("Missing portal DB configuration");auto db=std::make_unique<Database>();if(!db->Connect(c->DatabaseHost,c->DatabaseUsername,c->DatabasePassword,c->DatabaseDB,c->DatabasePort,"housing-guild-portal-runtime"))throw std::runtime_error("Portal DB unavailable");return db;}
State& CurrentState(Client& c){auto& s=states[&c];if(s.character!=c.CharacterID()||s.account!=c.AccountID()||s.zone!=zone->GetZoneID()||s.instance!=zone->GetInstanceID()){s={};s.character=c.CharacterID();s.account=c.AccountID();s.zone=zone->GetZoneID();s.instance=zone->GetInstanceID();}return s;}
}
void Configure(Resolver r,Validator v){states.clear();resolve=std::move(r);validate=std::move(v);}
void Clear()noexcept{states.clear();resolve={};validate={};}
void Forget(Client& c)noexcept{states.erase(&c);}
bool HandIn(Client& c,Mob* merchant)noexcept{
 bool returning=false;auto return_items=[&]{returning=true;c.ResetTrade();};
 try{
  if(!ServiceNpcs::Managed(merchant))return false;auto live=ServiceNpcs::Find(merchant);
  if(live&&live->service!=ServiceNpcs::Service::TeleportMerchant){c.Message(Chat::Yellow,"Please use this assistant's service window. Handed items and coins have been returned.");return_items();return true;}
  // A stale managed entity cannot fall through to a legacy EVENT_TRADE script.
  if(!live||!Ready(c)||!resolve||!validate||!ServiceNpcs::Allows(c,merchant,ServiceNpcs::Service::TeleportMerchant)){c.Message(Chat::Red,"This housing service is not available.");return_items();return true;}
  std::optional<Source> source;for(const auto& candidate:resolve(c,false))if(Shape(candidate,false)&&SameBinding(candidate.binding,*live)){if(source)throw std::invalid_argument("Ambiguous portal merchant");source=candidate;}
  if(!source||!c.trade||c.trade->cp||c.trade->sp||c.trade->gp||c.trade->pp){c.Message(Chat::Red,"Hand the assistant one supported teleportation crystal. Coins are not accepted.");return_items();return true;}
  int16_t slot=-1;EQ::ItemInstance* stone=nullptr;for(int16_t i=EQ::invslot::TRADE_BEGIN;i<=EQ::invslot::TRADE_NPC_END;++i){auto* item=c.GetInv().GetItem(i);if(!item)continue;if(stone||!GuildPortal::Purchasable(item->GetID())){c.Message(Chat::Red,"Only one supported crystal type may be handed in at a time.");return_items();return true;}slot=i;stone=item;}
  auto* bridge=c.GetHousingInventoryBridge();if(!bridge||!stone){return_items();return true;}
  auto aligned=bridge->AlignGuildPortal(*source,uint32_t(slot),stone->GetSerialNumber(),validate);
  // ResetTrade is the ordinary unconsumed-item/coin return path. The bridge has
  // already removed exactly one stone from memory and DB atomically.
  return_items();if(!aligned){c.Message(Chat::Red,"The portal could not be aligned. Your remaining items were returned.");return true;}
  Forget(c);const auto* d=GuildPortal::Lookup(aligned.value.stone);c.Message(Chat::Yellow,"The teleport stone is now aligned to %s for one day.",d?d->name:"the selected destination");return true;
 }catch(...){if(!returning){try{return_items();}catch(...){}}c.Message(Chat::Red,"The portal could not be aligned. Your items were returned.");return true;}
}
void Process(Client& c)noexcept{
 try{
  if(!Ready(c)||!resolve||!validate){Forget(c);return;}auto& state=CurrentState(c);const auto tick=std::chrono::steady_clock::now();if(tick<state.next)return;state.next=tick+std::chrono::seconds(1);
  std::optional<Source> pad;for(const auto& candidate:resolve(c,true))if(Shape(candidate,true)&&GuildPortal::Near(Position(c),Position(candidate))){if(pad&&candidate.binding.custody_id>=pad->binding.custody_id)continue;pad=candidate;}
  if(!pad){state.pending.reset();state.last_pad=0;return;}if(state.last_pad==pad->binding.custody_id)return;
  auto db=Connection();const auto now=GuildPortal::DatabaseNow(*db);if(state.pending&&now>=state.pending->expires)state.pending.reset();if(state.pending)return;
  // Offers are read-only and are not authority for a later response. Confirmation
  // reruns source, entry, pose, owning hall, alignment revision and distance.
  std::optional<GuildPortal::Confirmation> offer;
  GuildPortal::Query(*db,"START TRANSACTION");try{if(!validate(*db,c,*pad))throw std::invalid_argument("Portal offer source changed");auto alignment=GuildPortal::Read(*db,pad->scope,true);if(alignment)offer=GuildPortal::Offer(*alignment,pad->scope,c.CharacterID(),pad->binding.custody_id,Position(c),Position(*pad),GuildPortal::DatabaseNow(*db));GuildPortal::Query(*db,"COMMIT");}catch(...){db->QueryDatabase(std::string("ROLLBACK"),false);throw;}if(!offer)return;
  state.pending=*offer;state.last_pad=pad->binding.custody_id;const auto* d=GuildPortal::Lookup(offer->stone);std::string text="Do you wish to travel to ";text+=d->name;text+="?";c.SendFullPopup("Guild Hall Portal",text.c_str(),offer->stone,offer->stone,1,uint32_t(GuildPortal::ConfirmationSeconds),"Yes","No",0);
 }catch(...){auto it=states.find(&c);if(it!=states.end()){it->second.pending.reset();it->second.last_pad=0;it->second.next=std::chrono::steady_clock::now()+std::chrono::seconds(1);}}
}
bool HandlePopup(Client& c,const EQApplicationPacket* app)noexcept{
 try{
  if(!app||app->size!=sizeof(PopupResponse_Struct))return false;const auto* response=reinterpret_cast<const PopupResponse_Struct*>(app->pBuffer);if(!GuildPortal::Lookup(response->popupid)&&response->popupid!=88737&&response->popupid!=76008)return false;
  auto it=states.find(&c);std::optional<GuildPortal::Confirmation> pending;if(it!=states.end()){pending=it->second.pending;it->second.pending.reset();}
  if(!Ready(c))return bool(pending);auto db=Connection();auto current=GuildPortal::Current(*db,zone->GetZoneID(),zone->GetInstanceID(),false);
  // In housing halls, never delegate these legacy stone IDs to the basic hall
  // popup quest: an arbitrary popup ID must not bypass the placed pad contract.
  // Original755080 sends kind0 for LDW_Yes/Okay and kind1 for LDW_No.
  // Both IDs point here so No clears this one-shot state without teleporting.
  if(!current)return bool(pending)||(c.GetEntityVariable("housing_verified_interior")==std::to_string(zone->GetInstanceID()));if(response->unknown0000!=0||!pending||!resolve||!validate)return true;
  std::optional<Source> pad;for(const auto& candidate:resolve(c,true))if(Shape(candidate,true)&&candidate.binding.custody_id==pending->pad_custody&&GuildPortal::Same(candidate.scope,pending->scope)){if(pad)return true;pad=candidate;}if(!pad)return true;
  bool tx=false;const GuildPortal::Destination* destination=nullptr;
  try{GuildPortal::Query(*db,"START TRANSACTION");tx=true;if(!validate(*db,c,*pad))throw std::invalid_argument("Portal pad changed");auto alignment=GuildPortal::Read(*db,pad->scope,true);if(!alignment)throw std::invalid_argument("Portal alignment missing");destination=GuildPortal::Confirm(*pending,*alignment,pad->scope,c.CharacterID(),pad->binding.custody_id,Position(c),Position(*pad),response->popupid,GuildPortal::DatabaseNow(*db));if(!destination)throw std::invalid_argument("Portal confirmation stale");GuildPortal::Query(*db,"COMMIT");tx=false;}catch(...){if(tx)db->QueryDatabase(std::string("ROLLBACK"),false);return true;}
  Forget(c);c.MovePC(destination->zone,destination->x,destination->y,destination->z,0);return true;
 }catch(...){Forget(c);return zone&&zone->GetInstanceID()&&c.GetEntityVariable("housing_verified_interior")==std::to_string(zone->GetInstanceID());}
}
}
