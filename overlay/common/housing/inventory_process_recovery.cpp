#include "inventory_process_recovery.h"
#include "layout_mixed_journal_db.h"
#include "coin_pending_guard.h"
#include "live_escrow_service.h"
#include "native_plot_purchase.h"
#include "house_inventory_transition.h"
#include "house_binding_service.h"
#include <mysqld_error.h>
#include <type_traits>
#include <stdexcept>

namespace Housing::InventoryRecovery {
namespace {
struct Failure { Error error; };
MySQLRequestResult Query(Database& db, const std::string& sql)
{
 auto rows = db.QueryDatabase(sql, false);
 if (!rows.Success()) throw Failure{Error::DatabaseFailure};
 return rows;
}
std::string Quote(Database& db, const std::string& value)
{
 return "'" + db.Escape(value) + "'";
}
bool Key(const std::string& value)
{
 if (value.empty() || value.size() > 96) return false;
 for (const unsigned char c : value) {
  if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == ':' || c == '_' || c == '-' || c == '.')) return false;
 }
 return true;
}
std::string Hex(const std::array<uint8_t, 32>& token)
{
 static constexpr char digits[] = "0123456789ABCDEF";
 std::string out;
 for (const auto byte : token) { out += digits[byte >> 4]; out += digits[byte & 15]; }
 return out;
}
std::array<uint8_t,32> Token(const char* text)
{
 if (!text || std::string(text).size() != 64) throw Failure{Error::DatabaseFailure};
 auto nibble = [](char c) -> uint8_t {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  throw Failure{Error::DatabaseFailure};
 };
 std::array<uint8_t,32> out;
 for (size_t i=0; i<out.size(); ++i) out[i] = (nibble(text[i*2]) << 4) | nibble(text[i*2+1]);
 return out;
}
class Transaction {
 Database& db_; bool active_ = true;
public:
 explicit Transaction(Database& db) : db_(db) { Query(db_, "START TRANSACTION"); }
 ~Transaction() { if (active_) try { db_.QueryDatabase(std::string("ROLLBACK"), false); } catch (...) {} }
 void Commit() {
  try {
   if (!db_.QueryDatabase(std::string("COMMIT"), false).Success()) throw Failure{Error::CommitUncertain};
   active_ = false;
  } catch (...) { throw Failure{Error::CommitUncertain}; }
 }
};
void Verify(Database& db)
{
 auto rows = Query(db, "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_inventory_processes','housing_inventory_custody_links','housing_inventory_death_reconciliations','housing_inventory_sessions','housing_inventory_session_history','housing_live_item_leases','housing_live_item_receipts','housing_live_item_custody','inventory')");
 if (rows.RowCount()!=1 || std::stoul(rows.begin()[0])!=9) throw Failure{Error::DatabaseFailure};
}
std::string LeasePredicate(Database& db, const InventorySessions::Lease& lease)
{
 return "character_id=" + std::to_string(lease.actor.character_id) +
  " AND account_id=" + std::to_string(lease.actor.account_id) +
  " AND epoch=" + std::to_string(lease.epoch) +
  " AND token=x'" + Hex(lease.token) + "' AND process_key=" + Quote(db, lease.owner.process_key) +
  " AND client_key=" + Quote(db, lease.owner.client_key) +
  " AND zone_id=" + std::to_string(lease.owner.zone_id) +
  " AND instance_id=" + std::to_string(lease.owner.instance_id);
}
}

bool RegisteredLiveProcess(Database& db, const std::string& process_key) noexcept
{
 try {
  if (!Key(process_key)) return false;
  auto engine = Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_inventory_processes'");
  if (engine.RowCount()!=1 || !engine.begin()[0] || std::string(engine.begin()[0])!="InnoDB") return false;
  auto rows = Query(db, "SELECT reaped FROM housing_inventory_processes WHERE process_key=" + Quote(db, process_key) + " FOR UPDATE");
  return rows.RowCount()==1 && rows.begin()[0] && std::string(rows.begin()[0])=="0";
 } catch (...) { return false; }
}

