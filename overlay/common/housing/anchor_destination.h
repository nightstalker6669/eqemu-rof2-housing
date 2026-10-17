#ifndef EQEMU_HOUSING_ANCHOR_DESTINATION_H
#define EQEMU_HOUSING_ANCHOR_DESTINATION_H
#include "persistence_service.h"
#include "placement_policy.h"
namespace Housing::Scene {
// SERVER spell source and anchor item owner, never decoded from7bb1. Supports
// actual yard or reviewed attached-room anchors; crates/stowed items cannot become destinations.
struct AnchorRequest {AuthenticatedPurchaser caster,owner;uint32_t item_id=0;};
struct AnchorDestination {
 AuthenticatedPurchaser caster,owner;uint32_t item_id=0,guild_id=UINT32_MAX;
 EstateId estate_id=0;uint32_t native_estate_id=0;Route route;
 uint64_t custody_id=0,pose_revision=0,house_id=0,house_revision=0,profile_revision=0,scope_revision=0;PlacementPolicy::Transform pose;
 std::string caster_name,destination_name;
};
inline bool SameAnchor(const AnchorDestination& a,const AnchorDestination& b){
 return a.caster.character_id==b.caster.character_id&&a.caster.account_id==b.caster.account_id&&a.owner.character_id==b.owner.character_id&&a.owner.account_id==b.owner.account_id&&
  a.item_id==b.item_id&&a.guild_id==b.guild_id&&a.estate_id==b.estate_id&&a.native_estate_id==b.native_estate_id&&
  a.route.zone_id==b.route.zone_id&&a.route.zone_version==b.route.zone_version&&a.route.instance_id==b.route.instance_id&&
  a.custody_id==b.custody_id&&a.pose_revision==b.pose_revision&&a.house_id==b.house_id&&a.house_revision==b.house_revision&&a.profile_revision==b.profile_revision&&a.scope_revision==b.scope_revision&&a.pose.heading==b.pose.heading&&
  a.pose.pitch==b.pose.pitch&&a.pose.roll==b.pose.roll&&a.pose.scale==b.pose.scale&&a.pose.position==b.pose.position;
}

}
#endif
