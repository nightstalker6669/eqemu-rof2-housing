#include "upkeep_enrollment_db.h"
#include "native_plot_purchase.h"
#include "catalog_service.h"
#include "coin_pending_guard.h"
#include "guild_purchase_authority.h"
#include "../guilds.h"
#include "../emu_constants.h"
#include "../repositories/character_currency_repository.h"
#include <openssl/sha.h>
#include <algorithm>
#include <type_traits>
namespace Housing::NativePlotPurchase {namespace {
using Bytes=std::vector<uint8_t>;using Currency=LiveEscrow::Currency;
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
Bytes Take(Reader&r,size_t n){if(n>r.bytes.size()-r.at)throw Failure{Error::DatabaseFailure};Bytes b(r.bytes.begin()+r.at,r.bytes.begin()+r.at+n);r.at+=n;return b;}
std::string Digest(const Bytes&b){unsigned char sum[32];SHA256(b.data(),b.size(),sum);const char*h="0123456789abcdef";std::string s;for(auto x:sum){s+=h[x>>4];s+=h[x&15];}return s;}
void Verify(Database&db){auto rows=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_native_plot_purchase_policies','housing_native_plot_purchase_receipts','housing_native_plot_purchase_pending','housing_native_plot_purchase_reconciliations','housing_inventory_sessions','housing_live_item_leases','character_data','account','character_currency','inventory','housing_native_estates','housing_catalogs','housing_neighborhoods','housing_plot_definitions','housing_estates','instance_list')");if(rows.RowCount()!=1||Number(rows.begin()[0])!=16)throw Failure{Error::DatabaseFailure};auto cols=Query(db,"SELECT column_name FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='character_currency' ORDER BY ordinal_position");auto expected=CharacterCurrencyRepository::Columns();if(cols.RowCount()!=expected.size())throw Failure{Error::DatabaseFailure};size_t i=0;for(auto row:cols)if(expected[i++]!=row[0])throw Failure{Error::DatabaseFailure};}
std::vector<Price> Prices(const Catalog::Bundle&b,uint32_t scheme,bool positive,bool absent_zero=false){if(!scheme)return absent_zero?std::vector<Price>{{0,0}}:std::vector<Price>{};auto s=std::find_if(b.costs.begin(),b.costs.end(),[&](auto&s){return s.id==scheme;});if(s==b.costs.end())throw Failure{Error::Conflict};std::vector<Price>out;for(auto&c:s->components){int64_t total=0;bool valid=!c.values.empty();for(auto&v:c.values){if(v.type_id!=0||v.subtype_id!=0||v.amount<0||v.amount>INT64_MAX-total){valid=false;break;}total+=v.amount;}if(valid&&(positive||!total))out.push_back({c.id,total});}return out;}
GuildOwner LockedGuildLeader(Database& db,const LiveItems::Lease& lease,Fence& fence){
 if(!fence.GuildExpansionAllowed())throw Failure{Error::Unauthorized};
 auto permission=GuildPurchaseAuthority::CheckUnderTransaction(db,lease,fence,true);
 if(!permission){if(permission.error==GuildPurchaseAuthority::Error::DatabaseFailure||permission.error==GuildPurchaseAuthority::Error::CorruptData)throw Failure{Error::DatabaseFailure};throw Failure{Error::Unauthorized};}
 // This initial native acquisition path admits the actual current leader only.
 // Action27 remains independently required; no invented implicit rank/GM grant.
 if(permission.value.native_rank!=GUILD_LEADER)throw Failure{Error::Unauthorized};
 auto current=Query(db,"SELECT leader FROM guilds WHERE id="+std::to_string(permission.value.guild_id)+" FOR UPDATE");
 if(current.RowCount()!=1||Number(current.begin()[0])!=lease.actor.character_id)throw Failure{Error::Unauthorized};
 return {permission.value.guild_id,lease.actor.character_id,permission.value.rank_mask};
}
bool GuildOfferShape(const Offer& o){
 if(!o.guild)return true;const auto& g=*o.guild;
 return g.guild_id&&g.guild_id!=GUILD_NONE&&g.leader_character_id&&g.leader_character_id!=UINT32_MAX&&g.reviewed_minimum_members&&GuildPurchaseAuthority::RankGranted(g.permission_mask,GUILD_LEADER)&&!o.prices.empty()&&std::all_of(o.prices.begin(),o.prices.end(),[&](const Price&p){return p.copper>=0&&(o.carried_copper||p.copper==0);});
}
void GuildFundingPolicy(Database& db,Offer& offer){
 // Original5426 proves a minimum, not its2013 numeric value. This explicit
 // emulator review policy is unseeded; zero/missing leaves acquisition closed.
 auto columns=Query(db,"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_native_plot_purchase_policies' AND column_name IN ('allow_guild_zero_copper','reviewed_minimum_guild_members')");
 if(columns.RowCount()!=1||Number(columns.begin()[0])!=2)throw Failure{Error::UnsupportedFunding};
 auto policy=Query(db,"SELECT allow_guild_zero_copper,reviewed_minimum_guild_members FROM housing_native_plot_purchase_policies WHERE catalog_key="+Quote(db,offer.catalog_key)+" AND definition_id="+std::to_string(offer.definition_id)+" FOR UPDATE");
 if(policy.RowCount()!=1||Number(policy.begin()[0])>1)throw Failure{Error::Conflict};
 auto minimum=Number(policy.begin()[1]);
 // The new opt-in mode debits only the actual leader's carried coin through
 // the same authenticated profile journal as personal purchases. No guild-bank
 // funding or implicit zero-price permission is inferred.
 bool paid=false;
 auto paid_column=Query(db,"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_native_plot_purchase_policies' AND column_name='allow_guild_carried_copper'");
 if(paid_column.RowCount()!=1)throw Failure{Error::DatabaseFailure};
 if(Number(paid_column.begin()[0])){auto review=Query(db,"SELECT allow_guild_carried_copper FROM housing_native_plot_purchase_policies WHERE catalog_key="+Quote(db,offer.catalog_key)+" AND definition_id="+std::to_string(offer.definition_id)+" FOR UPDATE");if(review.RowCount()!=1||Number(review.begin()[0])>1)throw Failure{Error::Conflict};paid=Number(review.begin()[0])==1;}
 if(!minimum||(!paid&&Number(policy.begin()[0])!=1)||(paid&&!offer.carried_copper))throw Failure{Error::UnsupportedFunding};
 offer.carried_copper=paid;

 auto members=Query(db,"SELECT m.char_id FROM guild_members m JOIN character_data c ON c.id=m.char_id WHERE m.guild_id="+std::to_string(offer.guild->guild_id)+" AND c.deleted_at IS NULL FOR UPDATE");
 if(members.RowCount()<minimum)throw Failure{Error::Unauthorized};
 offer.guild->reviewed_minimum_members=minimum;
}
Offer LockedOffer(Database&db,const LiveItems::Lease&lease,uint32_t native,Route source,Fence&fence,bool* no_upkeep_scheme=nullptr){if(source.zone_id!=712||source.zone_version||!source.instance_id||source.instance_id>32000)throw Failure{Error::Unauthorized};auto n=Query(db,"SELECT n.catalog_key,n.neighborhood_id,n.plot_id,n.definition_id,n.ownership_estate_id,p.catalog_key,p.definition_id,p.active,p.parent_native_id,h.zone_id,h.zone_version,h.instance_id,i.zone,i.version,i.never_expires,p.neighborhood_id,p.plot_id,p.ownership_estate_id FROM housing_native_estates n JOIN housing_native_estates p ON p.native_id=n.parent_native_id JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN instance_list i ON i.id=h.instance_id WHERE n.native_id="+std::to_string(native)+" AND n.active=1 FOR UPDATE");if(n.RowCount()!=1)throw Failure{Error::NotFound};auto row=n.begin();if(!row[0]||!row[5]||std::string(row[0])!=row[5]||Number(row[7])!=1||row[8]||Number(row[9])!=712||Number(row[10])||Number(row[11])!=source.instance_id||Number(row[12])!=712||Number(row[13])||Number(row[14])!=1||!row[2]||!Number(row[2]))throw Failure{Error::Unauthorized};if(row[4])throw Failure{Error::Unavailable};Offer out;out.native_plot_id=native;out.catalog_key=row[0];out.neighborhood_id=Number(row[1]);out.plot_id=Number(row[2]);out.definition_id=Number(row[3]);out.source=source;const auto root_definition=Number(row[6]);auto plot=Query(db,"SELECT neighborhood_id FROM housing_plot_definitions WHERE id="+std::to_string(out.plot_id)+" FOR UPDATE");if(plot.RowCount()!=1||Number(plot.begin()[0])!=out.neighborhood_id)throw Failure{Error::Conflict};auto owners=Query(db,"SELECT id FROM housing_current_estates WHERE plot_id="+std::to_string(out.plot_id)+" FOR UPDATE");if(owners.RowCount())throw Failure{Error::Unavailable};auto physical=Query(db,"SELECT r.native_item_id FROM housing_live_placement_reservations r WHERE r.native_estate_id="+std::to_string(native)+" AND r.state<>2 FOR UPDATE");if(physical.RowCount())throw Failure{Error::Unavailable};
 auto catalog=Query(db,"SELECT revision,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,out.catalog_key)+" FOR UPDATE");if(catalog.RowCount()!=1)throw Failure{Error::NotFound};auto cr=catalog.begin();out.catalog_revision=std::stoull(cr[0]);auto definitions=Unhex(cr[1]),regions=Unhex(cr[2]);out.definitions_sha256=Digest(definitions);out.regions_sha256=Digest(regions);auto b=Catalog::DecodePayloads(definitions,regions);auto d=std::find_if(b.definitions.begin(),b.definitions.end(),[&](auto&d){return d.id==out.definition_id;});auto root=std::find_if(b.definitions.begin(),b.definitions.end(),[&](auto&d){return d.id==root_definition;});if(!out.catalog_revision||d==b.definitions.end()||root==b.definitions.end()||(d->type!=4&&d->type!=8)||d->zone_id!=712||root->type!=5||root->zone_id!=712||std::find(root->children.begin(),root->children.end(),d->id)==root->children.end()||!d->area_id||std::none_of(b.regions.begin(),b.regions.end(),[&](auto&r){return r.id==d->area_id;}))throw Failure{Error::Unauthorized};
 if(d->type==8){if(Number(row[15])!=out.neighborhood_id||row[16]||row[17])throw Failure{Error::Unauthorized};out.guild=LockedGuildLeader(db,lease,fence);}
 auto policy=Query(db,"SELECT catalog_revision,definitions_sha256,regions_sha256,policy_revision,allow_carried_copper,max_owned_plots FROM housing_native_plot_purchase_policies WHERE catalog_key="+Quote(db,out.catalog_key)+" AND definition_id="+std::to_string(out.definition_id)+" AND enabled=1 FOR UPDATE");if(policy.RowCount()!=1)throw Failure{Error::UnsupportedFunding};auto pr=policy.begin();if(std::stoull(pr[0])!=out.catalog_revision||!pr[1]||!pr[2]||std::string(pr[1])!=out.definitions_sha256||std::string(pr[2])!=out.regions_sha256)throw Failure{Error::Conflict};out.policy_revision=std::stoull(pr[3]);out.carried_copper=Number(pr[4])==1;out.max_owned_plots=Number(pr[5]);if(!out.policy_revision||Number(pr[4])>1||!out.max_owned_plots||out.max_owned_plots>3)throw Failure{Error::Conflict};if(out.guild){GuildFundingPolicy(db,out);}out.prices=Prices(b,d->purchase_scheme,out.carried_copper);auto rent=UpkeepEnrollment::Read(db,out.catalog_key,out.definition_id,out.catalog_revision,out.definitions_sha256,out.regions_sha256);for(auto&p:Prices(b,d->upkeep_scheme,bool(rent),true))if(!p.copper||(rent&&UpkeepClock::Hourly(uint64_t(p.copper),3600,true)))out.zero_upkeep_components.push_back(p.component);if(out.prices.empty()||out.zero_upkeep_components.empty())throw Failure{Error::UnsupportedFunding};if(no_upkeep_scheme)*no_upkeep_scheme=d->upkeep_scheme==0;return out;}
void OfferData(Bytes&b,const Offer&o){if(!GuildOfferShape(o))throw Failure{Error::InvalidArgument};Word(b,o.guild?(o.carried_copper?3:2):1,4);for(auto n:{o.native_plot_id,o.plot_id,o.neighborhood_id,o.definition_id})Word(b,n,4);Word(b,o.catalog_revision,8);Word(b,o.policy_revision,8);Text(b,o.catalog_key);Text(b,o.definitions_sha256);Text(b,o.regions_sha256);Word(b,o.source.zone_id,4);Word(b,o.source.zone_version,1);Word(b,o.source.instance_id,2);Word(b,o.max_owned_plots,4);Word(b,o.carried_copper,1);Word(b,o.prices.size(),4);for(auto&p:o.prices){Word(b,p.component,4);Word(b,uint64_t(p.copper),8);}Word(b,o.zero_upkeep_components.size(),4);for(auto n:o.zero_upkeep_components)Word(b,n,4);if(o.guild){Word(b,o.guild->guild_id,4);Word(b,o.guild->leader_character_id,4);Word(b,o.guild->permission_mask,1);Word(b,o.guild->reviewed_minimum_members,4);}}
Offer ReadOffer(Reader&r){const auto version=r.Word(4);if(version!=1&&version!=2&&version!=3)throw Failure{Error::DatabaseFailure};Offer o;o.native_plot_id=r.Word(4);o.plot_id=r.Word(4);o.neighborhood_id=r.Word(4);o.definition_id=r.Word(4);o.catalog_revision=r.Word(8);o.policy_revision=r.Word(8);o.catalog_key=r.Text();o.definitions_sha256=r.Text();o.regions_sha256=r.Text();o.source={uint32_t(r.Word(4)),uint8_t(r.Word(1)),uint16_t(r.Word(2))};o.max_owned_plots=r.Word(4);o.carried_copper=r.Word(1);auto n=r.Word(4);if(n>1000)throw Failure{Error::DatabaseFailure};for(unsigned i=0;i<n;++i)o.prices.push_back({uint32_t(r.Word(4)),int64_t(r.Word(8))});n=r.Word(4);if(n>1000)throw Failure{Error::DatabaseFailure};for(unsigned i=0;i<n;++i)o.zero_upkeep_components.push_back(r.Word(4));if(version>=2){GuildOwner g;g.guild_id=r.Word(4);g.leader_character_id=r.Word(4);g.permission_mask=r.Word(1);g.reviewed_minimum_members=r.Word(4);o.guild=g;}if(!GuildOfferShape(o)||(version==2&&o.carried_copper)||(version==3&&!o.carried_copper))throw Failure{Error::DatabaseFailure};return o;}
Bytes Binding(const Request&r){Bytes b;Word(b,1,4);Text(b,r.request_key);Word(b,r.lease.actor.character_id,4);Word(b,r.lease.actor.account_id,4);Word(b,r.lease.epoch,8);Text(b,r.lease.session_key);b.insert(b.end(),r.lease.token.begin(),r.lease.token.end());OfferData(b,r.offer);b.insert(b.end(),r.wire.begin(),r.wire.end());CurrencyBytes(b,r.expected);return b;}
Request ReadBinding(const Bytes&b){Reader r{b};if(r.Word(4)!=1)throw Failure{Error::DatabaseFailure};Request out;out.request_key=r.Text();out.lease.actor={uint32_t(r.Word(4)),uint32_t(r.Word(4))};out.lease.epoch=r.Word(8);out.lease.session_key=r.Text();auto token=Take(r,32);std::copy(token.begin(),token.end(),out.lease.token.begin());out.offer=ReadOffer(r);auto wire=Take(r,out.wire.size());std::copy(wire.begin(),wire.end(),out.wire.begin());out.expected=ReadCurrency(r);if(r.at!=b.size()||Binding(out)!=b)throw Failure{Error::DatabaseFailure};return out;}
Bytes Outcome(const Receipt&r){Bytes b;Word(b,1,4);Word(b,r.estate_id,4);Word(b,uint64_t(r.charged_copper),8);CurrencyBytes(b,r.after);Word(b,r.reservation.epoch,8);b.insert(b.end(),r.reservation.token.begin(),r.reservation.token.end());Text(b,r.reservation.owner.process_key);Text(b,r.reservation.owner.client_key);Word(b,r.reservation.owner.zone_id,4);Word(b,r.reservation.owner.instance_id,2);return b;}
Receipt ReadOutcome(const Request&request,const Bytes&b){Reader r{b};if(r.Word(4)!=1)throw Failure{Error::DatabaseFailure};Receipt out;out.request=request;out.estate_id=r.Word(4);out.charged_copper=int64_t(r.Word(8));out.after=ReadCurrency(r);out.reservation.actor=request.lease.actor;out.reservation.epoch=r.Word(8);auto t=Take(r,32);std::copy(t.begin(),t.end(),out.reservation.token.begin());out.reservation.owner={r.Text(),r.Text(),uint32_t(r.Word(4)),uint16_t(r.Word(2))};out.reservation.active=true;if(r.at!=b.size()||!out.estate_id||out.charged_copper<0||!Valid(out.after)||!out.reservation.epoch||Outcome(out)!=b)throw Failure{Error::DatabaseFailure};auto expected=request.expected;if(out.charged_copper){auto p=CoinWallet::DebitCarried(expected.carried,out.charged_copper);if(!p)throw Failure{Error::DatabaseFailure};expected.carried=p.value.after;}if(!(expected==out.after))throw Failure{Error::DatabaseFailure};return out;}
bool Shape(const Request&r){auto w=EstateWire::DecodePurchase(r.wire.data(),r.wire.size());return Key(r.request_key)&&Key(r.lease.session_key)&&r.lease.actor.character_id&&r.lease.actor.account_id&&r.lease.epoch&&r.lease.pending_key.empty()&&Valid(r.expected)&&GuildOfferShape(r.offer)&&(!r.offer.guild||r.offer.guild->leader_character_id==r.lease.actor.character_id)&&w&&w->estate_id==r.offer.native_plot_id&&w->sentinel_a==UINT32_MAX&&w->sentinel_b==UINT32_MAX&&std::all_of(w->reserved.begin(),w->reserved.end(),[](auto x){return x==0;})&&std::all_of(w->flags.begin(),w->flags.end(),[](auto x){return x==0;});}
bool Same(const InventorySessions::Lease&a,const InventorySessions::Lease&b){return a.actor.character_id==b.actor.character_id&&a.actor.account_id==b.actor.account_id&&a.epoch==b.epoch&&a.token==b.token&&a.owner.process_key==b.owner.process_key&&a.owner.client_key==b.owner.client_key&&a.owner.zone_id==b.owner.zone_id&&a.owner.instance_id==b.owner.instance_id;}
void Committed(Database&db,const Receipt&r){EmptyCursor(db,r.request.lease.actor.character_id);const auto&o=r.request.offer;auto estate=Query(db,"SELECT n.plot_id,n.ownership_estate_id,n.neighborhood_id,e.owner_kind,e.owner_id,e.request_key,h.instance_id,i.zone,i.version,i.never_expires FROM housing_native_estates n JOIN housing_current_estates e ON e.id=n.ownership_estate_id AND e.plot_id=n.plot_id JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN instance_list i ON i.id=h.instance_id WHERE n.native_id="+std::to_string(o.native_plot_id)+" AND n.active=1 FOR UPDATE");if(estate.RowCount()!=1)throw Failure{Error::Conflict};auto e=estate.begin();if(Number(e[0])!=o.plot_id||Number(e[1])!=r.estate_id||Number(e[2])!=o.neighborhood_id||Number(e[3])!=(o.guild?3u:1u)||Number(e[4])!=(o.guild?o.guild->guild_id:r.request.lease.actor.character_id)||!e[5]||std::string(e[5])!=r.request.request_key||Number(e[6])!=o.source.instance_id||Number(e[7])!=712||Number(e[8])||Number(e[9])!=1||!(LockedCurrency(db,r.request.lease.actor.character_id)==r.after))throw Failure{Error::Conflict};}
std::optional<Receipt> Lookup(Database&db,const Request&r,const InventorySessions::Lease&reservation){auto rows=Query(db,"SELECT character_id,account_id,estate_id,HEX(binding_blob),HEX(outcome_blob),applied,reconciled,native_plot_id,inventory_epoch,HEX(inventory_token),process_key FROM housing_native_plot_purchase_receipts WHERE request_key="+Quote(db,r.request_key)+" FOR UPDATE");if(!rows.RowCount())return {};if(rows.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto row=rows.begin();if(Number(row[0])!=r.lease.actor.character_id||Number(row[1])!=r.lease.actor.account_id||Unhex(row[3])!=Binding(r))throw Failure{Error::Conflict};auto out=ReadOutcome(r,Unhex(row[4]));if(Number(row[2])!=out.estate_id||!Same(out.reservation,reservation)||Number(row[5])>1||Number(row[6])>1||(Number(row[5])&&Number(row[6]))||Number(row[7])!=r.offer.native_plot_id||std::stoull(row[8])!=reservation.epoch||Unhex(row[9])!=Bytes(reservation.token.begin(),reservation.token.end())||!row[10]||std::string(row[10])!=reservation.owner.process_key)throw Failure{Error::Conflict};out.replayed=true;out.requires_application=Number(row[5])==0&&Number(row[6])==0;auto pending=Query(db,"SELECT request_key,account_id,inventory_epoch,HEX(inventory_token) FROM housing_native_plot_purchase_pending WHERE character_id="+std::to_string(r.lease.actor.character_id)+" FOR UPDATE");if(out.requires_application){if(pending.RowCount()!=1||!pending.begin()[0]||std::string(pending.begin()[0])!=r.request_key||Number(pending.begin()[1])!=r.lease.actor.account_id||std::stoull(pending.begin()[2])!=reservation.epoch||Unhex(pending.begin()[3])!=Bytes(reservation.token.begin(),reservation.token.end()))throw Failure{Error::Conflict};Committed(db,out);}else if(pending.RowCount()&&std::string(pending.begin()[0])==r.request_key)throw Failure{Error::Conflict};return out;}
void UpdateCurrency(Database&db,const Request&r,const Currency&after){if(after==r.expected)return;auto cols=CharacterCurrencyRepository::Columns();auto values=Fields(r.expected);std::string where="id="+std::to_string(r.lease.actor.character_id);for(size_t i=0;i<values.size();++i)where+=" AND `"+cols[i+1]+"`="+std::to_string(values[i]);auto q=Query(db,"UPDATE character_currency SET copper="+std::to_string(after.carried.copper)+",silver="+std::to_string(after.carried.silver)+",gold="+std::to_string(after.carried.gold)+",platinum="+std::to_string(after.carried.platinum)+" WHERE "+where);if(q.RowsAffected()!=1)throw Failure{Error::SnapshotMismatch};}
Receipt BuyBody(Database&db,const Request&r,Fence&f){Verify(db);auto reservation=Session(db,r.lease,true);if(auto old=Lookup(db,r,reservation))return *old;if(CoinPending::Inspect(db,r.lease.actor.character_id,true)!=CoinPending::State::Clear)throw Failure{Error::PendingApplication};if(!f.CurrencyMatches(r.expected)||!f.CursorItemsEmpty()||!f.InspectedOfferMatches(r.offer))throw Failure{Error::UnheldFence};EmptyCursor(db,r.lease.actor.character_id);if(reservation.owner.zone_id!=r.offer.source.zone_id||reservation.owner.instance_id!=r.offer.source.instance_id)throw Failure{Error::Unauthorized};bool no_upkeep_scheme=false;auto actual=LockedOffer(db,r.lease,r.offer.native_plot_id,r.offer.source,f,&no_upkeep_scheme);if(OfferBytes(actual)!=OfferBytes(r.offer))throw Failure{Error::Conflict};auto wire=*EstateWire::DecodePurchase(r.wire.data(),r.wire.size());auto price=std::find_if(actual.prices.begin(),actual.prices.end(),[&](auto&p){return p.component==wire.price_component;});if(price==actual.prices.end()||(wire.upkeep_component==UINT32_MAX?!no_upkeep_scheme:std::find(actual.zero_upkeep_components.begin(),actual.zero_upkeep_components.end(),wire.upkeep_component)==actual.zero_upkeep_components.end()))throw Failure{Error::UnsupportedFunding};const uint32_t owner_kind=actual.guild?3u:1u,owner_id=actual.guild?actual.guild->guild_id:r.lease.actor.character_id;auto owned=Query(db,"SELECT id FROM housing_current_estates WHERE owner_kind="+std::to_string(owner_kind)+" AND owner_id="+std::to_string(owner_id)+" FOR UPDATE");if(owned.RowCount()>=actual.max_owned_plots)throw Failure{Error::TooManyPlots};if(!(LockedCurrency(db,r.lease.actor.character_id)==r.expected))throw Failure{Error::SnapshotMismatch};Receipt out;out.request=r;out.reservation=reservation;out.charged_copper=price->copper;out.after=r.expected;if(out.charged_copper){if(!actual.carried_copper)throw Failure{Error::UnsupportedFunding};auto plan=CoinWallet::DebitCarried(r.expected.carried,out.charged_copper);if(!plan)throw Failure{plan.error==CoinWallet::Error::InsufficientFunds?Error::InsufficientFunds:Error::Overflow};out.after.carried=plan.value.after;}UpdateCurrency(db,r,out.after);auto estate=Query(db,"INSERT INTO housing_estates(request_key,plot_id,owner_kind,owner_id) VALUES ("+Quote(db,r.request_key)+","+std::to_string(actual.plot_id)+","+std::to_string(owner_kind)+","+std::to_string(owner_id)+")");auto id=estate.LastInsertedID();if(!id||id>=UINT32_MAX)throw Failure{Error::DatabaseFailure};out.estate_id=uint32_t(id);auto native=Query(db,"UPDATE housing_native_estates SET ownership_estate_id="+std::to_string(out.estate_id)+" WHERE native_id="+std::to_string(actual.native_plot_id)+" AND ownership_estate_id IS NULL AND active=1");if(native.RowsAffected()!=1)throw Failure{Error::Unavailable};// Enroll only a genuinely paid selected upkeep component; legacy free purchase remains unchanged.
if(wire.upkeep_component!=UINT32_MAX){auto snapshot=Query(db,"SELECT HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,actual.catalog_key)+" AND revision="+std::to_string(actual.catalog_revision)+" FOR UPDATE");if(snapshot.RowCount()!=1)throw Failure{Error::Conflict};auto bundle=Catalog::DecodePayloads(Unhex(snapshot.begin()[0]),Unhex(snapshot.begin()[1]));auto definition=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==actual.definition_id;});if(definition==bundle.definitions.end())throw Failure{Error::Conflict};auto choices=Prices(bundle,definition->upkeep_scheme,true,true);auto selected=std::find_if(choices.begin(),choices.end(),[&](const auto& p){return p.component==wire.upkeep_component;});if(selected==choices.end())throw Failure{Error::UnsupportedFunding};if(selected->copper){auto policy=UpkeepEnrollment::Read(db,actual.catalog_key,actual.definition_id,actual.catalog_revision,actual.definitions_sha256,actual.regions_sha256);if(!policy||!UpkeepClock::Hourly(uint64_t(selected->copper),3600,true))throw Failure{Error::UnsupportedFunding};UpkeepEnrollment::Enroll(db,out.estate_id,actual.native_plot_id,owner_kind,owner_id,actual.definition_id,wire.upkeep_component,actual.catalog_revision,*policy);}}
const auto&owner=out.reservation;Query(db,"INSERT INTO housing_native_plot_purchase_receipts(request_key,character_id,account_id,estate_id,native_plot_id,inventory_epoch,inventory_token,process_key,binding_blob,outcome_blob) VALUES ("+Quote(db,r.request_key)+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+","+std::to_string(out.estate_id)+","+std::to_string(actual.native_plot_id)+","+std::to_string(owner.epoch)+",x'"+Hex(Bytes(owner.token.begin(),owner.token.end()))+"',"+Quote(db,owner.owner.process_key)+",x'"+Hex(Binding(r))+"',x'"+Hex(Outcome(out))+"')");Query(db,"INSERT INTO housing_native_plot_purchase_pending(character_id,account_id,request_key,inventory_epoch,inventory_token) VALUES ("+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+","+Quote(db,r.request_key)+","+std::to_string(owner.epoch)+",x'"+Hex(Bytes(owner.token.begin(),owner.token.end()))+"')");if(!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key)||!f.CurrencyMatches(r.expected)||!f.InspectedOfferMatches(r.offer)||(r.offer.guild&&!f.GuildExpansionAllowed()))throw Failure{Error::UnheldFence};return out;}
} // namespace
std::vector<uint8_t> OfferBytes(const Offer&o){Bytes b;OfferData(b,o);return b;}
bool MatchesPublished(const Offer&o,const Catalog::Export&e){return o.catalog_key==e.catalog.key&&o.catalog_revision==e.catalog.revision&&o.definitions_sha256==Digest(e.definition_payload)&&o.regions_sha256==Digest(e.region_payload);}
Service::Service(std::unique_ptr<Database>db,bool enabled):db_(std::move(db)),enabled_(enabled){}
Result<Offer> Service::Inspect(const LiveItems::Lease&l,uint32_t native,Route source,Fence&f){std::lock_guard<std::mutex>g(mutex_);if(!enabled_)return {{},Error::Disabled};if(!db_||!native||native==UINT32_MAX||!f.HeldFor(l.actor,l.epoch,l.session_key))return {{},Error::UnheldFence};return Run<Offer>(*db_,[&]{Verify(*db_);auto owner=Session(*db_,l,true);if(owner.owner.zone_id!=source.zone_id||owner.owner.instance_id!=source.instance_id||CoinPending::Inspect(*db_,l.actor.character_id,true)!=CoinPending::State::Clear)throw Failure{Error::Unauthorized};return LockedOffer(*db_,l,native,source,f);});}
Result<Receipt> Service::Buy(const Request&r,Fence&f){std::lock_guard<std::mutex>g(mutex_);if(!enabled_)return {{},Error::Disabled};if(!Shape(r))return {{},Error::InvalidArgument};if(!db_||!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};return Run<Receipt>(*db_,[&]{return BuyBody(*db_,r,f);});}
Result<Receipt> Service::Resolve(const Request&r,Fence&f){std::lock_guard<std::mutex>g(mutex_);if(!enabled_)return {{},Error::Disabled};if(!Shape(r)||!db_||!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};return Run<Receipt>(*db_,[&]{Verify(*db_);auto reservation=Session(*db_,r.lease,false);auto old=Lookup(*db_,r,reservation);if(!old)throw Failure{Error::NotFound};return *old;});}
Result<bool> Service::AcknowledgeApplied(const Request&r,Fence&f){std::lock_guard<std::mutex>g(mutex_);if(!enabled_)return {{},Error::Disabled};if(!Shape(r)||!db_||!f.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};return Run<bool>(*db_,[&]{Verify(*db_);auto reservation=Session(*db_,r.lease,false);auto receipt=Lookup(*db_,r,reservation);if(!receipt)throw Failure{Error::NotFound};if(!receipt->requires_application)return true;if(!f.PurchaseApplied(*receipt)||!f.CurrencyMatches(receipt->after)||!f.CursorItemsEmpty())throw Failure{Error::UnheldFence};EmptyCursor(*db_,r.lease.actor.character_id);auto saved=Query(*db_,"UPDATE housing_native_plot_purchase_receipts SET applied=1 WHERE request_key="+Quote(*db_,r.request_key)+" AND applied=0 AND reconciled=0");auto cleared=Query(*db_,"DELETE FROM housing_native_plot_purchase_pending WHERE character_id="+std::to_string(r.lease.actor.character_id)+" AND request_key="+Quote(*db_,r.request_key)+" AND inventory_epoch="+std::to_string(reservation.epoch)+" AND inventory_token=x'"+Hex(Bytes(reservation.token.begin(),reservation.token.end()))+"'");if(saved.RowsAffected()!=1||cleared.RowsAffected()!=1)throw Failure{Error::Conflict};return true;});}
Result<std::pair<std::string,std::vector<uint8_t>>> Service::ReconcileRestartUnderTransaction(Database&db,const InventorySessions::Lease&reservation,const LiveItems::Lease&lease){try{auto tx=Query(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||Number(tx.begin()[0])!=1||!lease.pending_key.empty())throw Failure{Error::Conflict};Verify(db);auto death=Query(db,"SELECT reaped,child_pid,exit_code,reaped_at FROM housing_inventory_processes WHERE process_key="+Quote(db,reservation.owner.process_key)+" FOR UPDATE");if(death.RowCount()!=1||Number(death.begin()[0])!=1||!death.begin()[1]||!std::stoull(death.begin()[1])||!death.begin()[2]||!death.begin()[3]||!Same(Session(db,lease,true),reservation))throw Failure{Error::Unauthorized};auto rows=Query(db,"SELECT r.request_key,HEX(r.binding_blob),HEX(r.outcome_blob) FROM housing_native_plot_purchase_pending p JOIN housing_native_plot_purchase_receipts r ON r.request_key=p.request_key WHERE p.character_id="+std::to_string(lease.actor.character_id)+" AND r.applied=0 AND r.reconciled=0 FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::Conflict};auto row=rows.begin();auto request=ReadBinding(Unhex(row[1]));if(!Shape(request)||request.request_key!=row[0]||request.lease.actor.character_id!=lease.actor.character_id||request.lease.actor.account_id!=lease.actor.account_id||request.lease.epoch!=lease.epoch||request.lease.session_key!=lease.session_key||request.lease.token!=lease.token)throw Failure{Error::Conflict};auto receipt=Lookup(db,request,reservation);if(!receipt||!receipt->requires_application||!Same(receipt->reservation,reservation))throw Failure{Error::Conflict};Committed(db,*receipt);auto out=Unhex(row[2]);auto saved=Query(db,"INSERT INTO housing_native_plot_purchase_reconciliations(request_key,character_id,inventory_epoch,inventory_token,process_key,outcome_blob) VALUES ("+Quote(db,request.request_key)+","+std::to_string(lease.actor.character_id)+","+std::to_string(reservation.epoch)+",x'"+Hex(Bytes(reservation.token.begin(),reservation.token.end()))+"',"+Quote(db,reservation.owner.process_key)+",x'"+Hex(out)+"')");auto marked=Query(db,"UPDATE housing_native_plot_purchase_receipts SET reconciled=1 WHERE request_key="+Quote(db,request.request_key)+" AND applied=0 AND reconciled=0");auto cleared=Query(db,"DELETE FROM housing_native_plot_purchase_pending WHERE character_id="+std::to_string(lease.actor.character_id)+" AND request_key="+Quote(db,request.request_key)+" AND inventory_epoch="+std::to_string(reservation.epoch)+" AND inventory_token=x'"+Hex(Bytes(reservation.token.begin(),reservation.token.end()))+"'");if(saved.RowsAffected()!=1||marked.RowsAffected()!=1||cleared.RowsAffected()!=1)throw Failure{Error::Conflict};return {{request.request_key,std::move(out)},Error::None};}catch(const Failure&x){return {{},x.error};}catch(...){return {{},Error::DatabaseFailure};}}
}
