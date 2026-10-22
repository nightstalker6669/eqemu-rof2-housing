#include "runtime_activation_db.h"
#include "house_interior_allocator.h"
#include "catalog_content_db.h"
#include "current_estate_access.h"
#include "crate_house_rules.h"
#include "crate_property_limit.h"
#include "../database.h"
namespace Housing::HouseInteriors {
namespace {
struct Failure{Error error;};
[[noreturn]] void Bad(Error e){throw Failure{e};}
MySQLRequestResult Q(Database& db,const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())Bad(Error::DatabaseFailure);return result;}
uint64_t N(const char* p,uint64_t max=UINT64_MAX){if(!p||!*p)Bad(Error::DatabaseFailure);std::string s(p);if(s.find_first_not_of("0123456789")!=std::string::npos)Bad(Error::DatabaseFailure);auto value=std::stoull(s);if(value>max)Bad(Error::DatabaseFailure);return value;}
std::string Quote(Database& db,const std::string& s){return "'"+db.Escape(s)+"'";}
std::vector<uint8_t> Unhex(const char* p){if(!p)Bad(Error::DatabaseFailure);std::string s(p);if(s.size()%2||s.size()>NeighborhoodWire::MaximumPayload*2)Bad(Error::DatabaseFailure);auto digit=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;Bad(Error::DatabaseFailure);};std::vector<uint8_t> out;for(size_t i=0;i<s.size();i+=2)out.push_back((digit(s[i])<<4)|digit(s[i+1]));return out;}
}
Result<HouseBinding::Interior> AllocateUnderTransaction(Database& db,const HouseBinding::Binding& before,
 const LiveItems::Request& request,LiveItems::InventorySessionFence& fence,const ContentBinding::HouseRoute& route,
 const AllocationPolicy& policy,const Options& options){
 try {
  const auto parent=LiveItems::Service::VerifyHouseAllocationParent(db,request,fence);
  if(!parent||!parent.value)Bad(Error::Unauthorized);
  if(!before.house_id||!before.revision||before.item_template_id!=request.expected.values.item_id||!before.pending_key.empty()||before.location.kind!=HouseBinding::Location::Kind::Inventory||before.location.character_id!=request.lease.actor.character_id||before.location.slot_id!=request.expected.values.slot_id||before.location.custody_id)Bad(Error::Conflict);
  if((request.house->house_id&&(request.house->house_id!=before.house_id||request.house->expected_revision!=before.revision))||(!request.house->house_id&&(before.revision!=1||before.interior)))Bad(Error::Conflict);
  auto tables=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_house_instances','housing_house_locations','housing_house_interiors','housing_native_estates','housing_catalogs')");if(tables.RowCount()!=1||N(tables.begin()[0])!=5)Bad(Error::DatabaseFailure);
  auto ledger=Q(db,"SELECT h.revision,h.item_template_id,h.created_by_character_id,h.pending_request_key,l.location_kind,l.character_id,l.slot_id,l.custody_id FROM housing_house_instances h JOIN housing_house_locations l ON l.house_id=h.house_id WHERE h.house_id="+std::to_string(before.house_id)+" FOR UPDATE");
  if(ledger.RowCount()!=1)Bad(Error::Conflict);auto l=ledger.begin();
  if(N(l[0])!=before.revision||N(l[1])!=before.item_template_id||N(l[2])!=request.lease.actor.character_id||l[3]||N(l[4])!=1||N(l[5])!=before.location.character_id||N(l[6])!=before.location.slot_id||l[7])Bad(Error::Conflict);
  const auto& reservation=request.placement->reservation;
  auto access=LoadCurrentEstateAccessUnderTransaction(db,request.estate_id,reservation.native_estate_id,request.lease.actor);
  if(!access||!access.value.effective.Allows(AccessPolicy::Capability::AddHouse))Bad(Error::Unauthorized);
  auto mapping=Q(db,"SELECT n.definition_id,n.neighborhood_id,n.catalog_key,h.zone_id,h.zone_version,h.instance_id,i.zone,i.version,i.never_expires FROM housing_native_estates n JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN instance_list i ON i.id=h.instance_id WHERE n.native_id="+std::to_string(reservation.native_estate_id)+" AND n.active=1 AND n.ownership_estate_id="+std::to_string(request.estate_id)+" FOR UPDATE");
  if(mapping.RowCount()!=1)Bad(Error::Conflict);auto m=mapping.begin();
  if(!m[2]||std::string(m[2])!=reservation.catalog_key||N(m[3])!=N(m[6])||N(m[4])!=N(m[7])||N(m[8])!=1||!fence.PlacementContextMatches(request.estate_id,reservation.native_estate_id,uint16_t(N(m[5],32000))))Bad(Error::Unauthorized);
  auto catalog=Q(db,"SELECT revision,origin,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,reservation.catalog_key)+" FOR UPDATE");
  if(catalog.RowCount()!=1||N(catalog.begin()[2])!=20130510)Bad(Error::Conflict);auto c=catalog.begin();const auto revision=N(c[0]);if(!revision)Bad(Error::Conflict);
  if(N(c[1])==2){auto current=Q(db,"SELECT DATABASE()");std::string name=current.begin()[0]?current.begin()[0]:"";if(!Housing::RuntimeActivation::AllowsReconstructedContent(db))Bad(Error::Unauthorized);}else if(N(c[1])!=1)Bad(Error::Unauthorized);
  auto bundle=Catalog::DecodePayloads(Unhex(c[3]),Unhex(c[4]));
  const EstateWire::Definition *plot=nullptr,*house=nullptr;
  for(const auto& d:bundle.definitions){if(d.id==N(m[0]))plot=&d;if(d.id==route.definition_id)house=&d;}
  if(!plot||!house||house->type!=(HouseTemplates::Type(before.item_template_id))||house->zone_id!=route.zone_id||!plot->max_children||std::find(plot->children.begin(),plot->children.end(),house->id)==plot->children.end())Bad(Error::InvalidRoute);
  const auto identity=ContentBinding::Current(db,reservation.catalog_key,bundle,reservation.native_estate_id,{uint32_t(N(m[3],UINT32_MAX)),uint8_t(N(m[4],UINT8_MAX)),uint16_t(N(m[5],32000))},before.item_template_id,&route);
  if(!ContentBinding::Same(identity,*reservation.content_binding)||!policy.ReviewedUnderTransaction(db,fence,before,request,route))Bad(Error::Unauthorized);
  auto old=Q(db,"SELECT native_estate_id,definition_id,zone_id,instance_id,zone_version,catalog_key,catalog_revision FROM housing_house_interiors WHERE house_id="+std::to_string(before.house_id)+" FOR UPDATE");
  if(old.RowCount()>1||bool(old.RowCount())!=bool(before.interior))Bad(Error::Conflict);
  const auto node_key="house:"+std::to_string(before.house_id);
  if(before.interior){
   if(CrateHouseRules::LockedHouse(db,before.house_id).placed_crate&&!CratePropertyLimit::AvailableUnderTransaction(db,request.estate_id))Bad(Error::Conflict);
   const auto& retained=*before.interior;auto o=old.begin();
   if(N(o[0])!=retained.native_estate_id||N(o[1])!=retained.definition_id||N(o[2])!=retained.zone_id||N(o[3])!=retained.instance_id||N(o[4])!=retained.zone_version||!o[5]||std::string(o[5])!=retained.catalog_key||N(o[6])!=retained.catalog_revision)Bad(Error::Conflict);
   if(retained.definition_id!=route.definition_id||retained.zone_id!=route.zone_id||retained.zone_version!=route.zone_version)Bad(Error::InvalidRoute);
   const bool catalog_move=retained.catalog_key!=reservation.catalog_key||retained.catalog_revision!=revision;
   if(catalog_move){
    if(!policy.CatalogRelocationReviewedUnderTransaction(db,fence,before,request,route))Bad(Error::Unauthorized);
    auto old_catalog=Q(db,"SELECT revision,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,retained.catalog_key)+" FOR UPDATE");
    if(old_catalog.RowCount()!=1||N(old_catalog.begin()[0])!=retained.catalog_revision||N(old_catalog.begin()[1])!=20130510)Bad(Error::Conflict);
    auto old_bundle=Catalog::DecodePayloads(Unhex(old_catalog.begin()[2]),Unhex(old_catalog.begin()[3]));
    const EstateWire::Definition* old_definition=nullptr;for(const auto& d:old_bundle.definitions)if(d.id==retained.definition_id)old_definition=&d;
    if(!old_definition||EstateWire::EncodeBootstrap({*old_definition},{})!=EstateWire::EncodeBootstrap({*house},{}))Bad(Error::InvalidRoute);
   }
   auto facts=Q(db,"SELECT n.parent_native_id,n.definition_id,n.active,n.catalog_key,i.zone,i.version,i.never_expires FROM housing_native_estates n JOIN instance_list i ON i.id="+std::to_string(retained.instance_id)+" WHERE n.native_id="+std::to_string(retained.native_estate_id)+" FOR UPDATE");
   if(facts.RowCount()!=1)Bad(Error::Conflict);auto f=facts.begin();
   if(!N(f[0],UINT32_MAX)||N(f[1])!=retained.definition_id||N(f[2])!=1||!f[3]||std::string(f[3])!=retained.catalog_key||N(f[4])!=retained.zone_id||N(f[5])!=retained.zone_version||N(f[6])!=1)Bad(Error::Conflict);
   if(N(f[0])!=reservation.native_estate_id||catalog_move){
    const auto old_parent=uint32_t(N(f[0],UINT32_MAX));
    if(old_parent!=reservation.native_estate_id&&!policy.ReparentReviewedUnderTransaction(db,fence,before,request,route,old_parent))Bad(Error::Unauthorized);
    auto origin=Q(db,"SELECT n.native_id FROM housing_native_estates n JOIN housing_native_estates root ON root.native_id=n.parent_native_id AND root.parent_native_id IS NULL AND root.catalog_key=n.catalog_key AND root.neighborhood_id=n.neighborhood_id WHERE n.plot_id IS NOT NULL AND n.native_id="+std::to_string(old_parent)+" AND n.catalog_key="+Quote(db,retained.catalog_key)+" FOR UPDATE");
    if(origin.RowCount()!=1)Bad(Error::Conflict);
    auto children=Q(db,"SELECT n.native_id FROM housing_native_estates n JOIN housing_house_interiors i ON i.native_estate_id=n.native_id JOIN housing_house_locations l ON l.house_id=i.house_id AND l.location_kind=2 JOIN housing_live_placed_items p ON p.custody_id=l.custody_id WHERE n.parent_native_id="+std::to_string(reservation.native_estate_id)+" AND n.active=1 AND n.native_id<>"+std::to_string(retained.native_estate_id)+" AND NOT EXISTS (SELECT 1 FROM housing_live_item_storage st WHERE st.custody_id=p.custody_id AND st.stowed=1) AND NOT EXISTS (SELECT 1 FROM housing_crate_members cm WHERE cm.custody_id=p.custody_id) ORDER BY n.native_id FOR UPDATE");
    // Relocating a retained house must not consume the shared crate child budget.
    if(!request.initial_stowed&&children.RowCount()>=1)Bad(Error::InstanceCapacity);
    auto changed=Q(db,"UPDATE housing_native_estates SET catalog_key="+Quote(db,reservation.catalog_key)+",parent_native_id="+std::to_string(reservation.native_estate_id)+",neighborhood_id="+std::to_string(N(m[1],UINT32_MAX))+" WHERE native_id="+std::to_string(retained.native_estate_id)+" AND parent_native_id="+std::to_string(old_parent)+" AND catalog_key="+Quote(db,retained.catalog_key)+" AND definition_id="+std::to_string(retained.definition_id)+" AND active=1");
    if(changed.RowsAffected()!=1)Bad(Error::Conflict);
   }
   auto rebound=retained;rebound.catalog_key=reservation.catalog_key;rebound.catalog_revision=revision;return {rebound,Error::None};
  }
  if(Q(db,"SELECT native_id FROM housing_native_estates WHERE catalog_key="+Quote(db,reservation.catalog_key)+" AND node_key="+Quote(db,node_key)+" FOR UPDATE").RowCount())Bad(Error::Conflict);
  auto instance=AllocatePermanentInstanceUnderTransaction(db,options,route.zone_id,route.zone_version);if(!instance)Bad(instance.error);
  auto created=Q(db,"INSERT INTO housing_native_estates(catalog_key,node_key,parent_native_id,neighborhood_id,definition_id,static_id,active) VALUES ("+Quote(db,reservation.catalog_key)+","+Quote(db,node_key)+","+std::to_string(reservation.native_estate_id)+","+std::to_string(N(m[1],UINT32_MAX))+","+std::to_string(route.definition_id)+",0,1)");
  if(created.RowsAffected()!=1||!created.LastInsertedID()||created.LastInsertedID()>=UINT32_MAX)Bad(Error::InstanceCapacity);
  return {HouseBinding::Interior{uint32_t(created.LastInsertedID()),route.definition_id,route.zone_id,instance.value,route.zone_version,reservation.catalog_key,revision},Error::None};
 }catch(const Failure& failure){return Result<HouseBinding::Interior>::Failure(failure.error);}catch(...){return Result<HouseBinding::Interior>::Failure(Error::DatabaseFailure);}
}
}
