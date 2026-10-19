#ifndef EQEMU_HOUSING_UPKEEP_CATCHUP_DB_H
#define EQEMU_HOUSING_UPKEEP_CATCHUP_DB_H
#include "../database.h"
#include <openssl/sha.h>
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>
namespace Housing::UpkeepCatchup {
// This helper never opens/commits a transaction, changes a due date, or certifies
// an operator-supplied past snapshot. The billing caller owns every assessment.
using Digest=std::array<uint8_t,32>;
inline Digest Hash(const std::vector<uint8_t>& bytes){Digest out;SHA256(bytes.data(),bytes.size(),out.data());return out;}
inline std::string Hex(const Digest& d){const char* h="0123456789ABCDEF";std::string out;for(auto b:d){out+=h[b>>4];out+=h[b&15];}return out;}
inline void Word(std::vector<uint8_t>& b,uint64_t n){for(unsigned i=0;i<8;++i)b.push_back(uint8_t(n>>(8*i)));}
inline void Blob(std::vector<uint8_t>& b,const std::vector<uint8_t>& v){Word(b,v.size());b.insert(b.end(),v.begin(),v.end());}
inline MySQLRequestResult Q(Database& db,const std::string& s){auto r=db.QueryDatabase(s,false);if(!r.Success())throw std::runtime_error("Catch-up query failed");return r;}
inline uint64_t N(const char* s){if(!s||!*s||*s=='-')throw std::invalid_argument("Catch-up number missing");std::string v=s;size_t at=0;auto n=std::stoull(v,&at);if(at!=v.size())throw std::invalid_argument("Catch-up number invalid");return n;}
inline std::string Quote(Database& db,const std::string& s){return "'"+db.Escape(s)+"'";}
struct Policy {uint32_t max_hours=0;Digest identity{};};
struct Checkpoint {uint32_t next_due=0,anchor_due=0,observed_at=0;uint64_t contract_revision=0,balance=0;Digest obligations{};};
inline std::optional<Policy> Reviewed(Database& db,uint32_t estate,uint32_t native,uint32_t kind,uint32_t owner,uint64_t revision,const std::vector<uint8_t>& definitions,const std::vector<uint8_t>& regions){
 auto table=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_upkeep_catchup_policy'");if(!table.RowCount())return {};if(table.RowCount()!=1||!table.begin()[0]||std::string(table.begin()[0])!="InnoDB")throw std::invalid_argument("Catch-up policy engine");
 auto r=Q(db,"SELECT native_plot_id,owner_kind,owner_id,catalog_revision,UPPER(definitions_sha256),UPPER(regions_sha256),max_overdue_hours,review_reference FROM housing_upkeep_catchup_policy WHERE estate_id="+std::to_string(estate)+" AND enabled=1 FOR UPDATE");if(!r.RowCount())return {};if(r.RowCount()!=1)throw std::invalid_argument("Ambiguous catch-up review");auto p=r.begin();
 if(N(p[0])!=native||N(p[1])!=kind||N(p[2])!=owner||N(p[3])!=revision||!p[4]||!p[5]||Hex(Hash(definitions))!=p[4]||Hex(Hash(regions))!=p[5]||N(p[6])<1||N(p[6])>2160||!p[7]||!*p[7])return {};
 const auto reference=std::string(p[7]);auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_upkeep_catchup_checkpoints','housing_upkeep_catchup_audits')");if(engines.RowCount()!=1||N(engines.begin()[0])!=2)throw std::invalid_argument("Partial catch-up schema");
 std::vector<uint8_t> proof;for(uint64_t n:{uint64_t(estate),uint64_t(native),uint64_t(kind),uint64_t(owner),revision,N(p[6])})Word(proof,n);Blob(proof,definitions);Blob(proof,regions);proof.insert(proof.end(),reference.begin(),reference.end());return Policy{uint32_t(N(p[6])),Hash(proof)};
}
inline std::optional<Checkpoint> MatchPast(Database& db,uint32_t estate,uint32_t due,uint32_t now,uint64_t contract_revision,uint64_t balance,const Digest& obligations,const Policy& policy){
 auto r=Q(db,"SELECT next_due_epoch,anchor_due_epoch,observed_at_epoch,contract_revision,escrow_copper,HEX(obligations_sha256),HEX(policy_sha256) FROM housing_upkeep_catchup_checkpoints WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(!r.RowCount())return {};if(r.RowCount()!=1)throw std::invalid_argument("Ambiguous catch-up checkpoint");auto p=r.begin();
 const auto anchor=N(p[1]),observed=N(p[2]);
 if(N(p[0])!=due||!anchor||anchor>due||observed>=anchor||!observed||anchor>UINT32_MAX||observed>UINT32_MAX||uint64_t(now)>anchor+uint64_t(policy.max_hours)*3600||N(p[3])!=contract_revision||N(p[4])!=balance||!p[5]||!p[6]||Hex(obligations)!=p[5]||Hex(policy.identity)!=p[6])return {};
 return Checkpoint{due,uint32_t(anchor),uint32_t(observed),contract_revision,balance,obligations};
}
inline void Advance(Database& db,uint32_t estate,uint32_t due,uint32_t now,uint64_t revision,uint64_t balance,const Digest& obligations,const Policy& policy,const std::optional<Checkpoint>& past){
 auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0])!=1||!due||due>UINT32_MAX-3600||!revision)throw std::invalid_argument("Catch-up requires billing transaction");
 auto assessment=Q(db,"SELECT c.next_due_epoch,c.revision,r.contract_revision,r.escrow_after FROM housing_upkeep_contracts c JOIN housing_upkeep_cycles r ON r.estate_id=c.estate_id AND r.due_epoch="+std::to_string(due)+" WHERE c.estate_id="+std::to_string(estate)+" FOR UPDATE");if(assessment.RowCount()!=1||N(assessment.begin()[0])!=due+3600||N(assessment.begin()[1])!=revision||N(assessment.begin()[2])==UINT64_MAX||N(assessment.begin()[2])+1!=revision||N(assessment.begin()[3])!=balance)throw std::invalid_argument("Catch-up checkpoint lacks committed assessment bookkeeping");
 const auto next=due+3600,anchor=past?past->anchor_due:next,observed=past?past->observed_at:now;if(observed>=anchor)throw std::invalid_argument("No retroactive catch-up checkpoint");
 if(past){auto u=Q(db,"UPDATE housing_upkeep_catchup_checkpoints SET next_due_epoch="+std::to_string(next)+",contract_revision="+std::to_string(revision)+",escrow_copper="+std::to_string(balance)+",obligations_sha256=UNHEX('"+Hex(obligations)+"') WHERE estate_id="+std::to_string(estate)+" AND next_due_epoch="+std::to_string(due)+" AND contract_revision="+std::to_string(past->contract_revision)+" AND escrow_copper="+std::to_string(past->balance)+" AND obligations_sha256=UNHEX('"+Hex(past->obligations)+"') AND policy_sha256=UNHEX('"+Hex(policy.identity)+"')");if(u.RowsAffected()!=1)throw std::invalid_argument("Stale catch-up checkpoint");
  Q(db,"INSERT INTO housing_upkeep_catchup_audits(estate_id,due_epoch,anchor_due_epoch,observed_at_epoch,processed_at_epoch,before_sha256,after_sha256,policy_sha256) VALUES("+std::to_string(estate)+","+std::to_string(due)+","+std::to_string(anchor)+","+std::to_string(observed)+","+std::to_string(now)+",UNHEX('"+Hex(past->obligations)+"'),UNHEX('"+Hex(obligations)+"'),UNHEX('"+Hex(policy.identity)+"'))");
 }else Q(db,"INSERT INTO housing_upkeep_catchup_checkpoints(estate_id,next_due_epoch,anchor_due_epoch,observed_at_epoch,contract_revision,escrow_copper,obligations_sha256,policy_sha256) VALUES("+std::to_string(estate)+","+std::to_string(next)+","+std::to_string(anchor)+","+std::to_string(observed)+","+std::to_string(revision)+","+std::to_string(balance)+",UNHEX('"+Hex(obligations)+"'),UNHEX('"+Hex(policy.identity)+"')) ON DUPLICATE KEY UPDATE next_due_epoch=VALUES(next_due_epoch),anchor_due_epoch=VALUES(anchor_due_epoch),observed_at_epoch=VALUES(observed_at_epoch),contract_revision=VALUES(contract_revision),escrow_copper=VALUES(escrow_copper),obligations_sha256=VALUES(obligations_sha256),policy_sha256=VALUES(policy_sha256)");
}
}
#endif