bool LinkCustodyToReservation(Database& db, const LiveItems::Lease& lease) noexcept
{
 try {
  // Schema absence is an unsupervised path, never an eligible recovery path.
  auto probe = db.QueryDatabase(std::string("SELECT process_key FROM housing_inventory_processes WHERE 1=0"), false);
  if (!probe.Success()) return probe.ErrorNumber()==ER_NO_SUCH_TABLE;
  auto owners = Query(db, "SELECT account_id,epoch,HEX(token),process_key,client_key,active FROM housing_inventory_sessions WHERE character_id=" + std::to_string(lease.actor.character_id) + " FOR UPDATE");
  if (!owners.RowCount()) return true;
  if (owners.RowCount()!=1) return false;
  auto owner = owners.begin();
  auto process = Query(db, "SELECT reaped FROM housing_inventory_processes WHERE process_key=" + Quote(db, owner[3]) + " FOR UPDATE");
  if (!process.RowCount()) return true; // Existing unregistered process stays unrecoverable.
  if (process.RowCount()!=1 || std::string(process.begin()[0])!="0" || std::string(owner[5])!="1" || std::stoul(owner[0])!=lease.actor.account_id) return false;
  auto engines = Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_inventory_processes','housing_inventory_custody_links','housing_inventory_death_reconciliations')");
  if (engines.RowCount()!=1 || std::stoul(engines.begin()[0])!=3) return false;
  auto old = Query(db, "SELECT inventory_epoch,HEX(inventory_token),process_key,client_key,account_id,custody_session,HEX(custody_token) FROM housing_inventory_custody_links WHERE character_id=" + std::to_string(lease.actor.character_id) + " AND custody_epoch=" + std::to_string(lease.epoch) + " FOR UPDATE");
  if (old.RowCount()) {
   auto row=old.begin();
   return old.RowCount()==1 && std::string(row[0])==owner[1] && std::string(row[1])==owner[2] &&
    std::string(row[2])==owner[3] && std::string(row[3])==owner[4] && std::stoul(row[4])==lease.actor.account_id &&
    std::string(row[5])==lease.session_key && std::string(row[6])==Hex(lease.token);
  }
  auto inserted = Query(db, "INSERT INTO housing_inventory_custody_links(character_id,inventory_epoch,inventory_token,process_key,client_key,account_id,custody_epoch,custody_session,custody_token) VALUES (" +
   std::to_string(lease.actor.character_id) + "," + owner[1] + ",x'" + owner[2] + "'," + Quote(db,owner[3]) + "," + Quote(db,owner[4]) + "," + std::to_string(lease.actor.account_id) + "," + std::to_string(lease.epoch) + "," + Quote(db,lease.session_key) + ",x'" + Hex(lease.token) + "')");
  return inserted.RowsAffected()==1;
 } catch (...) { return false; }
}

bool LockCustodyReservation(Database& db, AuthenticatedPurchaser actor) noexcept
{
 try {
  auto probe=db.QueryDatabase(std::string("SELECT process_key FROM housing_inventory_processes WHERE 1=0"),false);
  if(!probe.Success())return probe.ErrorNumber()==ER_NO_SUCH_TABLE;
  auto lookup=Query(db,"SELECT process_key FROM housing_inventory_sessions WHERE character_id="+std::to_string(actor.character_id));
  if(!lookup.RowCount())return true;
  if(lookup.RowCount()!=1||!lookup.begin()[0])return false;
  const std::string process_key=lookup.begin()[0];
  auto process=Query(db,"SELECT reaped FROM housing_inventory_processes WHERE process_key="+Quote(db,process_key)+" FOR UPDATE");
  if(!process.RowCount())return true;
  if(process.RowCount()!=1||std::string(process.begin()[0])!="0")return false;
  auto owner=Query(db,"SELECT account_id,process_key,active FROM housing_inventory_sessions WHERE character_id="+std::to_string(actor.character_id)+" FOR UPDATE");
  return owner.RowCount()==1&&std::stoul(owner.begin()[0])==actor.account_id&&std::string(owner.begin()[1])==process_key&&std::string(owner.begin()[2])=="1";
 }catch(...){return false;}
}

