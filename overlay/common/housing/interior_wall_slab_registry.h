#ifndef EQEMU_HOUSING_INTERIOR_WALL_SLAB_REGISTRY_H
#define EQEMU_HOUSING_INTERIOR_WALL_SLAB_REGISTRY_H
#include "interior_wall_slab_policy.h"
#include "interior_mounted_region_data.h"
#include "original_planar_inventory_data.h"
#include "original_curved_inventory_data.h"
namespace Housing::InteriorWallSlab {
// Explicit technical representative content, approved only by review/applying
// this candidate. Original source geometry; NOT an original Live region catalog.
// Experimental runtime profile guards still apply; no active DB blob is changed.
inline bool Eligible(const PlacementPolicy::Geometry& g){return g.native_collision_verified&&(g.mounting_type==1||g.mounting_type==2)&&std::all_of(g.center.begin(),g.center.end(),[](double v){return std::isfinite(v);});}
inline std::optional<Policy> LegacyReviewed(const InteriorContent::Profile& base){
 if(base.house_id!=3||base.origin!=Catalog::Origin::Experimental||base.route.zone_id!=345||base.zone_short_name!="guildhall"||base.definition.type!=2)return {};
 Policy p;p.reviewed=true;p.zone_id=345;p.zone_short_name="guildhall";p.asset_sha256="086b30c6236546054fde24efea0274b0c34e561320935809afa74a9a03182416";p.map_sha256="6af6b64420b29eeb146545a704758ab11e996ee2056c4f7e4446f5fad39c2030";p.base_manifest_sha256="5f696b2f890ed2661fb54b3e630b446b04443b48b62d7395cf86d028c151410b";p.base_regions_sha256="0d8790797983364c3993c2a2f29db77f77f091373fbf1b1c4c6820a36d1bb11a";p.reviewed_sha256="13fcb76680414736c73d26be48350a9711e8d0fbe7f25fb1c707bb9cdf9c0ad1";
p.slabs.push_back({RegionWire::Region{1376680004,{{0x1.0000000000000p+1f,0x0.0p+0f,0x0.0p+0f,0x0.0p+0f,0x0.0p+0f,0x1.cf75c80000000p+2f,0x0.0p+0f,0x0.0p+0f,0x0.0p+0f,0x0.0p+0f,0x1.a38cb80000000p+1f,0x0.0p+0f,0x1.26addc0000000p+6f,0x1.cd54a40000000p+5f,0x1.86aa140000000p+3f,0x1.0000000000000p+0f}}},1,0x1.2eaddc0000000p+6f,-1,{{0x1.9365ea0000000p+5f,0x1.03a1ae0000000p+6f,0x1.1dc6e60000000p+3f,0x1.ef8d420000000p+3f}},{{9,769}}});
if(!Prepare(base,p))return {};return p;
}
// Geometry-family registration, never item/SKU or house-ID enrollment. Exact
// original asset/map and an independently reviewed CURRENT base profile required.
inline std::optional<Policy> PairReviewed(const InteriorContent::Profile& base){
 auto old=LegacyReviewed(base);auto data=ConnectedData(base.route.zone_id);if(!data)return old;
 if(!base.reviewed_room||base.origin!=Catalog::Origin::Experimental||(base.definition.type!=2&&base.definition.type!=3)||base.zone_short_name!=data->short_name||base.client_asset_sha256!=data->asset||base.collision_map_sha256!=data->map)return old;

 Policy p;p.reviewed=true;p.revision=2;p.zone_id=data->zone;p.zone_short_name=data->short_name;p.asset_sha256=data->asset;p.map_sha256=data->map;p.base_manifest_sha256=base.manifest_sha256;p.base_regions_sha256=Hash(InteriorContent::RegionPayload(base));p.source_face_inventory_sha256=data->source_faces;p.source_envelope=data->envelope;p.source_faces=std::move(data->faces);if(old){for(const auto& slab:old->slabs)p.slabs.push_back(slab);}if(old)p.prior=std::make_shared<Policy>(*old);
 ContentBinding::Detail::Writer w;w.Text("EQEmuReviewedConnectedInteriorMountRegionsV2");w.Word(p.zone_id);w.Text(p.zone_short_name);w.Text(p.asset_sha256);w.Text(p.map_sha256);w.Text(p.base_manifest_sha256);w.Text(p.base_regions_sha256);w.Text(p.source_face_inventory_sha256);w.Word(uint32_t(p.slabs.size()));
 auto encoded=RegionWire::EncodeBootstrap({*p.source_envelope});w.Word(uint32_t(encoded.size()));w.bytes.insert(w.bytes.end(),encoded.begin(),encoded.end());w.Word(uint32_t(p.source_faces.size()));for(const auto& face:p.source_faces){w.Word(face.axis);for(float f:face.interval)w.Float(f);for(float f:face.rectangle)w.Float(f);for(auto t:face.triangles)w.Word(t);}

 for(const auto& slab:p.slabs){auto bytes=RegionWire::EncodeBootstrap({slab.region});w.Word(uint32_t(bytes.size()));w.bytes.insert(w.bytes.end(),bytes.begin(),bytes.end());w.Word(slab.axis);w.Float(slab.plane);w.Word(uint32_t(slab.side));for(auto f:slab.rectangle)w.Float(f);for(auto t:slab.triangles)w.Word(t);}
 p.reviewed_sha256=Hash(w.bytes);if(!Prepare(base,p))return old;return p;
}
// Immutable V3 predecessor; V2/V1/base tags resolve through prior.
inline std::optional<Policy> CoplanarReviewed(const InteriorContent::Profile& base){
 auto previous=PairReviewed(base);if(!previous||previous->revision!=2)return previous;
 auto p=*previous;p.revision=3;p.prior=std::make_shared<Policy>(*previous);
 ContentBinding::Detail::Writer w;w.Text("EQEmuReviewedCoplanarInteriorMountRegionsV3");w.Word(p.zone_id);w.Text(p.zone_short_name);w.Text(p.asset_sha256);w.Text(p.map_sha256);w.Text(p.base_manifest_sha256);w.Text(p.base_regions_sha256);w.Text(p.source_face_inventory_sha256);w.Word(uint32_t(p.slabs.size()));
 auto encoded=RegionWire::EncodeBootstrap({*p.source_envelope});w.Word(uint32_t(encoded.size()));w.bytes.insert(w.bytes.end(),encoded.begin(),encoded.end());w.Word(uint32_t(p.source_faces.size()));for(const auto& face:p.source_faces){w.Word(face.axis);for(float f:face.interval)w.Float(f);for(float f:face.rectangle)w.Float(f);for(auto t:face.triangles)w.Word(t);}

 for(const auto& slab:p.slabs){auto bytes=RegionWire::EncodeBootstrap({slab.region});w.Word(uint32_t(bytes.size()));w.bytes.insert(w.bytes.end(),bytes.begin(),bytes.end());w.Word(slab.axis);w.Float(slab.plane);w.Word(uint32_t(slab.side));for(auto f:slab.rectangle)w.Float(f);for(auto t:slab.triangles)w.Word(t);}
 p.reviewed_sha256=Hash(w.bytes);if(!Prepare(base,p))return previous;return p;
}
// New Add alone adopts oriented V4. Existing stored V3/V2/V1/base digests
// keep their exact prior interpretation; there is no automatic content rewrite.
inline std::optional<Policy> OrientedReviewed(const InteriorContent::Profile& base){
 auto previous=CoplanarReviewed(base);if(!previous||previous->revision!=3)return previous;
 auto p=*previous;p.revision=4;p.prior=std::make_shared<Policy>(*previous);
 ContentBinding::Detail::Writer w;w.Text("EQEmuReviewedOrientedInteriorMountRegionsV4");w.Word(p.zone_id);w.Text(p.zone_short_name);w.Text(p.asset_sha256);w.Text(p.map_sha256);w.Text(p.base_manifest_sha256);w.Text(p.base_regions_sha256);w.Text(p.source_face_inventory_sha256);w.Word(uint32_t(p.slabs.size()));
 auto encoded=RegionWire::EncodeBootstrap({*p.source_envelope});w.Word(uint32_t(encoded.size()));w.bytes.insert(w.bytes.end(),encoded.begin(),encoded.end());w.Word(uint32_t(p.source_faces.size()));for(const auto& face:p.source_faces){w.Word(face.axis);for(float f:face.interval)w.Float(f);for(float f:face.rectangle)w.Float(f);for(auto t:face.triangles)w.Word(t);}

 for(const auto& slab:p.slabs){auto bytes=RegionWire::EncodeBootstrap({slab.region});w.Word(uint32_t(bytes.size()));w.bytes.insert(w.bytes.end(),bytes.begin(),bytes.end());w.Word(slab.axis);w.Float(slab.plane);w.Word(uint32_t(slab.side));for(auto f:slab.rectangle)w.Float(f);for(auto t:slab.triangles)w.Word(t);}
 p.reviewed_sha256=Hash(w.bytes);if(!Prepare(base,p))return previous;return p;
}
// Default enrollment uses only the explicit reviewed original-source ledger.
// Missing/mismatched source families retain immutable V4; an explicitly supplied
// invalid inventory fails closed. Triangle count alone never enrolls IDs.
inline std::optional<Policy> PlanarReviewed(const InteriorContent::Profile&base,const PlanarInventory*inventory=nullptr){
 auto previous=OrientedReviewed(base);const bool automatic=inventory==nullptr;std::optional<PlanarInventory> enrolled;
 if(automatic){enrolled=OriginalPlanarInventory(base.route.zone_id);if(!enrolled)return previous;inventory=&*enrolled;}
 if(!previous||previous->revision!=4||!inventory->reviewed||inventory->zone_id!=base.route.zone_id||inventory->zone_short_name!=base.zone_short_name||inventory->asset_sha256!=base.client_asset_sha256||inventory->map_sha256!=base.collision_map_sha256)return automatic?previous:std::optional<Policy>{};
 auto p=*previous;p.revision=5;p.prior=std::make_shared<Policy>(*previous);p.planar_inventory_sha256=inventory->ledger_sha256;p.planar_source_triangle_count=inventory->source_triangle_count;p.planar_candidates=inventory->triangle_ids;p.planar_side_masks=inventory->side_masks;
 ContentBinding::Detail::Writer w;w.Text("EQEmuReviewedPlanarInteriorMountRegionsV5");w.Word(p.zone_id);w.Text(p.zone_short_name);w.Text(p.asset_sha256);w.Text(p.map_sha256);w.Text(p.base_manifest_sha256);w.Text(p.base_regions_sha256);w.Text(p.source_face_inventory_sha256);w.Word(uint32_t(p.slabs.size()));
 auto encoded=RegionWire::EncodeBootstrap({*p.source_envelope});w.Word(uint32_t(encoded.size()));w.bytes.insert(w.bytes.end(),encoded.begin(),encoded.end());w.Word(uint32_t(p.source_faces.size()));for(const auto&face:p.source_faces){w.Word(face.axis);for(float f:face.interval)w.Float(f);for(float f:face.rectangle)w.Float(f);for(auto id:face.triangles)w.Word(id);}
 for(const auto&slab:p.slabs){auto bytes=RegionWire::EncodeBootstrap({slab.region});w.Word(uint32_t(bytes.size()));w.bytes.insert(w.bytes.end(),bytes.begin(),bytes.end());w.Word(slab.axis);w.Float(slab.plane);w.Word(uint32_t(slab.side));for(float f:slab.rectangle)w.Float(f);for(auto id:slab.triangles)w.Word(id);}
 if(!AppendPlanarIdentity(w,p))return automatic?previous:std::optional<Policy>{};p.reviewed_sha256=Hash(w.bytes);if(!Prepare(base,p))return automatic?previous:std::optional<Policy>{};return p;
}
// Explicit V6 candidate enrollment only: no automatic source metadata or V5
// digest reinterpretation. Source mode masks must come from reviewed native math.
inline std::optional<Policy> CurvedReviewed(const InteriorContent::Profile&base,const CurvedInventory&inventory,CurvedMount::Limits limits={}){
 auto previous=PlanarReviewed(base,&inventory.source);if(!previous||previous->revision!=5)return {};
 auto p=*previous;p.revision=6;p.prior=std::make_shared<Policy>(*previous);p.curved_mode_masks=inventory.native_mode_masks;p.curved_limits=limits;
 ContentBinding::Detail::Writer w;w.Text(CurvedMount::PolicyTag);w.Word(p.zone_id);w.Text(p.zone_short_name);w.Text(p.asset_sha256);w.Text(p.map_sha256);w.Text(p.base_manifest_sha256);w.Text(p.base_regions_sha256);w.Text(p.source_face_inventory_sha256);w.Word(uint32_t(p.slabs.size()));
 auto encoded=RegionWire::EncodeBootstrap({*p.source_envelope});w.Word(uint32_t(encoded.size()));w.bytes.insert(w.bytes.end(),encoded.begin(),encoded.end());w.Word(uint32_t(p.source_faces.size()));for(const auto&face:p.source_faces){w.Word(face.axis);for(float f:face.interval)w.Float(f);for(float f:face.rectangle)w.Float(f);for(auto id:face.triangles)w.Word(id);}
 for(const auto&slab:p.slabs){auto bytes=RegionWire::EncodeBootstrap({slab.region});w.Word(uint32_t(bytes.size()));w.bytes.insert(w.bytes.end(),bytes.begin(),bytes.end());w.Word(slab.axis);w.Float(slab.plane);w.Word(uint32_t(slab.side));for(float f:slab.rectangle)w.Float(f);for(auto id:slab.triangles)w.Word(id);}
 if(!AppendPlanarIdentity(w,p))return {};p.reviewed_sha256=Hash(w.bytes);return Prepare(base,p)?std::optional<Policy>{p}:std::nullopt;
}
// New Add selects V6 only for this exact hash-bound source/mode family. Explicit
// legacy inventory calls and stored digests retain V5 and all older semantics.
inline std::optional<Policy> Reviewed(const InteriorContent::Profile&base,const PlanarInventory*inventory=nullptr){
 if(inventory)return PlanarReviewed(base,inventory);auto previous=PlanarReviewed(base);auto source=OriginalCurvedInventory(base.route.zone_id);if(!source)return previous;auto next=CurvedReviewed(base,*source);return next?next:previous;
}
// Pure native presentation projection; mutation uses locked DB metadata instead.
inline std::optional<InteriorContent::Input> CachedInput(const InteriorContent::Profile& room,const EQ::ItemData& d){
 if(!NpcItemMetadata::Ordinary(d)||d.HousingDefinitionID)return {};
 const auto* model=StaticFurniture::Lookup(d.ID,std::string(d.IDFile));if(!model||!StaticFurniture::Mounted(d.ID)||!Eligible(model->geometry))return {};
 InteriorContent::Input in;in.room=room;in.item={d.ID,model->model_id,d.Placeable,d.HousingMinimumScale,d.HousingMaximumScale,true,0};in.geometry=model->geometry;in.model_member_sha256=model->member_sha256;in.geometry_policy_revision=model->geometry_policy_revision;in.authenticated_current_house=in.native_metadata_verified=true;in.mount_flags_known=d.HousingPlacementFlagsKnown;
 auto& n=in.native_metadata;n.placeable=d.Placeable;n.ignore_collisions=d.HousingIgnoreCollisions;n.placement_type=d.HousingPlacementType;n.scale_min=d.HousingMinimumScale;n.scale_max=d.HousingMaximumScale;n.upkeep_id=d.HousingUpkeepID;n.max_per_estate=d.HousingMaxPerEstate;n.disable_rotation=d.HousingDisableRotation;n.disable_free_placement=d.HousingDisableFreePlacement;
 return in;
}
inline std::optional<InteriorContent::Profile> Bootstrap(const InteriorContent::Profile& base){auto policy=Reviewed(base);return policy?Prepare(base,*policy):std::optional<InteriorContent::Profile>{base};}
}
#endif
