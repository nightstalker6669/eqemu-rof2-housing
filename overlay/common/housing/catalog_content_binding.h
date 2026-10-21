#ifndef EQEMU_HOUSING_CATALOG_CONTENT_BINDING_H
#define EQEMU_HOUSING_CATALOG_CONTENT_BINDING_H
#include "catalog_service.h"
#include "placement_policy.h"
#include "item_wire.h"
#include "npc_runtime.h"
#include "static_furniture_registry.h"
#include "house_geometry_registry.h"
#include "crate_geometry_registry.h"
#include <openssl/sha.h>
#include <map>
#include <set>
#include <cstring>

#include "known_original_interior_zones.h"
namespace Housing::ContentBinding {
// This is a server semantic identity, not proof of historical authenticity.
inline constexpr uint32_t Version=1;
inline constexpr uint32_t HouseVersion=2;
// Version3 belongs to the separate, non-durable interior-content codec.
inline constexpr uint32_t CrateVersion=4;
inline constexpr uint32_t StaticClassVersion=5;
inline constexpr uint32_t MountedClassVersion=6;
inline constexpr uint32_t NpcClassVersion=7;
inline constexpr uint32_t GenericNpcClassVersion=8;
inline bool FurnitureVersion(uint32_t v){return v==Version||v==StaticClassVersion||v==MountedClassVersion||v==NpcClassVersion||v==GenericNpcClassVersion;}
inline bool SupportedVersion(uint32_t v){return FurnitureVersion(v)||v==HouseVersion||v==CrateVersion;}
// Reviewed SERVER template route, independent of the per-house allocated
// instance/native IDs. No packet, model name or definition zone alone supplies
// this witness. Exact Live template routing is not shipped by this interface.
struct HouseRoute {
 uint32_t definition_id=0,zone_id=0;
 uint8_t zone_version=0;
 std::string zone_short_name,client_asset_sha256,collision_map_sha256,content_manifest_sha256;
 std::array<float,4> spawn{},exit{}; // server XYZ + heading, reviewed coordinates
 bool reviewed_server_route=false;
};
// Non-zoned type7 content. Its definition and capacity are bound below; this
// is deliberately not a HouseRoute and supplies no interior instance/spawn.
struct CrateProfile {
 uint32_t definition_id=0;
 std::string content_manifest_sha256;
 bool reviewed_server_content=false;
};
struct Input {
 const Catalog::Bundle* catalog=nullptr;
 uint32_t native_estate_id=0;
 Route neighborhood_route;
 PlacementPolicy::ItemMetadata item;
 PlacementPolicy::Geometry geometry;
 ItemWire::Metadata native_metadata;
 std::string model_member_sha256;
 uint32_t geometry_policy_revision=0;
 bool native_metadata_verified=false;
 bool authenticated_server_context=false;
 bool permanent_route_verified=false;
 std::optional<HouseRoute> house_route;
 std::optional<CrateProfile> crate_profile;
 bool mount_flags_known=false;
 std::optional<NpcRuntime::Profile> npc;
};
struct Identity {uint32_t version=Version;std::array<uint8_t,SHA256_DIGEST_LENGTH> digest{};};
inline bool Same(const Identity& a,const Identity& b){return SupportedVersion(a.version)&&a.version==b.version&&a.digest==b.digest;}
struct Fingerprint {
 uint32_t version=Version;
 std::array<uint8_t,SHA256_DIGEST_LENGTH> digest{};
 std::vector<uint8_t> canonical_bytes;
};
enum class Error:uint8_t {None,Untrusted,InvalidCatalog,InvalidRoute,UnknownTarget,Unsupported};
struct Result {Fingerprint value;Error error=Error::None;explicit operator bool()const{return error==Error::None;}};
inline bool Same(const Fingerprint& a,const Fingerprint& b){return SupportedVersion(a.version)&&a.version==b.version&&a.digest==b.digest;}
namespace Detail {
inline constexpr const char* KnownChairMemberSha256="7102e1e5b903acc0b00ec2675a2aabf2cf9cbf216521b90481425af59c119da3";
inline bool Identity(uint32_t id){return id&&id!=UINT32_MAX;}
inline bool Key(const std::string& s){if(s.empty()||s.size()>96)return false;for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==':'||c=='-'||c=='_'||c=='.'))return false;return true;}
inline bool Hash(const std::string& s){return s.size()==64&&s.find_first_not_of("0123456789abcdef")==std::string::npos;}
inline void Require(bool valid){if(!valid)throw std::invalid_argument("Invalid housing semantic content");}
struct Writer {
 std::vector<uint8_t> bytes;
 void Word(uint32_t v){for(size_t i=0;i<4;++i)bytes.push_back(uint8_t(v>>(8*i)));}
 void Text(const std::string& s){Require(s.size()<=255&&s.find('\0')==std::string::npos);for(unsigned char c:s)Require(c>=32&&c<=126);Word(uint32_t(s.size()));bytes.insert(bytes.end(),s.begin(),s.end());}
 void Float(float f){Require(std::isfinite(f));uint32_t bits;static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559);std::memcpy(&bits,&f,4);Word(bits);}
 void Double(double f){Require(std::isfinite(f));uint64_t bits;static_assert(sizeof(double)==8&&std::numeric_limits<double>::is_iec559);std::memcpy(&bits,&f,8);for(size_t i=0;i<8;++i)bytes.push_back(uint8_t(bits>>(8*i)));}
 void RouteValue(const Route& r){Word(r.zone_id);Word(r.zone_version);Word(r.instance_id);}
};
inline bool RouteShape(const Route& r){return Identity(r.zone_id)&&r.instance_id&&r.instance_id<=32000;}
inline void TargetDefinition(Writer& w,const EstateWire::Definition& d){
 // Purchase/upkeep scheme IDs and values are economic identity, not this spatial
 // binding. Every other target field is conservatively retained, including opaque
 // address/icon/group/switch fields. Related child ID order is preserved.
 for(uint32_t v:{d.id,d.area_id,d.max_items,d.max_non_estate_items,d.max_children,d.zone_id,d.dynamic_zone_id,d.type,d.group_id})w.Word(v);
 w.Text(d.address);w.Word(d.icon);w.Word(d.max_npc_items);
 for(float f:d.switch_coordinates)w.Float(f);w.Float(d.switch_heading);
 w.Word(uint32_t(d.children.size()));for(uint32_t id:d.children)w.Word(id);
}
}
inline Result Build(const Input& input){
 auto fail=[](Error e){Result r;r.error=e;return r;};
 if(!input.authenticated_server_context||!input.permanent_route_verified)return fail(Error::Untrusted);
 if(!Detail::Identity(input.native_estate_id))return fail(Error::UnknownTarget);
 if(!Detail::RouteShape(input.neighborhood_route))return fail(Error::InvalidRoute);
 if(!input.catalog)return fail(Error::InvalidCatalog);
 const auto& b=*input.catalog;
 if(!Detail::Key(b.key)||(b.provenance.origin!=Catalog::Origin::HistoricalVerified&&b.provenance.origin!=Catalog::Origin::Experimental)||b.provenance.client_build!=20130510||b.nodes.size()>1000||b.definitions.size()>1000||b.regions.size()>1000)return fail(Error::InvalidCatalog);
 const auto& item=input.item;const auto& geometry=input.geometry;
 // Preserve furniture v1 bytes exactly. Houses use a separate v2 domain and
 // additionally require an explicit reviewed template/interior route.
 const bool house=input.house_route.has_value();
 const bool crate=input.crate_profile.has_value();
 const bool npc=input.npc.has_value();
 if((house&&crate)||(npc&&(house||crate)))return fail(Error::Unsupported);
 const HouseGeometry::Record* house_geometry=nullptr;
 if(house){
  for(const auto& r:HouseGeometry::Records)if(r.item_id==item.item_id)house_geometry=&r;
  if(!house_geometry||item.furniture_only||item.house_definition_type!=(HouseTemplates::Type(item.item_id))||item.model_id!=house_geometry->model_id||geometry.model_id!=house_geometry->model_id||geometry.center!=house_geometry->geometry.center||geometry.extents!=house_geometry->geometry.extents||geometry.measured_local_error!=house_geometry->geometry.measured_local_error||input.geometry_policy_revision!=house_geometry->geometry_policy_revision||input.model_member_sha256!=house_geometry->member_sha256)return fail(Error::Unsupported);
  const auto& route=*input.house_route;
  if(OriginalInteriors::ClientSha256!=std::string_view(HouseGeometry::ClientSha256)||!OriginalInteriors::Matches(route.zone_id,route.zone_short_name))return fail(Error::Untrusted);
  if(!route.reviewed_server_route||!Detail::Identity(route.definition_id)||!Detail::Identity(route.zone_id)||!Detail::Key(route.zone_short_name)||!Detail::Hash(route.client_asset_sha256)||!Detail::Hash(route.collision_map_sha256)||!Detail::Hash(route.content_manifest_sha256))return fail(Error::Untrusted);
  for(const auto& point:{route.spawn,route.exit}){for(float f:point)if(!std::isfinite(f))return fail(Error::InvalidRoute);if(point[3]<0||point[3]>512)return fail(Error::InvalidRoute);}
 }else if(crate){
  const auto& profile=*input.crate_profile;
  const auto* known=CrateGeometry::Lookup(item.item_id,profile.definition_id,CrateGeometry::Original.model);
  if(!profile.reviewed_server_content||!Detail::Hash(profile.content_manifest_sha256))return fail(Error::Untrusted);
  if(!known||item.furniture_only||item.house_definition_type!=7||item.model_id!=known->model_id||geometry.model_id!=known->model_id||geometry.center!=known->geometry.center||geometry.extents!=known->geometry.extents||geometry.measured_local_error!=known->geometry.measured_local_error||input.geometry_policy_revision!=1||input.model_member_sha256!=known->member_sha256)return fail(Error::Unsupported);
 }else if(npc){
  const auto& n=*input.npc;if(!NpcRuntime::Valid(n))return fail(Error::Unsupported);const auto expected=NpcRuntime::Geometry(n);
  if(StaticFurniture::Lookup(item.item_id)||HouseTemplates::Type(item.item_id)||CrateGeometry::TemplateDefinition(item.item_id)||!NpcRuntime::Valid(n)||item.item_id!=n.item_id||item.furniture_only||item.house_definition_type||item.model_id!=n.model_id||geometry.model_id!=n.model_id||geometry.npc_height!=n.entity_height||geometry.center!=expected.center||geometry.extents!=expected.extents||input.model_member_sha256!=n.source_sha256||input.geometry_policy_revision!=1)return fail(Error::Unsupported);
 }else{
  const auto* supported=StaticFurniture::Lookup(item.item_id);
  if(supported&&(supported->evidence==StaticFurniture::Evidence::ReviewedStaticClass||StaticFurniture::Mounted(item.item_id))&&(geometry.center!=supported->geometry.center||geometry.extents!=supported->geometry.extents||geometry.measured_local_error!=supported->geometry.measured_local_error||input.geometry_policy_revision!=supported->geometry_policy_revision))return fail(Error::Unsupported);
  if(!supported||item.model_id!=supported->model_id||geometry.model_id!=supported->model_id||!item.furniture_only||item.house_definition_type||!input.geometry_policy_revision||input.model_member_sha256!=supported->member_sha256)return fail(Error::Unsupported);
 }
 if(!geometry.native_collision_verified||!input.native_metadata_verified)return fail(Error::Unsupported);
 const auto& native=input.native_metadata;
 if(!npc&&!house&&!crate){const auto* r=StaticFurniture::Lookup(item.item_id);if(r&&(r->evidence==StaticFurniture::Evidence::ReviewedStaticClass||StaticFurniture::Mounted(item.item_id))&&!StaticFurniture::NativeProfile(item.item_id,native.placeable,native.ignore_collisions,native.definition_id,native.scale_min,native.scale_max,native.upkeep_id,native.max_per_estate))return fail(Error::Unsupported);}
 if(native.placeable!=item.placement_mask||native.scale_min!=item.minimum_scale||native.scale_max!=item.maximum_scale||(!house&&!crate&&!npc?!StaticFurniture::PlacementTypeMatches(item.item_id,native.placement_type):native.placement_type!=0)||(npc?!NpcRuntime::NativeItem(native,*input.npc)||native.housepet_filename!=input.npc->filename:(!native.housepet_filename.empty()||native.interactive||native.respawn_interval)))return fail(Error::Unsupported);
 if(!std::isfinite(item.minimum_scale)||!std::isfinite(item.maximum_scale)||item.minimum_scale<=0||item.maximum_scale<item.minimum_scale||!std::isfinite(geometry.measured_local_error)||geometry.measured_local_error<0)return fail(Error::Unsupported);
 for(size_t i=0;i<3;++i)if(!std::isfinite(geometry.center[i])||!std::isfinite(geometry.extents[i])||geometry.extents[i]<0)return fail(Error::Unsupported);
 try{
  std::map<uint32_t,const EstateWire::Definition*> defs;for(const auto& d:b.definitions){Detail::Require(Detail::Identity(d.id)&&d.children.size()<=1000&&defs.emplace(d.id,&d).second);}
  std::map<std::string,const Catalog::Node*> nodes;std::set<uint32_t> native_ids;const Catalog::Node* target=nullptr;
  for(const auto& n:b.nodes){Detail::Require(Detail::Key(n.key)&&nodes.emplace(n.key,&n).second);if(n.native_id.value){Detail::Require(Detail::Identity(n.native_id.value)&&native_ids.insert(n.native_id.value).second);if(n.native_id.value==input.native_estate_id)target=&n;}}
  if(!target)return fail(Error::UnknownTarget);
  auto d=defs.find(target->definition_id);if(d==defs.end())return fail(Error::InvalidCatalog);const auto& definition=*d->second;
  // Both versions target exterior plots. Furniture v1 infers no interior;
  // house v2 binds its explicitly supplied child/template route below.
  if(definition.type!=4&&definition.type!=8)return fail(Error::Unsupported);
  if(!(item.placement_mask&PlacementPolicy::EstateFlag(definition.type)))return fail(Error::Unsupported);
  Detail::Require(Detail::Identity(target->plot_id)&&Detail::Identity(target->neighborhood_id));
  std::vector<const Catalog::Node*> chain;std::set<std::string> visited;auto node=target;
  while(true){Detail::Require(chain.size()<11&&visited.insert(node->key).second&&Detail::Identity(node->native_id.value)&&node->neighborhood_id==target->neighborhood_id&&defs.count(node->definition_id));chain.push_back(node);if(node->parent_key.empty())break;auto parent=nodes.find(node->parent_key);Detail::Require(parent!=nodes.end());node=parent->second;}
  // Established exterior relationship is plot4/8 -> neighborhood5. Other chains
  // must not gain authorization merely because they terminate at a root.
  Detail::Require(chain.size()==2);const auto& root=*defs.at(chain.back()->definition_id);
  Detail::Require(root.type==5&&root.zone_id==input.neighborhood_route.zone_id&&!chain.back()->plot_id&&!chain.back()->ownership_estate_id);
  Detail::Require(std::find(root.children.begin(),root.children.end(),definition.id)!=root.children.end());
  std::set<uint32_t> children;for(uint32_t id:definition.children)Detail::Require(Detail::Identity(id)&&id!=definition.id&&defs.count(id)&&children.insert(id).second);
  const EstateWire::Definition* house_definition=nullptr;
  const EstateWire::Definition* crate_definition=nullptr;
  if(house){
   const auto& route=*input.house_route;
   if((item.house_definition_type==2&&definition.type!=8)||(item.house_definition_type==3&&definition.type!=4)||!definition.max_children||native.definition_id!=route.definition_id||!children.count(route.definition_id))return fail(Error::Unsupported);
   house_definition=defs.at(route.definition_id);
   if(house_definition->type!=item.house_definition_type||house_definition->zone_id!=route.zone_id)return fail(Error::InvalidRoute);
  }
  if(crate){
   const auto id=input.crate_profile->definition_id;
   if(!definition.max_children||native.definition_id!=id||!children.count(id))return fail(Error::Unsupported);
   crate_definition=defs.at(id);
   if(crate_definition->type!=7||!crate_definition->max_items||crate_definition->max_non_estate_items>crate_definition->max_items)return fail(Error::Unsupported);
  }
  const RegionWire::Region* region=nullptr;std::set<uint32_t> region_ids;for(const auto& r:b.regions){Detail::Require(Detail::Identity(r.id)&&region_ids.insert(r.id).second);if(r.id==definition.area_id)region=&r;}if(!region)return fail(Error::InvalidCatalog);
  RegionWire::Detail::ValidateMatrix(region->matrix);
  const auto* furniture=!npc&&!house&&!crate?StaticFurniture::Lookup(item.item_id):nullptr;
  const bool static_class=furniture&&furniture->evidence==StaticFurniture::Evidence::ReviewedStaticClass;
  const bool mounted_class=furniture&&StaticFurniture::Mounted(item.item_id);
  if(mounted_class&&(geometry.mounting_type!=furniture->mounting_type||input.geometry_policy_revision!=3))return fail(Error::Unsupported);
  const auto version=npc?(input.npc->legacy_fixed_eye?NpcClassVersion:GenericNpcClassVersion):house?HouseVersion:crate?CrateVersion:mounted_class?MountedClassVersion:static_class?StaticClassVersion:Version;
  Detail::Writer w;w.Text("EQEmuHousingContentBinding");w.Word(version);w.Word(20130510);w.Text(b.key);w.Word(input.native_estate_id);w.RouteValue(input.neighborhood_route);
  w.Word(uint32_t(chain.size()));for(const auto* n:chain){w.Word(n->native_id.value);w.Text(n->key);w.Word(n->definition_id);w.Word(n->static_id);w.Word(n->neighborhood_id);w.Word(n->plot_id);}
  // Root capacity/other child lists and economic content are unrelated to the
  // target's geometry. Root physical zone identity and the exact edge remain.
  w.Word(root.id);w.Word(root.type);w.Word(root.zone_id);w.Word(root.dynamic_zone_id);w.Word(definition.id);
  Detail::TargetDefinition(w,definition);w.Word(region->id);for(float f:region->matrix)w.Float(f);for(float f:region->opaque_vector)w.Float(f);for(uint32_t id:region->opaque_ids)w.Word(id);
  w.Word(item.item_id);w.Word(item.model_id);w.Word(item.placement_mask);w.Float(item.minimum_scale);w.Float(item.maximum_scale);w.Word(item.furniture_only);
  auto semantic_metadata=native;semantic_metadata.upkeep_id=0;semantic_metadata.maker_id=0;auto native_bytes=ItemWire::EncodeTail(semantic_metadata);Detail::Require(ItemWire::DecodeTail(native_bytes.data(),native_bytes.size()).has_value());w.Word(uint32_t(native_bytes.size()));w.bytes.insert(w.bytes.end(),native_bytes.begin(),native_bytes.end());w.Text(input.model_member_sha256);
  w.Word(geometry.model_id);for(double f:geometry.center)w.Double(f);for(double f:geometry.extents)w.Double(f);w.Double(geometry.measured_local_error);w.Word(input.geometry_policy_revision);
  if(npc){const auto& n=*input.npc;w.Text(n.legacy_fixed_eye?"OriginalFixedHeightNpcClass":n.original_model_default?"OriginalNpcModelDefaultPC53PC24Chop":"OriginalGenericNpcClassPC53PC24Chop");w.Word(n.race);w.Word(n.gender);w.Text(n.model);w.Float(n.actor_height);w.Float(n.entity_height);w.Text(n.source_sha256);if(n.original_model_default){w.Word(n.model_default_size_bits);w.Text(std::string(NpcReviewedBindings::RaceDataSha256));}if(n.service){w.Text("ReviewedInteractiveServiceEquivalentV1");w.Text(ServiceItemBindings::SourceSha256);w.Word(n.model_default_size_bits);}if(!n.legacy_fixed_eye){w.Word(n.roffset_bits);w.Word(n.race_flag);for(auto h:n.allowed_heights.height_bits)w.Word(h);w.Text(std::string(NpcNativeModels::ClientSha256));w.Text(std::string(NpcNativeModels::ModdatSha256));}}
  if(house){const auto& route=*input.house_route;w.Text("HouseTemplateInterior");w.Word(item.house_definition_type);Detail::TargetDefinition(w,*house_definition);w.Word(route.zone_id);w.Word(route.zone_version);w.Text(route.zone_short_name);for(float f:route.spawn)w.Float(f);for(float f:route.exit)w.Float(f);w.Text(route.client_asset_sha256);w.Text(route.collision_map_sha256);w.Text(route.content_manifest_sha256);w.Text(house_geometry->witness_sha256);}
  if(crate){w.Text("NonZonedCrateContainer");w.Word(7);Detail::TargetDefinition(w,*crate_definition);w.Text(input.crate_profile->content_manifest_sha256);w.Text(CrateGeometry::Original.witness_sha256);}
  if(mounted_class){w.Text("MountedStaticResourceClass");w.Text(StaticFurniture::MountedInputSha256);w.Text(StaticFurniture::ClientSha256);w.Text(StaticFurniture::RendererSha256);w.Text(StaticFurniture::ManifestSha256);w.Text(StaticFurniture::ActorTagSha256);w.Text(furniture->archive_sha256);w.Word(input.mount_flags_known);w.Word(geometry.mounting_type);}
  if(static_class){w.Text("OrdinaryStaticResourceClass");w.Text(StaticFurniture::ClassInputSha256);w.Text(StaticFurniture::ClientSha256);w.Text(StaticFurniture::RendererSha256);w.Text(StaticFurniture::ManifestSha256);w.Text(StaticFurniture::ActorTagSha256);w.Text(furniture->archive_sha256);}
  Result result;result.value.version=version;result.value.canonical_bytes=std::move(w.bytes);if(!SHA256(result.value.canonical_bytes.data(),result.value.canonical_bytes.size(),result.value.digest.data()))return fail(Error::InvalidCatalog);return result;
 }catch(const std::invalid_argument&){return fail(Error::InvalidCatalog);}catch(...){return fail(Error::InvalidCatalog);}
}
}
#endif
