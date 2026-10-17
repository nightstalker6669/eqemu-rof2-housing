#ifndef EQEMU_HOUSING_INTERIOR_WALL_SLAB_POLICY_H
#define EQEMU_HOUSING_INTERIOR_WALL_SLAB_POLICY_H
#include "interior_content_binding.h"
#include "mount_admission.h"
#include <memory>
namespace Housing::InteriorWallSlab {
// Separate, explicitly reviewed SERVER content. Not original Live region data.
// Enrollment requires an independently reviewed current room plus exact assets.
struct Slab {RegionWire::Region region;uint32_t axis=0;float plane=0;int32_t side=0;std::array<float,4> rectangle{};std::array<uint32_t,2> triangles{};};
struct Face {uint32_t axis=0;std::array<float,2> interval{};std::array<float,4> rectangle{};std::array<uint32_t,2> triangles{};};
struct Policy {
 bool reviewed=false;
 uint32_t revision=1; //1 immutable legacy;2 connected source-face technical regions
 std::shared_ptr<const Policy> prior; //stored-digest compatibility only
 uint32_t zone_id=0;std::string zone_short_name,asset_sha256,map_sha256,base_manifest_sha256,base_regions_sha256,reviewed_sha256;
 std::string source_face_inventory_sha256; //V2 exact original structural map quad inventory
 std::vector<Slab> slabs;
 std::optional<RegionWire::Region> source_envelope;std::vector<Face> source_faces;
};
using SourceBodyClear=std::function<bool(const PlacementContact::Vector&,const PlacementContact::Vector&,const std::array<uint32_t,2>&)>;
using SourcePatch=std::function<std::vector<uint32_t>(const PlacementContact::Vector&,const PlacementContact::Vector&,uint32_t,const std::array<uint32_t,2>&,const std::vector<uint32_t>&)>;
using SourceListBodyClear=std::function<bool(const PlacementContact::Vector&,const PlacementContact::Vector&,const std::vector<uint32_t>&)>;
using BodyClear=std::function<bool(const PlacementContact::Vector&,const PlacementContact::Vector&)>;
inline std::string Hash(const std::vector<uint8_t>& b){std::array<uint8_t,SHA256_DIGEST_LENGTH>d{};if(!SHA256(b.data(),b.size(),d.data()))throw std::invalid_argument("Wall policy SHA256");static constexpr char hex[]="0123456789abcdef";std::string out;for(auto v:d){out+=hex[v>>4];out+=hex[v&15];}return out;}
inline std::pair<PlacementContact::Vector,PlacementContact::Vector> Bounds(const RegionWire::Region& r){
 const auto& m=r.matrix;for(auto i:{1,2,3,4,6,7,8,9,11})if(m[i]!=0)throw std::invalid_argument("Wall slab requires axis box");if(m[15]!=1||m[0]<=0||m[5]<=0||m[10]<=0)throw std::invalid_argument("Wall slab extent");RegionWire::Detail::ValidateMatrix(m);
 return {{{m[13]-m[5],m[12]-m[0],m[14]-m[10]}},{{m[13]+m[5],m[12]+m[0],m[14]+m[10]}}};
}
inline bool Overlap(const RegionWire::Region& a,const RegionWire::Region& b){auto [lo,hi]=Bounds(a);auto [other_lo,other_hi]=Bounds(b);for(unsigned i=0;i<3;++i)if(std::min(hi[i],other_hi[i])<=std::max(lo[i],other_lo[i]))return false;return true;}
// Exact immutable base profile/regions + source map + each reviewed slab and
// its triangle witness enter the NEW identity. Existing Build(base) is unchanged.
inline std::optional<InteriorContent::Profile> Prepare(const InteriorContent::Profile& base,const Policy& policy){
 try{
  if(policy.revision<1||policy.revision>3||!policy.reviewed||!base.reviewed_room||(base.definition.type!=2&&base.definition.type!=3)||!OriginalInteriors::Matches(base.route.zone_id,base.zone_short_name)||policy.zone_id!=base.route.zone_id||policy.zone_short_name!=base.zone_short_name||policy.asset_sha256!=base.client_asset_sha256||policy.map_sha256!=base.collision_map_sha256||policy.base_manifest_sha256!=base.manifest_sha256||policy.base_regions_sha256!=Hash(InteriorContent::RegionPayload(base))||!ContentBinding::Detail::Hash(policy.reviewed_sha256)||(policy.revision>=2&&!ContentBinding::Detail::Hash(policy.source_face_inventory_sha256))||(policy.revision==1?policy.slabs.empty():!policy.source_envelope||policy.source_faces.empty()))return {};
  auto effective=base;const auto base_regions=InteriorContent::Regions(base);std::set<uint32_t> ids;for(const auto& r:base_regions)ids.insert(r.id);
  ContentBinding::Detail::Writer w;w.Text(policy.revision==1?"EQEmuReviewedInteriorAxisWallSlabsV1":(policy.revision==3?"EQEmuReviewedCoplanarInteriorMountRegionsV3":"EQEmuReviewedConnectedInteriorMountRegionsV2"));w.Word(policy.zone_id);w.Text(policy.zone_short_name);w.Text(policy.asset_sha256);w.Text(policy.map_sha256);w.Text(policy.base_manifest_sha256);w.Text(policy.base_regions_sha256);if(policy.revision>=2)w.Text(policy.source_face_inventory_sha256);w.Word(uint32_t(policy.slabs.size()));
  if(policy.revision==1){
  for(const auto& slab:policy.slabs){
   if((policy.revision!=1&&policy.revision!=2)||slab.axis>(policy.revision==1?1u:2u)||(slab.side!=-1&&slab.side!=1)||(policy.revision==1&&slab.triangles[0]==slab.triangles[1])||!std::isfinite(slab.plane)||!ids.insert(slab.region.id).second)return {};
   const auto [lo,hi]=Bounds(slab.region);const auto tangent=slab.axis==0?1u:0u;const auto vertical=slab.axis==2?1u:2u;
   // Technical supported subset: exact wall plane, <=4 units deep, <=16 unit
   // tangential windows wholly inside a reviewed source rectangle. Not Live limits.
   if((policy.revision==1?(slab.side<0?hi[slab.axis]:lo[slab.axis])!=slab.plane:(slab.side<0?hi[slab.axis]>slab.plane:lo[slab.axis]<slab.plane))||(policy.revision==1&&(hi[slab.axis]-lo[slab.axis]>4.00001f||hi[tangent]-lo[tangent]>16.00001f||hi[vertical]-lo[vertical]>16.00001f))||lo[tangent]<slab.rectangle[0]||hi[tangent]>slab.rectangle[1]||lo[vertical]<slab.rectangle[2]||hi[vertical]>slab.rectangle[3])return {};
   bool touches_base=false;for(const auto& r:base_regions)if(Overlap(slab.region,r)){touches_base=true;break;}if(!touches_base)return {};
   auto bytes=RegionWire::EncodeBootstrap({slab.region});w.Word(uint32_t(bytes.size()));w.bytes.insert(w.bytes.end(),bytes.begin(),bytes.end());w.Word(slab.axis);w.Float(slab.plane);w.Word(uint32_t(slab.side));for(auto f:slab.rectangle)w.Float(f);for(auto n:slab.triangles)w.Word(n);
   effective.additional_regions.push_back(slab.region);
  }
  }else if(policy.revision>=2){
   const auto [lo,hi]=Bounds(*policy.source_envelope);for(const auto& region:base_regions)(void)Bounds(region);if(!ids.insert(policy.source_envelope->id).second)return {};
   auto bytes=RegionWire::EncodeBootstrap({*policy.source_envelope});w.Word(uint32_t(bytes.size()));w.bytes.insert(w.bytes.end(),bytes.begin(),bytes.end());
   w.Word(uint32_t(policy.source_faces.size()));for(const auto& face:policy.source_faces){if(face.axis>2||face.triangles[0]==face.triangles[1]||!std::all_of(face.interval.begin(),face.interval.end(),[](float v){return std::isfinite(v);})||face.interval[0]>face.interval[1]||face.interval[1]-face.interval[0]>.001f||!std::all_of(face.rectangle.begin(),face.rectangle.end(),[](float v){return std::isfinite(v);})||face.rectangle[0]>=face.rectangle[1]||face.rectangle[2]>=face.rectangle[3])return {};w.Word(face.axis);for(float f:face.interval)w.Float(f);for(float f:face.rectangle)w.Float(f);for(auto t:face.triangles)w.Word(t);}
   effective.additional_regions.push_back(*policy.source_envelope);
   // Preserve legacy native region IDs as well as their stored identity.
   for(const auto& slab:policy.slabs){if(!ids.insert(slab.region.id).second)return {};auto legacy_bytes=RegionWire::EncodeBootstrap({slab.region});w.Word(uint32_t(legacy_bytes.size()));w.bytes.insert(w.bytes.end(),legacy_bytes.begin(),legacy_bytes.end());w.Word(slab.axis);w.Float(slab.plane);w.Word(uint32_t(slab.side));for(auto f:slab.rectangle)w.Float(f);for(auto t:slab.triangles)w.Word(t);effective.additional_regions.push_back(slab.region);}
  }else return {};

  if(Hash(w.bytes)!=policy.reviewed_sha256)return {};InteriorContent::RegionPayload(effective);effective.manifest_sha256=policy.reviewed_sha256;return effective;
 }catch(...){return {};}
}
struct Selection {InteriorContent::Input input;InteriorContent::Fingerprint fingerprint;bool wall_policy=false;uint32_t policy_revision=1;std::shared_ptr<const Policy> source_policy;};
// Read/move/remove/replay MUST pass the stored trusted digest. No loose item or
// profile match promotes legacy bindings. New Add alone may request the enrolled
// policy. No stored bytes are rewritten: migration is Return + fresh Add through
// the existing coordinator, or a separately reviewed journaled migration.
inline std::optional<Selection> Select(const InteriorContent::Input& base,const Policy* policy,const std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>>& stored={},bool new_add=false){
 if(stored&&policy&&policy->prior){auto previous=Select(base,policy->prior.get(),stored,false);if(previous)return previous;}
 const auto original=InteriorContent::Build(base);if(!original)return {};
 if((stored&&*stored==original.value.digest)||(!stored&&!new_add))return Selection{base,original.value,false};
 if(!policy||base.npc||!base.item.furniture_only||base.item.house_definition_type||(base.geometry.mounting_type!=1&&(base.geometry.mounting_type!=2||!policy||policy->revision<2))){if(!stored)return Selection{base,original.value,false};return {};}
 auto effective=Prepare(base.room,*policy);if(!effective){if(!stored&&!new_add)return Selection{base,original.value,false};return {};}
 auto input=base;input.room=std::move(*effective);auto fingerprint=InteriorContent::Build(input);if(!fingerprint)return {};
 if(stored?*stored!=fingerprint.value.digest:!new_add)return {};
 return Selection{std::move(input),std::move(fingerprint.value),true,policy->revision,policy->revision>=2?std::make_shared<Policy>(*policy):nullptr};
}
// The old base path stays exact. Extra slabs require BOTH genuine sphere-wall
// contact (even when Free is known enabled) and conservative actual-map body
// clearance. Neither callback grants current source, room, ACL or lease authority.
inline PlacementPolicy::Result Check(const InteriorContent::Profile& base,const Selection& selected,const PlacementPolicy::Context& context,const PlacementPolicy::Transform& pose,PlacementPolicy::Operation operation,const MountFlags::Flags& flags,const MountAdmission::Trace& trace,const BodyClear& body_clear,float default_heading=0,const SourceBodyClear& source_clear={},const SourcePatch& source_patch={},const SourceListBodyClear& source_list_clear={}){
 auto old=InteriorContent::CheckPlacement(base,context,selected.input.item,selected.input.geometry,pose,operation);if(old||!selected.wall_policy)return old;
 auto bad=old;if(old.error!=PlacementPolicy::Error::OutsideRegion&&old.error!=PlacementPolicy::Error::NumericallyAmbiguous)return bad;
 if(!trace||!body_clear||(selected.input.geometry.mounting_type!=1&&(selected.input.geometry.mounting_type!=2||selected.policy_revision<2))||selected.input.npc)return bad;
 auto wall_flags=flags;wall_flags.known=true;wall_flags.disable_free=true;wall_flags.disable_rotation=true;
 std::optional<PlacementContact::Contact> contact;auto witnessed_trace=[&](const auto& a,const auto& b){contact=trace(a,b);return contact;};
 if(!MountAdmission::Check(selected.input.geometry,pose,wall_flags,witnessed_trace,default_heading)||!contact)return bad;
 auto accepted=InteriorContent::CheckPlacement(selected.input.room,context,selected.input.item,selected.input.geometry,pose,operation);if(!accepted)return accepted;
 PlacementContact::Vector minimum{INFINITY,INFINITY,INFINITY},maximum{-INFINITY,-INFINITY,-INFINITY};
 const auto rotation=PlacementPolicy::Rotation(pose.heading,pose.pitch,pose.roll);PlacementPolicy::Point margin{};for(unsigned i=0;i<3;++i)margin[i]=selected.input.geometry.measured_local_error*pose.scale*(std::abs(rotation[i])+std::abs(rotation[3+i])+std::abs(rotation[6+i]));
 for(const auto& native:accepted.world_corners){const PlacementPolicy::Point server{native[1],native[0],native[2]},error{margin[1],margin[0],margin[2]};for(unsigned i=0;i<3;++i){minimum[i]=std::min(minimum[i],std::nextafter(float(server[i]-error[i]),-INFINITY));maximum[i]=std::max(maximum[i],std::nextafter(float(server[i]+error[i]),INFINITY));}}
 if(selected.policy_revision==1)return body_clear(minimum,maximum)?accepted:bad; //immutable legacy semantics
 if(!selected.source_policy||(selected.policy_revision==2?!source_clear:!source_patch||!source_list_clear))return bad;
 // Source-volume regions are native permission envelopes, not collision solids.
 // The original radius query uses UNROTATED thin extent. Its quantized normal
 // heading can put oriented renderer corners beyond the exact contact plane.
 // Never invent padding: exempt exactly the trusted contacted triangle pair,
 // then test the WHOLE conservative body and its WHOLE axis-normal connection
 // into one authenticated base box against every other actual map triangle.
 const auto point=contact->point;const auto normal=contact->normal;
 for(const auto& face:selected.source_policy->source_faces){
  const unsigned axis=face.axis;unsigned tangent[2],n=0;for(unsigned k=0;k<3;++k)if(k!=axis)tangent[n++]=k;
  if((selected.input.geometry.mounting_type==1&&axis==2)||(selected.input.geometry.mounting_type==2&&axis!=2)||std::abs(normal[axis])<.999999f)continue;
  bool same=true;for(unsigned k=0;k<3;++k)if(k!=axis&&normal[k]*normal[k]>1-.999999f*.999999f)same=false;
  // Contact point must lie on the exact source face, including finite rectangle.
  const float tolerance=std::max(8*std::max(std::abs(std::nextafter(point[axis],INFINITY)-point[axis]),std::abs(point[axis]-std::nextafter(point[axis],-INFINITY))),float(4*selected.input.geometry.measured_local_error*pose.scale));
  if(!same||point[axis]<face.interval[0]-tolerance||point[axis]>face.interval[1]+tolerance||point[tangent[0]]<face.rectangle[0]||point[tangent[0]]>face.rectangle[1]||point[tangent[1]]<face.rectangle[2]||point[tangent[1]]>face.rectangle[3])continue;
  // The complete tangential body footprint must lie over the proved finite
  // source pair. Ignore exactly those2 actual triangles, never a half-space.
  std::vector<uint32_t> patch;
  if(selected.policy_revision==3){
   // Only hash-bound original structural source-face IDs may be considered.
   // The actual Map mesh proves exact positive-length source-edge connectivity, common axis
   // band/orientation and full triangle-union footprint, including every gap.
   std::vector<uint32_t> candidates;for(const auto& other:selected.source_policy->source_faces)if(other.axis==face.axis&&other.interval==face.interval)for(auto id:other.triangles)candidates.push_back(id);
   std::sort(candidates.begin(),candidates.end());candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
   patch=source_patch(minimum,maximum,axis,face.triangles,candidates);
   if(patch.empty()||!source_list_clear(minimum,maximum,patch))continue;
  }else if(minimum[tangent[0]]<face.rectangle[0]||maximum[tangent[0]]>face.rectangle[1]||minimum[tangent[1]]<face.rectangle[2]||maximum[tangent[1]]>face.rectangle[3]||!source_clear(minimum,maximum,face.triangles))continue;
  const auto& inner_min=minimum;const auto& inner_max=maximum;
  for(const auto& region:InteriorContent::Regions(base)){
   auto [lo,hi]=Bounds(region);bool fit=true;for(unsigned k=0;k<3;++k)if(k!=axis&&(inner_min[k]<lo[k]||inner_max[k]>hi[k]))fit=false;
   const float width=inner_max[axis]-inner_min[axis];if(!fit||width>hi[axis]-lo[axis])continue;
   const float shift=inner_min[axis]<lo[axis]?lo[axis]-inner_min[axis]:inner_max[axis]>hi[axis]?hi[axis]-inner_max[axis]:0;
   // A connection must remain on the inward normal side of the source face.
   if(shift*normal[axis]<0)continue;
   auto start=inner_min,end=inner_max;start[axis]=std::min(inner_min[axis],inner_min[axis]+shift);end[axis]=std::max(inner_max[axis],inner_max[axis]+shift);
   if(inner_min[axis]+shift<lo[axis]||inner_max[axis]+shift>hi[axis])continue;
   if(selected.policy_revision==3?source_list_clear(start,end,patch):source_clear(start,end,face.triangles))return accepted;
  }
 }
 return bad;
}
}
#endif
