#include "housing_service_npcs.h"
#include "client.h"
#include "entity.h"
#include "guild_mgr.h"
#include "housing_service_bank.h"
#include "npc.h"
#include "zone.h"
#include "zonedb.h"
#include "../common/classes.h"
#include "../common/housing/service_item_bindings.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <set>
extern Zone* zone;
extern ZoneDatabase content_db;
namespace Housing::ServiceNpcs {
namespace {
struct Entry { Binding binding; uint16_t entity=0; NPC* owned=nullptr; };
std::map<uint64_t,Entry> entries;
Revalidator revalidate;
AccessValidator validate_access;
uint32_t active_zone=0;
uint16_t active_instance=0;
uint8_t ClassFor(Service s){switch(s){
 case Service::Vendor:case Service::TeleportMerchant:return Class::Merchant;
 case Service::Banker:return Class::Banker;
 case Service::PersonalTribute:return Class::TributeMaster;
 case Service::GuildTribute:return Class::GuildTributeMaster;
 case Service::GuildBank:return Class::GuildBanker;
 case Service::TeleportPad:return Class::Warrior;
 }return 0;}
ServiceItemBindings::Kind KindFor(Service s){switch(s){
 case Service::Vendor:return ServiceItemBindings::Kind::Vendor;
 case Service::Banker:return ServiceItemBindings::Kind::Banker;
 case Service::PersonalTribute:return ServiceItemBindings::Kind::PersonalTribute;
 case Service::GuildTribute:return ServiceItemBindings::Kind::GuildTribute;
 case Service::GuildBank:return ServiceItemBindings::Kind::GuildBank;
 case Service::TeleportMerchant:return ServiceItemBindings::Kind::TeleportMerchant;
 case Service::TeleportPad:return ServiceItemBindings::Kind::TeleportPad;
 }return ServiceItemBindings::Kind::Vendor;}
bool GuildOnly(Service s){return s==Service::GuildTribute||s==Service::GuildBank||s==Service::TeleportMerchant||s==Service::TeleportPad;}
uint64_t Key(const Binding&b){return b.custody_id;}
bool Source(const Binding&b){return b.custody_id&&b.native_item_id&&b.item_id&&b.guid.size()==16;}
bool Same(const Binding&a,const Binding&b){return a.guid==b.guid&&a.house_id==b.house_id&&a.estate_id==b.estate_id&&a.custody_id==b.custody_id&&a.native_estate_id==b.native_estate_id&&a.native_item_id==b.native_item_id&&a.item_id==b.item_id&&a.npc_type_id==b.npc_type_id&&a.guild_id==b.guild_id&&a.zone_id==b.zone_id&&a.instance_id==b.instance_id&&a.service==b.service&&a.x==b.x&&a.y==b.y&&a.z==b.z&&a.heading==b.heading;}
bool Finite(const Binding&b){return std::isfinite(b.x)&&std::isfinite(b.y)&&std::isfinite(b.z)&&std::isfinite(b.heading)&&std::abs(b.x)<=65536&&std::abs(b.y)<=65536&&std::abs(b.z)<=65536&&b.heading>=0&&b.heading<512;}
bool MerchantCatalog(const Binding&b,const NPCType&source){
 if(b.service!=Service::Vendor&&b.service!=Service::TeleportMerchant)return true;
 if(!zone->merchanttable.count(source.merchanttype))zone->LoadNewMerchantData(source.merchanttype);
 auto catalog=zone->merchanttable.find(source.merchanttype);if(catalog==zone->merchanttable.end()||catalog->second.empty())return false;
 // The basic script has no valid destination for these two stones. Do not
 // advertise them in the managed assistant shop while leaving basic halls alone.
 if(b.service==Service::TeleportMerchant)catalog->second.remove_if([](const auto& row){return row.item==88737||row.item==76008;});
 if(!zone->tmpmerchanttable.count(b.npc_type_id)){
  auto rows=database.QueryDatabase("SELECT slot,charges,itemid FROM merchantlist_temp WHERE npcid="+std::to_string(b.npc_type_id)+" AND zone_id="+std::to_string(b.zone_id)+" AND instance_id="+std::to_string(b.instance_id)+" ORDER BY slot",false);
  if(!rows.Success()||rows.RowCount()>1000)return false;std::list<TempMerchantList> restored;std::set<uint32_t> slots;
  for(auto row=rows.begin();row!=rows.end();++row){if(!row[0]||!row[1]||!row[2])return false;TempMerchantList item{};item.npcid=b.npc_type_id;item.slot=std::stoul(row[0]);item.origslot=item.slot;item.charges=std::stoul(row[1]);item.item=std::stoul(row[2]);if(!item.slot||!item.item||!slots.insert(item.slot).second)return false;restored.push_back(item);}
  if(b.service==Service::TeleportMerchant)restored.remove_if([](const auto& row){return row.item==88737||row.item==76008;});
  zone->tmpmerchanttable.emplace(b.npc_type_id,std::move(restored));
 }
 return true;
}
bool Route(){return zone&&active_zone==zone->GetZoneID()&&active_instance==zone->GetInstanceID()&&active_instance;}
// FaceTarget/DoQuestPause can cosmetically turn even AI-stopped service NPCs.
// Only the immutable binding/DB pose owns placed heading; runtime yaw is not
// service identity or spherical height-query geometry authority.
bool Live(const Entry&e){auto* m=entity_list.GetMob(e.entity);return m&&m==e.owned&&m->IsNPC()&&!m->CastToNPC()->GetDepop()&&m->GetNPCTypeID()==e.binding.npc_type_id&&m->GetClass()==ClassFor(e.binding.service)&&m->GetX()==e.binding.x&&m->GetY()==e.binding.y&&m->GetZ()==e.binding.z&&std::isfinite(m->GetHeading())&&m->GetHousingItemGuid()==e.binding.guid&&m->GetHousingEstateId()==e.binding.native_estate_id&&m->GetHousingItemId()==e.binding.native_item_id;}
void Remove(Entry&e){if(auto*m=entity_list.GetMob(e.entity);m&&m==e.owned&&m->IsNPC())m->CastToNPC()->Depop(false);}
Entry* Lookup(const Mob*m){if(!m)return nullptr;for(auto&[key,e]:entries)if(e.owned==m&&entity_list.GetMob(e.entity)==m)return &e;return nullptr;}
}
void Clear()noexcept{try{for(auto&[key,e]:entries)Remove(e);}catch(...){}entries.clear();revalidate={};validate_access={};active_zone=0;active_instance=0;}
void Revoke(uint64_t custody_id)noexcept{
 try{auto i=entries.find(custody_id);if(i==entries.end())return;Remove(i->second);entries.erase(i);}catch(...){}
}
bool Reconcile(const std::vector<Binding>& wanted,Revalidator validate,AccessValidator access){
 try{
  if(!zone||!zone->GetInstanceID()||!validate||!access||wanted.size()>128){Clear();return false;}
  if(!entries.empty()&&!Route())Clear();
  std::set<uint64_t> identities;std::set<uint32_t> native_ids;
  std::map<uint64_t,std::unique_ptr<NPCType>> types;
  for(const auto&b:wanted){
   if(!b.house_id||!b.estate_id||!Source(b)||!b.native_estate_id||!b.npc_type_id||!Finite(b)||!ClassFor(b.service)||b.zone_id!=zone->GetZoneID()||b.instance_id!=zone->GetInstanceID()||(GuildOnly(b.service)&&!b.guild_id)||!identities.insert(Key(b)).second||(b.native_item_id&&!native_ids.insert(b.native_item_id).second)||!validate(b)){Clear();return false;}
   const auto* reviewed=ServiceItemBindings::Lookup(b.item_id);
   if(!reviewed||reviewed->npc_type_id!=b.npc_type_id||reviewed->kind!=KindFor(b.service)){Clear();return false;}
   const auto* source=content_db.LoadNPCTypesData(b.npc_type_id);
   uint32_t source_size_bits=0;if(source)std::memcpy(&source_size_bits,&source->size,sizeof(source_size_bits));
   if(!source||source->npc_id!=reviewed->npc_type_id||source->class_!=ClassFor(b.service)||source->race!=reviewed->race||source->gender!=reviewed->gender||source_size_bits!=reviewed->default_size_bits||!std::isfinite(source->size)||source->size<=0||source->size>256){Clear();return false;}
   // Vendor and gnome are reviewed original basic-hall equivalents. Refuse a
   // repurposed source row/catalogue instead of silently changing the service.
   if((b.service==Service::Vendor&&(source->merchanttype!=345000||std::string_view(source->name)!="Yenny_Werlikanin"))||(b.service==Service::TeleportMerchant&&(source->merchanttype!=345004||std::string_view(source->name)!="Zeflmin_Werlikanin"))){Clear();return false;}
   if(!MerchantCatalog(b,*source)){Clear();return false;}
   auto copy=std::make_unique<NPCType>(*source);
   // Service entities retain the reviewed original appearance/class/catalogue;
   // they never inherit combat, loot, wander or unrelated NPC spell effects.
   copy->loottable_id=0;copy->npc_spells_id=0;copy->npc_spells_effects_id=0;copy->npc_faction_id=0;copy->faction_amount=0;copy->emoteid=0;
   copy->npc_aggro=false;copy->aggroradius=0;copy->assistradius=0;copy->runspeed=0;copy->skip_global_loot=true;copy->skip_auto_scale=true;copy->unique_spawn_by_name=false;copy->spawn_limit=0;copy->findable=false;copy->trackable=false;copy->untargetable=false;copy->show_name=source->show_name;
   types.emplace(Key(b),std::move(copy));
  }
  active_zone=zone->GetZoneID();active_instance=zone->GetInstanceID();revalidate=std::move(validate);validate_access=std::move(access);
  for(auto i=entries.begin();i!=entries.end();){const Binding* match=nullptr;for(const auto&b:wanted)if(Key(b)==i->first){match=&b;break;}if(!match||!Same(i->second.binding,*match)||!Live(i->second)){Remove(i->second);i=entries.erase(i);}else ++i;}
  for(const auto&b:wanted){if(entries.count(Key(b)))continue;
   auto type=std::move(types.at(Key(b)));
   auto npc=std::make_unique<NPC>(type.get(),nullptr,glm::vec4(b.x,b.y,b.z,b.heading),GravityBehavior::Flying);
   npc->GiveNPCTypeData(type.release());
   if(!npc->SetHousingItemBinding(b.guid,b.native_estate_id,b.native_item_id)){Clear();return false;}npc->AI_Stop();npc->Mob::AI_Stop();npc->SetInvul(true);npc->SaveGuardSpot();
   npc->SetEntityVariable("housing_service_owned","1");
   npc->SetEntityVariable("housing_service_custody",std::to_string(b.custody_id));
   npc->SetEntityVariable("housing_service_house",std::to_string(b.house_id));
   npc->SetEntityVariable("housing_service_estate",std::to_string(b.estate_id));
   npc->SetEntityVariable("housing_service_guild",std::to_string(b.guild_id));
   entity_list.AddNPC(npc.get(),true,true);
   if(!npc->GetID()){Clear();return false;}
   Entry e{b,npc->GetID(),npc.get()};npc.release();entries.emplace(Key(b),e);
  }
  if(!wanted.empty()&&std::any_of(wanted.begin(),wanted.end(),[](const Binding&b){return b.service==Service::GuildBank;})&&!GuildBanks)GuildBanks=new GuildBankManager;
  return true;
 }catch(...){Clear();return false;}
}
bool Managed(const Mob*m)noexcept{try{return m&&const_cast<Mob*>(m)->GetEntityVariable("housing_service_owned")=="1";}catch(...){return false;}}
std::optional<Binding> LiveBinding(const Mob*m)noexcept{try{auto*e=Lookup(m);if(!e||!Route()||!Live(*e))return {};return e->binding;}catch(...){return {};}}
std::vector<Binding> LiveBindings()noexcept{std::vector<Binding> result;try{if(!Route())return {};for(auto&[key,e]:entries)if(Live(e))result.push_back(e.binding);}catch(...){return {};}return result;}
std::optional<Binding> Find(const Mob*m)noexcept{try{auto*e=Lookup(m);if(!e||!Route()||!Live(*e)||!revalidate||!revalidate(e->binding))return {};return e->binding;}catch(...){return {};}}
std::vector<Binding> Bindings()noexcept{std::vector<Binding> result;try{if(!Route()||!revalidate)return {};for(auto&[key,e]:entries)if(Live(e)&&revalidate(e.binding))result.push_back(e.binding);}catch(...){return {};}return result;}
bool PublishBindings(Client&c,const std::map<uint32_t,std::string>& guids)noexcept{
 try{if(!Route()||!c.Connected()||!c.ClientDataLoaded()||!revalidate||!validate_access)return false;
  bool owning_guild_bank=false;
  for(auto&[key,e]:entries){auto match=guids.find(e.binding.native_item_id);if(match==guids.end())continue;const auto&guid=match->second;
   if(guid.size()!=16||guid.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-")!=std::string::npos||!Live(e)||!revalidate(e.binding))return false;
   // An allowed visitor may lack access to a particular guild service. Its
   // actor confers no service authority; omit only this viewer's binding and
   // continue publishing other accessible items in the same room.
   if(!validate_access(c,e.binding))continue;
   EQApplicationPacket packet(OP_RealEstateBindSpawn,32);std::memset(packet.pBuffer,0,32);
   const auto word=[&](size_t at,uint32_t value){for(size_t n=0;n<4;++n)packet.pBuffer[at+n]=uint8_t(value>>(8*n));};
   word(0,e.entity);std::memcpy(packet.pBuffer+4,guid.data(),16);word(24,e.binding.native_estate_id);word(28,e.binding.native_item_id);c.QueuePacket(&packet);
   if(e.binding.service==Service::GuildBank&&c.IsInAGuild()&&c.GuildID()==e.binding.guild_id)owning_guild_bank=true;
  }
  // Publish the authenticated bank snapshot with its newly visible actor so
  // the first ordinary deposit has a current publication to compare against.
  // Subsequent requests still recheck the exact durable bank snapshot.
  return !owning_guild_bank||PublishGuildBank(c);
 }catch(...){return false;}
}
bool Allows(Client&c,const Mob*m,Service service)noexcept{
 try{auto*e=Lookup(m);if(!e)return !Managed(m);
  if(!Route()||!c.Connected()||!c.ClientDataLoaded()||!Live(*e)||e->binding.service!=service||!revalidate||!revalidate(e->binding)||!validate_access||!validate_access(c,e->binding)||(GuildOnly(service)&&(!c.IsInAGuild()||c.GuildID()!=e->binding.guild_id)))return false;
  const auto&b=e->binding;const double dx=c.GetX()-b.x,dy=c.GetY()-b.y,dz=c.GetZ()-b.z;
  return std::isfinite(dx)&&std::isfinite(dy)&&std::isfinite(dz)&&dx*dx+dy*dy+dz*dz<=USE_NPC_RANGE2;
 }catch(...){return false;}
}
bool HasNearby(Client&c,Service s)noexcept{try{for(auto&[key,e]:entries)if(e.binding.service==s){auto*m=entity_list.GetMob(e.entity);if(m&&m==e.owned&&Allows(c,m,s))return true;}}catch(...){}return false;}
bool AllowsMerchant(Client&c,const Mob*m)noexcept{try{if(!Managed(m))return true;auto b=Find(m);return b&&(b->service==Service::Vendor||b->service==Service::TeleportMerchant)&&Allows(c,m,b->service);}catch(...){return false;}}
bool PublishGuildBank(Client&c)noexcept{try{if(!Route()||!c.Connected()||!c.ClientDataLoaded()||!c.IsInAGuild())return false;for(auto&[key,e]:entries)if(e.binding.service==Service::GuildBank&&e.binding.guild_id==c.GuildID()&&Live(e)&&revalidate&&revalidate(e.binding)&&validate_access&&validate_access(c,e.binding)){if(!GuildBanks)GuildBanks=new GuildBankManager;return ServiceBank::Publish(c);}}catch(...){}return false;}
bool EnsureGuildBank(Client&c)noexcept{try{if(!HasNearby(c,Service::GuildBank))return false;if(!GuildBanks)GuildBanks=new GuildBankManager;return ServiceBank::Publish(c);}catch(...){return false;}}
}
