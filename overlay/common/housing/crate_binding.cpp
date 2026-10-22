#include "runtime_activation_db.h"
#include "crate_binding.h"
#include "crate_creation_origin_db.h"
#include "crate_vault_members_db.h"
#include "crate_foreign_member_policy_db.h"
#include "crate_house_rules.h"
#include "crate_property_limit.h"
#include "interior_placement_review.h"
#include "custody_codec.h"
#include "catalog_content_db.h"
#include "inventory_row_db.h"
#include "static_furniture_registry.h"
#include "house_geometry_registry.h"
#include "house_binding_service.h"
#include "house_relocation_preflight.h"
#include "../database.h"
#include <algorithm>
#include <limits>
namespace Housing::CrateBinding {
namespace {
using Error=LiveItems::Error;
struct Failure {Error error;};
[[noreturn]] void Bad(Error error){throw Failure{error};}
MySQLRequestResult Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())Bad(Error::DatabaseFailure);return r;}
uint64_t N(const char* p,uint64_t maximum=UINT64_MAX){if(!p||!*p)Bad(Error::DatabaseFailure);std::string s(p);if(s.find_first_not_of("0123456789")!=std::string::npos)Bad(Error::DatabaseFailure);auto n=std::stoull(s);if(n>maximum)Bad(Error::DatabaseFailure);return n;}
std::string S(Database& db,const std::string& s){return "'"+db.Escape(s)+"'";}
std::vector<uint8_t> Unhex(const char* p){if(!p)Bad(Error::DatabaseFailure);std::string s(p);if(s.size()%2||s.size()>600000)Bad(Error::DatabaseFailure);auto nibble=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;Bad(Error::DatabaseFailure);};std::vector<uint8_t> out;for(size_t i=0;i<s.size();i+=2)out.push_back((nibble(s[i])<<4)|nibble(s[i+1]));return out;}
std::string Hex(const std::array<uint8_t,32>& bytes){const char* h="0123456789abcdef";std::string s;for(auto b:bytes){s+=h[b>>4];s+=h[b&15];}return s;}
float F(const char* p){uint32_t bits=uint32_t(N(p,UINT32_MAX));float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f))Bad(Error::UnsupportedItem);return f;}
}
static LiveItems::Result<Allocation> ReadCarriedState(Database&,const LiveItems::InventoryRow&,bool);
static void RelocateMembers(Database&,const Allocation&,const LiveItems::Request&,const Catalog::Bundle&,Route,LiveItems::InventorySessionFence&,HouseBinding::CustodyContext*,const InteriorContent::PlacementReview* room=nullptr);


LiveItems::Result<Binding> BindInteriorPlacedUnderTransaction(Database& db,const LiveItems::Request& request,LiveItems::InventorySessionFence& fence,const CustodyContext& policy,const InteriorContent::PlacementReview& reviewed,const Allocation* carried){
 try{
  if(!request.room||request.house||request.crate_member||request.initial_stowed||request.action!=LiveItems::Action::Store||!request.placement||request.placement->reservation.content_binding)Bad(Error::InvalidArgument);
  const auto proof=LiveItems::Service::VerifyPlacedStoreAllocationParent(db,request,fence);if(!proof||!proof.value)Bad(proof.error);
  if(!Installed(db))Bad(Error::Disabled);
  const auto& scene=reviewed.profile.context;const auto& room=reviewed.profile.room;const auto& reservation=request.placement->reservation;
  if(!scene.house||!scene.house->interior||scene.house->house_id!=request.room->house_id||scene.house->revision!=request.room->house_revision||reviewed.profile.profile_revision!=request.room->profile_revision||room.house_id!=scene.house->house_id||room.native_estate_id!=reservation.native_estate_id||scene.context.estate_id!=request.estate_id||reviewed.identity.digest!=request.room->content_digest||!scene.access.effective.Allows(AccessPolicy::Capability::AddCrate))Bad(Error::Unauthorized);
  auto scope=HouseItemScope::Query(db,"SELECT custody_id FROM housing_house_item_scopes WHERE house_id="+std::to_string(room.house_id)+" AND custody_id IN (SELECT custody_id FROM housing_live_placed_items WHERE native_item_id="+std::to_string(reservation.native_item_id)+") FOR UPDATE");if(scope.RowCount()!=1)Bad(Error::Conflict);const auto custody=N(scope.begin()[0]);
  const auto definition=CrateGeometry::TemplateDefinition(request.expected.values.item_id);
  if(!definition||reviewed.metadata.house_definition_type!=7||reviewed.metadata.furniture_only||!room.definition.max_children)Bad(Error::UnsupportedItem);
  auto child=std::find_if(room.child_definitions.begin(),room.child_definitions.end(),[&](const auto& d){return d.id==definition&&d.type==7;});if(child==room.child_definitions.end())Bad(Error::UnsupportedItem);
  auto source=Q(db,"SELECT crate_id FROM housing_crate_locations WHERE location_kind=1 AND character_id="+std::to_string(request.expected.values.character_id)+" AND slot_id="+std::to_string(request.expected.values.slot_id)+" FOR UPDATE");if(bool(source.RowCount())!=bool(carried)||bool(request.crate)!=bool(carried))Bad(Error::Conflict);
  if(carried){auto current=ReadCarriedState(db,request.expected,false);if(!current||EncodeAllocation(current.value)!=EncodeAllocation(*carried)||carried->binding.revision==UINT64_MAX||request.crate->crate_id!=carried->binding.crate_id||request.crate->expected_revision!=carried->binding.revision||carried->binding.definition_id!=definition||carried->packed)Bad(Error::Conflict);
  }
  if(carried){auto members=ValidateMembersUnderTransaction(db,*carried);if(!members)Bad(members.error);if(members.value>child->max_items)Bad(Error::Conflict);}
  if(!policy.ReviewInteriorPlacementUnderTransaction(db,fence,request,reviewed,carried))Bad(Error::Unauthorized);
  auto parent=Q(db,"SELECT n.neighborhood_id,n.parent_native_id,n.catalog_key,n.active,i.house_id FROM housing_native_estates n JOIN housing_house_interiors i ON i.native_estate_id=n.native_id WHERE n.native_id="+std::to_string(room.native_estate_id)+" FOR UPDATE");if(parent.RowCount()!=1)Bad(Error::Conflict);auto node=parent.begin();if(N(node[1])!=scene.context.native_plot_id||!node[2]||std::string(node[2])!=reservation.catalog_key||N(node[3])!=1||N(node[4])!=room.house_id)Bad(Error::Conflict);
  if(!CratePropertyLimit::AvailableUnderTransaction(db,request.estate_id,carried?carried->binding.crate_id:0))Bad(Error::Conflict);
  auto children=Q(db,"SELECT native_id FROM housing_native_estates WHERE parent_native_id="+std::to_string(room.native_estate_id)+" AND active=1 FOR UPDATE");if(children.RowCount()>=room.definition.max_children)Bad(Error::Conflict);
  const auto digest=policy.ManifestDigest();if(std::all_of(digest.begin(),digest.end(),[](auto b){return b==0;}))Bad(Error::Unauthorized);
  Binding after;
  if(carried){after=carried->binding;after.revision++;after.custody_id=custody;after.catalog_key=reservation.catalog_key;after.catalog_revision=reservation.catalog_revision;after.parent_native_estate_id=room.native_estate_id;after.estate_id=request.estate_id;
   const auto id=std::to_string(after.crate_id);
   if(Q(db,"UPDATE housing_crate_locations SET location_kind=2,character_id=NULL,slot_id=NULL,custody_id="+std::to_string(custody)+" WHERE crate_id="+id+" AND location_kind=1 AND character_id="+std::to_string(carried->inventory_character_id)+" AND slot_id="+std::to_string(carried->inventory_slot_id)).RowsAffected()!=1)Bad(Error::Conflict);
   if(Q(db,"UPDATE housing_crate_instances SET revision=revision+1,content_manifest_sha256=UNHEX('"+Hex(digest)+"') WHERE crate_id="+id+" AND revision="+std::to_string(carried->binding.revision)+" AND packed=0").RowsAffected()!=1)Bad(Error::Conflict);
   if(Q(db,"UPDATE housing_native_estates SET active=1,parent_native_id="+std::to_string(room.native_estate_id)+",neighborhood_id="+std::to_string(N(node[0],UINT32_MAX))+",catalog_key="+S(db,reservation.catalog_key)+" WHERE native_id="+std::to_string(after.native_estate_id)+" AND parent_native_id="+std::to_string(carried->binding.parent_native_estate_id)+" AND active=0").RowsAffected()!=1)Bad(Error::Conflict);
   RelocateMembers(db,*carried,request,{},room.route,fence,nullptr,&reviewed);
   Q(db,"UPDATE housing_crate_estates SET parent_native_estate_id="+std::to_string(room.native_estate_id)+",ownership_estate_id="+std::to_string(request.estate_id)+",catalog_key="+S(db,reservation.catalog_key)+",catalog_revision="+std::to_string(reservation.catalog_revision)+" WHERE crate_id="+id);
  }else{
   CrateCreationOrigin::Live(db,request.request_key);
   auto made=Q(db,"INSERT INTO housing_crate_instances(item_template_id,created_by_character_id,creation_request_key,content_manifest_sha256) VALUES("+std::to_string(request.expected.values.item_id)+","+std::to_string(request.lease.actor.character_id)+","+S(db,request.request_key)+",UNHEX('"+Hex(digest)+"'))");if(made.RowsAffected()!=1||!made.LastInsertedID())Bad(Error::Conflict);const auto id=made.LastInsertedID();
   auto native=Q(db,"INSERT INTO housing_native_estates(catalog_key,node_key,parent_native_id,neighborhood_id,definition_id,static_id,active) VALUES("+S(db,reservation.catalog_key)+","+S(db,"crate:"+std::to_string(id))+","+std::to_string(room.native_estate_id)+","+std::to_string(N(node[0],UINT32_MAX))+","+std::to_string(definition)+",0,1)");if(native.RowsAffected()!=1||!native.LastInsertedID()||native.LastInsertedID()>=UINT32_MAX)Bad(Error::Conflict);
   Q(db,"INSERT INTO housing_crate_locations(crate_id,location_kind,custody_id) VALUES("+std::to_string(id)+",2,"+std::to_string(custody)+")");
   Q(db,"INSERT INTO housing_crate_estates(crate_id,native_estate_id,parent_native_estate_id,ownership_estate_id,definition_id,catalog_key,catalog_revision) VALUES("+std::to_string(id)+","+std::to_string(native.LastInsertedID())+","+std::to_string(room.native_estate_id)+","+std::to_string(request.estate_id)+","+std::to_string(definition)+","+S(db,reservation.catalog_key)+","+std::to_string(reservation.catalog_revision)+")");
   after={id,1,custody,reservation.catalog_revision,request.expected.values.item_id,uint32_t(native.LastInsertedID()),room.native_estate_id,definition,request.estate_id,reservation.catalog_key};
  }
  return {after,Error::None};
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}

LiveItems::Result<EstateWire::Definition> ReadDefinitionUnderTransaction(Database& db,const Allocation& a){
 try{
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0])!=1)Bad(Error::InvalidArgument);CrateCreationOrigin::Validate(db,a.creation_request_key,a.created_by_character_id);EncodeAllocation(a);const auto& b=a.binding;
  auto parent=Q(db,"SELECT i.house_id,i.definition_id,i.zone_id,n.definition_id,i.zone_version,i.instance_id FROM housing_house_interiors i JOIN housing_native_estates n ON n.native_id=i.native_estate_id WHERE i.native_estate_id="+std::to_string(b.parent_native_estate_id)+" FOR UPDATE");
  Catalog::Bundle bundle;
  if(parent.RowCount()){
   if(parent.RowCount()!=1||N(parent.begin()[1])!=N(parent.begin()[3]))Bad(Error::Conflict);
   auto profile=Q(db,"SELECT revision,client_build,origin,zone_short_name,HEX(client_asset_sha256),HEX(collision_map_sha256),HEX(manifest_sha256),HEX(definitions_blob),HEX(regions_blob) FROM housing_house_room_profiles WHERE house_id="+std::to_string(N(parent.begin()[0]))+" FOR UPDATE");if(profile.RowCount()!=1||N(profile.begin()[1])!=20130510)Bad(Error::Conflict);
   auto metadata=profile.begin();for(unsigned n=0;n<9;++n)if(!metadata[n])Bad(Error::Conflict);
   bundle=Catalog::DecodePayloads(Unhex(metadata[7]),Unhex(metadata[8]));if(bundle.regions.empty()||!bundle.costs.empty())Bad(Error::Conflict);InteriorContent::Profile room;bool found=false;
   for(const auto& d:bundle.definitions)if(d.id==N(parent.begin()[1])){if(found||d.zone_id!=N(parent.begin()[2])||(d.type!=2&&d.type!=3))Bad(Error::Conflict);room.definition=d;found=true;}else room.child_definitions.push_back(d);
   if(!found||!InteriorContent::ValidChildDefinitions(room))Bad(Error::Conflict);
   if(N(parent.begin()[4])>UINT8_MAX||!N(parent.begin()[5])||N(parent.begin()[5])>32000)Bad(Error::Conflict);room.house_id=N(parent.begin()[0]);room.native_estate_id=b.parent_native_estate_id;room.route={uint32_t(N(parent.begin()[2])),uint8_t(N(parent.begin()[4])),uint16_t(N(parent.begin()[5]))};room.origin=Catalog::Origin(N(metadata[2]));room.zone_short_name=metadata[3];room.client_asset_sha256=InteriorCapacity::Detail::Lower(metadata[4]);room.collision_map_sha256=InteriorCapacity::Detail::Lower(metadata[5]);room.manifest_sha256=InteriorCapacity::Detail::Lower(metadata[6]);room.region=bundle.regions.front();room.additional_regions.assign(bundle.regions.begin()+1,bundle.regions.end());
   if(!InteriorCapacity::ApplyUnderTransaction(db,room,N(metadata[0])))Bad(Error::Conflict);bundle.definitions=InteriorContent::Definitions(room);
  }else{
   auto catalog=Q(db,"SELECT revision,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+S(db,b.catalog_key)+" FOR UPDATE");if(catalog.RowCount()!=1||N(catalog.begin()[0])!=b.catalog_revision)Bad(Error::Conflict);bundle=Catalog::DecodePayloads(Unhex(catalog.begin()[1]),Unhex(catalog.begin()[2]));
  }
  auto definition=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==b.definition_id&&d.type==7;});if(definition==bundle.definitions.end()||!definition->max_items)Bad(Error::Conflict);return {*definition,Error::None};
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}

