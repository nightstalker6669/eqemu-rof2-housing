#ifndef EQEMU_HOUSING_HOUSE_REVIEWED_CONTENT_H
#define EQEMU_HOUSING_HOUSE_REVIEWED_CONTENT_H
#include "house_binding_service.h"
#include "custody_codec.h"
#include "inventory_projection.h"
#include <set>
#include <memory>
namespace Housing::HouseReviewedContent {
// Immutable server-selected content. Construction validates coherence, not the
// historical truth of the supplied hashes/route review. No default dataset.
struct Entry { uint32_t item_id=0; ContentBinding::HouseRoute route; };
struct Adoption { AuthenticatedPurchaser actor; LiveItems::InventoryRow source; };
// Framing only for a genuine held loader handoff. SQL and actual loaded
// pointer/session proof are required separately; this alone grants nothing.
inline bool GuidOnlyLoadPostimage(const LiveItems::InventoryRow& before,const LiveItems::InventoryRow& after,int32_t serial){
 if(serial<=0||!CustodyCodec::Valid(before)||!CustodyCodec::Valid(after)||
  (after.null_fields&LiveItems::NullGuid)||after.values.guid!=uint64_t(serial))return false;
 auto expected=before;expected.values.guid=uint64_t(serial);expected.null_fields&=~LiveItems::NullGuid;
 return CustodyCodec::Encode(expected)==CustodyCodec::Encode(after);
}
class Policies final : public HouseBinding::HouseTemplatePolicy,
                       public HouseBinding::InteriorRoutePolicy {
public:
 explicit Policies(std::vector<Entry> entries,std::vector<Adoption> grants={})
 : entries_(std::move(entries)),grants_(std::move(grants)) {
  std::set<uint32_t> items;
  for(const auto& e:entries_){
   const auto definition=Definition(e.item_id);const auto& r=e.route;
   if(!definition||!items.insert(e.item_id).second||r.definition_id!=definition||
      !r.reviewed_server_route||r.zone_version||!OriginalInteriors::Matches(r.zone_id,r.zone_short_name)||
      ((HouseTemplates::Type(e.item_id)==2)!=(r.zone_id==345||r.zone_id==737||r.zone_id==738||r.zone_id==751))||
      (e.item_id!=61823&&!HouseTemplates::Matches(e.item_id,r.definition_id,r.zone_id))||
      !ContentBinding::Detail::Hash(r.client_asset_sha256)||!ContentBinding::Detail::Hash(r.collision_map_sha256)||
      !ContentBinding::Detail::Hash(r.content_manifest_sha256))throw std::invalid_argument("Invalid reviewed house content");
   for(const auto& point:{r.spawn,r.exit}){
    for(float value:point)if(!std::isfinite(value))throw std::invalid_argument("Invalid house route point");
    if(point[3]<0||point[3]>512)throw std::invalid_argument("Invalid house route heading");
   }
  }
  std::set<std::pair<uint32_t,uint32_t>> sources;
  for(const auto& g:grants_)if(!g.actor.character_id||!g.actor.account_id||
   g.source.values.character_id!=g.actor.character_id||!VerifiedHouse(g.source)||
   !g.source.values.guid||(g.source.null_fields&LiveItems::NullGuid)||
   !sources.emplace(g.actor.character_id,g.source.values.slot_id).second)
    throw std::invalid_argument("Invalid exact house adoption grant");
 }
 const std::vector<Entry>& Entries()const noexcept{return entries_;}
 const ContentBinding::HouseRoute* Route(uint32_t item)const noexcept {
  for(const auto& entry:entries_)if(entry.item_id==item)return &entry.route;
  return nullptr;
 }
 bool VerifiedHouse(const LiveItems::InventoryRow& row)const noexcept override {
  // SQL metadata recognition only; actual template/model and memory source
  // checks remain mandatory in the custody/scene adapters under their fence.
  return Route(row.values.item_id)&&InventoryProjection::Shape(row)&&CustodyCodec::Valid(row);
 }
 bool PermitInitialBinding(AuthenticatedPurchaser actor,const LiveItems::InventoryRow& row)const noexcept override {
  try{
   if(!VerifiedHouse(row))return false;
   for(const auto& grant:grants_)if(actor.character_id==grant.actor.character_id&&actor.account_id==grant.actor.account_id&&
     CustodyCodec::Encode(row)==CustodyCodec::Encode(grant.source))return true;
  }catch(...){}
  return false;
 }
 bool VerifiedDestination(const HouseBinding::Interior& i,const EstateWire::Definition& d,uint32_t item)const noexcept override {
  const auto* route=Route(item);
  return route&&i.native_estate_id&&i.native_estate_id!=UINT32_MAX&&i.instance_id&&i.instance_id<=32000&&
   ContentBinding::Detail::Key(i.catalog_key)&&i.catalog_revision&&
   i.definition_id==route->definition_id&&d.id==route->definition_id&&d.type==(HouseTemplates::Type(item))&&
   i.zone_id==route->zone_id&&d.zone_id==route->zone_id&&i.zone_version==route->zone_version;
 }
private:
 static uint32_t Definition(uint32_t item)noexcept{
  return HouseTemplates::Definition(item);
 }
 const std::vector<Entry> entries_;
 const std::vector<Adoption> grants_;
};
// Immutable union for simultaneous selected estates. Each template has one
// reviewed route; adoption remains delegated to its exact original policy.
class PolicySet final:public HouseBinding::HouseTemplatePolicy,public HouseBinding::InteriorRoutePolicy {
 std::vector<std::shared_ptr<const Policies>> parts_;
public:
 explicit PolicySet(std::vector<std::shared_ptr<const Policies>> parts):parts_(std::move(parts)){
  if(parts_.empty())throw std::invalid_argument("Empty house policy set");std::set<uint32_t> items;
  for(const auto& p:parts_){if(!p)throw std::invalid_argument("Missing house policy");for(const auto& entry:p->Entries())if(!items.insert(entry.item_id).second)throw std::invalid_argument("Ambiguous house template route");}
 }
 bool VerifiedHouse(const LiveItems::InventoryRow& row)const noexcept override{for(const auto& p:parts_)if(p->VerifiedHouse(row))return true;return false;}
 bool PermitInitialBinding(AuthenticatedPurchaser actor,const LiveItems::InventoryRow& row)const noexcept override{for(const auto& p:parts_)if(p->PermitInitialBinding(actor,row))return true;return false;}
 bool VerifiedDestination(const HouseBinding::Interior& i,const EstateWire::Definition& d,uint32_t item)const noexcept override{for(const auto& p:parts_)if(p->VerifiedDestination(i,d,item))return true;return false;}
};

}
#endif
