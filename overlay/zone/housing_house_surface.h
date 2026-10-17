#ifndef EQEMU_HOUSING_HOUSE_SURFACE_H
#define EQEMU_HOUSING_HOUSE_SURFACE_H
#include "../common/housing/house_binding_service.h"
#include "../common/housing/placement_contact.h"
#include "raycast_mesh.h"
#include <memory>
namespace Housing::HouseSurface {
// Geometry only. Both initial and current rows must be loaded by the caller
// under its actual existing placement transaction/estate/lease/ACL fence.
// This class does not authenticate a caller, load SQL, or authorize placement.
class Provider {
public:
 static std::unique_ptr<Provider> Build(const HouseBinding::Binding&,const LiveItems::Stored&);
 bool Matches(const HouseBinding::Binding&,const LiveItems::Stored&)const;
 std::optional<PlacementContact::Contact> Trace(const HouseBinding::Binding&,const LiveItems::Stored&,
  const PlacementContact::Vector& server_start,const PlacementContact::Vector& server_end)const;
private:
 struct DeleteMesh {void operator()(RaycastMesh* p)const{if(p)p->release();}};
 struct Identity {
  uint64_t house{},house_revision{},custody{},pose_revision{},catalog_revision{},storage_revision{};
  EstateId estate{};uint32_t native_estate{},native_item{},item{};
  std::string catalog;ContentBinding::Identity content;PlacementPolicy::Transform transform;
 } identity_;
 std::unique_ptr<RaycastMesh,DeleteMesh> mesh_;
 static std::optional<Identity> Identify(const HouseBinding::Binding&,const LiveItems::Stored&);
};
}
#endif
