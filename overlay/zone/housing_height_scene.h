#ifndef EQEMU_ZONE_HOUSING_HEIGHT_SCENE_H
#define EQEMU_ZONE_HOUSING_HEIGHT_SCENE_H
#include "../common/housing/scene_service.h"
#include "../common/housing/mount_flags_db.h"
class Client;
namespace Housing::HeightScene {
enum class Status {Witness,NotApplicable,Unavailable,Inconclusive,SourceChanged};
struct Result {Status status=Status::Unavailable;uint32_t queries{},narrowphase{},stage{};uint64_t kernel_steps{},blocked_source{};explicit operator bool()const{return status==Status::Witness;}};
// Synchronous zone-thread only. The owning Bridge has just called the dedicated
// strict published-collision reader on THIS transaction, and keeps all creator
// leases, selected subject and InventorySessionFence held for this entire call.
// No event-loop yield, raw packet scene or caller completeness flag is accepted.
// NotApplicable means another permitted mount mode may be checked independently;
// Unavailable/SourceChanged must never become a legacy Floor/Ceiling fallback.
Result Admit(Client&,Database&,const LiveItems::Lease&,const Scene::InteriorSnapshot&,
 const LiveItems::MountSubject&,const LiveItems::InventoryRow&,const PlacementPolicy::Transform&,
 const MountFlags::Flags&,LiveItems::InventorySessionFence&)noexcept;
}
#endif
