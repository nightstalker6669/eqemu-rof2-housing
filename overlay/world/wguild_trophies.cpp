#include "wguild_mgr.h"
#include "clientlist.h"
#include "cliententry.h"
#include "../common/eq_packet_structs.h"
#include "../common/rulesys.h"
#include "zonelist.h"
#include "../common/servertalk.h"
#include "../common/housing/guild_trophy_billing_db.h"
namespace {void Refresh(uint32 guild){ServerPacket packet(ServerOP_HousingGuildTrophyRefresh,sizeof(uint32));std::memcpy(packet.pBuffer,&guild,sizeof guild);ZSList::Instance()->SendPacketToBootedZones(&packet);}}
uint32 WorldGuildManager::GuildTrophyRemaining(uint32 guild)const{auto it=m_guild_trophy_deadlines.find(guild);if(it==m_guild_trophy_deadlines.end()||it->second<=std::chrono::steady_clock::now())return 0;auto left=std::chrono::duration_cast<std::chrono::milliseconds>(it->second-std::chrono::steady_clock::now()).count();return left>0&&left<=INT32_MAX?uint32(left):0;}
bool WorldGuildManager::UpdateGuildTrophies(uint32 guild,uint32 action,uint32 actor,uint32 account){
 if(!m_db||!guild||guild==UINT32_MAX||action>3)return false;m_guild_trophy_known.insert(guild);bool transaction=false;
 try{
  using namespace Housing;auto mapped=action==1?TrophyBenefits::BillAction::Enable:action==2?TrophyBenefits::BillAction::Disable:action==3?TrophyBenefits::BillAction::Renew:TrophyBenefits::BillAction::Read;
  auto due=m_guild_trophy_deadlines.find(guild);if(!action&&due!=m_guild_trophy_deadlines.end()&&due->second<=std::chrono::steady_clock::now())mapped=TrophyBenefits::BillAction::Renew;
  GuildTrophies::Q(*m_db,"START TRANSACTION");transaction=true;
  auto result=GuildTrophies::BillUnderTransaction(*m_db,guild,mapped,ClientList::Instance()->GetInZoneGuildLevels(guild),actor?std::optional<AuthenticatedPurchaser>{{actor,account}}:std::nullopt);
  if(!result){GuildTrophies::Q(*m_db,"ROLLBACK");transaction=false;m_guild_trophy_deadlines.erase(guild);Refresh(guild);return false;}
  GuildTrophies::Q(*m_db,"COMMIT");transaction=false;const auto& view=result.value;
  if(auto* current=GetGuildByGuildID(guild))current->tribute.favor=view.favor;
  m_guild_trophy_deadlines.erase(guild);if(view.state.enabled&&view.remaining_ms)m_guild_trophy_deadlines.emplace(guild,std::chrono::steady_clock::now()+std::chrono::milliseconds(view.remaining_ms));
  SendGuildTributeFavorAndTimer(guild);Refresh(guild);return true;
 }catch(...){if(transaction)m_db->QueryDatabase(std::string("ROLLBACK"),false);m_guild_trophy_deadlines.erase(guild);Refresh(guild);return false;}
}
void WorldGuildManager::GuildTrophyRequest(uint32 actor,uint32 account,uint32 action,uint32 target){
 if(!m_db||action>2||!RuleB(Housing,EnablePlayerHousing))return;try{Housing::GuildTrophies::Q(*m_db,"START TRANSACTION");auto member=Housing::GuildTrophies::MemberUnderTransaction(*m_db,{actor,account});Housing::GuildTrophies::Q(*m_db,"ROLLBACK");UpdateGuildTrophies(member.guild,action,actor,account);
 if(target){auto rows=Housing::GuildTrophies::Q(*m_db,"SELECT c.name,m.tribute_enable,m.trophy_enable,m.total_tribute FROM character_data c JOIN guild_members m ON m.char_id=c.id WHERE c.deleted_at IS NULL AND c.id="+std::to_string(target)+" AND m.guild_id="+std::to_string(member.guild));if(rows.RowCount()==1){auto r=rows.begin();ServerPacket packet(ServerOP_HousingGuildTrophyMember,sizeof(GuildTributeOptInOutReply_Struct));auto* out=reinterpret_cast<GuildTributeOptInOutReply_Struct*>(packet.pBuffer);out->guild_id=member.guild;if(r[0])strn0cpy(out->player_name,r[0],sizeof out->player_name);out->tribute_toggle=Housing::GuildTrophies::Number(r[1]);out->tribute_trophy_toggle=Housing::GuildTrophies::Number(r[2]);out->no_donations=Housing::GuildTrophies::Number(r[3]);out->time=uint32(std::time(nullptr));auto* online=ClientList::Instance()->FindCLEByCharacterID(target);out->command=online&&online->GuildID()==member.guild&&online->Online()==CLE_Status::InZone&&online->Server()?1:0;ZSList::Instance()->SendPacketToBootedZones(&packet);}}
}catch(...){m_db->QueryDatabase(std::string("ROLLBACK"),false);}
}
void WorldGuildManager::ProcessGuildTrophies(){
 if(!m_db||!RuleB(Housing,EnablePlayerHousing))return;
 if(!m_guild_trophy_initialized){m_guild_trophy_initialized=true;try{auto tables=Housing::GuildTrophies::Q(*m_db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_guild_trophy_state'");if(tables.RowCount()!=1||Housing::GuildTrophies::Number(tables.begin()[0])!=1)return;auto rows=Housing::GuildTrophies::Q(*m_db,"SELECT guild_id FROM housing_guild_trophy_state");std::vector<uint32> ids;for(auto row=rows.begin();row!=rows.end();++row)ids.push_back(Housing::GuildTrophies::Number(row[0]));for(auto id:ids)UpdateGuildTrophies(id,0);}catch(...){return;}}
 std::set<uint32> work;const auto now=std::chrono::steady_clock::now();for(const auto& [guild,deadline]:m_guild_trophy_deadlines)if(deadline<=now)work.insert(guild);
 if(m_guild_trophy_refresh_all){m_guild_trophy_refresh_all=false;for(auto guild:m_guild_trophy_known)work.insert(guild);}
 for(auto guild:work)UpdateGuildTrophies(guild,m_guild_trophy_deadlines.count(guild)&&m_guild_trophy_deadlines.at(guild)<=now?3:0);
}
