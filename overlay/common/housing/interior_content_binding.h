#ifndef EQEMU_HOUSING_INTERIOR_CONTENT_BINDING_H
#define EQEMU_HOUSING_INTERIOR_CONTENT_BINDING_H
#include "catalog_content_binding.h"
#include "crate_geometry_registry.h"
#include <algorithm>
#include <memory>
namespace Housing::InteriorContent {
// Separate room domain, carried by explicit interior reservation tags rather
// than the exterior content-binding decoder. Crate custody remains a separate
// typed allocator contract; recognizing its physical content grants none.
inline constexpr uint32_t Version=3;
inline constexpr size_t MaximumManifestBytes=2*(RegionWire::MaximumPayload+NeighborhoodWire::MaximumPayload)+4096;
// Capacity authority is versioned separately from immutable content identity.
// A loaded projection retains these exact original definitions for all v3 tags.
struct CapacityIdentity {
 EstateWire::Definition definition;
 std::vector<EstateWire::Definition> child_definitions;
};
struct Profile {
 uint64_t house_id=0;
 uint32_t native_estate_id=0;
 Route route;
 EstateWire::Definition definition;
 RegionWire::Region region;
 std::vector<RegionWire::Region> additional_regions;
 std::string zone_short_name,client_asset_sha256,collision_map_sha256,manifest_sha256;
 Catalog::Origin origin=Catalog::Origin::Experimental;
 uint32_t client_build=20130510;
 bool reviewed_room=false;
 std::vector<EstateWire::Definition> child_definitions;
 std::shared_ptr<const CapacityIdentity> capacity_identity;
 uint64_t capacity_policy_revision=0;
};
// Primary area_id stays stable; the remaining regions belong to this exact
// reviewed room profile and are never inferred from unrelated loaded areas.
inline std::vector<RegionWire::Region> Regions(const Profile& p){auto regions=std::vector<RegionWire::Region>{p.region};regions.insert(regions.end(),p.additional_regions.begin(),p.additional_regions.end());return regions;}
inline std::vector<uint8_t> RegionPayload(const Profile& p){return RegionWire::EncodeBootstrap(Regions(p));}
inline PlacementPolicy::Result CheckPlacement(const Profile& p,const PlacementPolicy::Context& context,const PlacementPolicy::ItemMetadata& item,const PlacementPolicy::Geometry& geometry,const PlacementPolicy::Transform& pose,PlacementPolicy::Operation operation){
 if(!p.reviewed_room||context.estate_id!=p.native_estate_id||context.estate_type!=p.definition.type||p.definition.area_id!=p.region.id){PlacementPolicy::Result r;r.error=PlacementPolicy::Error::InvalidContext;return r;}
 return PlacementPolicy::CheckAny(context,item,geometry,p.region,p.additional_regions,pose,operation);
}
inline bool ValidChildDefinitions(const Profile& p){
 if(p.child_definitions.size()!=p.definition.children.size()||p.child_definitions.size()>1000)return false;
 std::set<uint32_t> ids;
 for(const auto& d:p.child_definitions){
  if(!ids.insert(d.id).second||(d.id!=31&&(d.id<174||d.id>184))||d.type!=7||d.zone_id||d.area_id||
     !d.max_items||d.max_non_estate_items>d.max_items||(int32_t(d.max_npc_items)>=0&&d.max_npc_items>d.max_items)||d.max_children||!d.children.empty()||
     std::find(p.definition.children.begin(),p.definition.children.end(),d.id)==p.definition.children.end())return false;
 }
 return true;
}
inline std::vector<EstateWire::Definition> Definitions(const Profile& p){
 if(!ValidChildDefinitions(p))throw std::invalid_argument("Invalid room child definitions");
 auto definitions=p.child_definitions;definitions.insert(definitions.begin(),p.definition);return definitions;
}
inline const EstateWire::Definition& IdentityDefinition(const Profile& p){return p.capacity_identity?p.capacity_identity->definition:p.definition;}
inline std::vector<uint8_t> DefinitionPayload(const Profile& p){
 if(!p.capacity_identity)return EstateWire::EncodeBootstrap(Definitions(p),{});
 Profile base=p;base.definition=p.capacity_identity->definition;base.child_definitions=p.capacity_identity->child_definitions;base.capacity_identity.reset();base.capacity_policy_revision=0;
 return EstateWire::EncodeBootstrap(Definitions(base),{});
}
inline const EstateWire::Definition& IdentityChildDefinition(const Profile& p,const EstateWire::Definition& child){
 if(p.capacity_identity)for(const auto& original:p.capacity_identity->child_definitions)if(original.id==child.id)return original;
 return child;
}
struct Input {
 Profile room;
 PlacementPolicy::ItemMetadata item;
 PlacementPolicy::Geometry geometry;
 ItemWire::Metadata native_metadata;
 std::string model_member_sha256;
 uint32_t geometry_policy_revision=0;
 bool authenticated_current_house=false,native_metadata_verified=false;
 bool mount_flags_known=false;
 std::optional<NpcRuntime::Profile> npc;
};
struct Fingerprint {uint32_t version=Version;std::array<uint8_t,SHA256_DIGEST_LENGTH> digest{};std::vector<uint8_t> canonical_bytes;};
struct Result {Fingerprint value;ContentBinding::Error error=ContentBinding::Error::None;explicit operator bool()const{return error==ContentBinding::Error::None;}};
inline bool Same(const Fingerprint& a,const Fingerprint& b){return a.version==Version&&b.version==Version&&a.digest==b.digest;}
// The profile and source witness are SERVER-owned. This pure builder grants no
// ACL, lease, custody mutation, room allocation or historical Live provenance.
// Plot ID/owner/ACL, house pose/revision, original custody estate and creator are
// deliberately absent: current authorization is separately resolved for every
// operation; moving a house must not invalidate the furniture inside it.
inline Result Build(const Input& input){
 using Error=ContentBinding::Error;auto fail=[](Error e){Result r;r.error=e;return r;};
 const auto& p=input.room;const auto& d=p.definition;const auto& m=input.item;const auto& g=input.geometry;const auto& n=input.native_metadata;
 if(!input.authenticated_current_house||!input.native_metadata_verified||!p.reviewed_room)return fail(Error::Untrusted);
 if(!p.house_id||!ContentBinding::Detail::Identity(p.native_estate_id))return fail(Error::UnknownTarget);
 if(p.client_build!=20130510||(p.origin!=Catalog::Origin::Experimental&&p.origin!=Catalog::Origin::HistoricalVerified)||
    !OriginalInteriors::Matches(p.route.zone_id,p.zone_short_name)||!ContentBinding::Detail::RouteShape(p.route)||
    d.zone_id!=p.route.zone_id||(d.type!=2&&d.type!=3))return fail(Error::InvalidRoute);
 if(!ContentBinding::Detail::Identity(d.id)||!ContentBinding::Detail::Identity(d.area_id)||d.area_id!=p.region.id||!d.max_items||d.max_non_estate_items>d.max_items||d.children.size()>1000||!ValidChildDefinitions(p))return fail(Error::InvalidCatalog);
 if(!ContentBinding::Detail::Hash(p.client_asset_sha256)||!ContentBinding::Detail::Hash(p.collision_map_sha256)||!ContentBinding::Detail::Hash(p.manifest_sha256))return fail(Error::Untrusted);
 const auto* model=input.npc?nullptr:StaticFurniture::Lookup(m.item_id);
 const auto crate_definition=CrateGeometry::TemplateDefinition(m.item_id);
 const auto* crate=CrateGeometry::Lookup(m.item_id,n.definition_id,CrateGeometry::Original.model);
 if(input.npc){
  const auto& npc=*input.npc;if(!NpcRuntime::Valid(npc))return fail(Error::Unsupported);const auto geometry=NpcRuntime::Geometry(npc);
  if(StaticFurniture::Lookup(m.item_id)||HouseTemplates::Type(m.item_id)||!NpcRuntime::Valid(npc)||crate_definition||m.item_id!=npc.item_id||m.furniture_only||m.house_definition_type||m.model_id!=npc.model_id||g.npc_height!=npc.entity_height||g.center!=geometry.center||g.extents!=geometry.extents||input.model_member_sha256!=npc.source_sha256||input.geometry_policy_revision!=1||!NpcRuntime::NativeItem(n,npc)||n.housepet_filename!=npc.filename)return fail(Error::Unsupported);
 }else if(crate_definition){
  if(!crate||m.furniture_only||m.house_definition_type!=7||!d.max_children||
     std::find(d.children.begin(),d.children.end(),crate_definition)==d.children.end()||
     m.model_id!=crate->model_id||g.model_id!=crate->model_id||!g.native_collision_verified||
     g.center!=crate->geometry.center||g.extents!=crate->geometry.extents||g.measured_local_error!=crate->geometry.measured_local_error||
     input.model_member_sha256!=crate->member_sha256||input.geometry_policy_revision!=CrateGeometry::GeometryPolicyRevision)return fail(Error::Unsupported);
 }else if(!model||!m.furniture_only||m.house_definition_type||m.model_id!=model->model_id||g.model_id!=model->model_id||
    !g.native_collision_verified||g.center!=model->geometry.center||g.extents!=model->geometry.extents||g.measured_local_error!=model->geometry.measured_local_error||
    input.model_member_sha256!=model->member_sha256||input.geometry_policy_revision!=model->geometry_policy_revision)return fail(Error::Unsupported);
 if(model&&(model->evidence==StaticFurniture::Evidence::ReviewedStaticClass||StaticFurniture::Mounted(m.item_id))&&!StaticFurniture::NativeProfile(m.item_id,n.placeable,n.ignore_collisions,n.definition_id,n.scale_min,n.scale_max,n.upkeep_id,n.max_per_estate))return fail(Error::Unsupported);
 if(!(m.placement_mask&PlacementPolicy::EstateFlag(d.type))||n.placeable!=m.placement_mask||n.scale_min!=m.minimum_scale||n.scale_max!=m.maximum_scale||
    !std::isfinite(m.minimum_scale)||!std::isfinite(m.maximum_scale)||m.minimum_scale<=0||m.maximum_scale<m.minimum_scale||((crate_definition||input.npc)?n.placement_type!=0:!StaticFurniture::PlacementTypeMatches(m.item_id,n.placement_type))||(!input.npc&&(!n.housepet_filename.empty()||n.interactive||n.respawn_interval)))return fail(Error::Unsupported);
 try{
  RegionPayload(p);
  std::set<uint32_t> children;for(auto child:d.children)ContentBinding::Detail::Require(ContentBinding::Detail::Identity(child)&&child!=d.id&&children.insert(child).second);
  ContentBinding::Detail::Writer w;w.Text(crate_definition?"EQEmuHousingInteriorCrateContentBinding":"EQEmuHousingInteriorContentBinding");w.Word(Version);w.Word(p.client_build);
  w.Word(uint32_t(p.house_id));w.Word(uint32_t(p.house_id>>32));w.Word(p.native_estate_id);w.RouteValue(p.route);
  ContentBinding::Detail::TargetDefinition(w,IdentityDefinition(p));w.Text(p.zone_short_name);w.Text(p.client_asset_sha256);w.Text(p.collision_map_sha256);w.Text(p.manifest_sha256);
  w.Word(p.region.id);for(auto f:p.region.matrix)w.Float(f);for(auto f:p.region.opaque_vector)w.Float(f);for(auto id:p.region.opaque_ids)w.Word(id);
  w.Word(m.item_id);w.Word(m.model_id);w.Word(m.placement_mask);w.Float(m.minimum_scale);w.Float(m.maximum_scale);
  auto native=n;native.upkeep_id=0;native.maker_id=0;auto tail=ItemWire::EncodeTail(native);ContentBinding::Detail::Require(ItemWire::DecodeTail(tail.data(),tail.size()).has_value());
  w.Word(uint32_t(tail.size()));w.bytes.insert(w.bytes.end(),tail.begin(),tail.end());w.Text(input.model_member_sha256);w.Word(input.geometry_policy_revision);
  for(auto f:g.center)w.Double(f);for(auto f:g.extents)w.Double(f);w.Double(g.measured_local_error);
  if(input.npc){const auto& npc=*input.npc;w.Text(npc.legacy_fixed_eye?"OriginalFixedHeightNpcClass":npc.original_model_default?"OriginalNpcModelDefaultPC53PC24Chop":"OriginalGenericNpcClassPC53PC24Chop");w.Word(npc.race);w.Word(npc.gender);w.Text(npc.model);w.Float(npc.actor_height);w.Float(npc.entity_height);w.Text(npc.source_sha256);if(npc.original_model_default){w.Word(npc.model_default_size_bits);w.Text(std::string(NpcReviewedBindings::RaceDataSha256));}if(npc.service){w.Text("ReviewedInteractiveServiceEquivalentV1");w.Text(ServiceItemBindings::SourceSha256);w.Word(npc.model_default_size_bits);}if(!npc.legacy_fixed_eye){w.Word(npc.roffset_bits);w.Word(npc.race_flag);for(auto h:npc.allowed_heights.height_bits)w.Word(h);w.Text(std::string(NpcNativeModels::ClientSha256));w.Text(std::string(NpcNativeModels::ModdatSha256));}}
  if(crate){w.Text(crate->archive_sha256);w.Text(crate->positions_sha256);w.Text(crate->indices_sha256);w.Text(crate->witness_sha256);
   const auto child=std::find_if(p.child_definitions.begin(),p.child_definitions.end(),[&](const auto& value){return value.id==crate_definition;});
   ContentBinding::Detail::Require(child!=p.child_definitions.end());ContentBinding::Detail::TargetDefinition(w,IdentityChildDefinition(p,*child));
  }
  // Preserve existing single-region v3 identities byte-for-byte. New room
  // sets additionally bind every owned region's exact serialized bytes.
  if(!p.additional_regions.empty()){auto regions=RegionPayload(p);w.Text("InteriorAdditionalRegions");w.Word(uint32_t(regions.size()));w.bytes.insert(w.bytes.end(),regions.begin(),regions.end());}
  if(model&&StaticFurniture::Mounted(m.item_id)){if(g.mounting_type!=model->mounting_type)return fail(Error::Unsupported);w.Text("MountedStaticResourceClass");w.Text(StaticFurniture::MountedInputSha256);w.Word(input.mount_flags_known);w.Word(g.mounting_type);w.Text(model->archive_sha256);}
  if(model&&model->evidence==StaticFurniture::Evidence::ReviewedStaticClass){w.Text("OrdinaryStaticResourceClass");w.Text(StaticFurniture::ClassInputSha256);w.Text(StaticFurniture::ClientSha256);w.Text(StaticFurniture::RendererSha256);w.Text(StaticFurniture::ManifestSha256);w.Text(StaticFurniture::ActorTagSha256);w.Text(model->archive_sha256);}
  Result r;r.value.canonical_bytes=std::move(w.bytes);if(!SHA256(r.value.canonical_bytes.data(),r.value.canonical_bytes.size(),r.value.digest.data()))return fail(Error::InvalidCatalog);return r;
 }catch(...){return fail(Error::InvalidCatalog);}
}
}
#endif
