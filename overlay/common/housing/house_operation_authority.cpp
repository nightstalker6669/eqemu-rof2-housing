#include "runtime_activation_db.h"
#include "house_operation_authority.h"
#include "catalog_service.h"
#include "../database.h"
#include <algorithm>
#include <limits>
namespace Housing::HouseAuthority {
namespace {
struct Failure { Error error; };
MySQLRequestResult Q(Database& db, const std::string& sql) {
    auto r = db.QueryDatabase(sql, false);
    if (!r.Success()) throw Failure{Error::DatabaseFailure};
    return r;
}
uint64_t N(const char* s, uint64_t max = UINT32_MAX) {
    if (!s || !*s) throw Failure{Error::DatabaseFailure};
    std::string t(s); if (t.find_first_not_of("0123456789") != std::string::npos) throw Failure{Error::DatabaseFailure};
    auto n = std::stoull(t); if (n > max) throw Failure{Error::DatabaseFailure}; return n;
}
std::vector<uint8_t> Bytes(const char* s) {
    if (!s) throw Failure{Error::DatabaseFailure};
    std::string t(s); if (t.size() > 8000000 || t.size() % 2) throw Failure{Error::DatabaseFailure};
    auto digit=[](char c)->uint8_t { if(c>='0'&&c<='9')return c-'0'; if(c>='A'&&c<='F')return c-'A'+10; if(c>='a'&&c<='f')return c-'a'+10; throw Failure{Error::DatabaseFailure}; };
    std::vector<uint8_t> out; out.reserve(t.size()/2);
    for(size_t i=0;i<t.size();i+=2)out.push_back((digit(t[i])<<4)|digit(t[i+1])); return out;
}
struct Node { uint32_t parent=0, definition=0, neighborhood=0, plot=0, estate=0; std::string catalog; };
Node ReadNode(Database& db, uint32_t id) {
    auto r=Q(db,"SELECT parent_native_id,definition_id,neighborhood_id,plot_id,ownership_estate_id,HEX(catalog_key) FROM housing_native_estates WHERE native_id="+std::to_string(id)+" AND active=1 FOR UPDATE");
    if(r.RowCount()!=1)throw Failure{Error::InvalidRoute}; auto row=r.begin(); auto bytes=Bytes(row[5]);
    Node n; n.parent=row[0]?N(row[0]):0; n.definition=N(row[1]); n.neighborhood=N(row[2]); n.plot=row[3]?N(row[3]):0; n.estate=row[4]?N(row[4]):0; n.catalog.assign(bytes.begin(),bytes.end());
    if(!n.definition||!n.neighborhood||n.catalog.empty()||n.catalog.size()>96)throw Failure{Error::InvalidRoute}; return n;
}
const EstateWire::Definition& Definition(const Catalog::Bundle& b,uint32_t id) {
    const EstateWire::Definition* found=nullptr;
    for(const auto& d:b.definitions)if(d.id==id){if(found)throw Failure{Error::InvalidRoute};found=&d;}
    if(!found)throw Failure{Error::InvalidRoute};return *found;
}
void ExistingCreator(Database& db,uint32_t id) {
    if(!AccessPolicy::PositiveIdentity(id))throw Failure{Error::Unauthorized};
    auto c=Q(db,"SELECT c.id FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(id)+" AND c.deleted_at IS NULL FOR UPDATE");
    if(c.RowCount()!=1)throw Failure{Error::Unauthorized};
}
}
Result<Decision> AuthorizeUnderTransaction(Database& db,const Request& request,const TrustedWitness& witness) {
    if((request.operation!=Operation::Add&&request.operation!=Operation::Remove)||!request.estate_id
        ||!AccessPolicy::PositiveIdentity(request.native_plot_id)||!AccessPolicy::PositiveIdentity(request.item_template_id)
        ||(request.native_house_id&&!AccessPolicy::PositiveIdentity(request.native_house_id))
        ||(request.operation==Operation::Remove&&(!request.native_house_id||!request.house_id)))return {{},Error::InvalidArgument};
    try {
        auto access=LoadCurrentEstateAccessUnderTransaction(db,request.estate_id,request.native_plot_id,request.actor);
        if(!access)return {{},access.error};
        auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_catalogs','housing_neighborhoods','housing_plot_definitions','items','instance_list')");
        if(engines.RowCount()!=1||N(engines.begin()[0])!=5)throw Failure{Error::DatabaseFailure};
        LockedContext ctx;ctx.request=request;ctx.access=access.value;
        auto plot=ReadNode(db,request.native_plot_id);
        if(plot.estate!=request.estate_id||plot.plot!=access.value.plot_id||!plot.parent)throw Failure{Error::InvalidRoute};
        auto root=ReadNode(db,plot.parent);
        if(root.parent||root.plot||root.estate||root.catalog!=plot.catalog||root.neighborhood!=plot.neighborhood)throw Failure{Error::InvalidRoute};
        auto route=Q(db,"SELECT n.zone_id,n.zone_version,n.instance_id,i.zone,i.version,i.never_expires FROM housing_neighborhoods n JOIN instance_list i ON i.id=n.instance_id JOIN housing_plot_definitions p ON p.neighborhood_id=n.id WHERE n.id="+std::to_string(plot.neighborhood)+" AND p.id="+std::to_string(plot.plot)+" FOR UPDATE");
        if(route.RowCount()!=1)throw Failure{Error::InvalidRoute};auto r=route.begin();
        ctx.neighborhood_id=plot.neighborhood;ctx.zone_id=N(r[0]);ctx.zone_version=N(r[1],UINT8_MAX);ctx.instance_id=N(r[2],32000);
        if(!ctx.zone_id||!ctx.instance_id||N(r[3])!=ctx.zone_id||N(r[4])!=ctx.zone_version||N(r[5])!=1)throw Failure{Error::InvalidRoute};
        ctx.catalog_key=plot.catalog;
        auto catalog=Q(db,"SELECT revision,origin,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key='"+db.Escape(plot.catalog)+"' AND OCTET_LENGTH(definitions_blob)<=4000000 AND OCTET_LENGTH(regions_blob)<=4000000 FOR UPDATE");
        if(catalog.RowCount()!=1)throw Failure{Error::InvalidRoute};auto c=catalog.begin();ctx.catalog_revision=N(c[0],UINT64_MAX);
        if(!ctx.catalog_revision||N(c[2])!=20130510)throw Failure{Error::InvalidRoute};
        auto origin=N(c[1]);if(origin==2){auto name=Q(db,"SELECT DATABASE()");if(name.RowCount()!=1||!name.begin()[0])throw Failure{Error::DatabaseFailure};std::string n=name.begin()[0];if(!Housing::RuntimeActivation::AllowsReconstructedContent(db))throw Failure{Error::Unauthorized};}else if(origin!=1)throw Failure{Error::InvalidRoute};
        auto decoded=Catalog::DecodePayloads(Bytes(c[3]),Bytes(c[4]));
        ctx.plot_definition=Definition(decoded,plot.definition);
        const auto& root_definition=Definition(decoded,root.definition);
        const uint32_t plot_type=access.value.owner_kind==OwnerKind::Guild?8:4;
        if(ctx.plot_definition.type!=plot_type||ctx.plot_definition.zone_id!=ctx.zone_id||root_definition.type!=5||root_definition.zone_id!=ctx.zone_id)throw Failure{Error::InvalidRoute};
        if(std::find(root_definition.children.begin(),root_definition.children.end(),ctx.plot_definition.id)==root_definition.children.end())throw Failure{Error::InvalidRoute};
        auto item=Q(db,"SELECT UNK224,UNK227 FROM items WHERE id="+std::to_string(request.item_template_id)+" FOR UPDATE");
        if(item.RowCount()!=1)throw Failure{Error::NotFound};ctx.item_placement_mask=N(item.begin()[0]);auto definition_id=N(item.begin()[1]);
        ctx.house_definition=Definition(decoded,definition_id);
        if(ctx.house_definition.type!=(plot_type==8?2:3)||!(ctx.item_placement_mask&(plot_type==8?0x80:0x08)))throw Failure{Error::InvalidRoute};
        if(std::find(ctx.plot_definition.children.begin(),ctx.plot_definition.children.end(),ctx.house_definition.id)==ctx.plot_definition.children.end())throw Failure{Error::InvalidRoute};
        if(request.native_house_id){auto house=ReadNode(db,request.native_house_id);if(house.parent!=request.native_plot_id||house.definition!=definition_id||house.catalog!=plot.catalog||house.neighborhood!=plot.neighborhood||house.plot||house.estate)throw Failure{Error::InvalidRoute};}
        // Loader validates current rank; a corrupt/stale rank1 row must not turn
        // a guild's DIFFERENT current leader into an implicit owner.
        if(access.value.owner_kind==OwnerKind::Guild&&access.value.effective.implicit_owner){auto leader=Q(db,"SELECT leader FROM guilds WHERE id="+std::to_string(access.value.owner_id)+" FOR UPDATE");if(leader.RowCount()!=1||N(leader.begin()[0])!=request.actor.character_id)throw Failure{Error::Unauthorized};}
        auto proof=witness.VerifyUnderTransaction(db,ctx);if(!proof)throw Failure{Error::InvalidRoute};
        ExistingCreator(db,proof->creator_character_id);
        Decision decision;decision.context=ctx;decision.creator_character_id=proof->creator_character_id;
        if(request.operation==Operation::Add){if(!ctx.access.effective.Allows(AccessPolicy::Capability::AddHouse))throw Failure{Error::Unauthorized};}
        else if(proof->creator_character_id==request.actor.character_id)decision.creator_removal_bypass=true;
        else if(!ctx.access.effective.Allows(AccessPolicy::Capability::RemoveHouse)||!proof->other_creator_removal_allowed)throw Failure{Error::Unauthorized};
        return {decision,Error::None};
    } catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}
}
