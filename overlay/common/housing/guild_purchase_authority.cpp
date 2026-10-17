#include "guild_purchase_authority.h"
#include "../database.h"
#include "../guilds.h"
#include <stdexcept>
namespace Housing::GuildPurchaseAuthority {
static_assert(GUILD_ACTION_REAL_ESTATE_GUILD_PLOT_BUY==GuildPlotBuyAction);
namespace {
struct Failure{Error error;};
MySQLRequestResult Query(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw Failure{Error::DatabaseFailure};return r;}
uint32_t Number(const char* text){if(!text)throw Failure{Error::CorruptData};auto n=std::stoull(text);if(n>UINT32_MAX)throw Failure{Error::CorruptData};return uint32_t(n);}
std::array<uint8_t,32> Token(const char* text){if(!text||std::string(text).size()!=64)throw Failure{Error::CorruptData};std::array<uint8_t,32> out;auto nibble=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw Failure{Error::CorruptData};};for(size_t i=0;i<32;++i)out[i]=(nibble(text[2*i])<<4)|nibble(text[2*i+1]);return out;}
bool Key(const std::string& s){if(s.empty()||s.size()>96)return false;for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==':'||c=='_'||c=='-'||c=='.'))return false;return true;}
}
Result<Authority> CheckUnderTransaction(Database& db,const LiveItems::Lease& lease,LiveItems::InventorySessionFence& fence,bool enabled){
 if(!enabled)return {{},Error::Disabled};
 if(!lease.actor.character_id||!lease.actor.account_id||!lease.epoch||!Key(lease.session_key))return {{},Error::InvalidArgument};
 if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))return {{},Error::UnheldFence};
 if(db.GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
 try{
  auto transaction=Query(db,"SELECT @@in_transaction");if(transaction.RowCount()!=1||Number(transaction.begin()[0])!=1)throw Failure{Error::TransactionRequired};
  auto engines=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('character_data','account','housing_live_item_leases','guild_members','guilds','guild_permissions')");if(engines.RowCount()!=1||Number(engines.begin()[0])!=6)throw Failure{Error::DatabaseFailure};
  const auto actor=std::to_string(lease.actor.character_id);
  auto character=Query(db,"SELECT account_id,deleted_at FROM character_data WHERE id="+actor+" FOR UPDATE");if(character.RowCount()!=1||character.begin()[1]||Number(character.begin()[0])!=lease.actor.account_id)throw Failure{Error::Unauthorized};
  auto account=Query(db,"SELECT id FROM account WHERE id="+std::to_string(lease.actor.account_id)+" FOR UPDATE");if(account.RowCount()!=1)throw Failure{Error::Unauthorized};
  auto current=Query(db,"SELECT account_id,epoch,session_key,HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id="+actor+" FOR UPDATE");if(current.RowCount()!=1)throw Failure{Error::StaleLease};auto l=current.begin();if(Number(l[0])!=lease.actor.account_id||std::stoull(l[1])!=lease.epoch||std::string(l[2])!=lease.session_key||Token(l[3])!=lease.token||Number(l[4])!=1)throw Failure{Error::StaleLease};if(l[5])throw Failure{Error::PendingInventory};
  auto membership=Query(db,"SELECT guild_id,`rank` FROM guild_members WHERE char_id="+actor+" FOR UPDATE");if(membership.RowCount()!=1)throw Failure{Error::Unauthorized};auto guild_id=Number(membership.begin()[0]),rank=Number(membership.begin()[1]);if(!guild_id||guild_id==GUILD_NONE||rank<1||rank>8)throw Failure{Error::Unauthorized};
  auto guild=Query(db,"SELECT id FROM guilds WHERE id="+std::to_string(guild_id)+" FOR UPDATE");if(guild.RowCount()!=1)throw Failure{Error::Unauthorized};
  auto permission=Query(db,"SELECT permission FROM guild_permissions WHERE guild_id="+std::to_string(guild_id)+" AND perm_id=27 FOR UPDATE");if(!permission.RowCount())throw Failure{Error::Unauthorized};if(permission.RowCount()!=1)throw Failure{Error::CorruptData};auto mask=Number(permission.begin()[0]);if(mask>255)throw Failure{Error::CorruptData};if(!RankGranted(mask,rank))throw Failure{Error::Unauthorized};
  return {{lease.actor,guild_id,uint8_t(rank),uint8_t(mask),OwnerKind::Guild},Error::None};
 }catch(const Failure& failure){return {{},failure.error};}catch(...){return {{},Error::CorruptData};}
}
}
