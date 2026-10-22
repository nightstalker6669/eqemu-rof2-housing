#include "occupied_retirement_authority.h"
#include "npc_native_frame_db.h"
#include "npc_item_metadata_db.h"
#include "npc_runtime_db.h"
#include "rent_state_db.h"
#include "occupied_parent_yard.h"
#include "crate_house_rules.h"
#include "crate_stack_partition.h"
#include "layout_inventory_units.h"
#include "crate_member_scope_db.h"
#include "furniture_estate_limit.h"
#include "interior_placement_review.h"
#include "vault_storage_db.h"
#include "inventory_row_db.h"
#include "item_payload_db.h"
#include "runtime_activation_db.h"
#include "house_item_scope_db.h"
#include "item_storage_db.h"
#include "live_item_service.h"
#include "foreign_furniture_return_policy.h"
#include "house_binding_service.h"
#include "crate_binding.h"
#include "house_inventory_transition.h"
#include "current_estate_access.h"
#include "coin_pending_guard.h"
#include "custody_codec.h"
#include "catalog_content_db.h"
#include "inventory_process_recovery.h"
#include "../database.h"
#include "../eqemu_logsys.h"
#include "catalog_service.h"
#include "../inventory_profile.h"
#include <openssl/rand.h>
#include <limits>
#include <cstring>
#include <algorithm>
#include <type_traits>
#include <stdexcept>
namespace Housing::LiveItems {
    namespace {
        struct Failure{
            Error error;
        };
        using Bytes=std::vector<uint8_t>;
        MySQLRequestResult Q(Database& d,const std::string& s){
            auto r=d.QueryDatabase(s,false);
            if(!r.Success())throw Failure{
                Error::DatabaseFailure
            };
            return r;
        }
        class Tx{
            Database& db;
            bool active=true;
            public:explicit Tx(Database& d):db(d){
                Q(db,"START TRANSACTION");
            }
            ~Tx(){
                if(active)try{
                    db.QueryDatabase(std::string("ROLLBACK"),false);
                }
                catch(...) {
                }
            }
            void Commit(){
                try{
                    auto r=db.QueryDatabase(std::string("COMMIT"),false);
                    if(!r.Success())throw Failure{
                        Error::CommitUncertain
                    };
                    active=false;
                }
                catch(...){
                    throw Failure{
                        Error::CommitUncertain
                    };
                }
            }
        };
        bool Key(const std::string& s){
            if(s.empty()||s.size()>96)return false;
            for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c==':'||c=='.'))return false;
            return true;
        }
        std::string Quote(Database& db,const std::string& s){
            return "'"+db.Escape(s)+"'";
        }
        std::string Hex(const Bytes& b){
            const char* h="0123456789abcdef";
            std::string s;
            for(auto c:b){
                s+=h[c>>4];
                s+=h[c&15];
            }
            return s;
        }
        Bytes Unhex(const char* p){
            if(!p)throw Failure{
                Error::DatabaseFailure
            };
            std::string s=p;
            if(s.size()%2||s.size()>std::max(size_t(8*CustodyCodec::MaximumPayload),2*RegionWire::MaximumPayload))throw Failure{
                Error::DatabaseFailure
            };
            Bytes b;
            auto n=[](char c)->uint8_t{
                if(c>='0'&&c<='9')return uint8_t(c-'0');
                if(c>='A'&&c<='F')return uint8_t(c-'A'+10);
                if(c>='a'&&c<='f')return uint8_t(c-'a'+10);
                throw Failure{
                    Error::DatabaseFailure
                };
            };
            for(size_t i=0;i<s.size();i+=2)b.push_back(uint8_t(n(s[i])*16+n(s[i+1])));
            return b;
        }
        void W(Bytes& b,uint64_t v,unsigned n){
            for(unsigned i=0;i<n;++i)b.push_back(uint8_t(v>>(8*i)));
        }
        void S(Bytes& b,const std::string& s){
            W(b,s.size(),4);
            b.insert(b.end(),s.begin(),s.end());
        }
        struct Reader{
            const Bytes& b;
            size_t at=0;
            uint64_t N(size_t n){
                if(n>b.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                uint64_t v=0;
                for(size_t i=0;i<n;++i)v|=uint64_t(b[at++])<<(8*i);
                return v;
            }
            std::string Text(size_t max){
                auto n=N(4);
                if(n>max||n>b.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                std::string s(b.begin()+at,b.begin()+at+n);
                at+=n;
                return s;
            }
            Bytes Blob(size_t max){
                auto n=N(4);
                if(n>max||n>b.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                Bytes out(b.begin()+at,b.begin()+at+n);
                at+=n;
                return out;
            }
        };
        bool Valid(const InventoryRow& r){return CustodyCodec::Valid(r);}
        Bytes Encode(const InventoryRow& row){return CustodyCodec::Encode(row);}
        InventoryRow Decode(const Bytes& bytes){try{return CustodyCodec::Decode(bytes);}catch(const std::invalid_argument&){throw Failure{Error::DatabaseFailure};}}
        void B(Bytes& b,const Bytes& v){
            W(b,v.size(),4);
            b.insert(b.end(),v.begin(),v.end());
        }
        Bytes Cat(const Catalog& c){
            Bytes b;
            for(auto n:{
                c.item_class,c.bag_slots,c.lore_group,c.no_drop
            })W(b,uint32_t(n),4);
            return b;
        }
        Catalog Uncat(const Bytes& b){
            if(b.size()!=16)throw Failure{
                Error::DatabaseFailure
            };
            Reader r{
                b
            };
            Catalog c;
            for(auto* n:{
                &c.item_class,&c.bag_slots,&c.lore_group,&c.no_drop
            }){
                auto v=uint32_t(r.N(4));
                std::memcpy(n,&v,4);
            }
            return c;
        }
        bool General(uint32_t slot){
            return slot==uint32_t(EQ::invslot::slotCursor)||(slot>=uint32_t(EQ::invslot::GENERAL_BEGIN)&&slot<=uint32_t(EQ::invslot::GENERAL_END));
        }
        void Verify(Database& db){
            auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_live_item_leases','housing_live_item_custody','housing_live_item_receipts','inventory','housing_estates','character_data','account','items','items_evolving_details','character_evolving_items')");
            if(engines.RowCount()!=1||std::stoul(engines.begin()[0])!=10)throw Failure{
                Error::DatabaseFailure
            };
            auto cols=Q(db,"SELECT column_name FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='inventory' ORDER BY ordinal_position");
            auto expected=InventoryRepository::Columns();
            if(cols.RowCount()!=expected.size())throw Failure{
                Error::UnsupportedItem
            };
            size_t i=0;
            for(auto row=cols.begin();row.operator!=(cols.end());++row)if(expected[i++]!=row[0])throw Failure{
                Error::UnsupportedItem
            };
        }
        void Actor(Database& db,AuthenticatedPurchaser actor){
            auto c=Q(db,"SELECT account_id,deleted_at FROM character_data WHERE id="+std::to_string(actor.character_id)+" FOR UPDATE");
            if(c.RowCount()!=1||c.begin()[1]||std::stoul(c.begin()[0])!=actor.account_id)throw Failure{
                Error::Unauthorized
            };
            auto a=Q(db,"SELECT id FROM account WHERE id="+std::to_string(actor.account_id)+" FOR UPDATE");
            if(a.RowCount()!=1)throw Failure{
                Error::Unauthorized
            };
        }
        void Estate(Database& db,EstateId estate,AuthenticatedPurchaser actor){
            auto e=Q(db,"SELECT owner_kind,owner_id FROM housing_current_estates WHERE id="+std::to_string(estate)+" FOR UPDATE");
            if(e.RowCount()!=1)throw Failure{
                Error::NotFound
            };
            if(std::stoul(e.begin()[0])!=uint32_t(OwnerKind::Character)||std::stoul(e.begin()[1])!=actor.character_id)throw Failure{
                Error::Unauthorized
            };
        }
        CurrentEstateAccess CurrentAccess(Database& db,EstateId estate,uint32_t native,
            AuthenticatedPurchaser actor,std::optional<AccessPolicy::Capability> required,bool retiring=false){
            if(!retiring&&!RentState::PlotActive(db,estate,native))throw Failure{Error::Unauthorized};
            // Preserve the original personal-owner storage/placement path even
            // when the optional typed ACL tables have not been installed.
            auto domain=Q(db,"SELECT owner_kind,owner_id,plot_id FROM housing_current_estates WHERE id="+std::to_string(estate)+" FOR UPDATE");
            if(domain.RowCount()!=1)throw Failure{Error::NotFound};
            if(std::stoul(domain.begin()[0])==uint32_t(OwnerKind::Character)
                &&std::stoul(domain.begin()[1])==actor.character_id){
                return {OwnerKind::Character,actor.character_id,uint32_t(std::stoul(domain.begin()[2])),0,{6,AccessPolicy::KnownCapabilities,true}};
            }
            if(retiring)throw Failure{Error::Unauthorized}; // Never delegate a retirement rent exception.
            auto current=LoadCurrentEstateAccessUnderTransaction(db,estate,native,actor);
            if(!current){
                if(current.error==Housing::Error::Unauthorized)throw Failure{Error::Unauthorized};
                if(current.error==Housing::Error::NotFound)throw Failure{Error::NotFound};
                throw Failure{Error::DatabaseFailure};
            }
            if(required&&!current.value.effective.Allows(*required))throw Failure{Error::Unauthorized};
            return current.value;
        }
        const std::string lease_columns="account_id,epoch,session_key,HEX(token),active,pending_key";
        Lease LeaseRow(MySQLRequestRow row,uint32_t id){
            Lease l;
            l.actor={
                id,uint32_t(std::stoul(row[0]))
            };
            l.epoch=std::stoull(row[1]);
            l.session_key=row[2];
            auto token=Unhex(row[3]);
            if(token.size()!=32)throw Failure{
                Error::DatabaseFailure
            };
            std::copy(token.begin(),token.end(),l.token.begin());
            l.pending_key=row[5]?row[5]:"";
            return l;
        }
        Lease Locked(Database& db,const Lease& input){
            Actor(db,input.actor);
            auto rows=Q(db,"SELECT "+lease_columns+" FROM housing_live_item_leases WHERE character_id="+std::to_string(input.actor.character_id)+" FOR UPDATE");
            if(rows.RowCount()!=1)throw Failure{
                Error::StaleLease
            };
            auto row=rows.begin();
            auto l=LeaseRow(row,input.actor.character_id);
            if(std::stoul(row[4])!=1||l.actor.account_id!=input.actor.account_id||l.epoch!=input.epoch||l.session_key!=input.session_key||l.token!=input.token)throw Failure{
                Error::StaleLease
            };
            const auto moving=HouseInventory::InspectPending(db,input.actor.character_id,true);
            if(moving!=HouseInventory::PendingState::Clear)throw Failure{moving==HouseInventory::PendingState::Pending?Error::PendingApplication:Error::DatabaseFailure};
            return l;
        }
        bool LeaseShape(const Lease& l){
            return l.actor.character_id&&l.actor.account_id&&l.epoch&&Key(l.session_key);
        }
        const std::string select_inventory=InventoryRowDB::Columns;
        InventoryRow InvRow(MySQLRequestRow row){try{return InventoryRowDB::Decode(row);}catch(...){throw Failure{Error::UnsupportedItem};}}
        std::string Slot(uint32_t character,uint32_t slot){
            return "character_id="+std::to_string(character)+" AND slot_id="+std::to_string(slot);
        }
        void NoChildren(Database& db,uint32_t actor,uint32_t slot){
            auto first=EQ::InventoryProfile::CalcSlotId(int16_t(slot),EQ::invbag::SLOT_BEGIN),last=EQ::InventoryProfile::CalcSlotId(int16_t(slot),EQ::invbag::SLOT_END);
            if(first<0||last<first)throw Failure{
                Error::UnsupportedItem
            };
            auto rows=Q(db,"SELECT slot_id FROM inventory WHERE character_id="+std::to_string(actor)+" AND slot_id BETWEEN "+std::to_string(first)+" AND "+std::to_string(last)+" FOR UPDATE");
            if(rows.RowCount())throw Failure{
                Error::UnsupportedItem
            };
        }
        Catalog Simple(Database& db,const InventoryRow& r,bool vault=false){
            const auto& v=r.values;
            if((r.null_fields&NullItemId)||!v.item_id)throw Failure{
                Error::UnsupportedItem
            };
            ItemPayload::Metadata(db,r);
            auto rows=Q(db,"SELECT itemclass,bagslots,loregroup,nodrop FROM items WHERE id="+std::to_string(v.item_id)+" FOR UPDATE");
            if(rows.RowCount()!=1)throw Failure{
                Error::UnsupportedItem
            };
            auto row=rows.begin();
            Catalog c{
                int32_t(std::stol(row[0])),int32_t(std::stol(row[1])),int32_t(std::stol(row[2])),int32_t(std::stol(row[3]))
            };
            if(vault){auto current=VaultStorage::Profile(db,r);if(Cat(current)!=Cat(c))throw Failure{Error::Conflict};}
            if(!vault&&(c.item_class!=0||c.bag_slots!=0))throw Failure{
                Error::UnsupportedItem
            };
            return c;
        }
        void ForeignCustody(Database& db,const Stored& stored){
            if(!stored.placement||!ForeignFurnitureReturn::Eligible(stored.row,stored.catalog))throw Failure{Error::UnsupportedItem};
            auto transfer=Q(db,"SELECT notransfer FROM items WHERE id="+std::to_string(stored.row.values.item_id)+" FOR UPDATE");if(transfer.RowCount()!=1||!transfer.begin()[0]||std::string(transfer.begin()[0])!="0")throw Failure{Error::UnsupportedItem};
            // A vault is a separately authenticated typed storage realm. Neither
            // missing pose assumptions nor foreign pickup may bypass its marker.
            auto schema=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_vault_items'");
            if(schema.RowCount()){if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB")throw Failure{Error::DatabaseFailure};if(Q(db,"SELECT custody_id FROM housing_live_vault_items WHERE custody_id="+std::to_string(stored.custody_id)+" FOR UPDATE").RowCount())throw Failure{Error::Unauthorized};}
            auto creator=Q(db,"SELECT pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(stored.row.values.character_id)+" FOR UPDATE");
            if(creator.RowCount()>1)throw Failure{Error::DatabaseFailure};if(creator.RowCount()&&creator.begin()[0])throw Failure{Error::PendingApplication};
        }
        const std::string stored_columns="estate_id,owner_character_id,schema_descriptor,HEX(row_blob),HEX(catalog_blob)";
        Stored StoredRow(Database& db,MySQLRequestRow row,uint64_t id){
            Stored s;
            s.custody_id=id;
            s.estate_id=uint32_t(std::stoul(row[0]));
            if(std::string(row[2])!=Service::SchemaDescriptor())throw Failure{
                Error::UnsupportedItem
            };
            s.row=Decode(Unhex(row[3]));
            s.catalog=Uncat(Unhex(row[4]));
            if(s.row.values.character_id!=std::stoul(row[1]))throw Failure{
                Error::DatabaseFailure
            };
            if(id)ItemPayload::ValidateHeld(db,s.row,id);
            return s;
        }
        Bytes Pose(const PlacementPolicy::Transform& t){Bytes b;for(float v:{t.heading,t.pitch,t.roll,t.scale,t.position[0],t.position[1],t.position[2]}){if(!std::isfinite(v))throw Failure{Error::InvalidArgument};uint32_t bits;std::memcpy(&bits,&v,4);W(b,bits,4);}return b;}
        PlacementPolicy::Transform Unpose(const Bytes& b){if(b.size()!=28)throw Failure{Error::DatabaseFailure};Reader p{b};std::array<float,7>a{};for(float& f:a){uint32_t bits=p.N(4);std::memcpy(&f,&bits,4);if(!std::isfinite(f))throw Failure{Error::DatabaseFailure};}return {a[0],a[1],a[2],a[3],{a[4],a[5],a[6]}};}
        Bytes PlacementBytes(const Placed& p){Bytes b;W(b,p.reservation.native_item_id,4);W(b,p.reservation.native_estate_id,4);W(b,p.reservation.estate_id,4);S(b,p.reservation.catalog_key);W(b,p.reservation.catalog_revision,8);W(b,p.pose_revision,8);B(b,Pose(p.transform));if(p.reservation.content_binding){if(!ContentBinding::SupportedVersion(p.reservation.content_binding->version))throw Failure{Error::InvalidArgument};W(b,p.reservation.content_binding->version,4);b.insert(b.end(),p.reservation.content_binding->digest.begin(),p.reservation.content_binding->digest.end());}if(p.npc_frame){NpcNativeFrame::Shape(*p.npc_frame,p.transform);W(b,0x4e504346,4);W(b,p.npc_frame->height_bits,4);W(b,p.npc_frame->origin_z_bits,4);}return b;}
        Placed Unplacement(const Bytes& b){Reader r{b};Placed p;p.reservation.native_item_id=r.N(4);p.reservation.native_estate_id=r.N(4);p.reservation.estate_id=r.N(4);p.reservation.catalog_key=r.Text(96);p.reservation.catalog_revision=r.N(8);p.pose_revision=r.N(8);p.transform=Unpose(r.Blob(28));const auto remaining=b.size()-r.at;if(remaining==36||remaining==48){ContentBinding::Identity id;id.version=r.N(4);if(!ContentBinding::SupportedVersion(id.version))throw Failure{Error::DatabaseFailure};for(auto& byte:id.digest)byte=r.N(1);p.reservation.content_binding=id;}if(b.size()-r.at==12){if(r.N(4)!=0x4e504346)throw Failure{Error::DatabaseFailure};p.npc_frame=NpcNativeFrame::Frame{uint32_t(r.N(4)),uint32_t(r.N(4))};NpcNativeFrame::Shape(*p.npc_frame,p.transform);}if(r.at!=b.size()||!p.reservation.native_item_id||!p.pose_revision)throw Failure{Error::DatabaseFailure};return p;}

        void PlacementTables(Database& db){auto t=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_live_placement_reservations','housing_live_placed_items','housing_catalogs','housing_native_estates')");if(t.RowCount()!=1||std::stoul(t.begin()[0])!=4)throw Failure{Error::DatabaseFailure};}
        bool HouseStorageInstalled(Database& db){auto rows=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_house_storage_reservations'");if(!rows.RowCount())return false;if(rows.RowCount()!=1||!rows.begin()[0]||std::string(rows.begin()[0])!="InnoDB")throw Failure{Error::DatabaseFailure};return true;}
        bool HouseStorageReservation(Database& db,uint32_t id){return HouseStorageInstalled(db)&&Q(db,"SELECT native_item_id FROM housing_house_storage_reservations WHERE native_item_id="+std::to_string(id)+" FOR UPDATE").RowCount()==1;}
        // Caller has already locked the acting lease; current typed operation access
        // is freshly resolved here under the same transaction as the pose/source CAS.
        uint64_t PlacementCatalog(Database& db,const Reservation& p,const InventoryRow& row,const std::optional<PlacementPolicy::Transform>& pose,bool capacity,InventorySessionFence& fence,AuthenticatedPurchaser actor,PlacementPolicy::Operation operation=PlacementPolicy::Operation::Add,ContentBinding::Identity* bind_new=nullptr,uint64_t expected_current_revision=0,HouseBinding::CustodyContext* houses=nullptr,const Lease* lease=nullptr,HouseBinding::PlacementStage stage=HouseBinding::PlacementStage::Preview,int32_t runtime_serial=0,CrateBinding::CustodyContext* crates=nullptr,const Scene::Snapshot* parent_yard=nullptr,const std::optional<NpcNativeFrame::Frame>& npc_frame={},bool house_storage=false,const MountSubject& mount_subject={}){
            if(!p.native_estate_id||!p.estate_id||!Key(p.catalog_key)||!p.catalog_revision)throw Failure{Error::InvalidArgument};
            auto npc_source=NpcItemMetadata::ReadUnderTransaction(db,row.values.item_id);auto npc=npc_source?NpcRuntime::Review(*npc_source):std::nullopt;
            if(npc_source&&!NpcItemMetadata::Ordinary(*npc_source)&&!npc)throw Failure{Error::UnsupportedItem};
            if(npc&&(!lease||(!p.content_binding&&!bind_new)||!fence.NpcProfileMatchesUnderTransaction(db,*lease,*npc)))throw Failure{Error::Conflict};
            const HouseGeometry::Record* house=nullptr;
            for(const auto& candidate:HouseGeometry::Records)if(candidate.item_id==row.values.item_id)house=&candidate;
            if(house&&(!houses||!lease||!fence.HeldFor(lease->actor,lease->epoch,lease->session_key)||!houses->Templates().VerifiedHouse(row)||(!p.content_binding&&!bind_new)))throw Failure{Error::UnsupportedItem};
            if(house_storage&&(!house||pose))throw Failure{Error::InvalidArgument};
            const bool crate=CrateGeometry::TemplateDefinition(row.values.item_id)!=0;
            if(crate&&(!crates||house||!lease||!fence.HeldFor(lease->actor,lease->epoch,lease->session_key)||(!p.content_binding&&!bind_new)))throw Failure{Error::UnsupportedItem};
            auto rent_source=Q(db,"SELECT custody_id FROM housing_live_placed_items WHERE native_item_id="+std::to_string(p.native_item_id)+" FOR UPDATE");if(rent_source.RowCount()>1||(rent_source.RowCount()&&!RentState::HouseActive(db,std::stoull(rent_source.begin()[0]))))throw Failure{Error::Unauthorized};
            auto access=CurrentAccess(db,p.estate_id,p.native_estate_id,actor,
                operation==PlacementPolicy::Operation::Add?(house?AccessPolicy::Capability::AddHouse:crate?AccessPolicy::Capability::AddCrate:AccessPolicy::Capability::AddItem):AccessPolicy::Capability::MoveItem);
            auto cat=Q(db,"SELECT revision,origin,HEX(definitions_blob),HEX(regions_blob),client_build FROM housing_catalogs WHERE catalog_key="+Quote(db,p.catalog_key)+" FOR UPDATE");if(cat.RowCount()!=1||(!p.content_binding&&std::stoull(cat.begin()[0])!=p.catalog_revision)||(expected_current_revision&&std::stoull(cat.begin()[0])!=expected_current_revision)||std::stoul(cat.begin()[4])!=20130510)throw Failure{Error::Conflict};
            if(std::stoul(cat.begin()[1])==2){if(!RuntimeActivation::AllowsReconstructedContent(db))throw Failure{Error::Unauthorized};}else if(std::stoul(cat.begin()[1])!=1)throw Failure{Error::UnsupportedItem};
            auto bundle=Housing::Catalog::DecodePayloads(Unhex(cat.begin()[2]),Unhex(cat.begin()[3]));
            auto map=Q(db,"SELECT n.definition_id,n.neighborhood_id,p.definition_id,h.zone_id,h.zone_version,h.instance_id,i.zone,i.version,i.never_expires,n.plot_id,e.plot_id FROM housing_native_estates n JOIN housing_native_estates p ON p.native_id=n.parent_native_id AND p.active=1 AND p.parent_native_id IS NULL AND p.catalog_key=n.catalog_key AND p.neighborhood_id=n.neighborhood_id JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN instance_list i ON i.id=h.instance_id JOIN housing_current_estates e ON e.id=n.ownership_estate_id WHERE n.native_id="+std::to_string(p.native_estate_id)+" AND n.catalog_key="+Quote(db,p.catalog_key)+" AND n.active=1 AND n.ownership_estate_id="+std::to_string(p.estate_id)+" FOR UPDATE");
            if(map.RowCount()!=1)throw Failure{Error::Unauthorized};auto m=map.begin();if(std::stoul(m[3])!=std::stoul(m[6])||std::stoul(m[4])!=std::stoul(m[7])||std::stoul(m[8])!=1||!std::stoul(m[5])||std::stoul(m[5])>32000||std::stoul(m[9])!=std::stoul(m[10]))throw Failure{Error::Conflict};
            bool occupied_parent=false;
            if(parent_yard){occupied_parent=lease&&!house&&fence.HeldFor(lease->actor,lease->epoch,lease->session_key)&&parent_yard->estate_id==p.estate_id&&parent_yard->native_plot_id==p.native_estate_id&&parent_yard->current_catalog_revision==std::stoull(cat.begin()[0])&&parent_yard->route.zone_id==std::stoul(m[3])&&parent_yard->route.zone_version==std::stoul(m[4])&&parent_yard->route.instance_id==std::stoul(m[5]);if(!occupied_parent)throw Failure{Error::Conflict};}
            if(!fence.PlacementContextMatches(p.estate_id,p.native_estate_id,uint16_t(std::stoul(m[5])))&&!occupied_parent)throw Failure{Error::UnheldFence};
            const EstateWire::Definition *definition=nullptr,*root=nullptr;for(const auto& d:bundle.definitions){if(d.id==std::stoul(m[0]))definition=&d;if(d.id==std::stoul(m[2]))root=&d;}
            if(!definition||!root||((access.owner_kind==OwnerKind::Character&&definition->type!=4)||(access.owner_kind==OwnerKind::Guild&&definition->type!=8))||root->type!=5||root->zone_id!=std::stoul(m[3])||std::find(root->children.begin(),root->children.end(),definition->id)==root->children.end())throw Failure{Error::UnsupportedItem};
            const RegionWire::Region* region=nullptr;for(const auto& r:bundle.regions)if(r.id==definition->area_id)region=&r;if(!region)throw Failure{Error::UnsupportedItem};
            auto meta=Q(db,"SELECT idfile,UNK224,UNK228,UNK229,UNK227,UNK231 FROM items WHERE id="+std::to_string(row.values.item_id)+" FOR UPDATE");const auto* supported=!npc&&!house&&!crate&&meta.RowCount()==1&&meta.begin()[0]?StaticFurniture::Lookup(row.values.item_id,std::string(meta.begin()[0])):nullptr;
            const auto* crate_model=crate&&meta.RowCount()==1&&meta.begin()[0]&&meta.begin()[4]?CrateGeometry::Lookup(row.values.item_id,uint32_t(std::stoul(meta.begin()[4])),meta.begin()[0]):nullptr;
            if(house){if(meta.RowCount()!=1||!meta.begin()[0]||!meta.begin()[1]||!meta.begin()[2]||!meta.begin()[3]||!meta.begin()[4]||std::string(meta.begin()[0])!=house->model)throw Failure{Error::UnsupportedItem};}
            else if(!npc&&(crate?!crate_model:!supported))throw Failure{Error::UnsupportedItem};
            if(npc&&(house||crate||meta.RowCount()!=1||!meta.begin()[1]||!meta.begin()[2]||!meta.begin()[3]||!meta.begin()[4]))throw Failure{Error::UnsupportedItem};
            uint32_t minbits=std::stoul(meta.begin()[2]),maxbits=std::stoul(meta.begin()[3]);float minimum,maximum;std::memcpy(&minimum,&minbits,4);std::memcpy(&maximum,&maxbits,4);
            PlacementPolicy::ItemMetadata metadata{row.values.item_id,npc?npc->model_id:house?house->model_id:crate?crate_model->model_id:supported->model_id,uint32_t(std::stoul(meta.begin()[1])),minimum,maximum,bool(supported),house?(HouseTemplates::Type(row.values.item_id)):crate?7u:0u};
            auto geometry=npc?NpcRuntime::Geometry(*npc):house?house->geometry:crate?crate_model->geometry:supported->geometry;
            std::optional<ContentBinding::HouseRoute> house_route;
            std::optional<ContentBinding::CrateProfile> crate_profile;
            uint64_t placed_children=0;
            if(house){
                const EstateWire::Definition* child=nullptr;
                const auto child_id=std::stoul(meta.begin()[4]);
                for(const auto& d:bundle.definitions)if(d.id==child_id)child=&d;
                if(!child||child->type!=metadata.house_definition_type||!definition->max_children||std::find(definition->children.begin(),definition->children.end(),child_id)==definition->children.end())throw Failure{Error::UnsupportedItem};
                auto occupied=Q(db,"SELECT r.content_binding_version,l.house_id,h.item_template_id,h.pending_request_key,c.owner_character_id,HEX(c.row_blob),i.native_estate_id,n.parent_native_id,n.active,n.catalog_key,h.created_by_character_id,placed.custody_id FROM housing_live_placed_items placed JOIN housing_live_placement_reservations r ON r.native_item_id=placed.native_item_id LEFT JOIN housing_live_item_custody c ON c.custody_id=placed.custody_id LEFT JOIN housing_house_locations l ON l.location_kind=2 AND l.custody_id=placed.custody_id LEFT JOIN housing_house_instances h ON h.house_id=l.house_id LEFT JOIN housing_house_interiors i ON i.house_id=h.house_id LEFT JOIN housing_native_estates n ON n.native_id=i.native_estate_id WHERE r.native_estate_id="+std::to_string(p.native_estate_id)+" AND r.state=1"+(operation==PlacementPolicy::Operation::Move?" AND placed.native_item_id<>"+std::to_string(p.native_item_id):std::string{})+" FOR UPDATE");
                for(auto existing=occupied.begin();existing.operator!=(occupied.end());++existing){
                    if(!existing[4]||!existing[5])throw Failure{Error::DatabaseFailure};
                    const auto stored_row=Decode(Unhex(existing[5]));bool known_house=false;
                    for(const auto& candidate:HouseGeometry::Records)if(candidate.item_id==stored_row.values.item_id)known_house=true;
                    const bool tagged=existing[0]&&std::stoul(existing[0])==ContentBinding::HouseVersion;
                    if(!tagged&&!existing[1]&&!known_house)continue;
                    if(!tagged||!known_house||!existing[1]||!existing[2]||existing[3]||!existing[6]||!existing[7]||!existing[8]||!existing[9]||!existing[10]||std::stoul(existing[10])!=stored_row.values.character_id||std::stoul(existing[2])!=stored_row.values.item_id||std::stoul(existing[4])!=stored_row.values.character_id||std::stoul(existing[7])!=p.native_estate_id||std::stoul(existing[8])!=1||std::string(existing[9])!=p.catalog_key)throw Failure{Error::Conflict};
                    if(ItemStorage::Locked(db,std::stoull(existing[11])).stowed||CrateMemberScope::Locked(db,std::stoull(existing[11])))continue;
                    ++placed_children;
                }
                // Live permits one placed house/hall; max_children also budgets moving crates.
                if(capacity&&!house_storage&&placed_children>=1)throw Failure{Error::Conflict};
                HouseBinding::PlacementFacts facts;facts.lease=*lease;facts.reservation=p;facts.source=row;facts.runtime_serial=runtime_serial;facts.pose=pose;facts.operation=operation;facts.stage=stage;facts.catalog_revision=std::stoull(cat.begin()[0]);facts.placed_children=placed_children;facts.storage_only=house_storage;facts.neighborhood_instance=uint16_t(std::stoul(m[5]));facts.access=access.effective;facts.plot_definition=*definition;facts.house_definition=*child;facts.region=*region;
                house_route=houses->ReviewPlacementUnderTransaction(db,fence,facts);
                if(!house_route)throw Failure{Error::Unauthorized};
            }
            if(crate){
                const auto id=uint32_t(std::stoul(meta.begin()[4]));const EstateWire::Definition* child=nullptr;
                for(const auto& d:bundle.definitions)if(d.id==id)child=&d;
                if(!child||child->type!=7||!definition->max_children||std::find(definition->children.begin(),definition->children.end(),id)==definition->children.end())throw Failure{Error::UnsupportedItem};
                CrateBinding::ContentFacts facts;facts.lease=*lease;facts.reservation=p;facts.source=row;facts.runtime_serial=runtime_serial;facts.pose=pose;facts.stage=stage==HouseBinding::PlacementStage::Preview?CrateBinding::Stage::Preview:stage==HouseBinding::PlacementStage::Store?CrateBinding::Stage::Store:stage==HouseBinding::PlacementStage::Read?CrateBinding::Stage::Read:CrateBinding::Stage::Move;facts.catalog_revision=std::stoull(cat.begin()[0]);facts.neighborhood_instance=uint16_t(std::stoul(m[5]));facts.access=access;facts.plot=*definition;facts.crate=*child;facts.region=*region;
                crate_profile=crates->ReviewContentUnderTransaction(db,fence,facts);
                const auto digest=crates->ManifestDigest();
                if(!crate_profile||crate_profile->definition_id!=id||crate_profile->content_manifest_sha256!=Hex(Bytes(digest.begin(),digest.end())))throw Failure{Error::Unauthorized};
            }
            if(p.content_binding||bind_new){auto identity=ContentBinding::Current(db,p.catalog_key,bundle,p.native_estate_id,{uint32_t(std::stoul(m[3])),uint8_t(std::stoul(m[4])),uint16_t(std::stoul(m[5]))},row.values.item_id,house_route?&*house_route:nullptr,crate_profile?&*crate_profile:nullptr);if(p.content_binding&&!ContentBinding::Same(*p.content_binding,identity))throw Failure{Error::Conflict};if(bind_new)*bind_new=identity;}
            if(capacity&&operation==PlacementPolicy::Operation::Add&&!house&&!crate){
                if(!meta.begin()[5])throw Failure{Error::UnsupportedItem};size_t parsed=0;const std::string raw=meta.begin()[5];const auto maximum=std::stoll(raw,&parsed);
                if(parsed!=raw.size()||maximum<INT32_MIN||maximum>INT32_MAX)throw Failure{Error::UnsupportedItem};
                if(!FurnitureEstateLimit::AvailableUnderTransaction(db,p.native_estate_id,row.values.item_id,int32_t(maximum)))throw Failure{Error::Conflict};
            }
            if(npc){if(pose)geometry=NpcRuntime::Geometry(*npc,npc_frame,*pose);else if(stage!=HouseBinding::PlacementStage::Preview&&!npc->legacy_fixed_eye&&!npc_frame)throw Failure{Error::UnsupportedItem};}else if(npc_frame)throw Failure{Error::InvalidArgument};
            if(pose){PlacementPolicy::Context c{p.native_estate_id,definition->type,region->id,access.effective,true};if(geometry.mounting_type){if(stage==HouseBinding::PlacementStage::Read)c.geometry_only=true;else c.mount_contact_verified=lease&&fence.MountSurfaceWithSubjectUnderTransaction(db,*lease,row,*pose,p.native_estate_id,stage==HouseBinding::PlacementStage::Move?mount_subject:MountSubject::NewSource(p.native_item_id));}if(!PlacementPolicy::Check(c,metadata,geometry,*region,*pose,operation))throw Failure{Error::UnsupportedItem};}
            auto used=Q(db,"SELECT COUNT(*) FROM housing_live_placed_items p JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE r.native_estate_id="+std::to_string(p.native_estate_id)+" AND r.state=1"+(CrateMemberScope::Installed(db)?" AND NOT EXISTS (SELECT 1 FROM housing_crate_members packed WHERE packed.custody_id=p.custody_id)":"")+" FOR UPDATE");if((!house&&!crate||house_storage)&&capacity&&(!definition->max_items||std::stoull(used.begin()[0])+(VaultStorage::Installed(db)?VaultStorage::Count(db,p.native_estate_id):0)>=definition->max_items))throw Failure{Error::Conflict};if(npc&&!npc->service&&capacity&&!NpcRuntime::Capacity(definition->max_npc_items,NpcRuntime::Count(db,p.native_estate_id)))throw Failure{Error::NpcFull};return std::stoull(cat.begin()[0]);
        }
        void CurrentRemovalContext(Database& db,const Placed& placed,InventorySessionFence& fence){
            const auto& p=placed.reservation;
            auto rows=Q(db,"SELECT h.instance_id,h.zone_id,h.zone_version,i.zone,i.version,i.never_expires,n.definition_id,parent.definition_id,HEX(c.definitions_blob),HEX(c.regions_blob),e.owner_kind,c.origin,c.client_build FROM housing_native_estates n JOIN housing_native_estates parent ON parent.native_id=n.parent_native_id AND parent.active=1 AND parent.parent_native_id IS NULL AND parent.catalog_key=n.catalog_key AND parent.neighborhood_id=n.neighborhood_id JOIN housing_catalogs c ON c.catalog_key=n.catalog_key JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN instance_list i ON i.id=h.instance_id JOIN housing_current_estates e ON e.id=n.ownership_estate_id AND e.plot_id=n.plot_id WHERE n.native_id="+std::to_string(p.native_estate_id)+" AND n.ownership_estate_id="+std::to_string(p.estate_id)+" AND n.catalog_key="+Quote(db,p.catalog_key)+" AND n.active=1 FOR UPDATE");
            if(rows.RowCount()!=1)throw Failure{Error::Unauthorized};auto row=rows.begin();
            auto instance=std::stoul(row[0]);if(!instance||instance>32000||std::stoul(row[1])!=std::stoul(row[3])||std::stoul(row[2])!=std::stoul(row[4])||std::stoul(row[5])!=1)throw Failure{Error::Conflict};
            if(std::stoul(row[12])!=20130510)throw Failure{Error::UnsupportedItem};
            auto origin=std::stoul(row[11]);if(origin==2){if(!RuntimeActivation::AllowsReconstructedContent(db))throw Failure{Error::Unauthorized};}else if(origin!=1)throw Failure{Error::UnsupportedItem};
            auto bundle=Housing::Catalog::DecodePayloads(Unhex(row[8]),Unhex(row[9]));const EstateWire::Definition *definition=nullptr,*root=nullptr;for(const auto& d:bundle.definitions){if(d.id==std::stoul(row[6]))definition=&d;if(d.id==std::stoul(row[7]))root=&d;}
            auto kind=std::stoul(row[10]);if(!definition||!root||root->type!=5||root->zone_id!=std::stoul(row[1])||((kind==1&&definition->type!=4)||(kind==3&&definition->type!=8))||(kind!=1&&kind!=3)||std::find(root->children.begin(),root->children.end(),definition->id)==root->children.end())throw Failure{Error::UnsupportedItem};
            if(!fence.PlacementContextMatches(p.estate_id,p.native_estate_id,uint16_t(instance)))throw Failure{Error::UnheldFence};
        }
        Reservation ReservationRow(MySQLRequestRow row,size_t offset=13){Reservation r{uint32_t(std::stoul(row[0])),uint32_t(std::stoul(row[1])),uint32_t(std::stoul(row[2])),row[3],std::stoull(row[4])};r.content_binding=ContentBinding::DecodeIdentity(row[offset],row[offset+1]);return r;}
        const std::string reservation_base_columns="native_item_id,native_estate_id,estate_id,catalog_key,catalog_revision,character_id,account_id,lease_epoch,session_key,HEX(source_row),runtime_serial,state,custody_id";
        std::string ReservationColumns(Database& db){return reservation_base_columns+","+ContentBinding::SelectColumns(db);}
        void HeldReservation(MySQLRequestRow row,const Lease& lease){if(std::stoul(row[5])!=lease.actor.character_id||std::stoul(row[6])!=lease.actor.account_id)throw Failure{Error::Unauthorized};if(std::stoull(row[7])!=lease.epoch||std::string(row[8])!=lease.session_key)throw Failure{Error::StaleLease};}
        void StorePose(Database& db,const Request& request,Stored& stored,InventorySessionFence& fence,HouseBinding::CustodyContext* houses,CrateBinding::CustodyContext* crates){
            const auto& placed=*request.placement;auto rows=Q(db,"SELECT "+ReservationColumns(db)+" FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(placed.reservation.native_item_id)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::NotFound};auto row=rows.begin();HeldReservation(row,request.lease);auto reserved=ReservationRow(row);
            if(std::stoul(row[11])||reserved.estate_id!=request.estate_id||PlacementBytes({reserved,placed.transform,1,placed.npc_frame})!=PlacementBytes(placed)||Unhex(row[9])!=Encode(request.expected)||std::stol(row[10])!=request.verified_runtime_serial)throw Failure{Error::Conflict};
            const bool house_storage=request.house&&!request.crate_member&&request.initial_stowed;
            if(request.house&&!request.crate_member&&HouseStorageReservation(db,reserved.native_item_id)!=house_storage)throw Failure{Error::Conflict};
            PlacementCatalog(db,reserved,stored.row,request.initial_stowed?std::nullopt:std::optional<PlacementPolicy::Transform>(placed.transform),!request.crate_member,fence,request.lease.actor,PlacementPolicy::Operation::Add,nullptr,0,houses,&request.lease,HouseBinding::PlacementStage::Store,request.verified_runtime_serial,crates,nullptr,placed.npc_frame,house_storage);
            Q(db,"INSERT INTO housing_live_placed_items(custody_id,native_item_id,pose_revision,pose_blob) VALUES ("+std::to_string(stored.custody_id)+","+std::to_string(reserved.native_item_id)+",1,x'"+Hex(Pose(placed.transform))+"')");
            auto update=Q(db,"UPDATE housing_live_placement_reservations SET state=1,custody_id="+std::to_string(stored.custody_id)+" WHERE native_item_id="+std::to_string(reserved.native_item_id)+" AND state=0");if(update.RowsAffected()!=1)throw Failure{Error::Conflict};stored.placement=placed;
        }
        std::optional<Placed> CurrentPose(Database& db,uint64_t custody,bool contained=false){
            if(CrateMemberScope::Locked(db,custody,contained)&&!contained)throw Failure{Error::Unauthorized};
            // LEFT JOIN makes an invalid existing placed record an error rather
            // than silently treating it as unplaced legacy storage on Return.
            auto rows=Q(db,"SELECT r.native_item_id,r.native_estate_id,r.estate_id,r.catalog_key,r.catalog_revision,r.character_id,r.account_id,r.lease_epoch,r.session_key,HEX(r.source_row),r.runtime_serial,r.state,r.custody_id,p.pose_revision,HEX(p.pose_blob),"+ContentBinding::SelectColumns(db,"r.")+",c.estate_id,c.owner_character_id,HEX(c.row_blob) FROM housing_live_placed_items p LEFT JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id LEFT JOIN housing_live_item_custody c ON c.custody_id=p.custody_id WHERE p.custody_id="+std::to_string(custody)+" FOR UPDATE");
            if(!rows.RowCount())return {};if(rows.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto row=rows.begin();
            if(!row[0]||!row[11]||!row[12]||!row[17]||!row[18]||std::stoul(row[11])!=1||std::stoull(row[12])!=custody||std::stoul(row[2])!=std::stoul(row[17])||std::stoul(row[5])!=std::stoul(row[18])||Unhex(row[9])!=Unhex(row[19]))throw Failure{Error::DatabaseFailure};
            auto reservation=ReservationRow(row,15);auto revision=std::stoull(row[13]);if(!reservation.native_item_id||reservation.native_item_id==UINT32_MAX||!reservation.native_estate_id||reservation.native_estate_id==UINT32_MAX||!revision)throw Failure{Error::DatabaseFailure};
            Placed placed{std::move(reservation),Unpose(Unhex(row[14])),revision};placed.npc_frame=NpcNativeFrame::ReadUnderTransaction(db,custody,placed.reservation.native_item_id,revision,placed.transform);return placed;
        }
        bool HasHouseEffect(Database& db,const std::string& key){
            auto schema=Q(db,"SELECT COUNT(*),SUM(engine='InnoDB') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_house_instances','housing_house_locations','housing_house_interiors','housing_house_receipt_effects')");
            if(!std::stoul(schema.begin()[0]))return false;
            if(std::stoul(schema.begin()[0])!=4||!schema.begin()[1]||std::stoul(schema.begin()[1])!=4)throw Failure{Error::DatabaseFailure};
            auto rows=Q(db,"SELECT house_id FROM housing_house_receipt_effects WHERE request_key="+Quote(db,key)+" FOR UPDATE");
            if(rows.RowCount()>1)throw Failure{Error::DatabaseFailure};
            return rows.RowCount()==1;
        }
        void AttachHouse(Database& db,const Lease& lease,Receipt& receipt,InventorySessionFence& fence,HouseBinding::CustodyContext* context){
            const auto present=HasHouseEffect(db,receipt.request_key);
            if(present!=receipt.house_effect_required)throw Failure{Error::DatabaseFailure};
            if(!present)return;
            if(!context)throw Failure{Error::UnsupportedItem};
            HouseBinding::TransactionHelpers helper(db,fence,context->Authorization());
            receipt.house_effect=std::make_shared<HouseBinding::Effect>(helper.Resolve(lease,receipt));
        }
        void MatchHouseRequest(const Request& request,const Receipt& receipt){
            if(bool(request.house)!=bool(receipt.house_effect))throw Failure{Error::Conflict};
            if(!request.house)return;
            const auto& expected=*request.house;const auto& effect=*receipt.house_effect;
            if(expected.house_id){
                if(effect.before.house_id!=expected.house_id||effect.before.revision!=expected.expected_revision||effect.minted)throw Failure{Error::Conflict};
            }else if(expected.expected_revision||!effect.minted||request.action!=Action::Store)throw Failure{Error::Conflict};
        }
        void RejectUnadaptedHouse(Database& db,const Request& request){
            auto schema=Q(db,"SELECT COUNT(*),SUM(engine='InnoDB') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_house_instances','housing_house_locations','housing_house_receipt_effects','housing_house_interiors')");
            if(!std::stoul(schema.begin()[0]))return;
            if(std::stoul(schema.begin()[0])!=4||!schema.begin()[1]||std::stoul(schema.begin()[1])!=4)throw Failure{Error::DatabaseFailure};
            auto where=request.action==Action::Store?
                "location_kind=1 AND character_id="+std::to_string(request.lease.actor.character_id)+" AND slot_id="+std::to_string(request.expected.values.slot_id):
                "location_kind=2 AND custody_id="+std::to_string(request.custody_id);
            if(Q(db,"SELECT house_id FROM housing_house_locations WHERE "+where+" FOR UPDATE").RowCount())throw Failure{Error::UnsupportedItem};
        }
        Bytes RoomMoveBytes(const InteriorMoveContext& r){if(!r.house_id||!r.house_revision||!r.profile_revision||!r.scope_revision||!r.source.zone_id||!r.source.instance_id||r.source.instance_id>32000)throw Failure{Error::InvalidArgument};Bytes b;W(b,r.house_id,8);W(b,r.house_revision,8);W(b,r.profile_revision,8);W(b,r.scope_revision,8);W(b,r.source.zone_id,4);W(b,r.source.zone_version,1);W(b,r.source.instance_id,2);return b;}
        InteriorMoveContext UnroomMove(const Bytes& bytes){Reader r{bytes};InteriorMoveContext out;out.house_id=r.N(8);out.house_revision=r.N(8);out.profile_revision=r.N(8);out.scope_revision=r.N(8);out.source={uint32_t(r.N(4)),uint8_t(r.N(1)),uint16_t(r.N(2))};if(r.at!=bytes.size()||RoomMoveBytes(out)!=bytes)throw Failure{Error::DatabaseFailure};return out;}
        Bytes RoomBytes(const InteriorReservation& room){
            if(!room.house_id||!room.house_revision||!room.profile_revision||!room.source.zone_id||!room.source.instance_id||room.source.instance_id>32000)throw Failure{Error::InvalidArgument};
            Bytes b;W(b,room.house_id,8);W(b,room.house_revision,8);W(b,room.profile_revision,8);W(b,room.source.zone_id,4);W(b,room.source.zone_version,1);W(b,room.source.instance_id,2);b.insert(b.end(),room.content_digest.begin(),room.content_digest.end());return b;
        }
        InteriorReservation Unroom(const Bytes& bytes,const Reservation& placement){Reader r{bytes};InteriorReservation room;room.placement=placement;room.house_id=r.N(8);room.house_revision=r.N(8);room.profile_revision=r.N(8);room.source={uint32_t(r.N(4)),uint8_t(r.N(1)),uint16_t(r.N(2))};for(auto& byte:room.content_digest)byte=r.N(1);if(r.at!=bytes.size()||RoomBytes(room)!=bytes)throw Failure{Error::DatabaseFailure};return room;}
        void MatchRoomTag(Database& db,const InteriorReservation& room){
            auto rows=Q(db,"SELECT house_id,house_revision,profile_revision,source_zone,source_version,source_instance,content_version,HEX(content_digest) FROM housing_interior_placement_reservations WHERE native_item_id="+std::to_string(room.placement.native_item_id)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::Conflict};auto row=rows.begin();for(size_t i=0;i<8;++i)if(!row[i])throw Failure{Error::Conflict};
            if(std::stoull(row[0])!=room.house_id||std::stoull(row[1])!=room.house_revision||std::stoull(row[2])!=room.profile_revision||std::stoul(row[3])!=room.source.zone_id||std::stoul(row[4])!=room.source.zone_version||std::stoul(row[5])!=room.source.instance_id||std::stoul(row[6])!=InteriorContent::Version||Unhex(row[7])!=Bytes(room.content_digest.begin(),room.content_digest.end()))throw Failure{Error::Conflict};
        }
        void AttachRoom(Database& db,Receipt& receipt){
            auto schema=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_interior_placement_reservations' AND engine='InnoDB'");bool installed=schema.RowCount()==1&&std::stoul(schema.begin()[0])==1;
            if(!installed){if(receipt.room)throw Failure{Error::DatabaseFailure};return;}
            bool present=false;if(receipt.stored.placement){auto row=Q(db,"SELECT native_item_id FROM housing_interior_placement_reservations WHERE native_item_id="+std::to_string(receipt.stored.placement->reservation.native_item_id)+" FOR UPDATE");present=row.RowCount()==1;}
            if(present!=bool(receipt.room))throw Failure{Error::DatabaseFailure};if(!receipt.room)return;
            MatchRoomTag(db,*receipt.room);
            if(receipt.requires_application){auto scope=HouseItemScope::Locked(db,receipt.stored.custody_id);auto pose=CurrentPose(db,receipt.stored.custody_id,bool(receipt.crate_member));
                if(receipt.action==Action::Store){if(!scope||scope->house_id!=receipt.room->house_id||scope->revision!=1||!pose||!receipt.stored.placement||PlacementBytes(*pose)!=PlacementBytes(*receipt.stored.placement))throw Failure{Error::Conflict};}
                else if(receipt.partition){const auto& part=*receipt.partition;if(!part.room_source||!scope||scope->house_id!=part.room_source->house_id||scope->revision!=part.room_source->scope_revision||!pose||!receipt.stored.placement||PlacementBytes(*pose)!=PlacementBytes(*receipt.stored.placement))throw Failure{Error::Conflict};}
                else{if(scope||pose||Q(db,"SELECT custody_id FROM housing_live_item_custody WHERE custody_id="+std::to_string(receipt.stored.custody_id)+" FOR UPDATE").RowCount())throw Failure{Error::Conflict};auto base=Q(db,"SELECT state,custody_id FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(receipt.room->placement.native_item_id)+" FOR UPDATE");if(base.RowCount()!=1||!base.begin()[0]||std::stoul(base.begin()[0])!=2||base.begin()[1])throw Failure{Error::Conflict};}
            }
        }
        Bytes MemberContextBytes(const CrateMemberContext& m){
            if(!m.crate_id||!m.revision||m.revision==UINT64_MAX||!m.native_plot_id||m.native_plot_id==UINT32_MAX||!m.catalog_revision||!m.source.zone_id||!m.source.instance_id||m.source.instance_id>32000)throw Failure{Error::InvalidArgument};
            Bytes b;W(b,m.crate_id,8);W(b,m.revision,8);W(b,m.native_plot_id,4);W(b,m.catalog_revision,8);W(b,m.source.zone_id,4);W(b,m.source.zone_version,1);W(b,m.source.instance_id,2);return b;
        }
        CrateMemberContext UnmemberContext(const Bytes& bytes){Reader b{bytes};CrateMemberContext m;m.crate_id=b.N(8);m.revision=b.N(8);m.native_plot_id=b.N(4);m.catalog_revision=b.N(8);m.source={uint32_t(b.N(4)),uint8_t(b.N(1)),uint16_t(b.N(2))};if(b.at!=bytes.size()||MemberContextBytes(m)!=bytes)throw Failure{Error::DatabaseFailure};return m;}
        uint32_t StackCount(const InventoryRow& row){if((row.null_fields&NullCharges)||!row.values.charges||row.values.charges>=32767)throw Failure{Error::UnsupportedItem};return row.values.charges;}
        uint32_t PartitionMaximum(Database& db,const Request& r,const InventoryRow& row){
            if(!row.children.empty()||!row.evolving_payload.empty()||row.values.augment_one||row.values.augment_two||row.values.augment_three||row.values.augment_four||row.values.augment_five||row.values.augment_six||!r.partition_quantity||!r.partition_serial||r.partition_serial>INT32_MAX||r.partition_serial==row.values.guid||r.house||r.crate||(r.placement&&r.placement->npc_frame)||row.values.character_id!=r.lease.actor.character_id)throw Failure{Error::UnsupportedItem};
            if(r.vault)(void)VaultStorage::Profile(db,row);if(r.action==Action::Return&&Encode(r.expected)!=Encode(row))throw Failure{Error::Conflict};
            auto m=Q(db,"SELECT itemclass,bagslots,loregroup,stackable,stacksize,UNK227,idfile FROM items WHERE id="+std::to_string(row.values.item_id)+" FOR UPDATE");if(m.RowCount()!=1)throw Failure{Error::UnsupportedItem};auto x=m.begin();for(unsigned i=0;i<7;++i)if(!x[i])throw Failure{Error::UnsupportedItem};
            const auto* model=StaticFurniture::Lookup(row.values.item_id,std::string(x[6]));if((r.vault?std::stoul(x[0])>2:std::stoul(x[0])!=0)||std::stoul(x[1])||std::stoll(x[2])||std::stoul(x[3])!=1||std::stoll(x[4])<=1||std::stoll(x[4])>=32767||std::stoll(x[5])||(!r.vault&&(!model||!StaticFurniture::Enabled(*model)))||r.partition_quantity>=StackCount(row)||StackCount(row)>std::stoul(x[4]))throw Failure{Error::UnsupportedItem};
            // Future reviewed NPC metadata cannot silently enter ordinary stacks.
            auto table=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_native_npc_item_sources'");if(table.RowCount()){if(table.RowCount()!=1||!table.begin()[0]||std::string(table.begin()[0])!="InnoDB"||Q(db,"SELECT item_id FROM housing_native_npc_item_sources WHERE item_id="+std::to_string(row.values.item_id)+" FOR UPDATE").RowCount())throw Failure{Error::UnsupportedItem};}
            if(Q(db,"SELECT id FROM items_evolving_details WHERE item_id="+std::to_string(row.values.item_id)+" FOR UPDATE").RowCount())throw Failure{Error::UnsupportedItem};return std::stoul(x[4]);
        }
        void PartitionShape(const Receipt& r){
            if(!r.partition)return;const auto& p=*r.partition;const auto count=StackCount(p.before);if(!p.quantity||p.quantity>=count||!p.serial||p.serial>INT32_MAX||p.serial==p.before.values.guid||p.maximum<count||p.maximum>=32767||r.house_effect_required||r.crate_allocation||r.crate_before)throw Failure{Error::DatabaseFailure};
            if(r.stored.placement&&(r.stored.placement->npc_frame||(r.stored.placement->reservation.content_binding&&(r.stored.placement->reservation.content_binding->version==ContentBinding::NpcClassVersion||r.stored.placement->reservation.content_binding->version==ContentBinding::GenericNpcClassVersion))))throw Failure{Error::DatabaseFailure};
            if(!CrateStackPartition::Coherent(r))throw Failure{Error::DatabaseFailure};
            if(!p.vault_source&&p.vault_scope_revision)throw Failure{Error::DatabaseFailure};if(p.room_source&&p.vault_source)throw Failure{Error::DatabaseFailure};
            if(r.room&&!p.room_source)throw Failure{Error::DatabaseFailure};
            if(p.room_source){(void)RoomMoveBytes(*p.room_source);if(!r.stored.placement)throw Failure{Error::DatabaseFailure};}
            if(p.vault_source){const auto& v=*p.vault_source;if(r.room||r.stored.placement||!v.estate_id||!v.native_estate_id||v.native_estate_id==UINT32_MAX||!v.catalog_revision||!v.source.zone_id||!v.source.instance_id||v.source.instance_id>32000||bool(v.house_id)!=bool(v.house_revision)||bool(v.house_id)!=bool(v.profile_revision)||bool(v.house_id)!=bool(p.vault_scope_revision)||p.stowed)throw Failure{Error::DatabaseFailure};if(r.action==Action::Store&&p.vault_scope_revision!=(v.house_id?1:0))throw Failure{Error::DatabaseFailure};if(r.action==Action::Return?(v.expected_native_item_id==0||v.expected_revision!=p.storage_before):(v.expected_native_item_id||v.expected_revision))throw Failure{Error::DatabaseFailure};}
            auto after=p.before;after.values.charges=count-p.quantity;after.null_fields&=~NullCharges;if(Encode(after)!=Encode(p.after))throw Failure{Error::DatabaseFailure};
            auto piece=p.before;piece.values.charges=p.quantity;piece.values.guid=p.serial;piece.null_fields&=~uint8_t(NullCharges|NullGuid);if(r.action==Action::Return){piece.values.character_id=r.inventory_after.values.character_id;piece.values.slot_id=r.inventory_after.values.slot_id;if(Encode(r.stored.row)!=Encode(p.before)||p.storage_before==UINT64_MAX||p.storage_after!=p.storage_before+1)throw Failure{Error::DatabaseFailure};}else if(p.storage_before||p.storage_after||p.stowed)throw Failure{Error::DatabaseFailure};
            if(Encode(piece)!=Encode(r.action==Action::Store?r.stored.row:r.inventory_after))throw Failure{Error::DatabaseFailure};
        }
        bool VaultPartitionPostimage(Database& db,const Receipt& r){
            if(!r.partition||!r.partition->vault_source||r.action!=Action::Return||r.stored.placement)return false;const auto& p=*r.partition;const auto& v=*p.vault_source;
            auto rows=Q(db,"SELECT v.native_item_id,v.revision,v.house_id,v.realm_estate_id,r.native_estate_id,r.state,r.custody_id,HEX(r.source_row),r.estate_id,r.content_binding_version,r.content_binding_digest FROM housing_live_vault_items v JOIN housing_live_placement_reservations r ON r.native_item_id=v.native_item_id WHERE v.custody_id="+std::to_string(r.stored.custody_id)+" FOR UPDATE");if(rows.RowCount()!=1)return false;auto x=rows.begin();for(unsigned i=0;i<9;++i)if(i!=2&&!x[i])return false;
            if(std::stoul(x[0])!=v.expected_native_item_id||std::stoull(x[1])!=p.storage_after||(x[2]?std::stoull(x[2]):0)!=v.house_id||std::stoul(x[3])!=v.estate_id||std::stoul(x[4])!=v.native_estate_id||std::stoul(x[5])!=1||std::stoull(x[6])!=r.stored.custody_id||Unhex(x[7])!=Encode(p.after)||std::stoul(x[8])!=r.stored.estate_id||x[9]||x[10])return false;
            auto remaining=r.stored;remaining.row=p.after;if(!VaultStorage::CratePostimage(db,v,remaining,p.storage_after,2))return false;auto scope=HouseItemScope::Locked(db,r.stored.custody_id);return v.house_id?(scope&&scope->house_id==v.house_id&&scope->revision==p.vault_scope_revision):!scope;
        }
        void PartitionPostimage(Database& db,const Receipt& r){
            if(!r.partition)return;PartitionShape(r);const auto& p=*r.partition;
            if(r.action==Action::Store){auto row=Q(db,"SELECT "+select_inventory+" FROM inventory WHERE "+Slot(p.after.values.character_id,p.after.values.slot_id)+" FOR UPDATE");if(row.RowCount()!=1||Encode(ItemPayload::Complete(db,InvRow(row.begin()),&p.after))!=Encode(p.after))throw Failure{Error::Conflict};auto source=Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id="+std::to_string(r.stored.custody_id)+" FOR UPDATE");if(source.RowCount()!=1)throw Failure{Error::Conflict};auto current=StoredRow(db,source.begin(),r.stored.custody_id);if(current.estate_id!=r.stored.estate_id||Encode(current.row)!=Encode(r.stored.row)||Cat(current.catalog)!=Cat(r.stored.catalog))throw Failure{Error::Conflict};if(p.vault_source){if(!VaultStorage::Outcome(db,*p.vault_source,r.stored,true)||CurrentPose(db,r.stored.custody_id)||ItemStorage::Locked(db,r.stored.custody_id).revision)throw Failure{Error::Conflict};auto scope=HouseItemScope::Locked(db,r.stored.custody_id);if(p.vault_source->house_id?(!scope||scope->revision!=p.vault_scope_revision):bool(scope))throw Failure{Error::Conflict};}else{auto pose=CurrentPose(db,r.stored.custody_id,bool(r.crate_member));auto marker=ItemStorage::Locked(db,r.stored.custody_id);if(bool(pose)!=bool(r.stored.placement)||(pose&&PlacementBytes(*pose)!=PlacementBytes(*r.stored.placement))||marker.revision!=r.stored.storage_revision||marker.stowed!=r.stored.stowed||(marker.revision&&(!pose||marker.native_estate_id!=pose->reservation.native_estate_id)))throw Failure{Error::Conflict};if(pose){auto reserved=Q(db,"SELECT state,custody_id,HEX(source_row) FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(pose->reservation.native_item_id)+" FOR UPDATE");if(reserved.RowCount()!=1||!reserved.begin()[0]||std::stoul(reserved.begin()[0])!=1||!reserved.begin()[1]||std::stoull(reserved.begin()[1])!=r.stored.custody_id||Unhex(reserved.begin()[2])!=Encode(r.stored.row))throw Failure{Error::Conflict};}}return;}
            auto source=Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id="+std::to_string(r.stored.custody_id)+" FOR UPDATE");if(source.RowCount()!=1)throw Failure{Error::Conflict};auto current=StoredRow(db,source.begin(),r.stored.custody_id);if(current.estate_id!=r.stored.estate_id||Encode(current.row)!=Encode(p.after)||Cat(current.catalog)!=Cat(r.stored.catalog))throw Failure{Error::Conflict};if(p.vault_source){if(CurrentPose(db,r.stored.custody_id)||ItemStorage::Locked(db,r.stored.custody_id).revision||!VaultPartitionPostimage(db,r))throw Failure{Error::Conflict};return;}if(p.room_source){auto scope=HouseItemScope::Locked(db,r.stored.custody_id);if(!scope||scope->house_id!=p.room_source->house_id||scope->revision!=p.room_source->scope_revision)throw Failure{Error::Conflict};}auto pose=CurrentPose(db,r.stored.custody_id,bool(r.crate_member));if(!pose||!r.stored.placement||PlacementBytes(*pose)!=PlacementBytes(*r.stored.placement))throw Failure{Error::Conflict};auto marker=ItemStorage::Locked(db,r.stored.custody_id);if(marker.revision!=p.storage_after||marker.stowed!=p.stowed||marker.native_estate_id!=pose->reservation.native_estate_id)throw Failure{Error::Conflict};auto reserved=Q(db,"SELECT state,custody_id,HEX(source_row) FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(pose->reservation.native_item_id)+" FOR UPDATE");if(reserved.RowCount()!=1||!reserved.begin()[0]||std::stoul(reserved.begin()[0])!=1||!reserved.begin()[1]||std::stoull(reserved.begin()[1])!=r.stored.custody_id||Unhex(reserved.begin()[2])!=Encode(p.after))throw Failure{Error::Conflict};
        }
        Bytes Binding(const Request& r){
            if(r.crate_member&&(r.crate||r.crate_member->revision>=UINT64_MAX-1||(r.action==Action::Store?(!r.initial_stowed||!r.placement):(r.action!=Action::Return||r.initial_stowed||r.placement))))throw Failure{Error::InvalidArgument};
            if(r.crate&&(r.house||r.initial_stowed||(r.action!=Action::Return&&r.action!=Action::Store)||!r.crate->crate_id||!r.crate->expected_revision||r.crate->expected_revision==UINT64_MAX||!CrateGeometry::TemplateDefinition(r.expected.values.item_id)))throw Failure{Error::InvalidArgument};
            if(r.initial_stowed&&(r.action!=Action::Store||!r.placement))throw Failure{Error::InvalidArgument};
            if(r.room_exterior_source&&(!r.room||(r.action==Action::Store&&!r.initial_stowed)||!r.room_exterior_source->zone_id||!r.room_exterior_source->instance_id||r.room_exterior_source->instance_id>32000))throw Failure{Error::InvalidArgument};
            if(r.room){if(r.house||r.room->placement.content_binding)throw Failure{Error::InvalidArgument};
                if(r.action==Action::Store){if(!r.placement||r.room_return_context||r.expected_room_pose_revision||PlacementBytes({r.room->placement,r.placement->transform,r.placement->pose_revision,r.placement->npc_frame})!=PlacementBytes(*r.placement))throw Failure{Error::InvalidArgument};}
                else if(r.action==Action::Return){if(r.placement||!r.room_return_context||!r.expected_room_pose_revision)throw Failure{Error::InvalidArgument};}
                else throw Failure{Error::InvalidArgument};
            }else if(r.room_return_context||r.expected_room_pose_revision)throw Failure{Error::InvalidArgument};
            if(r.placement&&r.placement->reservation.content_binding&&r.placement->reservation.content_binding->version==ContentBinding::HouseVersion&&!r.house)throw Failure{Error::InvalidArgument};
            if(r.vault&&(r.placement||r.room||r.house||r.crate||r.crate_member||r.initial_stowed||r.room_exterior_source||!r.vault->estate_id||!r.vault->native_estate_id||!r.vault->catalog_revision||!r.vault->source.zone_id||!r.vault->source.instance_id||(r.action==Action::Store?(r.estate_id!=r.vault->estate_id||r.vault->expected_native_item_id||r.vault->expected_revision):(!r.vault->expected_native_item_id||!r.vault->expected_revision))))throw Failure{Error::InvalidArgument};
            if(bool(r.partition_quantity)!=bool(r.partition_serial)||(r.partition_quantity&&(r.house||r.crate)))throw Failure{Error::InvalidArgument};
            Bytes b;
            W(b,1,4);
            W(b,r.lease.actor.character_id,4);
            W(b,r.lease.actor.account_id,4);
            W(b,r.lease.epoch,8);
            S(b,r.lease.session_key);
            b.insert(b.end(),r.lease.token.begin(),r.lease.token.end());
            W(b,r.estate_id,4);
            W(b,uint8_t(r.action),1);
            B(b,Encode(r.expected));
            W(b,uint32_t(r.verified_runtime_serial),4);
            W(b,r.custody_id,8);
            W(b,r.destination_slot,4);
            if(r.placement){W(b,r.placement->reservation.content_binding?3:2,4);B(b,PlacementBytes(*r.placement));}
            if(r.house){W(b,4,4);W(b,r.house->house_id,8);W(b,r.house->expected_revision,8);}
            if(r.room){if(r.action==Action::Store){W(b,5,4);B(b,RoomBytes(*r.room));}else{W(b,6,4);B(b,RoomBytes(*r.room));B(b,PlacementBytes({r.room->placement,{0,0,0,1,{0,0,0}},1}));B(b,RoomMoveBytes(*r.room_return_context));W(b,r.expected_room_pose_revision,8);}}
            if(r.initial_stowed)W(b,7,4); // Preserve every legacy binding byte when false.
            if(r.room_exterior_source){W(b,8,4);W(b,r.room_exterior_source->zone_id,4);W(b,r.room_exterior_source->zone_version,1);W(b,r.room_exterior_source->instance_id,2);}
            if(r.crate){W(b,9,4);W(b,r.crate->crate_id,8);W(b,r.crate->expected_revision,8);}
            if(r.crate_member){W(b,10,4);B(b,MemberContextBytes(*r.crate_member));}
            if(r.vault){VaultStorage::ContextShape(*r.vault);if(r.vault->crate_id&&(r.action==Action::Store?bool(r.vault->member_revision):!r.vault->member_revision))throw Failure{Error::InvalidArgument};W(b,11,4);const auto&v=*r.vault;W(b,v.estate_id,4);W(b,v.native_estate_id,4);for(auto x:{v.catalog_revision,v.house_id,v.house_revision,v.profile_revision})W(b,x,8);W(b,v.source.zone_id,4);W(b,v.source.zone_version,1);W(b,v.source.instance_id,2);W(b,v.expected_native_item_id,4);W(b,v.expected_revision,8);}
            if(r.partition_quantity){W(b,12,4);W(b,r.partition_quantity,4);W(b,r.partition_serial,4);}
            if(r.vault&&r.vault->crate_id){VaultStorage::ContextShape(*r.vault);W(b,13,4);W(b,r.vault->crate_id,8);W(b,r.vault->crate_revision,8);W(b,r.vault->member_revision,8);}
            return b;
        }
        Request Unbinding(const Bytes& bytes){
            Reader b{bytes};if(b.N(4)!=1)throw Failure{Error::DatabaseFailure};
            Request r;
            r.lease.actor.character_id=b.N(4);r.lease.actor.account_id=b.N(4);
            r.lease.epoch=b.N(8);r.lease.session_key=b.Text(96);
            for(auto& byte:r.lease.token)byte=b.N(1);
            r.estate_id=b.N(4);r.action=Action(b.N(1));r.expected=Decode(b.Blob(CustodyCodec::MaximumPayload));
            uint32_t serial=b.N(4);std::memcpy(&r.verified_runtime_serial,&serial,4);
            r.custody_id=b.N(8);r.destination_slot=b.N(4);
            while(b.at!=bytes.size()){
                const auto extension=b.N(4);
                if((extension==2||extension==3)&&!r.placement&&!r.house){
                    r.placement=Unplacement(b.Blob(512));
                    if(bool(r.placement->reservation.content_binding)!=(extension==3))throw Failure{Error::DatabaseFailure};
                }else if(extension==4&&!r.house){
                    r.house=HouseIdentity{b.N(8),b.N(8)};
                    if((!r.house->house_id!=!r.house->expected_revision)||(r.action==Action::Return&&!r.house->house_id))throw Failure{Error::DatabaseFailure};
                }else if(extension==5&&!r.room&&!r.house&&r.placement&&r.action==Action::Store){r.room=Unroom(b.Blob(128),r.placement->reservation);
                }else if(extension==6&&!r.room&&!r.house&&!r.placement&&r.action==Action::Return){auto tag=b.Blob(128);auto base=Unplacement(b.Blob(512));r.room=Unroom(tag,base.reservation);r.room_return_context=UnroomMove(b.Blob(128));r.expected_room_pose_revision=b.N(8);
                }else if(extension==7&&!r.initial_stowed&&r.placement&&r.action==Action::Store){r.initial_stowed=true;
                }else if(extension==8&&!r.room_exterior_source&&r.room){r.room_exterior_source=Route{uint32_t(b.N(4)),uint8_t(b.N(1)),uint16_t(b.N(2))};
                }else if(extension==9&&!r.crate){r.crate=CrateIdentity{b.N(8),b.N(8)};
                }else if(extension==10&&!r.crate_member){r.crate_member=UnmemberContext(b.Blob(64));
                }else if(extension==11&&!r.vault){VaultContext v;v.estate_id=b.N(4);v.native_estate_id=b.N(4);v.catalog_revision=b.N(8);v.house_id=b.N(8);v.house_revision=b.N(8);v.profile_revision=b.N(8);v.source={uint32_t(b.N(4)),uint8_t(b.N(1)),uint16_t(b.N(2))};v.expected_native_item_id=b.N(4);v.expected_revision=b.N(8);r.vault=v;
                }else if(extension==12&&!r.partition_quantity&&!r.partition_serial){r.partition_quantity=b.N(4);r.partition_serial=b.N(4);if(!r.partition_quantity||!r.partition_serial)throw Failure{Error::DatabaseFailure};}
                else if(extension==13&&r.vault&&!r.vault->crate_id){r.vault->crate_id=b.N(8);r.vault->crate_revision=b.N(8);r.vault->member_revision=b.N(8);VaultStorage::ContextShape(*r.vault);}else throw Failure{Error::DatabaseFailure};
            }
            if(Binding(r)!=bytes)throw Failure{Error::DatabaseFailure};
            return r;
        }
        std::shared_ptr<const CrateBinding::Allocation> CurrentCrate(Database& db,const Stored& stored){
            const bool known=CrateGeometry::TemplateDefinition(stored.row.values.item_id)!=0;
            const bool tagged=stored.placement&&stored.placement->reservation.content_binding&&stored.placement->reservation.content_binding->version==ContentBinding::CrateVersion;
            if(!known&&!tagged)return {};
            if(!known||(!tagged&&(!stored.placement||stored.placement->reservation.content_binding)))throw Failure{Error::Conflict};
            auto current=CrateBinding::ReadPlacedUnderTransaction(db,stored);if(!current)throw Failure{current.error};
            return std::make_shared<CrateBinding::Allocation>(std::move(current.value));
        }
        void CrateOutcomeShape(const Receipt& r){
            if(!CrateBinding::CoherentOutcome(r))throw Failure{Error::DatabaseFailure};
            if(r.crate_member){MemberContextBytes(*r.crate_member);if(!r.stored.placement||(r.action==Action::Store?(!r.stored.stowed||r.stored.storage_revision!=1):(r.action!=Action::Return||r.stored.stowed||r.stored.storage_revision))||r.crate_allocation||r.crate_before||r.crate_member->native_plot_id!=r.stored.placement->reservation.native_estate_id||(r.action==Action::Store&&r.crate_member->catalog_revision!=r.stored.placement->reservation.catalog_revision))throw Failure{Error::DatabaseFailure};}
        }
        LiveItems::Stored CountContainer(Database& db,uint64_t crate){
            auto rows=Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id=(SELECT custody_id FROM housing_crate_locations WHERE crate_id="+std::to_string(crate)+" AND location_kind=2) FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::Conflict};
            auto stored=StoredRow(db,rows.begin(),0);auto id=Q(db,"SELECT custody_id FROM housing_crate_locations WHERE crate_id="+std::to_string(crate)+" AND location_kind=2 FOR UPDATE");if(id.RowCount()!=1||!id.begin()[0])throw Failure{Error::Conflict};stored.custody_id=std::stoull(id.begin()[0]);ItemPayload::ValidateHeld(db,stored.row,stored.custody_id);stored.placement=CurrentPose(db,stored.custody_id);auto marker=ItemStorage::Locked(db,stored.custody_id);stored.stowed=marker.stowed;stored.storage_revision=marker.revision;return stored;
        }
        LayoutCrateMember::Proof CountMember(Database& db,const Stored& source,uint64_t crate){
            auto container=CountContainer(db,crate);auto proof=LayoutCrateMember::CaptureUnderTransaction(db,source,container);if(!proof)throw Failure{proof.error};return proof.value;
        }
        void MemberCountPostimage(Database& db,const Receipt& r){
            if(!r.partition||!r.crate_member||!CrateStackPartition::Coherent(r))throw Failure{Error::Conflict};const auto& p=*r.partition;
            auto stored=r.stored;stored.row=r.action==Action::Store?r.stored.row:p.after;stored.stowed=true;stored.storage_revision=r.action==Action::Store?1:p.storage_after;
            if(!LayoutCrateMember::Same(CountMember(db,stored,r.crate_member->crate_id),p.member_after))throw Failure{Error::Conflict};
        }
        void ValidatePendingCrate(Database& db,const Receipt& r){
            CrateOutcomeShape(r);
            if(r.crate_member&&r.requires_application){
                if(r.partition)MemberCountPostimage(db,r);
                const auto& m=*r.crate_member;auto scope=CrateMemberScope::Locked(db,r.stored.custody_id,bool(r.room));auto storage=ItemStorage::Locked(db,r.stored.custody_id);
                if(r.action==Action::Store){if(!scope||scope->crate_id!=m.crate_id||scope->revision!=1||scope->origin_native_estate_id!=m.native_plot_id||scope->origin_house_id!=(r.room?r.room->house_id:0)||!scope->origin_stowed||scope->origin_pose!=Pose(r.stored.placement->transform)||!storage.stowed||storage.revision!=1||storage.native_estate_id!=m.native_plot_id)throw Failure{Error::Conflict};}
                else if(r.partition){MemberCountPostimage(db,r);}
                else if(scope||storage.revision||Q(db,"SELECT custody_id FROM housing_live_item_custody WHERE custody_id="+std::to_string(r.stored.custody_id)+" FOR UPDATE").RowCount())throw Failure{Error::Conflict};
                auto container=Q(db,"SELECT h.revision,e.parent_native_estate_id,e.ownership_estate_id,e.catalog_revision,n.active,COALESCE(s.stowed,0),h.created_by_character_id FROM housing_crate_instances h JOIN housing_crate_locations l ON l.crate_id=h.crate_id JOIN housing_crate_estates e ON e.crate_id=h.crate_id JOIN housing_native_estates n ON n.native_id=e.native_estate_id LEFT JOIN housing_live_item_storage s ON s.custody_id=l.custody_id WHERE h.crate_id="+std::to_string(m.crate_id)+" AND h.packed=0 AND l.location_kind=2 AND l.custody_id IS NOT NULL FOR UPDATE");
                if(container.RowCount()!=1)throw Failure{Error::Conflict};auto c=container.begin();for(size_t i=0;i<7;++i)if(!c[i])throw Failure{Error::Conflict};
                if(std::stoull(c[0])!=m.revision||std::stoul(c[1])!=m.native_plot_id||(!r.room&&std::stoul(c[2])!=r.stored.estate_id)||(!r.room&&std::stoull(c[3])!=m.catalog_revision)||std::stoul(c[4])!=1||std::stoul(c[5])||std::stoul(c[6])!=(r.action==Action::Return?r.inventory_after.values.character_id:r.stored.row.values.character_id))throw Failure{Error::Conflict};
                if(r.room){auto house=Q(db,"SELECT house_id FROM housing_house_interiors WHERE native_estate_id="+std::to_string(m.native_plot_id)+" FOR UPDATE");if(house.RowCount()!=1||std::stoull(house.begin()[0])!=r.room->house_id)throw Failure{Error::Conflict};}
            }
            if(!r.crate_allocation||!r.requires_application)return;
            std::shared_ptr<const CrateBinding::Allocation> current;
            if(r.action==Action::Store)current=CurrentCrate(db,r.stored);
            else{auto carried=CrateBinding::ReadCarriedUnderTransaction(db,r.inventory_after);if(!carried)throw Failure{carried.error};current=std::make_shared<CrateBinding::Allocation>(std::move(carried.value));}
            if(!current||CrateBinding::EncodeAllocation(*current)!=CrateBinding::EncodeAllocation(*r.crate_allocation))throw Failure{Error::Conflict};
            if(!CrateBinding::RelocationEffectsMatch(db,r))throw Failure{Error::Conflict};
        }
        Bytes Outcome(const Receipt& r){
            PartitionShape(r);CrateOutcomeShape(r);
            if(r.stored.placement&&r.stored.placement->reservation.content_binding&&r.stored.placement->reservation.content_binding->version==ContentBinding::HouseVersion&&!r.house_effect_required)throw Failure{Error::DatabaseFailure};
            Bytes b;
            W(b,r.stored.placement?(r.stored.placement->reservation.content_binding?3:2):1,4);
            S(b,r.request_key);
            W(b,uint8_t(r.action),1);
            W(b,r.stored.custody_id,8);
            W(b,r.stored.estate_id,4);
            B(b,Encode(r.stored.row));
            B(b,Cat(r.stored.catalog));
            if(r.action==Action::Return)B(b,Encode(r.inventory_after));
            if(r.stored.placement)B(b,PlacementBytes(*r.stored.placement));
            if(r.stored.stowed){if(r.action!=Action::Store||!r.stored.placement||r.stored.storage_revision!=1)throw Failure{Error::DatabaseFailure};W(b,4,4);W(b,1,8);}
            if(r.crate_allocation){W(b,5,4);B(b,CrateBinding::EncodeAllocation(*r.crate_allocation));}
            if(r.crate_before){W(b,6,4);B(b,CrateBinding::EncodeAllocation(*r.crate_before));}
            if(r.crate_member){W(b,7,4);B(b,MemberContextBytes(*r.crate_member));}
            if(r.partition){const auto& p=*r.partition;W(b,p.member_after?10:(p.room_source||p.vault_source)?9:8,4);B(b,Encode(p.before));B(b,Encode(p.after));for(auto n:{p.quantity,p.serial,p.maximum})W(b,n,4);W(b,p.storage_before,8);W(b,p.storage_after,8);W(b,p.stowed,1);if(p.member_after){W(b,bool(p.room_source),1);if(p.room_source)B(b,RoomMoveBytes(*p.room_source));B(b,CrateBinding::EncodeAllocation(*p.member_container_before));W(b,bool(p.member_before),1);if(p.member_before)B(b,LayoutCrateMember::Encode(*p.member_before));B(b,LayoutCrateMember::Encode(*p.member_after));}else if(p.room_source){W(b,1,1);B(b,RoomMoveBytes(*p.room_source));}else if(p.vault_source){W(b,p.vault_source->crate_id?3:2,1);const auto& v=*p.vault_source;W(b,v.estate_id,4);W(b,v.native_estate_id,4);for(auto n:{v.catalog_revision,v.house_id,v.house_revision,v.profile_revision})W(b,n,8);W(b,v.source.zone_id,4);W(b,v.source.zone_version,1);W(b,v.source.instance_id,2);W(b,v.expected_native_item_id,4);W(b,v.expected_revision,8);W(b,p.vault_scope_revision,8);if(v.crate_id){W(b,v.crate_id,8);W(b,v.crate_revision,8);W(b,v.member_revision,8);}}}
            return b;
        }
        Receipt Unout(const Bytes& b){
            Reader p{
                b
            };
            const auto version=p.N(4);if(version!=1&&version!=2&&version!=3)throw Failure{Error::DatabaseFailure};
            Receipt r;
            r.request_key=p.Text(96);
            r.action=Action(p.N(1));
            if(r.action!=Action::Store&&r.action!=Action::Return)throw Failure{
                Error::DatabaseFailure
            };
            r.stored.custody_id=p.N(8);
            r.stored.estate_id=uint32_t(p.N(4));
            r.stored.row=Decode(p.Blob(CustodyCodec::MaximumPayload));
            r.stored.catalog=Uncat(p.Blob(16));
            if(r.action==Action::Return)r.inventory_after=Decode(p.Blob(CustodyCodec::MaximumPayload));
            if(version>=2){r.stored.placement=Unplacement(p.Blob(512));if(bool(r.stored.placement->reservation.content_binding)!=(version==3))throw Failure{Error::DatabaseFailure};}
            while(p.at!=b.size()){
                const auto extension=p.N(4);
                if(extension==4&&!r.stored.stowed&&r.action==Action::Store&&r.stored.placement){if(p.N(8)!=1)throw Failure{Error::DatabaseFailure};r.stored.stowed=true;r.stored.storage_revision=1;}
                else if(extension==5&&!r.crate_allocation){auto decoded=CrateBinding::DecodeAllocation(p.Blob(512));if(!decoded)throw Failure{decoded.error};r.crate_allocation=std::make_shared<CrateBinding::Allocation>(std::move(decoded.value));}
                else if(extension==6&&!r.crate_before){auto decoded=CrateBinding::DecodeAllocation(p.Blob(512));if(!decoded)throw Failure{decoded.error};r.crate_before=std::make_shared<CrateBinding::Allocation>(std::move(decoded.value));}
                else if(extension==7&&!r.crate_member){r.crate_member=UnmemberContext(p.Blob(64));}
                else if((extension==8||extension==9||extension==10)&&!r.partition){auto part=std::make_shared<StackPartition>();part->before=Decode(p.Blob(CustodyCodec::MaximumPayload));part->after=Decode(p.Blob(CustodyCodec::MaximumPayload));part->quantity=p.N(4);part->serial=p.N(4);part->maximum=p.N(4);part->storage_before=p.N(8);part->storage_after=p.N(8);auto flag=p.N(1);if(flag>1)throw Failure{Error::DatabaseFailure};part->stowed=flag;if(extension==10){auto room=p.N(1);if(room>1)throw Failure{Error::DatabaseFailure};if(room)part->room_source=UnroomMove(p.Blob(128));auto container=CrateBinding::DecodeAllocation(p.Blob(512));if(!container)throw Failure{Error::DatabaseFailure};part->member_container_before=std::make_shared<const CrateBinding::Allocation>(std::move(container.value));auto before=p.N(1);if(before>1)throw Failure{Error::DatabaseFailure};if(before)part->member_before=LayoutCrateMember::Decode(p.Blob(1024));part->member_after=LayoutCrateMember::Decode(p.Blob(1024));}else if(extension==9){auto kind=p.N(1);if(kind==1)part->room_source=UnroomMove(p.Blob(128));else if(kind==2||kind==3){VaultContext v;v.estate_id=p.N(4);v.native_estate_id=p.N(4);v.catalog_revision=p.N(8);v.house_id=p.N(8);v.house_revision=p.N(8);v.profile_revision=p.N(8);v.source={uint32_t(p.N(4)),uint8_t(p.N(1)),uint16_t(p.N(2))};v.expected_native_item_id=p.N(4);v.expected_revision=p.N(8);part->vault_scope_revision=p.N(8);if(kind==3){v.crate_id=p.N(8);v.crate_revision=p.N(8);v.member_revision=p.N(8);VaultStorage::ContextShape(v);}part->vault_source=v;}else throw Failure{Error::DatabaseFailure};}r.partition=part;}
                else throw Failure{Error::DatabaseFailure};
            }
            PartitionShape(r);
            if(p.at!=b.size()||!r.stored.custody_id||!r.stored.estate_id)throw Failure{
                Error::DatabaseFailure
            };
            return r;
        }
        const std::string receipt_columns="character_id,account_id,lease_epoch,session_key,HEX(binding_blob),completed,applied,HEX(after_blob)";
        Receipt ReceiptRow(MySQLRequestRow row,const Lease& l,const std::string& key,bool allow_acknowledged_pending=false){
            if(std::stoul(row[0])!=l.actor.character_id||std::stoul(row[1])!=l.actor.account_id)throw Failure{
                Error::Unauthorized
            };
            if(std::stoull(row[2])!=l.epoch||std::string(row[3])!=l.session_key)throw Failure{
                Error::StaleLease
            };
            if(std::stoul(row[5])!=1)throw Failure{
                Error::NotFound
            };
            auto r=Unout(Unhex(row[7]));
            if(r.request_key!=key)throw Failure{
                Error::DatabaseFailure
            };
            const auto binding=Unbinding(Unhex(row[4]));
            if(binding.lease.actor.character_id!=l.actor.character_id||binding.lease.actor.account_id!=l.actor.account_id||binding.lease.epoch!=l.epoch||binding.lease.session_key!=l.session_key||binding.lease.token!=l.token||binding.estate_id!=r.stored.estate_id||binding.action!=r.action||Encode(binding.expected)!=Encode(r.partition?r.partition->before:r.stored.row))throw Failure{Error::DatabaseFailure};
            if(bool(binding.partition_quantity)!=bool(r.partition)||(r.partition&&(binding.partition_quantity!=r.partition->quantity||binding.partition_serial!=r.partition->serial)))throw Failure{Error::DatabaseFailure};
            r.house_effect_required=bool(binding.house);r.room=binding.room;
            if(r.partition){if(r.stored.row.values.character_id!=l.actor.character_id)throw Failure{Error::DatabaseFailure};const auto& part=*r.partition;if(bool(part.room_source)!=bool(binding.room)||bool(part.vault_source)!=bool(binding.vault))throw Failure{Error::DatabaseFailure};if(part.room_source){InteriorMoveContext expected;if(binding.action==Action::Return)expected=*binding.room_return_context;else expected={binding.room->house_id,binding.room->house_revision,binding.room->profile_revision,1,binding.room->source};if(RoomMoveBytes(expected)!=RoomMoveBytes(*part.room_source))throw Failure{Error::DatabaseFailure};}if(part.vault_source){auto expected=binding;expected.vault=part.vault_source;if(Binding(expected)!=Unhex(row[4]))throw Failure{Error::DatabaseFailure};}PartitionShape(r);}

            if(r.stored.row.values.character_id!=l.actor.character_id&&(binding.house||binding.crate||binding.vault||!ForeignFurnitureReturn::Outcome(r,l.actor.character_id)))throw Failure{Error::DatabaseFailure};
            if(bool(binding.crate_member)!=bool(r.crate_member))throw Failure{Error::DatabaseFailure};
            if(binding.crate_member){auto expected=*binding.crate_member;++expected.revision;if(MemberContextBytes(expected)!=MemberContextBytes(*r.crate_member))throw Failure{Error::DatabaseFailure};}
            if(r.room){if(!r.stored.placement)throw Failure{Error::DatabaseFailure};if(r.action==Action::Store){if(!binding.placement||PlacementBytes(*binding.placement)!=PlacementBytes(*r.stored.placement))throw Failure{Error::DatabaseFailure};}else{if(!binding.room_return_context||binding.expected_room_pose_revision!=r.stored.placement->pose_revision||PlacementBytes({binding.room->placement,{0,0,0,1,{0,0,0}},1})!=PlacementBytes({r.stored.placement->reservation,{0,0,0,1,{0,0,0}},1}))throw Failure{Error::DatabaseFailure};}}
            if(r.stored.placement&&r.stored.placement->reservation.content_binding&&r.stored.placement->reservation.content_binding->version==ContentBinding::HouseVersion&&!r.house_effect_required)throw Failure{Error::DatabaseFailure};
            CrateOutcomeShape(r);
            if(r.crate_before){if(!binding.crate||binding.crate->crate_id!=r.crate_before->binding.crate_id||binding.crate->expected_revision!=r.crate_before->binding.revision)throw Failure{Error::DatabaseFailure};}
            else if(binding.crate)throw Failure{Error::DatabaseFailure};
            r.replayed=true;
            r.requires_application=std::stoul(row[6])!=1;
            if(r.requires_application&&l.pending_key!=key)throw Failure{
                Error::DatabaseFailure
            };
            if(!r.requires_application&&l.pending_key==key&&!allow_acknowledged_pending)throw Failure{
                Error::DatabaseFailure
            };
            return r;
        }
        std::string Values(const InventoryRow& r,uint32_t slot){return InventoryRowDB::Values(r,slot);}
        template<class T,class F>Result<T> Run(Database& db,F fn){
            try{
                Tx t(db);
                Verify(db);
                auto v=fn();
                static_assert(std::is_nothrow_move_constructible_v<Result<T>>);
                Result<T> prepared{std::move(v),Error::None};
                t.Commit();
                return prepared;
            }
            catch(const Failure& e){
                return {
                    {
                    },e.error
                };
            }
            catch(const HouseBinding::Failure& e){
                Error error=Error::DatabaseFailure;
                switch(e.error){
                case HouseBinding::Error::InvalidArgument:error=Error::InvalidArgument;break;
                case HouseBinding::Error::Unauthorized:error=Error::Unauthorized;break;
                case HouseBinding::Error::UnheldFence:error=Error::UnheldFence;break;
                case HouseBinding::Error::Conflict:error=Error::Conflict;break;
                case HouseBinding::Error::PendingApplication:error=Error::PendingApplication;break;
                case HouseBinding::Error::NotFound:error=Error::NotFound;break;
                case HouseBinding::Error::UnsupportedItem:error=Error::UnsupportedItem;break;
                default:break;
                }
                return {{},error};
            }
            catch(const std::exception& error){
                LogError("Housing item transaction rejected [{}]",error.what());
                return {
                    {
                    },Error::DatabaseFailure
                };
            }
        }
    }
    std::string Service::SchemaDescriptor(){
        return "EQEmu.inventory.v1:character_id:u32,slot_id:u32,item_id:?u32,charges:?u16,color:u32,augment_one:u32,augment_two:u32,augment_three:u32,augment_four:u32,augment_five:u32,augment_six:u32,instnodrop:u8,custom_data:?bytes,ornament_icon:u32,ornament_idfile:u32,ornament_hero_model:i32,guid:?u64";
    }
    Service::Service(std::unique_ptr<Database> d,bool enabled,HouseBinding::CustodyContext* houses,CrateBinding::CustodyContext* crates):database_(std::move(d)),enabled_(enabled),houses_(houses),crates_(crates){
    }
    Service::~Service()=default;
    Result<Lease> Service::OpenLease(AuthenticatedPurchaser actor,const std::string& key,InventorySessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!actor.character_id||!actor.account_id||!Key(key))return {
            {
            },Error::InvalidArgument
        };
        if(!fence.HeldFor(actor,0,key))return {
            {
            },Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Lease>(*database_,[&]{
            auto& db=*database_;Actor(db,actor);if(!InventoryRecovery::LockCustodyReservation(db,actor))throw Failure{Error::UnheldFence};auto rows=Q(db,"SELECT "+lease_columns+" FROM housing_live_item_leases WHERE character_id="+std::to_string(actor.character_id)+" FOR UPDATE");
            const auto coin_pending=CoinPending::Inspect(db,actor.character_id,true);
            if(coin_pending!=CoinPending::State::Clear)throw Failure{
                coin_pending==CoinPending::State::Pending?Error::PendingApplication:Error::DatabaseFailure
            };
            uint64_t epoch=1;if(rows.RowCount()){
                auto row=rows.begin();auto old=LeaseRow(row,actor.character_id);if(std::stoul(row[4])==1){
                    if(old.actor.account_id!=actor.account_id||old.session_key!=key)throw Failure{
                        Error::Conflict
                    };if(!InventoryRecovery::LinkCustodyToReservation(db,old))throw Failure{Error::UnheldFence};return old;
                }
                if(!old.pending_key.empty()||old.epoch==UINT64_MAX)throw Failure{
                    Error::Conflict
                };epoch=old.epoch+1;
            }
            Lease l{
                actor,epoch,key,{
                },{
                }
            };if(RAND_bytes(l.token.data(),int(l.token.size()))!=1)throw Failure{
                Error::DatabaseFailure
            };auto token=Hex(Bytes(l.token.begin(),l.token.end()));Q(db,"INSERT INTO housing_live_item_leases(character_id,account_id,epoch,session_key,token,active,pending_key) VALUES ("+std::to_string(actor.character_id)+","+std::to_string(actor.account_id)+","+std::to_string(epoch)+","+Quote(db,key)+",x'"+token+"',1,NULL) ON DUPLICATE KEY UPDATE account_id=VALUES(account_id),epoch=VALUES(epoch),session_key=VALUES(session_key),token=VALUES(token),active=1,pending_key=NULL");if(!InventoryRecovery::LinkCustodyToReservation(db,l))throw Failure{Error::UnheldFence};return l;
        });
    }
    Result<bool> Service::ReleaseLease(const Lease& l,InventorySessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!LeaseShape(l))return {
            {
            },Error::InvalidArgument
        };
        if(!fence.HeldFor(l.actor,l.epoch,l.session_key))return {
            {
            },Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<bool>(*database_,[&]{
            auto current=Locked(*database_,l);
            const auto coin_pending=CoinPending::Inspect(*database_,l.actor.character_id,true);
            if(coin_pending!=CoinPending::State::Clear)throw Failure{
                coin_pending==CoinPending::State::Pending?Error::PendingApplication:Error::DatabaseFailure
            };
            if(!current.pending_key.empty())throw Failure{
                Error::PendingApplication
            };auto previews=Q(*database_,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_placement_reservations'");if(std::stoul(previews.begin()[0])){PlacementTables(*database_);Q(*database_,"UPDATE housing_live_placement_reservations SET state=2 WHERE character_id="+std::to_string(l.actor.character_id)+" AND lease_epoch="+std::to_string(l.epoch)+" AND session_key="+Quote(*database_,l.session_key)+" AND state=0");}auto rows=Q(*database_,"UPDATE housing_live_item_leases SET active=0 WHERE character_id="+std::to_string(l.actor.character_id));if(rows.RowsAffected()!=1)throw Failure{
                Error::DatabaseFailure
            };return true;
        });
    }
    Result<Receipt> Service::OperateVault(const Request& r,InventorySessionFence& fence,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* profile){if(!r.vault)return {{},Error::InvalidArgument};return OperateCurrent(r,fence,house,profile);}
    Result<Receipt> Service::Operate(const Request& r,InventorySessionFence& fence){if(r.room||r.crate_member||r.vault)return {{},Error::InvalidArgument};return OperateCurrent(r,fence,nullptr,nullptr);}
    Result<Receipt> Service::OperateCrate(const Request& r,InventorySessionFence& fence,const Scene::HouseScenePolicy* scene){if(!r.crate_member||r.room)return {{},Error::InvalidArgument};return OperateCurrent(r,fence,scene,nullptr);}
    Result<Receipt> Service::OperateInterior(const Request& r,InventorySessionFence& fence,const Scene::HouseScenePolicy& scene,const InteriorContent::ProfilePolicy& profile){if(!r.room||r.house||(r.action==Action::Store?!r.placement:(r.action!=Action::Return||!r.room_return_context||!r.expected_room_pose_revision)))return {{},Error::InvalidArgument};return OperateCurrent(r,fence,&scene,&profile);}
    Result<Receipt> Service::OperateCurrent(const Request& r,InventorySessionFence& fence,const Scene::HouseScenePolicy* room_scene,const InteriorContent::ProfilePolicy* room_profile,Database* parent_transaction,const OccupiedRetirement::Authority* retirement){
        if(retirement&&(!parent_transaction||!retirement->AllowsReturn(*parent_transaction,r,fence)))return {{},Error::Unauthorized};
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(r.initial_stowed&&(r.action!=Action::Store||!r.placement))return {{},Error::InvalidArgument};
        if(CrateGeometry::TemplateDefinition(r.expected.values.item_id)&&(!crates_||r.house||r.initial_stowed||(r.action==Action::Store?(!r.placement):(!r.crate||!r.crate->crate_id||!r.crate->expected_revision||r.crate->expected_revision==UINT64_MAX))))return {{},Error::UnsupportedItem};
        if(r.crate&&(!CrateGeometry::TemplateDefinition(r.expected.values.item_id)||(r.action!=Action::Return&&r.action!=Action::Store)||r.house||r.initial_stowed||!r.crate->crate_id||!r.crate->expected_revision||r.crate->expected_revision==UINT64_MAX))return {{},Error::InvalidArgument};
        if(r.house&&(!houses_||(!r.house->house_id!=!r.house->expected_revision)||(r.action==Action::Return&&!r.house->house_id)))return {{},Error::InvalidArgument};
        if(r.placement&&r.placement->reservation.content_binding&&r.placement->reservation.content_binding->version==ContentBinding::HouseVersion&&!r.house)return {{},Error::InvalidArgument};
        if((r.placement&&(r.action!=Action::Store||r.placement->pose_revision!=1))||!Key(r.request_key)||!LeaseShape(r.lease)||!r.estate_id||!Valid(r.expected)||(r.action==Action::Store&&r.expected.values.character_id!=r.lease.actor.character_id)||!General(r.expected.values.slot_id)||(r.action!=Action::Store&&r.action!=Action::Return)||(r.action==Action::Store?(r.custody_id||r.destination_slot||r.verified_runtime_serial<=0):(!r.custody_id||!General(r.destination_slot))))return {
            {
            },Error::InvalidArgument
        };
        if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {
            {
            },Error::UnheldFence
        };
        auto* connection=parent_transaction?parent_transaction:database_.get();
        if(!connection||connection->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        auto execute=[&]()->Receipt{
            auto& db=*connection;auto lease=Locked(db,r.lease);
            // Preserve legacy storage-only Operate authority semantics. Explicit
            // Resolve remains available for its historical committed outcomes.
            if(r.action==Action::Store&&!r.placement&&!r.vault)Estate(db,r.estate_id,r.lease.actor);
            auto key=Quote(db,r.request_key);auto bind=Binding(r);auto old=Q(db,"SELECT "+receipt_columns+" FROM housing_live_item_receipts WHERE request_key="+key+" FOR UPDATE");if(old.RowCount()){
                auto row=old.begin();auto out=ReceiptRow(row,lease,r.request_key);if(Unhex(row[4])!=bind)throw Failure{
                    Error::Conflict
                };if(out.stored.estate_id!=r.estate_id||Encode(out.partition?out.partition->before:out.stored.row)!=Encode(r.expected)||out.action!=r.action)throw Failure{
                    Error::DatabaseFailure
                };
                if(!out.stored.placement&&!r.vault)Estate(db,r.estate_id,r.lease.actor);
                AttachHouse(db,lease,out,fence,houses_);MatchHouseRequest(r,out);AttachRoom(db,out);ValidatePendingCrate(db,out);if(out.requires_application)PartitionPostimage(db,out);
                return out;
            }
            if(r.action==Action::Store&&r.crate_member&&!NpcItemMetadata::OrdinaryUnderTransaction(db,r.expected.values.item_id))throw Failure{Error::UnsupportedItem};
            if(!lease.pending_key.empty())throw Failure{
                Error::PendingApplication
            };
            // Historical own outcomes above require no newly granted permission.
            // A new placed Store is authorized again inside StorePose. Unplaced
            // legacy storage retains its original personal-owner restriction.
            if(r.action==Action::Store&&!r.placement&&!r.vault)Estate(db,r.estate_id,r.lease.actor);
            std::optional<VaultStorage::Target> vault_target;
            if(r.vault){
                if(!VaultStorage::Installed(db))throw Failure{Error::Disabled};
                vault_target=VaultStorage::CurrentTarget(db,lease,r.estate_id,*r.vault,fence,room_scene,room_profile);
                if(r.action==Action::Store){if(!vault_target->access.Allows(AccessPolicy::Capability::AddItem))throw Failure{Error::Unauthorized};if(auto capacity=VaultStorage::CapacityError(*vault_target,VaultStorage::Count(db,*vault_target));capacity!=Error::None)throw Failure{capacity};}
                else{const auto current=VaultStorage::ContextItems(db,*r.vault);auto found=std::find_if(current.begin(),current.end(),[&](const auto&v){return v.stored.custody_id==r.custody_id&&v.native_item_id==r.vault->expected_native_item_id&&v.revision==r.vault->expected_revision&&v.membership_revision==r.vault->member_revision;});if(found==current.end()||found->stored.estate_id!=r.estate_id||Encode(found->stored.row)!=Encode(r.expected)||found->stored.row.values.character_id!=lease.actor.character_id)throw Failure{Error::Conflict};}
            }
            if(!r.house)RejectUnadaptedHouse(db,r);
            std::optional<InteriorContent::PlacementReview> room_review;
            if(r.room&&r.action==Action::Store){
                if(!room_scene||!room_profile)throw Failure{Error::InvalidArgument};
                auto reviewed=InteriorContent::ReviewInventoryAddUnderTransaction(db,lease,r.room->source,fence,*room_scene,*room_profile,r.expected,r.verified_runtime_serial,r.initial_stowed?std::nullopt:std::optional<PlacementPolicy::Transform>(r.placement->transform),r.room_exterior_source?std::optional<InteriorContent::AttachedSelection>{{r.estate_id,r.room->house_id,*r.room_exterior_source,crates_}}:std::nullopt,!r.crate_member,r.placement->npc_frame,r.room->content_digest);if(!reviewed)throw Failure{reviewed.error};const auto& view=reviewed.value.profile;
                if(!view.context.house||view.context.house->house_id!=r.room->house_id||view.context.house->revision!=r.room->house_revision||view.profile_revision!=r.room->profile_revision||view.context.context.estate_id!=r.estate_id||reviewed.value.identity.digest!=r.room->content_digest)throw Failure{Error::Conflict};
                MatchRoomTag(db,*r.room);auto reserved=Q(db,"SELECT "+ReservationColumns(db)+" FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(r.room->placement.native_item_id)+" FOR UPDATE");if(reserved.RowCount()!=1)throw Failure{Error::NotFound};auto row=reserved.begin();HeldReservation(row,lease);auto base=ReservationRow(row);
                if(std::stoul(row[11])!=0||row[12]||base.content_binding||PlacementBytes({base,r.placement->transform,1,r.placement->npc_frame})!=PlacementBytes(*r.placement)||Unhex(row[9])!=Encode(r.expected)||std::stol(row[10])!=r.verified_runtime_serial)throw Failure{Error::Conflict};
                room_review=std::move(reviewed.value);
            }
            std::shared_ptr<const CrateBinding::Allocation> count_container_before;LayoutCrateMember::Proof count_member_before;
            if(r.crate_member){
                if(!crates_||r.crate)throw Failure{Error::InvalidArgument};const auto& m=*r.crate_member;
                if(r.room){
                    if(!room_scene||!room_profile)throw Failure{Error::InvalidArgument};const auto& room=*r.room;
                    auto read=r.room_exterior_source?Scene::Service::ReadCurrentAttachedInteriorUnderTransaction(db,lease,room.house_id,fence,*room_scene,room_profile,crates_):Scene::Service::ReadInteriorSceneUnderTransaction(db,lease,room.source,fence,*room_scene,*room_profile,crates_);if(!read)throw Failure{read.error};const auto& view=read.value;
                    const auto physical=view.exterior_source.value_or(view.context.source);
                    if(view.context.source.zone_id!=room.source.zone_id||view.context.source.zone_version!=room.source.zone_version||view.context.source.instance_id!=room.source.instance_id||(r.room_exterior_source&&(!view.exterior_source||physical.zone_id!=r.room_exterior_source->zone_id||physical.zone_version!=r.room_exterior_source->zone_version||physical.instance_id!=r.room_exterior_source->instance_id)))throw Failure{Error::Conflict};
                    if(!view.house||!view.house->interior||view.house->house_id!=room.house_id||view.house->revision!=room.house_revision||view.profile_revision!=room.profile_revision||view.house->interior->native_estate_id!=m.native_plot_id||view.context.catalog_revision!=m.catalog_revision||physical.zone_id!=m.source.zone_id||physical.zone_version!=m.source.zone_version||physical.instance_id!=m.source.instance_id)throw Failure{Error::Conflict};
                    auto container=std::find_if(view.contents.begin(),view.contents.end(),[&](const auto& i){return i.crate&&i.crate->binding.crate_id==m.crate_id;});
                    auto member=std::find_if(view.crate_contents.begin(),view.crate_contents.end(),[&](const auto& i){return i.stored.custody_id==r.custody_id&&i.contained_in_crate==m.crate_id;});
                    if(container==view.contents.end()||container->stored.stowed||container->crate->packed||container->crate->binding.revision!=m.revision||container->crate->created_by_character_id!=lease.actor.character_id||(r.action==Action::Return&&(member==view.crate_contents.end()||member->crate||!member->room||!member->stored.stowed||Encode(member->stored.row)!=Encode(r.expected))))throw Failure{Error::Conflict};
                    if(r.action==Action::Store){
                        if(!r.initial_stowed||!r.placement||r.house||CrateGeometry::TemplateDefinition(r.expected.values.item_id)||!view.access.effective.Allows(AccessPolicy::Capability::AddItem)||r.placement->reservation.native_estate_id!=m.native_plot_id)throw Failure{Error::Unauthorized};
                        auto definition=CrateBinding::ReadDefinitionUnderTransaction(db,*container->crate);if(!definition)throw Failure{definition.error};size_t count=0;for(const auto& i:view.crate_contents)if(i.contained_in_crate==m.crate_id)++count;if(!definition.value.max_items||count>=definition.value.max_items)throw Failure{Error::Conflict};
                    }
                }else{
                auto read=Scene::Service::ReadSceneUnderTransaction(db,lease,r.estate_id,fence,room_scene,crates_);if(!read)throw Failure{read.error};const auto& yard=read.value;
                if(yard.native_plot_id!=m.native_plot_id||yard.current_catalog_revision!=m.catalog_revision||yard.route.zone_id!=m.source.zone_id||yard.route.zone_version!=m.source.zone_version||yard.route.instance_id!=m.source.instance_id||(r.action==Action::Store&&!yard.access.Allows(AccessPolicy::Capability::AddItem)))throw Failure{Error::Unauthorized};
                auto container=std::find_if(yard.items.begin(),yard.items.end(),[&](const auto& item){return item.crate&&item.crate->binding.crate_id==m.crate_id;});
                if(container==yard.items.end()||container->stored.stowed||container->crate->packed||container->crate->binding.revision!=m.revision||container->crate->created_by_character_id!=lease.actor.character_id||(r.action==Action::Store&&(m.native_plot_id!=r.placement->reservation.native_estate_id||m.catalog_revision!=r.placement->reservation.catalog_revision)))throw Failure{Error::Conflict};
                if(r.action==Action::Return){
                    auto member=std::find_if(yard.crate_contents.begin(),yard.crate_contents.end(),[&](const auto& item){return item.stored.custody_id==r.custody_id&&item.contained_in_crate==m.crate_id;});
                    if(member==yard.crate_contents.end()||member->crate||!member->stored.placement||!member->stored.stowed||Encode(member->stored.row)!=Encode(r.expected))throw Failure{Error::Conflict};
                    if(bool(member->house)!=bool(r.house)||(member->house&&(member->house->house_id!=r.house->house_id||member->house->revision!=r.house->expected_revision||!yard.access.Allows(AccessPolicy::Capability::RemoveHouse))))throw Failure{Error::Unauthorized};
                }else{
                if(r.house&&(!houses_||!yard.access.Allows(AccessPolicy::Capability::AddHouse)))throw Failure{Error::Unauthorized};
                auto catalog=Q(db,"SELECT HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,container->crate->binding.catalog_key)+" AND revision="+std::to_string(m.catalog_revision)+" FOR UPDATE");if(catalog.RowCount()!=1)throw Failure{Error::Conflict};auto bundle=Housing::Catalog::DecodePayloads(Unhex(catalog.begin()[0]),Unhex(catalog.begin()[1]));auto definition=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==container->crate->binding.definition_id&&d.type==7;});
                size_t total=0;for(const auto& member:yard.crate_contents)if(member.contained_in_crate==m.crate_id)++total;
                if(definition==bundle.definitions.end()||total>=definition->max_items)throw Failure{Error::Conflict};
                }
            }
            }
            if(r.partition_quantity&&r.crate_member){
                auto container=CountContainer(db,r.crate_member->crate_id);count_container_before=CurrentCrate(db,container);
                if(!count_container_before||count_container_before->binding.revision!=r.crate_member->revision)throw Failure{Error::Conflict};
                if(r.action==Action::Return){auto rows=Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id="+std::to_string(r.custody_id)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::Conflict};auto member=StoredRow(db,rows.begin(),r.custody_id);member.placement=CurrentPose(db,r.custody_id,true);auto marker=ItemStorage::Locked(db,r.custody_id);member.stowed=marker.stowed;member.storage_revision=marker.revision;count_member_before=CountMember(db,member,r.crate_member->crate_id);}
            }
            Q(db,"INSERT INTO housing_live_item_receipts(request_key,character_id,account_id,lease_epoch,session_key,binding_blob) VALUES ("+key+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+","+std::to_string(r.lease.epoch)+","+Quote(db,r.lease.session_key)+",x'"+Hex(bind)+"')");Receipt out;out.request_key=r.request_key;out.action=r.action;out.house_effect_required=bool(r.house);out.room=r.room;
            std::unique_ptr<HouseBinding::TransactionHelpers> house_helper;
            std::optional<HouseBinding::Prepared> house_prepared;
            if(r.house){
                house_helper=std::make_unique<HouseBinding::TransactionHelpers>(db,fence,houses_->Authorization());
                if(r.action==Action::Store)house_prepared=house_helper->PrepareStore(lease,r.request_key,r.expected,r.verified_runtime_serial,r.house->house_id,r.house->expected_revision,houses_->Templates());
                else house_prepared=house_helper->PrepareReturn(lease,r.request_key,r.house->house_id,r.house->expected_revision,r.custody_id,r.expected,r.destination_slot);
            }
            if(r.action==Action::Store){
                if(!fence.SourceMatches(r.expected,r.verified_runtime_serial))throw Failure{
                    Error::UnheldFence
                };auto rows=Q(db,"SELECT "+select_inventory+" FROM inventory WHERE "+Slot(r.lease.actor.character_id,r.expected.values.slot_id)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{
                    Error::Conflict
                };auto actual=ItemPayload::Complete(db,InvRow(rows.begin()),&r.expected);ItemPayload::ValidateInventory(db,actual);if(Encode(actual)!=Encode(r.expected))throw Failure{
                    Error::Conflict
                };
                if(r.crate){auto carried=CrateBinding::ReadCarriedUnderTransaction(db,actual);if(!carried||carried.value.binding.crate_id!=r.crate->crate_id||carried.value.binding.revision!=r.crate->expected_revision)throw Failure{Error::Conflict};out.crate_before=std::make_shared<CrateBinding::Allocation>(std::move(carried.value));}
                const auto full_source=actual;if(r.partition_quantity){auto part=std::make_shared<StackPartition>();part->member_container_before=count_container_before;part->member_before=count_member_before;part->maximum=PartitionMaximum(db,r,actual);if(r.room)part->room_source=InteriorMoveContext{r.room->house_id,r.room->house_revision,r.room->profile_revision,1,r.room->source};if(r.vault){part->vault_source=r.vault;part->vault_scope_revision=r.vault->house_id?1:0;}part->before=actual;part->after=actual;part->quantity=r.partition_quantity;part->serial=r.partition_serial;part->after.values.charges=StackCount(actual)-part->quantity;part->after.null_fields&=~NullCharges;actual.values.charges=part->quantity;actual.values.guid=part->serial;actual.null_fields&=~uint8_t(NullCharges|NullGuid);out.partition=part;}
                auto catalog=Simple(db,actual,bool(r.vault));auto stored=Q(db,"INSERT INTO housing_live_item_custody(estate_id,owner_character_id,schema_descriptor,row_blob,catalog_blob) VALUES ("+std::to_string(r.estate_id)+","+std::to_string(r.lease.actor.character_id)+","+Quote(db,SchemaDescriptor())+",x'"+Hex(Encode(actual))+"',x'"+Hex(Cat(catalog))+"')");if(stored.RowsAffected()!=1||!stored.LastInsertedID())throw Failure{
                    Error::DatabaseFailure
                };auto removed=out.partition?Q(db,"UPDATE inventory SET charges="+std::to_string(out.partition->after.values.charges)+" WHERE "+Slot(actual.values.character_id,actual.values.slot_id)+" AND charges="+std::to_string(full_source.values.charges)):Q(db,"DELETE FROM inventory WHERE "+Slot(actual.values.character_id,actual.values.slot_id));if(removed.RowsAffected()!=1)throw Failure{
                    Error::Conflict
                };if(!out.partition){ItemPayload::Hold(db,actual,stored.LastInsertedID());ItemPayload::RemoveChildren(db,actual);}out.stored={
                    stored.LastInsertedID(),r.estate_id,actual,catalog
                };
                if(r.vault){
                    const auto& target=*vault_target;PlacementTables(db);
                    auto native=Q(db,"INSERT INTO housing_live_placement_reservations(request_key,native_estate_id,estate_id,catalog_key,catalog_revision,character_id,account_id,lease_epoch,session_key,source_row,runtime_serial,state,custody_id) VALUES("+Quote(db,r.request_key)+","+std::to_string(target.native)+","+std::to_string(r.estate_id)+","+Quote(db,target.catalog_key)+","+std::to_string(target.catalog_revision)+","+std::to_string(lease.actor.character_id)+","+std::to_string(lease.actor.account_id)+","+std::to_string(lease.epoch)+","+Quote(db,lease.session_key)+",x'"+Hex(Encode(actual))+"',"+std::to_string(r.verified_runtime_serial)+",1,"+std::to_string(out.stored.custody_id)+")");if(native.RowsAffected()!=1||!native.LastInsertedID()||native.LastInsertedID()>=UINT32_MAX)throw Failure{Error::Conflict};
                    auto marker=Q(db,"INSERT INTO housing_live_vault_items(custody_id,native_item_id,realm_estate_id,house_id,revision) VALUES("+std::to_string(out.stored.custody_id)+","+std::to_string(native.LastInsertedID())+","+std::to_string(r.vault->estate_id)+","+(target.house_id?std::to_string(target.house_id):"NULL")+",1)");if(marker.RowsAffected()!=1)throw Failure{Error::Conflict};
                    VaultStorage::CrateWrite(db,*r.vault,out.stored,1,1);
                    if(target.house_id){auto scope=Q(db,"INSERT INTO housing_house_item_scopes(custody_id,house_id,revision) VALUES("+std::to_string(out.stored.custody_id)+","+std::to_string(target.house_id)+",1)");if(scope.RowsAffected()!=1)throw Failure{Error::Conflict};}
                }
                if(r.placement){PlacementTables(db);
                    auto npc=NpcRuntime::Current(db,r.expected.values.item_id);if(npc)(void)NpcRuntime::Geometry(*npc,r.placement->npc_frame,r.placement->transform);else if(r.placement->npc_frame)throw Failure{Error::InvalidArgument};
                    if(r.room){
                        auto scoped=Q(db,"INSERT INTO housing_house_item_scopes(custody_id,house_id,revision) VALUES("+std::to_string(out.stored.custody_id)+","+std::to_string(r.room->house_id)+",1)");if(scoped.RowsAffected()!=1)throw Failure{Error::Conflict};
                        auto pose=Q(db,"INSERT INTO housing_live_placed_items(custody_id,native_item_id,pose_revision,pose_blob) VALUES("+std::to_string(out.stored.custody_id)+","+std::to_string(r.placement->reservation.native_item_id)+",1,x'"+Hex(Pose(r.placement->transform))+"')");if(pose.RowsAffected()!=1)throw Failure{Error::Conflict};
                        auto reserved=Q(db,"UPDATE housing_live_placement_reservations SET state=1,custody_id="+std::to_string(out.stored.custody_id)+" WHERE native_item_id="+std::to_string(r.placement->reservation.native_item_id)+" AND state=0 AND custody_id IS NULL");if(reserved.RowsAffected()!=1)throw Failure{Error::Conflict};out.stored.placement=r.placement;
                    }else StorePose(db,r,out.stored,fence,houses_,crates_);
                    NpcNativeFrame::InsertUnderTransaction(db,out.stored.custody_id,r.placement->reservation.native_item_id,r.placement->pose_revision,r.placement->transform,r.placement->npc_frame);
                    if(out.partition){auto changed=Q(db,"UPDATE housing_live_placement_reservations SET source_row=x'"+Hex(Encode(out.stored.row))+"' WHERE native_item_id="+std::to_string(out.stored.placement->reservation.native_item_id)+" AND state=1 AND custody_id="+std::to_string(out.stored.custody_id)+" AND source_row=x'"+Hex(Encode(r.expected))+"'");if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};}
                }
                if(r.initial_stowed){
                    if(!out.stored.placement||!ItemStorage::Installed(db))throw Failure{Error::Disabled};
                    auto marker=Q(db,"INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+std::to_string(out.stored.custody_id)+",1,1,"+std::to_string(out.stored.placement->reservation.native_estate_id)+")");if(marker.RowsAffected()!=1)throw Failure{Error::Conflict};out.stored.storage_revision=1;out.stored.stowed=true;
                }
                if(CrateGeometry::TemplateDefinition(r.expected.values.item_id)){
                    if(!crates_)throw Failure{Error::UnsupportedItem};
                    auto bound=r.room?CrateBinding::BindInteriorPlacedUnderTransaction(db,r,fence,*crates_,*room_review,out.crate_before.get()):(out.crate_before?CrateBinding::RebindPlacedUnderTransaction(db,r,fence,*crates_,*out.crate_before,houses_):CrateBinding::BindNewPlacedUnderTransaction(db,r,fence,*crates_));
                    if(!bound)throw Failure{bound.error};
                    out.crate_allocation=CurrentCrate(db,out.stored);
                }
                if(house_prepared){
                    auto interior=houses_->AllocateInterior(db,house_prepared->Before(),r);
                    out.house_effect=std::make_shared<HouseBinding::Effect>(house_helper->FinishStore(*house_prepared,out.stored.custody_id,interior,houses_->Routes()));
                    if(!houses_->FinalizeInteriorUnderTransaction(db,fence,*out.house_effect,r))throw Failure{Error::Unauthorized};
                }
                if(r.crate_member){
                    const auto& m=*r.crate_member;if(!out.stored.placement||!out.stored.stowed||CrateGeometry::TemplateDefinition(r.expected.values.item_id))throw Failure{Error::UnsupportedItem};
                    if(out.house_effect&&!CrateHouseRules::CanAdmit(db,m.crate_id,out.house_effect->after.house_id))throw Failure{Error::Conflict};
                    auto inserted=Q(db,"INSERT INTO housing_crate_members(custody_id,crate_id,revision,origin_native_estate_id,origin_house_id,origin_pose_blob,origin_stowed) VALUES("+std::to_string(out.stored.custody_id)+","+std::to_string(m.crate_id)+",1,"+std::to_string(m.native_plot_id)+","+(r.room?std::to_string(r.room->house_id):"NULL")+",x'"+Hex(Pose(out.stored.placement->transform))+"',1)");if(inserted.RowsAffected()!=1)throw Failure{Error::Conflict};
                    auto changed=Q(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+std::to_string(m.crate_id)+" AND revision="+std::to_string(m.revision)+" AND packed=0");if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};out.crate_member=m;++out.crate_member->revision;
                }
            }
            else{
                if(!RentState::HouseActive(db,r.custody_id))throw Failure{Error::Unauthorized};
                auto rows=Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id="+std::to_string(r.custody_id)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{
                    Error::NotFound
                };auto stored=StoredRow(db,rows.begin(),r.custody_id);if(!r.vault&&VaultStorage::Installed(db)){auto v=Q(db,"SELECT custody_id FROM housing_live_vault_items WHERE custody_id="+std::to_string(r.custody_id)+" FOR UPDATE");if(v.RowCount())throw Failure{Error::Unauthorized};}auto room_scope=HouseItemScope::Locked(db,r.custody_id);if(r.vault){if(bool(room_scope)!=bool(r.vault->house_id)||(room_scope&&room_scope->house_id!=r.vault->house_id))throw Failure{Error::Conflict};}else if(bool(room_scope)!=bool(r.room))throw Failure{Error::Unauthorized};if(stored.estate_id!=r.estate_id)throw Failure{
                    Error::Unauthorized
                };
                auto pose_tables=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_placed_items'");
                if(std::stoul(pose_tables.begin()[0])){PlacementTables(db);stored.placement=CurrentPose(db,r.custody_id,bool(r.crate_member));}
                const bool foreign=stored.row.values.character_id!=lease.actor.character_id;
                if(foreign){if(r.house||r.crate||r.vault)throw Failure{Error::UnsupportedItem};ForeignCustody(db,stored);}
                if(r.room){
                    if(!room_scene||!room_profile||!room_scope||!stored.placement||!r.room_return_context)throw Failure{Error::Conflict};const auto& cached=*r.room_return_context;
                    auto view=r.room_exterior_source?Scene::Service::ReadCurrentAttachedInteriorUnderTransaction(db,lease,cached.house_id,fence,*room_scene,room_profile,crates_):Scene::Service::ReadInteriorSceneUnderTransaction(db,lease,cached.source,fence,*room_scene,*room_profile,crates_);if(!view)throw Failure{view.error};
                    if(view.value.context.source.zone_id!=cached.source.zone_id||view.value.context.source.zone_version!=cached.source.zone_version||view.value.context.source.instance_id!=cached.source.instance_id)throw Failure{Error::Conflict};
                    if(r.room_exterior_source&&(!view.value.exterior_source||view.value.exterior_source->zone_id!=r.room_exterior_source->zone_id||view.value.exterior_source->zone_version!=r.room_exterior_source->zone_version||view.value.exterior_source->instance_id!=r.room_exterior_source->instance_id))throw Failure{Error::Conflict};
                    if(!view.value.house||view.value.house->house_id!=cached.house_id||view.value.house->revision!=cached.house_revision||view.value.profile_revision!=cached.profile_revision||room_scope->house_id!=cached.house_id||room_scope->revision!=cached.scope_revision||stored.placement->pose_revision!=r.expected_room_pose_revision||r.room->house_id!=cached.house_id)throw Failure{Error::Conflict};
                    const auto& members=r.crate_member?view.value.crate_contents:view.value.contents;auto item=std::find_if(members.begin(),members.end(),[&](const auto& c){return c.stored.custody_id==r.custody_id;});if(item==members.end()||!item->stored.placement||PlacementBytes(*item->stored.placement)!=PlacementBytes(*stored.placement)||PlacementBytes({r.room->placement,{0,0,0,1,{0,0,0}},1})!=PlacementBytes({stored.placement->reservation,{0,0,0,1,{0,0,0}},1}))throw Failure{Error::Conflict};
                    // Original creator pickup bypasses ordinary RemoveItem/CanDrop;
                    // creator equality is already proven from immutable custody.
                    if(r.crate&&!view.value.access.effective.Allows(AccessPolicy::Capability::RemoveCrate))throw Failure{Error::Unauthorized};
                    if(foreign&&!view.value.access.effective.Allows(AccessPolicy::Capability::RemoveItem))throw Failure{Error::Unauthorized};
                    MatchRoomTag(db,*r.room);
                }else if(stored.placement){
                    // Native5b74c4 creator-match bypasses ordinary RemoveItem8 and
                    // CanDrop. Creator was proven above from immutable server rows;
                    // never accept the packet's displayed owner name as proof.
                    CurrentAccess(db,r.estate_id,stored.placement->reservation.native_estate_id,r.lease.actor,CrateGeometry::TemplateDefinition(stored.row.values.item_id)?std::optional<AccessPolicy::Capability>(AccessPolicy::Capability::RemoveCrate):(foreign?std::optional<AccessPolicy::Capability>(AccessPolicy::Capability::RemoveItem):std::nullopt),bool(retirement));
                    // Only resolve current routing, not an Add permission or a
                    // new placement: removal preserves the original creator.
                    CurrentRemovalContext(db,*stored.placement,fence);
                }else if(!r.vault)Estate(db,r.estate_id,r.lease.actor);
                if(Encode(stored.row)!=Encode(r.expected))throw Failure{
                    Error::Conflict
                };auto catalog=Simple(db,stored.row,bool(r.vault));if(Cat(catalog)!=Cat(stored.catalog))throw Failure{
                    Error::UnsupportedItem
                };
                if(r.crate){
                    out.crate_before=CurrentCrate(db,stored);
                    if(!out.crate_before||out.crate_before->binding.crate_id!=r.crate->crate_id||out.crate_before->binding.revision!=r.crate->expected_revision||out.crate_before->packed||!crates_||crates_->ManifestDigest()!=out.crate_before->content_manifest_sha256||!crates_->ReviewPickupUnderTransaction(db,fence,lease,stored,*out.crate_before))throw Failure{Error::Unauthorized};
                    if(r.room){auto released=CrateBinding::ReleaseInteriorMemberScopesUnderTransaction(db,*out.crate_before);if(!released)throw Failure{released.error};}
                    // CurrentCrate validates each retained member under these same locks.
                    // Contents stay in crate custody; only the container's location changes.
                }
                auto after=stored.row;ItemPayload::Owner(after,lease.actor.character_id);after.values.slot_id=r.destination_slot;
                if(r.partition_quantity){if(!stored.placement&&!r.vault)throw Failure{Error::UnsupportedItem};auto part=std::make_shared<StackPartition>();part->member_container_before=count_container_before;part->member_before=count_member_before;part->maximum=PartitionMaximum(db,r,stored.row);if(r.room)part->room_source=r.room_return_context;if(r.vault){part->vault_source=r.vault;part->vault_scope_revision=room_scope?room_scope->revision:0;}part->before=stored.row;part->after=stored.row;part->quantity=r.partition_quantity;part->serial=r.partition_serial;part->after.values.charges=StackCount(stored.row)-part->quantity;part->after.null_fields&=~NullCharges;auto marker=r.vault?ItemStorage::State{r.vault->expected_revision,false,r.vault->native_estate_id}:ItemStorage::Locked(db,stored.custody_id);if(marker.revision==UINT64_MAX||(marker.revision&&marker.native_estate_id!=(r.vault?r.vault->native_estate_id:stored.placement->reservation.native_estate_id)))throw Failure{Error::Conflict};part->storage_before=marker.revision;part->storage_after=marker.revision+1;part->stowed=marker.stowed;after.values.charges=part->quantity;after.values.guid=part->serial;after.null_fields&=~uint8_t(NullCharges|NullGuid);out.partition=part;}
                if(!fence.ReturnAllowed(after))throw Failure{
                    Error::UnheldFence
                };NoChildren(db,after.values.character_id,after.values.slot_id);auto destination=Q(db,"SELECT slot_id FROM inventory WHERE "+Slot(after.values.character_id,after.values.slot_id)+" FOR UPDATE");if(destination.RowCount())throw Failure{
                    Error::Conflict
                };if(!out.partition)after=ItemPayload::Release(db,stored.row,stored.custody_id,lease.actor.character_id,r.destination_slot);ItemPayload::Insert(db,after);
                if(out.crate_before){
                    auto carried=std::make_shared<CrateBinding::Allocation>(*out.crate_before);const auto& before=out.crate_before->binding;
                    carried->binding.revision++;carried->binding.custody_id=0;carried->inventory_character_id=after.values.character_id;carried->inventory_slot_id=after.values.slot_id;
                    auto location=Q(db,"UPDATE housing_crate_locations SET location_kind=1,character_id="+std::to_string(carried->inventory_character_id)+",slot_id="+std::to_string(carried->inventory_slot_id)+",custody_id=NULL WHERE crate_id="+std::to_string(before.crate_id)+" AND location_kind=2 AND custody_id="+std::to_string(before.custody_id));
                    auto identity=Q(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+std::to_string(before.crate_id)+" AND revision="+std::to_string(before.revision)+" AND packed=0");
                    auto native=Q(db,"UPDATE housing_native_estates SET active=0 WHERE native_id="+std::to_string(before.native_estate_id)+" AND parent_native_id="+std::to_string(before.parent_native_estate_id)+" AND active=1");
                    if(location.RowsAffected()!=1||identity.RowsAffected()!=1||native.RowsAffected()!=1)throw Failure{Error::Conflict};out.crate_allocation=std::move(carried);
                }
                if(house_prepared)out.house_effect=std::make_shared<HouseBinding::Effect>(house_helper->FinishReturn(*house_prepared,after));
                if(r.crate_member){const auto& m=*r.crate_member;
                    if(!out.partition){auto removed=Q(db,"DELETE FROM housing_crate_members WHERE custody_id="+std::to_string(r.custody_id)+" AND crate_id="+std::to_string(m.crate_id));if(removed.RowsAffected()!=1)throw Failure{Error::Conflict};}
                    auto changed=Q(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+std::to_string(m.crate_id)+" AND revision="+std::to_string(m.revision)+" AND packed=0");if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};out.crate_member=m;++out.crate_member->revision;
                }
                if(out.partition){const auto& p=*out.partition;auto write=[&](const std::string& sql){if(Q(db,sql).RowsAffected()!=1)throw Failure{Error::Conflict};};write("UPDATE housing_live_item_custody SET row_blob=x'"+Hex(Encode(p.after))+"' WHERE custody_id="+std::to_string(stored.custody_id)+" AND row_blob=x'"+Hex(Encode(p.before))+"'");const auto native=r.vault?r.vault->expected_native_item_id:stored.placement->reservation.native_item_id;write("UPDATE housing_live_placement_reservations SET source_row=x'"+Hex(Encode(p.after))+"' WHERE native_item_id="+std::to_string(native)+" AND state=1 AND custody_id="+std::to_string(stored.custody_id)+" AND source_row=x'"+Hex(Encode(p.before))+"'");if(r.vault)write("UPDATE housing_live_vault_items SET revision=revision+1 WHERE custody_id="+std::to_string(stored.custody_id)+" AND native_item_id="+std::to_string(native)+" AND revision="+std::to_string(p.storage_before));else if(p.storage_before)write("UPDATE housing_live_item_storage SET revision=revision+1 WHERE custody_id="+std::to_string(stored.custody_id)+" AND revision="+std::to_string(p.storage_before));else write("INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+std::to_string(stored.custody_id)+",1,0,"+std::to_string(native==0?0:stored.placement->reservation.native_estate_id)+")");out.stored=stored;out.inventory_after=after;
                }else{
                ItemStorage::Retire(db,r.custody_id);
                if(r.vault){
                    auto marker=Q(db,"DELETE FROM housing_live_vault_items WHERE custody_id="+std::to_string(r.custody_id)+" AND native_item_id="+std::to_string(r.vault->expected_native_item_id)+" AND revision="+std::to_string(r.vault->expected_revision));if(marker.RowsAffected()!=1)throw Failure{Error::Conflict};
                    if(r.vault->house_id){auto scope=Q(db,"DELETE FROM housing_house_item_scopes WHERE custody_id="+std::to_string(r.custody_id)+" AND house_id="+std::to_string(r.vault->house_id));if(scope.RowsAffected()!=1)throw Failure{Error::Conflict};}
                    auto retired=Q(db,"UPDATE housing_live_placement_reservations SET state=2,custody_id=NULL WHERE native_item_id="+std::to_string(r.vault->expected_native_item_id)+" AND state=1 AND custody_id="+std::to_string(r.custody_id));if(retired.RowsAffected()!=1)throw Failure{Error::Conflict};
                }
                if(r.room){auto removed_scope=Q(db,"DELETE FROM housing_house_item_scopes WHERE custody_id="+std::to_string(r.custody_id)+" AND house_id="+std::to_string(r.room->house_id)+" AND revision="+std::to_string(r.room_return_context->scope_revision));if(removed_scope.RowsAffected()!=1)throw Failure{Error::Conflict};}
                auto placement_table=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_placed_items'");if(std::stoul(placement_table.begin()[0])){PlacementTables(db);stored.placement=CurrentPose(db,r.custody_id);if(stored.placement){NpcNativeFrame::RetireUnderTransaction(db,r.custody_id,stored.placement->reservation.native_item_id,stored.placement->pose_revision,stored.placement->npc_frame);Q(db,"DELETE FROM housing_live_placed_items WHERE custody_id="+std::to_string(r.custody_id));Q(db,"UPDATE housing_live_placement_reservations SET state=2,custody_id=NULL WHERE native_item_id="+std::to_string(stored.placement->reservation.native_item_id));}}auto removed=Q(db,"DELETE FROM housing_live_item_custody WHERE custody_id="+std::to_string(r.custody_id));if(removed.RowsAffected()!=1)throw Failure{
                    Error::Conflict
                };out.stored=stored;out.inventory_after=after;ValidatePendingCrate(db,out);
                } // Whole Return retires storage, NPC frame, pose, and custody; partition retains source.
                if(r.vault){auto current=out.stored;if(out.partition)current.row=out.partition->after;VaultStorage::CrateWrite(db,*r.vault,current,out.partition?out.partition->storage_after:0,out.partition?2:0);}
            }
            if(out.partition&&out.crate_member){auto part=std::make_shared<StackPartition>(*out.partition);auto stored=out.stored;stored.row=out.action==Action::Store?out.stored.row:part->after;stored.stowed=true;stored.storage_revision=out.action==Action::Store?1:part->storage_after;part->member_after=CountMember(db,stored,out.crate_member->crate_id);out.partition=part;PartitionShape(out);MemberCountPostimage(db,out);}
            auto complete=Q(db,"UPDATE housing_live_item_receipts SET completed=1,after_blob=x'"+Hex(Outcome(out))+"' WHERE request_key="+key+" AND completed=0");if(complete.RowsAffected()!=1)throw Failure{
                Error::DatabaseFailure
            };auto pending=Q(db,"UPDATE housing_live_item_leases SET pending_key="+key+" WHERE character_id="+std::to_string(r.lease.actor.character_id)+" AND pending_key IS NULL");if(pending.RowsAffected()!=1)throw Failure{
                Error::PendingApplication
            };return out;
        };
        if(parent_transaction){
            try{
                auto tx=Q(*connection,"SELECT @@in_transaction");
                if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return {{},Error::InvalidArgument};
                Verify(*connection);
                return {execute(),Error::None};
            }catch(const Failure& failure){return {{},failure.error};}
            catch(const HouseBinding::Failure& failure){
                Error error=Error::DatabaseFailure;
                switch(failure.error){
                case HouseBinding::Error::InvalidArgument:error=Error::InvalidArgument;break;
                case HouseBinding::Error::Unauthorized:error=Error::Unauthorized;break;
                case HouseBinding::Error::UnheldFence:error=Error::UnheldFence;break;
                case HouseBinding::Error::Conflict:error=Error::Conflict;break;
                case HouseBinding::Error::PendingApplication:error=Error::PendingApplication;break;
                case HouseBinding::Error::NotFound:error=Error::NotFound;break;
                case HouseBinding::Error::UnsupportedItem:error=Error::UnsupportedItem;break;
                default:break;
                }
                return {{},error};
            }
            catch(...){return {{},Error::DatabaseFailure};}
        }
        return Run<Receipt>(*connection,execute);
    }
    Result<Receipt> Service::ReturnUnderTransaction(Database& db,bool enabled,const Request& request,
      InventorySessionFence& fence,HouseBinding::CustodyContext* houses,CrateBinding::CustodyContext* crates,
      const Scene::HouseScenePolicy* scene,const InteriorContent::ProfilePolicy* profile){
        if(!enabled)return {{},Error::Disabled};
        if(request.action!=Action::Return)return {{},Error::InvalidArgument};
        Service service(nullptr,true,houses,crates);
        return service.OperateCurrent(request,fence,scene,profile,&db);
    }
    Result<Receipt> Service::ReturnCrateForRetirementUnderTransaction(Database& db,bool enabled,const Request& request,
      InventorySessionFence& fence,const OccupiedRetirement::Authority& retirement,HouseBinding::CustodyContext* houses,CrateBinding::CustodyContext* crates,
      const Scene::HouseScenePolicy* scene,const InteriorContent::ProfilePolicy* profile){
        if(!enabled)return {{},Error::Disabled};
        if(!retirement.AllowsReturn(db,request,fence))return {{},Error::Unauthorized};
        Service service(nullptr,true,houses,crates);
        return service.OperateCurrent(request,fence,scene,profile,&db,&retirement);
    }
    Result<Receipt> Service::PlaceRetainedLayoutUnderTransaction(Database& db,bool enabled,const LayoutUnitDestination& target,
      const InventoryRow& row,int32_t serial,uint64_t house_id,uint64_t crate_id,InventorySessionFence& fence,
      HouseBinding::CustodyContext* houses,CrateBinding::CustodyContext* crates,const Scene::HouseScenePolicy* scene,const InteriorContent::ProfilePolicy* profile){
      if(!enabled)return {{},Error::Disabled};
      if(bool(house_id)==bool(crate_id)||serial<=0||target.parent_yard)return {{},Error::InvalidArgument};
      try{
       auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")throw Failure{Error::InvalidArgument};
       Service service(nullptr,true,houses,crates);Request r;r.request_key=target.request_key;r.lease=target.lease;r.estate_id=target.estate_id;r.expected=row;r.verified_runtime_serial=serial;r.action=Action::Store;
       if(house_id){if(!houses||target.room)throw Failure{Error::UnsupportedItem};HouseBinding::TransactionHelpers helper(db,fence,houses->Authorization());auto before=helper.ReadInventory(r.lease,row.values.slot_id);if(before.house_id!=house_id||!before.interior)throw Failure{Error::Conflict};r.house=HouseIdentity{house_id,before.revision};}
       else{if(!crates)throw Failure{Error::UnsupportedItem};auto before=CrateBinding::ReadCarriedUnderTransaction(db,row);if(!before||before.value.binding.crate_id!=crate_id||before.value.content_manifest_sha256!=crates->ManifestDigest())throw Failure{Error::Conflict};r.crate=CrateIdentity{crate_id,before.value.binding.revision};}
       if(target.room){if(!scene||!profile)throw Failure{Error::InvalidArgument};const auto& room=*target.room;InteriorPreviewRequest preview{target.request_key,target.lease,room.source,room.house_id,room.house_revision,room.profile_revision,row,serial,target.estate_id,target.exterior_source};auto reserved=service.ReserveInteriorPlacementCurrent(preview,fence,*scene,*profile,nullptr,&db);if(!reserved)throw Failure{reserved.error};r.room=reserved.value;r.room_exterior_source=target.exterior_source;r.placement=Placed{reserved.value.placement,target.transform,1};}
       else{PreviewRequest preview{target.request_key,target.lease,target.estate_id,target.native_estate_id,target.catalog_key,target.catalog_revision,row,serial};auto reserved=service.ReservePlacementCurrent(preview,fence,nullptr,scene,&db);if(!reserved)throw Failure{reserved.error};r.placement=Placed{reserved.value,target.transform,1};}
       auto stored=service.OperateCurrent(r,fence,scene,profile,&db);if(!stored)return stored;
       auto pending=r.lease;pending.pending_key=r.request_key;auto valid=ValidateRestartOutcome(db,pending);if(!valid)throw Failure{valid.error};
       if(bool(stored.value.house_effect)!=bool(house_id)||bool(stored.value.crate_before)!=bool(crate_id)||bool(stored.value.crate_allocation)!=bool(crate_id)||(stored.value.house_effect&&(stored.value.house_effect->minted||!stored.value.house_effect->before.interior||!HouseBinding::SamePhysicalInterior(*stored.value.house_effect->before.interior,*stored.value.house_effect->after.interior))))throw Failure{Error::Conflict};
       // All writes remain uncommitted. The enclosing typed mixed outcome must
       // publish its own pending marker before COMMIT; failure rolls back all.
       if(Q(db,"UPDATE housing_live_item_leases SET pending_key=NULL WHERE character_id="+std::to_string(r.lease.actor.character_id)+" AND pending_key="+Quote(db,r.request_key)).RowsAffected()!=1)throw Failure{Error::Conflict};
       if(stored.value.house_effect&&Q(db,"UPDATE housing_house_instances SET pending_request_key=NULL WHERE house_id="+std::to_string(house_id)+" AND revision="+std::to_string(stored.value.house_effect->after.revision)+" AND pending_request_key="+Quote(db,r.request_key)).RowsAffected()!=1)throw Failure{Error::Conflict};
       return stored;
      }catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
    }
    Result<LayoutUnitPlacement> Service::PlaceLayoutUnitUnderTransaction(Database& db,bool enabled,
        const LayoutUnitDestination& r,const LayoutInventory::Plan& plan,const LayoutInventory::Prepared& prepared,size_t unit_index,
        EQ::InventoryProfile& inv,InventorySessionFence& fence,const Scene::HouseScenePolicy* house_policy,const InteriorContent::ProfilePolicy* profile_policy,const CrateBinding::CustodyContext* crates){
        if(!enabled)return {{},Error::Disabled};
        if(!Key(r.request_key)||!r.estate_id||!r.native_estate_id||r.native_estate_id==UINT32_MAX||!Key(r.catalog_key)||!r.catalog_revision||unit_index>=prepared.units.size()||(!r.room&&r.exterior_source)||(r.parent_yard&&(r.room||r.exterior_source||!OccupiedParentYard::Shape(*r.parent_yard))))return {{},Error::InvalidArgument};
        auto sources=LayoutInventory::ReviewSourcesUnderTransaction(db,true,r.lease,plan,prepared,inv,fence);if(!sources)return {{},sources.error};
        try{
            const auto& unit=prepared.units[unit_index];const auto& row=unit.row;
            if(!unit.item||unit.source_index>=plan.sources.size()||!LayoutInventory::OrdinarySource(plan.sources[unit.source_index])||std::any_of(std::begin(HouseGeometry::Records),std::end(HouseGeometry::Records),[&](const auto& h){return h.item_id==row.values.item_id;})||CrateGeometry::TemplateDefinition(row.values.item_id))throw Failure{Error::UnsupportedItem};
            PlacementTables(db);
            auto previous=Q(db,"SELECT native_item_id FROM housing_live_placement_reservations WHERE request_key="+Quote(db,r.request_key)+" OR (character_id="+std::to_string(r.lease.actor.character_id)+" AND lease_epoch="+std::to_string(r.lease.epoch)+" AND session_key="+Quote(db,r.lease.session_key)+" AND runtime_serial="+std::to_string(unit.item->GetSerialNumber())+") FOR UPDATE");if(previous.RowCount())throw Failure{Error::Conflict};
            LayoutUnitPlacement out;Reservation reservation{0,r.native_estate_id,r.estate_id,r.catalog_key,r.catalog_revision};
            if(r.room){
                if(!house_policy||!profile_policy)throw Failure{Error::InvalidArgument};const auto& target=*r.room;
                auto profile=InteriorContent::ReadProfileUnderTransaction(db,r.lease,target.source,fence,*house_policy,*profile_policy,r.exterior_source?std::optional<InteriorContent::AttachedSelection>{{r.estate_id,target.house_id,*r.exterior_source,crates}}:std::nullopt);
                if(!profile)throw Failure{profile.error};
                auto reviewed=InteriorContent::Detail::ReviewDestinationItem(db,std::move(profile.value),row,r.transform,true,fence,r.lease);if(!reviewed)throw Failure{reviewed.error};
                const auto& view=reviewed.value.profile;const auto& house=view.context.house;
                if(!house||!house->interior||house->house_id!=target.house_id||house->revision!=target.house_revision||view.profile_revision!=target.profile_revision||view.context.context.estate_id!=r.estate_id||house->interior->native_estate_id!=r.native_estate_id||house->interior->catalog_key!=r.catalog_key||house->interior->catalog_revision!=r.catalog_revision)throw Failure{Error::Conflict};
                out.room=InteriorReservation{reservation,target.house_id,target.house_revision,target.profile_revision,target.source,reviewed.value.identity.digest};
            }else{
                ContentBinding::Identity identity;const bool bind=ContentBinding::ColumnsInstalled(db);
                std::optional<Scene::Snapshot> parent;
                if(r.parent_yard){auto read=OccupiedParentYard::ReadUnderTransaction(db,r.lease,*r.parent_yard,fence,house_policy,profile_policy,crates);if(!read)throw Failure{read.error};parent=std::move(read.value);}
                PlacementCatalog(db,reservation,row,r.transform,true,fence,r.lease.actor,PlacementPolicy::Operation::Add,bind?&identity:nullptr,0,nullptr,&r.lease,HouseBinding::PlacementStage::Store,unit.item->GetSerialNumber(),nullptr,parent?&*parent:nullptr);
                if(bind)reservation.content_binding=identity;
            }
            auto catalog=Simple(db,row);
            auto allocated=Q(db,std::string("INSERT INTO housing_live_placement_reservations(request_key,character_id,account_id,lease_epoch,session_key,estate_id,native_estate_id,catalog_key,catalog_revision,source_row,runtime_serial")+(reservation.content_binding?",content_binding_version,content_binding_digest":"")+") VALUES("+Quote(db,r.request_key)+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+","+std::to_string(r.lease.epoch)+","+Quote(db,r.lease.session_key)+","+std::to_string(r.estate_id)+","+std::to_string(r.native_estate_id)+","+Quote(db,r.catalog_key)+","+std::to_string(r.catalog_revision)+",x'"+Hex(Encode(row))+"',"+std::to_string(unit.item->GetSerialNumber())+(reservation.content_binding?","+std::to_string(reservation.content_binding->version)+",x'"+Hex(Bytes(reservation.content_binding->digest.begin(),reservation.content_binding->digest.end()))+"'":"")+")");
            if(!allocated.LastInsertedID()||allocated.LastInsertedID()>=UINT32_MAX)throw Failure{Error::Conflict};reservation.native_item_id=allocated.LastInsertedID();
            if(out.room){out.room->placement=reservation;const auto& room=*out.room;
                auto tagged=Q(db,"INSERT INTO housing_interior_placement_reservations(native_item_id,house_id,house_revision,profile_revision,source_zone,source_version,source_instance,content_version,content_digest) VALUES("+std::to_string(reservation.native_item_id)+","+std::to_string(room.house_id)+","+std::to_string(room.house_revision)+","+std::to_string(room.profile_revision)+","+std::to_string(room.source.zone_id)+","+std::to_string(room.source.zone_version)+","+std::to_string(room.source.instance_id)+","+std::to_string(InteriorContent::Version)+",x'"+Hex(Bytes(room.content_digest.begin(),room.content_digest.end()))+"')");if(tagged.RowsAffected()!=1)throw Failure{Error::Conflict};
            }
            auto stored=Q(db,"INSERT INTO housing_live_item_custody(estate_id,owner_character_id,schema_descriptor,row_blob,catalog_blob) VALUES("+std::to_string(r.estate_id)+","+std::to_string(r.lease.actor.character_id)+","+Quote(db,SchemaDescriptor())+",x'"+Hex(Encode(row))+"',x'"+Hex(Cat(catalog))+"')");if(stored.RowsAffected()!=1||!stored.LastInsertedID())throw Failure{Error::DatabaseFailure};
            out.stored.custody_id=stored.LastInsertedID();out.stored.estate_id=r.estate_id;out.stored.row=row;out.stored.catalog=catalog;out.stored.placement=Placed{reservation,r.transform,1};out.stored.current_catalog_revision=r.catalog_revision;
            if(out.room){auto scoped=Q(db,"INSERT INTO housing_house_item_scopes(custody_id,house_id,revision) VALUES("+std::to_string(out.stored.custody_id)+","+std::to_string(out.room->house_id)+",1)");if(scoped.RowsAffected()!=1)throw Failure{Error::Conflict};}
            auto posed=Q(db,"INSERT INTO housing_live_placed_items(custody_id,native_item_id,pose_revision,pose_blob) VALUES("+std::to_string(out.stored.custody_id)+","+std::to_string(reservation.native_item_id)+",1,x'"+Hex(Pose(r.transform))+"')");if(posed.RowsAffected()!=1)throw Failure{Error::Conflict};
            auto reserved=Q(db,"UPDATE housing_live_placement_reservations SET state=1,custody_id="+std::to_string(out.stored.custody_id)+" WHERE native_item_id="+std::to_string(reservation.native_item_id)+" AND state=0 AND custody_id IS NULL");if(reserved.RowsAffected()!=1)throw Failure{Error::Conflict};return {std::move(out),Error::None};
        }catch(const Failure& failure){return {{},failure.error};}catch(...){return {{},Error::DatabaseFailure};}
    }
    Result<Receipt> Service::Resolve(const Lease& input,const std::string& key,InventorySessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!LeaseShape(input)||!Key(key))return {
            {
            },Error::InvalidArgument
        };
        if(!fence.HeldFor(input.actor,input.epoch,input.session_key))return {
            {
            },Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Receipt>(*database_,[&]{
            auto& db=*database_;auto lease=Locked(db,input);auto rows=Q(db,"SELECT "+receipt_columns+" FROM housing_live_item_receipts WHERE request_key="+Quote(db,key)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{
                Error::NotFound
            };auto receipt=ReceiptRow(rows.begin(),lease,key);AttachHouse(db,lease,receipt,fence,houses_);AttachRoom(db,receipt);ValidatePendingCrate(db,receipt);if(receipt.requires_application)PartitionPostimage(db,receipt);return receipt;
        });
    }
    Result<bool> Service::AcknowledgeApplied(const Lease& input,const std::string& key,InventorySessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!LeaseShape(input)||!Key(key))return {
            {
            },Error::InvalidArgument
        };
        if(!fence.HeldFor(input.actor,input.epoch,input.session_key))return {
            {
            },Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<bool>(*database_,[&]{
            auto& db=*database_;auto lease=Locked(db,input);auto rows=Q(db,"SELECT "+receipt_columns+" FROM housing_live_item_receipts WHERE request_key="+Quote(db,key)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{
                Error::NotFound
            };auto row=rows.begin();auto receipt=ReceiptRow(row,lease,key);AttachHouse(db,lease,receipt,fence,houses_);AttachRoom(db,receipt);ValidatePendingCrate(db,receipt);if(receipt.requires_application)PartitionPostimage(db,receipt);if(!receipt.requires_application){
                if(!lease.pending_key.empty())throw Failure{
                    Error::PendingApplication
                };
                if(receipt.house_effect){HouseBinding::TransactionHelpers helper(db,fence,houses_->Authorization());helper.ClearAfterParentAcknowledgement(lease,receipt,houses_->Application());}
                return true;
            }
            if(lease.pending_key!=key)throw Failure{
                Error::PendingApplication
            };if(!fence.Applied(receipt))throw Failure{
                Error::UnheldFence
            };auto ack=Q(db,"UPDATE housing_live_item_receipts SET applied=1 WHERE request_key="+Quote(db,key)+" AND applied=0");auto cleared=Q(db,"UPDATE housing_live_item_leases SET pending_key=NULL WHERE character_id="+std::to_string(input.actor.character_id)+" AND pending_key="+Quote(db,key));if(ack.RowsAffected()!=1||cleared.RowsAffected()!=1)throw Failure{
                Error::DatabaseFailure
            };
            if(receipt.house_effect){HouseBinding::TransactionHelpers helper(db,fence,houses_->Authorization());helper.ClearAfterParentAcknowledgement(lease,receipt,houses_->Application());}
            return true;
        });
    }
    Result<std::shared_ptr<const HouseBinding::Binding>> Service::ReadInventoryHouse(const Lease& input,const InventoryRow& expected,int32_t serial,InventorySessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {{},Error::Disabled};
        if(!houses_)return {{},Error::UnsupportedItem};
        if(!LeaseShape(input)||!Valid(expected)||expected.values.character_id!=input.actor.character_id||!General(expected.values.slot_id)||serial<=0)return {{},Error::InvalidArgument};
        if(!fence.HeldFor(input.actor,input.epoch,input.session_key)||!fence.SourceMatches(expected,serial))return {{},Error::UnheldFence};
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
        return Run<std::shared_ptr<const HouseBinding::Binding>>(*database_,[&]{
            auto& db=*database_;auto lease=Locked(db,input);
            if(!lease.pending_key.empty())throw Failure{Error::PendingApplication};
            auto rows=Q(db,"SELECT "+select_inventory+" FROM inventory WHERE "+Slot(input.actor.character_id,expected.values.slot_id)+" FOR UPDATE");
            if(rows.RowCount()!=1||Encode(ItemPayload::Complete(db,InvRow(rows.begin()),&expected))!=Encode(expected))throw Failure{Error::Conflict};
            NoChildren(db,expected.values.character_id,expected.values.slot_id);Simple(db,expected);
            if(!houses_->Templates().VerifiedHouse(expected))throw Failure{Error::UnsupportedItem};
            HouseBinding::TransactionHelpers helper(db,fence,houses_->Authorization());
            auto binding=helper.ReadInventory(lease,expected.values.slot_id);
            if(binding.item_template_id!=expected.values.item_id)throw Failure{Error::Conflict};
            return std::shared_ptr<const HouseBinding::Binding>(std::make_shared<HouseBinding::Binding>(std::move(binding)));
        });
    }
    Result<std::shared_ptr<const CrateBinding::Allocation>> Service::ReadInventoryCrate(const Lease& input,const InventoryRow& expected,int32_t serial,InventorySessionFence& fence){
        std::lock_guard<std::mutex> lock(mutex_);if(!enabled_)return {{},Error::Disabled};if(!crates_)return {{},Error::UnsupportedItem};
        if(!LeaseShape(input)||!Valid(expected)||expected.values.character_id!=input.actor.character_id||!General(expected.values.slot_id)||serial<=0)return {{},Error::InvalidArgument};
        if(!fence.HeldFor(input.actor,input.epoch,input.session_key)||!fence.SourceMatches(expected,serial))return {{},Error::UnheldFence};
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
        return Run<std::shared_ptr<const CrateBinding::Allocation>>(*database_,[&]{auto& db=*database_;auto lease=Locked(db,input);if(!lease.pending_key.empty())throw Failure{Error::PendingApplication};Simple(db,expected);NoChildren(db,expected.values.character_id,expected.values.slot_id);
            auto current=CrateBinding::ReadCarriedUnderTransaction(db,expected);if(!current)throw Failure{current.error};if(current.value.content_manifest_sha256!=crates_->ManifestDigest())throw Failure{Error::Unauthorized};
            return std::shared_ptr<const CrateBinding::Allocation>(std::make_shared<CrateBinding::Allocation>(std::move(current.value)));
        });
    }
    Result<Stored> Service::InspectCustodyUnderTransaction(Database& db,uint64_t id){
        return InspectCustodyUnderTransaction(db,id,false);
    }
    Result<Stored> Service::InspectCustodyUnderTransaction(Database& db,uint64_t id,bool allow_contained){
        try{
            auto tx=Q(db,"SELECT @@in_transaction");if(!id||tx.RowCount()!=1||std::string(tx.begin()[0])!="1")return {{},Error::InvalidArgument};
            auto rows=Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id="+std::to_string(id)+" FOR UPDATE");if(rows.RowCount()!=1)return {{},Error::NotFound};
            auto item=StoredRow(db,rows.begin(),id);PlacementTables(db);item.placement=CurrentPose(db,id,allow_contained);
            auto state=ItemStorage::Locked(db,id);item.stowed=state.stowed;item.storage_revision=state.revision;
            return {std::move(item),Error::None};
        }catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
    }
    Result<Stored> Service::ReadStored(const Lease& input,uint64_t id,InventorySessionFence& fence){
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!LeaseShape(input)||!id)return {
            {
            },Error::InvalidArgument
        };
        if(!fence.HeldFor(input.actor,input.epoch,input.session_key))return {
            {
            },Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Stored>(*database_,[&]{
            auto& db=*database_;Locked(db,input);auto rows=Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id="+std::to_string(id)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{
                Error::NotFound
            };auto item=StoredRow(db,rows.begin(),id);if(HouseItemScope::Locked(db,id))throw Failure{Error::Unauthorized};const bool foreign=item.row.values.character_id!=input.actor.character_id;
            auto tables=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_placed_items'");
            if(std::stoul(tables.begin()[0])){PlacementTables(db);item.placement=CurrentPose(db,id);}
            if(foreign){ForeignCustody(db,item);auto catalog=Simple(db,item.row);if(Cat(catalog)!=Cat(item.catalog))throw Failure{Error::UnsupportedItem};}
            if(item.placement){CurrentAccess(db,item.estate_id,item.placement->reservation.native_estate_id,input.actor,foreign?std::optional<AccessPolicy::Capability>(AccessPolicy::Capability::RemoveItem):std::nullopt);CurrentRemovalContext(db,*item.placement,fence);}
            else Estate(db,item.estate_id,input.actor);
            CurrentCrate(db,item);
            return item;
        });
    }    Result<Reservation> Service::ReservePlacement(const PreviewRequest& r,InventorySessionFence& fence){return ReservePlacementCurrent(r,fence,nullptr,nullptr);}
    Result<Reservation> Service::ReserveCratePlacement(const PreviewRequest& r,const CrateMemberContext& target,InventorySessionFence& fence,const Scene::HouseScenePolicy* house){return ReservePlacementCurrent(r,fence,&target,house);}
    Result<Reservation> Service::ReservePlacementCurrent(const PreviewRequest& r,InventorySessionFence& fence,const CrateMemberContext* destination,const Scene::HouseScenePolicy* house,Database* parent_transaction){
        std::lock_guard<std::mutex> lock(mutex_);if(!enabled_)return {{},Error::Disabled};if(!LeaseShape(r.lease)||!Key(r.request_key)||!r.estate_id||!Valid(r.expected)||r.expected.values.character_id!=r.lease.actor.character_id||!General(r.expected.values.slot_id)||r.verified_runtime_serial<=0)return {{},Error::InvalidArgument};if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};auto* connection=parent_transaction?parent_transaction:database_.get();if(!connection||connection->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
        auto execute=[&]()->Reservation{auto& db=*connection;auto lease=Locked(db,r.lease);PlacementTables(db);if(!lease.pending_key.empty())throw Failure{Error::PendingApplication};Reservation p{0,r.native_estate_id,r.estate_id,r.catalog_key,r.catalog_revision};
            if(r.house_storage_only&&(destination||!HouseStorageInstalled(db)))throw Failure{Error::Disabled};
            if(destination){
                MemberContextBytes(*destination);if(!NpcItemMetadata::OrdinaryUnderTransaction(db,r.expected.values.item_id))throw Failure{Error::UnsupportedItem};if(!crates_||destination->revision>=UINT64_MAX-1||CrateGeometry::TemplateDefinition(r.expected.values.item_id))throw Failure{Error::UnsupportedItem};
                auto read=Scene::Service::ReadSceneUnderTransaction(db,lease,r.estate_id,fence,house,crates_);if(!read)throw Failure{read.error};const auto& yard=read.value;const auto& target=*destination;
                if(target.native_plot_id!=r.native_estate_id||target.catalog_revision!=r.catalog_revision||yard.native_plot_id!=target.native_plot_id||yard.current_catalog_revision!=target.catalog_revision||yard.route.zone_id!=target.source.zone_id||yard.route.zone_version!=target.source.zone_version||yard.route.instance_id!=target.source.instance_id||!yard.access.Allows(AccessPolicy::Capability::AddItem))throw Failure{Error::Unauthorized};
                auto container=std::find_if(yard.items.begin(),yard.items.end(),[&](const auto& item){return item.crate&&item.crate->binding.crate_id==target.crate_id;});
                if(container==yard.items.end()||container->stored.stowed||container->crate->packed||container->crate->binding.revision!=target.revision||container->crate->binding.catalog_key!=r.catalog_key||container->crate->created_by_character_id!=lease.actor.character_id)throw Failure{Error::Conflict};
                bool house_item=false;for(const auto& model:HouseGeometry::Records)house_item|=model.item_id==r.expected.values.item_id;
                if(house_item&&(!houses_||!yard.access.Allows(AccessPolicy::Capability::AddHouse)))throw Failure{Error::Unauthorized};
                auto catalog=Q(db,"SELECT HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,r.catalog_key)+" AND revision="+std::to_string(target.catalog_revision)+" FOR UPDATE");if(catalog.RowCount()!=1)throw Failure{Error::Conflict};auto bundle=Housing::Catalog::DecodePayloads(Unhex(catalog.begin()[0]),Unhex(catalog.begin()[1]));auto definition=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==container->crate->binding.definition_id&&d.type==7;});
                const auto used=std::count_if(yard.crate_contents.begin(),yard.crate_contents.end(),[&](const auto& item){return item.contained_in_crate==target.crate_id;});if(definition==bundle.definitions.end()||!definition->max_items||uint64_t(used)>=definition->max_items)throw Failure{Error::Conflict};
            }
            ContentBinding::Identity fresh;bool bound=ContentBinding::ColumnsInstalled(db);PlacementCatalog(db,p,r.expected,{},!destination,fence,r.lease.actor,PlacementPolicy::Operation::Add,bound?&fresh:nullptr,0,houses_,&lease,HouseBinding::PlacementStage::Preview,r.verified_runtime_serial,crates_,nullptr,{},r.house_storage_only);if(bound)p.content_binding=fresh;auto old=Q(db,"SELECT "+ReservationColumns(db)+" FROM housing_live_placement_reservations WHERE request_key="+Quote(db,r.request_key)+" FOR UPDATE");if(old.RowCount()){auto row=old.begin();HeldReservation(row,lease);auto prior=ReservationRow(row);if(HouseStorageReservation(db,prior.native_item_id)!=r.house_storage_only)throw Failure{Error::Conflict};if(prior.native_estate_id!=p.native_estate_id||prior.estate_id!=p.estate_id||prior.catalog_key!=p.catalog_key||prior.catalog_revision!=p.catalog_revision||Unhex(row[9])!=Encode(r.expected)||std::stol(row[10])!=r.verified_runtime_serial||std::stoul(row[11])!=0||(prior.content_binding&&(!p.content_binding||!ContentBinding::Same(*prior.content_binding,*p.content_binding))))throw Failure{Error::Conflict};return prior;}
            if(!fence.SourceMatches(r.expected,r.verified_runtime_serial))throw Failure{Error::UnheldFence};auto inventory=Q(db,"SELECT "+select_inventory+" FROM inventory WHERE "+Slot(lease.actor.character_id,r.expected.values.slot_id)+" FOR UPDATE");if(inventory.RowCount()!=1||Encode(ItemPayload::Complete(db,InvRow(inventory.begin()),&r.expected))!=Encode(r.expected))throw Failure{Error::Conflict};NoChildren(db,r.expected.values.character_id,r.expected.values.slot_id);Simple(db,r.expected);
            auto inserted=Q(db,std::string("INSERT INTO housing_live_placement_reservations(request_key,character_id,account_id,lease_epoch,session_key,estate_id,native_estate_id,catalog_key,catalog_revision,source_row,runtime_serial")+(p.content_binding?",content_binding_version,content_binding_digest":"")+") VALUES ("+Quote(db,r.request_key)+","+std::to_string(lease.actor.character_id)+","+std::to_string(lease.actor.account_id)+","+std::to_string(lease.epoch)+","+Quote(db,lease.session_key)+","+std::to_string(p.estate_id)+","+std::to_string(p.native_estate_id)+","+Quote(db,p.catalog_key)+","+std::to_string(p.catalog_revision)+",x'"+Hex(Encode(r.expected))+"',"+std::to_string(r.verified_runtime_serial)+(p.content_binding?","+std::to_string(p.content_binding->version)+",x'"+Hex(Bytes(p.content_binding->digest.begin(),p.content_binding->digest.end()))+"'":"")+")");if(!inserted.LastInsertedID()||inserted.LastInsertedID()>=0xffffffff)throw Failure{Error::Conflict};p.native_item_id=inserted.LastInsertedID();if(r.house_storage_only){auto mark=Q(db,"INSERT INTO housing_house_storage_reservations(native_item_id) VALUES("+std::to_string(p.native_item_id)+")");if(mark.RowsAffected()!=1)throw Failure{Error::Conflict};}return p;};
        if(parent_transaction){try{auto tx=Q(*connection,"SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return {{},Error::InvalidArgument};return {execute(),Error::None};}catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}}
        return Run<Reservation>(*connection,execute);
    }
    Result<InteriorReservation> Service::ReserveInteriorPlacement(const InteriorPreviewRequest& r,InventorySessionFence& fence,const Scene::HouseScenePolicy& h,const InteriorContent::ProfilePolicy& p,const CrateMemberContext* d){return ReserveInteriorPlacementCurrent(r,fence,h,p,d);}
    Result<InteriorReservation> Service::ReserveInteriorPlacementCurrent(const InteriorPreviewRequest& r,InventorySessionFence& fence,const Scene::HouseScenePolicy& house_policy,const InteriorContent::ProfilePolicy& profile_policy,const CrateMemberContext* destination,Database* parent_transaction){
        std::lock_guard<std::mutex> lock(mutex_);if(!enabled_)return {{},Error::Disabled};
        if(!LeaseShape(r.lease)||!Key(r.request_key)||!r.expected_house_id||!r.expected_house_revision||!r.expected_profile_revision||!Valid(r.expected)||r.expected.values.character_id!=r.lease.actor.character_id||!General(r.expected.values.slot_id)||r.verified_runtime_serial<=0)return {{},Error::InvalidArgument};
        if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};auto* connection=parent_transaction?parent_transaction:database_.get();if(!connection||connection->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
        auto execute=[&]()->InteriorReservation{auto& db=*connection;auto lease=Locked(db,r.lease);if(!lease.pending_key.empty())throw Failure{Error::PendingApplication};PlacementTables(db);
            auto schema=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_interior_placement_reservations' AND engine='InnoDB'");if(schema.RowCount()!=1||std::stoul(schema.begin()[0])!=1)throw Failure{Error::DatabaseFailure};
            std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>> prior_digest;auto prior_tag=Q(db,"SELECT HEX(t.content_digest) FROM housing_live_placement_reservations r JOIN housing_interior_placement_reservations t ON t.native_item_id=r.native_item_id WHERE r.request_key="+Quote(db,r.request_key)+" AND t.content_version=3 FOR UPDATE");if(prior_tag.RowCount()>1)throw Failure{Error::Conflict};if(prior_tag.RowCount()){if(!prior_tag.begin()[0])throw Failure{Error::Conflict};auto bytes=Unhex(prior_tag.begin()[0]);if(bytes.size()!=SHA256_DIGEST_LENGTH)throw Failure{Error::Conflict};prior_digest.emplace();std::copy(bytes.begin(),bytes.end(),prior_digest->begin());}
            auto checked=InteriorContent::ReviewInventoryAddUnderTransaction(db,lease,r.current_source,fence,house_policy,profile_policy,r.expected,r.verified_runtime_serial,{},r.exterior_source?std::optional<InteriorContent::AttachedSelection>{{r.exterior_estate_id,r.expected_house_id,*r.exterior_source,crates_}}:std::nullopt,!destination,{},prior_digest);if(!checked)throw Failure{checked.error};auto& context=checked.value.profile;
            if(!context.context.house||!context.context.house->interior||context.context.house->house_id!=r.expected_house_id||context.context.house->revision!=r.expected_house_revision||context.profile_revision!=r.expected_profile_revision)throw Failure{Error::Conflict};
            if(destination){const auto& m=*destination;MemberContextBytes(m);if(!NpcItemMetadata::OrdinaryUnderTransaction(db,r.expected.values.item_id))throw Failure{Error::UnsupportedItem};
                const auto physical=r.exterior_source.value_or(r.current_source);
                if(!crates_||CrateGeometry::TemplateDefinition(r.expected.values.item_id)||m.source.zone_id!=physical.zone_id||m.source.zone_version!=physical.zone_version||m.source.instance_id!=physical.instance_id)throw Failure{Error::InvalidArgument};
                auto read=r.exterior_source?Scene::Service::ReadAttachedInteriorUnderTransaction(db,lease,r.exterior_estate_id,r.expected_house_id,fence,house_policy,&profile_policy,true,crates_):Scene::Service::ReadInteriorSceneUnderTransaction(db,lease,r.current_source,fence,house_policy,profile_policy,crates_);if(!read)throw Failure{read.error};const auto& view=read.value;
                if(view.context.source.zone_id!=r.current_source.zone_id||view.context.source.zone_version!=r.current_source.zone_version||view.context.source.instance_id!=r.current_source.instance_id||(r.exterior_source&&(!view.exterior_source||view.exterior_source->zone_id!=physical.zone_id||view.exterior_source->zone_version!=physical.zone_version||view.exterior_source->instance_id!=physical.instance_id)))throw Failure{Error::Conflict};
                if(!view.house||view.house->house_id!=r.expected_house_id||view.house->revision!=r.expected_house_revision||view.profile_revision!=r.expected_profile_revision||view.house->interior->native_estate_id!=m.native_plot_id||view.context.catalog_revision!=m.catalog_revision)throw Failure{Error::Conflict};
                auto container=std::find_if(view.contents.begin(),view.contents.end(),[&](const auto& i){return i.crate&&i.crate->binding.crate_id==m.crate_id;});if(container==view.contents.end()||container->stored.stowed||container->crate->packed||container->crate->binding.revision!=m.revision||container->crate->created_by_character_id!=lease.actor.character_id)throw Failure{Error::Conflict};
                auto definition=CrateBinding::ReadDefinitionUnderTransaction(db,*container->crate);if(!definition)throw Failure{definition.error};size_t count=0;for(const auto& i:view.crate_contents)if(i.contained_in_crate==m.crate_id)++count;if(!definition.value.max_items||count>=definition.value.max_items)throw Failure{Error::Conflict};
            }
            NoChildren(db,r.expected.values.character_id,r.expected.values.slot_id);Simple(db,r.expected);
            const auto& house=*context.context.house;const auto& room=*house.interior;
            InteriorReservation out;out.placement={0,room.native_estate_id,context.context.context.estate_id,room.catalog_key,room.catalog_revision};out.house_id=house.house_id;out.house_revision=house.revision;out.profile_revision=context.profile_revision;out.source=r.current_source;out.content_digest=checked.value.identity.digest;
            auto prior=Q(db,"SELECT "+ReservationColumns(db)+" FROM housing_live_placement_reservations WHERE request_key="+Quote(db,r.request_key)+" FOR UPDATE");
            if(prior.RowCount()){
                if(prior.RowCount()!=1)throw Failure{Error::Conflict};auto row=prior.begin();HeldReservation(row,lease);auto base=ReservationRow(row);
                if(base.content_binding||base.native_estate_id!=out.placement.native_estate_id||base.estate_id!=out.placement.estate_id||base.catalog_key!=out.placement.catalog_key||base.catalog_revision!=out.placement.catalog_revision||Unhex(row[9])!=Encode(r.expected)||std::stol(row[10])!=r.verified_runtime_serial||std::stoul(row[11])!=0||row[12])throw Failure{Error::Conflict};
                auto tagged=Q(db,"SELECT house_id,house_revision,profile_revision,source_zone,source_version,source_instance,content_version,HEX(content_digest) FROM housing_interior_placement_reservations WHERE native_item_id="+std::to_string(base.native_item_id)+" FOR UPDATE");
                if(tagged.RowCount()!=1)throw Failure{Error::Conflict};auto tag=tagged.begin();for(size_t i=0;i<8;++i)if(!tag[i])throw Failure{Error::Conflict};
                if(std::stoull(tag[0])!=out.house_id||std::stoull(tag[1])!=out.house_revision||std::stoull(tag[2])!=out.profile_revision||std::stoul(tag[3])!=out.source.zone_id||std::stoul(tag[4])!=out.source.zone_version||std::stoul(tag[5])!=out.source.instance_id||std::stoul(tag[6])!=InteriorContent::Version||Unhex(tag[7])!=Bytes(out.content_digest.begin(),out.content_digest.end()))throw Failure{Error::Conflict};
                out.placement=std::move(base);return out;
            }
            auto& p=out.placement;
            auto inserted=Q(db,"INSERT INTO housing_live_placement_reservations(request_key,character_id,account_id,lease_epoch,session_key,estate_id,native_estate_id,catalog_key,catalog_revision,source_row,runtime_serial) VALUES("+Quote(db,r.request_key)+","+std::to_string(lease.actor.character_id)+","+std::to_string(lease.actor.account_id)+","+std::to_string(lease.epoch)+","+Quote(db,lease.session_key)+","+std::to_string(p.estate_id)+","+std::to_string(p.native_estate_id)+","+Quote(db,p.catalog_key)+","+std::to_string(p.catalog_revision)+",x'"+Hex(Encode(r.expected))+"',"+std::to_string(r.verified_runtime_serial)+")");
            if(!inserted.LastInsertedID()||inserted.LastInsertedID()>=UINT32_MAX)throw Failure{Error::Conflict};p.native_item_id=inserted.LastInsertedID();
            auto tagged=Q(db,"INSERT INTO housing_interior_placement_reservations(native_item_id,house_id,house_revision,profile_revision,source_zone,source_version,source_instance,content_version,content_digest) VALUES("+std::to_string(p.native_item_id)+","+std::to_string(out.house_id)+","+std::to_string(out.house_revision)+","+std::to_string(out.profile_revision)+","+std::to_string(out.source.zone_id)+","+std::to_string(out.source.zone_version)+","+std::to_string(out.source.instance_id)+",3,x'"+Hex(Bytes(out.content_digest.begin(),out.content_digest.end()))+"')");if(tagged.RowsAffected()!=1)throw Failure{Error::Conflict};return out;
        };
        if(parent_transaction){try{auto tx=Q(*connection,"SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return {{},Error::InvalidArgument};return {execute(),Error::None};}catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}}
        return Run<InteriorReservation>(*connection,execute);
    }
    Result<bool> Service::CancelPlacement(const Lease& l,uint32_t id,InventorySessionFence& fence){std::lock_guard<std::mutex> lock(mutex_);if(!enabled_)return {{},Error::Disabled};if(!LeaseShape(l)||!id||id==0xffffffff)return {{},Error::InvalidArgument};if(!fence.HeldFor(l.actor,l.epoch,l.session_key))return {{},Error::UnheldFence};if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};return Run<bool>(*database_,[&]{auto& db=*database_;auto lease=Locked(db,l);PlacementTables(db);auto rows=Q(db,"SELECT "+ReservationColumns(db)+" FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(id)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::NotFound};auto row=rows.begin();HeldReservation(row,lease);if(std::stoul(row[11])==1)throw Failure{Error::Conflict};Q(db,"UPDATE housing_live_placement_reservations SET state=2 WHERE native_item_id="+std::to_string(id)+" AND state=0");return true;});}
    Result<std::vector<Stored>> Service::ReadPlaced(const Lease& l,EstateId estate,InventorySessionFence& fence){std::lock_guard<std::mutex> lock(mutex_);if(!enabled_)return {{},Error::Disabled};if(!LeaseShape(l)||!estate)return {{},Error::InvalidArgument};if(!fence.HeldFor(l.actor,l.epoch,l.session_key))return {{},Error::UnheldFence};if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};return Run<std::vector<Stored>>(*database_,[&]{auto& db=*database_;auto lease=Locked(db,l);if(!lease.pending_key.empty())throw Failure{Error::PendingApplication};Estate(db,estate,l.actor);PlacementTables(db);auto ids=Q(db,"SELECT c.custody_id FROM housing_live_item_custody c JOIN housing_live_placed_items p ON p.custody_id=c.custody_id WHERE c.estate_id="+std::to_string(estate)+HouseItemScope::ExteriorPredicate(db)+CrateMemberScope::ExteriorPredicate(db)+" ORDER BY c.custody_id LIMIT 1001 FOR UPDATE");if(ids.RowCount()>1000)throw Failure{Error::UnsupportedItem};std::vector<Stored> out;for(auto row=ids.begin();row.operator!=(ids.end());++row){auto current=Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id="+std::string(row[0])+" FOR UPDATE");auto item=StoredRow(db,current.begin(),std::stoull(row[0]));if(item.row.values.character_id!=l.actor.character_id)throw Failure{Error::Unauthorized};auto current_catalog=Simple(db,item.row);if(Cat(current_catalog)!=Cat(item.catalog))throw Failure{Error::UnsupportedItem};item.placement=CurrentPose(db,item.custody_id);if(!item.placement)throw Failure{Error::DatabaseFailure};item.current_catalog_revision=PlacementCatalog(db,item.placement->reservation,item.row,item.placement->transform,false,fence,l.actor,PlacementPolicy::Operation::Add,nullptr,0,houses_,&lease,HouseBinding::PlacementStage::Read,0,crates_,nullptr,item.placement->npc_frame);CurrentCrate(db,item);out.push_back(std::move(item));}return out;});}
    namespace {
        void MoveTable(Database& db){auto t=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_pose_moves' AND engine='InnoDB'");if(t.RowCount()!=1||std::stoul(t.begin()[0])!=1)throw Failure{Error::DatabaseFailure};}
        Bytes MoveBinding(const MoveRequest& r){Bytes b;W(b,1,4);S(b,r.request_key);W(b,r.lease.actor.character_id,4);W(b,r.lease.actor.account_id,4);W(b,r.lease.epoch,8);S(b,r.lease.session_key);b.insert(b.end(),r.lease.token.begin(),r.lease.token.end());W(b,r.estate_id,4);W(b,r.custody_id,8);W(b,r.native_item_id,4);W(b,r.expected_pose_revision,8);W(b,r.expected_catalog_revision,8);B(b,Pose(r.transform));if(r.room){W(b,3,4);B(b,RoomMoveBytes(*r.room));}if(r.room_exterior_source){if(!r.room||!r.room_exterior_source->zone_id||!r.room_exterior_source->instance_id||r.room_exterior_source->instance_id>32000)throw Failure{Error::InvalidArgument};W(b,5,4);W(b,r.room_exterior_source->zone_id,4);W(b,r.room_exterior_source->zone_version,1);W(b,r.room_exterior_source->instance_id,2);}if(r.parent_yard){if(r.room||r.room_exterior_source)throw Failure{Error::InvalidArgument};W(b,6,4);B(b,OccupiedParentYard::Encode(*r.parent_yard));}if(r.npc_frame){NpcNativeFrame::Shape(*r.npc_frame,r.transform);W(b,7,4);W(b,r.npc_frame->height_bits,4);W(b,r.npc_frame->origin_z_bits,4);}return b;}
        Bytes MoveOutcome(const MoveReceipt& r){Bytes b;W(b,r.parent_yard?6:r.room_exterior_source?5:r.room?4:(r.before.reservation.content_binding?2:1),4);S(b,r.request_key);W(b,r.custody_id,8);B(b,PlacementBytes(r.before));B(b,PlacementBytes(r.after));if(r.before.reservation.content_binding||r.room||r.parent_yard)W(b,r.committed_catalog_revision,8);if(r.room)B(b,RoomMoveBytes(*r.room));if(r.room_exterior_source){W(b,r.room_exterior_source->zone_id,4);W(b,r.room_exterior_source->zone_version,1);W(b,r.room_exterior_source->instance_id,2);}if(r.parent_yard)B(b,OccupiedParentYard::Encode(*r.parent_yard));return b;}
        MoveReceipt MoveRow(MySQLRequestRow row,const Lease& lease,const std::string& key){
            if(std::stoul(row[0])!=lease.actor.character_id||std::stoul(row[1])!=lease.actor.account_id)throw Failure{Error::Unauthorized};if(std::stoull(row[2])!=lease.epoch||std::string(row[3])!=lease.session_key)throw Failure{Error::StaleLease};
            auto bytes=Unhex(row[5]);Reader reader{bytes};auto version=reader.N(4);if(version!=1&&version!=2&&version!=4&&version!=5&&version!=6)throw Failure{Error::DatabaseFailure};
            MoveReceipt out;out.request_key=reader.Text(96);out.custody_id=reader.N(8);out.before=Unplacement(reader.Blob(512));out.after=Unplacement(reader.Blob(512));
            if(version!=1){out.committed_catalog_revision=reader.N(8);if(!out.committed_catalog_revision||(version==2&&!out.before.reservation.content_binding)||((version==4||version==5)&&out.before.reservation.content_binding))throw Failure{Error::DatabaseFailure};if(version==4||version==5)out.room=UnroomMove(reader.Blob(128));if(version==5)out.room_exterior_source=Route{uint32_t(reader.N(4)),uint8_t(reader.N(1)),uint16_t(reader.N(2))};if(version==6)out.parent_yard=OccupiedParentYard::Decode(reader.Blob(128));}else if(out.before.reservation.content_binding)throw Failure{Error::DatabaseFailure};
            if(reader.at!=bytes.size()||out.request_key!=key||!out.custody_id||out.after.pose_revision!=out.before.pose_revision+1||out.after.reservation.native_item_id!=out.before.reservation.native_item_id)throw Failure{Error::DatabaseFailure};auto expected_after=out.before;expected_after.pose_revision++;expected_after.transform=out.after.transform;expected_after.npc_frame=out.after.npc_frame;
            if(out.before.pose_revision==std::numeric_limits<uint64_t>::max()||PlacementBytes(expected_after)!=PlacementBytes(out.after))throw Failure{Error::DatabaseFailure};
            MoveRequest request;request.request_key=key;request.lease=lease;request.estate_id=out.before.reservation.estate_id;request.custody_id=out.custody_id;request.native_item_id=out.before.reservation.native_item_id;request.expected_pose_revision=out.before.pose_revision;request.expected_catalog_revision=version!=1?out.committed_catalog_revision:out.before.reservation.catalog_revision;request.transform=out.after.transform;request.room=out.room;request.room_exterior_source=out.room_exterior_source;request.parent_yard=out.parent_yard;request.npc_frame=out.after.npc_frame;
            if(Unhex(row[4])!=MoveBinding(request))throw Failure{Error::DatabaseFailure};out.replayed=true;return out;
        }
        const std::string move_columns="character_id,account_id,lease_epoch,session_key,HEX(binding_blob),HEX(outcome_blob)";
        bool MoveShape(const MoveRequest& r){return (!r.parent_yard||(!r.room&&!r.room_exterior_source&&OccupiedParentYard::Shape(*r.parent_yard)))&&(!r.room_exterior_source||(r.room&&r.room_exterior_source->zone_id&&r.room_exterior_source->instance_id&&r.room_exterior_source->instance_id<=32000))&&LeaseShape(r.lease)&&Key(r.request_key)&&r.estate_id&&r.custody_id&&r.native_item_id&&r.native_item_id!=UINT32_MAX&&r.expected_pose_revision&&r.expected_pose_revision!=UINT64_MAX&&r.expected_catalog_revision;}
        // SQL mutation only. Its caller owns transaction lifecycle and must
        // abort the entire parent operation if any checked member fails.
        MoveReceipt MoveChecked(Database& db,const MoveRequest& r,InventorySessionFence& fence,
          const Scene::HouseScenePolicy* room_scene,const InteriorContent::ProfilePolicy* room_profile,
          HouseBinding::CustodyContext* houses,CrateBinding::CustodyContext* crates){
            auto lease=Locked(db,r.lease);if(!lease.pending_key.empty())throw Failure{Error::PendingApplication};PlacementTables(db);MoveTable(db);auto binding=MoveBinding(r);auto prior=Q(db,"SELECT "+move_columns+" FROM housing_live_pose_moves WHERE request_key="+Quote(db,r.request_key)+" FOR UPDATE");if(prior.RowCount()){auto row=prior.begin();auto outcome=MoveRow(row,lease,r.request_key);if(Unhex(row[4])!=binding)throw Failure{Error::Conflict};return outcome;}
            // Historical replay above mutates nothing. Every new move freshly
            // verifies ownership/custody/catalog/current housing session context.
            auto rows=Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id="+std::to_string(r.custody_id)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::NotFound};auto stored=StoredRow(db,rows.begin(),r.custody_id);auto scope=HouseItemScope::Locked(db,r.custody_id);if(bool(scope)!=bool(r.room))throw Failure{Error::Unauthorized};if(stored.estate_id!=r.estate_id)throw Failure{Error::Unauthorized};
            // Pose-only movement does not transfer creator ownership. Lock the
            // creator's pending inventory application as well as the mover's
            // lease: the earlier Scene read cannot fence this transaction.
            // Missing/inactive/no-pending creator leases allow offline movement.
            auto creator_lease=Q(db,"SELECT pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(stored.row.values.character_id)+" FOR UPDATE");
            if(creator_lease.RowCount()>1)throw Failure{Error::DatabaseFailure};
            if(creator_lease.RowCount()&&creator_lease.begin()[0])throw Failure{Error::PendingApplication};
            auto current=CurrentPose(db,r.custody_id);if(!current)throw Failure{Error::NotFound};if(current->reservation.native_item_id!=r.native_item_id||current->pose_revision!=r.expected_pose_revision)throw Failure{Error::Conflict};auto catalog=Simple(db,stored.row);if(Cat(catalog)!=Cat(stored.catalog))throw Failure{Error::UnsupportedItem};
            if(ItemStorage::Locked(db,r.custody_id).stowed)throw Failure{Error::Conflict}; // Placement from storage uses an explicit transition, never a stale Move.
            uint64_t current_revision=0;
            if(r.room){
                if(!room_scene||!room_profile||!scope||scope->house_id!=r.room->house_id||scope->revision!=r.room->scope_revision)throw Failure{Error::Conflict};
                auto view=r.room_exterior_source?Scene::Service::ReadCurrentAttachedInteriorUnderTransaction(db,lease,r.room->house_id,fence,*room_scene,room_profile,crates):Scene::Service::ReadInteriorSceneUnderTransaction(db,lease,r.room->source,fence,*room_scene,*room_profile,crates);if(!view)throw Failure{view.error};
                if(view.value.context.source.zone_id!=r.room->source.zone_id||view.value.context.source.zone_version!=r.room->source.zone_version||view.value.context.source.instance_id!=r.room->source.instance_id||(r.room_exterior_source&&(!view.value.exterior_source||view.value.exterior_source->zone_id!=r.room_exterior_source->zone_id||view.value.exterior_source->zone_version!=r.room_exterior_source->zone_version||view.value.exterior_source->instance_id!=r.room_exterior_source->instance_id)))throw Failure{Error::Conflict};
                if(!view.value.house||!view.value.room_profile||view.value.house->house_id!=r.room->house_id||view.value.house->revision!=r.room->house_revision||view.value.profile_revision!=r.room->profile_revision||view.value.context.catalog_revision!=r.expected_catalog_revision)throw Failure{Error::Conflict};
                if(!view.value.access.effective.Allows(AccessPolicy::Capability::MoveItem))throw Failure{Error::Unauthorized};
                auto actor=std::find_if(view.value.contents.begin(),view.value.contents.end(),[&](const auto& item){return item.stored.custody_id==r.custody_id;});
                if(actor==view.value.contents.end()||!actor->stored.placement||PlacementBytes(*actor->stored.placement)!=PlacementBytes(*current))throw Failure{Error::Conflict};
                auto input=InteriorContent::ItemInputForStoredUnderTransaction(db,*view.value.room_profile,actor->stored);if(!input)throw Failure{input.error};if(input.value.npc&&!fence.NpcProfileMatchesUnderTransaction(db,lease,*input.value.npc))throw Failure{Error::Conflict};
                if(input.value.npc)input.value.geometry=NpcRuntime::Geometry(*input.value.npc,r.npc_frame,r.transform);else if(r.npc_frame)throw Failure{Error::InvalidArgument};
                PlacementPolicy::Context context{current->reservation.native_estate_id,view.value.room_profile->definition.type,view.value.room_profile->region.id,view.value.access.effective,true};if(input.value.geometry.mounting_type)context.mount_contact_verified=InteriorContent::MountForSelectedInputUnderTransaction(db,fence,lease,stored.row,r.transform,input.value,MountSubject::ExistingSource(stored.custody_id,current->reservation.native_item_id,current->pose_revision));if(!InteriorContent::CheckPlacement(input.value.room,context,input.value.item,input.value.geometry,r.transform,PlacementPolicy::Operation::Move))throw Failure{Error::UnsupportedItem};current_revision=view.value.context.catalog_revision;
            }else{std::optional<Scene::Snapshot> parent;if(r.parent_yard){auto read=OccupiedParentYard::ReadUnderTransaction(db,lease,*r.parent_yard,fence,room_scene,room_profile,crates);if(!read)throw Failure{read.error};parent=std::move(read.value);}current_revision=PlacementCatalog(db,current->reservation,stored.row,r.transform,false,fence,r.lease.actor,PlacementPolicy::Operation::Move,nullptr,r.expected_catalog_revision,houses,&lease,HouseBinding::PlacementStage::Move,0,crates,parent?&*parent:nullptr,r.npc_frame,false,MountSubject::ExistingSource(stored.custody_id,current->reservation.native_item_id,current->pose_revision));}
            stored.placement=current;CurrentCrate(db,stored);
            MoveReceipt out;out.room=r.room;out.room_exterior_source=r.room_exterior_source;out.parent_yard=r.parent_yard;out.committed_catalog_revision=current_revision;out.request_key=r.request_key;out.custody_id=r.custody_id;out.before=*current;out.after=*current;out.after.transform=r.transform;out.after.npc_frame=r.npc_frame;out.after.pose_revision++;
            auto changed=Q(db,"UPDATE housing_live_placed_items SET pose_revision="+std::to_string(out.after.pose_revision)+",pose_blob=x'"+Hex(Pose(out.after.transform))+"' WHERE custody_id="+std::to_string(r.custody_id)+" AND native_item_id="+std::to_string(r.native_item_id)+" AND pose_revision="+std::to_string(r.expected_pose_revision));if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};
            NpcNativeFrame::ReplaceUnderTransaction(db,r.custody_id,r.native_item_id,r.expected_pose_revision,r.native_item_id,out.after.pose_revision,out.after.transform,current->npc_frame,out.after.npc_frame);
            auto journal=Q(db,"INSERT INTO housing_live_pose_moves(request_key,character_id,account_id,lease_epoch,session_key,binding_blob,outcome_blob) VALUES ("+Quote(db,r.request_key)+","+std::to_string(lease.actor.character_id)+","+std::to_string(lease.actor.account_id)+","+std::to_string(lease.epoch)+","+Quote(db,lease.session_key)+",x'"+Hex(binding)+"',x'"+Hex(MoveOutcome(out))+"')");if(journal.RowsAffected()!=1)throw Failure{Error::Conflict};return out;
        }
    }
    Result<std::optional<Reservation>> Service::PendingNpcPlacementUnderTransaction(Database& db,bool enabled,const Lease& viewer,uint32_t creator,InventorySessionFence& fence){
        if(!enabled)return {{},Error::Disabled};
        if(!creator)return {{},Error::InvalidArgument};
        auto authenticated=AuthenticateUnderTransaction(db,enabled,viewer,fence);if(!authenticated)return {{},authenticated.error};
        if(!authenticated.value.pending_key.empty())return {{},Error::PendingApplication};
        try{
            auto subjects=Q(db,"SELECT "+lease_columns+" FROM housing_live_item_leases WHERE character_id="+std::to_string(creator)+" FOR UPDATE");
            if(!subjects.RowCount())return {std::nullopt,Error::None};if(subjects.RowCount()!=1)throw Failure{Error::DatabaseFailure};
            auto subject=LeaseRow(subjects.begin(),creator);if(subject.pending_key.empty())return {std::nullopt,Error::None};
            auto rows=Q(db,"SELECT "+receipt_columns+" FROM housing_live_item_receipts WHERE request_key="+Quote(db,subject.pending_key)+" FOR UPDATE");
            // An opaque/aggregate application cannot be assumed unrelated to
            // the caller's published NPC actors. Exact recovery resolves it.
            if(rows.RowCount()!=1)throw Failure{Error::PendingApplication};
            auto receipt=ReceiptRow(rows.begin(),subject,subject.pending_key);if(!receipt.requires_application)throw Failure{Error::Conflict};
            if(!receipt.stored.placement||!receipt.stored.placement->reservation.content_binding)return {std::nullopt,Error::None};
            const auto& r=receipt.stored.placement->reservation;
            if(r.content_binding->version!=ContentBinding::NpcClassVersion&&r.content_binding->version!=ContentBinding::GenericNpcClassVersion)return {std::nullopt,Error::None};
            return {std::optional<Reservation>{r},Error::None};
        }catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
    }
    Result<bool> Service::NoPendingInteriorPublicationUnderTransaction(Database& db,const Lease& viewer,uint32_t native,uint64_t house,InventorySessionFence& fence){
        if(!native||native==UINT32_MAX||!house)return {false,Error::InvalidArgument};
        auto authenticated=AuthenticateUnderTransaction(db,true,viewer,fence);if(!authenticated)return {false,authenticated.error};
        if(!authenticated.value.pending_key.empty())return {false,Error::PendingApplication};
        try{
            // Current locking read (not an old RR read-view): includes foreign
            // pickup recipients after source scope/custody was already deleted.
            // Existing room-head locks serialize physical writers; any SQL
            // deadlock/error refuses the owning transaction, never a partial set.
            auto subjects=Q(db,"SELECT character_id FROM housing_live_item_leases WHERE pending_key IS NOT NULL ORDER BY character_id LIMIT 1001 FOR UPDATE");
            if(subjects.RowCount()>1000)throw Failure{Error::PendingApplication};
            for(auto it=subjects.begin();it!=subjects.end();++it){
                if(!it[0])throw Failure{Error::DatabaseFailure};const auto creator=std::stoul(it[0]);auto current=Q(db,"SELECT "+lease_columns+" FROM housing_live_item_leases WHERE character_id="+std::to_string(creator)+" FOR UPDATE");if(current.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto subject=LeaseRow(current.begin(),creator);if(subject.pending_key.empty())continue;
                auto rows=Q(db,"SELECT "+receipt_columns+" FROM housing_live_item_receipts WHERE request_key="+Quote(db,subject.pending_key)+" FOR UPDATE");
                if(rows.RowCount()!=1)throw Failure{Error::PendingApplication};
                auto receipt=ReceiptRow(rows.begin(),subject,subject.pending_key);
                if(!receipt.requires_application)throw Failure{Error::PendingApplication};
                // House/crate aggregate effects or missing physical provenance
                // cannot be silently classified as another room's operation.
                if(receipt.house_effect_required||receipt.house_effect||!receipt.stored.placement)throw Failure{Error::PendingApplication};
                if(receipt.stored.placement->reservation.native_estate_id==native||(receipt.room&&receipt.room->house_id==house))throw Failure{Error::PendingApplication};
            }
            return {true,Error::None};
        }catch(const Failure& e){return {false,e.error};}catch(...){return {false,Error::PendingApplication};}
    }
    Result<Lease> Service::AuthenticateUnderTransaction(Database& db,bool enabled,const Lease& lease,InventorySessionFence& fence){
        if(!enabled)return {{},Error::Disabled};
        if(!LeaseShape(lease))return {{},Error::InvalidArgument};
        if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))return {{},Error::UnheldFence};
        try{
            auto tx=Q(db,"SELECT @@in_transaction");
            if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return {{},Error::InvalidArgument};
            Verify(db);return {Locked(db,lease),Error::None};
        }catch(const Failure& e){return {{},e.error};}catch(...){return {{},Error::DatabaseFailure};}
    }
    Result<MoveReceipt> Service::MovePlaced(const MoveRequest& r,InventorySessionFence& fence){if(r.room)return {{},Error::InvalidArgument};return MoveCurrent(r,fence,nullptr,nullptr);}
    Result<MoveReceipt> Service::MoveInterior(const MoveRequest& r,InventorySessionFence& fence,const Scene::HouseScenePolicy& scene,const InteriorContent::ProfilePolicy& profile){if(!r.room)return {{},Error::InvalidArgument};return MoveCurrent(r,fence,&scene,&profile);}
    Result<MoveReceipt> Service::MoveCurrent(const MoveRequest& r,InventorySessionFence& fence,const Scene::HouseScenePolicy* room_scene,const InteriorContent::ProfilePolicy* room_profile){
        std::lock_guard<std::mutex> lock(mutex_);
        if(!enabled_)return {{},Error::Disabled};
        if(!MoveShape(r))return {{},Error::InvalidArgument};
        if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
        return Run<MoveReceipt>(*database_,[&]{return MoveChecked(*database_,r,fence,room_scene,room_profile,houses_,crates_);});
    }
    Result<MoveReceipt> Service::MoveUnderTransaction(Database& db,bool enabled,const MoveRequest& r,InventorySessionFence& fence,
      HouseBinding::CustodyContext* houses,CrateBinding::CustodyContext* crates,
      const Scene::HouseScenePolicy* room_scene,const InteriorContent::ProfilePolicy* room_profile){
        if(!enabled)return {{},Error::Disabled};
        if(!MoveShape(r))return {{},Error::InvalidArgument};
        if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};
        if(db.GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
        try{
            auto transaction=Q(db,"SELECT @@in_transaction");
            if(transaction.RowCount()!=1||!transaction.begin()[0]||std::string(transaction.begin()[0])!="1")return {{},Error::InvalidArgument};
            Verify(db);
            return {MoveChecked(db,r,fence,room_scene,room_profile,houses,crates),Error::None};
        }catch(const Failure& e){return {{},e.error};}
        catch(const HouseBinding::Failure& e){
            switch(e.error){
            case HouseBinding::Error::InvalidArgument:return {{},Error::InvalidArgument};
            case HouseBinding::Error::Unauthorized:return {{},Error::Unauthorized};
            case HouseBinding::Error::UnheldFence:return {{},Error::UnheldFence};
            case HouseBinding::Error::Conflict:return {{},Error::Conflict};
            case HouseBinding::Error::PendingApplication:return {{},Error::PendingApplication};
            case HouseBinding::Error::NotFound:return {{},Error::NotFound};
            case HouseBinding::Error::UnsupportedItem:return {{},Error::UnsupportedItem};
            default:return {{},Error::DatabaseFailure};
            }
        }catch(const std::exception&){return {{},Error::DatabaseFailure};}
    }
    Result<MoveReceipt> Service::ResolveMove(const Lease& l,const std::string& key,InventorySessionFence& fence){
        std::lock_guard<std::mutex> lock(mutex_);if(!enabled_)return {{},Error::Disabled};if(!LeaseShape(l)||!Key(key))return {{},Error::InvalidArgument};if(!fence.HeldFor(l.actor,l.epoch,l.session_key))return {{},Error::UnheldFence};if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};return Run<MoveReceipt>(*database_,[&]{auto& db=*database_;auto lease=Locked(db,l);MoveTable(db);auto rows=Q(db,"SELECT "+move_columns+" FROM housing_live_pose_moves WHERE request_key="+Quote(db,key)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::NotFound};return MoveRow(rows.begin(),lease,key);});
    }

    Result<bool> Service::VerifyHouseAllocationParent(Database& db,const Request& request,InventorySessionFence& fence)
    {
        const auto parent=VerifyPlacedStoreAllocationParent(db,request,fence);
        if(parent&&(!request.house||request.room||!request.placement->reservation.content_binding||request.placement->reservation.content_binding->version!=ContentBinding::HouseVersion))return {{},Error::UnheldFence};
        return parent;
    }
    Result<bool> Service::VerifyPlacedStoreAllocationParent(Database& db,const Request& request,InventorySessionFence& fence)
    {
        try {
            auto transaction=Q(db,"SELECT @@in_transaction");
            if(transaction.RowCount()!=1||std::stoul(transaction.begin()[0])!=1)return {{},Error::InvalidArgument};
            if(request.action!=Action::Store||!request.placement||(!request.room&&(!request.placement->reservation.content_binding||!ContentBinding::SupportedVersion(request.placement->reservation.content_binding->version)))||(request.room&&request.placement->reservation.content_binding)||!Key(request.request_key)||!LeaseShape(request.lease)||!fence.HeldFor(request.lease.actor,request.lease.epoch,request.lease.session_key)||!fence.SourceMatches(request.expected,request.verified_runtime_serial))return {{},Error::UnheldFence};
            if(request.room){MatchRoomTag(db,*request.room);if(PlacementBytes({request.room->placement,request.placement->transform,1,request.placement->npc_frame})!=PlacementBytes(*request.placement))return {{},Error::Conflict};}
            Verify(db);const auto lease=Locked(db,request.lease);
            if(!lease.pending_key.empty())return {{},Error::PendingApplication};
            auto parent=Q(db,"SELECT "+receipt_columns+" FROM housing_live_item_receipts WHERE request_key="+Quote(db,request.request_key)+" FOR UPDATE");
            if(parent.RowCount()!=1)return {{},Error::NotFound};auto row=parent.begin();
            if(!row[0]||!row[1]||!row[2]||!row[3]||!row[4]||!row[5]||!row[6]||row[7]||std::stoul(row[0])!=lease.actor.character_id||std::stoul(row[1])!=lease.actor.account_id||std::stoull(row[2])!=lease.epoch||std::string(row[3])!=lease.session_key||std::stoul(row[5])||std::stoul(row[6])||Unhex(row[4])!=Binding(request))return {{},Error::Conflict};
            if(Q(db,"SELECT slot_id FROM inventory WHERE "+Slot(lease.actor.character_id,request.expected.values.slot_id)+" FOR UPDATE").RowCount())return {{},Error::Conflict};
            PlacementTables(db);
            auto custody=Q(db,"SELECT c.custody_id,c.estate_id,c.owner_character_id,HEX(c.row_blob) FROM housing_live_item_custody c JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id AND r.custody_id=c.custody_id AND r.state=1 WHERE p.native_item_id="+std::to_string(request.placement->reservation.native_item_id)+" FOR UPDATE");
            if(custody.RowCount()!=1)return {{},Error::Conflict};auto stored=custody.begin();
            if(!stored[0]||!stored[1]||!stored[2]||!stored[3]||std::stoul(stored[1])!=request.estate_id||std::stoul(stored[2])!=lease.actor.character_id||Unhex(stored[3])!=Encode(request.expected))return {{},Error::Conflict};
            auto placed=CurrentPose(db,std::stoull(stored[0]));
            if(!placed||PlacementBytes(*placed)!=PlacementBytes(*request.placement))return {{},Error::Conflict};
            return {true,Error::None};
        }catch(const Failure& failure){return {{},failure.error};}catch(...){return {{},Error::DatabaseFailure};}
    }
    Result<Receipt> Service::ValidateRestartOutcome(Database& db,const Lease& lease){return ValidateRestartOutcomeCurrent(db,lease,false,nullptr);}
    Result<Receipt> Service::ValidateRestartOutcome(Database& db,const Lease& lease,bool allow_acknowledged){return ValidateRestartOutcomeCurrent(db,lease,allow_acknowledged,nullptr);}
    Result<Receipt> Service::ValidateRestartOutcome(Database& db,const Lease& lease,const std::string* aggregate_pending){return ValidateRestartOutcomeCurrent(db,lease,false,aggregate_pending);}
    Result<Receipt> Service::ValidateRestartOutcomeCurrent(Database& db,const Lease& lease,bool allow_acknowledged,const std::string* aggregate_pending)
    {
        try {
            if (allow_acknowledged && aggregate_pending) return {{},Error::InvalidArgument};
            if (!LeaseShape(lease) || !Key(lease.pending_key)) return {{},Error::InvalidArgument};
            Verify(db);
            // Recovery already owns the process/session/lease locks. This lookup
            // validates immutable receipt identity, without current estate rights.
            auto rows = Q(db,"SELECT "+receipt_columns+" FROM housing_live_item_receipts WHERE request_key="+
                Quote(db,lease.pending_key)+" FOR UPDATE");
            if (rows.RowCount()!=1) return {{},Error::NotFound};
            auto receipt = ReceiptRow(rows.begin(),lease,lease.pending_key,allow_acknowledged);
            if (!receipt.requires_application&&!allow_acknowledged) return {{},Error::Conflict};
            const auto bytes = Unhex(rows.begin()[4]);
            auto request=Unbinding(bytes);request.request_key=lease.pending_key;
            if (request.lease.actor.character_id!=lease.actor.character_id ||
                request.lease.actor.account_id!=lease.actor.account_id || request.lease.epoch!=lease.epoch ||
                request.lease.session_key!=lease.session_key || request.lease.token!=lease.token ||
                request.estate_id!=receipt.stored.estate_id || request.action!=receipt.action ||
                Encode(request.expected)!=Encode(receipt.partition?receipt.partition->before:receipt.stored.row) || request.initial_stowed!=receipt.stored.stowed || Binding(request)!=bytes) {
                return {{},Error::DatabaseFailure};
            }
            auto custody = Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id="+
                std::to_string(receipt.stored.custody_id)+" FOR UPDATE");
            auto placement_schema=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_placed_items'");
            const bool have_placement=std::stoul(placement_schema.begin()[0])!=0;
            if (receipt.stored.placement && !have_placement) return {{},Error::DatabaseFailure};
            if(request.vault&&!(receipt.partition&&receipt.action==Action::Return)&&!VaultStorage::Outcome(db,*request.vault,receipt.stored,receipt.action==Action::Store))return {{},Error::Conflict};
            if (have_placement) PlacementTables(db);
            if (receipt.action==Action::Store) {
                if (custody.RowCount()!=1) return {{},Error::Conflict};
                const auto actual=StoredRow(db,custody.begin(),receipt.stored.custody_id);
                if (actual.estate_id!=receipt.stored.estate_id || Encode(actual.row)!=Encode(receipt.stored.row) ||
                    Cat(actual.catalog)!=Cat(receipt.stored.catalog)) return {{},Error::Conflict};
                auto source=Q(db,"SELECT slot_id FROM inventory WHERE "+Slot(lease.actor.character_id,
                    receipt.stored.row.values.slot_id)+" FOR UPDATE");
                if (source.RowCount()&&!receipt.partition) return {{},Error::Conflict};
                if(!receipt.partition){NoChildren(db,lease.actor.character_id,receipt.stored.row.values.slot_id);ItemPayload::ValidateHeld(db,actual.row,actual.custody_id);}
                if(receipt.partition)PartitionPostimage(db,receipt);
                const auto storage=ItemStorage::Locked(db,receipt.stored.custody_id);
                if(storage.stowed!=receipt.stored.stowed||storage.revision!=receipt.stored.storage_revision||(storage.revision&&(!receipt.stored.placement||storage.native_estate_id!=receipt.stored.placement->reservation.native_estate_id)))return {{},Error::Conflict};
                const auto pose=have_placement ? CurrentPose(db,receipt.stored.custody_id,bool(receipt.crate_member)) : std::optional<Placed>{};
                if (bool(pose)!=bool(receipt.stored.placement) || bool(request.placement)!=bool(receipt.stored.placement)) {
                    return {{},Error::Conflict};
                }
                if (pose && (PlacementBytes(*pose)!=PlacementBytes(*receipt.stored.placement) ||
                    PlacementBytes(*request.placement)!=PlacementBytes(*receipt.stored.placement))) return {{},Error::Conflict};
                if (pose) {
                    auto reservation=Q(db,"SELECT "+ReservationColumns(db)+" FROM housing_live_placement_reservations WHERE native_item_id="+
                        std::to_string(pose->reservation.native_item_id)+" FOR UPDATE");
                    if (reservation.RowCount()!=1) return {{},Error::Conflict};
                    auto row=reservation.begin();
                    HeldReservation(row,lease);
                    if (std::string(row[11])!="1" || !row[12] || std::stoull(row[12])!=receipt.stored.custody_id ||
                        Unhex(row[9])!=Encode(receipt.stored.row) || std::stoll(row[10])!=request.verified_runtime_serial) {
                        return {{},Error::Conflict};
                    }
                }
            } else {
                if ((!receipt.partition&&custody.RowCount()) || request.custody_id!=receipt.stored.custody_id || request.placement ||
                    request.destination_slot!=receipt.inventory_after.values.slot_id ||
                    receipt.inventory_after.values.character_id!=lease.actor.character_id) return {{},Error::Conflict};
                auto expected_after=receipt.stored.row;
                ItemPayload::Owner(expected_after,lease.actor.character_id);expected_after.values.slot_id=request.destination_slot;
                if(receipt.partition){expected_after.values.charges=receipt.partition->quantity;expected_after.values.guid=receipt.partition->serial;expected_after.null_fields&=~uint8_t(NullCharges|NullGuid);PartitionPostimage(db,receipt);}
                if (Encode(expected_after)!=Encode(receipt.inventory_after)) return {{},Error::Conflict};
                auto inventory=Q(db,"SELECT "+select_inventory+" FROM inventory WHERE "+Slot(lease.actor.character_id,
                    request.destination_slot)+" FOR UPDATE");
                if (inventory.RowCount()!=1 || Encode(ItemPayload::Complete(db,InvRow(inventory.begin()),&receipt.inventory_after))!=Encode(receipt.inventory_after)) {
                    return {{},Error::Conflict};
                }
                ItemPayload::ValidateInventory(db,receipt.inventory_after);
                if (have_placement&&!receipt.partition) {
                    auto remaining_pose=Q(db,"SELECT custody_id FROM housing_live_placed_items WHERE custody_id="+
                        std::to_string(receipt.stored.custody_id)+" FOR UPDATE");
                    if (remaining_pose.RowCount()) return {{},Error::Conflict};
                }
                if (receipt.stored.placement&&!receipt.partition) {
                    auto reservation=Q(db,"SELECT "+ReservationColumns(db)+" FROM housing_live_placement_reservations WHERE native_item_id="+
                        std::to_string(receipt.stored.placement->reservation.native_item_id)+" FOR UPDATE");
                    if (reservation.RowCount()!=1) return {{},Error::Conflict};
                    auto row=reservation.begin();
                    // The placement belongs to the original Store lease, which
                    // can precede the Return lease. Its immutable outcome is kept.
                    const auto immutable=ReservationRow(row);
                    if (std::string(row[11])!="2" || row[12] || std::stoul(row[5])!=receipt.stored.row.values.character_id ||
                        (receipt.stored.row.values.character_id==lease.actor.character_id?std::stoul(row[6])!=lease.actor.account_id:!std::stoul(row[6])) || Unhex(row[9])!=Encode(receipt.stored.row) ||
                        PlacementBytes({immutable,receipt.stored.placement->transform,receipt.stored.placement->pose_revision,receipt.stored.placement->npc_frame})!=
                            PlacementBytes(*receipt.stored.placement)) return {{},Error::Conflict};
                    auto remaining_native=Q(db,"SELECT custody_id FROM housing_live_placed_items WHERE native_item_id="+
                        std::to_string(immutable.native_item_id)+" FOR UPDATE");
                    if (remaining_native.RowCount()) return {{},Error::Conflict};
                }
            }
            AttachRoom(db,receipt);
            if(allow_acknowledged&&!receipt.requires_application){auto proof=receipt;proof.requires_application=true;ValidatePendingCrate(db,proof);}else ValidatePendingCrate(db,receipt);
            if(HasHouseEffect(db,lease.pending_key)!=receipt.house_effect_required)return {{},Error::DatabaseFailure};
            if(receipt.house_effect_required){
                receipt.house_effect=std::make_shared<HouseBinding::Effect>(HouseBinding::ValidateRestartOutcome(db,lease,receipt,aggregate_pending));
                MatchHouseRequest(request,receipt);
            }
            return {std::move(receipt),Error::None};
        } catch (const Failure& failure) { return {{},failure.error}; }
        catch (...) { return {{},Error::DatabaseFailure}; }
    }

    namespace {
        // New custody-only count journal. Existing whole transfer binding and
        // table rows remain exact. No inventory slot, Return or Store is run.
        void VaultCountInstalled(Database& db){auto rows=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_live_vault_partitions'");if(rows.RowCount()!=1||!rows.begin()[0]||std::string(rows.begin()[0])!="InnoDB")throw Failure{Error::Disabled};}
        Bytes VaultCountBytes(const StackPartition& p,EstateId estate){Bytes b;W(b,1,4);W(b,estate,4);B(b,Encode(p.before));B(b,Encode(p.after));for(auto n:{p.quantity,p.serial,p.maximum})W(b,n,4);W(b,p.vault_scope_revision,8);return b;}
        std::shared_ptr<StackPartition> VaultCountRead(const Bytes& bytes,const VaultTransferRequest& request,EstateId& estate){Reader reader{bytes};if(reader.N(4)!=1)throw Failure{Error::DatabaseFailure};estate=reader.N(4);if(!estate||estate==UINT32_MAX)throw Failure{Error::DatabaseFailure};auto p=std::make_shared<StackPartition>();p->before=CustodyCodec::Decode(reader.Blob(CustodyCodec::MaximumPayload));p->after=CustodyCodec::Decode(reader.Blob(CustodyCodec::MaximumPayload));p->quantity=reader.N(4);p->serial=reader.N(4);p->maximum=reader.N(4);p->vault_scope_revision=reader.N(8);p->storage_before=request.from.expected_revision;p->storage_after=p->storage_before+1;p->vault_source=request.from;if(reader.at!=bytes.size()||VaultCountBytes(*p,estate)!=bytes||Encode(p->before)!=Encode(request.expected)||p->quantity!=request.partition_quantity||p->serial!=request.partition_serial)throw Failure{Error::Conflict};return p;}
        InventoryRow VaultCountPiece(const StackPartition& p){auto row=p.before;row.values.charges=p.quantity;row.values.guid=p.serial;row.null_fields&=~uint8_t(NullCharges|NullGuid);return row;}
        void VaultCountPostimage(Database& db,const VaultTransferRequest& request,const VaultTransferReceipt& result){
            if(!result.partition||result.custody_id==request.custody_id||result.native_item_id==request.from.expected_native_item_id||result.native_estate_id!=request.to.native_estate_id||result.revision!=1)throw Failure{Error::Conflict};const auto& part=*result.partition;
            Request proof;proof.action=Action::Return;proof.lease=request.lease;proof.expected=request.expected;proof.vault=request.from;proof.partition_quantity=part.quantity;proof.partition_serial=part.serial;
            if(PartitionMaximum(db,proof,part.before)!=part.maximum)throw Failure{Error::Conflict};
            Receipt source;source.action=Action::Return;source.stored={request.custody_id,result.custody_estate_id,part.before,Simple(db,part.before,true)};source.inventory_after=VaultCountPiece(part);source.partition=result.partition;
            // Reuse the exact immutable source count/scope validator only;
            // this call performs no inventory read, mutation, apply or ACK.
            PartitionPostimage(db,source);
            auto rows=Q(db,"SELECT "+stored_columns+" FROM housing_live_item_custody WHERE custody_id="+std::to_string(result.custody_id)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{Error::Conflict};auto child=StoredRow(db,rows.begin(),result.custody_id);
            if(child.estate_id!=result.custody_estate_id||Encode(child.row)!=Encode(source.inventory_after)||Cat(child.catalog)!=Cat(source.stored.catalog)||CurrentPose(db,child.custody_id)||ItemStorage::Locked(db,child.custody_id).revision||CrateMemberScope::Locked(db,child.custody_id,true)||!VaultStorage::Outcome(db,request.to,child,true))throw Failure{Error::Conflict};
            auto identity=Q(db,"SELECT native_item_id,runtime_serial,character_id FROM housing_live_placement_reservations WHERE custody_id="+std::to_string(child.custody_id)+" AND state=1 FOR UPDATE");if(identity.RowCount()!=1||!identity.begin()[0]||!identity.begin()[1]||!identity.begin()[2]||std::stoul(identity.begin()[0])!=result.native_item_id||std::stoul(identity.begin()[1])!=part.serial||std::stoul(identity.begin()[2])!=part.before.values.character_id)throw Failure{Error::Conflict};
            auto scope=HouseItemScope::Locked(db,child.custody_id);if(request.to.house_id?(!scope||scope->revision!=1):bool(scope))throw Failure{Error::Conflict};
        }
    }

    Result<VaultTransferReceipt> Service::TransferVault(const VaultTransferRequest& request,InventorySessionFence& fence,const Scene::HouseScenePolicy* houses,const InteriorContent::ProfilePolicy* profile){
        std::lock_guard<std::mutex> guard(mutex_);if(!enabled_)return {{},Error::Disabled};
        if(!Key(request.request_key)||!LeaseShape(request.lease)||!request.estate_id||!request.custody_id||request.from.estate_id!=request.estate_id||request.to.estate_id!=request.estate_id||!request.from.expected_native_item_id||!request.from.expected_revision||request.from.expected_revision==UINT64_MAX||request.to.expected_native_item_id||request.to.expected_revision||!VaultStorage::SameRoute(request.from.source,request.to.source))return {{},Error::InvalidArgument};
        if(!fence.HeldFor(request.lease.actor,request.lease.epoch,request.lease.session_key))return {{},Error::UnheldFence};if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
        return Run<VaultTransferReceipt>(*database_,[&]{auto&db=*database_;auto lease=Locked(db,request.lease);if(!lease.pending_key.empty())throw Failure{Error::PendingApplication};if(!VaultStorage::Installed(db))throw Failure{Error::Disabled};
            Bytes binding;W(binding,1,4);W(binding,lease.actor.character_id,4);W(binding,lease.actor.account_id,4);W(binding,lease.epoch,8);S(binding,lease.session_key);binding.insert(binding.end(),lease.token.begin(),lease.token.end());W(binding,request.estate_id,4);W(binding,request.custody_id,8);
            for(const auto* ptr:{&request.from,&request.to}){const auto&v=*ptr;W(binding,v.estate_id,4);W(binding,v.native_estate_id,4);for(auto x:{v.catalog_revision,v.house_id,v.house_revision,v.profile_revision})W(binding,x,8);W(binding,v.source.zone_id,4);W(binding,v.source.zone_version,1);W(binding,v.source.instance_id,2);W(binding,v.expected_native_item_id,4);W(binding,v.expected_revision,8);}
            if(request.partition_quantity){VaultCountInstalled(db);W(binding,1,4);B(binding,Encode(request.expected));W(binding,request.partition_quantity,4);W(binding,request.partition_serial,4);}else if(request.partition_serial)throw Failure{Error::InvalidArgument};
            if(request.from.crate_id||request.to.crate_id){W(binding,0x56435231,4);for(const auto* c:{&request.from,&request.to}){VaultStorage::ContextShape(*c);W(binding,c->crate_id,8);W(binding,c->crate_revision,8);W(binding,c->member_revision,8);}}
            const auto key=Quote(db,request.request_key);auto prior=Q(db,"SELECT character_id,account_id,HEX(binding_blob),custody_id,native_item_id,native_estate_id,revision FROM housing_live_vault_transfers WHERE request_key="+key+" FOR UPDATE");
            if(prior.RowCount()){if(prior.RowCount()!=1)throw Failure{Error::Conflict};auto r=prior.begin();for(unsigned i=0;i<7;++i)if(!r[i])throw Failure{Error::DatabaseFailure};if(std::stoul(r[0])!=lease.actor.character_id||std::stoul(r[1])!=lease.actor.account_id||Unhex(r[2])!=binding)throw Failure{Error::Conflict};VaultTransferReceipt result{std::stoull(r[3]),std::stoull(r[6]),uint32_t(std::stoul(r[4])),uint32_t(std::stoul(r[5])),true};if(request.partition_quantity){auto count=Q(db,"SELECT format_version,HEX(outcome_blob) FROM housing_live_vault_partitions WHERE request_key="+key+" FOR UPDATE");if(count.RowCount()!=1||!count.begin()[0]||std::stoul(count.begin()[0])!=1)throw Failure{Error::Conflict};result.partition=VaultCountRead(Unhex(count.begin()[1]),request,result.custody_estate_id);VaultCountPostimage(db,request,result);}return result;}
            const auto source=VaultStorage::CurrentTarget(db,lease,request.estate_id,request.from,fence,houses,profile);const auto destination=VaultStorage::CurrentTarget(db,lease,request.estate_id,request.to,fence,houses,profile);
            if(!source.access.Allows(AccessPolicy::Capability::MoveItem)||!destination.access.Allows(AccessPolicy::Capability::AddItem))throw Failure{Error::Unauthorized};
            if(request.to.member_revision||(request.from.crate_id&&request.from.crate_id==request.to.crate_id))throw Failure{Error::InvalidArgument};
            const auto items=VaultStorage::ContextItems(db,request.from);const auto found=std::find_if(items.begin(),items.end(),[&](const auto&item){return item.stored.custody_id==request.custody_id&&item.native_item_id==request.from.expected_native_item_id&&item.revision==request.from.expected_revision&&item.membership_revision==request.from.member_revision;});if(found==items.end()||((request.from.crate_id||request.to.crate_id)&&(found->stored.row.values.character_id!=lease.actor.character_id||Encode(request.expected)!=Encode(found->stored.row))))throw Failure{Error::Conflict};
            if(request.partition_quantity){
                if(found->stored.row.values.character_id!=lease.actor.character_id||Encode(request.expected)!=Encode(found->stored.row))throw Failure{Error::Conflict};
                if(auto capacity=VaultStorage::CapacityError(destination,VaultStorage::Count(db,destination));capacity!=Error::None)throw Failure{capacity};
                Request proof;proof.action=Action::Return;proof.lease=lease;proof.expected=request.expected;proof.vault=request.from;proof.partition_quantity=request.partition_quantity;proof.partition_serial=request.partition_serial;
                auto part=std::make_shared<StackPartition>();part->maximum=PartitionMaximum(db,proof,found->stored.row);part->before=found->stored.row;part->after=part->before;part->quantity=request.partition_quantity;part->serial=request.partition_serial;part->after.values.charges=StackCount(part->before)-part->quantity;part->after.null_fields&=~NullCharges;part->storage_before=found->revision;part->storage_after=found->revision+1;part->vault_source=request.from;auto scope=HouseItemScope::Locked(db,request.custody_id);part->vault_scope_revision=scope?scope->revision:0;auto piece=VaultCountPiece(*part);
                auto one=[&](const std::string& sql){auto r=Q(db,sql);if(r.RowsAffected()!=1)throw Failure{Error::Conflict};return r;};const auto source_id=std::to_string(request.custody_id),native=std::to_string(found->native_item_id);
                one("UPDATE housing_live_item_custody SET row_blob=x'"+Hex(Encode(part->after))+"' WHERE custody_id="+source_id+" AND row_blob=x'"+Hex(Encode(part->before))+"'");
                one("UPDATE housing_live_placement_reservations SET source_row=x'"+Hex(Encode(part->after))+"' WHERE native_item_id="+native+" AND custody_id="+source_id+" AND state=1 AND source_row=x'"+Hex(Encode(part->before))+"'");
                one("UPDATE housing_live_vault_items SET revision=revision+1 WHERE custody_id="+source_id+" AND native_item_id="+native+" AND revision="+std::to_string(found->revision));
                auto inserted=one("INSERT INTO housing_live_item_custody(estate_id,owner_character_id,schema_descriptor,row_blob,catalog_blob) SELECT estate_id,owner_character_id,schema_descriptor,x'"+Hex(Encode(piece))+"',catalog_blob FROM housing_live_item_custody WHERE custody_id="+source_id+" AND row_blob=x'"+Hex(Encode(part->after))+"'");if(!inserted.LastInsertedID())throw Failure{Error::Conflict};const auto child=inserted.LastInsertedID();
                auto allocated=one("INSERT INTO housing_live_placement_reservations(request_key,native_estate_id,estate_id,catalog_key,catalog_revision,character_id,account_id,lease_epoch,session_key,source_row,runtime_serial,state,custody_id) SELECT "+key+","+std::to_string(destination.native)+",estate_id,"+Quote(db,destination.catalog_key)+","+std::to_string(destination.catalog_revision)+",character_id,account_id,lease_epoch,session_key,x'"+Hex(Encode(piece))+"',"+std::to_string(part->serial)+",1,"+std::to_string(child)+" FROM housing_live_placement_reservations WHERE native_item_id="+native+" AND custody_id="+source_id+" AND state=1 AND source_row=x'"+Hex(Encode(part->after))+"'");if(!allocated.LastInsertedID()||allocated.LastInsertedID()>=UINT32_MAX)throw Failure{Error::Conflict};const uint32_t child_native=allocated.LastInsertedID();
                one("INSERT INTO housing_live_vault_items(custody_id,native_item_id,realm_estate_id,house_id,revision) VALUES("+std::to_string(child)+","+std::to_string(child_native)+","+std::to_string(request.to.estate_id)+","+(destination.house_id?std::to_string(destination.house_id):"NULL")+",1)");
                if(destination.house_id)one("INSERT INTO housing_house_item_scopes(custody_id,house_id,revision) VALUES("+std::to_string(child)+","+std::to_string(destination.house_id)+",1)");
                auto remaining=found->stored;remaining.row=part->after;VaultStorage::CrateWrite(db,request.from,remaining,part->storage_after,2);auto moved=found->stored;moved.custody_id=child;moved.row=piece;VaultStorage::CrateWrite(db,request.to,moved,1,1);
                VaultTransferReceipt result{child,1,child_native,destination.native,false,part,found->stored.estate_id};VaultCountPostimage(db,request,result);
                one("INSERT INTO housing_live_vault_partitions(request_key,format_version,outcome_blob) VALUES("+key+",1,x'"+Hex(VaultCountBytes(*part,found->stored.estate_id))+"')");
                one("INSERT INTO housing_live_vault_transfers(request_key,character_id,account_id,binding_blob,custody_id,native_item_id,native_estate_id,revision) VALUES("+key+","+std::to_string(lease.actor.character_id)+","+std::to_string(lease.actor.account_id)+",x'"+Hex(binding)+"',"+std::to_string(child)+","+std::to_string(child_native)+","+std::to_string(destination.native)+",1)");return result;
            }
            uint64_t revision=found->revision;
            if(source.native!=destination.native||source.house_id!=destination.house_id||request.from.crate_id!=request.to.crate_id){
                if(auto capacity=VaultStorage::CapacityError(destination,VaultStorage::Count(db,destination));capacity!=Error::None)throw Failure{capacity};
                auto changed=Q(db,"UPDATE housing_live_vault_items SET revision=revision+1,realm_estate_id="+std::to_string(request.to.estate_id)+",house_id="+(destination.house_id?std::to_string(destination.house_id):"NULL")+" WHERE custody_id="+std::to_string(found->stored.custody_id)+" AND native_item_id="+std::to_string(found->native_item_id)+" AND revision="+std::to_string(found->revision));if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};
                auto native=Q(db,"UPDATE housing_live_placement_reservations SET native_estate_id="+std::to_string(destination.native)+",catalog_key="+Quote(db,destination.catalog_key)+",catalog_revision="+std::to_string(destination.catalog_revision)+" WHERE native_item_id="+std::to_string(found->native_item_id)+" AND state=1 AND custody_id="+std::to_string(found->stored.custody_id));if(native.RowsAffected()>1||(source.native!=destination.native&&native.RowsAffected()!=1))throw Failure{Error::Conflict};
                if(source.house_id){auto removed=Q(db,"DELETE FROM housing_house_item_scopes WHERE custody_id="+std::to_string(found->stored.custody_id)+" AND house_id="+std::to_string(source.house_id));if(removed.RowsAffected()!=1)throw Failure{Error::Conflict};}
                if(destination.house_id){auto added=Q(db,"INSERT INTO housing_house_item_scopes(custody_id,house_id,revision) VALUES("+std::to_string(found->stored.custody_id)+","+std::to_string(destination.house_id)+",1)");if(added.RowsAffected()!=1)throw Failure{Error::Conflict};}++revision;
            }
            if(request.from.crate_id!=request.to.crate_id){VaultStorage::CrateWrite(db,request.from,found->stored,0,0);VaultStorage::CrateWrite(db,request.to,found->stored,revision,1);}
            auto saved=Q(db,"INSERT INTO housing_live_vault_transfers(request_key,character_id,account_id,binding_blob,custody_id,native_item_id,native_estate_id,revision) VALUES("+key+","+std::to_string(lease.actor.character_id)+","+std::to_string(lease.actor.account_id)+",x'"+Hex(binding)+"',"+std::to_string(found->stored.custody_id)+","+std::to_string(found->native_item_id)+","+std::to_string(destination.native)+","+std::to_string(revision)+")");if(saved.RowsAffected()!=1)throw Failure{Error::Conflict};return VaultTransferReceipt{found->stored.custody_id,revision,found->native_item_id,destination.native,false};
        });
    }

}
