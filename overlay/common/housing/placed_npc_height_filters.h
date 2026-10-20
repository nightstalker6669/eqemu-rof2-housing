#ifndef EQEMU_HOUSING_PLACED_NPC_HEIGHT_FILTERS_H
#define EQEMU_HOUSING_PLACED_NPC_HEIGHT_FILTERS_H
#include "placed_height_filters.h"
#include "npc_exit_collision.h"
namespace Housing::PlacedNpcHeightFilters {
// Received CPlacedNPC: 4d08a0 forces object+98=1; successful local entity
// 504e26..e38 binds public type1 (NOT ordinary furniture type9), and
// 504e4a..e67 ORs graphics restriction5. Local599bd0/5961e0 uses the ordinary
// factory with no special mode/properties. SphereMode's reviewed719 resource
// classes prove hierarchy and classifierbit0 clear => native volume2.
// Caller must independently bind current SQL metadata and actual outgoing
// ItemData cache, committed stored frame/pose and strict publication snapshot.
// This helper supplies filter equivalence only, never body/mesh geometry.
inline std::optional<PlacedHeightFilters::Actor> Project(const NpcRuntime::Profile& profile,
 const PlacementPolicy::Transform& ground,const std::optional<NpcNativeFrame::Frame>& frame,
 uint32_t native_item_id)noexcept{
 try{
  if(!native_item_id||native_item_id==UINT32_MAX||!NpcExitCollision::SphereMode(profile))return {};
  for(float value:ground.position)if(!std::isfinite(value))return {};
  for(float value:{ground.heading,ground.pitch,ground.roll})if(!std::isfinite(value)||value<0||value>512)return {};
  const auto actor_pose=NpcRuntime::ActorPose(profile,ground,frame);
  for(float value:actor_pose.position)if(!std::isfinite(value))return {};
  PlacedHeightFilters::Actor actor;actor.application_identity=native_item_id;
  actor.type=1;actor.restriction_mask=5;actor.volume=2;
  // Floor excludes restrictionbit0 before radius; Ceiling excludes volume2.
  // No arbitrary radius value or physical-nonblocking claim is required.
  return actor;
 }catch(...){return {};}
}
}
#endif