LiveItems::Result<uint32_t> ValidateMembersUnderTransaction(Database& db,const Allocation& a){
 try{
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0])!=1)Bad(Error::InvalidArgument);
  CrateCreationOrigin::Validate(db,a.creation_request_key,a.created_by_character_id);EncodeAllocation(a);const auto& b=a.binding;
  auto ids=Q(db,"SELECT custody_id FROM housing_crate_members WHERE crate_id="+std::to_string(b.crate_id)+" ORDER BY custody_id LIMIT 1001 FOR UPDATE");
  auto vaults=CrateVault::Read(db,a,true);if(!ids.RowCount()&&vaults.empty())return {0,Error::None};if(ids.RowCount()+vaults.size()>1000)Bad(Error::UnsupportedItem);
  if(a.inventory_character_id&&a.inventory_character_id!=a.created_by_character_id)Bad(Error::Conflict);
  auto definition=ReadDefinitionUnderTransaction(db,a);if(!definition)Bad(definition.error);if(ids.RowCount()+vaults.size()>definition.value.max_items||vaults.size()>definition.value.max_non_estate_items)Bad(Error::Conflict);
  auto members=Q(db,"SELECT m.custody_id,m.revision,m.origin_native_estate_id,m.origin_house_id,HEX(m.origin_pose_blob),m.origin_stowed,c.estate_id,c.owner_character_id,c.schema_descriptor,HEX(c.row_blob),p.native_item_id,p.pose_revision,HEX(p.pose_blob),r.native_estate_id,r.estate_id,r.state,r.custody_id,s.revision,s.stowed,s.native_estate_id,r.character_id,HEX(r.source_row),r.catalog_key FROM housing_crate_members m LEFT JOIN housing_live_item_custody c ON c.custody_id=m.custody_id LEFT JOIN housing_live_placed_items p ON p.custody_id=c.custody_id LEFT JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id LEFT JOIN housing_live_item_storage s ON s.custody_id=c.custody_id WHERE m.crate_id="+std::to_string(b.crate_id)+" ORDER BY m.custody_id FOR UPDATE");
  if(members.RowCount()!=ids.RowCount())Bad(Error::Conflict);
  std::vector<CrateHouseRules::House> houses;
  for(auto row=members.begin();row.operator!=(members.end());++row){
   for(size_t i=0;i<23;++i)if(i!=3&&!row[i])Bad(Error::Conflict);
   if(!N(row[0])||N(row[0])==b.custody_id||!N(row[1])||N(row[2])!=b.parent_native_estate_id||N(row[5])>1||(!row[3]&&N(row[6])!=b.estate_id)||!N(row[7],UINT32_MAX)||std::string(row[8])!=LiveItems::Service::SchemaDescriptor()||!N(row[10],UINT32_MAX)||!N(row[11])||Unhex(row[4]).size()!=28||Unhex(row[4])!=Unhex(row[12])||N(row[13])!=b.parent_native_estate_id||N(row[14])!=N(row[6])||N(row[15])!=1||N(row[16])!=N(row[0])||!N(row[17])||N(row[18])!=1||N(row[19])!=b.parent_native_estate_id||N(row[20])!=N(row[7])||Unhex(row[9])!=Unhex(row[21])||(!row[3]&&std::string(row[22])!=b.catalog_key))Bad(Error::Conflict);
   const auto source=CustodyCodec::Decode(Unhex(row[9]));if(!CustodyCodec::Valid(source)||source.values.character_id!=N(row[7]))Bad(Error::Conflict);
   if(source.values.character_id!=a.created_by_character_id&&!CrateForeignMember::EligibleUnderTransaction(db,source))Bad(Error::UnsupportedItem);
   auto metadata=Q(db,"SELECT idfile FROM items WHERE id="+std::to_string(source.values.item_id)+" FOR UPDATE");
   if(metadata.RowCount()!=1||!metadata.begin()[0])Bad(Error::UnsupportedItem);
   const auto* furniture=StaticFurniture::Lookup(source.values.item_id,metadata.begin()[0]);
   const auto* house=HouseGeometry::Lookup(source.values.item_id,metadata.begin()[0]);
   if(!furniture&&!house)Bad(Error::UnsupportedItem);
   if(house){
    // The house remains in custody with the same permanent interior. Its room
    // objects retain their existing house scopes; packing never flattens them.
    auto binding=Q(db,"SELECT h.house_id,h.revision,h.item_template_id,h.created_by_character_id,h.pending_request_key,i.native_estate_id,i.catalog_key,i.catalog_revision,n.parent_native_id,n.active,n.catalog_key,n.definition_id,i.definition_id,instance.zone,i.zone_id,instance.version,i.zone_version,instance.never_expires FROM housing_house_locations l JOIN housing_house_instances h ON h.house_id=l.house_id JOIN housing_house_interiors i ON i.house_id=h.house_id JOIN housing_native_estates n ON n.native_id=i.native_estate_id JOIN instance_list instance ON instance.id=i.instance_id WHERE l.location_kind=2 AND l.custody_id="+std::to_string(N(row[0]))+" FOR UPDATE");
    if(binding.RowCount()!=1)Bad(Error::Conflict);auto h=binding.begin();for(size_t j=0;j<18;++j)if(j!=4&&!h[j])Bad(Error::Conflict);
    if(!N(h[0])||!N(h[1])||N(h[2])!=source.values.item_id||N(h[3])!=a.created_by_character_id||h[4]||!N(h[5],UINT32_MAX)||N(h[5])==UINT32_MAX||std::string(h[6])!=b.catalog_key||N(h[7])!=b.catalog_revision||N(h[8])!=b.parent_native_estate_id||N(h[9])!=1||std::string(h[10])!=b.catalog_key||N(h[11])!=N(h[12])||N(h[13])!=N(h[14])||N(h[15])!=N(h[16])||N(h[17])!=1)Bad(Error::Conflict);
    // A contained furnished house retains its own typed vault scope. Validate
    // all non-placeable payloads rather than flattening them into direct members.
    if(VaultStorage::Installed(db)){auto vault=VaultStorage::Items(db,uint32_t(N(row[6])),uint32_t(N(h[5])),N(h[0]),{true,true,false});auto expected=Q(db,"SELECT COUNT(*) FROM housing_live_vault_items WHERE house_id="+std::to_string(N(h[0]))+" FOR UPDATE");if(expected.RowCount()!=1||N(expected.begin()[0])!=vault.size())Bad(Error::Conflict);for(const auto& item:vault)if(item.stored.row.values.character_id!=a.created_by_character_id){auto pending=Q(db,"SELECT pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(item.stored.row.values.character_id)+" FOR UPDATE");if(pending.RowCount()&&pending.begin()[0])Bad(Error::PendingApplication);}}
    houses.push_back(CrateHouseRules::LockedHouse(db,N(h[0])));
   }
   auto scoped=HouseItemScope::Locked(db,N(row[0]));
   if(row[3]){
    if(!furniture||house||!N(row[3])||((a.inventory_character_id||a.parcel_id)?bool(scoped):(!scoped||scoped->house_id!=N(row[3]))))Bad(Error::Conflict);
    auto room=Q(db,"SELECT i.native_estate_id,t.house_id,t.content_version,HEX(t.content_digest),r.content_binding_version,r.content_binding_digest,t.source_zone,t.source_version,t.source_instance,i.zone_id,i.zone_version,i.instance_id FROM housing_house_interiors i JOIN housing_interior_placement_reservations t ON t.house_id=i.house_id JOIN housing_live_placement_reservations r ON r.native_item_id=t.native_item_id WHERE i.house_id="+std::to_string(N(row[3]))+" AND t.native_item_id="+std::to_string(N(row[10]))+" FOR UPDATE");
    if(room.RowCount()!=1)Bad(Error::Conflict);auto rr=room.begin();for(size_t j=0;j<12;++j)if(j!=4&&j!=5&&!rr[j])Bad(Error::Conflict);
    if(N(rr[0])!=b.parent_native_estate_id||N(rr[1])!=N(row[3])||N(rr[2])!=InteriorContent::Version||Unhex(rr[3]).size()!=32||rr[4]||rr[5]||N(rr[6])!=N(rr[9])||N(rr[7])!=N(rr[10])||N(rr[8])!=N(rr[11]))Bad(Error::Conflict);
   }else if(scoped)Bad(Error::Conflict);
  }
  if(!CrateHouseRules::Allowed(houses))Bad(Error::Conflict);
  return {uint32_t(members.RowCount()+vaults.size()),Error::None};
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}

LiveItems::Result<uint32_t> ReleaseInteriorMemberScopesUnderTransaction(Database& db,const Allocation& source){
 try{
  if(source.inventory_character_id||!source.binding.custody_id)Bad(Error::InvalidArgument);
  auto count=ValidateMembersUnderTransaction(db,source);if(!count)Bad(count.error);if(!count.value)return count;
  auto current=Q(db,"SELECT crate_id FROM housing_crate_locations WHERE crate_id="+std::to_string(source.binding.crate_id)+" AND location_kind=2 AND custody_id="+std::to_string(source.binding.custody_id)+" FOR UPDATE");if(current.RowCount()!=1)Bad(Error::Conflict);
  auto ordinary=Q(db,"SELECT custody_id FROM housing_crate_members WHERE crate_id="+std::to_string(source.binding.crate_id)+" FOR UPDATE");
  auto removed=Q(db,"DELETE scope FROM housing_house_item_scopes scope JOIN housing_crate_members m ON m.custody_id=scope.custody_id AND m.origin_house_id=scope.house_id WHERE m.crate_id="+std::to_string(source.binding.crate_id));if(removed.RowsAffected()!=ordinary.RowCount())Bad(Error::Conflict);
  if(VaultStorage::CrateMembersInstalled(db)){auto removed_vault=Q(db,"DELETE scope FROM housing_house_item_scopes scope JOIN housing_crate_vault_members m ON m.custody_id=scope.custody_id AND m.origin_house_id=scope.house_id WHERE m.crate_id="+std::to_string(source.binding.crate_id));if(removed_vault.RowsAffected()!=count.value-ordinary.RowCount())Bad(Error::Conflict);}return count;
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}

bool Installed(Database& db){
 auto rows=Q(db,"SELECT COUNT(*),SUM(engine='InnoDB') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_crate_instances','housing_crate_locations','housing_crate_estates','housing_crate_members')");
 if(rows.RowCount()!=1)Bad(Error::DatabaseFailure);const auto n=N(rows.begin()[0]);if(!n)return false;
 if(n!=4||!rows.begin()[1]||N(rows.begin()[1])!=4)Bad(Error::DatabaseFailure);return true;
}
static std::optional<ContentBinding::HouseRoute> RelocateContainedHouse(Database& db,uint64_t custody,const Allocation& carried,const LiveItems::Request& request,const LiveItems::InventoryRow& source,const Catalog::Bundle& bundle,LiveItems::InventorySessionFence& fence,HouseBinding::CustodyContext* policy){
 auto rows=Q(db,"SELECT h.house_id,h.revision,h.item_template_id,h.pending_request_key,i.native_estate_id,i.definition_id,i.zone_id,i.instance_id,i.zone_version,i.catalog_key,i.catalog_revision,n.parent_native_id FROM housing_house_locations l JOIN housing_house_instances h ON h.house_id=l.house_id JOIN housing_house_interiors i ON i.house_id=h.house_id JOIN housing_native_estates n ON n.native_id=i.native_estate_id WHERE l.location_kind=2 AND l.custody_id="+std::to_string(custody)+" FOR UPDATE");
 if(!rows.RowCount())return {};if(rows.RowCount()!=1||!policy)Bad(Error::UnsupportedItem);auto h=rows.begin();for(size_t i=0;i<12;++i)if(i!=3&&!h[i])Bad(Error::Conflict);
 const auto& target=request.placement->reservation;HouseBinding::Binding house;house.house_id=N(h[0]);house.revision=N(h[1]);house.item_template_id=N(h[2],UINT32_MAX);house.location={HouseBinding::Location::Kind::Custody,0,0,custody};house.interior=HouseBinding::Interior{uint32_t(N(h[4],UINT32_MAX)),uint32_t(N(h[5],UINT32_MAX)),uint32_t(N(h[6],UINT32_MAX)),uint16_t(N(h[7],32000)),uint8_t(N(h[8],255)),h[9],N(h[10])};
 if(h[3]||house.revision==UINT64_MAX||house.item_template_id!=source.values.item_id||house.interior->catalog_key!=carried.binding.catalog_key||house.interior->catalog_revision!=carried.binding.catalog_revision||N(h[11])!=carried.binding.parent_native_estate_id)Bad(Error::Conflict);
 auto access=LoadCurrentEstateAccessUnderTransaction(db,request.estate_id,target.native_estate_id,request.lease.actor);if(!access||!access.value.effective.Allows(AccessPolicy::Capability::AddHouse))Bad(Error::Unauthorized);
 auto route=policy->ReviewCrateRelocationUnderTransaction(db,fence,house,request,source);if(!route||!policy->Routes())Bad(Error::Unauthorized);
 const auto quiet=HouseRelocationPreflight::UnderTransaction(db,house,request.lease,fence);if(quiet!=LiveItems::Error::None)Bad(quiet);
 auto child=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==house.interior->definition_id;});
 const bool catalog_move=house.interior->catalog_key!=target.catalog_key||house.interior->catalog_revision!=target.catalog_revision;
 if(catalog_move){
  auto old_catalog=Q(db,"SELECT revision,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+S(db,house.interior->catalog_key)+" FOR UPDATE");
  if(old_catalog.RowCount()!=1||N(old_catalog.begin()[0])!=house.interior->catalog_revision||N(old_catalog.begin()[1])!=20130510||child==bundle.definitions.end())Bad(Error::Conflict);
  auto old_bundle=Catalog::DecodePayloads(Unhex(old_catalog.begin()[2]),Unhex(old_catalog.begin()[3]));
  const EstateWire::Definition* old_definition=nullptr;for(const auto& d:old_bundle.definitions)if(d.id==house.interior->definition_id)old_definition=&d;
  if(!old_definition||EstateWire::EncodeBootstrap({*old_definition},{})!=EstateWire::EncodeBootstrap({*child},{}))Bad(Error::UnsupportedItem);
 }
 auto destination_interior=*house.interior;destination_interior.catalog_key=target.catalog_key;destination_interior.catalog_revision=target.catalog_revision;
 auto parent=Q(db,"SELECT definition_id,neighborhood_id FROM housing_native_estates WHERE native_id="+std::to_string(target.native_estate_id)+" FOR UPDATE");if(parent.RowCount()!=1)Bad(Error::Conflict);
 auto plot=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==N(parent.begin()[0],UINT32_MAX);});
 if(child==bundle.definitions.end()||plot==bundle.definitions.end()||std::find(plot->children.begin(),plot->children.end(),child->id)==plot->children.end()||!policy->Routes()->VerifiedDestination(destination_interior,*child,house.item_template_id)||route->definition_id!=child->id||route->zone_id!=house.interior->zone_id||route->zone_version!=house.interior->zone_version)Bad(Error::UnsupportedItem);
 auto schema=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_crate_house_relocations'");if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB")Bad(Error::Disabled);
 auto native=Q(db,"UPDATE housing_native_estates SET catalog_key="+S(db,target.catalog_key)+",parent_native_id="+std::to_string(target.native_estate_id)+",neighborhood_id="+std::to_string(N(parent.begin()[1],UINT32_MAX))+" WHERE native_id="+std::to_string(house.interior->native_estate_id)+" AND parent_native_id="+std::to_string(carried.binding.parent_native_estate_id)+" AND active=1 AND catalog_key="+S(db,house.interior->catalog_key));if(native.RowsAffected()>1)Bad(Error::Conflict);
 // Same physical plot/new ownership keeps the native room parent unchanged.
 // MariaDB reports zero changed rows for that valid update; verify the locked
 // final identity rather than interpreting a no-op as a failed reparent.
 auto final_native=Q(db,"SELECT native_id FROM housing_native_estates WHERE native_id="+std::to_string(house.interior->native_estate_id)+" AND parent_native_id="+std::to_string(target.native_estate_id)+" AND catalog_key="+S(db,target.catalog_key)+" AND neighborhood_id="+std::to_string(N(parent.begin()[1],UINT32_MAX))+" AND definition_id="+std::to_string(house.interior->definition_id)+" AND active=1 FOR UPDATE");if(final_native.RowCount()!=1)Bad(Error::Conflict);
 if(catalog_move){auto interior=Q(db,"UPDATE housing_house_interiors SET catalog_key="+S(db,target.catalog_key)+",catalog_revision="+std::to_string(target.catalog_revision)+" WHERE house_id="+std::to_string(house.house_id)+" AND native_estate_id="+std::to_string(house.interior->native_estate_id)+" AND instance_id="+std::to_string(house.interior->instance_id)+" AND catalog_key="+S(db,house.interior->catalog_key)+" AND catalog_revision="+std::to_string(house.interior->catalog_revision));if(interior.RowsAffected()!=1)Bad(Error::Conflict);}
 auto changed=Q(db,"UPDATE housing_house_instances SET revision=revision+1 WHERE house_id="+std::to_string(house.house_id)+" AND revision="+std::to_string(house.revision)+" AND pending_request_key IS NULL");if(changed.RowsAffected()!=1)Bad(Error::Conflict);
 auto record=Q(db,"INSERT INTO housing_crate_house_relocations(request_key,house_id,crate_id,before_revision,after_revision,before_parent_native_id,after_parent_native_id,interior_native_id,interior_instance_id,before_ownership_estate_id,after_ownership_estate_id) VALUES("+S(db,request.request_key)+","+std::to_string(house.house_id)+","+std::to_string(carried.binding.crate_id)+","+std::to_string(house.revision)+","+std::to_string(house.revision+1)+","+std::to_string(carried.binding.parent_native_estate_id)+","+std::to_string(target.native_estate_id)+","+std::to_string(house.interior->native_estate_id)+","+std::to_string(house.interior->instance_id)+","+std::to_string(carried.binding.estate_id)+","+std::to_string(request.estate_id)+")");if(record.RowsAffected()!=1)Bad(Error::Conflict);
 return route;
}

