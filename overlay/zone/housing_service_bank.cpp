#include "housing_service_bank.h"
#include "client.h"
#include "guild_mgr.h"
#include <map>
#include <stdexcept>
namespace Housing::ServiceBank {
namespace {
struct Publication {uint32_t character=0,guild=0;std::string rows;};
std::map<const Client*,Publication> published;
Request* current_request=nullptr;
MySQLRequestResult Q(const std::string&s){auto r=database.QueryDatabase(s,false);if(!r.Success())throw std::runtime_error("Guild bank query failed");return r;}
uint64_t Connection(){auto r=Q("SELECT CONNECTION_ID()");if(r.RowCount()!=1||!r.begin()[0])throw std::runtime_error("Guild bank connection missing");return std::stoull(r.begin()[0]);}
std::string Rows(uint32_t guild){
 auto r=Q("SELECT * FROM guild_bank WHERE guild_id="+std::to_string(guild)+" ORDER BY area,slot,id");
 if(r.RowCount()>240||r.ColumnCount()<15||r.ColumnCount()>32)throw std::runtime_error("Invalid guild bank shape");std::string s;
 for(auto row=r.begin();row!=r.end();++row)for(uint32_t i=0;i<r.ColumnCount();++i){if(!row[i])s+="N;";else{std::string value(row[i]);if(value.size()>4096)throw std::runtime_error("Invalid guild bank value");s+=std::to_string(value.size())+":"+value+";";}}
 return s;
}
}
Request::Request(Client&c,bool publish_only)noexcept:client_(c){
 try{
  if(current_request||!c.Connected()||!c.ClientDataLoaded()||!c.IsInAGuild())return;guild_=c.GuildID();name_="eqemu.guildbank."+std::to_string(guild_);
  auto lock=Q("SELECT GET_LOCK('"+name_+"',0)");if(lock.RowCount()!=1||!lock.begin()[0]||std::string(lock.begin()[0])!="1"){c.Message(Chat::Yellow,"The guild bank is busy. Please try again.");return;}locked_=true;connection_=Connection();
  auto current=Rows(guild_);auto prior=published.find(&c);const bool same=prior!=published.end()&&prior->second.character==c.CharacterID()&&prior->second.guild==guild_&&prior->second.rows==current;
  // A newly loaded manager must match the actual DB snapshot even when this
  // client's publication is current. Existing managers may cache another
  // guild or a different requester's older slot numbering.
  delete GuildBanks;GuildBanks=new GuildBankManager;
  GuildBanks->SendGuildBank(&c);
  if(!GuildBanks->GetGuildBank(guild_))throw std::runtime_error("Guild bank cache load failed");
  if(Connection()!=connection_||Rows(guild_)!=current)throw std::runtime_error("Guild bank changed while loading");
  if(published.size()>4096)published.clear();published[&c]={c.CharacterID(),guild_,current};published_=true;ready_=same&&!publish_only;
  if(ready_){auto engines=Q("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('guild_bank','inventory') AND engine='InnoDB'");if(engines.RowCount()!=1||!engines.begin()[0]||std::string(engines.begin()[0])!="2")throw std::runtime_error("Guild bank transactional schema unavailable");auto state=Q("SELECT @@in_transaction");if(state.RowCount()!=1||!state.begin()[0]||std::string(state.begin()[0])!="0"||current_request)throw std::runtime_error("Nested guild bank transaction");Q("START TRANSACTION");transaction_=true;current_request=this;}
  if(!same&&!publish_only)c.Message(Chat::Yellow,"The guild bank changed. Its contents have been refreshed; please try that action again.");
 }catch(...){ready_=false;c.Message(Chat::Red,"The guild bank is temporarily unavailable.");}
}
Request::~Request(){
 if(!locked_)return;
 try{
  if(Connection()!=connection_)throw std::runtime_error("Guild bank connection changed");
  if(transaction_){
   // These are the only character-owned rows mutated by guild-bank transfers.
   // Verify SaveCursor's durable head instead of trusting its ignored result.
   auto* head=client_.GetInv().GetItem(EQ::invslot::slotCursor);
   auto saved=Q("SELECT item_id,charges,augment_one,augment_two,augment_three,augment_four,augment_five,augment_six FROM inventory WHERE character_id="+std::to_string(client_.CharacterID())+" AND slot_id="+std::to_string(EQ::invslot::slotCursor));
   if(head){if(saved.RowCount()!=1||!saved.begin()[0]||!saved.begin()[1]||std::stoul(saved.begin()[0])!=head->GetID()||std::stoul(saved.begin()[1])!=uint32_t(head->GetCharges()<0?INT16_MAX:head->GetCharges()))failed_=true;else for(uint32_t i=0;i<6;++i)if(!saved.begin()[i+2]||std::stoul(saved.begin()[i+2])!=head->GetAugmentItemID(i))failed_=true;}
   else if(saved.RowCount())failed_=true;
   if(failed_){Q("ROLLBACK");transaction_=false;client_.RejectInventorySessionAdmission();client_.Message(Chat::Red,"The guild bank transfer was not completed. Please reconnect before continuing.");client_.Kick("Guild bank transaction rejected");published.erase(&client_);published_=false;}
   else {Q("COMMIT");transaction_=false;}
  }
  if(published_)published[&client_]={client_.CharacterID(),guild_,Rows(guild_)};
  Q("SELECT RELEASE_LOCK('"+name_+"')");
 }catch(...){
  if(transaction_)database.QueryDatabase(std::string("ROLLBACK"),false);
  client_.RejectInventorySessionAdmission();client_.Kick("Guild bank transaction could not be confirmed");published.erase(&client_);
  database.QueryDatabase("SELECT RELEASE_LOCK('"+name_+"')",false);
 }
 if(current_request==this)current_request=nullptr;
}
void AbortCurrent()noexcept{if(current_request)current_request->Abort();}

bool Publish(Client&c)noexcept{Request request(c,true);return request.Published();}
}
