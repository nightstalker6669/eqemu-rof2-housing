#include "item_service.h"
#include "../database.h"
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace Housing::Items {
    namespace {
        struct Failure{
            Error error;
        };
        MySQLRequestResult Q(Database& db,const std::string& sql){
            auto r=db.QueryDatabase(sql,false);
            if(!r.Success())throw Failure{
                Error::DatabaseFailure
            };
            return r;
        }
        class Tx {
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
            private:Database& db;
            bool active=true;
        };
        static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559, "Item fixture record requires IEEE754 binary64");
        using Bytes=std::vector<uint8_t>;
        bool Key(const std::string& s){
            if(s.empty()||s.size()>96)return false;
            for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c==':'||c=='_'||c=='.'))return false;
            return true;
        }
        bool Label(const std::string& s,size_t max){
            return !s.empty()&&s.size()<=max&&s.find('\0')==std::string::npos;
        }
        std::string Quote(Database& db,const std::string& s){
            return "'"+db.Escape(s)+"'";
        }
        std::string Hex(const Bytes& b){
            const char* digits="0123456789abcdef";
            std::string s;
            s.reserve(b.size()*2);
            for(auto v:b){
                s+=digits[v>>4];
                s+=digits[v&15];
            }
            return s;
        }
        Bytes Unhex(const char* p){
            if(!p)throw Failure{
                Error::DatabaseFailure
            };
            std::string s=p;
            if(s.size()%2||s.size()>140000)throw Failure{
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
        void Text(Bytes& b,const std::string& s){
            W(b,s.size(),4);
            b.insert(b.end(),s.begin(),s.end());
        }
        void Double(Bytes& b,double d){
            if(d==0)d=0;
            uint64_t bits;
            std::memcpy(&bits,&d,8);
            W(b,bits,8);
        }
        void Pose(Bytes& b,const Transform& t){
            for(auto v:t.position)Double(b,v);
            for(auto v:t.rotation)Double(b,v);
            Double(b,t.scale);
        }
        struct Reader {
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
            std::string S(size_t max){
                auto n=N(4);
                if(n>max||n>b.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                std::string s(b.begin()+at,b.begin()+at+n);
                at+=n;
                return s;
            }
            double D(){
                auto bits=N(8);
                double d;
                std::memcpy(&d,&bits,8);
                return d;
            }
        };
        bool Finite(const Transform& t){
            if(!std::isfinite(t.scale)||t.scale<=0)return false;
            for(auto v:t.position)if(!std::isfinite(v))return false;
            for(auto v:t.rotation)if(!std::isfinite(v))return false;
            return true;
        }
        bool DefaultPose(const Transform& t){
            for(auto v:t.position)if(v!=0)return false;
            for(auto v:t.rotation)if(v!=0)return false;
            return t.scale==1;
        }
        bool ValidIdentity(const Identity& i){
            return Key(i.instance_key)&&i.catalog_item_id&&i.original_owner_character_id&&Label(i.format,96)&&Label(i.provenance,256)&&!i.payload.empty()&&i.payload.size()<=65535;
        }
        bool ValidSnapshot(const Snapshot& s){
            if(!ValidIdentity(s.identity)||!s.revision||!Finite(s.transform))return false;
            if(s.location==Location::Inventory)return !s.estate_id&&s.inventory_slot&&s.bounds_provenance.empty()&&DefaultPose(s.transform);
            if(s.location==Location::Stored)return s.estate_id&&!s.inventory_slot&&s.bounds_provenance.empty()&&DefaultPose(s.transform);
            if(s.location==Location::Placed)return s.estate_id&&!s.inventory_slot&&Label(s.bounds_provenance,256);
            return false;
        }
        Bytes Encode(const Snapshot& s){
            Bytes b;
            W(b,1,4);
            Text(b,s.identity.instance_key);
            W(b,s.identity.catalog_item_id,4);
            W(b,s.identity.original_owner_character_id,4);
            W(b,uint32_t(s.identity.lore_group),4);
            W(b,s.identity.no_drop,1);
            Text(b,s.identity.format);
            Text(b,s.identity.provenance);
            W(b,s.identity.payload.size(),4);
            b.insert(b.end(),s.identity.payload.begin(),s.identity.payload.end());
            W(b,s.revision,8);
            W(b,uint8_t(s.location),1);
            W(b,s.estate_id,4);
            W(b,s.inventory_slot,4);
            Pose(b,s.transform);
            Text(b,s.bounds_provenance);
            return b;
        }
        Snapshot Decode(const Bytes& b){
            Reader r{
                b
            };
            if(r.N(4)!=1)throw Failure{
                Error::DatabaseFailure
            };
            Snapshot s;
            s.identity.instance_key=r.S(96);
            s.identity.catalog_item_id=uint32_t(r.N(4));
            s.identity.original_owner_character_id=uint32_t(r.N(4));
            auto lore=uint32_t(r.N(4));
            std::memcpy(&s.identity.lore_group,&lore,4);
            auto flag=r.N(1);
            if(flag>1)throw Failure{
                Error::DatabaseFailure
            };
            s.identity.no_drop=flag;
            s.identity.format=r.S(96);
            s.identity.provenance=r.S(256);
            auto size=r.N(4);
            if(!size||size>65535||size>b.size()-r.at)throw Failure{
                Error::DatabaseFailure
            };
            s.identity.payload.assign(b.begin()+r.at,b.begin()+r.at+size);
            r.at+=size;
            s.revision=r.N(8);
            s.location=Location(r.N(1));
            s.estate_id=uint32_t(r.N(4));
            s.inventory_slot=uint32_t(r.N(4));
            for(auto& v:s.transform.position)v=r.D();
            for(auto& v:s.transform.rotation)v=r.D();
            s.transform.scale=r.D();
            s.bounds_provenance=r.S(256);
            if(r.at!=b.size()||!ValidSnapshot(s))throw Failure{
                Error::DatabaseFailure
            };
            return s;
        }
        Bytes Binding(const Request& r){
            Bytes b;
            W(b,1,4);
            W(b,r.actor.character_id,4);
            W(b,r.actor.account_id,4);
            W(b,r.estate_id,4);
            W(b,uint8_t(r.action),1);
            W(b,r.destination_slot,4);
            auto record=Encode(r.expected);
            W(b,record.size(),4);
            b.insert(b.end(),record.begin(),record.end());
            Pose(b,r.transform);
            W(b,r.bounds.has_value(),1);
            if(r.bounds){
                const auto& v=*r.bounds;
                W(b,v.estate_id,4);
                Text(b,v.provenance);
                for(auto a:{
                    v.minimum_position,v.maximum_position,v.minimum_rotation,v.maximum_rotation
                })for(auto n:a)Double(b,n);
                Double(b,v.minimum_scale);
                Double(b,v.maximum_scale);
            }
            return b;
        }
        void Geometry(const Request& r){
            if(!r.bounds||r.bounds->estate_id!=r.estate_id||!Label(r.bounds->provenance,256)||!Finite(r.transform))throw Failure{
                Error::InvalidGeometry
            };
            const auto& b=*r.bounds;
            for(size_t i=0;i<3;++i){
                if(!std::isfinite(b.minimum_position[i])||!std::isfinite(b.maximum_position[i])||b.minimum_position[i]>b.maximum_position[i]||r.transform.position[i]<b.minimum_position[i]||r.transform.position[i]>b.maximum_position[i])throw Failure{
                    Error::InvalidGeometry
                };
                if(!std::isfinite(b.minimum_rotation[i])||!std::isfinite(b.maximum_rotation[i])||b.minimum_rotation[i]>b.maximum_rotation[i]||r.transform.rotation[i]<b.minimum_rotation[i]||r.transform.rotation[i]>b.maximum_rotation[i])throw Failure{
                    Error::InvalidGeometry
                };
            }
            if(!std::isfinite(b.minimum_scale)||!std::isfinite(b.maximum_scale)||b.minimum_scale<=0||b.minimum_scale>b.maximum_scale||r.transform.scale<b.minimum_scale||r.transform.scale>b.maximum_scale)throw Failure{
                Error::InvalidGeometry
            };
        }
        void VerifyDB(Database& db){
            auto n=Q(db,"SELECT DATABASE()");
            if(n.RowCount()!=1||!n.begin()[0])throw Failure{
                Error::UnsupportedFunding
            };
            std::string name=n.begin()[0];
            if(name!="peq_housing_test"&&name.rfind("peq_housing_test_",0)!=0)throw Failure{
                Error::UnsupportedFunding
            };
            auto tables=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_fixture_item_custody','housing_fixture_item_receipts','housing_estates','character_data','account')");
            if(tables.RowCount()!=1||std::stoul(tables.begin()[0])!=5)throw Failure{
                Error::DatabaseFailure
            };
        }
        void Actor(Database& db,AuthenticatedPurchaser who){
            auto c=Q(db,"SELECT account_id,name,gm,deleted_at,ingame FROM character_data WHERE id="+std::to_string(who.character_id)+" FOR UPDATE");
            if(c.RowCount()!=1||std::stoul(c.begin()[0])!=who.account_id||c.begin()[3]||std::stoul(c.begin()[2]))throw Failure{
                Error::Unauthorized
            };
            std::string name=c.begin()[1],prefix="ItemFixture";
            if(name.size()!=prefix.size()+13||name.compare(0,prefix.size(),prefix))throw Failure{
                Error::Unauthorized
            };
            for(size_t i=prefix.size();i<name.size();++i)if(name[i]<'0'||name[i]>'9')throw Failure{
                Error::Unauthorized
            };
            auto a=Q(db,"SELECT name,status,lsaccount_id,(password IS NULL OR password='') FROM account WHERE id="+std::to_string(who.account_id)+" FOR UPDATE");
            if(a.RowCount()!=1||name!=a.begin()[0]||std::stoul(a.begin()[1])||a.begin()[2]||std::stoul(a.begin()[3])!=1)throw Failure{
                Error::Unauthorized
            };
            if(std::stoul(c.begin()[4]))throw Failure{
                Error::UnsupportedFunding
            };
        }
        void EstateOwner(Database& db,EstateId id,AuthenticatedPurchaser actor){
            auto e=Q(db,"SELECT owner_kind,owner_id FROM housing_current_estates WHERE id="+std::to_string(id)+" FOR UPDATE");
            if(e.RowCount()!=1)throw Failure{
                Error::NotFound
            };
            if(std::stoul(e.begin()[0])!=uint32_t(OwnerKind::Character)||std::stoul(e.begin()[1])!=actor.character_id)throw Failure{
                Error::Unauthorized
            };
        }
        const std::string columns="owner_character_id,catalog_item_id,revision,custody_state,estate_id,inventory_slot,HEX(record_blob),holder_character_id";
        Snapshot Row(MySQLRequestRow row,const std::string& key){
            auto s=Decode(Unhex(row[6]));
            if(s.identity.instance_key!=key||s.identity.original_owner_character_id!=std::stoul(row[0])||s.identity.catalog_item_id!=std::stoul(row[1])||s.revision!=std::stoull(row[2])||uint8_t(s.location)!=std::stoul(row[3])||s.estate_id!=(row[4]?std::stoul(row[4]):0)||s.inventory_slot!=(row[5]?std::stoul(row[5]):0)||(s.location==Location::Inventory?(!row[7]||std::stoul(row[7])!=s.identity.original_owner_character_id):bool(row[7])))throw Failure{
                Error::DatabaseFailure
            };
            return s;
        }
        std::string LocationSQL(const Snapshot& s){
            return "revision="+std::to_string(s.revision)+",custody_state="+std::to_string(uint8_t(s.location))+",estate_id="+(s.estate_id?std::to_string(s.estate_id):"NULL")+",holder_character_id="+(s.location==Location::Inventory?std::to_string(s.identity.original_owner_character_id):"NULL")+",inventory_slot="+(s.inventory_slot?std::to_string(s.inventory_slot):"NULL")+",record_blob=x'"+Hex(Encode(s))+"'";
        }
        Snapshot Next(const Request& r,const Snapshot& current){
            auto after=current;
            ++after.revision;
            switch(r.action){
                case Action::ToEstate:if(current.location!=Location::Inventory)throw Failure{
                    Error::Conflict
                };
                after.location=Location::Stored;
                after.estate_id=r.estate_id;
                after.inventory_slot=0;
                break;
                case Action::Place:if(current.location!=Location::Stored||current.estate_id!=r.estate_id)throw Failure{
                    Error::Conflict
                };
                after.location=Location::Placed;
                after.transform=r.transform;
                after.bounds_provenance=r.bounds->provenance;
                break;
                case Action::MovePlaced:if(current.location!=Location::Placed||current.estate_id!=r.estate_id)throw Failure{
                    Error::Conflict
                };
                after.transform=r.transform;
                after.bounds_provenance=r.bounds->provenance;
                break;
                case Action::Store:if(current.location!=Location::Placed||current.estate_id!=r.estate_id)throw Failure{
                    Error::Conflict
                };
                after.location=Location::Stored;
                after.transform={
                };
                after.bounds_provenance.clear();
                break;
                case Action::ReturnInventory:if(current.location==Location::Inventory||current.estate_id!=r.estate_id)throw Failure{
                    Error::Conflict
                };
                after.location=Location::Inventory;
                after.estate_id=0;
                after.inventory_slot=r.destination_slot;
                after.transform={
                };
                after.bounds_provenance.clear();
                break;
            }
            return after;
        }
        template<class T,class F>Result<T> Run(Database& db,F f){
            try{
                Tx tx(db);
                VerifyDB(db);
                auto v=f();
                tx.Commit();
                return {
                    v,Error::None
                };
            }
            catch(const Failure& e){
                return {
                    {
                    },e.error
                };
            }
            catch(const std::exception&){
                return {
                    {
                    },Error::DatabaseFailure
                };
            }
        }
    }
    Service::Service(std::unique_ptr<Database> d,bool e):database_(std::move(d)),enabled_(e){
    }
    Service::Service(std::unique_ptr<Database> d,bool e,IsolatedFixture):database_(std::move(d)),enabled_(e),fixture_(true){
    }
    Service::~Service()=default;
    Result<Receipt> Service::Operate(const Request&){
        return {
            {
            },enabled_?Error::UnsupportedFunding:Error::Disabled
        };
    }
    Result<Snapshot> Service::SeedIsolatedInventory(AuthenticatedPurchaser actor,const Identity& identity,uint32_t slot){
        std::lock_guard<std::mutex> lock(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!fixture_)return {
            {
            },Error::UnsupportedFunding
        };
        if(!ValidIdentity(identity)||identity.original_owner_character_id!=actor.character_id||!actor.account_id||!slot)return {
            {
            },Error::InvalidArgument
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Snapshot>(*database_,[&]{
            auto& db=*database_;Actor(db,actor);Snapshot s{
                identity,1,Location::Inventory,0,slot,{
                },{
                }
            };auto old=Q(db,"SELECT "+columns+" FROM housing_fixture_item_custody WHERE instance_key="+Quote(db,identity.instance_key)+" FOR UPDATE");if(old.RowCount()){
                if(old.RowCount()!=1)throw Failure{
                    Error::DatabaseFailure
                };auto existing=Row(old.begin(),identity.instance_key);if(Encode(existing)!=Encode(s))throw Failure{
                    Error::Conflict
                };return existing;
            }
            auto occupied=Q(db,"SELECT instance_key FROM housing_fixture_item_custody WHERE holder_character_id="+std::to_string(actor.character_id)+" AND inventory_slot="+std::to_string(slot)+" FOR UPDATE");if(occupied.RowCount())throw Failure{
                Error::Conflict
            };Q(db,"INSERT INTO housing_fixture_item_custody SET instance_key="+Quote(db,identity.instance_key)+",owner_character_id="+std::to_string(actor.character_id)+",catalog_item_id="+std::to_string(identity.catalog_item_id)+","+LocationSQL(s));return s;
        });
    }
    Result<Snapshot> Service::ReadIsolatedItem(AuthenticatedPurchaser actor,const std::string& key){
        std::lock_guard<std::mutex> lock(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!fixture_)return {
            {
            },Error::UnsupportedFunding
        };
        if(!Key(key)||!actor.character_id||!actor.account_id)return {
            {
            },Error::InvalidArgument
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Snapshot>(*database_,[&]{
            auto& db=*database_;auto initial=Q(db,"SELECT "+columns+" FROM housing_fixture_item_custody WHERE instance_key="+Quote(db,key));if(initial.RowCount()!=1)throw Failure{
                Error::NotFound
            };auto previous=Row(initial.begin(),key);if(previous.estate_id)EstateOwner(db,previous.estate_id,actor);Actor(db,actor);auto rows=Q(db,"SELECT "+columns+" FROM housing_fixture_item_custody WHERE instance_key="+Quote(db,key)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{
                Error::NotFound
            };auto s=Row(rows.begin(),key);if(s.identity.original_owner_character_id!=actor.character_id)throw Failure{
                Error::Unauthorized
            };if(s.estate_id!=previous.estate_id)throw Failure{
                Error::Conflict
            };return s;
        });
    }
    Result<Receipt> Service::OperateIsolated(const Request& r){
        std::lock_guard<std::mutex> lock(mutex_);
        if(!enabled_)return {
            {
            },Error::Disabled
        };
        if(!fixture_)return {
            {
            },Error::UnsupportedFunding
        };
        if(!Key(r.request_key)||!r.estate_id||!r.actor.character_id||!r.actor.account_id||!ValidSnapshot(r.expected)||r.expected.revision==std::numeric_limits<uint64_t>::max()||r.expected.identity.original_owner_character_id!=r.actor.character_id||uint8_t(r.action)>4)return {
            {
            },Error::InvalidArgument
        };
        try{
            if(r.action==Action::Place||r.action==Action::MovePlaced)Geometry(r);
            else if(r.bounds||!DefaultPose(r.transform))return {
                {
                },Error::InvalidArgument
            };
            if((r.action==Action::ReturnInventory)!=bool(r.destination_slot))return {
                {
                },Error::InvalidArgument
            };
        }
        catch(const Failure& e){
            return {
                {
                },e.error
            };
        }
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {
            },Error::DatabaseFailure
        };
        return Run<Receipt>(*database_,[&]{
            auto& db=*database_;auto key=Quote(db,r.request_key);auto binding=Binding(r);Q(db,"INSERT INTO housing_fixture_item_receipts(request_key,character_id,account_id,estate_id,binding_blob) VALUES ("+key+","+std::to_string(r.actor.character_id)+","+std::to_string(r.actor.account_id)+","+std::to_string(r.estate_id)+",x'"+Hex(binding)+"') ON DUPLICATE KEY UPDATE request_key=request_key");auto receipt=Q(db,"SELECT character_id,account_id,estate_id,HEX(binding_blob),completed,HEX(after_blob) FROM housing_fixture_item_receipts WHERE request_key="+key+" FOR UPDATE");EstateOwner(db,r.estate_id,r.actor);Actor(db,r.actor);if(receipt.RowCount()!=1)throw Failure{
                Error::DatabaseFailure
            };auto rr=receipt.begin();if(std::stoul(rr[0])!=r.actor.character_id||std::stoul(rr[1])!=r.actor.account_id)throw Failure{
                Error::Unauthorized
            };if(std::stoul(rr[2])!=r.estate_id||Unhex(rr[3])!=binding)throw Failure{
                Error::Conflict
            };if(std::stoul(rr[4])==1){
                auto after=Decode(Unhex(rr[5]));if(Encode(after)!=Encode(Next(r,r.expected)))throw Failure{
                    Error::DatabaseFailure
                };return Receipt{
                    after,true
                };
            }
            auto rows=Q(db,"SELECT "+columns+" FROM housing_fixture_item_custody WHERE instance_key="+Quote(db,r.expected.identity.instance_key)+" FOR UPDATE");if(rows.RowCount()!=1)throw Failure{
                Error::NotFound
            };auto current=Row(rows.begin(),r.expected.identity.instance_key);if(Encode(current)!=Encode(r.expected))throw Failure{
                Error::Conflict
            };auto after=Next(r,current);if(r.action==Action::ReturnInventory){
                auto occupied=Q(db,"SELECT instance_key FROM housing_fixture_item_custody WHERE holder_character_id="+std::to_string(r.actor.character_id)+" AND inventory_slot="+std::to_string(r.destination_slot)+" FOR UPDATE");if(occupied.RowCount())throw Failure{
                    Error::Conflict
                };
            }
            auto moved=Q(db,"UPDATE housing_fixture_item_custody SET "+LocationSQL(after)+" WHERE instance_key="+Quote(db,current.identity.instance_key)+" AND revision="+std::to_string(current.revision)+" AND record_blob=x'"+Hex(Encode(current))+"'");if(moved.RowsAffected()!=1)throw Failure{
                Error::Conflict
            };auto saved=Q(db,"UPDATE housing_fixture_item_receipts SET completed=1,after_blob=x'"+Hex(Encode(after))+"' WHERE request_key="+key+" AND completed=0");if(saved.RowsAffected()!=1)throw Failure{
                Error::DatabaseFailure
            };return Receipt{
                after,false
            };
        });
    }
}
