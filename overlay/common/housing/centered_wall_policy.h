#ifndef EQEMU_HOUSING_CENTERED_WALL_POLICY_H
#define EQEMU_HOUSING_CENTERED_WALL_POLICY_H
#include "interior_wall_slab_policy.h"
#include "selected_wall_cache.h"
#include "original_placement_frame.h"
#include "original_wall_frame.h"
namespace Housing::CenteredWall {
inline constexpr const char* PolicyTag="EQEmuReviewedCenteredInteriorWallRegionsV7";
inline constexpr uint32_t Revision=7,ContactRevision=8,MaximumRayQueries=256,CoordinateProbes=4;
inline constexpr float Pullback=0.0010000000474974513f;
using Selection=InteriorWallSlab::Selection;
inline bool Eligible(const InteriorContent::Input& input){
 const auto* source=SelectedWallCache::Source(input.item.item_id);
 return source&&!input.npc&&input.item.furniture_only&&!input.item.house_definition_type&&input.geometry.mounting_type==1&&input.model_member_sha256==source->member_sha256&&std::any_of(input.geometry.center.begin(),input.geometry.center.end(),[](double c){return c!=0;});
}
// V8 uses the same authenticated ordinary Wall resource class for both zero
// and nonzero centers. V7 enrollment remains immutable for stored replay.
inline bool ContactEligible(const InteriorContent::Input& input){
 const auto* source=SelectedWallCache::Source(input.item.item_id);
 return source&&!input.npc&&input.item.furniture_only&&!input.item.house_definition_type&&input.geometry.mounting_type==1&&input.model_member_sha256==source->member_sha256;
}
// New domain only. Immutable V6 and older region identities/semantics are never
// rewritten. V7 grants a bounded necessary server extension, not camera replay.
inline std::optional<InteriorContent::Profile> Prepare(const InteriorContent::Profile& base,const InteriorWallSlab::Policy& predecessor){
 if(predecessor.revision!=6)return {};auto effective=InteriorWallSlab::Prepare(base,predecessor);if(!effective)return {};
 ContentBinding::Detail::Writer w;w.Text(PolicyTag);w.Word(Revision);w.Text(predecessor.reviewed_sha256);w.Text(SelectedWallCache::CatalogSha256);w.Text("Original4907f0PC53ChopBoxAnd5587c6WallSubtractionV1");w.Text("FiniteCentrePreimagesMapPlaneNecessaryWitnessV1");w.Word(MaximumRayQueries);w.Word(CoordinateProbes);w.Float(Pullback);w.Float(.125f);w.Word(8);w.Word(4);effective->manifest_sha256=InteriorWallSlab::Hash(w.bytes);return effective;
}
// Separate immutable successor: original sphere-contact compatibility and full
// body bounds in the exact advertised room regions. Footprint coverage, inward
// travel and full-body geometry collision are not recovered native Wall tests.
inline std::optional<InteriorContent::Profile> PrepareContact(const InteriorContent::Profile& base,const InteriorWallSlab::Policy& predecessor){
 auto effective=CenteredWall::Prepare(base,predecessor);if(!effective)return {};
 ContentBinding::Detail::Writer w;w.Text("EQEmuReviewedNativeSphereContactInteriorWallRegionsV8");w.Word(ContactRevision);w.Text(effective->manifest_sha256);w.Text("OriginalWallNativeSphereContactAndRoomBoundsV1");effective->manifest_sha256=InteriorWallSlab::Hash(w.bytes);return effective;
}
inline std::optional<Selection> Select(const InteriorContent::Input& base,const InteriorWallSlab::Policy* policy,const std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>>& stored={},bool new_add=false){
 if(stored){auto old=InteriorWallSlab::Select(base,policy,stored,false);if(old)return old;}
 auto old=InteriorWallSlab::Select(base,policy,{},new_add||stored.has_value());
 if(!policy||!ContactEligible(base)||(!stored&&!new_add)||!old||!old->wall_policy||old->policy_revision!=6)return stored?std::nullopt:old;
 const auto make=[&](uint32_t revision)->std::optional<Selection>{if(revision==Revision&&!Eligible(base))return {};auto room=revision==Revision?CenteredWall::Prepare(base.room,*policy):CenteredWall::PrepareContact(base.room,*policy);if(!room)return {};auto input=base;input.room=std::move(*room);auto fingerprint=InteriorContent::Build(input);if(!fingerprint||stored&&*stored!=fingerprint.value.digest)return {};return Selection{std::move(input),std::move(fingerprint.value),true,revision,std::make_shared<const InteriorWallSlab::Policy>(*policy)};};
 if(stored){auto prior=make(Revision);if(prior)return prior;}
 return make(ContactRevision);
}
inline bool ValidSelection(const InteriorContent::Profile& base,const Selection& selected){
 if(!selected.wall_policy||selected.policy_revision!=Revision||!selected.source_policy||selected.source_policy->revision!=6||!Eligible(selected.input))return false;
 auto profile=CenteredWall::Prepare(base,*selected.source_policy);if(!profile)return false;const auto& p=selected.input.room;
 if(p.manifest_sha256!=profile->manifest_sha256||p.house_id!=profile->house_id||p.native_estate_id!=profile->native_estate_id||p.route.zone_id!=profile->route.zone_id||p.route.zone_version!=profile->route.zone_version||p.route.instance_id!=profile->route.instance_id)return false;
 try{if(InteriorContent::RegionPayload(p)!=InteriorContent::RegionPayload(*profile)||InteriorContent::DefinitionPayload(p)!=InteriorContent::DefinitionPayload(*profile))return false;auto current=InteriorContent::Build(selected.input);return current&&current.value.digest==selected.fingerprint.digest&&current.value.canonical_bytes==selected.fingerprint.canonical_bytes;}catch(...){return false;}
}
inline bool ValidContactSelection(const InteriorContent::Profile& base,const Selection& selected){
 if(!selected.wall_policy||selected.policy_revision!=ContactRevision||!selected.source_policy||selected.source_policy->revision!=6||!ContactEligible(selected.input))return false;
 auto profile=CenteredWall::PrepareContact(base,*selected.source_policy);if(!profile)return false;const auto& p=selected.input.room;
 if(p.manifest_sha256!=profile->manifest_sha256||p.house_id!=profile->house_id||p.native_estate_id!=profile->native_estate_id||p.route.zone_id!=profile->route.zone_id||p.route.zone_version!=profile->route.zone_version||p.route.instance_id!=profile->route.instance_id)return false;
 try{if(InteriorContent::RegionPayload(p)!=InteriorContent::RegionPayload(*profile)||InteriorContent::DefinitionPayload(p)!=InteriorContent::DefinitionPayload(*profile))return false;auto current=InteriorContent::Build(selected.input);return current&&current.value.digest==selected.fingerprint.digest&&current.value.canonical_bytes==selected.fingerprint.canonical_bytes;}catch(...){return false;}
}
// Callable complete extension evaluator. Callbacks must remain bound to the
// same authenticated complete original Map and fresh transaction/room selection.
// Q is an exact subtraction preimage, NOT a recovered transient camera input.
// Actual-map finite support, conservative body and complete inward corridor are
// independent mandatory proofs before any successful extension result escapes.
inline PlacementPolicy::Result Check(const InteriorContent::Profile& base,const Selection& selected,const PlacementPolicy::Context& context,const PlacementPolicy::Transform& pose,PlacementPolicy::Operation operation,const MountFlags::Flags& flags,const SelectedWallCache::Prepared& cache,const InteriorWallSlab::PlanarTrace& trace,const InteriorWallSlab::PlanarPatch& planar,const InteriorWallSlab::OrientedClear& clear,float default_heading=0,const InteriorWallSlab::CurvedPatch& curved={}){
 auto old=InteriorContent::CheckPlacement(base,context,selected.input.item,selected.input.geometry,pose,operation);auto bad=old;if(old)bad.error=PlacementPolicy::Error::OutsideRegion;
 if((!old&&bad.error!=PlacementPolicy::Error::OutsideRegion&&bad.error!=PlacementPolicy::Error::NumericallyAmbiguous)||!ValidSelection(base,selected)||!trace||!planar||!clear||cache.ItemId()!=selected.input.item.item_id||cache.MemberSha256()!=selected.input.model_member_sha256||!std::isfinite(default_heading)||default_heading<0||default_heading>512)return bad;
 try{const auto& policy=*selected.source_policy;OriginalPlacementFrame::Pose input{pose.scale,{pose.heading,pose.pitch,pose.roll},pose.position};auto frame=OriginalPlacementFrame::Build(cache.Cache(),input);if(!frame||!policy.source_envelope)return bad;
  const auto& extents=frame->boxes.scaled_extent;bool choose_y=extents[1]<extents[0];if(((uint32_t(default_heading)+64)/128)&1)choose_y=!choose_y;const float radius=extents[choose_y?1:0];if(!std::isfinite(radius)||radius<=0)return bad;
  float ulp=0;for(float p:pose.position)ulp=std::max(ulp,std::max(std::abs(std::nextafter(p,INFINITY)-p),std::abs(p-std::nextafter(p,-INFINITY))));const float tolerance=std::max(8*ulp,float(4*selected.input.geometry.measured_local_error*pose.scale)+8*std::numeric_limits<float>::epsilon());if(!std::isfinite(tolerance)||tolerance<=0||tolerance>.125f)return bad;
  auto geometry_context=context;geometry_context.geometry_only=true;auto accepted=InteriorContent::CheckPlacement(selected.input.room,geometry_context,selected.input.item,selected.input.geometry,pose,operation);if(!accepted)return accepted;auto body=InteriorWallSlab::OrientedBody(selected.input.geometry,pose);if(!body)return bad;
  const auto bounds=InteriorWallSlab::Bounds(*policy.source_envelope);OriginalSphereQuery::Point lo{bounds.first[1],bounds.first[0],bounds.first[2]},hi{bounds.second[1],bounds.second[0],bounds.second[2]};auto preimage=OriginalWallFrame::Preimages(pose.position,frame->boxes.frame,lo,hi);if(!preimage)return bad;
  std::array<std::vector<float>,3> coordinates;for(unsigned k=0;k<3;++k)coordinates[k]=OriginalHeightPreimage::Probes(preimage->coordinates[k],CoordinateProbes);
  PlacementContact::Profile mode{1,flags.disable_rotation,flags.disable_free,false,true};uint32_t rays=0;
  const auto SourceAllowed=[&](const PlanarMount::SurfaceSeed& seed){if(seed.side!=1&&seed.side!=-1)return false;auto at=std::lower_bound(policy.planar_candidates.begin(),policy.planar_candidates.end(),seed.triangle_id);return at!=policy.planar_candidates.end()&&*at==seed.triangle_id&&(policy.planar_side_masks[size_t(at-policy.planar_candidates.begin())]&(seed.side>0?1u:2u));};
  const auto ProveBody=[&](const PlanarMount::Trace& hit)->bool{
   if(!SourceAllowed(hit.seed))return false;const auto& normal=hit.contact.normal;
   auto patch=planar(*body,hit.seed,policy.planar_candidates);bool valid=!patch.empty()&&std::is_sorted(patch.begin(),patch.end())&&std::adjacent_find(patch.begin(),patch.end())==patch.end()&&std::binary_search(patch.begin(),patch.end(),hit.seed.triangle_id)&&std::includes(policy.planar_candidates.begin(),policy.planar_candidates.end(),patch.begin(),patch.end());
   if(valid)for(auto id:patch)if(!SourceAllowed({id,hit.seed.side})){valid=false;break;}
   if(valid&&clear(*body,patch))for(const auto& corridor:InteriorWallSlab::CurvedCorridors(*body,normal,base,policy.curved_limits.corridors))if(clear(corridor,patch))return true;
   if(!curved)return false;std::vector<CurvedMount::Entry> entries;std::vector<uint32_t> eligible;for(size_t i=0;i<policy.planar_candidates.size();++i)if((policy.curved_mode_masks[i]&1)&&(policy.planar_side_masks[i]&(hit.seed.side>0?1u:2u))){entries.push_back({policy.planar_candidates[i],policy.planar_side_masks[i],policy.curved_mode_masks[i]});eligible.push_back(policy.planar_candidates[i]);}
   if(!std::binary_search(eligible.begin(),eligible.end(),hit.seed.triangle_id))return false;auto proof=curved(*body,hit.seed,entries,policy.curved_limits);if(!proof)return false;
   for(const auto* ids:{&proof.support_ids,&proof.collision_exempt_ids})if(ids->empty()||!std::is_sorted(ids->begin(),ids->end())||std::adjacent_find(ids->begin(),ids->end())!=ids->end()||!std::includes(eligible.begin(),eligible.end(),ids->begin(),ids->end()))return false;
   if(!std::binary_search(proof.collision_exempt_ids.begin(),proof.collision_exempt_ids.end(),hit.seed.triangle_id)||!clear(*body,proof.collision_exempt_ids))return false;
   for(const auto& corridor:InteriorWallSlab::CurvedCorridors(*body,normal,base,policy.curved_limits.corridors))if(clear(corridor,proof.collision_exempt_ids))return true;return false;
  };
  for(float y:coordinates[0])for(float x:coordinates[1])for(float z:coordinates[2]){
   const OriginalSphereQuery::Point q{y,x,z};if(!OriginalWallFrame::Contains(*preimage,q)||!OriginalWallFrame::Matches(pose.position,frame->boxes.frame,q))continue;const PlacementContact::Vector centre{x,y,z};
   const float length=2*(radius+Pullback+4*tolerance);if(!std::isfinite(length)||length<=0)return bad;
   for(unsigned axis=0;axis<2;++axis)for(float sign:{-1.f,1.f}){if(rays==MaximumRayQueries)return bad;auto end=centre;end[axis]+=sign*length;++rays;auto candidate=trace(centre,end);if(!candidate||!PlacementContact::NormalAllowed(mode,PlacementContact::Mode::Wall,candidate->contact.normal)||std::abs(PlacementContact::Dot(candidate->contact.normal,candidate->contact.normal)-1)>1e-5f)continue;
    for(unsigned k=0;k<3;++k)end[k]=centre[k]-candidate->contact.normal[k]*length;if(rays==MaximumRayQueries)return bad;++rays;auto hit=trace(centre,end);if(!hit||!PlacementContact::NormalAllowed(mode,PlacementContact::Mode::Wall,hit->contact.normal)||std::abs(PlacementContact::Dot(hit->contact.normal,hit->contact.normal)-1)>1e-5f||PlacementContact::Dot(candidate->contact.normal,hit->contact.normal)<1-1e-6f)continue;
    PlacementContact::Vector separation{};for(unsigned k=0;k<3;++k)separation[k]=centre[k]-hit->contact.point[k];const float clearance=PlacementContact::Dot(separation,hit->contact.normal);if(!std::isfinite(clearance)||clearance<radius-tolerance||clearance>radius+Pullback+tolerance||!ProveBody(*hit))continue;
    auto final_context=context;final_context.mount_contact_verified=true;return InteriorContent::CheckPlacement(selected.input.room,final_context,selected.input.item,selected.input.geometry,pose,operation);
   }
  }return bad;
 }catch(...){return bad;}
}
// V8 original sphere-contact successor. Trace must bind a finite real source
// seed on the same authenticated Map/census. This is an arithmetic/contact
// compatibility witness, not recovery of an untransmitted camera ray.
inline PlacementPolicy::Result CheckContact(const InteriorContent::Profile& base,const Selection& selected,const PlacementPolicy::Context& context,const PlacementPolicy::Transform& pose,PlacementPolicy::Operation operation,const MountFlags::Flags& flags,const SelectedWallCache::Prepared& cache,const InteriorWallSlab::PlanarTrace& trace,float default_heading=0){
 auto old=InteriorContent::CheckPlacement(base,context,selected.input.item,selected.input.geometry,pose,operation);auto bad=old;if(old)bad.error=PlacementPolicy::Error::OutsideRegion;
 if((!old&&bad.error!=PlacementPolicy::Error::OutsideRegion&&bad.error!=PlacementPolicy::Error::NumericallyAmbiguous)||!ValidContactSelection(base,selected)||!trace||cache.ItemId()!=selected.input.item.item_id||cache.MemberSha256()!=selected.input.model_member_sha256||!std::isfinite(default_heading)||default_heading<0||default_heading>512)return bad;
 try{const auto& policy=*selected.source_policy;OriginalPlacementFrame::Pose input{pose.scale,{pose.heading,pose.pitch,pose.roll},pose.position};auto frame=OriginalPlacementFrame::Build(cache.Cache(),input);if(!frame||!policy.source_envelope)return bad;
  const auto& extents=frame->boxes.scaled_extent;bool choose_y=extents[1]<extents[0];if(((uint32_t(default_heading)+64)/128)&1)choose_y=!choose_y;const float radius=extents[choose_y?1:0];if(!std::isfinite(radius)||radius<=0)return bad;
  float ulp=0;for(float p:pose.position)ulp=std::max(ulp,std::max(std::abs(std::nextafter(p,INFINITY)-p),std::abs(p-std::nextafter(p,-INFINITY))));const float tolerance=std::max(8*ulp,float(4*selected.input.geometry.measured_local_error*pose.scale)+8*std::numeric_limits<float>::epsilon());if(!std::isfinite(tolerance)||tolerance<=0||tolerance>.125f)return bad;
  auto geometry_context=context;geometry_context.geometry_only=true;auto accepted=InteriorContent::CheckPlacement(selected.input.room,geometry_context,selected.input.item,selected.input.geometry,pose,operation);if(!accepted)return accepted;
  const auto bounds=InteriorWallSlab::Bounds(*policy.source_envelope);OriginalSphereQuery::Point lo{bounds.first[1],bounds.first[0],bounds.first[2]},hi{bounds.second[1],bounds.second[0],bounds.second[2]};auto preimage=OriginalWallFrame::Preimages(pose.position,frame->boxes.frame,lo,hi);if(!preimage)return bad;
  std::array<std::vector<float>,3> coordinates;for(unsigned k=0;k<3;++k)coordinates[k]=OriginalHeightPreimage::Probes(preimage->coordinates[k],CoordinateProbes);
  PlacementContact::Profile mode{1,flags.disable_rotation,flags.disable_free,false,true};uint32_t rays=0;
  const auto SourceAllowed=[&](const PlanarMount::SurfaceSeed& seed){if(seed.side!=1&&seed.side!=-1)return false;auto at=std::lower_bound(policy.planar_candidates.begin(),policy.planar_candidates.end(),seed.triangle_id);return at!=policy.planar_candidates.end()&&*at==seed.triangle_id&&(policy.planar_side_masks[size_t(at-policy.planar_candidates.begin())]&(seed.side>0?1u:2u));};
  for(float y:coordinates[0])for(float x:coordinates[1])for(float z:coordinates[2]){
   const OriginalSphereQuery::Point q{y,x,z};if(!OriginalWallFrame::Contains(*preimage,q)||!OriginalWallFrame::Matches(pose.position,frame->boxes.frame,q))continue;const PlacementContact::Vector centre{x,y,z};
   const float length=2*(radius+Pullback+4*tolerance);if(!std::isfinite(length)||length<=0)return bad;
   for(unsigned axis=0;axis<2;++axis)for(float sign:{-1.f,1.f}){if(rays==MaximumRayQueries)return bad;auto end=centre;end[axis]+=sign*length;++rays;auto candidate=trace(centre,end);if(!candidate||!PlacementContact::NormalAllowed(mode,PlacementContact::Mode::Wall,candidate->contact.normal)||std::abs(PlacementContact::Dot(candidate->contact.normal,candidate->contact.normal)-1)>1e-5f)continue;
    for(unsigned k=0;k<3;++k)end[k]=centre[k]-candidate->contact.normal[k]*length;if(rays==MaximumRayQueries)return bad;++rays;auto hit=trace(centre,end);if(!hit||!PlacementContact::NormalAllowed(mode,PlacementContact::Mode::Wall,hit->contact.normal)||std::abs(PlacementContact::Dot(hit->contact.normal,hit->contact.normal)-1)>1e-5f||PlacementContact::Dot(candidate->contact.normal,hit->contact.normal)<1-1e-6f)continue;
    PlacementContact::Vector separation{};for(unsigned k=0;k<3;++k)separation[k]=centre[k]-hit->contact.point[k];const float clearance=PlacementContact::Dot(separation,hit->contact.normal);if(!std::isfinite(clearance)||clearance<radius-tolerance||clearance>radius+Pullback+tolerance||!SourceAllowed(hit->seed))continue;
    auto final_context=context;final_context.mount_contact_verified=true;return InteriorContent::CheckPlacement(selected.input.room,final_context,selected.input.item,selected.input.geometry,pose,operation);
   }
  }return bad;
 }catch(...){return bad;}
}
}
#endif
