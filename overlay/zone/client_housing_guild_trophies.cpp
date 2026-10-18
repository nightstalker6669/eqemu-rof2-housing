#include "client.h"
#include "worldserver.h"
#include "housing_inventory_bridge.h"
#include "housing_trophy_stat_item.h"
#include "../common/rulesys.h"
#include <ctime>
extern WorldServer worldserver;
namespace {
namespace GT=Housing::GuildTrophies;namespace GW=Housing::GuildTrophyWire;
bool Ready(Client& c){return worldserver.Connected()&&RuleB(Housing,EnablePlayerHousing)&&c.ClientVersion()==EQ::versions::ClientVersion::RoF2&&c.Connected()&&c.ClientDataLoaded()&&c.HasInventorySessionReservation()&&c.GetHP()>0&&c.GuildID()&&c.GuildID()!=GUILD_NONE;}
void Send(Client& c,EmuOpcode opcode,const std::vector<uint8_t>& body){EQApplicationPacket packet(opcode,body.size());if(!body.empty())std::memcpy(packet.pBuffer,body.data(),body.size());c.QueuePacket(&packet);}
void Clear(Client& c){Send(c,OP_GuildTrophyAvailable,GW::Available(c.GuildID(),{}));Send(c,OP_GuildTrophyActive,GW::Active(c.GuildID(),0,0,false,GW::Bank{}));}
void Install(Client& c,const GT::View& view){const auto old=c.m_housing_guild_trophy_stat_items;c.m_housing_guild_trophy_stat_items=view.stat_items;c.m_housing_guild_trophy_guild=view.member.guild;c.m_housing_guild_trophy_deadline=view.state.enabled&&view.remaining_ms?std::chrono::steady_clock::now()+std::chrono::milliseconds(view.remaining_ms):std::chrono::steady_clock::time_point{};
 Send(c,OP_GuildTrophyAvailable,GW::Available(view.member.guild,view.available));Send(c,OP_GuildTrophyActive,GW::Active(view.member.guild,view.member.favor,view.remaining_ms,view.state.enabled,view.state.bank));Housing::TrophyStatItem::Send(c,view.stat_item_slots,true);
 EQApplicationPacket reply(OP_GuildOptInOut,sizeof(GuildTributeOptInOutReply_Struct));auto* member=reinterpret_cast<GuildTributeOptInOutReply_Struct*>(reply.pBuffer);member->guild_id=view.member.guild;strn0cpy(member->player_name,c.GetCleanName(),sizeof member->player_name);member->tribute_toggle=view.member.ordinary_opt_in;member->tribute_trophy_toggle=view.member.opt_in;member->no_donations=view.member.donations;member->time=uint32(std::time(nullptr));member->command=1;c.QueuePacket(&reply);
 if(old!=c.m_housing_guild_trophy_stat_items){c.CalcBonuses();c.SendHPUpdate(true);}
}
void Apply(Client& c,GT::Request request){if(!Ready(c))return;auto* bridge=c.GetHousingInventoryBridge();if(!bridge)return;request.level=c.GetLevel();auto result=bridge->GuildTrophiesView(request);if(!result){c.InvalidateHousingGuildTrophyBenefits();c.Message(Chat::Red,"The guild trophy change is unavailable.");return;}Install(c,result.value);if(result.value.changed)c.SendHousingGuildTrophyRequest(0,result.value.changed_member);}
}
uint32 Client::HousingGuildTrophyRemaining()const{if(m_housing_guild_trophy_deadline==std::chrono::steady_clock::time_point{}||m_housing_guild_trophy_deadline<=std::chrono::steady_clock::now())return 0;auto left=std::chrono::duration_cast<std::chrono::milliseconds>(m_housing_guild_trophy_deadline-std::chrono::steady_clock::now()).count();return left>0&&left<=INT32_MAX?uint32(left):0;}
void Client::SendHousingGuildTrophyRequest(uint32 action,uint32 target){if(action>2||!worldserver.Connected()||!Connected()||!CharacterID())return;ServerPacket packet(ServerOP_HousingGuildTrophyRequest,sizeof(HousingGuildTrophyRequest));auto* request=reinterpret_cast<HousingGuildTrophyRequest*>(packet.pBuffer);request->character_id=CharacterID();request->action=action;request->target_id=target;worldserver.SendPacket(&packet);}
void Client::InvalidateHousingGuildTrophyBenefits(bool request_world){m_housing_guild_trophy_dirty=true;m_housing_guild_trophy_world_request=m_housing_guild_trophy_world_request||request_world;m_housing_guild_trophy_deadline={};if(Connected()&&ClientVersion()==EQ::versions::ClientVersion::RoF2)Clear(*this);if(!m_housing_guild_trophy_stat_items.empty()){m_housing_guild_trophy_stat_items.clear();CalcBonuses();SendHPUpdate(true);}}
void Client::RefreshHousingGuildTrophyBenefits(bool request_world){if(!Ready(*this))return;auto* bridge=GetHousingInventoryBridge();if(!bridge)return;GT::Request request;request.level=GetLevel();auto result=bridge->GuildTrophiesView(request);if(!result){InvalidateHousingGuildTrophyBenefits();m_housing_guild_trophy_dirty=false;return;}Install(*this,result.value);if(request_world||result.value.changed)SendHousingGuildTrophyRequest();}
void Client::ProcessHousingGuildTrophyBenefits(){bool ready=Ready(*this);bool due=ready&&m_housing_guild_trophy_deadline!=std::chrono::steady_clock::time_point{}&&std::chrono::steady_clock::now()>=m_housing_guild_trophy_deadline;
 if(ready!=m_housing_guild_trophy_ready||GetLevel()!=m_housing_guild_trophy_level||(ready&&GuildID()!=m_housing_guild_trophy_guild)){m_housing_guild_trophy_ready=ready;m_housing_guild_trophy_level=GetLevel();InvalidateHousingGuildTrophyBenefits(true);}
 if(due)InvalidateHousingGuildTrophyBenefits(true);
 if(ready&&m_housing_guild_trophy_dirty){auto request_world=m_housing_guild_trophy_world_request;m_housing_guild_trophy_dirty=false;m_housing_guild_trophy_world_request=false;RefreshHousingGuildTrophyBenefits(request_world);}
}
void Client::Handle_OP_GuildTrophyToggle(const EQApplicationPacket* app){if(!app||!Ready(*this))return;auto value=GW::Toggle(app->pBuffer,app->size);if(!value)return;InvalidateHousingGuildTrophyBenefits();SendHousingGuildTrophyRequest(*value?1:2);}
void Client::Handle_OP_GuildTrophySave(const EQApplicationPacket* app){if(!app)return;auto bank=GW::Save(app->pBuffer,app->size);if(!bank)return;GT::Request request;request.action=GT::Action::Save;request.bank=*bank;Apply(*this,request);}
void Client::Handle_OP_GuildTrophyOptIn(const EQApplicationPacket* app){if(!app)return;auto member=GW::Member(app->pBuffer,app->size);if(!member)return;GT::Request request;request.action=GT::Action::OptIn;request.opt_in=*member;Apply(*this,request);}
void Client::Handle_OP_GuildTrophyModify(const EQApplicationPacket* app){if(!app||!Ready(*this))return;auto modify=GW::Modify(app->pBuffer,app->size);if(!modify)return;auto* bridge=GetHousingInventoryBridge();if(!bridge)return;GT::Request request;request.action=GT::Action::Modify;request.level=GetLevel();auto result=bridge->GuildTrophiesView(request);const bool allowed=result&&result.value.can_modify&&*modify;Send(*this,OP_GuildTrophyModify,{uint8_t(allowed)});if(result)Install(*this,result.value);}