// The carried source was structurally validated before entry. The parent Store
// owns every mutation and rollback; historical reservation/content bytes remain.
static void RelocateMembers(Database& db,const Allocation& carried,const LiveItems::Request& request,const Catalog::Bundle& bundle,Route destination,LiveItems::InventorySessionFence& fence,HouseBinding::CustodyContext* houses,const InteriorContent::PlacementReview* room){
 const auto& before=carried.binding;const auto& target=request.placement->reservation;
 CrateVault::Relocate(db,carried,request,room);
 auto rows=Q(db,"SELECT m.custody_id,m.revision,p.native_item_id,p.pose_revision,HEX(p.pose_blob),r.character_id,r.account_id,r.lease_epoch,HEX(r.session_key),HEX(r.source_row),r.runtime_serial,s.revision,c.estate_id FROM housing_crate_members m JOIN housing_live_item_custody c ON c.custody_id=m.custody_id JOIN housing_live_placed_items p ON p.custody_id=m.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id AND r.custody_id=m.custody_id AND r.state=1 JOIN housing_live_item_storage s ON s.custody_id=m.custody_id AND s.stowed=1 WHERE m.crate_id="+std::to_string(before.crate_id)+" ORDER BY m.custody_id FOR UPDATE");
 for(auto row=rows.begin();row.operator!=(rows.end());++row){
  for(size_t i=0;i<13;++i)if(!row[i])Bad(Error::Conflict);
  const auto custody=N(row[0]),old_item=N(row[2],UINT32_MAX),storage_revision=N(row[11]);
  if(N(row[1])==UINT64_MAX||storage_revision==UINT64_MAX)Bad(Error::Conflict);
  if(Q(db,"SELECT house_id FROM housing_house_item_scopes WHERE custody_id="+std::to_string(custody)+" FOR UPDATE").RowCount())Bad(Error::UnsupportedItem);
  auto source=CustodyCodec::Decode(Unhex(row[9]));
  std::optional<ContentBinding::Identity> binding;std::optional<InteriorContent::Fingerprint> room_identity;
  if(room){
   auto input=InteriorContent::ItemInputUnderTransaction(db,room->profile.room,source.values.item_id,{},true);if(!input)Bad(input.error);auto identity=InteriorContent::Build(input.value);if(!identity||!input.value.item.furniture_only)Bad(Error::UnsupportedItem);room_identity=std::move(identity.value);
  }else{
   auto house_route=RelocateContainedHouse(db,custody,carried,request,source,bundle,fence,houses);
   binding=ContentBinding::Current(db,target.catalog_key,bundle,target.native_estate_id,destination,source.values.item_id,house_route?&*house_route:nullptr);
  }
  std::array<uint8_t,32> digest{};const auto seed=request.request_key+":"+std::to_string(custody)+":"+std::to_string(old_item);
  if(!SHA256(reinterpret_cast<const unsigned char*>(seed.data()),seed.size(),digest.data()))Bad(Error::DatabaseFailure);
  const auto key="crate-relocate:"+Hex(digest);
  auto retired=Q(db,"UPDATE housing_live_placement_reservations SET state=2,custody_id=NULL WHERE native_item_id="+std::to_string(old_item)+" AND custody_id="+std::to_string(custody)+" AND state=1");if(retired.RowsAffected()!=1)Bad(Error::Conflict);
  auto made=Q(db,"INSERT INTO housing_live_placement_reservations(request_key,character_id,account_id,lease_epoch,session_key,estate_id,native_estate_id,catalog_key,catalog_revision,source_row,runtime_serial,state,custody_id,content_binding_version,content_binding_digest) VALUES("+S(db,key)+","+row[5]+","+row[6]+","+row[7]+",UNHEX('"+row[8]+"'),"+std::to_string(request.estate_id)+","+std::to_string(target.native_estate_id)+","+S(db,target.catalog_key)+","+std::to_string(target.catalog_revision)+",UNHEX('"+row[9]+"'),"+row[10]+",1,"+std::to_string(custody)+","+(binding?std::to_string(binding->version):"NULL")+","+(binding?"UNHEX('"+Hex(binding->digest)+"')":"NULL")+")");
  if(made.RowsAffected()!=1||!made.LastInsertedID()||made.LastInsertedID()>=UINT32_MAX)Bad(Error::Conflict);
  if(N(row[12])!=request.estate_id){auto moved=Q(db,"UPDATE housing_live_item_custody SET estate_id="+std::to_string(request.estate_id)+" WHERE custody_id="+std::to_string(custody)+" AND estate_id="+row[12]);if(moved.RowsAffected()!=1)Bad(Error::Conflict);}
  if(room){
   const auto& profile=room->profile;const auto& context=profile.context;
   auto tag=Q(db,"INSERT INTO housing_interior_placement_reservations(native_item_id,house_id,house_revision,profile_revision,source_zone,source_version,source_instance,content_version,content_digest) VALUES("+std::to_string(made.LastInsertedID())+","+std::to_string(profile.room.house_id)+","+std::to_string(context.house->revision)+","+std::to_string(profile.profile_revision)+","+std::to_string(destination.zone_id)+","+std::to_string(destination.zone_version)+","+std::to_string(destination.instance_id)+",3,UNHEX('"+Hex(room_identity->digest)+"'))");if(tag.RowsAffected()!=1)Bad(Error::Conflict);
   auto scope=Q(db,"INSERT INTO housing_house_item_scopes(custody_id,house_id,revision) VALUES("+std::to_string(custody)+","+std::to_string(profile.room.house_id)+",1)");if(scope.RowsAffected()!=1)Bad(Error::Conflict);
  }
  auto pose=Q(db,"UPDATE housing_live_placed_items SET native_item_id="+std::to_string(made.LastInsertedID())+",pose_revision=1 WHERE custody_id="+std::to_string(custody)+" AND native_item_id="+std::to_string(old_item)+" AND pose_revision="+row[3]);if(pose.RowsAffected()!=1)Bad(Error::Conflict);
  auto member=Q(db,"UPDATE housing_crate_members SET revision=revision+1,origin_native_estate_id="+std::to_string(target.native_estate_id)+",origin_house_id="+(room?std::to_string(room->profile.room.house_id):"NULL")+" WHERE custody_id="+std::to_string(custody)+" AND crate_id="+std::to_string(before.crate_id)+" AND revision="+row[1]);if(member.RowsAffected()!=1)Bad(Error::Conflict);
  auto storage=Q(db,"UPDATE housing_live_item_storage SET revision=revision+1,native_estate_id="+std::to_string(target.native_estate_id)+" WHERE custody_id="+std::to_string(custody)+" AND revision="+row[11]+" AND stowed=1 AND native_estate_id="+std::to_string(before.parent_native_estate_id));if(storage.RowsAffected()!=1)Bad(Error::Conflict);
 }
}

