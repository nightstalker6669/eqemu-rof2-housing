#ifndef EQEMU_HOUSING_GUILD_TROPHY_BILLING_DB_H
#define EQEMU_HOUSING_GUILD_TROPHY_BILLING_DB_H
#include "guild_trophies_db.h"
#include "current_estate_access.h"
#include "house_inventory_transition.h"
namespace Housing::GuildTrophies {
// Zone authentication locks identity, lease and pending-house rows before the
// guild row. Billing must acquire that same prefix before shared guild state:
// guild -> source-house conflicts with zone pending-house -> guild otherwise.
// Lock characters and accounts in separate sorted phases: two online alts may
// share an account, so interleaving character/account pairs can itself deadlock.
inline void LockBillingInputsUnderTransaction(Database& db,
 const std::map<uint32_t,std::pair<uint32_t,uint32_t>>& online_levels,
 const std::optional<AuthenticatedPurchaser>& actor){
 std::map<uint32_t,uint32_t> identities;
 for(const auto& [character,audience]:online_levels){
  if(!character||!audience.first||!audience.second||audience.second>255)throw Failure{Error::InvalidArgument};
  identities.emplace(character,audience.first);
 }
 if(actor){
  if(!actor->character_id||!actor->account_id)throw Failure{Error::InvalidArgument};
  auto [it,inserted]=identities.emplace(actor->character_id,actor->account_id);
  if(!inserted&&it->second!=actor->account_id)throw Failure{Error::Unauthorized};
 }
 std::set<uint32_t> accounts;
 for(const auto& [character,account]:identities){
  Q(db,"SELECT id FROM character_data WHERE id="+std::to_string(character)+" FOR UPDATE");
  accounts.insert(account);
 }
 for(auto account:accounts)Q(db,"SELECT id FROM account WHERE id="+std::to_string(account)+" FOR UPDATE");
 for(const auto& [character,account]:identities)
  Q(db,"SELECT character_id FROM housing_live_item_leases WHERE character_id="+std::to_string(character)+" FOR UPDATE");
 for(const auto& [character,account]:identities){
  auto pending=HouseInventory::InspectPending(db,character,true);
  if(pending!=HouseInventory::PendingState::Clear)
   throw Failure{pending==HouseInventory::PendingState::Pending?Error::PendingApplication:Error::DatabaseFailure};
 }
 // Even an empty online audience must take the source-house lock before guild
 // state; the zone's pending proof currently scans this same primary index.
 Q(db,"SELECT house_id FROM housing_house_instances ORDER BY house_id FOR UPDATE");
}
struct BillingView {State state;uint32_t favor=0,remaining_ms=0,charged=0;bool insufficient=false,billing_unavailable=false,changed=false;};
// Invoked only by the existing WorldGuildManager. Online levels are taken from
// genuine InZone ClientList entries for this guild, never a client packet/count.
// DB membership/opt-in is independently rechecked under this same transaction.
inline LiveItems::Result<BillingView> BillUnderTransaction(Database& db,uint32_t guild,TrophyBenefits::BillAction action,const std::map<uint32_t,std::pair<uint32_t,uint32_t>>& online_levels,const std::optional<AuthenticatedPurchaser>& actor={}){try{
 auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||Number(tx.begin()[0])!=1||!guild||guild==UINT32_MAX)return {{},Error::InvalidArgument};
 auto engine=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('guilds','guild_members','guild_permissions','housing_guild_trophy_state','housing_guild_trophy_selections','housing_guild_trophy_billing_policy','housing_guild_trophy_favor_charges')");if(engine.RowCount()!=1||Number(engine.begin()[0])!=7)return {{},Error::DatabaseFailure};
 LockBillingInputsUnderTransaction(db,online_levels,actor);
 auto owner=Q(db,"SELECT favor FROM guilds WHERE id="+std::to_string(guild)+" FOR UPDATE");if(owner.RowCount()!=1)return {{},Error::Unauthorized};uint32_t before_favor=Number(owner.begin()[0]);
 if(action==TrophyBenefits::BillAction::Enable||action==TrophyBenefits::BillAction::Disable){if(!actor)return {{},Error::Unauthorized};auto member=MemberUnderTransaction(db,*actor);if(member.guild!=guild||!PermissionUnderTransaction(db,member,25))return {{},Error::Unauthorized};}
 auto definitions=DefinitionsUnderTransaction(db);BillingView out;out.favor=before_favor;out.state=StateUnderTransaction(db,guild);const auto before=out.state;bool unreviewed=false;bool dirty=PruneUnderTransaction(db,guild,out.state,definitions,&unreviewed);if(unreviewed&&action!=TrophyBenefits::BillAction::Disable&&action!=TrophyBenefits::BillAction::Invalidate)return {{},Error::UnsupportedItem};bool any=false,paid=false;
 for(size_t i=0;i<4;++i)if(out.state.bank.ids[i]!=UINT32_MAX){any=true;if(definitions.at(out.state.bank.ids[i]).tiers[out.state.bank.tiers[i]].cost)paid=true;}
 if(!any){out.state.enabled=false;out.state.paid={};}
 else if(!paid){out.state.paid={};if(action==TrophyBenefits::BillAction::Enable)out.state.enabled=true;if(action==TrophyBenefits::BillAction::Disable||action==TrophyBenefits::BillAction::Invalidate)out.state.enabled=false;}
 else{
  auto policies=Q(db,"SELECT period_ms,policy,source_sha256,source_description FROM housing_guild_trophy_billing_policy WHERE singleton=1 FOR UPDATE");
  TrophyBenefits::BillingPolicy policy;
  if(policies.RowCount()==1&&policies.begin()[3]&&policies.begin()[3][0])policy={Number(policies.begin()[0]),policies.begin()[2]?policies.begin()[2]:"",Number(policies.begin()[1])};
  if(!TrophyBenefits::Valid(policy)){out.state.enabled=false;out.state.paid={};out.billing_unavailable=true;}
  else{
   uint64_t cost=0;for(const auto& [character,audience]:online_levels){const auto account=audience.first,level=audience.second;if(!character||!account||!level||level>255)return {{},Error::InvalidArgument};auto member=Q(db,"SELECT m.trophy_enable,c.deleted_at,c.account_id FROM guild_members m JOIN character_data c ON c.id=m.char_id WHERE m.guild_id="+std::to_string(guild)+" AND m.char_id="+std::to_string(character)+" FOR UPDATE");if(member.RowCount()>1)return {{},Error::DatabaseFailure};if(member.RowCount()!=1||member.begin()[1]||Number(member.begin()[0])!=1||Number(member.begin()[2])!=account)continue;for(size_t i=0;i<4;++i)if(out.state.bank.ids[i]!=UINT32_MAX){const auto& tier=definitions.at(out.state.bank.ids[i]).tiers[out.state.bank.tiers[i]];if(level>=tier.minimum_level){const auto& source=*out.state.sources[i];auto plot=Q(db,"SELECT native_id FROM housing_native_estates WHERE ownership_estate_id="+std::to_string(source.estate)+" AND active=1 FOR UPDATE");if(plot.RowCount()!=1)return {{},Error::Conflict};auto access=LoadGuildTrophyRecipientAccessUnderTransaction(db,source.estate,Number(plot.begin()[0]),{character,account});if(!access){if(access.error==Housing::Error::DatabaseFailure)return {{},Error::DatabaseFailure};continue;}if(access.value.effective.Allows(AccessPolicy::Capability::Enter))cost+=tier.cost;}if(cost>UINT32_MAX)return {{},Error::UnsupportedItem};}}
   const auto selection=SelectionHash(out.state,definitions,policy);
   auto clock=Q(db,"SELECT CAST(UNIX_TIMESTAMP(CURRENT_TIMESTAMP(3))*1000 AS UNSIGNED)");auto now=std::stoull(clock.begin()[0]);auto state=out.state.paid;state.enabled=out.state.enabled;
   auto plan=TrophyBenefits::Bill(policy,state,action,now,uint32_t(cost),before_favor,selection);if(!plan)return {{},Error::UnsupportedItem};out.state.paid=plan->state;out.state.enabled=plan->state.enabled;out.remaining_ms=plan->remaining_ms;out.charged=plan->charge;out.insufficient=plan->insufficient;
   if(plan->favor_after){auto changed=GuildFavor::Adjust(db,guild,-int64_t(plan->charge));if(!changed||*changed!=*plan->favor_after)return {{},Error::Conflict};out.favor=*changed;Q(db,"INSERT INTO housing_guild_trophy_favor_charges(guild_id,state_revision,favor_before,favor_after,cost,period_ms,paid_until_ms,selection_hash) VALUES ("+std::to_string(guild)+","+std::to_string(before.revision+1)+","+std::to_string(before_favor)+","+std::to_string(out.favor)+","+std::to_string(out.charged)+","+std::to_string(policy.period_ms)+","+std::to_string(out.state.paid.until_ms)+",'"+selection+"')");}
  }
 }
 dirty=dirty||before.enabled!=out.state.enabled||before.paid.remaining_ms!=out.state.paid.remaining_ms||before.paid.until_ms!=out.state.paid.until_ms||before.paid.selection_hash!=out.state.paid.selection_hash;
 if(dirty){SaveStateUnderTransaction(db,guild,out.state);out.state.revision++;out.changed=true;}return {std::move(out),Error::None};
 }catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}}
}
#endif
