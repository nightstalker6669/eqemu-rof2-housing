#include "house_custody_authority.h"
#include "custody_codec.h"
#include "../database.h"
namespace Housing::HouseCustodyAuthority {
namespace {
using namespace HouseBinding;
using Bytes=CustodyCodec::Bytes;
[[noreturn]] void Bad(){throw HouseBinding::Failure(HouseBinding::Error::DatabaseFailure);}
MySQLRequestResult Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())Bad();return r;}
uint64_t N(const char* s,uint64_t maximum=UINT64_MAX){if(!s||!*s)Bad();std::string t(s);if(t.find_first_not_of("0123456789")!=std::string::npos)Bad();auto n=std::stoull(t);if(n>maximum)Bad();return n;}
std::string Hex(const Bytes& b){const char* h="0123456789abcdef";std::string s;s.reserve(b.size()*2);for(auto x:b){s+=h[x>>4];s+=h[x&15];}return s;}
Bytes Unhex(const char* s){if(!s)Bad();std::string t(s);if(t.size()%2||t.size()>600000)Bad();auto digit=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;Bad();};Bytes b;for(size_t i=0;i<t.size();i+=2)b.push_back((digit(t[i])<<4)|digit(t[i+1]));return b;}
std::string Literal(const std::string& s){return "x'"+Hex(Bytes(s.begin(),s.end()))+"'";}
bool Key(const std::string& k){if(k.empty()||k.size()>96)return false;for(unsigned char c:k)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c==':'||c=='.'))return false;return true;}
bool Slot(uint32_t s){return (s>=23&&s<=32)||s==33;}
std::string Position(const LiveItems::InventoryRow& row){return "character_id="+std::to_string(row.values.character_id)+" AND slot_id="+std::to_string(row.values.slot_id);}
std::string Exact(const LiveItems::InventoryRow& row){
    const auto& v=row.values;auto number=[&](const char* name,uint64_t n,uint8_t null=0){return std::string(" AND ")+name+" <=> "+((row.null_fields&null)?"NULL":std::to_string(n));};
    std::string s=Position(row)+number("item_id",v.item_id,LiveItems::NullItemId)+number("charges",v.charges,LiveItems::NullCharges)+number("color",v.color);
    s+=number("augment_one",v.augment_one)+number("augment_two",v.augment_two)+number("augment_three",v.augment_three)+number("augment_four",v.augment_four)+number("augment_five",v.augment_five)+number("augment_six",v.augment_six)+number("instnodrop",v.instnodrop);
    s+=" AND BINARY custom_data <=> "+((row.null_fields&LiveItems::NullCustomData)?std::string("NULL"):Literal(v.custom_data));
    s+=number("ornament_icon",v.ornament_icon)+number("ornament_idfile",v.ornament_idfile)+" AND ornament_hero_model <=> "+std::to_string(v.ornament_hero_model)+number("guid",v.guid,LiveItems::NullGuid);return s;
}
bool InteriorEquals(const HouseBinding::Interior& a,const HouseBinding::Interior& b){return a.native_estate_id==b.native_estate_id&&a.definition_id==b.definition_id&&a.zone_id==b.zone_id&&a.instance_id==b.instance_id&&a.zone_version==b.zone_version&&a.catalog_key==b.catalog_key&&a.catalog_revision==b.catalog_revision;}
class Witness final:public HouseAuthority::TrustedWitness {
public:
    Witness(LiveItems::InventorySessionFence& f,const AuthorizationRequest& r,const ReviewedOperationPolicy& p):fence(f),request(r),policy(p){}
    std::optional<HouseAuthority::TrustedItemProof> VerifyUnderTransaction(Database& db,const HouseAuthority::LockedContext& ctx)const override {
        const auto& r=request;const auto& lease=r.lease;const auto& before=r.before;const auto actor=std::to_string(lease.actor.character_id);
        if(!lease.epoch||!Key(lease.session_key)||!CustodyCodec::Valid(r.source)||r.source.values.character_id!=lease.actor.character_id||r.source.values.item_id!=ctx.request.item_template_id||before.house_id!=ctx.request.house_id||before.item_template_id!=r.source.values.item_id||!before.revision||!before.pending_key.empty())return std::nullopt;
        if(!fence.HeldFor(lease.actor,lease.epoch,lease.session_key)||!fence.PlacementContextMatches(r.estate_id,ctx.request.native_plot_id,ctx.instance_id))return std::nullopt;
        auto current=Q(db,"SELECT account_id,epoch,HEX(session_key),HEX(token),active,pending_key IS NULL FROM housing_live_item_leases WHERE character_id="+actor+" FOR UPDATE");
        if(current.RowCount()!=1)return std::nullopt;auto l=current.begin();if(N(l[0])!=lease.actor.account_id||N(l[1])!=lease.epoch||Unhex(l[2])!=Bytes(lease.session_key.begin(),lease.session_key.end())||Unhex(l[3])!=Bytes(lease.token.begin(),lease.token.end())||N(l[4])!=1||N(l[5])!=1)return std::nullopt;
        if(r.phase!=AuthorizationPhase::ReadStored){if(!Key(r.request_key))return std::nullopt;auto parent=Q(db,"SELECT character_id,account_id,lease_epoch,HEX(session_key),completed,applied,after_blob IS NULL FROM housing_live_item_receipts WHERE request_key="+Literal(r.request_key)+" FOR UPDATE");if(parent.RowCount()!=1)return std::nullopt;auto p=parent.begin();if(N(p[0])!=lease.actor.character_id||N(p[1])!=lease.actor.account_id||N(p[2])!=lease.epoch||Unhex(p[3])!=Bytes(lease.session_key.begin(),lease.session_key.end())||N(p[4])||N(p[5])||N(p[6])!=1)return std::nullopt;}
        auto ledger=Q(db,"SELECT h.revision,h.item_template_id,h.pending_request_key IS NULL,l.location_kind,l.character_id,l.slot_id,l.custody_id FROM housing_house_instances h JOIN housing_house_locations l ON l.house_id=h.house_id WHERE h.house_id="+std::to_string(before.house_id)+" FOR UPDATE");
        if(ledger.RowCount()!=1)return std::nullopt;auto h=ledger.begin();if(N(h[0])!=before.revision||N(h[1])!=before.item_template_id||N(h[2])!=1||N(h[3])!=uint8_t(before.location.kind))return std::nullopt;
        if(before.location.kind==Location::Kind::Inventory){if(!h[4]||!h[5]||h[6]||N(h[4])!=before.location.character_id||N(h[5])!=before.location.slot_id||before.location.character_id!=lease.actor.character_id||before.location.slot_id!=r.source.values.slot_id||before.location.custody_id)return std::nullopt;}
        else if(before.location.kind==Location::Kind::Custody){if(h[4]||h[5]||!h[6]||N(h[6])!=r.custody_id||before.location.custody_id!=r.custody_id||before.location.character_id||before.location.slot_id)return std::nullopt;}else return std::nullopt;
        auto interior=Q(db,"SELECT native_estate_id,definition_id,zone_id,instance_id,zone_version,HEX(catalog_key),catalog_revision FROM housing_house_interiors WHERE house_id="+std::to_string(before.house_id)+" FOR UPDATE");
        if(interior.RowCount()>1||bool(interior.RowCount())!=before.interior.has_value())return std::nullopt;
        if(before.interior){auto i=interior.begin();const auto& v=*before.interior;auto key=Unhex(i[5]);if(N(i[0])!=v.native_estate_id||N(i[1])!=v.definition_id||N(i[2])!=v.zone_id||N(i[3])!=v.instance_id||N(i[4])!=v.zone_version||key!=Bytes(v.catalog_key.begin(),v.catalog_key.end())||N(i[6])!=v.catalog_revision)return std::nullopt;}
        const auto selected=r.first_interior?r.first_interior:before.interior;
        if(r.first_interior&&before.interior&&!InteriorEquals(*r.first_interior,*before.interior)&&(r.phase!=AuthorizationPhase::StoreAfterCustodyInsert||!SamePhysicalInterior(*r.first_interior,*before.interior)))return std::nullopt;
        if(!selected||selected->native_estate_id!=ctx.request.native_house_id||selected->definition_id!=ctx.house_definition.id||selected->zone_id!=ctx.house_definition.zone_id||selected->catalog_key!=ctx.catalog_key||!selected->instance_id||selected->catalog_revision!=ctx.catalog_revision)return std::nullopt;
        auto destination=Q(db,"SELECT zone,version,never_expires FROM instance_list WHERE id="+std::to_string(selected->instance_id)+" FOR UPDATE");if(destination.RowCount()!=1||N(destination.begin()[0])!=selected->zone_id||N(destination.begin()[1])!=selected->zone_version||N(destination.begin()[2])!=1)return std::nullopt;
        auto custody=Q(db,"SELECT estate_id,owner_character_id,HEX(row_blob),HEX(schema_descriptor) FROM housing_live_item_custody WHERE custody_id="+std::to_string(r.custody_id)+" FOR UPDATE");
        if(custody.RowCount()!=1)return std::nullopt;auto c=custody.begin();auto schema=LiveItems::Service::SchemaDescriptor();if(N(c[0])!=r.estate_id||N(c[1])!=lease.actor.character_id||Unhex(c[2])!=CustodyCodec::Encode(r.source)||Unhex(c[3])!=Bytes(schema.begin(),schema.end()))return std::nullopt;
        if(r.phase==AuthorizationPhase::StoreAfterCustodyInsert){if(before.location.kind!=Location::Kind::Inventory||r.runtime_serial<=0||!fence.SourceMatches(r.source,r.runtime_serial)||Q(db,"SELECT character_id FROM inventory WHERE "+Position(r.source)+" FOR UPDATE").RowCount())return std::nullopt;}
        else {if(before.location.kind!=Location::Kind::Custody||r.first_interior)return std::nullopt;
            if(r.phase==AuthorizationPhase::ReturnBeforeInsert||r.phase==AuthorizationPhase::ReturnAfterInsert){if(!Slot(r.destination_slot))return std::nullopt;auto dest=r.source;dest.values.slot_id=r.destination_slot;if(!fence.ReturnAllowed(dest))return std::nullopt;auto rows=Q(db,"SELECT character_id FROM inventory WHERE "+Position(dest)+" FOR UPDATE");if(r.phase==AuthorizationPhase::ReturnBeforeInsert){if(rows.RowCount())return std::nullopt;}else if(rows.RowCount()!=1||Q(db,"SELECT character_id FROM inventory WHERE "+Exact(dest)+" FOR UPDATE").RowCount()!=1)return std::nullopt;}
            else if(r.phase!=AuthorizationPhase::ReadStored)return std::nullopt;
        }
        if(!policy.VerifiedUnderTransaction(db,fence,r,ctx))return std::nullopt;
        return HouseAuthority::TrustedItemProof{r.source.values.character_id,false};
    }
private:LiveItems::InventorySessionFence& fence;const AuthorizationRequest& request;const ReviewedOperationPolicy& policy;
};
}
bool Authorizer::AllowedUnderTransaction(Database& db,LiveItems::InventorySessionFence& fence,const HouseBinding::AuthorizationRequest& r)const {
    if(!r.custody_id||!r.estate_id||!r.before.house_id||!r.source.values.item_id||!r.lease.actor.character_id)return false;
    if(r.phase!=HouseBinding::AuthorizationPhase::StoreAfterCustodyInsert&&r.phase!=HouseBinding::AuthorizationPhase::ReturnBeforeInsert&&r.phase!=HouseBinding::AuthorizationPhase::ReturnAfterInsert&&r.phase!=HouseBinding::AuthorizationPhase::ReadStored)return false;
    auto mapping=Q(db,"SELECT native_id FROM housing_native_estates WHERE ownership_estate_id="+std::to_string(r.estate_id)+" AND active=1 FOR UPDATE");if(mapping.RowCount()!=1)return false;
    const auto interior=r.first_interior?r.first_interior:r.before.interior;if(!interior)return false;
    HouseAuthority::Request request{r.phase==HouseBinding::AuthorizationPhase::StoreAfterCustodyInsert?HouseAuthority::Operation::Add:HouseAuthority::Operation::Remove,r.lease.actor,r.estate_id,uint32_t(N(mapping.begin()[0],UINT32_MAX)),r.source.values.item_id,interior->native_estate_id,r.before.house_id};
    Witness witness(fence,r,policy_);auto result=HouseAuthority::AuthorizeUnderTransaction(db,request,witness);
    if(result.error==Housing::Error::DatabaseFailure)Bad();return bool(result);
}
}
