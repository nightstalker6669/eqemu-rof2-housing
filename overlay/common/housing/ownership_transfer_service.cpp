#include "ownership_transfer_service.h"
#include "exterior_exit_geometry.h"
#include "inventory_session_service.h"
#include "catalog_service.h"
#include "coin_pending_guard.h"
#include "../emu_constants.h"
#include "../repositories/character_currency_repository.h"
#include <openssl/sha.h>
#include <algorithm>
#include <type_traits>
namespace Housing::OwnershipTransfer {namespace {
using Bytes=std::vector<uint8_t>;
struct Failure{Error error;};
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
                if(size>255||size>bytes.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                std::string out(bytes.begin()+at,bytes.begin()+at+size);
                at+=size;
                return out;
            }
        };
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
std::string Digest(const Bytes&b){unsigned char sum[32];SHA256(b.data(),b.size(),sum);const char*h="0123456789abcdef";std::string s;for(auto x:sum){s+=h[x>>4];s+=h[x&15];}return s;}
void F32(Bytes&b,float f){uint32_t n;std::memcpy(&n,&f,4);Word(b,n,4);}float F32(Reader&r){uint32_t n=r.Word(4);float f;std::memcpy(&f,&n,4);if(!std::isfinite(f))throw Failure{Error::DatabaseFailure};return f;}
Bytes Take(Reader&r,size_t n){if(n>r.bytes.size()-r.at)throw Failure{Error::DatabaseFailure};Bytes b(r.bytes.begin()+r.at,r.bytes.begin()+r.at+n);r.at+=n;return b;}
std::string Lower(std::string s){for(auto&c:s)if(c>='A'&&c<='Z')c+=32;return s;}
void Verify(Database&db){auto r=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_ownership_state','housing_ownership_transfer_receipts','housing_estates','housing_native_estates','housing_catalogs','housing_neighborhoods','housing_plot_definitions','instance_list','housing_acl_state','housing_acl_groups','housing_acl_members','housing_acl_special_members','housing_inventory_sessions','housing_live_item_leases','character_data','account','housing_escrow_balances')");if(r.RowCount()!=1||Number(r.begin()[0])!=17)throw Failure{Error::DatabaseFailure};}
bool Shape(const Request&r){auto w=Decode(r.wire.data(),r.wire.size());return w&&Key(r.key)&&Key(r.lease.session_key)&&r.lease.epoch&&r.lease.pending_key.empty()&&r.lease.actor.character_id&&r.lease.actor.account_id&&r.recipient.actor.character_id&&r.recipient.actor.account_id&&r.recipient.runtime_entity_id&&r.lease.actor.character_id!=r.recipient.actor.character_id&&Lower(w->target_name)==Lower(r.recipient.name)&&r.source.zone_id==712&&!r.source.zone_version&&r.source.instance_id&&r.source.instance_id<=32000&&ExteriorExit::Finite(r.position);}
Bytes Binding(const Request&r){Bytes b;Word(b,1,4);Text(b,r.key);Word(b,r.lease.actor.character_id,4);Word(b,r.lease.actor.account_id,4);Word(b,r.lease.epoch,8);Text(b,r.lease.session_key);b.insert(b.end(),r.lease.token.begin(),r.lease.token.end());Word(b,r.recipient.actor.character_id,4);Word(b,r.recipient.actor.account_id,4);Word(b,r.recipient.runtime_entity_id,4);Text(b,r.recipient.name);Word(b,r.source.zone_id,4);Word(b,r.source.zone_version,1);Word(b,r.source.instance_id,2);for(auto f:r.position)F32(b,f);b.insert(b.end(),r.wire.begin(),r.wire.end());return b;}
void SourceData(Bytes&b,const Source&s){Word(b,s.estate_id,4);Word(b,s.native_plot_id,4);Word(b,s.definition_id,4);Word(b,s.reviewed_personal_limit,4);Word(b,s.catalog_revision,8);Word(b,s.acl_revision,8);Word(b,s.route.zone_id,4);Word(b,s.route.zone_version,1);Word(b,s.route.instance_id,2);Text(b,s.catalog_key);Text(b,s.definitions_sha256);Text(b,s.regions_sha256);Text(b,s.address);Word(b,s.region.id,4);for(auto f:s.region.matrix)F32(b,f);for(auto f:s.region.opaque_vector)F32(b,f);for(auto n:s.region.opaque_ids)Word(b,n,4);}
Source ReadSource(Reader&r){Source s;s.estate_id=r.Word(4);s.native_plot_id=r.Word(4);s.definition_id=r.Word(4);s.reviewed_personal_limit=r.Word(4);s.catalog_revision=r.Word(8);s.acl_revision=r.Word(8);s.route={uint32_t(r.Word(4)),uint8_t(r.Word(1)),uint16_t(r.Word(2))};s.catalog_key=r.Text();s.definitions_sha256=r.Text();s.regions_sha256=r.Text();s.address=r.Text();s.region.id=r.Word(4);for(auto&f:s.region.matrix)f=F32(r);for(auto&f:s.region.opaque_vector)f=F32(r);for(auto&n:s.region.opaque_ids)n=r.Word(4);RegionWire::Detail::ValidateMatrix(s.region.matrix);return s;}
Bytes Outcome(const Receipt&r){Bytes b;Word(b,1,4);SourceData(b,r.source);Word(b,r.old_owner,4);Word(b,r.new_owner,4);Word(b,r.ownership_revision,8);Word(b,r.acl_revision,8);Word(b,uint64_t(r.retained_copper),8);return b;}
Receipt ReadOutcome(const Request&request,const Bytes&b){Reader rd{b};if(rd.Word(4)!=1)throw Failure{Error::DatabaseFailure};Receipt r;r.request=request;r.source=ReadSource(rd);r.old_owner=rd.Word(4);r.new_owner=rd.Word(4);r.ownership_revision=rd.Word(8);r.acl_revision=rd.Word(8);r.retained_copper=int64_t(rd.Word(8));auto w=*Decode(request.wire.data(),request.wire.size());if(r.source.reviewed_personal_limit<1||r.source.reviewed_personal_limit>3||rd.at!=b.size()||!r.source.estate_id||r.source.native_plot_id!=w.native_plot_id||r.old_owner!=request.lease.actor.character_id||r.new_owner!=request.recipient.actor.character_id||!r.ownership_revision||!r.acl_revision||r.acl_revision!=r.source.acl_revision+1||r.retained_copper<0||Outcome(r)!=b)throw Failure{Error::Conflict};return r;}
std::optional<Receipt> Lookup(Database&db,const Request&r){auto rows=Query(db,"SELECT character_id,account_id,estate_id,native_plot_id,old_owner,new_owner,ownership_revision,acl_revision,HEX(binding_blob),HEX(outcome_blob) FROM housing_ownership_transfer_receipts WHERE request_key="+Quote(db,r.key)+" FOR UPDATE");if(!rows.RowCount())return {};if(rows.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto row=rows.begin();if(Number(row[0])!=r.lease.actor.character_id||Number(row[1])!=r.lease.actor.account_id||Unhex(row[8])!=Binding(r))throw Failure{Error::Conflict};auto out=ReadOutcome(r,Unhex(row[9]));if(Number(row[2])!=out.source.estate_id||Number(row[3])!=out.source.native_plot_id||Number(row[4])!=out.old_owner||Number(row[5])!=out.new_owner||std::stoull(row[6])!=out.ownership_revision||std::stoull(row[7])!=out.acl_revision)throw Failure{Error::Conflict};out.replayed=true;return out;}
Source LockedSource(Database&db,const Request&r){const auto wire=*Decode(r.wire.data(),r.wire.size());auto rows=Query(db,"SELECT n.ownership_estate_id,n.plot_id,n.definition_id,n.catalog_key,n.neighborhood_id,h.instance_id,p.definition_id,p.catalog_key,p.parent_native_id,p.active,i.zone,i.version,i.never_expires,h.zone_id,h.zone_version,d.neighborhood_id FROM housing_native_estates n JOIN housing_native_estates p ON p.native_id=n.parent_native_id JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN housing_plot_definitions d ON d.id=n.plot_id JOIN instance_list i ON i.id=h.instance_id WHERE n.native_id="+std::to_string(wire.native_plot_id)+" AND n.active=1 FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::NotFound};auto row=rows.begin();if(!row[0]||!Number(row[0])||!row[3]||!row[7]||std::string(row[3])!=row[7]||row[8]||Number(row[9])!=1||Number(row[5])!=r.source.instance_id||Number(row[10])!=712||Number(row[11])||Number(row[12])!=1||Number(row[13])!=712||Number(row[14])||Number(row[4])!=Number(row[15]))throw Failure{Error::Unauthorized};Source out;out.estate_id=Number(row[0]);out.native_plot_id=wire.native_plot_id;out.definition_id=Number(row[2]);out.catalog_key=row[3];out.route=r.source;auto root=Number(row[6]);auto access=LoadCurrentEstateAccessUnderTransaction(db,out.estate_id,out.native_plot_id,r.lease.actor);if(!access||access.value.owner_kind!=OwnerKind::Character||access.value.plot_id!=Number(row[1]))throw Failure{Error::Unauthorized};if(access.value.owner_id!=r.lease.actor.character_id)throw Failure{Error::NotPrimaryOwner};out.acl_revision=access.value.acl_revision;if(out.acl_revision==UINT64_MAX)throw Failure{Error::Conflict};auto c=Query(db,"SELECT revision,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,out.catalog_key)+" FOR UPDATE");if(c.RowCount()!=1)throw Failure{Error::Conflict};auto cr=c.begin();out.catalog_revision=std::stoull(cr[0]);auto definitions=Unhex(cr[1]),regions=Unhex(cr[2]);out.definitions_sha256=Digest(definitions);out.regions_sha256=Digest(regions);auto b=Catalog::DecodePayloads(definitions,regions);auto d=std::find_if(b.definitions.begin(),b.definitions.end(),[&](auto&d){return d.id==out.definition_id;});auto parent=std::find_if(b.definitions.begin(),b.definitions.end(),[&](auto&d){return d.id==root;});if(!out.catalog_revision||d==b.definitions.end()||d->type!=4||d->zone_id!=712||parent==b.definitions.end()||parent->type!=5||parent->zone_id!=712||std::find(parent->children.begin(),parent->children.end(),d->id)==parent->children.end())throw Failure{Error::Unauthorized};auto region=std::find_if(b.regions.begin(),b.regions.end(),[&](auto&x){return x.id==d->area_id;});if(region==b.regions.end())throw Failure{Error::Unauthorized};out.region=*region;out.address=d->address;if(!ExteriorExit::Inside(out.region,{r.position,r.position}))throw Failure{Error::Unauthorized};return out;}
int64_t Escrow(Database&db,uint32_t estate){auto cols=Query(db,"SELECT column_name FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_escrow_balances' ORDER BY ordinal_position");if(cols.RowCount()!=2||std::string(cols.begin()[0])!="estate_id")throw Failure{Error::UnsupportedEscrow};auto it=cols.begin();++it;if(std::string(it[0])!="copper")throw Failure{Error::UnsupportedEscrow};auto rows=Query(db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(rows.RowCount()>1)throw Failure{Error::UnsupportedEscrow};if(!rows.RowCount())return 0;auto row=rows.begin();if(!row[0])throw Failure{Error::UnsupportedEscrow};size_t end=0;auto value=std::stoll(row[0],&end);if(value<0||end!=std::string(row[0]).size())throw Failure{Error::UnsupportedEscrow};return value;}
Receipt Change(Database&db,const Request&r,LiveItems::InventorySessionFence&f,Authority&authority,uint32_t reviewed_limit){Verify(db);auto lease=Session(db,r.lease,true);if(auto receipt=Lookup(db,r))return *receipt;if(CoinPending::Inspect(db,r.lease.actor.character_id,true)!=CoinPending::State::Clear)throw Failure{Error::PendingApplication};if(lease.owner.zone_id!=r.source.zone_id||lease.owner.instance_id!=r.source.instance_id)throw Failure{Error::Unauthorized};auto source=LockedSource(db,r);source.reviewed_personal_limit=reviewed_limit;if(!authority.TransferFrameMatches(r,source))throw Failure{Error::UnheldFence};auto recipient=Query(db,"SELECT c.account_id,c.name,c.deleted_at,a.status FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(r.recipient.actor.character_id)+" FOR UPDATE");if(recipient.RowCount()!=1||Number(recipient.begin()[0])!=r.recipient.actor.account_id||!recipient.begin()[1]||Lower(recipient.begin()[1])!=Lower(r.recipient.name)||recipient.begin()[2]||!recipient.begin()[3]||std::stoll(recipient.begin()[3])<0)throw Failure{Error::Unauthorized};auto owned=Query(db,"SELECT id FROM housing_current_estates WHERE owner_kind=1 AND owner_id="+std::to_string(r.recipient.actor.character_id)+" FOR UPDATE");if(owned.RowCount()>=reviewed_limit)throw Failure{Error::TooManyPlots};Receipt out;out.request=r;out.source=source;out.old_owner=r.lease.actor.character_id;out.new_owner=r.recipient.actor.character_id;out.retained_copper=Escrow(db,source.estate_id);auto state=Query(db,"SELECT revision,owner_character_id FROM housing_ownership_state WHERE estate_id="+std::to_string(source.estate_id)+" FOR UPDATE");uint64_t revision=0;if(state.RowCount()){if(state.RowCount()!=1||Number(state.begin()[1])!=out.old_owner||!(revision=std::stoull(state.begin()[0]))||revision==UINT64_MAX)throw Failure{Error::Conflict};}out.ownership_revision=revision+1;out.acl_revision=source.acl_revision+1;
 auto changed=Query(db,"UPDATE housing_estates SET owner_id="+std::to_string(out.new_owner)+" WHERE id="+std::to_string(source.estate_id)+" AND active=1 AND owner_kind=1 AND owner_id="+std::to_string(out.old_owner));if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};if(!revision)Query(db,"INSERT INTO housing_ownership_state(estate_id,revision,owner_character_id) VALUES("+std::to_string(source.estate_id)+",1,"+std::to_string(out.new_owner)+")");else if(Query(db,"UPDATE housing_ownership_state SET revision=revision+1,owner_character_id="+std::to_string(out.new_owner)+" WHERE estate_id="+std::to_string(source.estate_id)+" AND revision="+std::to_string(revision)+" AND owner_character_id="+std::to_string(out.old_owner)).RowsAffected()!=1)throw Failure{Error::Conflict};
 if(!source.acl_revision){Query(db,"INSERT INTO housing_acl_state(estate_id,revision) VALUES("+std::to_string(source.estate_id)+",1)");for(unsigned i=0;i<7;++i)Query(db,"INSERT INTO housing_acl_groups(estate_id,group_id,capabilities) VALUES("+std::to_string(source.estate_id)+","+std::to_string(i)+","+std::to_string(AccessPolicy::NativeDefaults[i])+")");}else if(Query(db,"UPDATE housing_acl_state SET revision=revision+1 WHERE estate_id="+std::to_string(source.estate_id)+" AND revision="+std::to_string(source.acl_revision)).RowsAffected()!=1)throw Failure{Error::Conflict};
 Query(db,"INSERT INTO housing_ownership_transfer_receipts(request_key,character_id,account_id,estate_id,native_plot_id,old_owner,new_owner,ownership_revision,acl_revision,binding_blob,outcome_blob) VALUES("+Quote(db,r.key)+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+","+std::to_string(source.estate_id)+","+std::to_string(source.native_plot_id)+","+std::to_string(out.old_owner)+","+std::to_string(out.new_owner)+","+std::to_string(out.ownership_revision)+","+std::to_string(out.acl_revision)+",x'"+Hex(Binding(r))+"',x'"+Hex(Outcome(out))+"')");if(!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key)||!authority.TransferFrameMatches(r,source))throw Failure{Error::UnheldFence};return out;}
}
Service::Service(std::unique_ptr<Database>db,bool enabled,uint32_t reviewed_limit):db_(std::move(db)),enabled_(enabled),reviewed_personal_limit_(reviewed_limit){}
Result<Receipt> Service::Transfer(const Request&r,LiveItems::InventorySessionFence&f,Authority&a){std::lock_guard<std::mutex>g(mutex_);if(!enabled_)return {{},Error::Disabled};if(reviewed_personal_limit_<1||reviewed_personal_limit_>3)return {{},Error::UnreviewedLimit};if(!Shape(r))return {{},Error::InvalidArgument};if(!db_||!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};return Run<Receipt>(*db_,[&]{return Change(*db_,r,f,a,reviewed_personal_limit_);});}
Result<Receipt> Service::Resolve(const Request&r,LiveItems::InventorySessionFence&f){std::lock_guard<std::mutex>g(mutex_);if(!enabled_)return {{},Error::Disabled};if(!Shape(r)||!db_||!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};return Run<Receipt>(*db_,[&]{Verify(*db_);Session(*db_,r.lease,false);auto old=Lookup(*db_,r);if(!old)throw Failure{Error::NotFound};return *old;});}
}
