#ifndef EQEMU_HOUSING_ANCHOR_PENDING_H
#define EQEMU_HOUSING_ANCHOR_PENDING_H
#include "../common/housing/anchor_destination.h"
#include "../common/housing/trophy_anchor_wire.h"
#include <chrono>
class EQStreamInterface;
namespace Housing::AnchorTransport {
struct Pending {
 Scene::AnchorDestination destination;
 TrophyAnchorWire::Anchor offer;
 std::chrono::steady_clock::time_point expires;
 EQStreamInterface* connection=nullptr;
 Route source;
 bool Matches(const TrophyAnchorWire::Anchor& accepted,EQStreamInterface* current,
  Route current_source,std::chrono::steady_clock::time_point now)const {
  return current&&current==connection&&now<expires&&source.zone_id==current_source.zone_id&&
   source.zone_version==current_source.zone_version&&source.instance_id==current_source.instance_id&&
   TrophyAnchorWire::Matches(offer,accepted);
 }
};
}
#endif
