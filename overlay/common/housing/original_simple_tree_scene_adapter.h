#ifndef EQEMU_HOUSING_ORIGINAL_SIMPLE_TREE_SCENE_ADAPTER_H
#define EQEMU_HOUSING_ORIGINAL_SIMPLE_TREE_SCENE_ADAPTER_H
#include "original_simple_tree_sidecar.h"
#include "original_height_scene_query.h"
namespace Housing::OriginalSimpleTreeQuery {
struct ActorCandidates {Status status=Status::Invalid;OriginalHeightSceneQuery::ActorSource::Enumeration receipt;Enumeration trace;};
// Pure receipt computation, NOT zone/custody/resource/pose authorization. Bridge
// resolver must bind the reviewed immutable tree to this exact actor/model and
// retain stable complete scene coverage. No unverified all-mesh enrollment.
inline ActorCandidates Candidates(const Tree&tree,const OriginalActorLocalQuery::Matrix& matrix,float scale,const Point& world_start,const Point& world_delta,float radius){
 ActorCandidates out;const auto frame=OriginalActorLocalQuery::Build(matrix,scale,world_start,world_delta,radius);if(!frame){out.status=Status::Unknown;return out;}const auto delta=OriginalActorLocalQuery::SphereDelta(frame->start,frame->delta);if(!delta){out.status=Status::Unknown;return out;}
 out.trace=Enumerate(tree,frame->start,*delta,frame->radius);out.status=out.trace.status;if(out.status!=Status::Ready)return out;out.receipt={world_start,world_delta,radius,out.trace.polygon_ids};return out;
}
}
#endif
