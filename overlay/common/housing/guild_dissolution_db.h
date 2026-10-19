#ifndef EQEMU_HOUSING_GUILD_DISSOLUTION_DB_H
#define EQEMU_HOUSING_GUILD_DISSOLUTION_DB_H
#include "../database.h"
#include "return_settlement_db.h"
#include <mysqld_error.h>
#include <string>
#include <vector>
namespace Housing::GuildDissolution {
enum class Status { Committed, HousingAssets, BankItems, Unavailable, CommitUncertain };
namespace Detail {
struct Stop { Status status; };
inline MySQLRequestResult Q(Database& db,const std::string& sql) {
 auto r=db.QueryDatabase(sql,false);if(!r.Success())throw Stop{Status::Unavailable};return r;
}
inline void Empty(Database& db,const std::string& sql,Status reason=Status::HousingAssets) {
 if(Q(db,sql+" LIMIT 1 FOR UPDATE").RowCount())throw Stop{reason};
}
inline void Engines(Database& db,const std::vector<std::string>& tables) {
 for(const auto& t:tables){auto r=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='"+t+"' AND table_type='BASE TABLE'");if(r.RowCount()!=1||!r.begin()[0]||std::string(r.begin()[0])!="InnoDB")throw Stop{Status::Unavailable};}
}
inline void HousingClear(Database& db,uint32_t guild) {
 auto installed=Q(db,"SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() AND LEFT(table_name,8)='housing_'");
 if(!installed.RowCount()) {
  // Metadata visibility or SELECT errors cannot masquerade as no installation.
  auto probe=db.QueryDatabase(std::string("SELECT id FROM housing_estates WHERE 1=0"),false);
  if(!probe.Success()&&probe.ErrorNumber()==ER_NO_SUCH_TABLE)return;
  throw Stop{Status::Unavailable};
 }
 // This guard intentionally requires the full reviewed custody surface. Partial
 // installations are not evidence that absent custody/pending state is empty.
 Engines(db,{"housing_estates","housing_escrow_balances","housing_escrow_transfers","housing_native_estates","housing_live_item_custody","housing_fixture_item_custody","housing_live_item_storage","housing_live_placement_reservations","housing_crate_estates","housing_live_vault_items","housing_live_item_leases","housing_live_escrow_pending","housing_native_plot_purchase_pending","housing_house_inventory_pending","housing_house_instances"});
 const auto owners="SELECT id FROM housing_estates WHERE owner_kind=3 AND owner_id="+std::to_string(guild);
 auto history=Q(db,"SELECT id,active FROM housing_estates WHERE owner_kind=3 AND owner_id="+std::to_string(guild)+" FOR UPDATE");
 std::string settled;
 for(auto r=history.begin();r!=history.end();++r){
  if(!r[0]||!r[1]||std::string(r[1])!="0")throw Stop{Status::HousingAssets};
  const auto estate=std::stoull(r[0]);
  if(estate>UINT32_MAX)throw Stop{Status::Unavailable};
  if(ReturnSettlement::HasProofUnderTransaction(db,uint32_t(estate))){if(!ReturnSettlement::VerifyGuildUnderTransaction(db,uint32_t(estate),guild))throw Stop{Status::HousingAssets};if(!settled.empty())settled+=",";settled+=std::to_string(estate);}
 }
 // Only an exact canonical conservation seal can exempt retained historical
 // custody/bindings. Missing, partial or corrupt proof preserves the old guard.
 const auto unsettled=owners+(settled.empty()?"":" AND id NOT IN ("+settled+")");
 // Probe every required column even when this guild has no ownership history.
 Empty(db,"SELECT estate_id FROM housing_escrow_balances WHERE estate_id IN ("+owners+") AND (copper IS NULL OR copper<>0)");
 Empty(db,"SELECT estate_id FROM housing_escrow_transfers WHERE estate_id IN ("+owners+") AND (completed IS NULL OR completed<>1)");
 Empty(db,"SELECT native_id FROM housing_native_estates WHERE ownership_estate_id IN ("+owners+")");
 Empty(db,"SELECT custody_id FROM housing_live_item_custody WHERE estate_id IN ("+unsettled+")");
 Empty(db,"SELECT instance_key FROM housing_fixture_item_custody WHERE estate_id IN ("+owners+")");
 Empty(db,"SELECT native_estate_id FROM housing_live_item_storage WHERE native_estate_id IN (SELECT native_id FROM housing_native_estates WHERE ownership_estate_id IN ("+owners+"))");
 Empty(db,"SELECT native_item_id FROM housing_live_placement_reservations WHERE estate_id IN ("+owners+") AND (state IS NULL OR state=0)");
 Empty(db,"SELECT native_estate_id FROM housing_crate_estates WHERE ownership_estate_id IN ("+unsettled+")");
 Empty(db,"SELECT custody_id FROM housing_live_vault_items WHERE realm_estate_id IN ("+owners+")");
 // Opaque pending bindings cannot prove their absence from an old generation.
 // For guilds with history, any unresolved housing application conservatively
 // defers disband. Completed immutable receipts do not block it.
 const std::string predicate=history.RowCount()?"":" WHERE 1=0";
 Empty(db,"SELECT character_id FROM housing_live_item_leases WHERE pending_key IS NOT NULL"+std::string(history.RowCount()?"":" AND 1=0"));
 Empty(db,"SELECT character_id FROM housing_live_escrow_pending"+predicate);
 Empty(db,"SELECT character_id FROM housing_native_plot_purchase_pending"+predicate);
 Empty(db,"SELECT character_id FROM housing_house_inventory_pending"+predicate);
 Empty(db,"SELECT house_id FROM housing_house_instances WHERE pending_request_key IS NOT NULL"+std::string(history.RowCount()?"":" AND 1=0"));
}
}
// Owns exactly one transaction. No cache changes or notification are performed.
// Guild acquisition locks the same guild row; after deletion it fails authority.
// A failed/uncertain commit must leave the caller's cache and broadcasts intact.
inline Status Delete(Database& db,uint32_t guild) noexcept {
 using namespace Detail;bool active=false,committing=false;
 try {
  if(!guild||guild==UINT32_MAX)return Status::Unavailable;
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="0")return Status::Unavailable;
  Engines(db,{"guilds","guild_bank","guild_tributes","guild_ranks","guild_permissions","guild_members"});
  Q(db,"START TRANSACTION");active=true;
  auto identity=Q(db,"SELECT id FROM guilds WHERE id="+std::to_string(guild)+" FOR UPDATE");if(identity.RowCount()!=1)throw Stop{Status::Unavailable};
  const auto where=" WHERE guild_id="+std::to_string(guild);
  Empty(db,"SELECT guild_id FROM guild_bank"+where,Status::BankItems);
  HousingClear(db,guild);
  // Every mutation is checked. Rollback restores all authority on SQL failure.
  for(const auto* table:{"guild_tributes","guild_ranks","guild_permissions","guild_members"})Q(db,"DELETE FROM "+std::string(table)+where);
  auto removed=Q(db,"DELETE FROM guilds WHERE id="+std::to_string(guild));if(removed.RowsAffected()!=1)throw Stop{Status::Unavailable};
  committing=true;auto result=db.QueryDatabase(std::string("COMMIT"),false);active=false;
  return result.Success()?Status::Committed:Status::CommitUncertain;
 }catch(const Stop& e){if(active)try{db.QueryDatabase(std::string("ROLLBACK"),false);}catch(...){}return committing?Status::CommitUncertain:e.status;}
 catch(...){if(active)try{db.QueryDatabase(std::string("ROLLBACK"),false);}catch(...){}return committing?Status::CommitUncertain:Status::Unavailable;}
}
}
#endif