static LiveItems::Result<Binding> BindPlacedUnderTransaction(Database& db,const LiveItems::Request& r,
 LiveItems::InventorySessionFence& fence,const ReviewedPlacement& policy,const Allocation* carried,HouseBinding::CustodyContext* houses=nullptr){
 try{
  if(r.house||r.room||r.initial_stowed||r.action!=LiveItems::Action::Store||!r.placement||!r.placement->reservation.content_binding||r.placement->reservation.content_binding->version!=ContentBinding::CrateVersion)return {{},Error::InvalidArgument};
  if(carried?(!r.crate||r.crate->crate_id!=carried->binding.crate_id||r.crate->expected_revision!=carried->binding.revision||!carried->inventory_character_id||carried->binding.revision==UINT64_MAX):bool(r.crate))return {{},Error::InvalidArgument};
  const auto proof=LiveItems::Service::VerifyPlacedStoreAllocationParent(db,r,fence);if(!proof||!proof.value)return {{},proof.error};
  if(!Installed(db))return {{},Error::Disabled};
  // Existing carried identity is restored only by the explicit typed transition.
  // Never mint a second identity merely because its location changed.
  const auto source_crate=Q(db,"SELECT crate_id FROM housing_crate_locations WHERE location_kind=1 AND character_id="+std::to_string(r.lease.actor.character_id)+" AND slot_id="+std::to_string(r.expected.values.slot_id)+" FOR UPDATE");
  if(bool(source_crate.RowCount())!=bool(carried))Bad(Error::Conflict);
  if(carried){auto current=ReadCarriedState(db,r.expected,false);if(!current||EncodeAllocation(current.value)!=EncodeAllocation(*carried))Bad(Error::Conflict);if(carried->packed)Bad(Error::UnsupportedItem);}
  const auto& reserved=r.placement->reservation;
  auto access=LoadCurrentEstateAccessUnderTransaction(db,r.estate_id,reserved.native_estate_id,r.lease.actor);
  if(!access||!access.value.effective.Allows(AccessPolicy::Capability::AddCrate))Bad(Error::Unauthorized);
  if(access.value.owner_kind==OwnerKind::Guild&&access.value.effective.implicit_owner){auto leader=Q(db,"SELECT leader FROM guilds WHERE id="+std::to_string(access.value.owner_id)+" FOR UPDATE");if(leader.RowCount()!=1||N(leader.begin()[0])!=r.lease.actor.character_id)Bad(Error::Unauthorized);}
  auto parent=Q(db,"SELECT n.catalog_key,n.neighborhood_id,n.definition_id,h.zone_id,h.zone_version,h.instance_id,i.zone,i.version,i.never_expires FROM housing_native_estates n JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN instance_list i ON i.id=h.instance_id WHERE n.native_id="+std::to_string(reserved.native_estate_id)+" AND n.ownership_estate_id="+std::to_string(r.estate_id)+" AND n.active=1 FOR UPDATE");
  if(parent.RowCount()!=1)Bad(Error::Conflict);auto p=parent.begin();
  if(!p[0]||std::string(p[0])!=reserved.catalog_key||N(p[3])!=N(p[6])||N(p[4])!=N(p[7])||N(p[8])!=1||!fence.PlacementContextMatches(r.estate_id,reserved.native_estate_id,uint16_t(N(p[5],32000))))Bad(Error::Unauthorized);
  auto catalog=Q(db,"SELECT revision,origin,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+S(db,reserved.catalog_key)+" FOR UPDATE");
  if(catalog.RowCount()!=1)Bad(Error::Conflict);auto c=catalog.begin();const auto revision=N(c[0]);
  if(!revision||revision!=reserved.catalog_revision||N(c[2])!=20130510)Bad(Error::Conflict);
  if(N(c[1])==2){auto name=Q(db,"SELECT DATABASE()");if(name.RowCount()!=1||!name.begin()[0])Bad(Error::DatabaseFailure);std::string current=name.begin()[0];if(!Housing::RuntimeActivation::AllowsReconstructedContent(db))Bad(Error::Unauthorized);}else if(N(c[1])!=1)Bad(Error::Unauthorized);
  auto bundle=Catalog::DecodePayloads(Unhex(c[3]),Unhex(c[4]));
  auto metadata=Q(db,"SELECT idfile,UNK227,UNK224,UNK228,UNK229,UNK231,itemclass,bagslots,stackable FROM items WHERE id="+std::to_string(r.expected.values.item_id)+" FOR UPDATE");
  if(metadata.RowCount()!=1||!metadata.begin()[0])Bad(Error::UnsupportedItem);auto m=metadata.begin();
  const auto definition=uint32_t(N(m[1],UINT32_MAX));const auto* model=CrateGeometry::Lookup(r.expected.values.item_id,definition,m[0]);
  const auto minimum=F(m[3]),maximum=F(m[4]);
  if(!model||N(m[6])||N(m[7])||N(m[8])||!(minimum>0)||maximum<minimum||r.placement->transform.scale<minimum||r.placement->transform.scale>maximum)Bad(Error::UnsupportedItem);
  const EstateWire::Definition *plot=nullptr,*crate=nullptr;const RegionWire::Region* region=nullptr;
  for(const auto& d:bundle.definitions){if(d.id==N(p[2],UINT32_MAX))plot=&d;if(d.id==definition)crate=&d;}
  if(!plot||!crate||(plot->type!=4&&plot->type!=8)||crate->type!=7||!crate->max_items||!plot->max_children||std::find(plot->children.begin(),plot->children.end(),definition)==plot->children.end()||!(N(m[2],UINT32_MAX)&PlacementPolicy::EstateFlag(plot->type)))Bad(Error::UnsupportedItem);
  for(const auto& v:bundle.regions)if(v.id==plot->area_id)region=&v;if(!region)Bad(Error::Conflict);
  auto custody=Q(db,"SELECT c.custody_id,c.estate_id,c.owner_character_id,HEX(c.row_blob) FROM housing_live_item_custody c JOIN housing_live_placed_items v ON v.custody_id=c.custody_id WHERE v.native_item_id="+std::to_string(reserved.native_item_id)+" FOR UPDATE");
  if(custody.RowCount()!=1)Bad(Error::Conflict);auto stored=custody.begin();const auto custody_id=N(stored[0]);
  if(!custody_id||N(stored[1])!=r.estate_id||N(stored[2])!=r.lease.actor.character_id||Unhex(stored[3])!=CustodyCodec::Encode(r.expected))Bad(Error::Conflict);
  const auto digest=policy.ManifestDigest();if(std::all_of(digest.begin(),digest.end(),[](auto b){return b==0;}))Bad(Error::Unauthorized);
  ContentBinding::CrateProfile profile{definition,Hex(digest),true};
  auto identity=ContentBinding::Current(db,reserved.catalog_key,bundle,reserved.native_estate_id,{uint32_t(N(p[3],UINT32_MAX)),uint8_t(N(p[4],UINT8_MAX)),uint16_t(N(p[5],32000))},r.expected.values.item_id,nullptr,&profile);
  if(!ContentBinding::Same(identity,*reserved.content_binding))Bad(Error::Conflict);
  PlacementPolicy::Context context{reserved.native_estate_id,plot->type,region->id,access.value.effective,true};
  PlacementPolicy::ItemMetadata item{r.expected.values.item_id,model->model_id,uint32_t(N(m[2],UINT32_MAX)),minimum,maximum,false,7};
  if(!PlacementPolicy::Check(context,item,model->geometry,*region,r.placement->transform,PlacementPolicy::Operation::Add))Bad(Error::UnsupportedItem);
  if(carried&&carried->binding.definition_id!=definition)Bad(Error::Conflict);
  const bool catalog_move=carried&&(carried->binding.catalog_key!=reserved.catalog_key||carried->binding.catalog_revision!=revision||carried->content_manifest_sha256!=digest);
  PlacementFacts facts{r,custody_id,*plot,*crate,*region,*model,access.value,revision,carried};
  if(!policy.VerifiedUnderTransaction(db,fence,facts))Bad(Error::Unauthorized);
  if(catalog_move){
   if(!policy.CatalogRelocationReviewedUnderTransaction(db,fence,facts))Bad(Error::Unauthorized);
   auto old_catalog=Q(db,"SELECT revision,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+S(db,carried->binding.catalog_key)+" FOR UPDATE");
   if(old_catalog.RowCount()!=1||N(old_catalog.begin()[0])!=carried->binding.catalog_revision||N(old_catalog.begin()[1])!=20130510)Bad(Error::Conflict);
   auto old_definition=ReadDefinitionUnderTransaction(db,*carried);if(!old_definition)Bad(old_definition.error);
   if(EstateWire::EncodeBootstrap({old_definition.value},{})!=EstateWire::EncodeBootstrap({*crate},{}))Bad(Error::UnsupportedItem);
  }
  const bool relocating=carried&&(carried->binding.estate_id!=r.estate_id||carried->binding.parent_native_estate_id!=reserved.native_estate_id);
  if(relocating&&!policy.ReparentReviewedUnderTransaction(db,fence,facts))Bad(Error::Unauthorized);
  auto existing=Q(db,"SELECT h.crate_id,h.revision,h.item_template_id,h.created_by_character_id,h.packed,HEX(h.content_manifest_sha256),l.location_kind,l.custody_id,e.native_estate_id,e.parent_native_estate_id,e.ownership_estate_id,e.definition_id,e.catalog_key,e.catalog_revision,n.node_key,n.active FROM housing_crate_instances h JOIN housing_crate_locations l ON l.crate_id=h.crate_id JOIN housing_crate_estates e ON e.crate_id=h.crate_id JOIN housing_native_estates n ON n.native_id=e.native_estate_id AND n.parent_native_id=e.parent_native_estate_id AND n.catalog_key=e.catalog_key AND n.definition_id=e.definition_id WHERE h.creation_request_key="+S(db,r.request_key)+" FOR UPDATE");
  if(existing.RowCount()){
   if(existing.RowCount()!=1)Bad(Error::Conflict);auto e=existing.begin();Binding out{N(e[0]),N(e[1]),N(e[7]),N(e[13]),uint32_t(N(e[2],UINT32_MAX)),uint32_t(N(e[8],UINT32_MAX)),uint32_t(N(e[9],UINT32_MAX)),uint32_t(N(e[11],UINT32_MAX)),uint32_t(N(e[10],UINT32_MAX)),e[12]?e[12]:""};
   if(!out.crate_id||out.revision!=1||out.custody_id!=custody_id||out.item_template_id!=r.expected.values.item_id||out.parent_native_estate_id!=reserved.native_estate_id||out.estate_id!=r.estate_id||out.definition_id!=definition||out.catalog_key!=reserved.catalog_key||out.catalog_revision!=revision||N(e[3])!=r.lease.actor.character_id||N(e[4])||N(e[6])!=2||Unhex(e[5])!=std::vector<uint8_t>(digest.begin(),digest.end())||!e[14]||std::string(e[14])!="crate:"+std::to_string(out.crate_id)||N(e[15])!=1)Bad(Error::Conflict);
   return {out,Error::None};
  }
  // Permanent rooms survive house storage and are not placed plot children.
  // Keep unknown/orphan nodes in the count; exclude only an exact managed
  // house whose location/storage proves it is currently off the yard.
  auto children=Q(db,"SELECT COUNT(*) FROM housing_native_estates n WHERE n.parent_native_id="+std::to_string(reserved.native_estate_id)+" AND n.active=1 AND NOT EXISTS (SELECT 1 FROM housing_house_interiors i JOIN housing_house_locations l ON l.house_id=i.house_id LEFT JOIN housing_live_item_storage s ON s.custody_id=l.custody_id WHERE i.native_estate_id=n.native_id AND (l.location_kind=1 OR (l.location_kind=2 AND (s.stowed=1 OR EXISTS (SELECT 1 FROM housing_crate_members m WHERE m.custody_id=l.custody_id))))) FOR UPDATE");if(children.RowCount()!=1||N(children.begin()[0])>=plot->max_children)Bad(Error::Conflict);
  if(!CratePropertyLimit::AvailableUnderTransaction(db,r.estate_id,carried?carried->binding.crate_id:0))Bad(Error::Conflict);
  if(!m[5])Bad(Error::DatabaseFailure);const auto per_estate=std::stoll(m[5]);if(per_estate>0){auto count=Q(db,"SELECT COUNT(*) FROM housing_crate_estates e JOIN housing_crate_locations l ON l.crate_id=e.crate_id WHERE e.parent_native_estate_id="+std::to_string(reserved.native_estate_id)+" AND l.location_kind=2 FOR UPDATE");if(count.RowCount()!=1||N(count.begin()[0])>=uint64_t(per_estate))Bad(Error::Conflict);}
  if(carried){
   const auto& before=carried->binding;const auto id=std::to_string(before.crate_id);
   auto location=Q(db,"UPDATE housing_crate_locations SET location_kind=2,character_id=NULL,slot_id=NULL,custody_id="+std::to_string(custody_id)+" WHERE crate_id="+id+" AND location_kind=1 AND character_id="+std::to_string(carried->inventory_character_id)+" AND slot_id="+std::to_string(carried->inventory_slot_id)+" AND custody_id IS NULL");
   auto identity=Q(db,"UPDATE housing_crate_instances SET content_manifest_sha256=UNHEX('"+Hex(digest)+"'),revision=revision+1 WHERE crate_id="+id+" AND revision="+std::to_string(before.revision)+" AND packed=0");
   if(relocating||catalog_move)RelocateMembers(db,*carried,r,bundle,{uint32_t(N(p[3],UINT32_MAX)),uint8_t(N(p[4],UINT8_MAX)),uint16_t(N(p[5],32000))},fence,houses);
   auto native=Q(db,"UPDATE housing_native_estates SET catalog_key="+S(db,reserved.catalog_key)+",active=1,parent_native_id="+std::to_string(reserved.native_estate_id)+",neighborhood_id="+std::to_string(N(p[1],UINT32_MAX))+" WHERE native_id="+std::to_string(before.native_estate_id)+" AND parent_native_id="+std::to_string(before.parent_native_estate_id)+" AND active=0");
   if(location.RowsAffected()!=1||identity.RowsAffected()!=1||native.RowsAffected()!=1)Bad(Error::Conflict);
   Q(db,"UPDATE housing_crate_estates SET catalog_key="+S(db,reserved.catalog_key)+",parent_native_estate_id="+std::to_string(reserved.native_estate_id)+",ownership_estate_id="+std::to_string(r.estate_id)+",catalog_revision="+std::to_string(revision)+" WHERE crate_id="+id);
   auto after=before;after.revision++;after.custody_id=custody_id;after.catalog_key=reserved.catalog_key;after.catalog_revision=revision;after.parent_native_estate_id=reserved.native_estate_id;after.estate_id=r.estate_id;return {after,Error::None};
  }
  CrateCreationOrigin::Live(db,r.request_key);
  auto made=Q(db,"INSERT INTO housing_crate_instances(item_template_id,created_by_character_id,creation_request_key,content_manifest_sha256) VALUES("+std::to_string(r.expected.values.item_id)+","+std::to_string(r.lease.actor.character_id)+","+S(db,r.request_key)+",UNHEX('"+Hex(digest)+"'))");if(made.RowsAffected()!=1||!made.LastInsertedID())Bad(Error::DatabaseFailure);const auto id=made.LastInsertedID();
  auto native=Q(db,"INSERT INTO housing_native_estates(catalog_key,node_key,parent_native_id,neighborhood_id,definition_id,static_id,active) VALUES("+S(db,reserved.catalog_key)+","+S(db,"crate:"+std::to_string(id))+","+std::to_string(reserved.native_estate_id)+","+std::to_string(N(p[1],UINT32_MAX))+","+std::to_string(definition)+",0,1)");const auto native_id=native.LastInsertedID();if(native.RowsAffected()!=1||!native_id||native_id>=UINT32_MAX)Bad(Error::DatabaseFailure);
  Q(db,"INSERT INTO housing_crate_locations(crate_id,location_kind,custody_id) VALUES("+std::to_string(id)+",2,"+std::to_string(custody_id)+")");
  Q(db,"INSERT INTO housing_crate_estates(crate_id,native_estate_id,parent_native_estate_id,ownership_estate_id,definition_id,catalog_key,catalog_revision) VALUES("+std::to_string(id)+","+std::to_string(native_id)+","+std::to_string(reserved.native_estate_id)+","+std::to_string(r.estate_id)+","+std::to_string(definition)+","+S(db,reserved.catalog_key)+","+std::to_string(revision)+")");
  return {{id,1,custody_id,revision,r.expected.values.item_id,uint32_t(native_id),reserved.native_estate_id,definition,r.estate_id,reserved.catalog_key},Error::None};
 }catch(const Failure& failure){return {{},failure.error};}catch(...){return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Binding> BindNewPlacedUnderTransaction(Database& db,const LiveItems::Request& r,LiveItems::InventorySessionFence& f,const ReviewedPlacement& p){return BindPlacedUnderTransaction(db,r,f,p,nullptr);}
LiveItems::Result<Binding> RebindPlacedUnderTransaction(Database& db,const LiveItems::Request& r,LiveItems::InventorySessionFence& f,const ReviewedPlacement& p,const Allocation& carried,HouseBinding::CustodyContext* houses){return BindPlacedUnderTransaction(db,r,f,p,&carried,houses);}
std::vector<uint8_t> EncodeAllocation(const Allocation& a){
 const auto& b=a.binding;
 auto key=[](const std::string& s){return !s.empty()&&s.size()<=96&&s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:_-.")==std::string::npos;};
 const bool carried=a.inventory_character_id!=0,queued=a.parcel_id!=0;
 const bool slot=a.inventory_slot_id==33||(a.inventory_slot_id>=23&&a.inventory_slot_id<=32);
 if((queued?(carried||a.inventory_slot_id||b.custody_id):(carried?(!slot||b.custody_id!=0):(!b.custody_id||a.inventory_slot_id)))||!b.crate_id||!b.revision||!b.catalog_revision||!b.native_estate_id||b.native_estate_id==UINT32_MAX||!b.parent_native_estate_id||b.parent_native_estate_id==UINT32_MAX||b.native_estate_id==b.parent_native_estate_id||!b.estate_id||!a.created_by_character_id||!key(b.catalog_key)||!key(a.creation_request_key)||CrateGeometry::TemplateDefinition(b.item_template_id)!=b.definition_id||!b.definition_id||std::all_of(a.content_manifest_sha256.begin(),a.content_manifest_sha256.end(),[](auto v){return v==0;}))throw std::invalid_argument("Invalid crate allocation outcome");
 std::vector<uint8_t> out;auto word=[&](uint64_t n,size_t width){for(size_t i=0;i<width;++i)out.push_back(uint8_t(n>>(8*i)));};
 word(queued?3:(carried?2:1),4);for(auto n:{b.crate_id,b.revision,b.custody_id,b.catalog_revision})word(n,8);
 for(auto n:{b.item_template_id,b.native_estate_id,b.parent_native_estate_id,b.definition_id,b.estate_id,a.created_by_character_id})word(n,4);
 for(const auto& text:{b.catalog_key,a.creation_request_key}){word(text.size(),1);out.insert(out.end(),text.begin(),text.end());}
 word(a.packed,1);out.insert(out.end(),a.content_manifest_sha256.begin(),a.content_manifest_sha256.end());if(carried){word(a.inventory_character_id,4);word(a.inventory_slot_id,4);}if(queued)word(a.parcel_id,4);return out;
}
LiveItems::Result<Allocation> DecodeAllocation(const std::vector<uint8_t>& bytes){
 try{
  size_t at=0;auto word=[&](size_t width){if(width>bytes.size()-at)throw std::invalid_argument("Truncated crate outcome");uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(bytes[at++])<<(8*i);return n;};
  const auto version=word(4);if(version!=1&&version!=2&&version!=3)return {{},Error::DatabaseFailure};Allocation a;auto& b=a.binding;
  for(auto* n:{&b.crate_id,&b.revision,&b.custody_id,&b.catalog_revision})*n=word(8);
  for(auto* n:{&b.item_template_id,&b.native_estate_id,&b.parent_native_estate_id,&b.definition_id,&b.estate_id,&a.created_by_character_id})*n=uint32_t(word(4));
  for(auto* text:{&b.catalog_key,&a.creation_request_key}){const auto size=word(1);if(size>bytes.size()-at)throw std::invalid_argument("Truncated crate key");text->assign(bytes.begin()+at,bytes.begin()+at+size);at+=size;}
  const auto packed=word(1);if(packed>1)return {{},Error::DatabaseFailure};a.packed=packed;
  for(auto& byte:a.content_manifest_sha256)byte=uint8_t(word(1));
  if(version==2){a.inventory_character_id=uint32_t(word(4));a.inventory_slot_id=uint32_t(word(4));}
  if(version==3)a.parcel_id=uint32_t(word(4));
  if(at!=bytes.size()||EncodeAllocation(a)!=bytes)return {{},Error::DatabaseFailure};return {a,Error::None};
 }catch(...){return {{},Error::DatabaseFailure};}
}
static LiveItems::Result<Allocation> ReadInteriorPlacedState(Database& db,const LiveItems::Stored& stored){
 try{
  if(!stored.placement||stored.placement->reservation.content_binding||!CrateGeometry::TemplateDefinition(stored.row.values.item_id))Bad(Error::InvalidArgument);
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0])!=1)Bad(Error::InvalidArgument);if(!Installed(db))Bad(Error::Disabled);
  const auto& p=*stored.placement;const auto& r=p.reservation;
  auto rows=Q(db,"SELECT h.crate_id,h.revision,h.item_template_id,h.created_by_character_id,h.packed,HEX(h.content_manifest_sha256),h.creation_request_key,l.location_kind,l.custody_id,e.native_estate_id,e.parent_native_estate_id,e.ownership_estate_id,e.definition_id,e.catalog_key,e.catalog_revision,n.node_key,n.active,n.parent_native_id,n.definition_id,n.catalog_key,scope.house_id,i.native_estate_id,t.house_id,t.content_version,HEX(t.content_digest),r.native_estate_id,r.estate_id,r.catalog_key,r.catalog_revision,r.state,r.custody_id,r.character_id,HEX(r.source_row),c.estate_id,c.owner_character_id,HEX(c.row_blob),v.native_item_id,v.pose_revision,HEX(v.pose_blob),r.content_binding_version,r.content_binding_digest,t.source_zone,t.source_version,t.source_instance,i.zone_id,i.zone_version,i.instance_id FROM housing_crate_instances h JOIN housing_crate_locations l ON l.crate_id=h.crate_id AND l.location_kind=2 JOIN housing_crate_estates e ON e.crate_id=h.crate_id JOIN housing_native_estates n ON n.native_id=e.native_estate_id JOIN housing_live_item_custody c ON c.custody_id=l.custody_id JOIN housing_live_placed_items v ON v.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=v.native_item_id JOIN housing_house_item_scopes scope ON scope.custody_id=c.custody_id JOIN housing_house_interiors i ON i.house_id=scope.house_id JOIN housing_interior_placement_reservations t ON t.native_item_id=v.native_item_id AND t.house_id=scope.house_id WHERE c.custody_id="+std::to_string(stored.custody_id)+" FOR UPDATE");
  if(rows.RowCount()!=1)Bad(Error::Conflict);auto row=rows.begin();for(size_t j=0;j<47;++j)if(j!=39&&j!=40&&!row[j])Bad(Error::Conflict);
  Allocation a;auto& b=a.binding;b={N(row[0]),N(row[1]),N(row[8]),N(row[14]),uint32_t(N(row[2],UINT32_MAX)),uint32_t(N(row[9],UINT32_MAX)),uint32_t(N(row[10],UINT32_MAX)),uint32_t(N(row[12],UINT32_MAX)),uint32_t(N(row[11],UINT32_MAX)),row[13]};a.created_by_character_id=N(row[3],UINT32_MAX);if(N(row[4])>1)Bad(Error::Conflict);a.packed=N(row[4]);a.creation_request_key=row[6];auto manifest=Unhex(row[5]);if(manifest.size()!=32)Bad(Error::Conflict);std::copy(manifest.begin(),manifest.end(),a.content_manifest_sha256.begin());CrateCreationOrigin::Validate(db,a.creation_request_key,a.created_by_character_id);EncodeAllocation(a);
  std::vector<uint8_t> pose;for(float f:{p.transform.heading,p.transform.pitch,p.transform.roll,p.transform.scale,p.transform.position[0],p.transform.position[1],p.transform.position[2]}){uint32_t bits;std::memcpy(&bits,&f,4);for(size_t j=0;j<4;++j)pose.push_back(uint8_t(bits>>(8*j)));}
  const auto source=CustodyCodec::Encode(stored.row);
  if(b.custody_id!=stored.custody_id||b.item_template_id!=stored.row.values.item_id||a.created_by_character_id!=stored.row.values.character_id||b.estate_id!=stored.estate_id||b.estate_id!=r.estate_id||b.parent_native_estate_id!=r.native_estate_id||b.catalog_key!=r.catalog_key||b.catalog_revision!=r.catalog_revision||N(row[7])!=2||std::string(row[15])!="crate:"+std::to_string(b.crate_id)||N(row[16])!=1||N(row[17])!=b.parent_native_estate_id||N(row[18])!=b.definition_id||std::string(row[19])!=b.catalog_key||!N(row[20])||N(row[21])!=b.parent_native_estate_id||N(row[22])!=N(row[20])||N(row[23])!=InteriorContent::Version||Unhex(row[24]).size()!=32||N(row[25])!=r.native_estate_id||N(row[26])!=stored.estate_id||std::string(row[27])!=r.catalog_key||N(row[28])!=r.catalog_revision||N(row[29])!=1||N(row[30])!=stored.custody_id||N(row[31])!=stored.row.values.character_id||Unhex(row[32])!=source||N(row[33])!=stored.estate_id||N(row[34])!=stored.row.values.character_id||Unhex(row[35])!=source||N(row[36])!=r.native_item_id||N(row[37])!=p.pose_revision||Unhex(row[38])!=pose||row[39]||row[40]||N(row[41])!=N(row[44])||N(row[42])!=N(row[45])||N(row[43])!=N(row[46]))Bad(Error::Conflict);
  auto members=ValidateMembersUnderTransaction(db,a);if(!members)Bad(members.error);return {a,Error::None};
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}

LiveItems::Result<Allocation> ReadInteriorNativeUnderTransaction(Database& db,uint32_t native){
 try{
  if(!native||native==UINT32_MAX)Bad(Error::InvalidArgument);
  auto rows=Q(db,"SELECT c.custody_id,c.estate_id,HEX(c.row_blob),p.native_item_id,p.pose_revision,HEX(p.pose_blob),r.native_estate_id,r.catalog_key,r.catalog_revision FROM housing_crate_estates e JOIN housing_crate_locations l ON l.crate_id=e.crate_id AND l.location_kind=2 JOIN housing_live_item_custody c ON c.custody_id=l.custody_id JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE e.native_estate_id="+std::to_string(native)+" FOR UPDATE");
  if(rows.RowCount()!=1)Bad(Error::NotFound);auto row=rows.begin();for(size_t j=0;j<9;++j)if(!row[j])Bad(Error::Conflict);
  LiveItems::Stored stored;stored.custody_id=N(row[0]);stored.estate_id=N(row[1],UINT32_MAX);stored.row=CustodyCodec::Decode(Unhex(row[2]));LiveItems::Placed placed;placed.reservation={uint32_t(N(row[3],UINT32_MAX)),uint32_t(N(row[6],UINT32_MAX)),stored.estate_id,row[7],N(row[8])};placed.pose_revision=N(row[4]);
  auto pose=Unhex(row[5]);if(pose.size()!=28)Bad(Error::Conflict);float values[7];for(size_t j=0;j<7;++j){uint32_t bits=0;for(size_t k=0;k<4;++k)bits|=uint32_t(pose[j*4+k])<<(8*k);std::memcpy(&values[j],&bits,4);if(!std::isfinite(values[j]))Bad(Error::Conflict);}placed.transform={values[0],values[1],values[2],values[3],{values[4],values[5],values[6]}};stored.placement=placed;stored.current_catalog_revision=placed.reservation.catalog_revision;
  auto allocation=ReadInteriorPlacedState(db,stored);if(!allocation||allocation.value.binding.native_estate_id!=native)Bad(allocation?Error::Conflict:allocation.error);return allocation;
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}

LiveItems::Result<Allocation> ReadPlacedUnderTransaction(Database& db,const LiveItems::Stored& stored){
 if(stored.placement&&!stored.placement->reservation.content_binding)return ReadInteriorPlacedState(db,stored);
 try{
  auto transaction=Q(db,"SELECT @@in_transaction");if(transaction.RowCount()!=1||N(transaction.begin()[0])!=1)return {{},Error::InvalidArgument};
  if(!stored.custody_id||!stored.placement||!stored.placement->reservation.content_binding||stored.placement->reservation.content_binding->version!=ContentBinding::CrateVersion||!CrateGeometry::TemplateDefinition(stored.row.values.item_id))return {{},Error::InvalidArgument};
  if(!Installed(db))return {{},Error::Disabled};const auto& p=*stored.placement;const auto& r=p.reservation;
  auto rows=Q(db,"SELECT h.crate_id,h.revision,h.item_template_id,h.created_by_character_id,h.packed,HEX(h.content_manifest_sha256),h.creation_request_key,l.location_kind,l.custody_id,e.native_estate_id,e.parent_native_estate_id,e.ownership_estate_id,e.definition_id,e.catalog_key,e.catalog_revision,n.node_key,n.active,n.parent_native_id,n.definition_id,n.catalog_key,p.active,p.ownership_estate_id,p.catalog_key,p.neighborhood_id,n.neighborhood_id,c.estate_id,c.owner_character_id,HEX(c.row_blob),v.native_item_id,v.pose_revision,HEX(v.pose_blob),r.native_estate_id,r.estate_id,r.catalog_key,r.catalog_revision,r.state,r.custody_id,r.content_binding_version,HEX(r.content_binding_digest),HEX(r.source_row) FROM housing_crate_instances h JOIN housing_crate_locations l ON l.crate_id=h.crate_id JOIN housing_crate_estates e ON e.crate_id=h.crate_id JOIN housing_native_estates n ON n.native_id=e.native_estate_id JOIN housing_native_estates p ON p.native_id=e.parent_native_estate_id JOIN housing_live_item_custody c ON c.custody_id=l.custody_id JOIN housing_live_placed_items v ON v.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=v.native_item_id WHERE l.custody_id="+std::to_string(stored.custody_id)+" FOR UPDATE");
  if(rows.RowCount()!=1)Bad(Error::Conflict);auto row=rows.begin();Allocation a;auto& b=a.binding;
  b={N(row[0]),N(row[1]),N(row[8]),N(row[14]),uint32_t(N(row[2],UINT32_MAX)),uint32_t(N(row[9],UINT32_MAX)),uint32_t(N(row[10],UINT32_MAX)),uint32_t(N(row[12],UINT32_MAX)),uint32_t(N(row[11],UINT32_MAX)),row[13]?row[13]:""};
  a.created_by_character_id=N(row[3],UINT32_MAX);if(N(row[4])>1)Bad(Error::Conflict);a.packed=N(row[4]);a.creation_request_key=row[6]?row[6]:"";
  auto manifest=Unhex(row[5]);if(manifest.size()!=32)Bad(Error::Conflict);std::copy(manifest.begin(),manifest.end(),a.content_manifest_sha256.begin());CrateCreationOrigin::Validate(db,a.creation_request_key,a.created_by_character_id);EncodeAllocation(a);
  std::vector<uint8_t> pose;for(float v:{p.transform.heading,p.transform.pitch,p.transform.roll,p.transform.scale,p.transform.position[0],p.transform.position[1],p.transform.position[2]}){uint32_t bits;std::memcpy(&bits,&v,4);for(size_t i=0;i<4;++i)pose.push_back(uint8_t(bits>>(8*i)));}
  const auto source=CustodyCodec::Encode(stored.row);
  if(b.custody_id!=stored.custody_id||b.item_template_id!=stored.row.values.item_id||b.estate_id!=stored.estate_id||b.estate_id!=r.estate_id||b.parent_native_estate_id!=r.native_estate_id||b.catalog_key!=r.catalog_key||b.catalog_revision!=r.catalog_revision||N(row[7])!=2||!row[15]||std::string(row[15])!="crate:"+std::to_string(b.crate_id)||N(row[16])!=1||N(row[17])!=b.parent_native_estate_id||N(row[18])!=b.definition_id||!row[19]||std::string(row[19])!=b.catalog_key||N(row[20])!=1||N(row[21])!=b.estate_id||!row[22]||std::string(row[22])!=b.catalog_key||N(row[23])!=N(row[24])||N(row[25])!=stored.estate_id||N(row[26])!=stored.row.values.character_id||Unhex(row[27])!=source||N(row[28])!=r.native_item_id||N(row[29])!=p.pose_revision||Unhex(row[30])!=pose||N(row[31])!=r.native_estate_id||N(row[32])!=r.estate_id||!row[33]||std::string(row[33])!=r.catalog_key||N(row[34])!=r.catalog_revision||N(row[35])!=1||N(row[36])!=stored.custody_id||N(row[37])!=ContentBinding::CrateVersion||Unhex(row[38])!=std::vector<uint8_t>(r.content_binding->digest.begin(),r.content_binding->digest.end())||Unhex(row[39])!=source)Bad(Error::Conflict);
  auto members=ValidateMembersUnderTransaction(db,a);if(!members)Bad(members.error);
  return {a,Error::None};
 }catch(const Failure& failure){return {{},failure.error};}catch(...){return {{},Error::DatabaseFailure};}
}

static LiveItems::Result<Allocation> ReadCarriedState(Database& db,const LiveItems::InventoryRow& expected,bool require_inventory){
 try{
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0])!=1)return {{},Error::InvalidArgument};
  if(!Installed(db))return {{},Error::Disabled};
  auto rows=Q(db,"SELECT h.crate_id,h.revision,h.item_template_id,h.created_by_character_id,h.packed,HEX(h.content_manifest_sha256),h.creation_request_key,l.character_id,l.slot_id,e.native_estate_id,e.parent_native_estate_id,e.ownership_estate_id,e.definition_id,e.catalog_key,e.catalog_revision,n.node_key,n.active,n.parent_native_id,n.definition_id,n.catalog_key,n.neighborhood_id,p.neighborhood_id FROM housing_crate_instances h JOIN housing_crate_locations l ON l.crate_id=h.crate_id JOIN housing_crate_estates e ON e.crate_id=h.crate_id JOIN housing_native_estates n ON n.native_id=e.native_estate_id JOIN housing_native_estates p ON p.native_id=e.parent_native_estate_id WHERE l.location_kind=1 AND l.custody_id IS NULL AND l.character_id="+std::to_string(expected.values.character_id)+" AND l.slot_id="+std::to_string(expected.values.slot_id)+" FOR UPDATE");
  if(rows.RowCount()!=1)Bad(Error::Conflict);auto row=rows.begin();Allocation a;auto& b=a.binding;
  b={N(row[0]),N(row[1]),0,N(row[14]),uint32_t(N(row[2],UINT32_MAX)),uint32_t(N(row[9],UINT32_MAX)),uint32_t(N(row[10],UINT32_MAX)),uint32_t(N(row[12],UINT32_MAX)),uint32_t(N(row[11],UINT32_MAX)),row[13]?row[13]:""};
  a.created_by_character_id=N(row[3],UINT32_MAX);if(N(row[4])>1)Bad(Error::Conflict);a.packed=N(row[4]);a.creation_request_key=row[6]?row[6]:"";a.inventory_character_id=N(row[7],UINT32_MAX);a.inventory_slot_id=N(row[8],UINT32_MAX);
  auto manifest=Unhex(row[5]);if(manifest.size()!=32)Bad(Error::Conflict);std::copy(manifest.begin(),manifest.end(),a.content_manifest_sha256.begin());CrateCreationOrigin::Validate(db,a.creation_request_key,a.created_by_character_id);EncodeAllocation(a);
  if(b.item_template_id!=expected.values.item_id||!row[15]||std::string(row[15])!="crate:"+std::to_string(b.crate_id)||N(row[16])||N(row[17])!=b.parent_native_estate_id||N(row[18])!=b.definition_id||!row[19]||std::string(row[19])!=b.catalog_key)Bad(Error::Conflict);
  if(N(row[20])!=N(row[21])){auto room_parent=Q(db,"SELECT house_id FROM housing_house_interiors WHERE native_estate_id="+std::to_string(b.parent_native_estate_id)+" FOR UPDATE");if(room_parent.RowCount()!=1)Bad(Error::Conflict);}
  if(require_inventory){auto inventory=Q(db,"SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(expected.values.character_id)+" AND slot_id="+std::to_string(expected.values.slot_id)+" FOR UPDATE");
  if(inventory.RowCount()!=1)Bad(Error::Conflict);
  // Decode with the exact inventory row/null-field codec, never a GUID-only match.
  if(CustodyCodec::Encode(InventoryRowDB::Decode(inventory.begin()))!=CustodyCodec::Encode(expected))Bad(Error::Conflict);}
  auto members=ValidateMembersUnderTransaction(db,a);if(!members)Bad(members.error);
  return {a,Error::None};
 }catch(const Failure& failure){return {{},failure.error};}catch(...){return {{},Error::DatabaseFailure};}
}

LiveItems::Result<Allocation> ReadCarriedUnderTransaction(Database& db,const LiveItems::InventoryRow& expected){return ReadCarriedState(db,expected,true);}

bool RelocationEffectsMatch(Database& db,const LiveItems::Receipt& r){
 try{
  if(!r.requires_application||r.action!=LiveItems::Action::Store||!r.crate_before||!r.crate_allocation)return true;
  const auto& before=r.crate_before->binding;const auto& after=r.crate_allocation->binding;
  if(before.parent_native_estate_id==after.parent_native_estate_id&&before.estate_id==after.estate_id&&before.catalog_key==after.catalog_key&&before.catalog_revision==after.catalog_revision)return true;
  auto houses=Q(db,"SELECT h.house_id,h.revision,i.native_estate_id,i.instance_id,n.parent_native_id,i.catalog_key,i.catalog_revision,n.catalog_key FROM housing_crate_members m JOIN housing_house_locations l ON l.location_kind=2 AND l.custody_id=m.custody_id JOIN housing_house_instances h ON h.house_id=l.house_id AND h.pending_request_key IS NULL JOIN housing_house_interiors i ON i.house_id=h.house_id JOIN housing_native_estates n ON n.native_id=i.native_estate_id WHERE m.crate_id="+std::to_string(after.crate_id)+" ORDER BY h.house_id FOR UPDATE");
  auto schema=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_crate_house_relocations'");if(!schema.RowCount())return houses.RowCount()==0;if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB")return false;
  auto effects=Q(db,"SELECT house_id,crate_id,before_revision,after_revision,before_parent_native_id,after_parent_native_id,interior_native_id,interior_instance_id,before_ownership_estate_id,after_ownership_estate_id FROM housing_crate_house_relocations WHERE request_key="+S(db,r.request_key)+" ORDER BY house_id FOR UPDATE");
  if(effects.RowCount()!=houses.RowCount())return false;auto h=houses.begin();
  for(auto e=effects.begin();e.operator!=(effects.end());++e,++h){
   // Legacy cross-plot effects predate ownership columns. Same-plot recovery
   // always requires exact, distinct generation identities from this receipt.
   if(bool(e[8])!=bool(e[9]))return false;
   if(e[8]){if(N(e[8])!=before.estate_id||N(e[9])!=after.estate_id)return false;}
   else if(before.parent_native_estate_id==after.parent_native_estate_id)return false;
   if(N(e[0])!=N(h[0])||N(e[1])!=after.crate_id||N(e[2])==UINT64_MAX||N(e[3])!=N(e[2])+1||N(e[3])!=N(h[1])||N(e[4])!=before.parent_native_estate_id||N(e[5])!=after.parent_native_estate_id||N(e[6])!=N(h[2])||N(e[7])!=N(h[3])||N(h[4])!=after.parent_native_estate_id||!h[5]||std::string(h[5])!=after.catalog_key||N(h[6])!=after.catalog_revision||!h[7]||std::string(h[7])!=after.catalog_key)return false;
  }
  return true;
 }catch(...){return false;}
}

bool CoherentOutcome(const LiveItems::Receipt& r)noexcept{
 try{
  const bool known=CrateGeometry::TemplateDefinition(r.stored.row.values.item_id)!=0;
  const bool tagged=r.stored.placement&&((r.stored.placement->reservation.content_binding&&r.stored.placement->reservation.content_binding->version==ContentBinding::CrateVersion)||(known&&r.room&&!r.stored.placement->reservation.content_binding&&r.room->house_id&&r.room->placement.native_item_id==r.stored.placement->reservation.native_item_id&&r.room->placement.native_estate_id==r.stored.placement->reservation.native_estate_id));
  if(!known&&!tagged&&!r.crate_allocation&&!r.crate_before)return true;
  if(!known||!tagged||!r.crate_allocation||r.house_effect_required||r.stored.stowed||!CustodyCodec::Valid(r.stored.row))Bad(Error::DatabaseFailure);
  if(r.action!=LiveItems::Action::Store&&r.action!=LiveItems::Action::Return)Bad(Error::DatabaseFailure);
  const auto& a=*r.crate_allocation;const auto& b=a.binding;const auto& p=r.stored.placement->reservation;
  CrateBinding::EncodeAllocation(a);
  if(b.item_template_id!=r.stored.row.values.item_id||b.estate_id!=r.stored.estate_id||b.estate_id!=p.estate_id||b.parent_native_estate_id!=p.native_estate_id||b.catalog_key!=p.catalog_key||b.catalog_revision!=p.catalog_revision)Bad(Error::DatabaseFailure);
  if(r.action==LiveItems::Action::Store){
      if(a.packed||a.inventory_character_id||b.custody_id!=r.stored.custody_id)Bad(Error::DatabaseFailure);
      if(r.crate_before){
          auto expected=*r.crate_before;if(expected.binding.revision==UINT64_MAX||expected.inventory_character_id!=r.stored.row.values.character_id||expected.inventory_slot_id!=r.stored.row.values.slot_id||expected.binding.custody_id)Bad(Error::DatabaseFailure);
          expected.binding.revision++;expected.binding.custody_id=b.custody_id;expected.binding.catalog_key=b.catalog_key;expected.binding.catalog_revision=b.catalog_revision;expected.content_manifest_sha256=a.content_manifest_sha256;expected.binding.estate_id=b.estate_id;expected.binding.parent_native_estate_id=b.parent_native_estate_id;expected.inventory_character_id=expected.inventory_slot_id=0;
          if(CrateBinding::EncodeAllocation(expected)!=CrateBinding::EncodeAllocation(a))Bad(Error::DatabaseFailure);
      }else if(b.revision!=1||a.creation_request_key!=r.request_key||a.created_by_character_id!=r.stored.row.values.character_id)Bad(Error::DatabaseFailure);
  }else{
      auto destination=r.stored.row;destination.values.slot_id=r.inventory_after.values.slot_id;if(CustodyCodec::Encode(destination)!=CustodyCodec::Encode(r.inventory_after))Bad(Error::DatabaseFailure);
      if(!r.crate_before||!a.inventory_character_id||b.custody_id||!r.inventory_after.values.character_id||a.inventory_character_id!=r.inventory_after.values.character_id||a.inventory_slot_id!=r.inventory_after.values.slot_id)Bad(Error::DatabaseFailure);
      auto expected=*r.crate_before;
      if(expected.inventory_character_id||expected.inventory_slot_id||expected.binding.custody_id!=r.stored.custody_id||expected.binding.revision==UINT64_MAX)Bad(Error::DatabaseFailure);
      expected.binding.revision++;expected.binding.custody_id=0;expected.inventory_character_id=a.inventory_character_id;expected.inventory_slot_id=a.inventory_slot_id;
      if(CrateBinding::EncodeAllocation(expected)!=CrateBinding::EncodeAllocation(a))Bad(Error::DatabaseFailure);
  }

 return true;
 }catch(...){return false;}
}

}
