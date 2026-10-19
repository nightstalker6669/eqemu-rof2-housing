#ifndef EQEMU_HOUSING_CRATE_STORAGE_SERVICE_H
#define EQEMU_HOUSING_CRATE_STORAGE_SERVICE_H
#include "scene_service.h"
#include <vector>
namespace Housing::OccupiedRetirement {class Authority;}
namespace Housing::CrateStorage {
// Server-resolved current plot/room, crate and member identities; never packet proof.
struct RoomContext {uint64_t house_id=0,house_revision=0,profile_revision=0;bool attached=false;};
struct Request {
 std::string request_key;LiveItems::Lease lease;
 EstateId estate_id=0;uint32_t native_plot_id=0;uint64_t catalog_revision=0;Route source;
 uint64_t crate_id=0,crate_revision=0,custody_id=0,pose_revision=0,storage_revision=0;
 std::optional<RoomContext> room;
 std::optional<LiveItems::OccupiedParentContext> parent_yard; // Current occupied room authority for its yard.
 bool to_crate=true;
 bool vault=false; // Typed list-only source; pose_revision must be0, storage_revision is vault CAS.
 bool restore_original=true; // Explicit original-state restore; bulk Unpack and Move-to-Shed keep it stored.
};
struct Receipt {
 uint64_t crate_id=0,custody_id=0,crate_revision=0,storage_revision=0;
 bool replayed=false;
};
struct BatchMember {uint64_t custody_id=0,pose_revision=0,storage_revision=0;bool vault=false;};
struct BatchRequest {
 std::string request_key;LiveItems::Lease lease;
 EstateId estate_id=0;uint32_t native_plot_id=0;uint64_t catalog_revision=0;Route source;
 uint64_t crate_id=0,crate_revision=0;bool packing=true;std::optional<RoomContext> room;
 std::vector<BatchMember> members; // Complete server-read candidate set, ordered by custody ID; Pack selects eligible items under locks.
};
struct BatchReceipt {uint64_t crate_id=0,crate_revision=0;uint32_t member_count=0;bool replayed=false;};
class Service {
 std::unique_ptr<Database> database_;bool enabled_;std::mutex mutex_;
public:
 explicit Service(std::unique_ptr<Database> db,bool enabled=false):database_(std::move(db)),enabled_(enabled){}
 // Mutation-only parent transaction hook. No lifecycle/application/sends;
 // caller must roll back the entire batch on error. Aggregate replay must be
 // resolved separately before entering these current-scene mutation checks.
 static LiveItems::Result<Receipt> TransferUnderTransaction(Database&,bool enabled,const Request&,LiveItems::InventorySessionFence&,
  const CrateBinding::CustodyContext&,const Scene::HouseScenePolicy* =nullptr,const InteriorContent::ProfilePolicy* =nullptr);
 // Only the exact locked aggregate may pack these original members while
 // rent has lapsed. Ordinary Transfer/Pack keep their normal rent checks.
 static LiveItems::Result<Receipt> PackForRetirementUnderTransaction(Database&,bool,const Request&,LiveItems::InventorySessionFence&,
  const OccupiedRetirement::Authority&,const CrateBinding::CustodyContext&,const Scene::HouseScenePolicy* =nullptr,const InteriorContent::ProfilePolicy* =nullptr);
 LiveItems::Result<Receipt> Transfer(const Request&,LiveItems::InventorySessionFence&,const CrateBinding::CustodyContext&,const Scene::HouseScenePolicy* =nullptr,const InteriorContent::ProfilePolicy* =nullptr);
 LiveItems::Result<BatchReceipt> Pack(const BatchRequest&,LiveItems::InventorySessionFence&,const CrateBinding::CustodyContext&,const Scene::HouseScenePolicy* =nullptr,const InteriorContent::ProfilePolicy* =nullptr);
};
}
#endif
