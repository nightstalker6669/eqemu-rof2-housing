#include "housing_service_tribute.h"
#include "client.h"
#include "../common/housing/guild_favor.h"
#include <array>
#include <stdexcept>
namespace Housing::ServiceTribute {
namespace {
uint64_t Number(const char* p){if(!p||!*p)throw std::runtime_error("Missing donation number");size_t n=0;std::string s(p);if(s[0]<'0'||s[0]>'9')throw std::runtime_error("Invalid donation number");auto x=std::stoull(s,&n);if(n!=s.size())throw std::runtime_error("Invalid donation number");return x;}
MySQLRequestResult Q(const std::string&s){auto r=database.QueryDatabase(s,false);if(!r.Success())throw std::runtime_error("Donation query failed");return r;}
class Transaction {
 Client& c;bool active=false;uint64_t connection=0;
public:
 explicit Transaction(Client& client):c(client){
  auto mode=Q("SELECT CONNECTION_ID(),@@in_transaction");if(mode.RowCount()!=1||Number(mode.begin()[1]))throw std::runtime_error("Nested donation transaction");connection=Number(mode.begin()[0]);
  auto engines=Q("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('inventory','character_currency','guilds','guild_members') AND engine='InnoDB'");if(engines.RowCount()!=1||Number(engines.begin()[0])!=4)throw std::runtime_error("Donation transaction schema unavailable");
  Q("START TRANSACTION");active=true;
 }
 void Commit(){auto mode=Q("SELECT CONNECTION_ID(),@@in_transaction");if(mode.RowCount()!=1||Number(mode.begin()[0])!=connection||Number(mode.begin()[1])!=1)throw std::runtime_error("Donation connection changed");auto r=database.QueryDatabase(std::string("COMMIT"),false);active=false;if(!r.Success()){c.RejectInventorySessionAdmission();c.Message(Chat::Red,"The donation result could not be confirmed. Please reconnect before continuing.");c.Kick("Guild tribute donation commit could not be confirmed");throw std::runtime_error("Unknown donation commit");}}
 ~Transaction(){if(active){auto r=database.QueryDatabase(std::string("ROLLBACK"),false);if(!r.Success()){c.RejectInventorySessionAdmission();c.Kick("Guild tribute rollback could not be confirmed");}}}
};
Receipt Credit(Client&c,uint32_t favor){
 auto guild=GuildFavor::Adjust(database,c.GuildID(),favor);if(!guild)throw std::runtime_error("Guild donation favor rejected");
 auto member=Q("UPDATE guild_members SET total_tribute=LAST_INSERT_ID(total_tribute+"+std::to_string(favor)+"),last_tribute=UNIX_TIMESTAMP() WHERE char_id="+std::to_string(c.CharacterID())+" AND guild_id="+std::to_string(c.GuildID())+" AND total_tribute<="+std::to_string(uint64_t(UINT32_MAX)-favor));
 if(member.RowsAffected()!=1||member.LastInsertedID()>UINT32_MAX)throw std::runtime_error("Guild donation membership rejected");return {favor,*guild,uint32_t(member.LastInsertedID())};
}
bool Carried(uint16_t slot){return (slot>=EQ::invslot::GENERAL_BEGIN&&slot<=EQ::invslot::GENERAL_END)||(slot>=EQ::invbag::GENERAL_BAGS_BEGIN&&slot<=EQ::invbag::GENERAL_BAGS_END);}
}
std::optional<Receipt> DonateItem(Client&c,uint16_t slot,uint32_t quantity){
 try{
  if(!c.IsInAGuild()||!Carried(slot)||!quantity||quantity>INT16_MAX)return {};
  auto* item=c.GetInv().GetItem(slot);if(!item||!item->GetItemGuildFavor()||item->GetHousingHouseID()||item->GetHousingCrateID()||item->GetEvolveUniqueID()||item->IsNoneEmptyContainer())return {};
  const auto charges=item->GetCharges();const uint32_t stored_charges=charges<0?uint32_t(INT16_MAX):uint32_t(charges);if((item->IsStackable()&&(charges<1||quantity>uint32_t(charges)))||(!item->IsStackable()&&quantity!=1))return {};
  const uint64_t favor=uint64_t(item->GetItemGuildFavor())*quantity;if(!favor||favor>UINT32_MAX)return {};
  Transaction tx(c);
  auto source=Q("SELECT item_id,charges FROM inventory WHERE character_id="+std::to_string(c.CharacterID())+" AND slot_id="+std::to_string(slot)+" FOR UPDATE");
  if(source.RowCount()!=1||Number(source.begin()[0])!=item->GetID()||!source.begin()[1]||Number(source.begin()[1])!=stored_charges)return {};
  const std::string where=" WHERE character_id="+std::to_string(c.CharacterID())+" AND slot_id="+std::to_string(slot)+" AND item_id="+std::to_string(item->GetID())+" AND charges="+std::to_string(stored_charges);
  auto debit=Q(item->IsStackable()&&quantity<uint32_t(charges)?"UPDATE inventory SET charges=charges-"+std::to_string(quantity)+where:"DELETE FROM inventory"+where);
  if(debit.RowsAffected()!=1)return {};
  auto receipt=Credit(c,uint32_t(favor));tx.Commit();
  c.DeleteItemInInventory(slot,item->IsStackable()?int16_t(quantity):0,false,false);
  return receipt;
 }catch(...){return {};}
}
std::optional<Receipt> DonatePlatinum(Client&c,uint32_t quantity,uint32_t conversion){
 try{
  const uint64_t favor=uint64_t(quantity)*conversion;if(!c.IsInAGuild()||!quantity||!conversion||!favor||favor>UINT32_MAX||quantity>c.GetCarriedPlatinum())return {};
  auto&pp=c.GetPP();const uint64_t total=uint64_t(pp.platinum)*1000+uint64_t(pp.gold)*100+uint64_t(pp.silver)*10+pp.copper;const uint64_t cost=uint64_t(quantity)*1000;if(cost>total)return {};
  const uint64_t remaining=total-cost;if(remaining/1000>UINT32_MAX)return {};
  std::array<uint32_t,4> coins{uint32_t(remaining/1000),uint32_t(remaining%1000/100),uint32_t(remaining%100/10),uint32_t(remaining%10)};
  Transaction tx(c);
  auto row=Q("SELECT platinum,gold,silver,copper FROM character_currency WHERE id="+std::to_string(c.CharacterID())+" FOR UPDATE");if(row.RowCount()!=1||Number(row.begin()[0])!=pp.platinum||Number(row.begin()[1])!=pp.gold||Number(row.begin()[2])!=pp.silver||Number(row.begin()[3])!=pp.copper)return {};
  auto debit=Q("UPDATE character_currency SET platinum="+std::to_string(coins[0])+",gold="+std::to_string(coins[1])+",silver="+std::to_string(coins[2])+",copper="+std::to_string(coins[3])+" WHERE id="+std::to_string(c.CharacterID()));if(debit.RowsAffected()!=1)return {};
  auto receipt=Credit(c,uint32_t(favor));tx.Commit();
  pp.platinum=coins[0];pp.gold=coins[1];pp.silver=coins[2];pp.copper=coins[3];c.RecalcWeight();c.SendMoneyUpdate();return receipt;
 }catch(...){return {};}
}
}
