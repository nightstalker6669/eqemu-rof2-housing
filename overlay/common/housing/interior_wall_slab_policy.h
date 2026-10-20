#ifndef EQEMU_HOUSING_INTERIOR_WALL_SLAB_POLICY_H
#define EQEMU_HOUSING_INTERIOR_WALL_SLAB_POLICY_H
#include "interior_content_binding.h"
#include "mount_admission.h"
#include "oriented_mount_volume.h"
#include "planar_mount_patch.h"
#include "curved_mount_support.h"
#include <memory>
namespace Housing::InteriorWallSlab {
// Separate, explicitly reviewed SERVER content. Not original Live region data.
// Enrollment requires an independently reviewed current room plus exact assets.
struct Slab {RegionWire::Region region;uint32_t axis=0;float plane=0;int32_t side=0;std::array<float,4> rectangle{};std::array<uint32_t,2> triangles{};};
struct Face {uint32_t axis=0;std::array<float,2> interval{};std::array<float,4> rectangle{};std::array<uint32_t,2> triangles{};};
// Explicit reviewed original-source ledger; absence does not enroll all mesh IDs.
// Side masks are 1=positive raw-map side, 2=negative, 3=both independently proved.
struct PlanarInventory {
 bool reviewed=false;uint32_t zone_id=0,source_triangle_count=0;
 std::string zone_short_name,asset_sha256,map_sha256,ledger_sha256;
 std::vector<uint32_t> triangle_ids;std::vector<uint8_t> side_masks;
};
struct CurvedInventory {PlanarInventory source;std::vector<uint8_t> native_mode_masks;};
struct Policy {
 bool reviewed=false;
 uint32_t revision=1; //1 legacy;2 pairs;3 connected patches;4 oriented;5 exact general planar
 std::shared_ptr<const Policy> prior; //stored-digest compatibility only
 uint32_t zone_id=0;std::string zone_short_name,asset_sha256,map_sha256,base_manifest_sha256,base_regions_sha256,reviewed_sha256;
 std::string source_face_inventory_sha256; //V2 exact original structural map quad inventory
 std::vector<Slab> slabs;
 std::optional<RegionWire::Region> source_envelope;std::vector<Face> source_faces;
 std::string planar_inventory_sha256;uint32_t planar_source_triangle_count=0;
 std::vector<uint32_t> planar_candidates;std::vector<uint8_t> planar_side_masks;
 std::vector<uint8_t> curved_mode_masks;CurvedMount::Limits curved_limits{};
};
using SourceBodyClear=std::function<bool(const PlacementContact::Vector&,const PlacementContact::Vector&,const std::array<uint32_t,2>&)>;
using SourcePatch=std::function<std::vector<uint32_t>(const PlacementContact::Vector&,const PlacementContact::Vector&,uint32_t,const std::array<uint32_t,2>&,const std::vector<uint32_t>&)>;
using SourceListBodyClear=std::function<bool(const PlacementContact::Vector&,const PlacementContact::Vector&,const std::vector<uint32_t>&)>;
using OrientedPatch=std::function<std::vector<uint32_t>(const OrientedMount::Volume&,uint32_t,const std::array<uint32_t,2>&,const std::vector<uint32_t>&)>;
using OrientedClear=std::function<bool(const OrientedMount::Volume&,const std::vector<uint32_t>&)>;
using PlanarTrace=std::function<std::optional<PlanarMount::Trace>(const PlacementContact::Vector&,const PlacementContact::Vector&)>;
using PlanarPatch=std::function<std::vector<uint32_t>(const OrientedMount::Volume&,const PlanarMount::SurfaceSeed&,const std::vector<uint32_t>&)>;
using CurvedPatch=std::function<CurvedMount::Proof(const OrientedMount::Volume&,const PlanarMount::SurfaceSeed&,const std::vector<CurvedMount::Entry>&,CurvedMount::Limits)>;
using BodyClear=std::function<bool(const PlacementContact::Vector&,const PlacementContact::Vector&)>;
inline std::string Hash(const std::vector<uint8_t>& b){std::array<uint8_t,SHA256_DIGEST_LENGTH>d{};if(!SHA256(b.data(),b.size(),d.data()))throw std::invalid_argument("Wall policy SHA256");static constexpr char hex[]="0123456789abcdef";std::string out;for(auto v:d){out+=hex[v>>4];out+=hex[v&15];}return out;}
inline bool AppendPlanarIdentity(ContentBinding::Detail::Writer&w,const Policy&p){
 if(p.revision!=5&&p.revision!=6)return true;
 if(!ContentBinding::Detail::Hash(p.planar_inventory_sha256)||!p.planar_source_triangle_count||p.planar_candidates.empty()||p.planar_candidates.size()>UINT32_MAX||p.planar_side_masks.size()!=p.planar_candidates.size()||!std::is_sorted(p.planar_candidates.begin(),p.planar_candidates.end())||std::adjacent_find(p.planar_candidates.begin(),p.planar_candidates.end())!=p.planar_candidates.end()||p.planar_candidates.back()>=p.planar_source_triangle_count)return false;
 w.Text(p.planar_inventory_sha256);w.Word(p.planar_source_triangle_count);w.Word(uint32_t(p.planar_candidates.size()));for(size_t i=0;i<p.planar_candidates.size();++i){if(!p.planar_side_masks[i]||p.planar_side_masks[i]>3)return false;w.Word(p.planar_candidates[i]);w.Word(p.planar_side_masks[i]);}
 if(p.revision==6){if(!std::isfinite(p.curved_limits.search_margin)||p.curved_limits.search_margin<=0||p.curved_limits.search_margin>8||p.curved_mode_masks.size()!=p.planar_candidates.size()||!p.curved_limits.source||p.curved_limits.source>65536||!p.curved_limits.corridors||p.curved_limits.corridors>32||!p.curved_limits.component||p.curved_limits.component>4096||!p.curved_limits.relevant||p.curved_limits.relevant>128||p.planar_candidates.size()>p.curved_limits.source)return false;w.Text(CurvedMount::PolicyTag);w.Word(p.curved_limits.source);w.Word(p.curved_limits.component);w.Word(p.curved_limits.relevant);w.Word(p.curved_limits.corridors);w.Float(p.curved_limits.search_margin);for(auto mask:p.curved_mode_masks){if(!mask||(mask&~3))return false;w.Word(mask);}}return true;
}
inline std::pair<PlacementContact::Vector,PlacementContact::Vector> Bounds(const RegionWire::Region& r){
 const auto& m=r.matrix;for(auto i:{1,2,3,4,6,7,8,9,11})if(m[i]!=0)throw std::invalid_argument("Wall slab requires axis box");if(m[15]!=1||m[0]<=0||m[5]<=0||m[10]<=0)throw std::invalid_argument("Wall slab extent");RegionWire::Detail::ValidateMatrix(m);
 return {{{m[13]-m[5],m[12]-m[0],m[14]-m[10]}},{{m[13]+m[5],m[12]+m[0],m[14]+m[10]}}};
}
inline bool Overlap(const RegionWire::Region& a,const RegionWire::Region& b){auto [lo,hi]=Bounds(a);auto [other_lo,other_hi]=Bounds(b);for(unsigned i=0;i<3;++i)if(std::min(hi[i],other_hi[i])<=std::max(lo[i],other_lo[i]))return false;return true;}
// Exact immutable base profile/regions + source map + each reviewed slab and
// its triangle witness enter the NEW identity. Existing Build(base) is unchanged.
inline std::optional<InteriorContent::Profile> Prepare(const InteriorContent::Profile& base,const Policy& policy){
 try{
  if(policy.revision<1||policy.revision>6||!policy.reviewed||!base.reviewed_room||(base.definition.type!=2&&base.definition.type!=3)||!OriginalInteriors::Matches(base.route.zone_id,base.zone_short_name)||policy.zone_id!=base.route.zone_id||policy.zone_short_name!=base.zone_short_name||policy.asset_sha256!=base.client_asset_sha256||policy.map_sha256!=base.collision_map_sha256||policy.base_manifest_sha256!=base.manifest_sha256||policy.base_regions_sha256!=Hash(InteriorContent::RegionPayload(base))||!ContentBinding::Detail::Hash(policy.reviewed_sha256)||(policy.revision>=2&&!ContentBinding::Detail::Hash(policy.source_face_inventory_sha256))||(policy.revision==1?policy.slabs.empty():!policy.source_envelope||policy.source_faces.empty()))return {};
  auto effective=base;const auto base_regions=InteriorContent::Regions(base);std::set<uint32_t> ids;for(const auto& r:base_regions)ids.insert(r.id);
  ContentBinding::Detail::Writer w;w.Text(policy.revision==1?"EQEmuReviewedInteriorAxisWallSlabsV1":(policy.revision==6?CurvedMount::PolicyTag:policy.revision==5?"EQEmuReviewedPlanarInteriorMountRegionsV5":policy.revision==4?"EQEmuReviewedOrientedInteriorMountRegionsV4":policy.revision==3?"EQEmuReviewedCoplanarInteriorMountRegionsV3":"EQEmuReviewedConnectedInteriorMountRegionsV2"));w.Word(policy.zone_id);w.Text(policy.zone_short_name);w.Text(policy.asset_sha256);w.Text(policy.map_sha256);w.Text(policy.base_manifest_sha256);w.Text(policy.base_regions_sha256);if(policy.revision>=2)w.Text(policy.source_face_inventory_sha256);w.Word(uint32_t(policy.slabs.size()));
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

  if(!AppendPlanarIdentity(w,policy))return {};
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
// Conservative server geometry, never incoming packet dimensions. Keep the
// recovered model center, all three rotated half-edges and their measured error.
// A projected convex footprint is smaller than its AABB without dropping corners.
inline std::optional<OrientedMount::Volume> OrientedBody(const PlacementPolicy::Geometry& g,const PlacementPolicy::Transform& p){
 try{
  if(!g.native_collision_verified||!std::isfinite(p.scale)||p.scale<=0||!PlacementContact::Finite(p.position)||!std::isfinite(g.measured_local_error)||g.measured_local_error<0)return {};
  for(float a:{p.heading,p.pitch,p.roll})if(!std::isfinite(a)||a<0||a>512)return {};
  for(unsigned a=0;a<3;++a)if(!std::isfinite(g.center[a])||!std::isfinite(g.extents[a])||g.extents[a]<=0)return {};
  const auto rotation=PlacementPolicy::Rotation(p.heading,p.pitch,p.roll);
  PlacementPolicy::Point c=g.center;for(auto& f:c)f*=p.scale;c=PlacementPolicy::Rotate(c,rotation);
  OrientedMount::Volume v;v.count=6;
  for(unsigned a=0;a<3;++a){const unsigned native=a==0?1:a==1?0:2;v.center[a]=c[native]+p.position[native];
   double radius=0,norm=0;for(unsigned edge=0;edge<3;++edge){v.generators[edge][a]=g.extents[edge]*p.scale*rotation[edge*3+native];radius+=std::abs(v.generators[edge][a]);norm+=std::abs(rotation[edge*3+native]);}
   const float magnitude=float(std::abs(v.center[a])+radius);if(!std::isfinite(magnitude))return {};
   const double ulp=double(std::nextafter(magnitude,INFINITY))-magnitude;
   // One binary32 outward-coordinate step matches the predecessor enclosure;
   // the double operation bound protects this helper's center/edge arithmetic.
   const double error=g.measured_local_error*p.scale*norm+ulp+64*std::numeric_limits<double>::epsilon()*(1+std::abs(v.center[a])+radius);
   v.generators[3+a][a]=std::nextafter(error,INFINITY);
  }
  if(!OrientedMount::Valid(v))return {};return v;
 }catch(...){return {};}
}
// Exact full inward translation interval into ONE existing base box. Rounded
// center/sweep generators are enclosed explicitly, then endpoint fit is proved
// again with exact rational supports. No epsilon can bridge a geometry gap.
inline std::optional<OrientedMount::Volume> PlanarCorridor(const OrientedMount::Volume&body,const PlacementContact::Vector&normal,const RegionWire::Region&region){
 try{using R=OrientedMount::Rational;if(!OrientedMount::Valid(body)||body.count!=6||!PlacementContact::Finite(normal))return {};Bounds(region);
  std::array<R,3>lo,hi,minimum,maximum,direction;R lower=0;std::optional<R>upper;
  for(unsigned k=0;k<3;++k){const unsigned native=k==0?1:k==1?0:2;const auto&m=region.matrix;lo[k]=OrientedMount::Exact(double(m[12+native]))-OrientedMount::Exact(double(m[5*native]));hi[k]=OrientedMount::Exact(double(m[12+native]))+OrientedMount::Exact(double(m[5*native]));
   R radius=0;for(unsigned j=0;j<body.count;++j)radius+=OrientedMount::Abs(OrientedMount::Exact(body.generators[j][k]));minimum[k]=OrientedMount::Exact(body.center[k])-radius;maximum[k]=OrientedMount::Exact(body.center[k])+radius;direction[k]=OrientedMount::Exact(double(normal[k]));
   if(direction[k]==0){if(minimum[k]<lo[k]||maximum[k]>hi[k])return {};continue;}R a=(lo[k]-minimum[k])/direction[k],b=(hi[k]-maximum[k])/direction[k];if(direction[k]<0)std::swap(a,b);lower=std::max(lower,a);if(!upper||b<*upper)upper=b;
  }
  if(!upper||*upper<lower)return {};R target=(lower+*upper)/2;const double time=target.convert_to<double>();if(!std::isfinite(time))return {};const R exact_time=OrientedMount::Exact(time);if(exact_time<lower||exact_time>*upper||exact_time<0)return {};
  auto corridor=body;bool swept=false;for(unsigned k=0;k<3;++k){for(unsigned j=3;j<6;++j)if(j!=3+k&&body.generators[j][k]!=0)return {};if(body.generators[3+k][k]<0)return {};
   const R half=direction[k]*exact_time/2,center=OrientedMount::Exact(body.center[k])+half;const double c=center.convert_to<double>(),g=half.convert_to<double>();if(!std::isfinite(c)||!std::isfinite(g))return {};corridor.center[k]=c;corridor.generators[6][k]=g;swept=swept||g!=0;
   const R error=OrientedMount::Exact(body.generators[3+k][k])+OrientedMount::Abs(OrientedMount::Exact(c)-center)+OrientedMount::Abs(OrientedMount::Exact(g)-half);double expanded=error.convert_to<double>();if(!std::isfinite(expanded))return {};while(OrientedMount::Exact(expanded)<error)expanded=std::nextafter(expanded,INFINITY);corridor.generators[3+k][k]=expanded;
   R radius=0;for(unsigned j=0;j<6;++j)radius+=OrientedMount::Abs(OrientedMount::Exact(corridor.generators[j][k]));const R endpoint=OrientedMount::Exact(c)+OrientedMount::Exact(g);if(endpoint-radius<lo[k]||endpoint+radius>hi[k])return {};
  }
  if(swept)corridor.count=7;else{for(unsigned k=0;k<3;++k)if(direction[k]*exact_time!=0)return {};}
  return OrientedMount::Valid(corridor)?std::optional<OrientedMount::Volume>{corridor}:std::nullopt;
 }catch(...){return {};}
}
inline PlacementPolicy::Result CheckPlanar(const InteriorContent::Profile&base,const Selection&selected,const PlacementPolicy::Context&context,const PlacementPolicy::Transform&pose,PlacementPolicy::Operation operation,const MountFlags::Flags&flags,float default_heading,const PlanarTrace&trace,const PlanarPatch&patcher,const OrientedClear&clear,PlacementPolicy::Result bad){
 if(!trace||!patcher||!clear||!selected.source_policy||selected.source_policy->revision!=5||selected.input.npc||(selected.input.geometry.mounting_type!=1&&selected.input.geometry.mounting_type!=2))return bad;
 const auto&policy=*selected.source_policy;ContentBinding::Detail::Writer identity;if(!AppendPlanarIdentity(identity,policy))return bad;
 auto wall_flags=flags;wall_flags.known=true;wall_flags.disable_free=true;wall_flags.disable_rotation=true;
 std::optional<PlanarMount::Trace>witness;auto contact_trace=[&](const auto&a,const auto&b)->std::optional<PlacementContact::Contact>{witness=trace(a,b);return witness?std::optional<PlacementContact::Contact>{witness->contact}:std::nullopt;};
 if(!MountAdmission::Check(selected.input.geometry,pose,wall_flags,contact_trace,default_heading)||!witness)return bad;
 const auto&seed=witness->seed;if(seed.side!=1&&seed.side!=-1)return bad;auto position=std::lower_bound(policy.planar_candidates.begin(),policy.planar_candidates.end(),seed.triangle_id);if(position==policy.planar_candidates.end()||*position!=seed.triangle_id)return bad;const auto index=size_t(position-policy.planar_candidates.begin());if(!(policy.planar_side_masks[index]&(seed.side>0?1u:2u)))return bad;
 const auto&normal=witness->contact.normal;if(!PlacementContact::Finite(normal)||std::abs(PlacementContact::Dot(normal,normal)-1)>1e-5f)return bad;
 auto accepted=InteriorContent::CheckPlacement(selected.input.room,context,selected.input.item,selected.input.geometry,pose,operation);if(!accepted)return accepted;auto body=OrientedBody(selected.input.geometry,pose);if(!body)return bad;
 auto patch=patcher(*body,seed,policy.planar_candidates);if(patch.empty()||!std::is_sorted(patch.begin(),patch.end())||std::adjacent_find(patch.begin(),patch.end())!=patch.end()||!std::binary_search(patch.begin(),patch.end(),seed.triangle_id)||!std::includes(policy.planar_candidates.begin(),policy.planar_candidates.end(),patch.begin(),patch.end()) )return bad;
 for(auto id:patch){const auto entry=std::lower_bound(policy.planar_candidates.begin(),policy.planar_candidates.end(),id);if(entry==policy.planar_candidates.end()||*entry!=id||!(policy.planar_side_masks[size_t(entry-policy.planar_candidates.begin())]&(seed.side>0?1u:2u)))return bad;}
 if(!clear(*body,patch))return bad;
 for(const auto&region:InteriorContent::Regions(base)){auto corridor=PlanarCorridor(*body,normal,region);if(corridor&&clear(*corridor,patch))return accepted;}return bad;
}
// Exact fit for each old authenticated box; rank only for bounded work order.
// Every accepted corridor still undergoes complete exact full-map collision.
inline std::vector<OrientedMount::Volume> CurvedCorridors(const OrientedMount::Volume&body,const PlacementContact::Vector&normal,const InteriorContent::Profile&base,uint32_t maximum){
 std::vector<std::pair<double,OrientedMount::Volume>>nearest;if(!maximum||maximum>32)return {};
 for(const auto&region:InteriorContent::Regions(base)){auto corridor=PlanarCorridor(body,normal,region);if(!corridor)continue;double score=0;for(unsigned k=0;k<3;++k){const double d=corridor->center[k]-body.center[k];score+=d*d;}if(!std::isfinite(score))continue;auto at=std::upper_bound(nearest.begin(),nearest.end(),score,[](double s,const auto&p){return s<p.first;});nearest.insert(at,{score,*corridor});if(nearest.size()>maximum)nearest.pop_back();}
 std::vector<OrientedMount::Volume>out;for(auto&entry:nearest)out.push_back(entry.second);return out;
}
// V6 is a conservative server-designed graph support proof, not recovered Live
// curved placement semantics. Only Wall extends here. V5 is attempted verbatim.
inline PlacementPolicy::Result CheckCurved(const InteriorContent::Profile&base,const Selection&selected,const PlacementPolicy::Context&context,const PlacementPolicy::Transform&pose,PlacementPolicy::Operation operation,const MountFlags::Flags&flags,float default_heading,const PlanarTrace&trace,const PlanarPatch&planar,const CurvedPatch&patcher,const OrientedClear&clear,PlacementPolicy::Result bad){
 if(!selected.source_policy||selected.source_policy->revision!=6)return bad;const auto&p=*selected.source_policy;
 if(!p.prior||p.prior->revision!=5)return bad;
 auto predecessor=selected;predecessor.source_policy=p.prior;predecessor.policy_revision=5;
 auto old=CheckPlanar(base,predecessor,context,pose,operation,flags,default_heading,trace,planar,clear,bad);if(old)return old;
 if(!trace||!patcher||!clear||selected.input.npc||selected.input.geometry.mounting_type!=1)return bad;
 ContentBinding::Detail::Writer identity;if(!AppendPlanarIdentity(identity,p))return bad;
 auto wall_flags=flags;wall_flags.known=true;wall_flags.disable_free=true;wall_flags.disable_rotation=true;
 std::optional<PlanarMount::Trace>witness;auto contact_trace=[&](const auto&a,const auto&b)->std::optional<PlacementContact::Contact>{witness=trace(a,b);return witness?std::optional<PlacementContact::Contact>{witness->contact}:std::nullopt;};
 if(!MountAdmission::Check(selected.input.geometry,pose,wall_flags,contact_trace,default_heading)||!witness)return bad;
 const auto seed=witness->seed;if(seed.side!=1&&seed.side!=-1)return bad;const auto&normal=witness->contact.normal;if(!PlacementContact::Finite(normal)||std::abs(PlacementContact::Dot(normal,normal)-1)>1e-5f)return bad;
 auto accepted=InteriorContent::CheckPlacement(selected.input.room,context,selected.input.item,selected.input.geometry,pose,operation);if(!accepted)return accepted;auto body=OrientedBody(selected.input.geometry,pose);if(!body)return bad;
 std::vector<CurvedMount::Entry>entries;std::vector<uint32_t>eligible;for(size_t i=0;i<p.planar_candidates.size();++i)if((p.curved_mode_masks[i]&1)&&(p.planar_side_masks[i]&(seed.side>0?1u:2u))){entries.push_back({p.planar_candidates[i],p.planar_side_masks[i],p.curved_mode_masks[i]});eligible.push_back(p.planar_candidates[i]);}
 if(!std::binary_search(eligible.begin(),eligible.end(),seed.triangle_id))return bad;
 auto proof=patcher(*body,seed,entries,p.curved_limits);if(!proof)return bad;
 for(const auto*ids:{&proof.support_ids,&proof.collision_exempt_ids})if(ids->empty()||!std::is_sorted(ids->begin(),ids->end())||std::adjacent_find(ids->begin(),ids->end())!=ids->end()||!std::includes(eligible.begin(),eligible.end(),ids->begin(),ids->end()))return bad;
 if(!std::binary_search(proof.collision_exempt_ids.begin(),proof.collision_exempt_ids.end(),seed.triangle_id)||!clear(*body,proof.collision_exempt_ids))return bad;
 for(const auto&corridor:CurvedCorridors(*body,normal,base,p.curved_limits.corridors))if(clear(corridor,proof.collision_exempt_ids))return accepted;return bad;
}
// The old base path stays exact. Extra slabs require BOTH genuine sphere-wall
// contact (even when Free is known enabled) and conservative actual-map body
// clearance. Neither callback grants current source, room, ACL or lease authority.
inline PlacementPolicy::Result Check(const InteriorContent::Profile& base,const Selection& selected,const PlacementPolicy::Context& context,const PlacementPolicy::Transform& pose,PlacementPolicy::Operation operation,const MountFlags::Flags& flags,const MountAdmission::Trace& trace,const BodyClear& body_clear,float default_heading=0,const SourceBodyClear& source_clear={},const SourcePatch& source_patch={},const SourceListBodyClear& source_list_clear={},const OrientedPatch& oriented_patch={},const OrientedClear& oriented_clear={},const PlanarTrace& planar_trace={},const PlanarPatch& planar_patch={},const CurvedPatch& curved_patch={}){
 auto old=InteriorContent::CheckPlacement(base,context,selected.input.item,selected.input.geometry,pose,operation);if(old||!selected.wall_policy)return old;
 auto bad=old;if(old.error!=PlacementPolicy::Error::OutsideRegion&&old.error!=PlacementPolicy::Error::NumericallyAmbiguous)return bad;
 if(selected.policy_revision==6)return CheckCurved(base,selected,context,pose,operation,flags,default_heading,planar_trace,planar_patch,curved_patch,oriented_clear,bad);
 if(selected.policy_revision==5)return CheckPlanar(base,selected,context,pose,operation,flags,default_heading,planar_trace,planar_patch,oriented_clear,bad);
 if(!trace||!body_clear||(selected.input.geometry.mounting_type!=1&&(selected.input.geometry.mounting_type!=2||selected.policy_revision<2))||selected.input.npc)return bad;
 auto wall_flags=flags;wall_flags.known=true;wall_flags.disable_free=true;wall_flags.disable_rotation=true;
 std::optional<PlacementContact::Contact> contact;auto witnessed_trace=[&](const auto& a,const auto& b){contact=trace(a,b);return contact;};
 if(!MountAdmission::Check(selected.input.geometry,pose,wall_flags,witnessed_trace,default_heading)||!contact)return bad;
 auto accepted=InteriorContent::CheckPlacement(selected.input.room,context,selected.input.item,selected.input.geometry,pose,operation);if(!accepted)return accepted;
 PlacementContact::Vector minimum{INFINITY,INFINITY,INFINITY},maximum{-INFINITY,-INFINITY,-INFINITY};
 const auto rotation=PlacementPolicy::Rotation(pose.heading,pose.pitch,pose.roll);PlacementPolicy::Point margin{};for(unsigned i=0;i<3;++i)margin[i]=selected.input.geometry.measured_local_error*pose.scale*(std::abs(rotation[i])+std::abs(rotation[3+i])+std::abs(rotation[6+i]));
 for(const auto& native:accepted.world_corners){const PlacementPolicy::Point server{native[1],native[0],native[2]},error{margin[1],margin[0],margin[2]};for(unsigned i=0;i<3;++i){minimum[i]=std::min(minimum[i],std::nextafter(float(server[i]-error[i]),-INFINITY));maximum[i]=std::max(maximum[i],std::nextafter(float(server[i]+error[i]),INFINITY));}}
 if(selected.policy_revision==1)return body_clear(minimum,maximum)?accepted:bad; //immutable legacy semantics
 if(!selected.source_policy||(selected.policy_revision==2?!source_clear:selected.policy_revision==4?!oriented_patch||!oriented_clear:!source_patch||!source_list_clear))return bad;
 std::optional<OrientedMount::Volume> oriented;
 if(selected.policy_revision==4){oriented=OrientedBody(selected.input.geometry,pose);if(!oriented)return bad;try{auto bounds=OrientedMount::Bounds(*oriented);minimum=bounds.first;maximum=bounds.second;}catch(...){return bad;}}
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
  if(selected.policy_revision>=3){
   // Only hash-bound original structural source-face IDs may be considered.
   // The actual Map mesh proves exact positive-length source-edge connectivity, common axis
   // band/orientation and full triangle-union footprint, including every gap.
   std::vector<uint32_t> candidates;for(const auto& other:selected.source_policy->source_faces)if(other.axis==face.axis&&other.interval==face.interval)for(auto id:other.triangles)candidates.push_back(id);
   std::sort(candidates.begin(),candidates.end());candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
   patch=selected.policy_revision==4?oriented_patch(*oriented,axis,face.triangles,candidates):source_patch(minimum,maximum,axis,face.triangles,candidates);
   if(patch.empty()||(selected.policy_revision==4?!oriented_clear(*oriented,patch):!source_list_clear(minimum,maximum,patch)))continue;
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
   if(selected.policy_revision==4){
    auto corridor=*oriented;
    // Exact full translation sweep: original body plus the complete inward
    // segment, not samples and not an AABB-filled corner volume.
    if(shift!=0){corridor.center[axis]+=double(shift)/2;corridor.generators[corridor.count++][axis]=double(shift)/2;
     const double rounding=8*std::numeric_limits<double>::epsilon()*(1+std::abs(corridor.center[axis])+std::abs(double(shift)));
     corridor.generators[3+axis][axis]+=rounding;}
    if(oriented_clear(corridor,patch))return accepted;
   }else if(selected.policy_revision==3?source_list_clear(start,end,patch):source_clear(start,end,face.triangles))return accepted;
  }
 }
 return bad;
}
}
#endif