bool ReceiptDeathReconciled(Database& db,uint32_t actor,const std::string& key)noexcept {
 try {
  if(!actor||!Key(key))return false;
  auto rows=Query(db,"SELECT COUNT(*) FROM housing_live_item_receipts r JOIN housing_inventory_death_reconciliations d ON d.character_id=r.character_id AND d.custody_epoch=r.lease_epoch AND BINARY d.custody_session=BINARY r.session_key AND BINARY d.pending_key=BINARY r.request_key AND d.outcome_blob=r.after_blob JOIN housing_inventory_session_history h ON h.character_id=d.character_id AND h.epoch=d.inventory_epoch AND h.token=d.inventory_token AND BINARY h.process_key=BINARY d.process_key AND h.account_id=r.account_id AND h.released=1 JOIN housing_inventory_custody_links l ON l.character_id=d.character_id AND l.inventory_epoch=d.inventory_epoch AND l.inventory_token=d.inventory_token AND BINARY l.process_key=BINARY d.process_key AND BINARY l.client_key=BINARY h.client_key AND l.account_id=r.account_id AND l.custody_epoch=d.custody_epoch AND BINARY l.custody_session=BINARY r.session_key JOIN housing_inventory_processes p ON BINARY p.process_key=BINARY d.process_key AND p.reaped=1 AND p.child_pid>0 AND p.exit_code IS NOT NULL AND p.reaped_at IS NOT NULL WHERE r.character_id="+std::to_string(actor)+" AND BINARY r.request_key=BINARY "+Quote(db,key)+" AND r.completed=1 AND r.applied=0 AND r.after_blob IS NOT NULL");
  if(rows.RowCount()!=1||!rows.begin()[0])return false;
  if(std::string(rows.begin()[0])=="1")return true;
  // Retained tokens have an immutable child receipt, but death reconciliation
  // belongs to their aggregate. Require the same full supervisor/history/link
  // proof plus the exact certified parent journal and exact saved child bytes.
  auto parents=Query(db,"SELECT d.pending_key,HEX(d.outcome_blob),r.account_id,r.lease_epoch,HEX(r.session_key),HEX(r.after_blob) FROM housing_live_item_receipts r JOIN housing_inventory_death_reconciliations d ON d.character_id=r.character_id AND d.custody_epoch=r.lease_epoch AND BINARY d.custody_session=BINARY r.session_key JOIN housing_layout_mixed_batches m ON BINARY m.request_key=BINARY d.pending_key AND m.character_id=r.character_id AND m.account_id=r.account_id AND m.lease_epoch=r.lease_epoch AND BINARY m.session_key=BINARY r.session_key AND m.outcome_blob=d.outcome_blob AND m.applied=0 JOIN housing_inventory_session_history h ON h.character_id=d.character_id AND h.epoch=d.inventory_epoch AND h.token=d.inventory_token AND BINARY h.process_key=BINARY d.process_key AND h.account_id=r.account_id AND h.released=1 JOIN housing_inventory_custody_links l ON l.character_id=d.character_id AND l.inventory_epoch=d.inventory_epoch AND l.inventory_token=d.inventory_token AND BINARY l.process_key=BINARY d.process_key AND BINARY l.client_key=BINARY h.client_key AND l.account_id=r.account_id AND l.custody_epoch=d.custody_epoch AND BINARY l.custody_session=BINARY r.session_key JOIN housing_inventory_processes p ON BINARY p.process_key=BINARY d.process_key AND p.reaped=1 AND p.child_pid>0 AND p.exit_code IS NOT NULL AND p.reaped_at IS NOT NULL WHERE r.character_id="+std::to_string(actor)+" AND BINARY r.request_key=BINARY "+Quote(db,key)+" AND r.completed=1 AND r.applied=0 AND r.after_blob IS NOT NULL");
  size_t matched=0;
  for(auto row=parents.begin();row!=parents.end();++row){
   for(size_t i=0;i<6;++i)if(!row[i])return false;
   const auto outcome=LayoutMixed::Decode(LayoutMixedJournal::Detail::Unhex(row[1],LayoutMixed::MaximumBytes));
   if(!outcome)return false;
   for(const auto& token:outcome->retained_tokens){
    if(token.child_key!=key)continue;
    if(token.child_key!=LayoutProperty::Detail::Key(row[0],token.operation_index,"inventory")||token.after.creator!=actor||token.after.account!=std::stoul(row[2])||token.after.epoch!=std::stoull(row[3])||CustodyCodec::Bytes(token.after.session.begin(),token.after.session.end())!=LayoutMixedJournal::Detail::Unhex(row[4],96)||token.child_outcome!=LayoutMixedJournal::Detail::Unhex(row[5],262144))return false;
    ++matched;
   }
  }
  return matched==1;
 }catch(...){return false;}
}

