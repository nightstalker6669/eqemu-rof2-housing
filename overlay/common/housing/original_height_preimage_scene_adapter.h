#ifndef EQEMU_HOUSING_ORIGINAL_HEIGHT_PREIMAGE_SCENE_ADAPTER_H
#define EQEMU_HOUSING_ORIGINAL_HEIGHT_PREIMAGE_SCENE_ADAPTER_H
#include "original_height_scene_query.h"
#include "original_height_preimage.h"
namespace Housing::OriginalHeightPreimageSceneAdapter {
namespace P=OriginalHeightPreimage;
namespace S=OriginalHeightSceneQuery;
// Independent provider responsibility: authenticate the complete immutable scene
// (terrain/alternate-height dispatch, both actor trees, filter globals, every
// resource, every actor frame and disabled state) against this receipt. It must
// also reject an obsolete epoch. No default authorizer is supplied. Merely
// returning scene.complete or hashing housing DB rows is NOT an authorizer.
// Generation is the frozen scene epoch in this adapter's explicit contract.
using Attest=std::function<std::optional<P::Snapshot>(const OriginalHeightScene::Snapshot&)>;
struct Frozen {
 std::shared_ptr<const OriginalHeightScene::Snapshot> scene;
 P::Snapshot expected;
 OriginalHeightScene::Resolve terrain;
 S::ResolveActorQuery actors;
 Attest attest;
};
inline P::Height Reduce(const S::HeightResult& h){
 P::Height out;const auto&s=h.sweep;auto& d=out.sweep;
 using A=OriginalSphereSource::Status;
 switch(s.status){
 case S::Status::Hit:d.status=A::Hit;break;
 case S::Status::Clear:d.status=A::Clear;break;
 case S::Status::Ambiguous:d.status=A::Ambiguous;break;
 case S::Status::BudgetExceeded:d.status=A::BudgetExceeded;break;
 case S::Status::Environment:d.status=A::Environment;break;
 default:d.status=A::InvalidSource;break;
 }
 d.source_id=s.source_id;d.sources=s.sources;d.narrowphase=s.narrowphase;d.operations=s.kernel_steps;
 d.contact={s.status==S::Status::Hit,s.fraction,s.normal,s.stopped};
 out.height=h.height;out.valid=h.valid&&s.status==S::Status::Hit;
 return out;
}
// The frozen owner, resolvers and independent attestation are retained for ALL
// preimage probes. Each varying start/radius still needs its own actor-tree
// enumeration receipt. Providers must not cache a stale first-query traversal.
inline P::Query Bind(Frozen frozen){
 return [frozen=std::move(frozen)](const OriginalActorQueryFrame::HeightRequest& request,OriginalSphereSource::Limits limits)->P::Reply{
  P::Reply out;
  try{
   if(!frozen.scene||!frozen.attest||!P::Nonzero(frozen.expected.digest)||!frozen.expected.generation||!frozen.scene->complete||frozen.scene->epoch!=frozen.expected.generation)return out;
   const auto before=frozen.attest(*frozen.scene);if(!before)return out;out.snapshot=*before;
   if(!P::Same(*before,frozen.expected))return out;
   const auto height=S::HeightQuery(*frozen.scene,request,frozen.terrain,frozen.actors,limits);
   out.height=Reduce(height);
   const auto after=frozen.attest(*frozen.scene);if(!after)return out;out.snapshot=*after;
   if(!P::Same(*after,frozen.expected))return out;
   // A known complete scene can still yield bounded exhaustion or ambiguity.
   // Preserve those statuses; an unrecognized source is never completeness.
   out.complete=height.sweep.status==S::Status::Hit||height.sweep.status==S::Status::Clear||height.sweep.status==S::Status::Ambiguous||height.sweep.status==S::Status::BudgetExceeded;
   return out;
  }catch(...){return out;}
 };
}
}
#endif
