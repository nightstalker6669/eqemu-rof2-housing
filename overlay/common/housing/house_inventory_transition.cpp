#include "house_inventory_transition.h"
#include "custody_codec.h"
#include "coin_pending_guard.h"
#include "../database.h"
#include "../inventory_profile.h"
#include <mysqld_error.h>
#include <set>
#include <algorithm>
#include <type_traits>
namespace Housing::HouseInventory {
    namespace {
        using Row=LiveItems::InventoryRow;
        using Bytes=CustodyCodec::Bytes;
        struct Failure{
            Error error;
        };
        bool Key(const std::string&);
        [[noreturn]]void Reject(Error error){
            throw Failure{
                error
            };
        }
        MySQLRequestResult Q(Database& db,const std::string& sql){
            auto r=db.QueryDatabase(sql,false);
            if(!r.Success())Reject(Error::DatabaseFailure);
            return r;
        }
        std::string Hex(const Bytes& bytes) {
            static constexpr char alphabet[] = "0123456789abcdef";
            std::string out;
            out.reserve(bytes.size() * 2);
            for (auto byte : bytes) {
                out += alphabet[byte >> 4];
                out += alphabet[byte & 15];
            }
            return out;
        }
        Bytes Unhex(const char* input) {
            if (!input) Reject(Error::DatabaseFailure);
            const std::string text(input);
            if (text.size() % 2 || text.size() > 2 * 1024 * 1024 * 2) Reject(Error::DatabaseFailure);
            auto nibble = [](char c) -> uint8_t {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                Reject(Error::DatabaseFailure);
            };
            Bytes out;
            for (size_t at = 0; at < text.size(); at += 2)
            out.push_back(uint8_t(nibble(text[at]) * 16 + nibble(text[at + 1])));
            return out;
        }
        std::string Literal(const std::string& text) {
            return "x'" + Hex(Bytes(text.begin(), text.end())) + "'";
        }
        uint64_t Number(const char* value, uint64_t max) {
            if (!value || !*value || *value == '-') Reject(Error::DatabaseFailure);
            size_t end = 0;
            auto out = std::stoull(value, &end);
            if (end != std::string(value).size() || out > max) Reject(Error::DatabaseFailure);
            return out;
        }
        const std::string InventoryColumns = "character_id,slot_id,item_id,charges,color,augment_one,augment_two,"
        "augment_three,augment_four,augment_five,augment_six,instnodrop,HEX(custom_data),ornament_icon,"
        "ornament_idfile,ornament_hero_model,guid";
        Row InventoryRow(MySQLRequestRow row) {
            Row out;
            auto number = [&](size_t column, uint64_t max) {
                return row[column] ? Number(row[column], max) : 0;
            };
            auto& v = out.values;
            v.character_id = number(0, UINT32_MAX);
            v.slot_id = number(1, UINT32_MAX);
            v.item_id = number(2, UINT32_MAX);
            v.charges = number(3, UINT16_MAX);
            v.color = number(4, UINT32_MAX);
            v.augment_one = number(5, UINT32_MAX);
            v.augment_two = number(6, UINT32_MAX);
            v.augment_three = number(7, UINT32_MAX);
            v.augment_four = number(8, UINT32_MAX);
            v.augment_five = number(9, UINT32_MAX);
            v.augment_six = number(10, UINT32_MAX);
            v.instnodrop = number(11, UINT8_MAX);
            if (row[12]) {
                auto text = Unhex(row[12]);
                v.custom_data.assign(text.begin(), text.end());
            }
            v.ornament_icon = number(13, UINT32_MAX);
            v.ornament_idfile = number(14, UINT32_MAX);
            if (!row[15]) Reject(Error::DatabaseFailure);
            size_t end = 0;
            auto hero = std::stoll(row[15], &end);
            if (end != std::string(row[15]).size() || hero < INT32_MIN || hero > INT32_MAX) Reject(Error::DatabaseFailure);
            v.ornament_hero_model = int32_t(hero);
            v.guid = number(16, UINT64_MAX);
            if (!row[2]) out.null_fields |= LiveItems::NullItemId;
            if (!row[3]) out.null_fields |= LiveItems::NullCharges;
            if (!row[12]) out.null_fields |= LiveItems::NullCustomData;
            if (!row[16]) out.null_fields |= LiveItems::NullGuid;
            // Non-null fields must never silently become zero on malformed legacy rows.
            for (auto i : {
                0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14
            }) if (!row[i]) Reject(Error::DatabaseFailure);
            if (!CustodyCodec::Valid(out)) Reject(Error::UnsupportedItem);
            return out;
        }
        std::string InventoryWhere(const Row& row) {
            return "character_id=" + std::to_string(row.values.character_id) + " AND slot_id=" + std::to_string(row.values.slot_id);
        }
        void InventoryExact(Database& db, const Row& expected, bool absent = false) {
            auto result = Q(db, "SELECT " + InventoryColumns + " FROM inventory WHERE " + InventoryWhere(expected) + " FOR UPDATE");
            if (absent) {
                if (result.RowCount()) Reject(Error::Conflict);
                return;
            }
            if (result.RowCount() != 1 || CustodyCodec::Encode(InventoryRow(result.begin())) != CustodyCodec::Encode(expected))
            Reject(Error::Conflict);
        }
        std::string Predicate(const InventorySessions::Lease& lease){
            return "character_id="+std::to_string(lease.actor.character_id)+" AND account_id="+std::to_string(lease.actor.account_id)+" AND epoch="+std::to_string(lease.epoch)+" AND token=x'"+Hex(Bytes(lease.token.begin(),lease.token.end()))+"' AND process_key="+Literal(lease.owner.process_key)+" AND client_key="+Literal(lease.owner.client_key)+" AND zone_id="+std::to_string(lease.owner.zone_id)+" AND instance_id="+std::to_string(lease.owner.instance_id);
        }
        void Authority(Database& db,const InventorySessions::Lease& lease,SessionFence& fence,const std::string& own_key={
        }){
            if(!lease.active||!lease.actor.character_id||!lease.actor.account_id||!lease.epoch||!lease.owner.zone_id||!Key(lease.owner.process_key)||!Key(lease.owner.client_key))Reject(Error::InvalidArgument);
            if(!fence.HeldFor(lease))Reject(Error::UnheldReservation);
            const auto character=std::to_string(lease.actor.character_id);
            auto actor=Q(db,"SELECT account_id,deleted_at FROM character_data WHERE id="+character+" FOR UPDATE");
            if(actor.RowCount()!=1||Number(actor.begin()[0],UINT32_MAX)!=lease.actor.account_id||actor.begin()[1])Reject(Error::Unauthorized);
            auto account=Q(db,"SELECT id FROM account WHERE id="+std::to_string(lease.actor.account_id)+" FOR UPDATE");
            if(account.RowCount()!=1)Reject(Error::Unauthorized);
            auto optional=Q(db,"SELECT COUNT(*),SUM(engine='InnoDB') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_inventory_processes','housing_inventory_custody_links','housing_inventory_death_reconciliations')");
            auto count=Number(optional.begin()[0],3);
            if(count&&(count!=3||Number(optional.begin()[1],3)!=3))Reject(Error::DatabaseFailure);
            bool registered=false;
            if(count){
                auto process=Q(db,"SELECT reaped FROM housing_inventory_processes WHERE process_key="+Literal(lease.owner.process_key)+" FOR UPDATE");
                if(process.RowCount()>1)Reject(Error::DatabaseFailure);
                if(process.RowCount()){
                    registered=true;
                    if(Number(process.begin()[0],1))Reject(Error::UnheldReservation);
                }
            }
            auto owner=Q(db,"SELECT active FROM housing_inventory_sessions WHERE "+Predicate(lease)+" FOR UPDATE");
            auto history=Q(db,"SELECT released FROM housing_inventory_session_history WHERE "+Predicate(lease)+" FOR UPDATE");
            if(owner.RowCount()!=1||history.RowCount()!=1||Number(owner.begin()[0],1)!=1||Number(history.begin()[0],1)!=0)Reject(Error::UnheldReservation);
            const auto coin=CoinPending::Inspect(db,lease.actor.character_id,true);
            if(coin!=CoinPending::State::Clear)Reject(coin==CoinPending::State::Pending?Error::PendingApplication:Error::DatabaseFailure);
            auto custody=Q(db,"SELECT account_id,epoch,HEX(session_key),HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id="+character+" FOR UPDATE");
            if(custody.RowCount()>1)Reject(Error::DatabaseFailure);
            if(custody.RowCount()){
                auto c=custody.begin();
                if(c[5])Reject(Error::PendingApplication);
                if(Number(c[4],1)){
                    if(!count||!registered||Number(c[0],UINT32_MAX)!=lease.actor.account_id)Reject(Error::UnheldReservation);
                    auto link=Q(db,"SELECT inventory_epoch,HEX(inventory_token),HEX(process_key),HEX(client_key),account_id,HEX(custody_session),HEX(custody_token) FROM housing_inventory_custody_links WHERE character_id="+character+" AND custody_epoch="+std::string(c[1])+" FOR UPDATE");
                    if(link.RowCount()!=1)Reject(Error::UnheldReservation);
                    auto l=link.begin();
                    if(Number(l[0],UINT64_MAX)!=lease.epoch||Unhex(l[1])!=Bytes(lease.token.begin(),lease.token.end())||Unhex(l[2])!=Bytes(lease.owner.process_key.begin(),lease.owner.process_key.end())||Unhex(l[3])!=Bytes(lease.owner.client_key.begin(),lease.owner.client_key.end())||Number(l[4],UINT32_MAX)!=lease.actor.account_id||Unhex(l[5])!=Unhex(c[2])||Unhex(l[6])!=Unhex(c[3]))Reject(Error::UnheldReservation);
                }
            }
            auto receipts=Q(db,"SELECT request_key FROM housing_live_item_receipts WHERE character_id="+character+" AND applied=0 FOR UPDATE");
            if(receipts.RowCount())Reject(Error::PendingApplication);
            auto pending=Q(db,"SELECT h.house_id FROM housing_house_instances h LEFT JOIN housing_house_locations l ON l.house_id=h.house_id LEFT JOIN housing_live_item_receipts r ON r.request_key=h.pending_request_key WHERE h.pending_request_key IS NOT NULL AND (l.character_id="+character+" OR r.character_id="+character+" OR h.created_by_character_id="+character+")"+(own_key.empty()?std::string{
            }:" AND h.pending_request_key<>"+Literal(own_key))+" FOR UPDATE");
            if(pending.RowCount())Reject(Error::PendingApplication);
            auto orphan=Q(db,"SELECT h.house_id FROM housing_house_instances h LEFT JOIN housing_house_locations l ON l.house_id=h.house_id WHERE h.created_by_character_id="+character+" AND l.house_id IS NULL FOR UPDATE");
            if(orphan.RowCount())Reject(Error::Conflict);
        }
        bool Key(const std::string& s){
            return !s.empty()&&s.size()<=96;
        }
        bool Slot(uint32_t s){
            return s>=23&&s<=33;
        }
        class Tx {
            Database& db_;
            bool open_=true;
            public:explicit Tx(Database& db):db_(db){
                Q(db,"START TRANSACTION");
            }~Tx(){
                if(open_)try{
                    db_.QueryDatabase(std::string("ROLLBACK"),false);
                }catch(...){
                }
            }void Commit(){
                try{
                    if(!db_.QueryDatabase(std::string("COMMIT"),false).Success())Reject(Error::CommitUncertain);
                    open_=false;
                }catch(...){
                    Reject(Error::CommitUncertain);
                }
            }
        };
        void Schema(Database& db){
            auto r=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_inventory_sessions','housing_inventory_session_history','account','character_data','inventory','items','items_evolving_details','character_evolving_items','housing_house_instances','housing_house_locations','housing_house_interiors','housing_house_receipt_effects','housing_live_item_leases','housing_live_item_receipts','housing_house_inventory_receipts','housing_house_inventory_pending')");
            if(Number(r.begin()[0],16)!=16)Reject(Error::DatabaseFailure);
            auto columns=Q(db,"SELECT column_name FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='inventory' ORDER BY ordinal_position");
            auto expected=InventoryRepository::Columns();
            if(columns.RowCount()!=expected.size())Reject(Error::UnsupportedItem);
            size_t i=0;
            for(auto c=columns.begin();c.operator!=(columns.end());++c)if(expected[i++]!=c[0])Reject(Error::UnsupportedItem);
        }
        void Validate(const Request& r){
            if(r.request_key.rfind("houseinv:",0)!=0||r.request_key.size()<=9||!Key(r.request_key)||r.before.size()<2||r.before.size()>11||r.before.size()!=r.after.size()||r.moves.empty()||r.moves.size()>r.before.size())Reject(Error::InvalidArgument);
            std::set<uint32_t> from,to;
            std::set<int32_t> serials;
            std::set<uint64_t> houses,crates;
            size_t occupied=0;
            for(size_t i=0;i<r.before.size();++i){
                const auto& b=r.before[i];
                const auto& a=r.after[i];
                if(!Slot(b.slot)||b.slot!=a.slot||(i&&r.before[i-1].slot>=b.slot))Reject(Error::InvalidArgument);
                for(const auto* state:{
                    &b,&a
                })if(state->row&&(!CustodyCodec::Valid(*state->row)||state->row->values.character_id!=r.reservation.actor.character_id||state->row->values.slot_id!=state->slot))Reject(Error::InvalidArgument);
                occupied+=bool(b.row);
            }
            if(occupied!=r.moves.size())Reject(Error::InvalidArgument);
            for(size_t i=0;i<r.moves.size();++i){
                const auto& move=r.moves[i];
                if(((move.house_id&&move.crate_id)||((move.house_id||move.crate_id)?(!move.expected_revision||move.expected_revision==UINT64_MAX||(move.house_id?!houses.insert(move.house_id).second:!crates.insert(move.crate_id).second)):bool(move.expected_revision)))||!Slot(move.from_slot)||!Slot(move.to_slot)||move.from_slot==move.to_slot||(i&&r.moves[i-1].from_slot>=move.from_slot)||move.runtime_serial<=0||!serials.insert(move.runtime_serial).second||!from.insert(move.from_slot).second||!to.insert(move.to_slot).second)Reject(Error::InvalidArgument);
                const SlotState* before=nullptr;
                const SlotState* after=nullptr;
                for(const auto& b:r.before)if(b.slot==move.from_slot)before=&b;
                for(const auto& a:r.after)if(a.slot==move.to_slot)after=&a;
                if(!before||!after||!before->row||!after->row)Reject(Error::InvalidArgument);
                auto expected=*before->row;
                expected.values.slot_id=move.to_slot;
                if(CustodyCodec::Encode(expected)!=CustodyCodec::Encode(*after->row))Reject(Error::InvalidArgument);
            }
            if(houses.empty()&&crates.empty())Reject(Error::UnsupportedItem);
            for(const auto& b:r.before)if(b.row&&!from.count(b.slot))Reject(Error::InvalidArgument);
            for(const auto& a:r.after)if(bool(a.row)!=bool(to.count(a.slot)))Reject(Error::InvalidArgument);
        }
        void Blob(Bytes& b,const Bytes& x){
            CustodyCodec::W(b,x.size(),4);
            b.insert(b.end(),x.begin(),x.end());
        }
        void LeaseBytes(Bytes& b,const InventorySessions::Lease& l){
            CustodyCodec::W(b,l.actor.character_id,4);
            CustodyCodec::W(b,l.actor.account_id,4);
            CustodyCodec::W(b,l.epoch,8);
            b.insert(b.end(),l.token.begin(),l.token.end());
            CustodyCodec::S(b,l.owner.process_key);
            CustodyCodec::S(b,l.owner.client_key);
            CustodyCodec::W(b,l.owner.zone_id,4);
            CustodyCodec::W(b,l.owner.instance_id,2);
            CustodyCodec::W(b,l.active?1:0,1);
        }
        void StatesBytes(Bytes& b,const std::vector<SlotState>& states){
            CustodyCodec::W(b,states.size(),1);
            for(const auto& s:states){
                CustodyCodec::W(b,s.slot,4);
                CustodyCodec::W(b,s.row?1:0,1);
                if(s.row)Blob(b,CustodyCodec::Encode(*s.row));
            }
        }
        Bytes Binding(const Request& r){
            Bytes b;
            const bool crates=std::any_of(r.moves.begin(),r.moves.end(),[](const auto& m){return m.crate_id!=0;});
            CustodyCodec::W(b,crates?2:1,4);
            CustodyCodec::S(b,r.request_key);
            LeaseBytes(b,r.reservation);
            StatesBytes(b,r.before);
            StatesBytes(b,r.after);
            CustodyCodec::W(b,r.moves.size(),1);
            for(const auto& m:r.moves){
                CustodyCodec::W(b,m.house_id,8);
                CustodyCodec::W(b,m.expected_revision,8);
                CustodyCodec::W(b,m.from_slot,4);
                CustodyCodec::W(b,m.to_slot,4);
                CustodyCodec::W(b,uint32_t(m.runtime_serial),4);
                if(crates)CustodyCodec::W(b,m.crate_id,8);
            }if(b.size()>2*1024*1024)Reject(Error::InvalidArgument);
            return b;
        }
        InventorySessions::Lease ReadLease(CustodyCodec::Reader& reader){
            InventorySessions::Lease l;
            l.actor.character_id=reader.N(4);
            l.actor.account_id=reader.N(4);
            l.epoch=reader.N(8);
            for(auto& b:l.token)b=reader.N(1);
            l.owner.process_key=reader.Text(96);
            l.owner.client_key=reader.Text(96);
            l.owner.zone_id=reader.N(4);
            l.owner.instance_id=reader.N(2);
            auto active=reader.N(1);
            if(active!=1)Reject(Error::DatabaseFailure);
            l.active=true;
            return l;
        }
        std::vector<SlotState> ReadStates(CustodyCodec::Reader& reader){
            auto count=reader.N(1);
            if(count>11)Reject(Error::DatabaseFailure);
            std::vector<SlotState> states;
            while(count--){
                SlotState s;
                s.slot=reader.N(4);
                auto exists=reader.N(1);
                if(exists>1)Reject(Error::DatabaseFailure);
                if(exists)s.row=CustodyCodec::Decode(reader.Blob(CustodyCodec::MaximumPayload));
                states.push_back(std::move(s));
            }return states;
        }
        Request Unbinding(const Bytes& bytes){
            try{
                CustodyCodec::Reader reader{
                    bytes
                };
                const auto version=reader.N(4);if(version!=1&&version!=2)Reject(Error::DatabaseFailure);
                Request r;
                r.request_key=reader.Text(96);
                r.reservation=ReadLease(reader);
                r.before=ReadStates(reader);
                r.after=ReadStates(reader);
                auto count=reader.N(1);
                if(count>11)Reject(Error::DatabaseFailure);
                while(count--){
                    Move m;
                    m.house_id=reader.N(8);
                    m.expected_revision=reader.N(8);
                    m.from_slot=reader.N(4);
                    m.to_slot=reader.N(4);
                    auto serial=reader.N(4);
                    if(serial>INT32_MAX)Reject(Error::DatabaseFailure);
                    m.runtime_serial=serial;
                    if(version==2)m.crate_id=reader.N(8);
                    r.moves.push_back(m);
                }Validate(r);
                if(reader.at!=bytes.size()||Binding(r)!=bytes)Reject(Error::DatabaseFailure);
                return r;
            }catch(const Failure&){
                throw;
            }catch(...){
                Reject(Error::DatabaseFailure);
            }
        }
        void HouseBytes(Bytes& b,const HouseBinding::Binding& h){
            CustodyCodec::W(b,h.house_id,8);
            CustodyCodec::W(b,h.revision,8);
            CustodyCodec::W(b,h.item_template_id,4);
            CustodyCodec::W(b,h.location.character_id,4);
            CustodyCodec::W(b,h.location.slot_id,4);
            CustodyCodec::W(b,h.interior?1:0,1);
            if(h.interior){
                auto& i=*h.interior;
                CustodyCodec::W(b,i.native_estate_id,4);
                CustodyCodec::W(b,i.definition_id,4);
                CustodyCodec::W(b,i.zone_id,4);
                CustodyCodec::W(b,i.instance_id,2);
                CustodyCodec::W(b,i.zone_version,1);
                CustodyCodec::S(b,i.catalog_key);
                CustodyCodec::W(b,i.catalog_revision,8);
            }
        }
        HouseBinding::Binding ReadHouseBytes(CustodyCodec::Reader& reader){
            HouseBinding::Binding h;
            h.house_id=reader.N(8);
            h.revision=reader.N(8);
            h.item_template_id=reader.N(4);
            h.location={
                HouseBinding::Location::Kind::Inventory,uint32_t(reader.N(4)),uint32_t(reader.N(4)),0
            };
            auto exists=reader.N(1);
            if(exists>1)Reject(Error::DatabaseFailure);
            if(exists){
                HouseBinding::Interior i;
                i.native_estate_id=reader.N(4);
                i.definition_id=reader.N(4);
                i.zone_id=reader.N(4);
                i.instance_id=reader.N(2);
                i.zone_version=reader.N(1);
                i.catalog_key=reader.Text(96);
                i.catalog_revision=reader.N(8);
                h.interior=i;
            }return h;
        }
        Bytes Outcome(const Receipt& receipt){
            Bytes b;
            CustodyCodec::W(b,receipt.crates_before.empty()?1:2,4);
            CustodyCodec::W(b,receipt.before.size(),1);
            for(size_t i=0;i<receipt.before.size();++i){
                HouseBytes(b,receipt.before[i]);
                HouseBytes(b,receipt.after[i]);
            }
            if(!receipt.crates_before.empty()){
                if(receipt.crates_before.size()!=receipt.crates_after.size()||receipt.crates_before.size()>11)Reject(Error::DatabaseFailure);
                CustodyCodec::W(b,receipt.crates_before.size(),1);for(size_t i=0;i<receipt.crates_before.size();++i){Blob(b,CrateBinding::EncodeAllocation(receipt.crates_before[i]));Blob(b,CrateBinding::EncodeAllocation(receipt.crates_after[i]));}
            }return b;
        }
        bool HouseEqual(const HouseBinding::Binding& a,const HouseBinding::Binding& b){
            Bytes first,second;
            HouseBytes(first,a);
            HouseBytes(second,b);
            return first==second;
        }
        HouseBinding::Binding ReadHouse(Database& db,uint64_t id){
            auto r=Q(db,"SELECT h.revision,h.item_template_id,HEX(h.pending_request_key),l.location_kind,l.character_id,l.slot_id,l.custody_id FROM housing_house_instances h JOIN housing_house_locations l ON l.house_id=h.house_id WHERE h.house_id="+std::to_string(id)+" FOR UPDATE");
            if(r.RowCount()!=1)Reject(Error::NotFound);
            auto row=r.begin();
            HouseBinding::Binding h;
            h.house_id=id;
            h.revision=Number(row[0],UINT64_MAX);
            h.item_template_id=Number(row[1],UINT32_MAX);
            if(row[2]){
                auto key=Unhex(row[2]);
                h.pending_key.assign(key.begin(),key.end());
            }if(Number(row[3],2)!=1||!row[4]||!row[5]||row[6])Reject(Error::Conflict);
            h.location={
                HouseBinding::Location::Kind::Inventory,uint32_t(Number(row[4],UINT32_MAX)),uint32_t(Number(row[5],UINT32_MAX)),0
            };
            auto interior=Q(db,"SELECT native_estate_id,definition_id,zone_id,instance_id,zone_version,HEX(catalog_key),catalog_revision FROM housing_house_interiors WHERE house_id="+std::to_string(id)+" FOR UPDATE");
            if(interior.RowCount()>1)Reject(Error::DatabaseFailure);
            if(interior.RowCount()){
                auto i=interior.begin();
                HouseBinding::Interior value;
                value.native_estate_id=Number(i[0],UINT32_MAX);
                value.definition_id=Number(i[1],UINT32_MAX);
                value.zone_id=Number(i[2],UINT32_MAX);
                value.instance_id=Number(i[3],32000);
                value.zone_version=Number(i[4],UINT8_MAX);
                auto key=Unhex(i[5]);
                value.catalog_key.assign(key.begin(),key.end());
                value.catalog_revision=Number(i[6],UINT64_MAX);
                h.interior=value;
            }return h;
        }
        void InventoryStates(Database& db,uint32_t character,const std::vector<SlotState>& states){
            for(const auto& state:states){
                Row empty;
                empty.values.character_id=character;
                empty.values.slot_id=state.slot;
                InventoryExact(db,state.row?*state.row:empty,!state.row);
            }
        }
        void Locations(Database& db,const std::vector<HouseBinding::Binding>& houses,const std::vector<SlotState>& states,uint32_t character){
            for(const auto& state:states){
                auto r=Q(db,"SELECT house_id FROM housing_house_locations WHERE character_id="+std::to_string(character)+" AND slot_id="+std::to_string(state.slot)+" FOR UPDATE");
                const HouseBinding::Binding* expected=nullptr;
                for(const auto& h:houses)if(h.location.slot_id==state.slot)expected=&h;
                if(expected){
                    if(r.RowCount()!=1||Number(r.begin()[0],UINT64_MAX)!=expected->house_id)Reject(Error::Conflict);
                }else if(r.RowCount())Reject(Error::Conflict);
            }
        }
        void CrateLocations(Database& db,const std::vector<CrateBinding::Allocation>& crates,const std::vector<SlotState>& states,uint32_t character){
            if(!CrateBinding::Installed(db)){if(!crates.empty())Reject(Error::DatabaseFailure);return;}
            for(const auto& state:states){auto row=Q(db,"SELECT crate_id FROM housing_crate_locations WHERE location_kind=1 AND character_id="+std::to_string(character)+" AND slot_id="+std::to_string(state.slot)+" FOR UPDATE");
                const CrateBinding::Allocation* wanted=nullptr;for(const auto& c:crates)if(c.inventory_slot_id==state.slot)wanted=&c;
                if(wanted){if(row.RowCount()!=1||Number(row.begin()[0],UINT64_MAX)!=wanted->binding.crate_id)Reject(Error::Conflict);}else if(row.RowCount())Reject(Error::Conflict);
            }
        }
        void Simple(Database& db,const Row& row,const HouseBinding::Binding& house,const CrateBinding::Allocation* crate=nullptr){
            const auto& v=row.values;
            if((house.house_id&&crate)||(house.house_id&&v.item_id!=house.item_template_id)||(crate&&v.item_id!=crate->binding.item_template_id)||!v.item_id||v.augment_one||v.augment_two||v.augment_three||v.augment_four||v.augment_five||v.augment_six)Reject(Error::UnsupportedItem);
            auto item=Q(db,"SELECT itemclass,bagslots,stackable,UNK227,idfile FROM items WHERE id="+std::to_string(v.item_id)+" FOR UPDATE");
            if(item.RowCount()!=1||Number(item.begin()[0],UINT32_MAX)||Number(item.begin()[1],UINT32_MAX)||Number(item.begin()[2],UINT32_MAX))Reject(Error::UnsupportedItem);
            auto definition=Number(item.begin()[3],UINT32_MAX);
            if(crate){if(definition!=crate->binding.definition_id||!item.begin()[4]||!CrateGeometry::Lookup(v.item_id,uint32_t(definition),item.begin()[4]))Reject(Error::UnsupportedItem);}
            else if(house.house_id?(!definition||definition==UINT32_MAX||(house.interior&&house.interior->definition_id!=definition)):bool(definition))Reject(Error::UnsupportedItem);
            auto evolving=Q(db,"SELECT id FROM items_evolving_details WHERE item_id="+std::to_string(v.item_id)+" FOR UPDATE");
            auto active=Q(db,"SELECT id FROM character_evolving_items WHERE character_id="+std::to_string(v.character_id)+" AND item_id="+std::to_string(v.item_id)+" AND deleted_at IS NULL FOR UPDATE");
            if(evolving.RowCount()||active.RowCount())Reject(Error::UnsupportedItem);
        }
        void NoChildren(Database& db,uint32_t character,const std::vector<SlotState>& states){
            for(const auto& state:states){
                auto first=EQ::InventoryProfile::CalcSlotId(int16_t(state.slot),EQ::invbag::SLOT_BEGIN),last=EQ::InventoryProfile::CalcSlotId(int16_t(state.slot),EQ::invbag::SLOT_END);
                if(first<0||last<first)Reject(Error::UnsupportedItem);
                auto r=Q(db,"SELECT slot_id FROM inventory WHERE character_id="+std::to_string(character)+" AND slot_id BETWEEN "+std::to_string(first)+" AND "+std::to_string(last)+" FOR UPDATE");
                if(r.RowCount())Reject(Error::UnsupportedItem);
            }
        }
        std::string Values(const Row& row){
            const auto& v=row.values;
            auto nullable=[&](uint8_t flag,uint64_t value){
                return row.null_fields&flag?std::string("NULL"):std::to_string(value);
            };
            std::string out=std::to_string(v.character_id)+","+std::to_string(v.slot_id)+","+nullable(LiveItems::NullItemId,v.item_id)+","+nullable(LiveItems::NullCharges,v.charges)+","+std::to_string(v.color);
            for(auto x:{
                v.augment_one,v.augment_two,v.augment_three,v.augment_four,v.augment_five,v.augment_six
            })out+=","+std::to_string(x);
            out+=","+std::to_string(v.instnodrop)+","+(row.null_fields&LiveItems::NullCustomData?"NULL":Literal(v.custom_data))+","+std::to_string(v.ornament_icon)+","+std::to_string(v.ornament_idfile)+","+std::to_string(v.ornament_hero_model)+","+nullable(LiveItems::NullGuid,v.guid);
            return out;
        }
        struct PendingRecord{
            uint64_t epoch;
            std::string key;
        };
        std::optional<PendingRecord> Pending(Database& db,uint32_t actor){
            auto p=Q(db,"SELECT inventory_epoch,HEX(request_key) FROM housing_house_inventory_pending WHERE character_id="+std::to_string(actor)+" FOR UPDATE");
            if(p.RowCount()>1)Reject(Error::DatabaseFailure);
            if(!p.RowCount())return std::nullopt;
            if(!Number(p.begin()[0],UINT64_MAX))Reject(Error::DatabaseFailure);
            auto key=Unhex(p.begin()[1]);
            return PendingRecord{
                Number(p.begin()[0],UINT64_MAX),std::string(key.begin(),key.end())
            };
        }
        Receipt ReceiptRow(MySQLRequestRow row,const InventorySessions::Lease& lease,const std::string& key){
            Receipt out;
            out.request=Unbinding(Unhex(row[3]));
            if(Number(row[0],UINT32_MAX)!=lease.actor.character_id||Number(row[1],UINT32_MAX)!=lease.actor.account_id||Number(row[2],UINT64_MAX)!=lease.epoch||out.request.request_key!=key){
                Reject(Error::Unauthorized);
            }Bytes expected,actual;
            LeaseBytes(expected,lease);
            LeaseBytes(actual,out.request.reservation);
            if(expected!=actual)Reject(Error::UnheldReservation);
            auto bytes=Unhex(row[4]);
            try{
                CustodyCodec::Reader reader{
                    bytes
                };
                const auto version=reader.N(4);if(version!=1&&version!=2)Reject(Error::DatabaseFailure);
                auto count=reader.N(1);
                size_t house_count=0;
                for(const auto& move:out.request.moves)house_count+=bool(move.house_id);
                if(count!=house_count)Reject(Error::DatabaseFailure);
                while(count--){
                    out.before.push_back(ReadHouseBytes(reader));
                    out.after.push_back(ReadHouseBytes(reader));
                }
                size_t crate_count=0;for(const auto& m:out.request.moves)crate_count+=bool(m.crate_id);
                if(version==2){auto count=reader.N(1);if(!count||count!=crate_count||count>11)Reject(Error::DatabaseFailure);while(count--){auto b=CrateBinding::DecodeAllocation(reader.Blob(512));auto a=CrateBinding::DecodeAllocation(reader.Blob(512));if(!b||!a)Reject(Error::DatabaseFailure);out.crates_before.push_back(std::move(b.value));out.crates_after.push_back(std::move(a.value));}}
                else if(crate_count)Reject(Error::DatabaseFailure);
                if(reader.at!=bytes.size()||Outcome(out)!=bytes)Reject(Error::DatabaseFailure);
            }catch(const Failure&){
                throw;
            }catch(...){
                Reject(Error::DatabaseFailure);
            }std::vector<Move> house_moves;
            for(const auto& move:out.request.moves)if(move.house_id)house_moves.push_back(move);
            std::sort(house_moves.begin(),house_moves.end(),[](const Move& a,const Move& b){
                return a.house_id<b.house_id;
            });
            for(size_t i=0;i<house_moves.size();++i){
                const auto& move=house_moves[i];
                const auto& b=out.before[i];
                const auto& a=out.after[i];
                auto wanted=b;
                wanted.revision++;
                wanted.location.slot_id=move.to_slot;
                const Row* source=nullptr;
                for(const auto& state:out.request.before)if(state.slot==move.from_slot)source=&*state.row;
                if(!source||b.item_template_id!=source->values.item_id||b.house_id!=move.house_id||b.revision!=move.expected_revision||b.location.character_id!=lease.actor.character_id||b.location.slot_id!=move.from_slot||!HouseEqual(a,wanted))Reject(Error::DatabaseFailure);
            }
            std::vector<Move> crate_moves;for(const auto& m:out.request.moves)if(m.crate_id)crate_moves.push_back(m);
            std::sort(crate_moves.begin(),crate_moves.end(),[](const auto& a,const auto& b){return a.crate_id<b.crate_id;});
            for(size_t i=0;i<crate_moves.size();++i){const auto& m=crate_moves[i];const auto& b=out.crates_before[i];const auto& a=out.crates_after[i];
                auto expected=b;expected.binding.revision++;expected.inventory_slot_id=m.to_slot;
                const Row* source=nullptr;for(const auto& state:out.request.before)if(state.slot==m.from_slot)source=&*state.row;
                if(!source||b.binding.crate_id!=m.crate_id||b.binding.revision!=m.expected_revision||b.binding.custody_id||b.inventory_character_id!=lease.actor.character_id||b.inventory_slot_id!=m.from_slot||b.binding.item_template_id!=source->values.item_id||CrateBinding::EncodeAllocation(expected)!=CrateBinding::EncodeAllocation(a))Reject(Error::DatabaseFailure);
            }
            out.requires_application=!Number(row[5],1);
            out.replayed=true;
            return out;
        }
        const std::string ReceiptColumns="character_id,account_id,inventory_epoch,HEX(binding_blob),HEX(outcome_blob),applied";
        void Postimage(Database& db,const Receipt& receipt){
            InventoryStates(db,receipt.request.reservation.actor.character_id,receipt.request.after);
            Locations(db,receipt.after,receipt.request.after,receipt.request.reservation.actor.character_id);
            CrateLocations(db,receipt.crates_after,receipt.request.after,receipt.request.reservation.actor.character_id);
            for(const auto& expected:receipt.crates_after){const Row* source=nullptr;for(const auto& state:receipt.request.after)if(state.slot==expected.inventory_slot_id)source=&*state.row;if(!source)Reject(Error::Conflict);auto actual=CrateBinding::ReadCarriedUnderTransaction(db,*source);if(!actual||CrateBinding::EncodeAllocation(actual.value)!=CrateBinding::EncodeAllocation(expected))Reject(Error::Conflict);}
            for(const auto& expected:receipt.after){
                auto actual=ReadHouse(db,expected.house_id);
                if(!HouseEqual(expected,actual)||actual.pending_key!=receipt.request.request_key)Reject(Error::Conflict);
            }auto pending=Pending(db,receipt.request.reservation.actor.character_id);
            if(!pending||pending->key!=receipt.request.request_key||pending->epoch!=receipt.request.reservation.epoch)Reject(Error::PendingApplication);
        }
        template<class T,class F>Result<T> Run(Database& db,F body){
            try{
                Tx tx(db);
                Schema(db);
                auto result=body();
                static_assert(std::is_nothrow_move_constructible_v<Result<T>>);
                Result<T> prepared{
                    std::move(result),Error::None
                };
                tx.Commit();
                return prepared;
            }catch(const Failure& e){
                return {
                    {
                    },e.error
                };
            }catch(...){
                return {
                    {
                    },Error::DatabaseFailure
                };
            }
        }
    }
    PendingState InspectPending(Database& db,uint32_t actor,bool lock)noexcept {
        try {
            auto schema=db.QueryDatabase(std::string("SELECT COUNT(*),SUM(engine='InnoDB') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_house_inventory_pending','housing_house_inventory_receipts')"),false);
            if(!schema.Success()||schema.RowCount()!=1)return PendingState::Unavailable;
            auto count=Number(schema.begin()[0],2);
            if(!count){
                auto p=db.QueryDatabase(std::string("SELECT request_key,inventory_epoch FROM housing_house_inventory_pending WHERE 1=0"),false);
                auto r=db.QueryDatabase(std::string("SELECT request_key,character_id,inventory_epoch,applied FROM housing_house_inventory_receipts WHERE 1=0"),false);
                return !p.Success()&&!r.Success()&&p.ErrorNumber()==ER_NO_SUCH_TABLE&&r.ErrorNumber()==ER_NO_SUCH_TABLE?PendingState::Clear:PendingState::Unavailable;
            }
            if(count!=2||Number(schema.begin()[1],2)!=2)return PendingState::Unavailable;
            auto rows=db.QueryDatabase("SELECT p.inventory_epoch,HEX(p.request_key),r.character_id,r.account_id,r.inventory_epoch,r.applied FROM housing_house_inventory_pending p LEFT JOIN housing_house_inventory_receipts r ON r.request_key=p.request_key WHERE p.character_id="+std::to_string(actor)+(lock?" FOR UPDATE":""),false);
            if(!rows.Success()||rows.RowCount()>1)return PendingState::Unavailable;
            if(!rows.RowCount()){
                auto orphan=db.QueryDatabase("SELECT request_key FROM housing_house_inventory_receipts WHERE character_id="+std::to_string(actor)+" AND applied=0"+(lock?" FOR UPDATE":""),false);
                if(!orphan.Success()||orphan.RowCount())return PendingState::Unavailable;
                auto dangling=db.QueryDatabase("SELECT h.house_id FROM housing_house_instances h JOIN housing_house_inventory_receipts r17 ON r17.request_key=h.pending_request_key LEFT JOIN housing_house_receipt_effects e7 ON e7.request_key=h.pending_request_key AND e7.house_id=h.house_id LEFT JOIN housing_live_item_receipts r7 ON r7.request_key=e7.request_key WHERE r17.character_id="+std::to_string(actor)+" AND NOT (r7.character_id IS NOT NULL AND r7.character_id="+std::to_string(actor)+" AND r7.completed=1 AND r7.applied=0)"+(lock?" FOR UPDATE":""),false);
                return dangling.Success()&&!dangling.RowCount()?PendingState::Clear:PendingState::Unavailable;
            }
            auto row=rows.begin();
            auto epoch=Number(row[0],UINT64_MAX);
            auto key=Unhex(row[1]);
            if(!epoch||key.empty()||key.size()>96||Number(row[2],UINT32_MAX)!=actor||!Number(row[3],UINT32_MAX)||Number(row[4],UINT64_MAX)!=epoch||Number(row[5],1)!=0)return PendingState::Unavailable;
            return PendingState::Pending;
        }catch(...){
            return PendingState::Unavailable;
        }
    }
    Result<Receipt> ValidateRestartOutcome(Database& db,const InventorySessions::Lease& lease)noexcept {
        try {
            Schema(db);
            auto pending=Pending(db,lease.actor.character_id);
            if(!pending||pending->epoch!=lease.epoch)Reject(Error::PendingApplication);
            auto rows=Q(db,"SELECT "+ReceiptColumns+" FROM housing_house_inventory_receipts WHERE request_key="+Literal(pending->key)+" FOR UPDATE");
            if(rows.RowCount()!=1)Reject(Error::NotFound);
            auto receipt=ReceiptRow(rows.begin(),lease,pending->key);
            if(!receipt.requires_application)Reject(Error::Conflict);
            Postimage(db,receipt);
            // An orphan unapplied receipt is never silently discarded.
            auto outstanding=Q(db,"SELECT request_key FROM housing_house_inventory_receipts WHERE character_id="+std::to_string(lease.actor.character_id)+" AND applied=0 FOR UPDATE");
            if(outstanding.RowCount()!=1||!outstanding.begin()[0]||pending->key!=outstanding.begin()[0])Reject(Error::Conflict);
            return {std::move(receipt),Error::None};
        }catch(const Failure& e){return {{},e.error};}
        catch(...){return {{},Error::DatabaseFailure};}
    }
    Service::Service(std::unique_ptr<Database> db,bool enabled):database_(std::move(db)),enabled_(enabled){
    }
    Result<Receipt> Service::Relocate(const Request& request,SessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        try{
            Validate(request);
        }catch(const Failure& e){
            return {
                {
                },e.error
            };
        }catch(...){
            return {
                {
                },Error::InvalidArgument
            };
        }return Run<Receipt>(*database_,[&]{
            auto& db=*database_;Authority(db,request.reservation,fence,request.request_key);auto prior=Q(db,"SELECT "+ReceiptColumns+" FROM housing_house_inventory_receipts WHERE request_key="+Literal(request.request_key)+" FOR UPDATE");if(prior.RowCount()){
                if(prior.RowCount()!=1)Reject(Error::DatabaseFailure);auto old=ReceiptRow(prior.begin(),request.reservation,request.request_key);if(Binding(old.request)!=Binding(request))Reject(Error::Conflict);if(old.requires_application)Postimage(db,old);return old;
            }if(Pending(db,request.reservation.actor.character_id))Reject(Error::PendingApplication);
            Receipt receipt;receipt.request=request;InventoryStates(db,request.reservation.actor.character_id,request.before);NoChildren(db,request.reservation.actor.character_id,request.before);
            std::vector<Move> house_moves;for(const auto& move:request.moves)if(move.house_id)house_moves.push_back(move);std::sort(house_moves.begin(),house_moves.end(),[](const Move& a,const Move& b){
                return a.house_id<b.house_id;
            });
            for(const auto& move:house_moves){
                auto house=ReadHouse(db,move.house_id);if(!house.pending_key.empty())Reject(Error::PendingApplication);if(house.revision!=move.expected_revision||house.location.character_id!=request.reservation.actor.character_id||house.location.slot_id!=move.from_slot)Reject(Error::Conflict);const Row* source=nullptr;for(const auto& state:request.before)if(state.slot==move.from_slot)source=&*state.row;Simple(db,*source,house);receipt.before.push_back(house);auto after=house;after.revision++;after.location.slot_id=move.to_slot;after.pending_key=request.request_key;receipt.after.push_back(after);
            }
            for(const auto& move:request.moves)if(!move.house_id&&!move.crate_id){
                const Row* source=nullptr;for(const auto& state:request.before)if(state.slot==move.from_slot)source=&*state.row;Simple(db,*source,HouseBinding::Binding{
                });
            }
            std::vector<Move> crate_moves;for(const auto& m:request.moves)if(m.crate_id)crate_moves.push_back(m);
            std::sort(crate_moves.begin(),crate_moves.end(),[](const auto& a,const auto& b){return a.crate_id<b.crate_id;});
            for(const auto& m:crate_moves){const Row* source=nullptr;for(const auto& state:request.before)if(state.slot==m.from_slot)source=&*state.row;if(!source)Reject(Error::Conflict);
                auto crate=CrateBinding::ReadCarriedUnderTransaction(db,*source);if(!crate||crate.value.binding.crate_id!=m.crate_id||crate.value.binding.revision!=m.expected_revision)Reject(Error::Conflict);
                Simple(db,*source,HouseBinding::Binding{},&crate.value);
                receipt.crates_before.push_back(crate.value);crate.value.binding.revision++;crate.value.inventory_slot_id=m.to_slot;receipt.crates_after.push_back(std::move(crate.value));
            }
            CrateLocations(db,receipt.crates_before,request.before,request.reservation.actor.character_id);
            Locations(db,receipt.before,request.before,request.reservation.actor.character_id);if(!fence.BeforeMatches(request,receipt.before,receipt.crates_before))Reject(Error::UnheldReservation);
            // Remove every affected location first: intermediate uniqueness holes never
            // commit. Complete permutation is reinserted before any checked COMMIT.
            for(const auto& house:receipt.before)if(Q(db,"DELETE FROM housing_house_locations WHERE house_id="+std::to_string(house.house_id)).RowsAffected()!=1)Reject(Error::Conflict);
            for(const auto& crate:receipt.crates_before)if(Q(db,"DELETE FROM housing_crate_locations WHERE crate_id="+std::to_string(crate.binding.crate_id)+" AND location_kind=1").RowsAffected()!=1)Reject(Error::Conflict);
            for(const auto& state:request.before)if(state.row&&Q(db,"DELETE FROM inventory WHERE "+InventoryWhere(*state.row)).RowsAffected()!=1)Reject(Error::Conflict);
            for(const auto& state:request.after)if(state.row&&Q(db,"INSERT INTO inventory("+InventoryRepository::ColumnsRaw()+") VALUES ("+Values(*state.row)+")").RowsAffected()!=1)Reject(Error::DatabaseFailure);
            for(const auto& house:receipt.after){
                if(Q(db,"INSERT INTO housing_house_locations(house_id,location_kind,character_id,slot_id) VALUES ("+std::to_string(house.house_id)+",1,"+std::to_string(house.location.character_id)+","+std::to_string(house.location.slot_id)+")").RowsAffected()!=1)Reject(Error::Conflict);if(Q(db,"UPDATE housing_house_instances SET revision=revision+1,pending_request_key="+Literal(request.request_key)+" WHERE house_id="+std::to_string(house.house_id)+" AND revision="+std::to_string(house.revision-1)+" AND pending_request_key IS NULL").RowsAffected()!=1)Reject(Error::Conflict);
            }
            for(const auto& c:receipt.crates_after){
                if(Q(db,"INSERT INTO housing_crate_locations(crate_id,location_kind,character_id,slot_id) VALUES("+std::to_string(c.binding.crate_id)+",1,"+std::to_string(c.inventory_character_id)+","+std::to_string(c.inventory_slot_id)+")").RowsAffected()!=1)Reject(Error::Conflict);
                if(Q(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+std::to_string(c.binding.crate_id)+" AND revision="+std::to_string(c.binding.revision-1)).RowsAffected()!=1)Reject(Error::Conflict);
            }
            auto binding=Binding(request),outcome=Outcome(receipt);if(Q(db,"INSERT INTO housing_house_inventory_receipts(request_key,character_id,account_id,inventory_epoch,binding_blob,outcome_blob) VALUES ("+Literal(request.request_key)+","+std::to_string(request.reservation.actor.character_id)+","+std::to_string(request.reservation.actor.account_id)+","+std::to_string(request.reservation.epoch)+",x'"+Hex(binding)+"',x'"+Hex(outcome)+"')").RowsAffected()!=1)Reject(Error::Conflict);
            if(Q(db,"INSERT INTO housing_house_inventory_pending(character_id,inventory_epoch,request_key) VALUES ("+std::to_string(request.reservation.actor.character_id)+","+std::to_string(request.reservation.epoch)+","+Literal(request.request_key)+")").RowsAffected()!=1)Reject(Error::PendingApplication);Postimage(db,receipt);return receipt;
        });
    }
    Result<Receipt> Service::Resolve(const InventorySessions::Lease& lease,const std::string& key,SessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!Key(key))return {
            {
            },Error::InvalidArgument
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Receipt>(*database_,[&]{
            Authority(*database_,lease,fence,key);auto rows=Q(*database_,"SELECT "+ReceiptColumns+" FROM housing_house_inventory_receipts WHERE request_key="+Literal(key)+" FOR UPDATE");if(rows.RowCount()!=1)Reject(Error::NotFound);auto receipt=ReceiptRow(rows.begin(),lease,key);if(receipt.requires_application)Postimage(*database_,receipt);return receipt;
        });
    }
    Result<bool> Service::AcknowledgeApplied(const InventorySessions::Lease& lease,const std::string& key,SessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!Key(key))return {
            {
            },Error::InvalidArgument
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<bool>(*database_,[&]{
            auto& db=*database_;Authority(db,lease,fence,key);auto rows=Q(db,"SELECT "+ReceiptColumns+" FROM housing_house_inventory_receipts WHERE request_key="+Literal(key)+" FOR UPDATE");if(rows.RowCount()!=1)Reject(Error::NotFound);auto receipt=ReceiptRow(rows.begin(),lease,key);if(!receipt.requires_application)return true;Postimage(db,receipt);if(!fence.Applied(receipt))Reject(Error::UnheldReservation);for(const auto& house:receipt.after)if(Q(db,"UPDATE housing_house_instances SET pending_request_key=NULL WHERE house_id="+std::to_string(house.house_id)+" AND revision="+std::to_string(house.revision)+" AND pending_request_key="+Literal(key)).RowsAffected()!=1)Reject(Error::Conflict);if(Q(db,"DELETE FROM housing_house_inventory_pending WHERE character_id="+std::to_string(lease.actor.character_id)+" AND inventory_epoch="+std::to_string(lease.epoch)+" AND request_key="+Literal(key)).RowsAffected()!=1)Reject(Error::Conflict);if(Q(db,"UPDATE housing_house_inventory_receipts SET applied=1 WHERE request_key="+Literal(key)+" AND applied=0").RowsAffected()!=1)Reject(Error::Conflict);return true;
        });
    }
}
