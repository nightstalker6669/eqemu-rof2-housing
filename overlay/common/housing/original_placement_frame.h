#ifndef EQEMU_HOUSING_ORIGINAL_PLACEMENT_FRAME_H
#define EQEMU_HOUSING_ORIGINAL_PLACEMENT_FRAME_H
#include "original_cached_box.h"
#include "original_renderer_box.h"
namespace Housing::OriginalPlacementFrame {
using Point=OriginalSphereQuery::Point;
// Ordinary non-mob static-model scope. Cache values MUST already be the
// authenticated renderer cache, including reviewed ActorTag/eligibility policy.
// This helper does not turn raw bounds or a client packet into reviewed cache.
struct Pose {float scale{};Point heading_pitch_roll{},native_yxz{};};
struct Result {Pose pose;OriginalCachedBox::Box cache;OriginalRendererBox::Result boxes;Point query_center{};};
// Getter84 order is H/P/R, not a permutation of world Y/X/Z. The retained
// selected-actor snapshots bind all three packet and renderer DWORDs. The
// game4907f0 box rotation is deliberately NOT the actor mesh lookup-table
// matrix: it uses the source's own angle normalization and FSIN/FCOS chain.
inline std::optional<Result> Build(const OriginalCachedBox::Box& reviewed_cache,const Pose& pose){
 if(!OriginalSphereSource::Finite(pose.native_yxz))return {};
 std::array<double,3> c{},e{};for(unsigned k=0;k<3;++k){c[k]=reviewed_cache.center[k];e[k]=reviewed_cache.extent[k];}
 auto boxes=OriginalRendererBox::Build(c,e,pose.scale,pose.heading_pitch_roll);if(!boxes)return {};
 auto q=OriginalActorQueryFrame::QueryCenter(pose.native_yxz,boxes->frame);if(!q)return {};
 return Result{pose,reviewed_cache,*boxes,*q};
}
}
#endif
