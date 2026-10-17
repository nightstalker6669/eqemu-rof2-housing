#include "live_escrow_service.h"
#include "catalog_service.h"
#include "current_estate_access.h"
#include "scene_service.h"
#include "house_binding_service.h"
#include "coin_pending_guard.h"
#include "../emu_constants.h"
#include "../repositories/character_currency_repository.h"
#include <algorithm>
#include <map>
#include <type_traits>
namespace Housing::LiveEscrow {
    namespace {
        using Bytes=std::vector<uint8_t>;
        struct Failure {
            Error error;
        };
        MySQLRequestResult Query(Database& db,const std::string& sql){
            auto rows=db.QueryDatabase(sql,false);
            if(!rows.Success())throw Failure{
                Error::DatabaseFailure
            };
            return rows;
        }
        std::string Quote(Database& db,const std::string& value){
            return "'"+db.Escape(value)+"'";
        }
        bool Key(const std::string& value){
            if(value.empty()||value.size()>96)return false;
            for(unsigned char c:value)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==':'||c=='-'||c=='_'||c=='.'))return false;
            return true;
        }
        uint32_t Number(const char* value){
            if(!value)throw Failure{
                Error::DatabaseFailure
            };
            const auto n=std::stoull(value);
            if(n>UINT32_MAX)throw Failure{
                Error::DatabaseFailure
            };
            return uint32_t(n);
        }
        std::string Hex(const Bytes& bytes){
            static constexpr char digits[]="0123456789ABCDEF";
            std::string out;
            for(auto byte:bytes){
                out+=digits[byte>>4];
                out+=digits[byte&15];
            }
            return out;
        }
        Bytes Unhex(const char* text){
            if(!text)throw Failure{
                Error::DatabaseFailure
            };
            const std::string s=text;
            if(s.size()%2||s.size()>140000)throw Failure{
                Error::DatabaseFailure
            };
            auto nibble=[](char c)->uint8_t{
                if(c>='0'&&c<='9')return c-'0';
                if(c>='A'&&c<='F')return c-'A'+10;
                throw Failure{
                    Error::DatabaseFailure
                };
            };
            Bytes out;
            for(size_t i=0;i<s.size();i+=2)out.push_back((nibble(s[i])<<4)|nibble(s[i+1]));
            return out;
        }
        void Word(Bytes& bytes,uint64_t value,size_t width){
            for(size_t i=0;i<width;++i)bytes.push_back(uint8_t(value>>(8*i)));
        }
        void Text(Bytes& bytes,const std::string& value){
            Word(bytes,value.size(),4);
            bytes.insert(bytes.end(),value.begin(),value.end());
        }
        void Blob(Bytes& bytes,const Bytes& value){
            Word(bytes,value.size(),4);
            bytes.insert(bytes.end(),value.begin(),value.end());
        }
        struct Reader{
            const Bytes& bytes;
            size_t at=0;
            uint64_t Word(size_t width){
                if(width>bytes.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                uint64_t out=0;
                for(size_t i=0;i<width;++i)out|=uint64_t(bytes[at++])<<(8*i);
                return out;
            }
            std::string Text(){
                const auto size=Word(4);
                if(size>96||size>bytes.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                std::string out(bytes.begin()+at,bytes.begin()+at+size);
                at+=size;
                return out;
            }
        };
        std::array<uint32_t,16> Fields(const Currency& value){
            return {
                uint32_t(value.carried.platinum),uint32_t(value.carried.gold),uint32_t(value.carried.silver),uint32_t(value.carried.copper),uint32_t(value.bank.platinum),uint32_t(value.bank.gold),uint32_t(value.bank.silver),uint32_t(value.bank.copper),uint32_t(value.cursor.platinum),uint32_t(value.cursor.gold),uint32_t(value.cursor.silver),uint32_t(value.cursor.copper),value.radiant,value.career_radiant,value.ebon,value.career_ebon
            };
        }
        bool Valid(const Currency& value){
            return CoinWallet::Valid(value.carried)&&CoinWallet::Valid(value.bank)&&CoinWallet::Valid(value.cursor);
        }
        Currency CurrencyFields(const std::array<uint32_t,16>& values){
            for(size_t i=0;i<12;++i)if(values[i]>INT32_MAX)throw Failure{
                Error::SnapshotMismatch
            };
            Currency out;
            out.carried={
                int32_t(values[3]),int32_t(values[2]),int32_t(values[1]),int32_t(values[0])
            };
            out.bank={
                int32_t(values[7]),int32_t(values[6]),int32_t(values[5]),int32_t(values[4])
            };
            out.cursor={
                int32_t(values[11]),int32_t(values[10]),int32_t(values[9]),int32_t(values[8])
            };
            out.radiant=values[12];
            out.career_radiant=values[13];
            out.ebon=values[14];
            out.career_ebon=values[15];
            return out;
        }
        void CurrencyBytes(Bytes& bytes,const Currency& value){
            for(auto field:Fields(value))Word(bytes,field,4);
        }
        Currency ReadCurrency(Reader& reader){
            std::array<uint32_t,16> fields;
            for(auto& field:fields)field=uint32_t(reader.Word(4));
            return CurrencyFields(fields);
        }
        Currency LockedCurrency(Database& db,uint32_t character){
            auto rows=Query(db,"SELECT "+CharacterCurrencyRepository::ColumnsRaw()+" FROM character_currency WHERE id="+std::to_string(character)+" FOR UPDATE");
            if(rows.RowCount()!=1)throw Failure{
                Error::NotFound
            };
            std::array<uint32_t,16> fields;
            auto row=rows.begin();
            if(Number(row[0])!=character)throw Failure{
                Error::SnapshotMismatch
            };
            for(size_t i=0;i<fields.size();++i)fields[i]=Number(row[i+1]);
            return CurrencyFields(fields);
        }
        class Transaction{
            Database& db_;
            bool active_=true;
            public:explicit Transaction(Database& db):db_(db){
                Query(db_,"START TRANSACTION");
            }
            ~Transaction(){
                if(active_)try{
                    db_.QueryDatabase(std::string("ROLLBACK"),false);
                }
                catch(...) {
                }
            }
            void Commit(){
                try{
                    if(!db_.QueryDatabase(std::string("COMMIT"),false).Success())throw Failure{
                        Error::CommitUncertain
                    };
                    active_=false;
                }
                catch(...){
                    throw Failure{
                        Error::CommitUncertain
                    };
                }
            }
        };
        template<class T,class F>Result<T> Run(Database& db,F body){
            try{
                Transaction transaction(db);
                auto value=body();
                static_assert(std::is_nothrow_move_constructible_v<Result<T>>);
                Result<T> out{
                    std::move(value),Error::None
                };
                transaction.Commit();
                return out;
            }
            catch(const Failure& failure){
                return {
                    {
                    },failure.error
                };
            }
            catch(...){
                return {
                    {
                    },Error::DatabaseFailure
                };
            }
        }
        void Verify(Database& db){
            auto engines=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_inventory_sessions','housing_live_item_leases','character_data','account','character_currency','inventory','housing_estates','housing_acl_state','housing_acl_groups','housing_acl_members','housing_escrow_balances','housing_live_escrow_receipts','housing_live_escrow_pending','housing_native_estates','housing_catalogs','housing_neighborhoods','housing_plot_definitions','instance_list')");
            if(engines.RowCount()!=1||Number(engines.begin()[0])!=18)throw Failure{
                Error::DatabaseFailure
            };
            auto columns=Query(db,"SELECT column_name FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='character_currency' ORDER BY ordinal_position");
            const auto expected=CharacterCurrencyRepository::Columns();
            if(columns.RowCount()!=expected.size())throw Failure{
                Error::DatabaseFailure
            };
            size_t i=0;
            for(auto row=columns.begin();row.operator!=(columns.end());++row)if(expected[i++]!=row[0])throw Failure{
                Error::DatabaseFailure
            };
        }
        InventorySessions::Lease Session(Database& db,const LiveItems::Lease& lease,bool authenticate_actor,const std::string& aggregate_key={}){
            if(authenticate_actor){
                auto actor=Query(db,"SELECT c.account_id,c.deleted_at,a.status FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(lease.actor.character_id)+" FOR UPDATE");
                if(actor.RowCount()!=1||Number(actor.begin()[0])!=lease.actor.account_id||actor.begin()[1]||!actor.begin()[2]||std::stoll(actor.begin()[2])<0)throw Failure{
                    Error::Unauthorized
                };
            }
            auto reservation=Query(db,"SELECT account_id,epoch,HEX(token),process_key,client_key,zone_id,instance_id,active FROM housing_inventory_sessions WHERE character_id="+std::to_string(lease.actor.character_id)+" FOR UPDATE");
            if(reservation.RowCount()!=1)throw Failure{
                Error::StaleLease
            };
            auto rr=reservation.begin();
            if(Number(rr[0])!=lease.actor.account_id||Number(rr[7])!=1)throw Failure{
                Error::StaleLease
            };
            auto token=Unhex(rr[2]);
            if(token.size()!=32||!std::stoull(rr[1])||!Key(rr[3])||!Key(rr[4])||!Number(rr[5])||Number(rr[6])>32000)throw Failure{
                Error::StaleLease
            };
            InventorySessions::Lease out;
            out.actor=lease.actor;
            out.epoch=std::stoull(rr[1]);
            std::copy(token.begin(),token.end(),out.token.begin());
            out.owner={
                rr[3],rr[4],Number(rr[5]),uint16_t(Number(rr[6]))
            };
            out.active=true;
            auto current=Query(db,"SELECT account_id,epoch,session_key,HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(lease.actor.character_id)+" FOR UPDATE");
            if(current.RowCount()!=1)throw Failure{
                Error::StaleLease
            };
            auto lr=current.begin();
            if(Number(lr[0])!=lease.actor.account_id||std::stoull(lr[1])!=lease.epoch||std::string(lr[2])!=lease.session_key||Unhex(lr[3])!=Bytes(lease.token.begin(),lease.token.end())||Number(lr[4])!=1)throw Failure{
                Error::StaleLease
            };
            if(lr[5]&&(aggregate_key.empty()||std::string(lr[5])!=aggregate_key))throw Failure{
                Error::PendingApplication
            };
            return out;
        }
        void EmptyCursor(Database& db,uint32_t character){
            auto items=Query(db,"SELECT slot_id FROM inventory WHERE character_id="+std::to_string(character)+" AND (slot_id="+std::to_string(EQ::invslot::slotCursor)+" OR slot_id BETWEEN "+std::to_string(EQ::invbag::CURSOR_BAG_BEGIN)+" AND "+std::to_string(EQ::invbag::CURSOR_BAG_END)+") FOR UPDATE");
            if(items.RowCount())throw Failure{
                Error::UnheldFence
            };
        }
        const EstateWire::Definition& Definition(const Catalog::Bundle& bundle,uint32_t id){
            const auto found=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& definition){
                return definition.id==id;
            });
            if(found==bundle.definitions.end())throw Failure{
                Error::InvalidArgument
            };
            return *found;
        }
        void Estate(Database& db,const Request& request,CoinSessionFence& fence,const Scene::HouseScenePolicy* houses){
            auto domain=Query(db,"SELECT owner_kind,owner_id,plot_id FROM housing_current_estates WHERE id="+std::to_string(request.estate_id)+" FOR UPDATE");
            if(domain.RowCount()!=1)throw Failure{
                Error::NotFound
            };
            auto dr=domain.begin();
            if((Number(dr[0])!=uint32_t(OwnerKind::Character)&&Number(dr[0])!=uint32_t(OwnerKind::Guild))||!Number(dr[1]))throw Failure{
                Error::Unauthorized
            };
            const uint32_t owner_kind=Number(dr[0]),owner=Number(dr[1]),plot=Number(dr[2]);
            if(owner_kind==uint32_t(OwnerKind::Character)){
            auto owner_actor=Query(db,"SELECT c.account_id,c.deleted_at,a.id FROM character_data c LEFT JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(owner)+" FOR UPDATE");
            if(owner_actor.RowCount()!=1||!Number(owner_actor.begin()[0])||owner_actor.begin()[1]||!owner_actor.begin()[2])throw Failure{
                Error::Unauthorized
            };
            }
            auto nodes=Query(db,"SELECT catalog_key,definition_id,parent_native_id,neighborhood_id FROM housing_native_estates WHERE native_id="+std::to_string(request.wire.estate_id)+" AND ownership_estate_id="+std::to_string(request.estate_id)+" AND plot_id="+std::to_string(plot)+" AND active=1 FOR UPDATE");
            if(nodes.RowCount()!=1)throw Failure{
                Error::NotFound
            };
            auto nr=nodes.begin();
            const std::string key=nr[0];
            const auto definition_id=Number(nr[1]),parent=Number(nr[2]),neighborhood=Number(nr[3]);
            auto catalogs=Query(db,"SELECT origin,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,key)+" FOR UPDATE");
            if(catalogs.RowCount()!=1||Number(catalogs.begin()[1])!=20130510)throw Failure{
                Error::InvalidArgument
            };
            auto cr=catalogs.begin();
            if(Number(cr[0])==2){
                auto database=Query(db,"SELECT DATABASE()");
                const std::string name=database.begin()[0];
                if(name!="peq_housing_test"&&name.rfind("peq_housing_test_",0)!=0)throw Failure{
                    Error::Unauthorized
                };
            }
            else if(Number(cr[0])!=1)throw Failure{
                Error::InvalidArgument
            };
            const auto bundle=Catalog::DecodePayloads(Unhex(cr[2]),Unhex(cr[3]));
            const auto& definition=Definition(bundle,definition_id);
            if(definition.type!=(owner_kind==uint32_t(OwnerKind::Guild)?8u:4u))throw Failure{
                Error::InvalidArgument
            };
            auto roots=Query(db,"SELECT definition_id FROM housing_native_estates WHERE native_id="+std::to_string(parent)+" AND catalog_key="+Quote(db,key)+" AND neighborhood_id="+std::to_string(neighborhood)+" AND parent_native_id IS NULL AND active=1 FOR UPDATE");
            if(roots.RowCount()!=1)throw Failure{
                Error::InvalidArgument
            };
            const auto& root=Definition(bundle,Number(roots.begin()[0]));
            if(root.type!=5||std::find(root.children.begin(),root.children.end(),definition.id)==root.children.end())throw Failure{
                Error::InvalidArgument
            };
            auto routes=Query(db,"SELECT h.zone_id,h.zone_version,h.instance_id,i.zone,i.version,i.never_expires,p.neighborhood_id FROM housing_neighborhoods h JOIN instance_list i ON i.id=h.instance_id JOIN housing_plot_definitions p ON p.id="+std::to_string(plot)+" WHERE h.id="+std::to_string(neighborhood)+" FOR UPDATE");
            if(routes.RowCount()!=1)throw Failure{
                Error::UnheldFence
            };
            auto route=routes.begin();
            const auto instance=Number(route[2]);
            if(!instance||instance>32000||Number(route[0])!=Number(route[3])||Number(route[1])!=Number(route[4])||Number(route[5])!=1||Number(route[6])!=neighborhood||root.zone_id!=Number(route[0]))throw Failure{
                Error::UnheldFence
            };
            if(request.interior_source){
                const auto& source=*request.interior_source;
                if(!houses||!fence.InteriorCoinSourceMatches(source))throw Failure{Error::UnheldFence};
                auto current=Scene::Service::ReadInteriorManagementUnderTransaction(db,request.lease,source.physical,fence,*houses);
                if(!current)throw Failure{current.error==LiveItems::Error::DatabaseFailure?Error::DatabaseFailure:Error::Unauthorized};
                if(!current.value.house||!current.value.house->interior||current.value.house->house_id!=source.house_id||current.value.house->revision!=source.house_revision||current.value.house->interior->native_estate_id!=source.native_house_id||current.value.context.estate_id!=request.estate_id||current.value.context.native_plot_id!=request.wire.estate_id||current.value.context.neighborhood.zone_id!=Number(route[0])||current.value.context.neighborhood.zone_version!=Number(route[1])||current.value.context.neighborhood.instance_id!=instance)throw Failure{Error::Conflict};
            }else if(!fence.PlacementContextMatches(request.estate_id,request.wire.estate_id,uint16_t(instance)))throw Failure{Error::UnheldFence};
            if(owner_kind==uint32_t(OwnerKind::Guild)){
                // Current typed loader proves actual membership/rank AND current
                // guild leader identity; a populated GuildID alone is not authority.
                auto access=LoadCurrentEstateAccessUnderTransaction(db,request.estate_id,request.wire.estate_id,request.lease.actor);
                if(!access)throw Failure{access.error==Housing::Error::DatabaseFailure?Error::DatabaseFailure:Error::Unauthorized};
                if(access.value.owner_kind!=OwnerKind::Guild||access.value.owner_id!=owner||!access.value.effective.implicit_owner)throw Failure{Error::Unauthorized};
                return;
            }
            if(owner==request.lease.actor.character_id)return;
            if(request.wire.action==uint32_t(EscrowWire::Action::Deposit))throw Failure{
                Error::Unauthorized
            };
            auto state=Query(db,"SELECT revision FROM housing_acl_state WHERE estate_id="+std::to_string(request.estate_id)+" FOR UPDATE");
            std::array<uint32_t,7> masks=AccessPolicy::NativeDefaults;
            if(state.RowCount()>1||(state.RowCount()&&!std::stoull(state.begin()[0])))throw Failure{
                Error::DatabaseFailure
            };
            auto groups=Query(db,"SELECT group_id,capabilities FROM housing_acl_groups WHERE estate_id="+std::to_string(request.estate_id)+" ORDER BY group_id FOR UPDATE");
            if(groups.RowCount()!=(state.RowCount()?7:0))throw Failure{
                Error::DatabaseFailure
            };
            uint32_t expected=0;
            for(auto row=groups.begin();row.operator!=(groups.end());++row){
                const auto group=Number(row[0]),mask=Number(row[1]);
                if(group!=expected++||group>6||(mask&~AccessPolicy::KnownCapabilities)||(group==6&&mask))throw Failure{
                    Error::DatabaseFailure
                };
                masks[group]=mask;
            }
            auto named=Query(db,"SELECT group_id FROM housing_acl_members WHERE estate_id="+std::to_string(request.estate_id)+" AND character_id="+std::to_string(request.lease.actor.character_id)+" FOR UPDATE");
            if(named.RowCount()>1)throw Failure{
                Error::DatabaseFailure
            };
            const auto group=named.RowCount()?Number(named.begin()[0]):0;
            if(group>5||!(masks[group]&(uint32_t(1)<<uint8_t(AccessPolicy::Capability::WithdrawUpkeep))))throw Failure{
                Error::Unauthorized
            };
            // Personal guild/fellowship association remains unresolved, so no special grant
            // can elevate this narrowly supported personal cursor withdrawal.
        }
        Bytes Binding(const Request& request){
            Bytes out;
            Word(out,request.occupied_refund?3:(request.interior_source?2:1),4);
            Word(out,request.lease.actor.character_id,4);
            Word(out,request.lease.actor.account_id,4);
            Word(out,request.lease.epoch,8);
            Text(out,request.lease.session_key);
            out.insert(out.end(),request.lease.token.begin(),request.lease.token.end());
            Word(out,request.estate_id,4);
            CurrencyBytes(out,request.expected);
            Blob(out,EscrowWire::Encode(request.wire));
            if(request.interior_source){const auto& source=*request.interior_source;Word(out,source.house_id,8);Word(out,source.house_revision,8);Word(out,source.native_house_id,4);Word(out,source.physical.zone_id,4);Word(out,source.physical.zone_version,1);Word(out,source.physical.instance_id,2);}
            return out;
        }
        Bytes Outcome(const Receipt& value){
            Bytes out;
            Word(out,value.occupied_refund?2:1,4);
            Word(out,value.native_plot_id,4);
            Word(out,uint32_t(value.action),4);
            Word(out,uint8_t(value.denomination),1);
            Word(out,uint64_t(value.quantity),8);
            Word(out,uint64_t(value.copper_transferred),8);
            Word(out,uint64_t(value.escrow_before),8);
            Word(out,uint64_t(value.escrow_after),8);
            CurrencyBytes(out,value.before);
            CurrencyBytes(out,value.after);
            Word(out,value.reservation.epoch,8);
            out.insert(out.end(),value.reservation.token.begin(),value.reservation.token.end());
            Text(out,value.reservation.owner.process_key);
            Text(out,value.reservation.owner.client_key);
            Word(out,value.reservation.owner.zone_id,4);
            Word(out,value.reservation.owner.instance_id,2);
            return out;
        }
        void RefundTables(Database& db){
            auto rows=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_occupied_refund_policy','housing_occupied_refund_death_reconciliations')");
            if(rows.RowCount()!=1||Number(rows.begin()[0])!=2)throw Failure{Error::DatabaseFailure};
        }
        CoinWallet::Result<CoinWallet::Plan> ReviewRefund(const CoinWallet::Snapshot& before,int64_t quantity){
            if(!CoinWallet::Valid(before)||quantity<=0)return CoinWallet::Fail<CoinWallet::Plan>(CoinWallet::Error::InvalidAmount);
            if(quantity>INT32_MAX-int64_t(before.copper))return CoinWallet::Fail<CoinWallet::Plan>(CoinWallet::Error::Overflow);
            auto after=before;after.copper+=int32_t(quantity);
            return {{before,after,quantity},CoinWallet::Error::None};
        }
        Receipt DecodeOutcome(const Request& request,const Bytes& bytes){
            Reader reader{
                bytes
            };
            const auto version=reader.Word(4);
            if(version!=1&&version!=2)throw Failure{
                Error::DatabaseFailure
            };
            Receipt out;
            out.occupied_refund=version==2;
            if(out.occupied_refund!=request.occupied_refund)throw Failure{Error::Conflict};
            out.request_key=request.request_key;
            out.lease=request.lease;
            out.estate_id=request.estate_id;
            out.native_plot_id=uint32_t(reader.Word(4));
            out.action=EscrowWire::Action(reader.Word(4));
            out.denomination=CoinWallet::Denomination(reader.Word(1));
            out.quantity=int64_t(reader.Word(8));
            out.copper_transferred=int64_t(reader.Word(8));
            out.escrow_before=int64_t(reader.Word(8));
            out.escrow_after=int64_t(reader.Word(8));
            out.before=ReadCurrency(reader);
            out.after=ReadCurrency(reader);
            out.reservation.actor=request.lease.actor;
            out.reservation.epoch=reader.Word(8);
            for(auto& byte:out.reservation.token)byte=uint8_t(reader.Word(1));
            out.reservation.owner.process_key=reader.Text();
            out.reservation.owner.client_key=reader.Text();
            out.reservation.owner.zone_id=uint32_t(reader.Word(4));
            out.reservation.owner.instance_id=uint16_t(reader.Word(2));
            out.reservation.active=true;
            if(reader.at!=bytes.size()||out.native_plot_id!=request.wire.estate_id||out.action!=EscrowWire::Action(request.wire.action)||out.denomination!=CoinWallet::Denomination(request.wire.values[0].subtype)||out.quantity!=request.wire.values[0].amount||!(out.before==request.expected)||out.escrow_before<0||out.escrow_after<0)throw Failure{
                Error::DatabaseFailure
            };
            const auto plan=out.action==EscrowWire::Action::Deposit?CoinWallet::DepositCursor(out.before.cursor,out.denomination,out.quantity):(out.occupied_refund?ReviewRefund(out.before.carried,out.quantity):CoinWallet::WithdrawCursor(out.before.cursor,out.denomination,out.quantity));
            if(!plan||plan.value.copper_amount!=out.copper_transferred)throw Failure{
                Error::DatabaseFailure
            };
            auto expected_after=out.before;
            if(out.occupied_refund)expected_after.carried=plan.value.after;else expected_after.cursor=plan.value.after;
            if(!(expected_after==out.after))throw Failure{
                Error::DatabaseFailure
            };
            if(out.action==EscrowWire::Action::Deposit){
                if(out.escrow_before>INT64_MAX-out.copper_transferred||out.escrow_before+out.copper_transferred!=out.escrow_after)throw Failure{
                    Error::DatabaseFailure
                };
            }
            else if(out.escrow_before<out.copper_transferred||out.escrow_before-out.copper_transferred!=out.escrow_after)throw Failure{
                Error::DatabaseFailure
            };
            return out;
        }
        bool Same(const InventorySessions::Lease& a,const InventorySessions::Lease& b){
            return a.actor.character_id==b.actor.character_id&&a.actor.account_id==b.actor.account_id&&a.epoch==b.epoch&&a.token==b.token&&a.owner.process_key==b.owner.process_key&&a.owner.client_key==b.owner.client_key&&a.owner.zone_id==b.owner.zone_id&&a.owner.instance_id==b.owner.instance_id;
        }
        std::optional<Receipt> Lookup(Database& db,const Request& request,const InventorySessions::Lease& reservation){
            auto rows=Query(db,"SELECT character_id,account_id,lease_epoch,session_key,HEX(lease_token),inventory_epoch,HEX(inventory_token),process_key,client_key,estate_id,native_plot_id,HEX(binding_blob),HEX(outcome_blob),applied FROM housing_live_escrow_receipts WHERE request_key="+Quote(db,request.request_key));
            if(!rows.RowCount())return {
            };
            if(rows.RowCount()!=1)throw Failure{
                Error::DatabaseFailure
            };
            if(request.occupied_refund){
                auto paired=Query(db,"SELECT r.estate_id FROM housing_estate_retirements r JOIN housing_live_item_receipts i ON i.request_key=r.request_key AND i.character_id=r.character_id AND i.account_id=r.account_id AND i.lease_epoch=r.lease_epoch AND i.session_key=r.session_key AND i.completed=1 WHERE r.request_key="+Quote(db,request.request_key)+" AND r.estate_id="+std::to_string(request.estate_id)+" AND r.native_plot_id="+std::to_string(request.wire.estate_id)+" AND r.character_id="+std::to_string(request.lease.actor.character_id)+" AND r.account_id="+std::to_string(request.lease.actor.account_id)+" AND r.lease_epoch="+std::to_string(request.lease.epoch)+" AND r.session_key="+Quote(db,request.lease.session_key)+" AND r.lease_token=x'"+Hex(Bytes(request.lease.token.begin(),request.lease.token.end()))+"'");
                if(paired.RowCount()!=1)throw Failure{Error::Conflict};
            }
            auto row=rows.begin();
            if(Number(row[0])!=request.lease.actor.character_id||Number(row[1])!=request.lease.actor.account_id||std::stoull(row[2])!=request.lease.epoch||std::string(row[3])!=request.lease.session_key||Unhex(row[4])!=Bytes(request.lease.token.begin(),request.lease.token.end())||std::stoull(row[5])!=reservation.epoch||Unhex(row[6])!=Bytes(reservation.token.begin(),reservation.token.end())||std::string(row[7])!=reservation.owner.process_key||std::string(row[8])!=reservation.owner.client_key||Number(row[9])!=request.estate_id||Number(row[10])!=request.wire.estate_id||Unhex(row[11])!=Binding(request))throw Failure{
                Error::Conflict
            };
            auto out=DecodeOutcome(request,Unhex(row[12]));
            if(!Same(out.reservation,reservation)||Number(row[13])>1)throw Failure{
                Error::Conflict
            };
            out.replayed=true;
            out.requires_application=Number(row[13])!=1;
            auto pending=Query(db,"SELECT request_key,inventory_epoch,HEX(inventory_token),account_id FROM housing_live_escrow_pending WHERE character_id="+std::to_string(request.lease.actor.character_id)+" FOR UPDATE");
            if(pending.RowCount()>1)throw Failure{
                Error::DatabaseFailure
            };
            if(out.requires_application){
                if(pending.RowCount()!=1||std::string(pending.begin()[0])!=request.request_key||std::stoull(pending.begin()[1])!=reservation.epoch||Unhex(pending.begin()[2])!=Bytes(reservation.token.begin(),reservation.token.end())||Number(pending.begin()[3])!=reservation.actor.account_id)throw Failure{
                    Error::Conflict
                };
            }
            else if(pending.RowCount()&&std::string(pending.begin()[0])==request.request_key)throw Failure{
                Error::Conflict
            };
            return out;
        }
        bool Shape(const Request& request){
            if(request.occupied_refund&&(request.interior_source||request.wire.action!=1||request.wire.values.size()!=1||request.wire.values[0].subtype!=0))return false;
            if(request.interior_source){const auto& source=*request.interior_source;if(!source.house_id||!source.house_revision||!source.native_house_id||source.native_house_id==UINT32_MAX||!source.physical.zone_id||source.physical.zone_id==712||!source.physical.instance_id||source.physical.instance_id>32000)return false;}
            return Key(request.request_key)&&request.estate_id&&request.estate_id!=UINT32_MAX&&AccessPolicy::PositiveIdentity(request.lease.actor.character_id)&&AccessPolicy::PositiveIdentity(request.lease.actor.account_id)&&request.lease.epoch&&Key(request.lease.session_key)&&Valid(request.expected)&&EscrowWire::IsSupportedCoinRequest(request.wire)&&(request.wire.action==uint32_t(EscrowWire::Action::Deposit)||request.wire.action==uint32_t(EscrowWire::Action::Withdraw));
        }
        void CursorUpdate(Database& db,uint32_t character,const Currency& before,const Currency& after,bool occupied_refund=false){
            auto columns=CharacterCurrencyRepository::Columns();
            auto values=Fields(before);
            std::string where="id="+std::to_string(character);
            for(size_t i=0;i<values.size();++i)where+=" AND `"+columns[i+1]+"`="+std::to_string(values[i]);
            auto update=occupied_refund?Query(db,"UPDATE character_currency SET copper="+std::to_string(after.carried.copper)+" WHERE "+where):Query(db,"UPDATE character_currency SET platinum_cursor="+std::to_string(after.cursor.platinum)+",gold_cursor="+std::to_string(after.cursor.gold)+",silver_cursor="+std::to_string(after.cursor.silver)+",copper_cursor="+std::to_string(after.cursor.copper)+" WHERE "+where);
            if(update.RowsAffected()!=1)throw Failure{
                Error::SnapshotMismatch
            };
        }
        void Journal(Database& db,const Request& request,const Receipt& receipt){
            const auto& owner=receipt.reservation;
            auto saved=Query(db,"INSERT INTO housing_live_escrow_receipts(request_key,character_id,account_id,lease_epoch,session_key,lease_token,inventory_epoch,inventory_token,process_key,client_key,estate_id,native_plot_id,binding_blob,outcome_blob) VALUES ("+Quote(db,request.request_key)+","+std::to_string(request.lease.actor.character_id)+","+std::to_string(request.lease.actor.account_id)+","+std::to_string(request.lease.epoch)+","+Quote(db,request.lease.session_key)+",x'"+Hex(Bytes(request.lease.token.begin(),request.lease.token.end()))+"',"+std::to_string(owner.epoch)+",x'"+Hex(Bytes(owner.token.begin(),owner.token.end()))+"',"+Quote(db,owner.owner.process_key)+","+Quote(db,owner.owner.client_key)+","+std::to_string(request.estate_id)+","+std::to_string(request.wire.estate_id)+",x'"+Hex(Binding(request))+"',x'"+Hex(Outcome(receipt))+"')");
            if(saved.RowsAffected()!=1)throw Failure{
                Error::DatabaseFailure
            };
            auto pending=Query(db,"INSERT INTO housing_live_escrow_pending(character_id,account_id,inventory_epoch,inventory_token,request_key) VALUES ("+std::to_string(owner.actor.character_id)+","+std::to_string(owner.actor.account_id)+","+std::to_string(owner.epoch)+",x'"+Hex(Bytes(owner.token.begin(),owner.token.end()))+"',"+Quote(db,request.request_key)+")");
            if(pending.RowsAffected()!=1)throw Failure{
                Error::DatabaseFailure
            };
        }
        Receipt TransferBody(Database& db,const Request& request,CoinSessionFence& fence,const Scene::HouseScenePolicy* houses){
            Verify(db);auto old_key=Query(db,"SELECT request_key FROM housing_live_escrow_receipts WHERE request_key="+Quote(db,request.request_key));if(old_key.RowCount()){
                const auto reservation=Session(db,request.lease,false,request.occupied_refund?request.request_key:std::string{});if(auto prior=Lookup(db,request,reservation))return *prior;throw Failure{
                    Error::Conflict
                };
            }
            auto estate_lock=Query(db,"SELECT id FROM housing_current_estates WHERE id="+std::to_string(request.estate_id)+" FOR UPDATE");if(estate_lock.RowCount()!=1)throw Failure{
                Error::NotFound
            };const auto reservation=Session(db,request.lease,true);if(request.interior_source&&(reservation.owner.zone_id!=request.interior_source->physical.zone_id||reservation.owner.instance_id!=request.interior_source->physical.instance_id))throw Failure{Error::UnheldFence};const auto pending=CoinPending::Inspect(db,request.lease.actor.character_id,true);if(pending!=CoinPending::State::Clear)throw Failure{
                pending==CoinPending::State::Pending?Error::PendingApplication:Error::DatabaseFailure
            };if(!fence.CurrencyMatches(request.expected)||!fence.CursorItemsEmpty())throw Failure{
                Error::UnheldFence
            };EmptyCursor(db,request.lease.actor.character_id);Estate(db,request,fence,houses);const auto before=LockedCurrency(db,request.lease.actor.character_id);if(!(before==request.expected))throw Failure{
                Error::SnapshotMismatch
            };const auto denomination=CoinWallet::Denomination(request.wire.values[0].subtype);const auto quantity=request.wire.values[0].amount;const auto plan=request.wire.action==uint32_t(EscrowWire::Action::Deposit)?CoinWallet::DepositCursor(before.cursor,denomination,quantity):(request.occupied_refund?ReviewRefund(before.carried,quantity):CoinWallet::WithdrawCursor(before.cursor,denomination,quantity));if(!plan)throw Failure{
                plan.error==CoinWallet::Error::Overflow?Error::Overflow:plan.error==CoinWallet::Error::InsufficientFunds?Error::InsufficientFunds:Error::InvalidArgument
            };Query(db,"INSERT IGNORE INTO housing_escrow_balances(estate_id,copper) VALUES ("+std::to_string(request.estate_id)+",0)");auto balances=Query(db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+std::to_string(request.estate_id)+" FOR UPDATE");if(balances.RowCount()!=1||!balances.begin()[0])throw Failure{
                Error::DatabaseFailure
            };const auto balance=std::stoll(balances.begin()[0]);if(balance<0)throw Failure{
                Error::DatabaseFailure
            };if(request.occupied_refund&&balance!=plan.value.copper_amount)throw Failure{Error::Conflict};int64_t after_balance;if(request.wire.action==uint32_t(EscrowWire::Action::Deposit)){
                if(balance>INT64_MAX-plan.value.copper_amount)throw Failure{
                    Error::Overflow
                };after_balance=balance+plan.value.copper_amount;
            }
            else{
                if(balance<plan.value.copper_amount)throw Failure{
                    Error::InsufficientFunds
                };after_balance=balance-plan.value.copper_amount;
            }
            Receipt out;out.occupied_refund=request.occupied_refund;out.request_key=request.request_key;out.lease=request.lease;out.reservation=reservation;out.estate_id=request.estate_id;out.native_plot_id=request.wire.estate_id;out.action=EscrowWire::Action(request.wire.action);out.denomination=denomination;out.quantity=quantity;out.copper_transferred=plan.value.copper_amount;out.escrow_before=balance;out.escrow_after=after_balance;out.before=before;out.after=before;if(request.occupied_refund)out.after.carried=plan.value.after;else out.after.cursor=plan.value.after;CursorUpdate(db,request.lease.actor.character_id,before,out.after,request.occupied_refund);auto updated=Query(db,"UPDATE housing_escrow_balances SET copper="+std::to_string(after_balance)+" WHERE estate_id="+std::to_string(request.estate_id)+" AND copper="+std::to_string(balance));if(updated.RowsAffected()!=1)throw Failure{
                Error::Conflict
            };Journal(db,request,out);return out;
        }
    }
    Service::Service(std::unique_ptr<Database> db,bool enabled,const Scene::HouseScenePolicy* houses):database_(std::move(db)),enabled_(enabled),houses_(houses){
    }
    Result<Receipt> Service::Transfer(const Request& request,CoinSessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!Shape(request)||request.occupied_refund)return {
            {
            },Error::InvalidArgument
        };
        if(!fence.HeldFor(request.lease.actor,request.lease.epoch,request.lease.session_key))return {
            {
            },Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Receipt>(*database_,[&]{
            return TransferBody(*database_,request,fence,houses_);
        });
    }
    Result<Receipt> Service::RefundOccupiedUnderTransaction(Database& db,const Request& r,CoinSessionFence& fence,const Scene::HouseScenePolicy* houses){
        if(!r.occupied_refund||!Shape(r)||r.interior_source||r.wire.action!=uint32_t(EscrowWire::Action::Withdraw)||r.wire.values[0].subtype!=0)return {{},Error::InvalidArgument};
        if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};
        try{
            RefundTables(db);
            auto tx=Query(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||Number(tx.begin()[0])!=1)return {{},Error::InvalidArgument};
            // No seed/default: neither installing schema nor receiving Sale enables a wallet choice.
            auto policy=Query(db,"SELECT p.destination_mode FROM housing_occupied_refund_policy p JOIN housing_current_estates e ON e.id=p.estate_id JOIN housing_native_estates n ON n.ownership_estate_id=e.id AND n.native_id=p.native_plot_id JOIN housing_catalogs catalog ON catalog.catalog_key=n.catalog_key WHERE p.estate_id="+std::to_string(r.estate_id)+" AND p.native_plot_id="+std::to_string(r.wire.estate_id)+" AND p.catalog_revision=catalog.revision AND p.enabled=1 AND e.owner_kind=1 AND e.owner_id="+std::to_string(r.lease.actor.character_id)+" FOR UPDATE");
            if(policy.RowCount()!=1||Number(policy.begin()[0])!=1)return {{},Error::Unauthorized};
            return {TransferBody(db,r,fence,houses),Error::None};
        }catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
    }
    Result<Receipt> Service::ValidateOccupiedRefundRestartUnderTransaction(Database& db,const InventorySessions::Lease& owner,const LiveItems::Lease& item){
        try{
            RefundTables(db);
            auto tx=Query(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||Number(tx.begin()[0])!=1)return {{},Error::InvalidArgument};
            auto proof=Query(db,"SELECT process_key FROM housing_inventory_processes WHERE process_key="+Quote(db,owner.owner.process_key)+" AND reaped=1 AND child_pid>0 AND exit_code IS NOT NULL AND reaped_at IS NOT NULL FOR UPDATE");if(proof.RowCount()!=1)return {{},Error::Unauthorized};
            auto rows=Query(db,"SELECT r.request_key,r.estate_id,r.native_plot_id,HEX(r.outcome_blob) FROM housing_live_escrow_pending p JOIN housing_live_escrow_receipts r ON r.request_key=p.request_key WHERE p.character_id="+std::to_string(item.actor.character_id)+" AND r.applied=0 FOR UPDATE");if(rows.RowCount()!=1)return {{},Error::Conflict};
            auto row=rows.begin();Request request;request.request_key=row[0];request.lease=item;request.estate_id=Number(row[1]);request.wire.estate_id=Number(row[2]);request.occupied_refund=true;
            const auto bytes=Unhex(row[3]);Reader reader{bytes};if(reader.Word(4)!=2||reader.Word(4)!=request.wire.estate_id)return {{},Error::Conflict};
            request.wire.action=uint32_t(reader.Word(4));const auto denomination=reader.Word(1);const auto quantity=int64_t(reader.Word(8));reader.Word(8);reader.Word(8);reader.Word(8);request.expected=ReadCurrency(reader);request.wire.values={{0,int32_t(denomination),quantity}};
            if(!Shape(request)||request.wire.action!=1||denomination!=0||(!item.pending_key.empty()&&item.pending_key!=request.request_key))return {{},Error::Conflict};
            const auto reservation=Session(db,item,false,request.request_key);if(!Same(reservation,owner))return {{},Error::StaleLease};
            auto prior=Lookup(db,request,reservation);if(!prior||!prior->requires_application||prior->escrow_after!=0||!(LockedCurrency(db,item.actor.character_id)==prior->after))return {{},Error::Conflict};
            auto pending=item;pending.pending_key=request.request_key;
            auto actual=LiveItems::Service::ValidateRestartOutcome(db,pending,true);
            if(!actual||actual.value.action!=LiveItems::Action::Return||!actual.value.crate_allocation||actual.value.stored.estate_id!=request.estate_id)return {{},Error::Conflict};
            if(item.pending_key.empty()&&actual.value.requires_application)return {{},Error::Conflict};
            return {*prior,Error::None};
        }catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
    }
    Result<Receipt> Service::Resolve(const Request& request,CoinSessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!Shape(request))return {
            {
            },Error::InvalidArgument
        };
        if(!fence.HeldFor(request.lease.actor,request.lease.epoch,request.lease.session_key))return {
            {
            },Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Receipt>(*database_,[&]{
            auto& db=*database_;Verify(db);const auto reservation=Session(db,request.lease,false,request.occupied_refund?request.request_key:std::string{});const auto prior=Lookup(db,request,reservation);if(!prior)throw Failure{
                Error::NotFound
            };return *prior;
        });
    }
    Result<bool> Service::AcknowledgeApplied(const Request& request,CoinSessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!Shape(request))return {
            {
            },Error::InvalidArgument
        };
        if(!fence.HeldFor(request.lease.actor,request.lease.epoch,request.lease.session_key))return {
            {
            },Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<bool>(*database_,[&]{
            auto& db=*database_;Verify(db);const auto reservation=Session(db,request.lease,false,request.occupied_refund?request.request_key:std::string{});const auto prior=Lookup(db,request,reservation);if(!prior)throw Failure{
                Error::NotFound
            };if(!prior->requires_application)return true;
            if(request.occupied_refund){auto paired=Query(db,"SELECT applied FROM housing_live_item_receipts WHERE request_key="+Quote(db,request.request_key)+" FOR UPDATE");if(paired.RowCount()!=1||Number(paired.begin()[0])!=1)throw Failure{Error::PendingApplication};}
            if(!fence.CoinApplied(*prior)||!fence.CurrencyMatches(prior->after)||!fence.CursorItemsEmpty())throw Failure{
                Error::UnheldFence
            };if(!(LockedCurrency(db,request.lease.actor.character_id)==prior->after))throw Failure{
                Error::SnapshotMismatch
            };auto applied=Query(db,"UPDATE housing_live_escrow_receipts SET applied=1 WHERE request_key="+Quote(db,request.request_key)+" AND applied=0");auto cleared=Query(db,"DELETE FROM housing_live_escrow_pending WHERE character_id="+std::to_string(request.lease.actor.character_id)+" AND request_key="+Quote(db,request.request_key)+" AND inventory_epoch="+std::to_string(reservation.epoch)+" AND inventory_token=x'"+Hex(Bytes(reservation.token.begin(),reservation.token.end()))+"'");if(applied.RowsAffected()!=1||cleared.RowsAffected()!=1)throw Failure{
                Error::Conflict
            };return true;
        });
    }
}
