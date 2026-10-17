#ifndef EQEMU_HOUSING_INTERIOR_STORAGE_SERVICE_H
#define EQEMU_HOUSING_INTERIOR_STORAGE_SERVICE_H
#include "scene_service.h"
#include "interior_profile_db.h"
#include "item_storage_db.h"
namespace Housing::InteriorStorage {
struct ExteriorContext {EstateId estate_id=0;uint32_t native_estate_id=0;uint64_t catalog_revision=0;Route source;};
struct Request {
 std::string request_key;LiveItems::Lease lease;LiveItems::InteriorMoveContext room;
 uint64_t custody_id=0,expected_pose_revision=0,expected_storage_revision=0;
 uint32_t native_item_id=0;
 std::optional<ExteriorContext> exterior;
 std::optional<LiveItems::OccupiedParentContext> parent_yard; // Fresh actual occupied-house relation to exterior target.
 std::optional<Route> room_exterior_source; // Actual yard access to separately reviewed attached room.
 std::optional<PlacementPolicy::Transform> placement; // Explicit stored-to-placed transition.
 std::optional<NpcNativeFrame::Frame> npc_frame;
 std::optional<LiveItems::CrateMemberContext> crate_member; // Atomically release a crate member into a checked yard or room pose.
};
// Server-selected exact set. No packet supplies lease, context or member revisions.
struct StowMember {uint64_t custody_id=0,pose_revision=0,storage_revision=0,scope_revision=0;uint32_t native_item_id=0;};
struct StowRequest {std::string request_key;LiveItems::Lease lease;LiveItems::InteriorMoveContext room;std::optional<ExteriorContext> exterior;std::vector<StowMember> items;};
struct Receipt {uint64_t custody_id=0,before_revision=0,after_revision=0;bool replayed=false;};
struct TransferRequest {
 std::string request_key;LiveItems::Lease lease;LiveItems::InteriorMoveContext occupied_room;
 uint64_t custody_id=0,expected_pose_revision=0,expected_storage_revision=0;
 uint32_t source_native_estate_id=0,native_item_id=0;
 bool to_room=false;
 std::optional<ExteriorContext> yard; // Actual yard source, separately checked; room is destination context.
 std::optional<LiveItems::CrateMemberContext> crate_member; // Checked source membership, released atomically.
 std::optional<PlacementPolicy::Transform> placement; // Optional direct placement in the freshly authorized yard or room destination.
 std::optional<NpcNativeFrame::Frame> npc_frame;
};
struct TransferReceipt {
 uint64_t custody_id=0,storage_revision=0;
 uint32_t before_native_item_id=0,after_native_item_id=0,target_native_estate_id=0;
 bool replayed=false;
};
class Service {
 std::unique_ptr<Database> database_;bool enabled_;std::mutex mutex_;const CrateBinding::CustodyContext* crates_=nullptr;
 LiveItems::Result<Receipt> ChangeCurrent(const Request&,LiveItems::InventorySessionFence&,const Scene::HouseScenePolicy*,const InteriorContent::ProfilePolicy*);
public:
 explicit Service(std::unique_ptr<Database> db,bool enabled=false,const CrateBinding::CustodyContext* crates=nullptr):database_(std::move(db)),enabled_(enabled),crates_(crates){}
 // Caller owns active transaction and held inventory fence. Reads fresh scene
 // state after preceding batch members; no hidden lifecycle or shared cache.
 // Any error requires the caller to roll back its entire parent operation.
 // This is a mutation hook, not an aggregate replay/recovery resolver.
 static LiveItems::Result<Receipt> ChangeUnderTransaction(Database&,bool enabled,const Request&,LiveItems::InventorySessionFence&,
  const Scene::HouseScenePolicy*,const InteriorContent::ProfilePolicy*,const CrateBinding::CustodyContext* =nullptr);
 static LiveItems::Result<TransferReceipt> TransferUnderTransaction(Database&,bool enabled,const TransferRequest&,LiveItems::InventorySessionFence&,
  const Scene::HouseScenePolicy&,const InteriorContent::ProfilePolicy&,const CrateBinding::CustodyContext* =nullptr);
 LiveItems::Result<std::vector<Receipt>> StowAll(const StowRequest&,LiveItems::InventorySessionFence&,const Scene::HouseScenePolicy*,const InteriorContent::ProfilePolicy*);
 LiveItems::Result<TransferReceipt> Transfer(const TransferRequest&,LiveItems::InventorySessionFence&,const Scene::HouseScenePolicy&,const InteriorContent::ProfilePolicy&);
 LiveItems::Result<Receipt> Change(const Request& r,LiveItems::InventorySessionFence& f,const Scene::HouseScenePolicy& h,const InteriorContent::ProfilePolicy& p){return ChangeCurrent(r,f,&h,&p);}
 LiveItems::Result<Receipt> ChangeExterior(const Request& r,LiveItems::InventorySessionFence& f,const Scene::HouseScenePolicy* h=nullptr,const InteriorContent::ProfilePolicy* p=nullptr){if(!r.exterior)return {{},LiveItems::Error::InvalidArgument};return ChangeCurrent(r,f,h,p);}
 LiveItems::Result<Receipt> Stow(const Request& r,LiveItems::InventorySessionFence& f,const Scene::HouseScenePolicy& h,const InteriorContent::ProfilePolicy& p){if(r.placement)return {{},LiveItems::Error::InvalidArgument};return Change(r,f,h,p);}
};
}
#endif
