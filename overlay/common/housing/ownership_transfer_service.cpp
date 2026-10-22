#include "ownership_transfer_service.h"
#include "exterior_exit_geometry.h"
#include "inventory_session_service.h"
#include "catalog_service.h"
#include "coin_pending_guard.h"
#include "custody_codec.h"
#include "foreign_furniture_return_policy.h"
#include "../emu_constants.h"
#include "../repositories/character_currency_repository.h"
#include <openssl/sha.h>
#include <algorithm>
#include <type_traits>
#include <set>
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
            if(s.size()%2||s.size()>2*CustodyCodec::MaximumPayload)throw Failure{
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
void SourceData(Bytes&b,const Source&s){Word(b,s.estate_id,4);Word(b,s.native_plot_id,4);Word(b,s.definition_id,4);Word(b,s.reviewed_personal_limit,4);Word(b,s.catalog_revision,8);Word(b,s.acl_revision,8);Word(b,s.route.zone_id,4);Word(b,s.route.zone_version,1);Word(b,s.route.instance_id,2);Text(b,s.catalog_key);Text(b,s.definitions_sha256);Text(b,s.regions_sha256);Text(b,s.address);if(!s.contents_sha256.empty())Text(b,s.contents_sha256);Word(b,s.region.id,4);for(auto f:s.region.matrix)F32(b,f);for(auto f:s.region.opaque_vector)F32(b,f);for(auto n:s.region.opaque_ids)Word(b,n,4);}
Source ReadSource(Reader&r,bool contents){Source s;s.estate_id=r.Word(4);s.native_plot_id=r.Word(4);s.definition_id=r.Word(4);s.reviewed_personal_limit=r.Word(4);s.catalog_revision=r.Word(8);s.acl_revision=r.Word(8);s.route={uint32_t(r.Word(4)),uint8_t(r.Word(1)),uint16_t(r.Word(2))};s.catalog_key=r.Text();s.definitions_sha256=r.Text();s.regions_sha256=r.Text();s.address=r.Text();if(contents){s.contents_sha256=r.Text();if(s.contents_sha256.size()!=64||!std::all_of(s.contents_sha256.begin(),s.contents_sha256.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))throw Failure{Error::DatabaseFailure};}s.region.id=r.Word(4);for(auto&f:s.region.matrix)f=F32(r);for(auto&f:s.region.opaque_vector)f=F32(r);for(auto&n:s.region.opaque_ids)n=r.Word(4);RegionWire::Detail::ValidateMatrix(s.region.matrix);return s;}
Bytes Outcome(const Receipt&r){Bytes b;Word(b,r.source.contents_sha256.empty()?1:2,4);SourceData(b,r.source);Word(b,r.old_owner,4);Word(b,r.new_owner,4);Word(b,r.ownership_revision,8);Word(b,r.acl_revision,8);Word(b,uint64_t(r.retained_copper),8);return b;}
Receipt ReadOutcome(const Request&request,const Bytes&b){Reader rd{b};const auto version=rd.Word(4);if(version!=1&&version!=2)throw Failure{Error::DatabaseFailure};Receipt r;r.request=request;r.source=ReadSource(rd,version==2);r.old_owner=rd.Word(4);r.new_owner=rd.Word(4);r.ownership_revision=rd.Word(8);r.acl_revision=rd.Word(8);r.retained_copper=int64_t(rd.Word(8));auto w=*Decode(request.wire.data(),request.wire.size());if(r.source.reviewed_personal_limit<1||r.source.reviewed_personal_limit>3||rd.at!=b.size()||!r.source.estate_id||r.source.native_plot_id!=w.native_plot_id||r.old_owner!=request.lease.actor.character_id||r.new_owner!=request.recipient.actor.character_id||!r.ownership_revision||!r.acl_revision||r.acl_revision!=r.source.acl_revision+1||r.retained_copper<0||Outcome(r)!=b)throw Failure{Error::Conflict};return r;}
std::optional<Receipt> Lookup(Database&db,const Request&r){auto rows=Query(db,"SELECT character_id,account_id,estate_id,native_plot_id,old_owner,new_owner,ownership_revision,acl_revision,HEX(binding_blob),HEX(outcome_blob) FROM housing_ownership_transfer_receipts WHERE request_key="+Quote(db,r.key)+" FOR UPDATE");if(!rows.RowCount())return {};if(rows.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto row=rows.begin();if(Number(row[0])!=r.lease.actor.character_id||Number(row[1])!=r.lease.actor.account_id||Unhex(row[8])!=Binding(r))throw Failure{Error::Conflict};auto out=ReadOutcome(r,Unhex(row[9]));if(Number(row[2])!=out.source.estate_id||Number(row[3])!=out.source.native_plot_id||Number(row[4])!=out.old_owner||Number(row[5])!=out.new_owner||std::stoull(row[6])!=out.ownership_revision||std::stoull(row[7])!=out.acl_revision)throw Failure{Error::Conflict};out.replayed=true;return out;}
Source LockedSource(Database&db,const Request&r){const auto wire=*Decode(r.wire.data(),r.wire.size());auto rows=Query(db,"SELECT n.ownership_estate_id,n.plot_id,n.definition_id,n.catalog_key,n.neighborhood_id,h.instance_id,p.definition_id,p.catalog_key,p.parent_native_id,p.active,i.zone,i.version,i.never_expires,h.zone_id,h.zone_version,d.neighborhood_id FROM housing_native_estates n JOIN housing_native_estates p ON p.native_id=n.parent_native_id JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN housing_plot_definitions d ON d.id=n.plot_id JOIN instance_list i ON i.id=h.instance_id WHERE n.native_id="+std::to_string(wire.native_plot_id)+" AND n.active=1 FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::NotFound};auto row=rows.begin();if(!row[0]||!Number(row[0])||!row[3]||!row[7]||std::string(row[3])!=row[7]||row[8]||Number(row[9])!=1||Number(row[5])!=r.source.instance_id||Number(row[10])!=712||Number(row[11])||Number(row[12])!=1||Number(row[13])!=712||Number(row[14])||Number(row[4])!=Number(row[15]))throw Failure{Error::Unauthorized};Source out;out.estate_id=Number(row[0]);out.native_plot_id=wire.native_plot_id;out.definition_id=Number(row[2]);out.catalog_key=row[3];out.route=r.source;auto root=Number(row[6]);auto access=LoadCurrentEstateAccessUnderTransaction(db,out.estate_id,out.native_plot_id,r.lease.actor);if(!access||access.value.owner_kind!=OwnerKind::Character||access.value.plot_id!=Number(row[1]))throw Failure{Error::Unauthorized};if(access.value.owner_id!=r.lease.actor.character_id)throw Failure{Error::NotPrimaryOwner};out.acl_revision=access.value.acl_revision;if(out.acl_revision==UINT64_MAX)throw Failure{Error::Conflict};auto c=Query(db,"SELECT revision,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,out.catalog_key)+" FOR UPDATE");if(c.RowCount()!=1)throw Failure{Error::Conflict};auto cr=c.begin();out.catalog_revision=std::stoull(cr[0]);auto definitions=Unhex(cr[1]),regions=Unhex(cr[2]);out.definitions_sha256=Digest(definitions);out.regions_sha256=Digest(regions);auto b=Catalog::DecodePayloads(definitions,regions);auto d=std::find_if(b.definitions.begin(),b.definitions.end(),[&](auto&d){return d.id==out.definition_id;});auto parent=std::find_if(b.definitions.begin(),b.definitions.end(),[&](auto&d){return d.id==root;});if(!out.catalog_revision||d==b.definitions.end()||d->type!=4||d->zone_id!=712||parent==b.definitions.end()||parent->type!=5||parent->zone_id!=712||std::find(parent->children.begin(),parent->children.end(),d->id)==parent->children.end())throw Failure{Error::Unauthorized};auto region=std::find_if(b.regions.begin(),b.regions.end(),[&](auto&x){return x.id==d->area_id;});if(region==b.regions.end())throw Failure{Error::Unauthorized};out.region=*region;out.address=d->address;if(!ExteriorExit::Inside(out.region,{r.position,r.position}))throw Failure{Error::Unauthorized};return out;}
int64_t Escrow(Database&db,uint32_t estate){auto cols=Query(db,"SELECT column_name FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_escrow_balances' ORDER BY ordinal_position");if(cols.RowCount()!=2||std::string(cols.begin()[0])!="estate_id")throw Failure{Error::UnsupportedEscrow};auto it=cols.begin();++it;if(std::string(it[0])!="copper")throw Failure{Error::UnsupportedEscrow};auto rows=Query(db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(rows.RowCount()>1)throw Failure{Error::UnsupportedEscrow};if(!rows.RowCount())return 0;auto row=rows.begin();if(!row[0])throw Failure{Error::UnsupportedEscrow};size_t end=0;auto value=std::stoll(row[0],&end);if(value<0||end!=std::string(row[0]).size())throw Failure{Error::UnsupportedEscrow};return value;}
// Owner-only property transfer: retained creator/item owners, poses, contents,
// house/crate identities and permanent routes never change in this transaction.
// The consent source binds a current complete graph, not just a visible scene.
void TransferContents(Database& db,const Request& request,Source& source,
 LiveItems::InventorySessionFence& fence,Authority& authority){
 const auto estate=std::to_string(source.estate_id);
 auto engines=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_live_item_custody','housing_house_locations','housing_house_instances','housing_crate_locations','housing_crate_instances','items','housing_live_item_storage','housing_live_placed_items','housing_house_item_scopes','housing_crate_members','housing_house_interiors','housing_live_vault_items')");
 if(engines.RowCount()!=1||Number(engines.begin()[0])!=12)throw Failure{Error::Unavailable};
 Bytes state;Word(state,1,4);
 // Every selected value is text or explicit HEX; NULL is distinct from empty.
 auto retain=[&](const std::string& name,const std::string& sql){
  Text(state,name);auto rows=Query(db,sql);Word(state,rows.RowCount(),4);Word(state,rows.ColumnCount(),4);
  if(rows.RowCount()>10000)throw Failure{Error::UnsupportedContent};
  for(auto row=rows.begin();row!=rows.end();++row)for(uint32_t i=0;i<rows.ColumnCount();++i){Word(state,row[i]?1:0,1);if(row[i])Text(state,row[i]);}
 };
 auto rows=Query(db,"SELECT c.custody_id,c.owner_character_id,c.schema_descriptor,HEX(c.row_blob),h.item_template_id,k.item_template_id FROM housing_live_item_custody c LEFT JOIN housing_house_locations hl ON hl.location_kind=2 AND hl.custody_id=c.custody_id LEFT JOIN housing_house_instances h ON h.house_id=hl.house_id LEFT JOIN housing_crate_locations kl ON kl.location_kind=2 AND kl.custody_id=c.custody_id LEFT JOIN housing_crate_instances k ON k.crate_id=kl.crate_id WHERE c.estate_id="+estate+" ORDER BY c.custody_id FOR UPDATE");
 if(rows.RowCount()>10000)throw Failure{Error::UnsupportedContent};
 std::set<uint32_t> creators;std::set<uint64_t> custody;
 for(auto row=rows.begin();row!=rows.end();++row){
  if(!row[2]||std::string(row[2])!=LiveItems::Service::SchemaDescriptor())throw Failure{Error::UnsupportedContent};
  auto raw=Unhex(row[3]);auto decoded=CustodyCodec::Decode(raw);const auto& item=decoded.values;
  if(!row[0]||!custody.insert(std::stoull(row[0])).second||!item.item_id||!item.character_id||item.character_id!=Number(row[1])||CustodyCodec::Encode(decoded)!=raw)throw Failure{Error::Conflict};
  creators.insert(item.character_id);
  if(row[4]&&row[5])throw Failure{Error::Conflict};
  if(!row[4]&&!row[5])continue;
  if(Number(row[4]?row[4]:row[5])!=item.item_id)throw Failure{Error::Conflict};
  auto metadata=Query(db,"SELECT nodrop,notransfer,itemclass,bagslots FROM items WHERE id="+std::to_string(item.item_id)+" FOR UPDATE");
  if(metadata.RowCount()!=1||!metadata.begin()[0]||!metadata.begin()[1]||!metadata.begin()[2]||!metadata.begin()[3])throw Failure{Error::Unavailable};
  if(!ForeignFurnitureReturn::NormalMode())throw Failure{Error::UnsupportedContent};
  if(Number(metadata.begin()[0])==0||item.instnodrop)throw Failure{Error::NoTradeHouseOrCrate};
  if(Number(metadata.begin()[1])||Number(metadata.begin()[2])||Number(metadata.begin()[3])||item.augment_one||item.augment_two||item.augment_three||item.augment_four||item.augment_five||item.augment_six)throw Failure{Error::UnsupportedContent};
 }
 for(auto creator:creators){auto pending=Query(db,"SELECT pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(creator)+" FOR UPDATE");if(pending.RowCount()>1)throw Failure{Error::Conflict};if(pending.RowCount()&&pending.begin()[0])throw Failure{Error::PendingApplication};}
 // The runtime policy independently validates all rows against complete reviewed
 // yard/room/crate/vault scenes. Missing or suppressed rows cannot disappear.
 if(rows.RowCount()&&!authority.TransferContentsReviewedUnderTransaction(db,request,source,fence))throw Failure{Error::UnsupportedContent};
 retain("custody","SELECT custody_id,owner_character_id,schema_descriptor,HEX(row_blob),HEX(catalog_blob) FROM housing_live_item_custody WHERE estate_id="+estate+" ORDER BY custody_id FOR UPDATE");
 const auto link=" JOIN housing_live_item_custody c ON c.custody_id=t.custody_id WHERE c.estate_id="+estate;
 retain("poses","SELECT t.custody_id,t.native_item_id,t.pose_revision,HEX(t.pose_blob) FROM housing_live_placed_items t"+link+" ORDER BY t.custody_id FOR UPDATE");
 retain("storage","SELECT t.custody_id,t.revision,t.stowed,t.native_estate_id FROM housing_live_item_storage t"+link+" ORDER BY t.custody_id FOR UPDATE");
 retain("rooms","SELECT t.custody_id,t.house_id,t.revision FROM housing_house_item_scopes t"+link+" ORDER BY t.custody_id FOR UPDATE");
 retain("members","SELECT t.custody_id,t.crate_id,t.revision,t.origin_native_estate_id,t.origin_house_id,HEX(t.origin_pose_blob),t.origin_stowed FROM housing_crate_members t"+link+" ORDER BY t.custody_id FOR UPDATE");
 retain("vault","SELECT t.custody_id FROM housing_live_vault_items t"+link+" ORDER BY t.custody_id FOR UPDATE");
 retain("houses","SELECT h.house_id,h.revision,h.item_template_id,h.created_by_character_id,h.pending_request_key,i.native_estate_id,i.definition_id,i.zone_id,i.zone_version,i.instance_id,i.catalog_key,i.catalog_revision FROM housing_house_locations t JOIN housing_house_instances h ON h.house_id=t.house_id JOIN housing_house_interiors i ON i.house_id=h.house_id"+link+" AND t.location_kind=2 ORDER BY h.house_id FOR UPDATE");
 retain("crates","SELECT h.crate_id,h.revision,h.item_template_id,h.created_by_character_id,h.packed,HEX(h.content_manifest_sha256) FROM housing_crate_locations t JOIN housing_crate_instances h ON h.crate_id=t.crate_id"+link+" AND t.location_kind=2 ORDER BY h.crate_id FOR UPDATE");
 source.contents_sha256=Digest(state);
}
Receipt Prepare(Database&db,const Request&r,LiveItems::InventorySessionFence&f,Authority&authority,uint32_t reviewed_limit){Verify(db);auto lease=Session(db,r.lease,true);if(auto receipt=Lookup(db,r))return *receipt;if(CoinPending::Inspect(db,r.lease.actor.character_id,true)!=CoinPending::State::Clear)throw Failure{Error::PendingApplication};if(lease.owner.zone_id!=r.source.zone_id||lease.owner.instance_id!=r.source.instance_id)throw Failure{Error::Unauthorized};auto source=LockedSource(db,r);TransferContents(db,r,source,f,authority);source.reviewed_personal_limit=reviewed_limit;if(!authority.TransferFrameMatches(r,source))throw Failure{Error::UnheldFence};auto recipient=Query(db,"SELECT c.account_id,c.name,c.deleted_at,a.status FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(r.recipient.actor.character_id)+" FOR UPDATE");if(recipient.RowCount()!=1||Number(recipient.begin()[0])!=r.recipient.actor.account_id||!recipient.begin()[1]||Lower(recipient.begin()[1])!=Lower(r.recipient.name)||recipient.begin()[2]||!recipient.begin()[3]||std::stoll(recipient.begin()[3])<0)throw Failure{Error::Unauthorized};auto owned=Query(db,"SELECT id FROM housing_current_estates WHERE owner_kind=1 AND owner_id="+std::to_string(r.recipient.actor.character_id)+" FOR UPDATE");if(owned.RowCount()>=reviewed_limit)throw Failure{Error::TooManyPlots};Receipt out;out.request=r;out.source=source;out.old_owner=r.lease.actor.character_id;out.new_owner=r.recipient.actor.character_id;out.retained_copper=Escrow(db,source.estate_id);return out;}
Receipt Change(Database&db,const Request&r,LiveItems::InventorySessionFence&f,Authority&authority,uint32_t reviewed_limit){auto out=Prepare(db,r,f,authority,reviewed_limit);if(out.replayed)return out;auto source=out.source;auto state=Query(db,"SELECT revision,owner_character_id FROM housing_ownership_state WHERE estate_id="+std::to_string(source.estate_id)+" FOR UPDATE");uint64_t revision=0;if(state.RowCount()){if(state.RowCount()!=1||Number(state.begin()[1])!=out.old_owner||!(revision=std::stoull(state.begin()[0]))||revision==UINT64_MAX)throw Failure{Error::Conflict};}out.ownership_revision=revision+1;out.acl_revision=source.acl_revision+1;
 auto changed=Query(db,"UPDATE housing_estates SET owner_id="+std::to_string(out.new_owner)+" WHERE id="+std::to_string(source.estate_id)+" AND active=1 AND owner_kind=1 AND owner_id="+std::to_string(out.old_owner));if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};if(!revision)Query(db,"INSERT INTO housing_ownership_state(estate_id,revision,owner_character_id) VALUES("+std::to_string(source.estate_id)+",1,"+std::to_string(out.new_owner)+")");else if(Query(db,"UPDATE housing_ownership_state SET revision=revision+1,owner_character_id="+std::to_string(out.new_owner)+" WHERE estate_id="+std::to_string(source.estate_id)+" AND revision="+std::to_string(revision)+" AND owner_character_id="+std::to_string(out.old_owner)).RowsAffected()!=1)throw Failure{Error::Conflict};
 if(!source.acl_revision){Query(db,"INSERT INTO housing_acl_state(estate_id,revision) VALUES("+std::to_string(source.estate_id)+",1)");for(unsigned i=0;i<7;++i)Query(db,"INSERT INTO housing_acl_groups(estate_id,group_id,capabilities) VALUES("+std::to_string(source.estate_id)+","+std::to_string(i)+","+std::to_string(AccessPolicy::NativeDefaults[i])+")");}else if(Query(db,"UPDATE housing_acl_state SET revision=revision+1 WHERE estate_id="+std::to_string(source.estate_id)+" AND revision="+std::to_string(source.acl_revision)).RowsAffected()!=1)throw Failure{Error::Conflict};
 Query(db,"INSERT INTO housing_ownership_transfer_receipts(request_key,character_id,account_id,estate_id,native_plot_id,old_owner,new_owner,ownership_revision,acl_revision,binding_blob,outcome_blob) VALUES("+Quote(db,r.key)+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+","+std::to_string(source.estate_id)+","+std::to_string(source.native_plot_id)+","+std::to_string(out.old_owner)+","+std::to_string(out.new_owner)+","+std::to_string(out.ownership_revision)+","+std::to_string(out.acl_revision)+",x'"+Hex(Binding(r))+"',x'"+Hex(Outcome(out))+"')");if(!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key)||!authority.TransferFrameMatches(r,source))throw Failure{Error::UnheldFence};return out;}
}
Service::Service(std::unique_ptr<Database>db,bool enabled,uint32_t reviewed_limit):db_(std::move(db)),enabled_(enabled),reviewed_personal_limit_(reviewed_limit){}
Result<Receipt> Service::Transfer(const Request&r,LiveItems::InventorySessionFence&f,Authority&a){std::lock_guard<std::mutex>g(mutex_);if(!enabled_)return {{},Error::Disabled};if(reviewed_personal_limit_<1||reviewed_personal_limit_>3)return {{},Error::UnreviewedLimit};if(!Shape(r))return {{},Error::InvalidArgument};if(!db_||!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};return Run<Receipt>(*db_,[&]{return Change(*db_,r,f,a,reviewed_personal_limit_);});}
Result<Receipt> Service::Inspect(const Request&r,LiveItems::InventorySessionFence&f,Authority&a){std::lock_guard<std::mutex>g(mutex_);if(!enabled_)return {{},Error::Disabled};if(reviewed_personal_limit_<1||reviewed_personal_limit_>3)return {{},Error::UnreviewedLimit};if(!Shape(r))return {{},Error::InvalidArgument};if(!db_||!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};return Run<Receipt>(*db_,[&]{return Prepare(*db_,r,f,a,reviewed_personal_limit_);});}
Result<Receipt> Service::Resolve(const Request&r,LiveItems::InventorySessionFence&f){std::lock_guard<std::mutex>g(mutex_);if(!enabled_)return {{},Error::Disabled};if(!Shape(r)||!db_||!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};return Run<Receipt>(*db_,[&]{Verify(*db_);Session(*db_,r.lease,false);auto old=Lookup(*db_,r);if(!old)throw Failure{Error::NotFound};return *old;});}
}
