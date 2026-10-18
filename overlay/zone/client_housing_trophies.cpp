#include "client.h"
#include "worldserver.h"
#include "entity.h"
extern WorldServer worldserver;
#include "housing_inventory_bridge.h"
#include "housing_trophy_stat_item.h"
#include "../common/rulesys.h"
#include <cstring>
namespace {
bool Ready(Client& c){return worldserver.Connected()&&RuleB(Housing,EnablePlayerHousing)&&c.ClientVersion()==EQ::versions::ClientVersion::RoF2&&c.Connected()&&c.ClientDataLoaded()&&c.HasInventorySessionReservation()&&c.GetHP()>0;}
void Send(Client& c,EmuOpcode opcode,const std::vector<uint8_t>& bytes){EQApplicationPacket packet(opcode,bytes.size());if(!bytes.empty())std::memcpy(packet.pBuffer,bytes.data(),bytes.size());c.QueuePacket(&packet);}
void Install(Client& c,const Housing::TrophyBenefits::View& view){
 c.m_housing_trophy_stat_items=view.stat_items;
 c.m_housing_trophy_deadline=view.selected.enabled&&view.remaining_ms?std::chrono::steady_clock::now()+std::chrono::milliseconds(view.remaining_ms):std::chrono::steady_clock::time_point{};
}
void Sync(Client& c,const Housing::TrophyBenefits::View& view){
 if(view.favor_after)c.AddTributePoints(0);
 if(view.insufficient_favor)c.MessageString(Chat::Red,5253);
 if(view.billing_unavailable)c.Message(Chat::Red,"Paid trophy billing is unavailable until its period and policy have been reviewed.");
 Send(c,OP_RealEstateTrophyTimer,Housing::TrophyAnchorWire::EncodeTimer(view.remaining_ms));Send(c,OP_RealEstateTrophyList,Housing::TrophyAnchorWire::EncodeBenefits(view.selected));if(!view.selected.enabled)Send(c,OP_RealEstateTrophyClear,{});Housing::TrophyStatItem::Send(c,view.stat_item_slots,false);}
}
void Client::InvalidateHousingTrophyBenefits(){
 InvalidateHousingGuildTrophyBenefits();
 m_housing_trophy_dirty=true;m_housing_trophy_deadline={};
 if(!m_housing_trophy_stat_items.empty()){m_housing_trophy_stat_items.clear();if(Connected()&&m_housing_trophy_sent)Send(*this,OP_RealEstateTrophyClear,{});CalcBonuses();SendHPUpdate(true);}
}
void Client::HousingTrophySourcesChanged() noexcept{
 try{
  for(const auto& entry:entity_list.GetClientList())entry.second->InvalidateHousingTrophyBenefits();
  if(worldserver.Connected()){ServerPacket packet(ServerOP_HousingTrophyRefresh,0);worldserver.SendPacket(&packet);}
 }catch(...){}
}
void Client::ProcessHousingTrophyBenefits(){
 const bool ready=Ready(*this);const auto capacity=HousingTrophyCapacity();const bool renew=ready&&m_housing_trophy_deadline!=std::chrono::steady_clock::time_point{}&&std::chrono::steady_clock::now()>=m_housing_trophy_deadline;
 if(renew)InvalidateHousingTrophyBenefits();
 if(ready!=m_housing_trophy_ready||capacity!=m_housing_trophy_capacity){m_housing_trophy_ready=ready;m_housing_trophy_capacity=capacity;InvalidateHousingTrophyBenefits();}
 if(ready&&m_housing_trophy_dirty){m_housing_trophy_dirty=false;RefreshHousingTrophyBenefits(true,true,renew);}
}
uint32_t Client::HousingTrophyCapacity()const{return Housing::TrophyBenefits::Slots(int64_t(aabonuses.ExpandMaxActiveTrophyBenefits)+spellbonuses.ExpandMaxActiveTrophyBenefits+itembonuses.ExpandMaxActiveTrophyBenefits);}
void Client::RefreshHousingTrophyBenefits(bool notify,bool recalculate,bool renew,bool force_publish){
 std::vector<uint32_t> prior=m_housing_trophy_stat_items;m_housing_trophy_stat_items.clear();
 if(Ready(*this))try{if(auto* bridge=GetHousingInventoryBridge()){Housing::TrophyBenefits::Request request;request.level=GetLevel();request.capacity=HousingTrophyCapacity();if(renew)request.action=Housing::TrophyBenefits::Action::Renew;auto result=bridge->Trophies(request);if(result){Install(*this,result.value);if(notify&&(force_publish||renew||result.value.insufficient_favor||result.value.billing_unavailable||!m_housing_trophy_sent||prior!=m_housing_trophy_stat_items)){Sync(*this,result.value);m_housing_trophy_sent=true;}}}}catch(...){}
 if(notify&&!prior.empty()&&m_housing_trophy_stat_items.empty())Send(*this,OP_RealEstateTrophyClear,{});
 if(recalculate&&prior!=m_housing_trophy_stat_items){CalcBonuses();SendHPUpdate(true);}
}
void Client::Handle_OP_RealEstateTrophyOpen(const EQApplicationPacket* app){
 if(!app||app->size!=0||!Ready(*this))return;
 const auto now=std::chrono::steady_clock::now();
 if(m_housing_trophy_open_last!=std::chrono::steady_clock::time_point{}&&now-m_housing_trophy_open_last<std::chrono::seconds(1))return;
 m_housing_trophy_open_last=now;
 // Original51a9 has no domain selector. Refreshing both authenticated domains
 // is our compatibility policy, not a recovered original response contract.
 // Read/revalidation may prune invalid state; open never requests renewal,
 // activation, selection changes, opt-in or a guild modify permission grant.
 const bool guilded=GuildID()&&GuildID()!=GUILD_NONE;
 LogInfo("Native trophy window open actor [{}] personal forced-read refresh; guild [{}] requested",CharacterID(),guilded?"read refresh":"clear");
 RefreshHousingTrophyBenefits(true,true,false,true);
 if(guilded)RefreshHousingGuildTrophyBenefits(false);
 else InvalidateHousingGuildTrophyBenefits(false);
}
void Client::Handle_OP_RealEstateTrophy(const EQApplicationPacket* app){
 if(!app||!Ready(*this))return;auto selection=Housing::TrophyAnchorWire::DecodeSelection(app->pBuffer,app->size);if(!selection)return;
 auto* bridge=GetHousingInventoryBridge();if(!bridge)return;Housing::TrophyBenefits::Request request;request.action=Housing::TrophyBenefits::Action::Select;request.selection=*selection;request.level=GetLevel();request.capacity=HousingTrophyCapacity();
 auto result=bridge->Trophies(request);LogInfo("Native trophy selection actor [{}] slot [{}] benefit [{}] tier [{}] backend [{}]",CharacterID(),selection->slot,selection->benefit_id,selection->tier,int(result.error));if(result)Sync(*this,result.value);else{RefreshHousingTrophyBenefits(false,false);Housing::TrophyBenefits::Request read;read.level=GetLevel();read.capacity=HousingTrophyCapacity();auto current=bridge->Trophies(read);if(current)Sync(*this,current.value);}
 RefreshHousingTrophyBenefits(false,false);CalcBonuses();SendHPUpdate(true);
}
void Client::Handle_OP_RealEstateTrophyToggle(const EQApplicationPacket* app){
 if(!app||!Ready(*this)||app->size!=4)return;const uint32_t enable=Housing::TrophyAnchorWire::Detail::Read(app->pBuffer);if(enable>1)return;
 auto* bridge=GetHousingInventoryBridge();if(!bridge)return;Housing::TrophyBenefits::Request request;request.action=enable?Housing::TrophyBenefits::Action::Enable:Housing::TrophyBenefits::Action::Disable;request.level=GetLevel();request.capacity=HousingTrophyCapacity();auto result=bridge->Trophies(request);
 if(result)Sync(*this,result.value);else{Message(Chat::Red,"The selected trophy benefits are unavailable or their billing policy is not reviewed.");Send(*this,OP_RealEstateTrophyClear,{});}
 RefreshHousingTrophyBenefits(false,false);CalcBonuses();SendHPUpdate(true);
}