Service::Service(std::unique_ptr<Database> db, bool enabled) : database_(std::move(db)), enabled_(enabled) {}
Result<Reconciliation> Service::Reconcile(const InventorySessions::Lease& lease)
{
 std::lock_guard<std::mutex> lock(mutex_);
 if (!enabled_) return {{},Error::Disabled};
 if (!lease.actor.character_id || !lease.actor.account_id || !lease.epoch || !lease.owner.zone_id ||
     !Key(lease.owner.process_key) || !Key(lease.owner.client_key)) return {{},Error::InvalidArgument};
 if (!database_ || database_->GetStatus()!=DBcore::Connected) return {{},Error::DatabaseFailure};
 try {
  auto& db=*database_; Transaction transaction(db); Verify(db);
  auto proof = Query(db, "SELECT reaped,child_pid,exit_code,reaped_at FROM housing_inventory_processes WHERE process_key=" + Quote(db,lease.owner.process_key) + " FOR UPDATE");
  if (proof.RowCount()!=1 || !proof.begin()[0] || std::string(proof.begin()[0])!="1" || !proof.begin()[1] ||
      std::stoull(proof.begin()[1])==0 || !proof.begin()[2] || !proof.begin()[3]) throw Failure{Error::NoDeathProof};
  const auto predicate = LeasePredicate(db,lease);
  auto history = Query(db,"SELECT released FROM housing_inventory_session_history WHERE " + predicate + " FOR UPDATE");
  if (history.RowCount()!=1) throw Failure{Error::Stale};
  auto audit = Query(db,"SELECT process_key,HEX(inventory_token),pending_key FROM housing_inventory_death_reconciliations WHERE character_id=" + std::to_string(lease.actor.character_id) + " AND inventory_epoch=" + std::to_string(lease.epoch) + " FOR UPDATE");
  Reconciliation out{lease.actor.character_id,lease.epoch,{},false};
  if (audit.RowCount()) {
   if (audit.RowCount()!=1 || std::string(audit.begin()[0])!=lease.owner.process_key || std::string(audit.begin()[1])!=Hex(lease.token) || std::string(history.begin()[0])!="1") throw Failure{Error::Stale};
   out.pending_key = audit.begin()[2] ? audit.begin()[2] : ""; out.replayed=true;
   Result<Reconciliation> result{std::move(out),Error::None}; transaction.Commit(); return result;
  }
  if (std::string(history.begin()[0])!="0") throw Failure{Error::Stale};
  auto current = Query(db,"SELECT active FROM housing_inventory_sessions WHERE " + predicate + " FOR UPDATE");
  if (current.RowCount()!=1 || std::string(current.begin()[0])!="1") throw Failure{Error::Stale};
  const auto coin=CoinPending::InspectEscrow(db,lease.actor.character_id,true);
  const auto purchase=CoinPending::InspectPurchase(db,lease.actor.character_id,true);
  if(purchase==CoinPending::State::Unavailable)throw Failure{Error::DatabaseFailure};
  if(purchase==CoinPending::State::Pending&&coin!=CoinPending::State::Clear)throw Failure{Error::InvalidOutcome};
  const auto ordinary_pending=HouseInventory::InspectPending(db,lease.actor.character_id,true);
  std::optional<HouseInventory::Receipt> house_move;
  if(ordinary_pending==HouseInventory::PendingState::Unavailable)throw Failure{Error::DatabaseFailure};
  if(ordinary_pending==HouseInventory::PendingState::Pending){
   auto validated=HouseInventory::ValidateRestartOutcome(db,lease);
   if(!validated)throw Failure{Error::InvalidOutcome};
   house_move=std::move(validated.value);
  }
  if(coin==CoinPending::State::Unavailable)throw Failure{Error::DatabaseFailure};
  if((coin==CoinPending::State::Pending||purchase==CoinPending::State::Pending)&&house_move)throw Failure{Error::InvalidOutcome};
  std::optional<LiveEscrow::Receipt> occupied_refund,bulk_escrow;
  auto custody = Query(db,"SELECT account_id,epoch,session_key,HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id=" + std::to_string(lease.actor.character_id) + " FOR UPDATE");
  std::string custody_epoch="NULL", custody_session="NULL", pending="NULL", outcome="NULL";
  if (custody.RowCount()) {
   if (custody.RowCount()!=1) throw Failure{Error::UnlinkedCustody};
   auto row=custody.begin();
   if (std::string(row[4])!="0" && std::string(row[4])!="1") throw Failure{Error::InvalidOutcome};
   if (std::string(row[4])=="1" || row[5]) {
    auto link=Query(db,"SELECT inventory_epoch,HEX(inventory_token),process_key,client_key,account_id,custody_session,HEX(custody_token) FROM housing_inventory_custody_links WHERE character_id=" + std::to_string(lease.actor.character_id) + " AND custody_epoch=" + row[1] + " FOR UPDATE");
    if (link.RowCount()!=1) throw Failure{Error::UnlinkedCustody};
    auto bound=link.begin();
    if (std::stoull(bound[0])!=lease.epoch || std::string(bound[1])!=Hex(lease.token) || std::string(bound[2])!=lease.owner.process_key || std::string(bound[3])!=lease.owner.client_key || std::stoul(bound[4])!=lease.actor.account_id || std::string(bound[5])!=row[2] || std::string(bound[6])!=row[3] || std::stoul(row[0])!=lease.actor.account_id || std::string(row[4])!="1") throw Failure{Error::UnlinkedCustody};
    LiveItems::Lease old{lease.actor,std::stoull(row[1]),row[2],Token(row[3]),row[5]?row[5]:""};
    custody_epoch=row[1]; custody_session=Quote(db,old.session_key);
    if(purchase==CoinPending::State::Pending){
     auto result=NativePlotPurchase::Service::ReconcileRestartUnderTransaction(db,lease,old);
     if(!result)throw Failure{Error::InvalidOutcome};out.pending_key=result.value.first;pending=Quote(db,out.pending_key);std::string bytes;const char*hex="0123456789ABCDEF";for(auto x:result.value.second){bytes+=hex[x>>4];bytes+=hex[x&15];}outcome="x'"+bytes+"'";
    }

    if(coin==CoinPending::State::Pending){
     auto validated=LiveEscrow::Service::ValidateOccupiedRefundRestartUnderTransaction(db,lease,old);
     if(!validated){
      auto bulk=LiveEscrow::Service::ValidateBulkRestartUnderTransaction(db,lease,old);
      if(!bulk)throw Failure{Error::InvalidOutcome};bulk_escrow=std::move(bulk.value);
      out.pending_key=bulk_escrow->request_key;pending=Quote(db,out.pending_key);
      auto receipt=Query(db,"SELECT HEX(outcome_blob) FROM housing_live_escrow_receipts WHERE request_key="+pending+" AND applied=0 FOR UPDATE");
      if(receipt.RowCount()!=1||!receipt.begin()[0])throw Failure{Error::InvalidOutcome};outcome="x'"+std::string(receipt.begin()[0])+"'";
     }else occupied_refund=std::move(validated.value);
     if(occupied_refund&&old.pending_key.empty()){
      out.pending_key=occupied_refund->request_key;pending=Quote(db,out.pending_key);
      auto receipt=Query(db,"SELECT HEX(after_blob) FROM housing_live_item_receipts WHERE request_key="+pending+" AND completed=1 AND applied=1 FOR UPDATE");
      if(receipt.RowCount()!=1||!receipt.begin()[0])throw Failure{Error::InvalidOutcome};outcome="x'"+std::string(receipt.begin()[0])+"'";
     }
    }
    if (!old.pending_key.empty()) {
     // Two committed protocols cannot own the same actor's application gap.
     if(house_move)throw Failure{Error::InvalidOutcome};
     auto record_layout=[&](const auto& bytes){
      // Exact process/history/custody linkage and certified child death were
      // proved above. Preserve unapplied=0: SQL recovery is not live memory ACK.
      out.pending_key=old.pending_key;pending=Quote(db,old.pending_key);
      std::string encoded;const char* digits="0123456789ABCDEF";for(auto byte:bytes){encoded+=digits[byte>>4];encoded+=digits[byte&15];}outcome="x'"+encoded+"'";
     };
     auto mixed=LayoutMixedJournal::ReadPendingForRestartUnderTransaction(db,old);
     if(mixed){record_layout(LayoutMixed::Encode(mixed.value.outcome));LayoutRetained::ReleaseHouses(db,mixed.value.outcome.retained_tokens,old.pending_key);}
     else{
      if(mixed.error!=LiveItems::Error::NotFound)throw Failure{Error::InvalidOutcome};
      auto layout=LayoutInventoryJournal::ReadPendingForRestartUnderTransaction(db,old);
      if(layout){record_layout(LayoutInventory::EncodeOutcome(layout.value.outcome));}
      else{
       if(layout.error!=LiveItems::Error::NotFound)throw Failure{Error::InvalidOutcome};
       auto validated=LiveItems::Service::ValidateRestartOutcome(db,old);
       if(!validated)throw Failure{Error::InvalidOutcome};
       if(validated.value.house_effect){
        const auto& house=validated.value.house_effect->after;
        if(Query(db,"UPDATE housing_house_instances SET pending_request_key=NULL WHERE house_id="+std::to_string(house.house_id)+" AND revision="+std::to_string(house.revision)+" AND pending_request_key="+Quote(db,old.pending_key)).RowsAffected()!=1)throw Failure{Error::InvalidOutcome};
       }
       out.pending_key=old.pending_key;pending=Quote(db,old.pending_key);
       auto receipt=Query(db,"SELECT HEX(after_blob) FROM housing_live_item_receipts WHERE request_key="+pending+" FOR UPDATE");
       if(receipt.RowCount()!=1||!receipt.begin()[0])throw Failure{Error::InvalidOutcome};outcome="x'"+std::string(receipt.begin()[0])+"'";
      }
     }
    }
    auto cleared=Query(db,"UPDATE housing_live_item_leases SET active=0,pending_key=NULL WHERE character_id=" + std::to_string(lease.actor.character_id) + " AND epoch=" + custody_epoch + " AND session_key=" + custody_session + " AND token=x'" + Hex(old.token) + "' AND active=1");
    if (cleared.RowsAffected()!=1) throw Failure{Error::Stale};
   }
  }
  if(purchase==CoinPending::State::Pending&&custody_epoch=="NULL")throw Failure{Error::UnlinkedCustody};
  if(house_move){
   const auto key=Quote(db,house_move->request.request_key);
   auto receipt=Query(db,"SELECT HEX(outcome_blob) FROM housing_house_inventory_receipts WHERE request_key="+key+" AND applied=0 FOR UPDATE");
   if(receipt.RowCount()!=1||!receipt.begin()[0])throw Failure{Error::InvalidOutcome};
   out.pending_key=house_move->request.request_key;pending=key;
   outcome="x'"+std::string(receipt.begin()[0])+"'";
   for(const auto& house:house_move->after){
    if(Query(db,"UPDATE housing_house_instances SET pending_request_key=NULL WHERE house_id="+std::to_string(house.house_id)+" AND revision="+std::to_string(house.revision)+" AND pending_request_key="+key).RowsAffected()!=1)throw Failure{Error::InvalidOutcome};
   }
   if(Query(db,"DELETE FROM housing_house_inventory_pending WHERE character_id="+std::to_string(lease.actor.character_id)+" AND inventory_epoch="+std::to_string(lease.epoch)+" AND request_key="+key).RowsAffected()!=1)throw Failure{Error::InvalidOutcome};
   if(Query(db,"UPDATE housing_house_inventory_receipts SET applied=1 WHERE request_key="+key+" AND applied=0").RowsAffected()!=1)throw Failure{Error::InvalidOutcome};
  }
  if(coin==CoinPending::State::Pending){
   if(!occupied_refund&&!bulk_escrow)throw Failure{Error::InvalidOutcome};
   const auto& recovered=bulk_escrow?*bulk_escrow:*occupied_refund;
   const auto table=bulk_escrow?"housing_escrow_bulk_death_reconciliations":"housing_occupied_refund_death_reconciliations";
   auto saved=Query(db,"INSERT INTO "+std::string(table)+"(request_key,character_id,inventory_epoch,inventory_token,process_key,outcome_blob) SELECT request_key,character_id,inventory_epoch,inventory_token,process_key,outcome_blob FROM housing_live_escrow_receipts WHERE request_key="+Quote(db,recovered.request_key)+" AND applied=0");
   auto cleared=Query(db,"DELETE FROM housing_live_escrow_pending WHERE request_key="+Quote(db,recovered.request_key)+" AND character_id="+std::to_string(lease.actor.character_id)+" AND inventory_epoch="+std::to_string(lease.epoch)+" AND inventory_token=x'"+Hex(lease.token)+"'");
   if(saved.RowsAffected()!=1||cleared.RowsAffected()!=1)throw Failure{Error::InvalidOutcome};
   // Currency and inventory were committed already. Certified death permits a
   // fresh DB load; it never asserts that dead-process memory was applied.
  }
  auto inserted=Query(db,"INSERT INTO housing_inventory_death_reconciliations(character_id,inventory_epoch,process_key,inventory_token,custody_epoch,custody_session,pending_key,outcome_blob) VALUES (" + std::to_string(lease.actor.character_id) + "," + std::to_string(lease.epoch) + "," + Quote(db,lease.owner.process_key) + ",x'" + Hex(lease.token) + "'," + custody_epoch + "," + custody_session + "," + pending + "," + outcome + ")");
  auto released=Query(db,"UPDATE housing_inventory_session_history SET released=1 WHERE " + predicate + " AND released=0");
  auto inactive=Query(db,"UPDATE housing_inventory_sessions SET active=0 WHERE " + predicate + " AND active=1");
  if (inserted.RowsAffected()!=1 || released.RowsAffected()!=1 || inactive.RowsAffected()!=1) throw Failure{Error::DatabaseFailure};
  static_assert(std::is_nothrow_move_constructible_v<Result<Reconciliation>>);
  Result<Reconciliation> result{std::move(out),Error::None}; transaction.Commit(); return result;
 } catch (const Failure& failure) { return {{},failure.error}; }
 catch (...) { return {{},Error::DatabaseFailure}; }
}
}
