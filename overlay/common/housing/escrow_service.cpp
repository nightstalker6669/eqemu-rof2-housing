#include "escrow_service.h"
#include "../database.h"
#include <limits>
#include <stdexcept>
#include <utility>
namespace Housing::Escrow {
namespace {
struct Failure { Error error; };
MySQLRequestResult Checked(Database& db, const std::string& sql) {
    auto rows = db.QueryDatabase(sql, false);
    if (!rows.Success()) throw Failure{Error::DatabaseFailure};
    return rows;
}
class Transaction {
public:
    explicit Transaction(Database& db) : db_(db) { Checked(db_, "START TRANSACTION"); }
    ~Transaction() { if(active_) { try { db_.QueryDatabase(std::string("ROLLBACK"),false); } catch(...) {} } }
    void Commit() {
        // Once COMMIT is attempted, even a thrown transport/allocation error may
        // follow a server-side commit. Never classify it as a known rollback.
        try {
            auto rows=db_.QueryDatabase(std::string("COMMIT"),false);
            if(!rows.Success()) throw Failure{Error::CommitUncertain};
            active_=false;
        } catch(...) { throw Failure{Error::CommitUncertain}; }
    }
private: Database& db_; bool active_=true;
};
bool ValidKey(const std::string& key) {
    if(key.empty() || key.size()>96) return false;
    for(unsigned char c:key) if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c==':'||c=='.')) return false;
    return true;
}
std::string Quote(Database& db,const std::string& value) { return "'"+db.Escape(value)+"'"; }
bool FixtureName(const std::string& name) {
    const std::string prefix="EscrowFixture";
    if(name.size()!=prefix.size()+13 || name.compare(0,prefix.size(),prefix)!=0) return false;
    for(size_t i=prefix.size();i<name.size();++i) if(name[i]<'0'||name[i]>'9') return false;
    return true;
}
void VerifyDatabase(Database& db) {
    auto name=Checked(db,"SELECT DATABASE()");
    if(name.RowCount()!=1 || !name.begin()[0]) throw Failure{Error::UnsupportedFunding};
    const std::string n=name.begin()[0];
    if(n!="peq_housing_test" && n.rfind("peq_housing_test_",0)!=0) throw Failure{Error::UnsupportedFunding};
    auto rows=Checked(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND engine='InnoDB' AND table_name IN ('housing_estates','housing_acl_state','housing_acl_groups',"
        "'housing_acl_members','character_data','account','character_currency','housing_escrow_balances','housing_escrow_transfers')");
    if(rows.RowCount()!=1 || std::stoul(rows.begin()[0])!=9) throw Failure{Error::DatabaseFailure};
}
void Authorize(Database& db,const Request& request) {
    auto estate=Checked(db,"SELECT owner_kind,owner_id FROM housing_current_estates WHERE id="+std::to_string(request.estate_id)+" FOR UPDATE");
    if(estate.RowCount()!=1) throw Failure{Error::NotFound};
    if(std::stoul(estate.begin()[0])!=static_cast<uint32_t>(OwnerKind::Character)) throw Failure{Error::Unauthorized};
    const auto owner=static_cast<uint32_t>(std::stoul(estate.begin()[1]));
    auto actor=Checked(db,"SELECT account_id,name,gm,deleted_at,ingame FROM character_data WHERE id="+
        std::to_string(request.actor.character_id)+" FOR UPDATE");
    if(actor.RowCount()!=1 || std::stoul(actor.begin()[0])!=request.actor.account_id || actor.begin()[3]) throw Failure{Error::Unauthorized};
    auto account=Checked(db,"SELECT name,status,lsaccount_id,(password IS NULL OR password='') FROM account WHERE id="+std::to_string(request.actor.account_id)+" FOR UPDATE");
    if(account.RowCount()!=1 || !FixtureName(actor.begin()[1]) || std::string(actor.begin()[1])!=account.begin()[0] ||
        std::stoul(actor.begin()[2]) || std::stoul(account.begin()[1]) || account.begin()[2] || std::stoul(account.begin()[3])!=1) throw Failure{Error::Unauthorized};
    // Additional heuristic only. ingame does NOT prove offline presence. Dedicated
    // unloginable harness identity/lifecycle is the actual isolated-actor restriction.
    if(std::stoul(actor.begin()[4])) throw Failure{Error::UnsupportedFunding};
    auto state=Checked(db,"SELECT revision FROM housing_acl_state WHERE estate_id="+std::to_string(request.estate_id)+" FOR UPDATE");
    auto masks=PersonalAclDefaultMasks;
    uint8_t rank=0;
    if(state.RowCount()) {
        if(state.RowCount()!=1 || !std::stoull(state.begin()[0])) throw Failure{Error::DatabaseFailure};
        auto groups=Checked(db,"SELECT group_id,capabilities FROM housing_acl_groups WHERE estate_id="+std::to_string(request.estate_id)+" ORDER BY group_id FOR UPDATE");
        if(groups.RowCount()!=7) throw Failure{Error::DatabaseFailure};
        uint32_t expected=0;
        for(auto row=groups.begin();row.operator!=(groups.end());++row) {
            const auto group=std::stoul(row[0]), mask=std::stoul(row[1]);
            if(group!=expected++ || (mask & ~PersonalAclKnownCapabilities) || (group==6 && mask)) throw Failure{Error::DatabaseFailure};
            masks[group]=static_cast<uint32_t>(mask);
        }
        auto member=Checked(db,"SELECT group_id FROM housing_acl_members WHERE estate_id="+std::to_string(request.estate_id)+
            " AND character_id="+std::to_string(request.actor.character_id)+" FOR UPDATE");
        if(member.RowCount()>1) throw Failure{Error::DatabaseFailure};
        if(member.RowCount()) { auto group=std::stoul(member.begin()[0]); if(group>5)throw Failure{Error::DatabaseFailure}; rank=static_cast<uint8_t>(group); }
    }
    // No DepositUpkeep capability exists in the proven enum. Owner-only deposit
    // is a conservative isolated-fixture policy, NOT recovered Live authorization.
    if(request.action==Action::Deposit && owner!=request.actor.character_id) throw Failure{Error::Unauthorized};
    if(request.action==Action::Withdraw && owner!=request.actor.character_id && !(masks[rank] & (1u<<2))) throw Failure{Error::Unauthorized};
}
int32_t SignedCounter(const char* text) {
    if(!text) throw Failure{Error::DatabaseFailure};
    auto v=std::stoll(text);
    if(v<0 || v>std::numeric_limits<int32_t>::max()) throw Failure{Error::Overflow};
    return static_cast<int32_t>(v);
}
const std::string receipt_columns="character_id,account_id,estate_id,action,denomination,quantity,"
    "before_copper,before_silver,before_gold,before_platinum,after_copper,after_silver,after_gold,after_platinum,"
    "copper_transferred,escrow_before,escrow_after,completed";
Receipt ExistingReceipt(MySQLRequestRow row,const Request& request) {
    if(std::stoul(row[0])!=request.actor.character_id || std::stoul(row[1])!=request.actor.account_id) throw Failure{Error::Unauthorized};
    CoinWallet::Snapshot before{SignedCounter(row[6]),SignedCounter(row[7]),SignedCounter(row[8]),SignedCounter(row[9])};
    if(std::stoul(row[2])!=request.estate_id || std::stoul(row[3])!=static_cast<uint8_t>(request.action) ||
        std::stoul(row[4])!=static_cast<uint8_t>(request.denomination) || std::stoll(row[5])!=request.quantity ||
        !(before==request.expected_cursor)) throw Failure{Error::Conflict};
    if(std::stoul(row[17])!=1) throw Failure{Error::DatabaseFailure};
    Receipt r{request,{SignedCounter(row[10]),SignedCounter(row[11]),SignedCounter(row[12]),SignedCounter(row[13])},
        std::stoll(row[14]),std::stoll(row[15]),std::stoll(row[16]),true};
    auto plan=request.action==Action::Deposit ? CoinWallet::DepositCursor(before,request.denomination,request.quantity) :
        CoinWallet::WithdrawCursor(before,request.denomination,request.quantity);
    if(!plan || !(r.cursor_after==plan.value.after) || r.copper_transferred!=plan.value.copper_amount ||
        r.escrow_before<0 || r.escrow_after<0) throw Failure{Error::DatabaseFailure};
    if(request.action==Action::Deposit) {
        if(r.escrow_before>std::numeric_limits<int64_t>::max()-r.copper_transferred || r.escrow_after!=r.escrow_before+r.copper_transferred) throw Failure{Error::DatabaseFailure};
    } else if(r.escrow_before<r.copper_transferred || r.escrow_after!=r.escrow_before-r.copper_transferred) throw Failure{Error::DatabaseFailure};
    return r;
}
std::string Values(const CoinWallet::Snapshot& v) {
    return std::to_string(v.copper)+","+std::to_string(v.silver)+","+std::to_string(v.gold)+","+std::to_string(v.platinum);
}
}
Service::Service(std::unique_ptr<Database> database,bool enabled):database_(std::move(database)),enabled_(enabled) {}
Service::Service(std::unique_ptr<Database> database,bool enabled,IsolatedFixture):database_(std::move(database)),enabled_(enabled),fixture_(true) {}
Service::~Service()=default;
Result<Receipt> Service::TransferCoins(const Request&) { return {{},enabled_?Error::UnsupportedFunding:Error::Disabled}; }
Result<Receipt> Service::TransferIsolatedCoins(const Request& request) { return Execute(request,false); }
Result<Receipt> Service::ResolveIsolatedReceipt(const Request& request) { return Execute(request,true); }
Result<Receipt> Service::Execute(const Request& request,bool lookup_only) {
    std::lock_guard<std::mutex> guard(mutex_);
    if(!enabled_) return {{},Error::Disabled};
    if(!fixture_) return {{},Error::UnsupportedFunding};
    if(!ValidKey(request.request_key) || !request.estate_id || !request.actor.character_id || !request.actor.account_id ||
        (request.action!=Action::Deposit && request.action!=Action::Withdraw) || !CoinWallet::Valid(request.expected_cursor)) return {{},Error::InvalidArgument};
    if(request.quantity<=0) return {{},Error::InvalidArgument};
    if(request.quantity>std::numeric_limits<int32_t>::max()) return {{},Error::Overflow};
    auto plan=request.action==Action::Deposit ? CoinWallet::DepositCursor(request.expected_cursor,request.denomination,request.quantity) :
        CoinWallet::WithdrawCursor(request.expected_cursor,request.denomination,request.quantity);
    if(!plan) return {{}, plan.error==CoinWallet::Error::Overflow?Error::Overflow:Error::InvalidArgument};
    if(!database_ || database_->GetStatus()!=DBcore::Connected) return {{},Error::DatabaseFailure};
    try {
        auto& db=*database_;Transaction transaction(db);VerifyDatabase(db);
        const auto key=Quote(db,request.request_key);
        if(!lookup_only) Checked(db,"INSERT INTO housing_escrow_transfers "
            "(request_key,character_id,account_id,estate_id,action,denomination,quantity,before_copper,before_silver,before_gold,before_platinum) VALUES ("+
            key+","+std::to_string(request.actor.character_id)+","+std::to_string(request.actor.account_id)+","+std::to_string(request.estate_id)+","+
            std::to_string(static_cast<uint8_t>(request.action))+","+std::to_string(static_cast<uint8_t>(request.denomination))+","+std::to_string(request.quantity)+","+
            Values(request.expected_cursor)+") ON DUPLICATE KEY UPDATE request_key=request_key");
        auto rows=Checked(db,"SELECT "+receipt_columns+" FROM housing_escrow_transfers WHERE request_key="+key+" FOR UPDATE");
        // Authenticate even historical receipt reads; revocation applies immediately.
        Authorize(db,request);
        if(!rows.RowCount()) throw Failure{Error::NotFound};
        if(rows.RowCount()!=1) throw Failure{Error::DatabaseFailure};
        auto row=rows.begin();
        if(std::stoul(row[0])!=request.actor.character_id || std::stoul(row[1])!=request.actor.account_id) throw Failure{Error::Unauthorized};
        CoinWallet::Snapshot bound{SignedCounter(row[6]),SignedCounter(row[7]),SignedCounter(row[8]),SignedCounter(row[9])};
        if(std::stoul(row[2])!=request.estate_id || std::stoul(row[3])!=static_cast<uint8_t>(request.action) ||
            std::stoul(row[4])!=static_cast<uint8_t>(request.denomination) || std::stoll(row[5])!=request.quantity || !(bound==request.expected_cursor)) throw Failure{Error::Conflict};
        if(std::stoul(row[17])==1) { auto r=ExistingReceipt(row,request);transaction.Commit();return {r,Error::None}; }
        if(lookup_only) throw Failure{Error::NotFound}; // Never finish a pending record through lookup.
        const auto actor=std::to_string(request.actor.character_id), estate=std::to_string(request.estate_id);
        auto wallet=Checked(db,"SELECT copper_cursor,silver_cursor,gold_cursor,platinum_cursor FROM character_currency WHERE id="+actor+" FOR UPDATE");
        if(wallet.RowCount()!=1) throw Failure{Error::UnsupportedFunding};
        CoinWallet::Snapshot before{SignedCounter(wallet.begin()[0]),SignedCounter(wallet.begin()[1]),SignedCounter(wallet.begin()[2]),SignedCounter(wallet.begin()[3])};
        if(!(before==request.expected_cursor)) throw Failure{Error::SnapshotMismatch};
        Checked(db,"INSERT INTO housing_escrow_balances (estate_id,copper) VALUES ("+estate+",0) ON DUPLICATE KEY UPDATE estate_id=estate_id");
        auto balances=Checked(db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+estate+" FOR UPDATE");
        if(balances.RowCount()!=1) throw Failure{Error::DatabaseFailure};
        const auto balance=std::stoll(balances.begin()[0]);
        if(balance<0) throw Failure{Error::DatabaseFailure};
        int64_t after=balance;
        if(request.action==Action::Deposit) {
            if(balance>std::numeric_limits<int64_t>::max()-plan.value.copper_amount) throw Failure{Error::Overflow};
            after+=plan.value.copper_amount;
        } else {
            if(balance<plan.value.copper_amount) throw Failure{Error::InsufficientFunds};
            after-=plan.value.copper_amount;
        }
        auto changed=Checked(db,"UPDATE character_currency SET copper_cursor="+std::to_string(plan.value.after.copper)+
            ",silver_cursor="+std::to_string(plan.value.after.silver)+",gold_cursor="+std::to_string(plan.value.after.gold)+
            ",platinum_cursor="+std::to_string(plan.value.after.platinum)+" WHERE id="+actor);
        if(changed.RowsAffected()!=1) throw Failure{Error::DatabaseFailure};
        auto escrow=Checked(db,"UPDATE housing_escrow_balances SET copper="+std::to_string(after)+" WHERE estate_id="+estate);
        if(escrow.RowsAffected()!=1) throw Failure{Error::DatabaseFailure};
        auto completed=Checked(db,"UPDATE housing_escrow_transfers SET after_copper="+std::to_string(plan.value.after.copper)+
            ",after_silver="+std::to_string(plan.value.after.silver)+",after_gold="+std::to_string(plan.value.after.gold)+
            ",after_platinum="+std::to_string(plan.value.after.platinum)+",copper_transferred="+std::to_string(plan.value.copper_amount)+
            ",escrow_before="+std::to_string(balance)+",escrow_after="+std::to_string(after)+",completed=1 WHERE request_key="+key+" AND completed=0");
        if(completed.RowsAffected()!=1) throw Failure{Error::DatabaseFailure};
        transaction.Commit();return {{request,plan.value.after,plan.value.copper_amount,balance,after,false},Error::None};
    } catch(const Failure& failure) { return {{},failure.error}; }
      catch(const std::exception&) { return {{},Error::DatabaseFailure}; }
}
}
