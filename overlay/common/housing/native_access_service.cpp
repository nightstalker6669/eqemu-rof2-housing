#include "runtime_activation_db.h"
#include "native_access_service.h"
#include "catalog_service.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <type_traits>
namespace Housing::NativeAccess {
    namespace {
        using Bytes=std::vector<uint8_t>;
        struct Failure {
            Error error;
        };
        MySQLRequestResult Query(Database& db,const std::string& sql)
        {
            auto result=db.QueryDatabase(sql,false);
            if(!result.Success())throw Failure{
                Error::DatabaseFailure
            };
            return result;
        }
        std::string Quote(Database& db,const std::string& value){
            return "'"+db.Escape(value)+"'";
        }
        std::string Fold(std::string value){
            for(char& c:value)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
            return value;
        }
        bool Key(const std::string& value)
        {
            if(value.empty()||value.size()>96)return false;
            for(unsigned char c:value)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c==':'||c=='.'))return false;
            return true;
        }
        uint32_t Number(const char* value)
        {
            if(!value)throw Failure{
                Error::DatabaseFailure
            };
            const auto n=std::stoull(value);
            if(n>UINT32_MAX)throw Failure{
                Error::DatabaseFailure
            };
            return uint32_t(n);
        }
        std::string Hex(const Bytes& bytes)
        {
            static constexpr char digits[]="0123456789ABCDEF";
            std::string out;
            for(auto value:bytes){
                out+=digits[value>>4];
                out+=digits[value&15];
            }
            return out;
        }
        Bytes Unhex(const char* text)
        {
            if(!text)throw Failure{
                Error::DatabaseFailure
            };
            const std::string value=text;
            if(value.size()%2||value.size()>2*NeighborhoodWire::MaximumPayload+1024)throw Failure{
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
            for(size_t at=0;at<value.size();at+=2)out.push_back((nibble(value[at])<<4)|nibble(value[at+1]));
            return out;
        }
        void Word(Bytes& bytes,uint64_t value,size_t size){
            for(size_t i=0;i<size;++i)bytes.push_back(uint8_t(value>>(8*i)));
        }
        void Text(Bytes& bytes,const std::string& value){
            Word(bytes,value.size(),4);
            bytes.insert(bytes.end(),value.begin(),value.end());
        }
        void Blob(Bytes& bytes,const Bytes& value){
            Word(bytes,value.size(),4);
            bytes.insert(bytes.end(),value.begin(),value.end());
        }
        struct Reader {
            const Bytes& bytes;
            size_t at=0;
            uint64_t Word(size_t size){
                if(size>bytes.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                uint64_t out=0;
                for(size_t i=0;i<size;++i)out|=uint64_t(bytes[at++])<<(8*i);
                return out;
            }
            Bytes Blob(){
                const auto size=Word(4);
                if(size>NeighborhoodWire::MaximumPayload||size>bytes.size()-at)throw Failure{
                    Error::DatabaseFailure
                };
                Bytes out(bytes.begin()+at,bytes.begin()+at+size);
                at+=size;
                return out;
            }
        };
        class Transaction {
            Database& db_;
            bool active_=true;
            public:
            explicit Transaction(Database& db):db_(db){
                Query(db_,"START TRANSACTION");
            }
            ~Transaction(){
                if(active_)try{
                    db_.QueryDatabase(std::string("ROLLBACK"),false);
                }
                catch(...) {}
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
        template<class T,class F>Result<T> Run(Database& db,F body)
        {
            try{
                Transaction transaction(db);
                auto value=body();
                static_assert(std::is_nothrow_move_constructible_v<Result<T>>);
                Result<T> result{
                    std::move(value),Error::None
                };
                transaction.Commit();
                return result;
            }
            catch(const Failure& failure){
                return {
                    {},failure.error
                };
            }
            catch(...){
                return {
                    {},Error::DatabaseFailure
                };
            }
        }
        void Verify(Database& db)
        {
            auto result=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_estates','housing_acl_state','housing_acl_groups','housing_acl_members','housing_acl_special_members','housing_native_acl_receipts','character_data','account','guild_members','guilds','housing_native_estates','housing_catalogs','housing_neighborhoods','housing_plot_definitions','instance_list','housing_live_item_leases')");
            if(result.RowCount()!=1||Number(result.begin()[0])!=16)throw Failure{
                Error::DatabaseFailure
            };
        }
        bool LeaseShape(const LiveItems::Lease& lease)
        {
            return AccessPolicy::PositiveIdentity(lease.actor.character_id)&&AccessPolicy::PositiveIdentity(lease.actor.account_id)&&lease.epoch&&Key(lease.session_key);
        }
        void Authenticate(Database& db,const LiveItems::Lease& lease,bool no_pending)
        {
            auto actor=Query(db,"SELECT c.account_id,c.deleted_at,a.status FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(lease.actor.character_id)+" FOR UPDATE");
            if(actor.RowCount()!=1||Number(actor.begin()[0])!=lease.actor.account_id||actor.begin()[1]||!actor.begin()[2]||std::stoll(actor.begin()[2])<0)throw Failure{
                Error::Unauthorized
            };
            auto current=Query(db,"SELECT account_id,epoch,session_key,HEX(token),active,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(lease.actor.character_id)+" FOR UPDATE");
            if(current.RowCount()!=1)throw Failure{
                Error::StaleLease
            };
            auto row=current.begin();
            if(Number(row[0])!=lease.actor.account_id||std::stoull(row[1])!=lease.epoch||std::string(row[2])!=lease.session_key||Unhex(row[3])!=Bytes(lease.token.begin(),lease.token.end())||Number(row[4])!=1)throw Failure{
                Error::StaleLease
            };
            if(no_pending&&row[5])throw Failure{
                Error::PendingInventory
            };
        }
        std::string Character(Database& db,uint32_t id)
        {
            auto character=Query(db,"SELECT c.name,c.deleted_at,c.account_id,a.id FROM character_data c LEFT JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(id)+" FOR UPDATE");
            if(character.RowCount()!=1||!character.begin()[0]||character.begin()[1]||!character.begin()[3]||!Number(character.begin()[2]))throw Failure{
                Error::InvalidArgument
            };
            std::string name=character.begin()[0];
            if(!PermissionWire::Detail::ValidMember({
                name,1
            })||name[0]=='_')throw Failure{
                Error::InvalidArgument
            };
            return name;
        }
        struct Context {
            Snapshot snapshot;
            AccessPolicy::ResolvedPlot plot;
            AccessPolicy::TrustedActor actor;
            std::string owner_handle;
        };
        const EstateWire::Definition& Definition(const Catalog::Bundle& bundle,uint32_t id)
        {
            const auto found=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& value){
                return value.id==id;
            });
            if(found==bundle.definitions.end())throw Failure{
                Error::InvalidArgument
            };
            return *found;
        }
        Context Plot(Database& db,const LiveItems::Lease& lease,EstateId estate,uint32_t native,LiveItems::InventorySessionFence& fence,const InteriorSourcePolicy* interior=nullptr)
        {
            auto domain=Query(db,"SELECT owner_kind,owner_id,plot_id FROM housing_current_estates WHERE id="+std::to_string(estate)+" FOR UPDATE");
            if(domain.RowCount()!=1)throw Failure{
                Error::NotFound
            };
            auto dr=domain.begin();
            const auto kind=Number(dr[0]),owner=Number(dr[1]),plot=Number(dr[2]);
            if(!AccessPolicy::PositiveIdentity(owner)||(kind!=uint32_t(OwnerKind::Character)&&kind!=uint32_t(OwnerKind::Guild)))throw Failure{
                Error::Unauthorized
            };
            auto nodes=Query(db,"SELECT catalog_key,definition_id,parent_native_id,neighborhood_id FROM housing_native_estates WHERE native_id="+std::to_string(native)+" AND active=1 AND ownership_estate_id="+std::to_string(estate)+" AND plot_id="+std::to_string(plot)+" FOR UPDATE");
            if(nodes.RowCount()!=1)throw Failure{
                Error::NotFound
            };
            auto nr=nodes.begin();
            const std::string key=nr[0];
            const auto definition_id=Number(nr[1]),parent=Number(nr[2]),neighborhood=Number(nr[3]);
            auto catalog=Query(db,"SELECT origin,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,key)+" FOR UPDATE");
            if(catalog.RowCount()!=1||Number(catalog.begin()[1])!=20130510)throw Failure{
                Error::InvalidArgument
            };
            auto cr=catalog.begin();
            if(Number(cr[0])==2){
                auto database=Query(db,"SELECT DATABASE()");
                const std::string name=database.begin()[0];
                if(!Housing::RuntimeActivation::AllowsReconstructedContent(db))throw Failure{
                    Error::Unauthorized
                };
            }
            else if(Number(cr[0])!=1)throw Failure{
                Error::InvalidArgument
            };
            const auto bundle=Catalog::DecodePayloads(Unhex(cr[2]),Unhex(cr[3]));
            const auto& definition=Definition(bundle,definition_id);
            if((kind==uint32_t(OwnerKind::Character)&&definition.type!=4)||(kind==uint32_t(OwnerKind::Guild)&&definition.type!=8))throw Failure{
                Error::InvalidArgument
            };
            auto root=Query(db,"SELECT definition_id FROM housing_native_estates WHERE native_id="+std::to_string(parent)+" AND active=1 AND parent_native_id IS NULL AND catalog_key="+Quote(db,key)+" AND neighborhood_id="+std::to_string(neighborhood)+" FOR UPDATE");
            if(root.RowCount()!=1)throw Failure{
                Error::InvalidArgument
            };
            const auto& root_definition=Definition(bundle,Number(root.begin()[0]));
            if(root_definition.type!=5||std::find(root_definition.children.begin(),root_definition.children.end(),definition.id)==root_definition.children.end())throw Failure{
                Error::InvalidArgument
            };
            auto route=Query(db,"SELECT h.zone_id,h.zone_version,h.instance_id,i.zone,i.version,i.never_expires,p.neighborhood_id FROM housing_neighborhoods h JOIN instance_list i ON i.id=h.instance_id JOIN housing_plot_definitions p ON p.id="+std::to_string(plot)+" WHERE h.id="+std::to_string(neighborhood)+" FOR UPDATE");
            if(route.RowCount()!=1)throw Failure{
                Error::UnheldFence
            };
            auto rr=route.begin();
            const auto instance=Number(rr[2]);
            if(!instance||instance>32000||Number(rr[0])!=Number(rr[3])||Number(rr[1])!=Number(rr[4])||Number(rr[5])!=1||Number(rr[6])!=neighborhood||root_definition.zone_id!=Number(rr[0])||(!fence.PlacementContextMatches(estate,native,uint16_t(instance))&&(!interior||!interior->ParentMatchesUnderTransaction(db,lease,fence,estate,native,uint16_t(instance)))))throw Failure{
                Error::UnheldFence
            };
            Context out;
            out.snapshot.estate_id=estate;
            out.snapshot.native_plot_id=native;
            out.snapshot.wire.estate_id=native;
            out.plot.estate_id=native;
            out.plot.type=kind==uint32_t(OwnerKind::Character)?AccessPolicy::PlotType::Personal:AccessPolicy::PlotType::Guild;
            if(kind==uint32_t(OwnerKind::Character)){
                out.plot.owner_character_id=owner;
                out.owner_handle=Character(db,owner);
            }
            else {
                out.plot.guild_id=owner;
                auto guild=Query(db,"SELECT name FROM guilds WHERE id="+std::to_string(owner)+" FOR UPDATE");
                if(guild.RowCount()!=1||!guild.begin()[0])throw Failure{
                    Error::Unauthorized
                };
                out.owner_handle=guild.begin()[0];
            }
            out.actor.character_id=lease.actor.character_id;
            out.actor.authenticated=out.actor.feature_allowed=true;
            auto membership=Query(db,"SELECT guild_id,`rank` FROM guild_members WHERE char_id="+std::to_string(lease.actor.character_id)+" FOR UPDATE");
            if(membership.RowCount()>1)throw Failure{
                Error::DatabaseFailure
            };
            if(membership.RowCount()){
                out.actor.guild_id=Number(membership.begin()[0]);
                const auto rank=AccessPolicy::NativeGuildRankFromServer(Number(membership.begin()[1]));
                if(!rank)throw Failure{
                    Error::Unauthorized
                };
                out.actor.guild_rank=*rank;
                // A stale rank1 row must not bypass ACL after leadership transfer.
                auto current_guild=Query(db,"SELECT leader FROM guilds WHERE id="+std::to_string(out.actor.guild_id)+" FOR UPDATE");
                if(current_guild.RowCount()!=1||(out.actor.guild_rank==1&&Number(current_guild.begin()[0])!=lease.actor.character_id))throw Failure{Error::Unauthorized};
            }
            return out;
        }
        using SubjectKey=std::pair<uint8_t,uint32_t>;
        SubjectKey Subject(const AccessPolicy::Subject& value){
            return {
                uint8_t(value.kind),value.identity
            };
        }
        struct Loaded {
            AccessPolicy::Acl acl;
            std::map<uint32_t,std::string> names;
        };
        Loaded Load(Database& db,Context& context)
        {
            const auto estate=std::to_string(context.snapshot.estate_id);
            Loaded out;
            auto state=Query(db,"SELECT revision FROM housing_acl_state WHERE estate_id="+estate+" FOR UPDATE");
            if(state.RowCount()>1)throw Failure{
                Error::DatabaseFailure
            };
            if(state.RowCount()){
                context.snapshot.revision=std::stoull(state.begin()[0]);
                if(!context.snapshot.revision)throw Failure{
                    Error::DatabaseFailure
                };
            }
            auto groups=Query(db,"SELECT group_id,capabilities FROM housing_acl_groups WHERE estate_id="+estate+" ORDER BY group_id FOR UPDATE");
            if(groups.RowCount()!=(state.RowCount()?7:0))throw Failure{
                Error::DatabaseFailure
            };
            uint32_t expected=0;
            for(auto row=groups.begin();row.operator!=(groups.end());++row){
                const auto group=Number(row[0]);
                if(group!=expected++||group>6)throw Failure{
                    Error::DatabaseFailure
                };
                out.acl.masks[group]=Number(row[1]);
            }
            auto members=Query(db,"SELECT character_id,group_id FROM housing_acl_members WHERE estate_id="+estate+" ORDER BY character_id FOR UPDATE");
            if(members.RowCount()>4096||(!state.RowCount()&&members.RowCount()))throw Failure{
                Error::DatabaseFailure
            };
            for(auto row=members.begin();row.operator!=(members.end());++row){
                const auto id=Number(row[0]),group=Number(row[1]);
                if(group>5)throw Failure{
                    Error::DatabaseFailure
                };
                auto name=Character(db,id);
                if(Fold(name)==Fold(context.owner_handle))throw Failure{
                    Error::DatabaseFailure
                };
                out.names.emplace(id,name);
                out.acl.assignments.push_back({
                    {
                        AccessPolicy::SubjectKind::Character,id
                    },uint8_t(group)
                });
            }
            auto special=Query(db,"SELECT subject_kind,subject_id,group_id FROM housing_acl_special_members WHERE estate_id="+estate+" ORDER BY subject_kind,subject_id FOR UPDATE");
            if(special.RowCount()>9||(!state.RowCount()&&special.RowCount()))throw Failure{
                Error::DatabaseFailure
            };
            for(auto row=special.begin();row.operator!=(special.end());++row){
                const auto kind=Number(row[0]),group=Number(row[2]);
                if((kind!=1&&kind!=2)||group>5)throw Failure{
                    Error::DatabaseFailure
                };
                out.acl.assignments.push_back({
                    {
                        kind==1?AccessPolicy::SubjectKind::GuildRank:AccessPolicy::SubjectKind::Fellowship,Number(row[1])
                    },uint8_t(group)
                });
            }
            if(!AccessPolicy::ValidAcl(out.acl))throw Failure{
                Error::DatabaseFailure
            };
            return out;
        }
        void Populate(Context& context,const Loaded& loaded)
        {
            context.snapshot.wire.masks=loaded.acl.masks;
            context.snapshot.wire.members.clear();
            for(const auto& assignment:loaded.acl.assignments){
                const auto reserved=AccessPolicy::ReservedWireKey(assignment.subject);
                const auto name=reserved?*reserved:loaded.names.at(assignment.subject.identity);
                context.snapshot.wire.members.push_back({
                    name,assignment.group
                });
            }
            const auto effective=AccessPolicy::Resolve(context.plot,context.actor,&loaded.acl);
            if(!effective)throw Failure{
                Error::Unauthorized
            };
            context.snapshot.actor_access=*effective;
        }
        Loaded Proposal(Database& db,const PermissionWire::Snapshot& proposal,const Context& context)
        {
            Loaded out;
            out.acl.masks=proposal.masks;
            std::set<SubjectKey> unique;
            for(const auto& member:proposal.members){
                AccessPolicy::Subject subject;
                const auto key=Fold(member.name);
                if(key=="_fellowship")subject={
                    AccessPolicy::SubjectKind::Fellowship,0
                };
                else if(key.size()==7&&key.rfind("_guild",0)==0&&key[6]>='1'&&key[6]<='8')subject={
                    AccessPolicy::SubjectKind::GuildRank,uint32_t(key[6]-'0')
                };
                else {
                    if(member.name[0]=='_')throw Failure{
                        Error::InvalidArgument
                    };
                    auto rows=Query(db,"SELECT id FROM character_data WHERE LOWER(name)="+Quote(db,key)+" ORDER BY id FOR UPDATE");
                    if(rows.RowCount()!=1)throw Failure{
                        Error::InvalidArgument
                    };
                    subject={
                        AccessPolicy::SubjectKind::Character,Number(rows.begin()[0])
                    };
                    const auto name=Character(db,subject.identity);
                    if(Fold(name)==Fold(context.owner_handle))throw Failure{
                        Error::InvalidArgument
                    };
                    out.names.emplace(subject.identity,name);
                }
                if(!unique.insert(Subject(subject)).second)throw Failure{
                    Error::InvalidArgument
                };
                out.acl.assignments.push_back({
                    subject,uint8_t(member.group)
                });
            }
            if(!AccessPolicy::ValidAcl(out.acl))throw Failure{
                Error::InvalidArgument
            };
            std::sort(out.acl.assignments.begin(),out.acl.assignments.end(),[](const auto& a,const auto& b){
                return Subject(a.subject)<Subject(b.subject);
            });
            return out;
        }
        std::map<SubjectKey,uint8_t> Assignments(const AccessPolicy::Acl& acl){
            std::map<SubjectKey,uint8_t> out;
            for(const auto& value:acl.assignments)out.emplace(Subject(value.subject),value.group);
            return out;
        }
        void AuthorizeDiff(const Loaded& old,const Loaded& proposed,const AccessPolicy::EffectiveAccess& actor)
        {
            if(!actor.Allows(AccessPolicy::Capability::Manage))throw Failure{
                Error::Unauthorized
            };
            for(uint8_t group=0;group<7;++group){
                const auto changed=old.acl.masks[group]^proposed.acl.masks[group];
                if(changed&&!actor.implicit_owner&&(group>=actor.group||(changed&~actor.capabilities)))throw Failure{
                    Error::Unauthorized
                };
            }
            const auto before=Assignments(old.acl),after=Assignments(proposed.acl);
            for(const auto& [subject,group]:after){
                const auto found=before.find(subject);
                const bool character=subject.first==uint8_t(AccessPolicy::SubjectKind::Character);
                if(found==before.end()){
                    if((character&&group==0)||group>actor.group)throw Failure{
                        character&&group==0?Error::InvalidArgument:Error::Unauthorized
                    };
                }
                else if(found->second!=group){
                    if((character&&group==0)||found->second>=actor.group||group>actor.group)throw Failure{
                        character&&group==0?Error::InvalidArgument:Error::Unauthorized
                    };
                }
            }
            for(const auto& [subject,group]:before)if(!after.count(subject)&&group>=actor.group)throw Failure{
                Error::Unauthorized
            };
        }
        Bytes Binding(const Request& request)
        {
            Bytes bytes;
            Word(bytes,1,4);
            Word(bytes,request.lease.actor.character_id,4);
            Word(bytes,request.lease.actor.account_id,4);
            Word(bytes,request.lease.epoch,8);
            Text(bytes,request.lease.session_key);
            bytes.insert(bytes.end(),request.lease.token.begin(),request.lease.token.end());
            Word(bytes,request.estate_id,4);
            Word(bytes,request.expected_revision,8);
            Blob(bytes,PermissionWire::EncodeSnapshot(request.proposal));
            return bytes;
        }
        Bytes Outcome(const Snapshot& snapshot)
        {
            Bytes bytes;
            Word(bytes,1,4);
            Word(bytes,snapshot.estate_id,4);
            Word(bytes,snapshot.native_plot_id,4);
            Word(bytes,snapshot.revision,8);
            Word(bytes,snapshot.actor_access.group,1);
            Word(bytes,snapshot.actor_access.capabilities,4);
            Word(bytes,snapshot.actor_access.implicit_owner,1);
            Blob(bytes,PermissionWire::EncodeSnapshot(snapshot.wire));
            return bytes;
        }
        Snapshot DecodeOutcome(const Bytes& bytes)
        {
            Reader reader{
                bytes
            };
            if(reader.Word(4)!=1)throw Failure{
                Error::DatabaseFailure
            };
            Snapshot out;
            out.estate_id=uint32_t(reader.Word(4));
            out.native_plot_id=uint32_t(reader.Word(4));
            out.revision=reader.Word(8);
            out.actor_access.group=uint8_t(reader.Word(1));
            out.actor_access.capabilities=uint32_t(reader.Word(4));
            const auto owner=reader.Word(1);
            if(owner>1||out.actor_access.group>6||(out.actor_access.capabilities&~AccessPolicy::KnownCapabilities))throw Failure{
                Error::DatabaseFailure
            };
            out.actor_access.implicit_owner=bool(owner);
            const auto wire=reader.Blob();
            const auto decoded=PermissionWire::DecodeEdit(wire.data(),wire.size());
            if(!decoded||decoded->estate_id!=out.native_plot_id||decoded->masks[6]||reader.at!=bytes.size())throw Failure{
                Error::DatabaseFailure
            };
            out.wire=*decoded;
            if(out.actor_access.implicit_owner){
                if(out.actor_access.group!=6||out.actor_access.capabilities!=AccessPolicy::KnownCapabilities)throw Failure{
                    Error::DatabaseFailure
                };
            }
            else if(out.actor_access.group==6||out.actor_access.capabilities!=out.wire.masks[out.actor_access.group])throw Failure{
                Error::DatabaseFailure
            };
            return out;
        }
        std::optional<Receipt> Lookup(Database& db,const Request& request,const Bytes& binding)
        {
            auto rows=Query(db,"SELECT character_id,account_id,lease_epoch,session_key,HEX(lease_token),estate_id,native_plot_id,expected_revision,committed_revision,HEX(binding_blob),HEX(outcome_blob) FROM housing_native_acl_receipts WHERE request_key="+Quote(db,request.request_key));
            if(!rows.RowCount())return {};
            if(rows.RowCount()!=1)throw Failure{
                Error::DatabaseFailure
            };
            auto row=rows.begin();
            if(Number(row[0])!=request.lease.actor.character_id||Number(row[1])!=request.lease.actor.account_id||std::stoull(row[2])!=request.lease.epoch||std::string(row[3])!=request.lease.session_key||Unhex(row[4])!=Bytes(request.lease.token.begin(),request.lease.token.end())||Number(row[5])!=request.estate_id||Number(row[6])!=request.proposal.estate_id||std::stoull(row[7])!=request.expected_revision||Unhex(row[9])!=binding)throw Failure{
                Error::Conflict
            };
            auto outcome=DecodeOutcome(Unhex(row[10]));
            if(outcome.estate_id!=request.estate_id||outcome.native_plot_id!=request.proposal.estate_id||outcome.revision!=std::stoull(row[8]))throw Failure{
                Error::DatabaseFailure
            };
            return Receipt{
                request.request_key,std::move(outcome),true
            };
        }
        bool RequestShape(const Request& request)
        {
            if(!Key(request.request_key)||!LeaseShape(request.lease)||!AccessPolicy::PositiveIdentity(request.estate_id)||request.expected_revision==UINT64_MAX||request.proposal.masks[6]||request.proposal.members.size()>4096)return false;
            try{
                PermissionWire::EncodeSnapshot(request.proposal);
                return true;
            }
            catch(...){
                return false;
            }
        }
        void Save(Database& db,const Context& context,const Loaded& proposed,uint64_t expected)
        {
            const auto estate=std::to_string(context.snapshot.estate_id);
            if(!expected){
                auto state=Query(db,"INSERT INTO housing_acl_state(estate_id,revision) VALUES ("+estate+",1)");
                if(state.RowsAffected()!=1)throw Failure{
                    Error::Conflict
                };
            }
            else {
                auto state=Query(db,"UPDATE housing_acl_state SET revision=revision+1 WHERE estate_id="+estate+" AND revision="+std::to_string(expected));
                if(state.RowsAffected()!=1)throw Failure{
                    Error::Conflict
                };
            }
            Query(db,"DELETE FROM housing_acl_groups WHERE estate_id="+estate);
            Query(db,"DELETE FROM housing_acl_members WHERE estate_id="+estate);
            Query(db,"DELETE FROM housing_acl_special_members WHERE estate_id="+estate);
            for(uint32_t group=0;group<7;++group)Query(db,"INSERT INTO housing_acl_groups(estate_id,group_id,capabilities) VALUES ("+estate+","+std::to_string(group)+","+std::to_string(proposed.acl.masks[group])+")");
            for(const auto& value:proposed.acl.assignments){
                if(value.subject.kind==AccessPolicy::SubjectKind::Character)Query(db,"INSERT INTO housing_acl_members(estate_id,character_id,group_id) VALUES ("+estate+","+std::to_string(value.subject.identity)+","+std::to_string(value.group)+")");
                else Query(db,"INSERT INTO housing_acl_special_members(estate_id,subject_kind,subject_id,group_id) VALUES ("+estate+","+std::to_string(value.subject.kind==AccessPolicy::SubjectKind::GuildRank?1:2)+","+std::to_string(value.subject.identity)+","+std::to_string(value.group)+")");
            }
        }
        void Journal(Database& db,const Request& request,const Bytes& binding,const Snapshot& result)
        {
            auto inserted=Query(db,"INSERT INTO housing_native_acl_receipts(request_key,character_id,account_id,lease_epoch,session_key,lease_token,estate_id,native_plot_id,expected_revision,committed_revision,binding_blob,outcome_blob) VALUES ("+Quote(db,request.request_key)+","+std::to_string(request.lease.actor.character_id)+","+std::to_string(request.lease.actor.account_id)+","+std::to_string(request.lease.epoch)+","+Quote(db,request.lease.session_key)+",x'"+Hex(Bytes(request.lease.token.begin(),request.lease.token.end()))+"',"+std::to_string(request.estate_id)+","+std::to_string(request.proposal.estate_id)+","+std::to_string(request.expected_revision)+","+std::to_string(result.revision)+",x'"+Hex(binding)+"',x'"+Hex(Outcome(result))+"')");
            if(inserted.RowsAffected()!=1)throw Failure{
                Error::DatabaseFailure
            };
        }
    }
    Service::Service(std::unique_ptr<Database> db,bool enabled,const InteriorSourcePolicy* interior):database_(std::move(db)),enabled_(enabled),interior_(interior){}
    Result<Snapshot> Service::Read(const LiveItems::Lease& lease,EstateId estate,uint32_t native,LiveItems::InventorySessionFence& fence)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {},Error::Disabled
        };
        if(!LeaseShape(lease)||!AccessPolicy::PositiveIdentity(estate)||!AccessPolicy::PositiveIdentity(native))return {
            {},Error::InvalidArgument
        };
        if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))return {
            {},Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {},Error::DatabaseFailure
        };
        return Run<Snapshot>(*database_,[&]{
            auto& db=*database_;
            Verify(db);
            auto domain_lock=Query(db,"SELECT id FROM housing_current_estates WHERE id="+std::to_string(estate)+" FOR UPDATE");
            if(domain_lock.RowCount()!=1)throw Failure{
                Error::NotFound
            };
            Authenticate(db,lease,true);
            auto context=Plot(db,lease,estate,native,fence,interior_);
            const auto loaded=Load(db,context);
            Populate(context,loaded);
            if(!context.snapshot.actor_access.Allows(AccessPolicy::Capability::Manage))throw Failure{
                Error::Unauthorized
            };
            return context.snapshot;
        });
    }
    Result<bool> Service::SetVotingOptOut(const LiveItems::Lease& lease,EstateId estate,uint32_t native,bool opted_out,LiveItems::InventorySessionFence& fence)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {{},Error::Disabled};
        if(!LeaseShape(lease)||!AccessPolicy::PositiveIdentity(estate)||!AccessPolicy::PositiveIdentity(native))return {{},Error::InvalidArgument};
        if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))return {{},Error::UnheldFence};
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
        return Run<bool>(*database_,[&]{
            auto& db=*database_;Verify(db);
            auto storage=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_estate_preferences' AND engine='InnoDB'");
            if(storage.RowCount()!=1||Number(storage.begin()[0])!=1)throw Failure{Error::DatabaseFailure};
            auto domain=Query(db,"SELECT id FROM housing_current_estates WHERE id="+std::to_string(estate)+" FOR UPDATE");
            if(domain.RowCount()!=1)throw Failure{Error::NotFound};
            Authenticate(db,lease,true);auto context=Plot(db,lease,estate,native,fence);
            Populate(context,Load(db,context));
            if(!context.snapshot.actor_access.Allows(AccessPolicy::Capability::Manage))throw Failure{Error::Unauthorized};
            Query(db,"INSERT INTO housing_estate_preferences(estate_id,voting_opted_out) VALUES ("+std::to_string(estate)+","+std::to_string(opted_out?1:0)+") ON DUPLICATE KEY UPDATE voting_opted_out=VALUES(voting_opted_out)");
            if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))throw Failure{Error::UnheldFence};
            return opted_out;
        });
    }
    Result<Receipt> Service::Replace(const Request& request,LiveItems::InventorySessionFence& fence)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {},Error::Disabled
        };
        if(!RequestShape(request))return {
            {},Error::InvalidArgument
        };
        if(!fence.HeldFor(request.lease.actor,request.lease.epoch,request.lease.session_key))return {
            {},Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {},Error::DatabaseFailure
        };
        const auto binding=Binding(request);
        return Run<Receipt>(*database_,[&]{
            auto& db=*database_;
            Verify(db);
            if(auto old=Lookup(db,request,binding)){
                Authenticate(db,request.lease,false);
                return *old;
            }
            auto domain_lock=Query(db,"SELECT id FROM housing_current_estates WHERE id="+std::to_string(request.estate_id)+" FOR UPDATE");
            if(domain_lock.RowCount()!=1)throw Failure{
                Error::NotFound
            };
            Authenticate(db,request.lease,true);
            auto context=Plot(db,request.lease,request.estate_id,request.proposal.estate_id,fence,interior_);
            const auto current=Load(db,context);
            Populate(context,current);
            if(context.snapshot.revision!=request.expected_revision)throw Failure{
                Error::Conflict
            };
            const auto proposed=Proposal(db,request.proposal,context);
            AuthorizeDiff(current,proposed,context.snapshot.actor_access);
            if(current.acl.masks!=proposed.acl.masks||Assignments(current.acl)!=Assignments(proposed.acl)){
                Save(db,context,proposed,request.expected_revision);
                context.snapshot.revision=request.expected_revision+1;
            }
            Populate(context,proposed);
            Journal(db,request,binding,context.snapshot);
            return Receipt{
                request.request_key,std::move(context.snapshot),false
            };
        });
    }
    Result<Receipt> Service::Resolve(const Request& request,LiveItems::InventorySessionFence& fence)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if(!enabled_)return {
            {},Error::Disabled
        };
        if(!RequestShape(request))return {
            {},Error::InvalidArgument
        };
        if(!fence.HeldFor(request.lease.actor,request.lease.epoch,request.lease.session_key))return {
            {},Error::UnheldFence
        };
        if(!database_||database_->GetStatus()!=DBcore::Connected)return {
            {},Error::DatabaseFailure
        };
        const auto binding=Binding(request);
        return Run<Receipt>(*database_,[&]{
            auto& db=*database_;
            Verify(db);
            Authenticate(db,request.lease,false);
            const auto old=Lookup(db,request,binding);
            if(!old)throw Failure{
                Error::NotFound
            };
            return *old;
        });
    }
}
