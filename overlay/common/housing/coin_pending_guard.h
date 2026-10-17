#ifndef EQEMU_HOUSING_COIN_PENDING_GUARD_H
#define EQEMU_HOUSING_COIN_PENDING_GUARD_H
#include "../database.h"
#include <mysqld_error.h>
namespace Housing::CoinPending {
    enum class State {
        Clear, Pending, Unavailable
    };
    // Optional absence preserves the pre-economy path. Query/schema failure never
    // proves absence. Call with lock=true only in the exclusive caller transaction.
    inline State InspectEscrow(Database& db,uint32_t character,bool lock=false) noexcept {
        try {
            auto schema=db.QueryDatabase(std::string("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_escrow_pending'"),false);
            if(!schema.Success())return State::Unavailable;
            if(!schema.RowCount()){
                auto probe=db.QueryDatabase(std::string("SELECT request_key,inventory_epoch FROM housing_live_escrow_pending WHERE 1=0"),false);
                return !probe.Success()&&probe.ErrorNumber()==ER_NO_SUCH_TABLE?State::Clear:State::Unavailable;
            }
            if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB")return State::Unavailable;
            auto rows=db.QueryDatabase("SELECT request_key,inventory_epoch FROM housing_live_escrow_pending WHERE character_id="+std::to_string(character)+(lock?" FOR UPDATE":""),false);
            if(!rows.Success()||rows.RowCount()>1)return State::Unavailable;
            if(!rows.RowCount())return State::Clear;
            auto row=rows.begin();
            if(!row[0]||!row[1]||std::string(row[0]).empty()||std::stoull(row[1])==0)return State::Unavailable;
            return State::Pending;
        }
        catch(...){
            return State::Unavailable;
        }
    }
    inline State InspectPurchase(Database& db,uint32_t character,bool lock=false) noexcept {
      try {auto schema=db.QueryDatabase(std::string("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_native_plot_purchase_pending'"),false);if(!schema.Success())return State::Unavailable;if(!schema.RowCount()){auto probe=db.QueryDatabase(std::string("SELECT request_key,inventory_epoch FROM housing_native_plot_purchase_pending WHERE 1=0"),false);return !probe.Success()&&probe.ErrorNumber()==ER_NO_SUCH_TABLE?State::Clear:State::Unavailable;}if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB")return State::Unavailable;auto rows=db.QueryDatabase("SELECT request_key,inventory_epoch FROM housing_native_plot_purchase_pending WHERE character_id="+std::to_string(character)+(lock?" FOR UPDATE":""),false);if(!rows.Success()||rows.RowCount()>1)return State::Unavailable;if(!rows.RowCount())return State::Clear;return rows.begin()[0]&&rows.begin()[1]&&!std::string(rows.begin()[0]).empty()&&std::stoull(rows.begin()[1])?State::Pending:State::Unavailable;}catch(...){return State::Unavailable;}
    }
    inline State Inspect(Database& db,uint32_t character,bool lock=false) noexcept {auto a=InspectEscrow(db,character,lock),b=InspectPurchase(db,character,lock);if(a==State::Unavailable||b==State::Unavailable)return State::Unavailable;return a==State::Pending||b==State::Pending?State::Pending:State::Clear;}

}
#endif
