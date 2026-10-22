#ifndef EQEMU_HOUSING_LIVE_ITEM_SERVICE_H
#define EQEMU_HOUSING_LIVE_ITEM_SERVICE_H
#include "persistence_service.h"
#include "../repositories/inventory_repository.h"
#include <array>
#include <string_view>
#include <memory>
#include <vector>
#include "placement_policy.h"
#include "npc_native_frame.h"
#include "catalog_content_binding.h"
namespace Housing::OccupiedRetirement {class Authority;}
namespace Housing::NpcRuntime {struct Profile;}
namespace Housing::Scene { class HouseScenePolicy; }
namespace Housing::InteriorContent { class ProfilePolicy; }
namespace Housing::HouseBinding { struct Effect; struct Binding; class CustodyContext; }
namespace Housing::CrateBinding { class CustodyContext; struct Allocation; }
namespace Housing::LayoutCrateMember {struct Record;}
namespace Housing::LayoutInventory {struct Plan;struct Prepared;}
namespace EQ {class InventoryProfile;}
namespace Housing::LiveItems {
inline constexpr uint8_t NullItemId=1,NullCharges=2,NullCustomData=4,NullGuid=8;
struct InventoryRow {
 InventoryRepository::Inventory values{}; uint8_t null_fields=0;
 // Versioned custody-only state. Children use relative native bag slots0..9;
 // ordinary SQL rows retain their exact original fields and v1 byte encoding.
 std::vector<uint8_t> evolving_payload;
 std::vector<InventoryRow> children;
};
struct Lease { AuthenticatedPurchaser actor; uint64_t epoch=0; std::string session_key; std::array<uint8_t,32> token{}; std::string pending_key; };
struct Catalog { int32_t item_class=0,bag_slots=0,lore_group=0,no_drop=0; };
struct Reservation {uint32_t native_item_id=0,native_estate_id=0;EstateId estate_id=0;std::string catalog_key;uint64_t catalog_revision=0;std::optional<ContentBinding::Identity> content_binding;};
struct Placed {Reservation reservation;PlacementPolicy::Transform transform;uint64_t pose_revision=1;std::optional<NpcNativeFrame::Frame> npc_frame;};
struct PreviewRequest {std::string request_key;Lease lease;EstateId estate_id=0;uint32_t native_estate_id=0;std::string catalog_key;uint64_t catalog_revision=0;InventoryRow expected;int32_t verified_runtime_serial=0;bool house_storage_only=false;};
struct InteriorPreviewRequest {std::string request_key;Lease lease;Route current_source;uint64_t expected_house_id=0,expected_house_revision=0,expected_profile_revision=0;InventoryRow expected;int32_t verified_runtime_serial=0;EstateId exterior_estate_id=0;std::optional<Route> exterior_source;};
struct InteriorReservation {Reservation placement;uint64_t house_id=0,house_revision=0,profile_revision=0;Route source;std::array<uint8_t,32> content_digest{};};
struct Stored { uint64_t custody_id=0; EstateId estate_id=0; InventoryRow row; Catalog catalog; std::optional<Placed> placement;uint64_t current_catalog_revision=0;uint64_t storage_revision=0;bool stowed=false; };
struct InteriorMoveContext {uint64_t house_id=0,house_revision=0,profile_revision=0,scope_revision=0;Route source;};
// Actual occupied house selecting its own parent yard; never an item room scope.
struct OccupiedParentContext {uint64_t house_id=0,house_revision=0,profile_revision=0;Route source;};
struct MoveRequest {std::string request_key;Lease lease;EstateId estate_id=0;uint64_t custody_id=0;uint32_t native_item_id=0;uint64_t expected_pose_revision=0,expected_catalog_revision=0;PlacementPolicy::Transform transform;std::optional<InteriorMoveContext> room;std::optional<Route> room_exterior_source;std::optional<OccupiedParentContext> parent_yard;std::optional<NpcNativeFrame::Frame> npc_frame;};
struct MoveReceipt {std::string request_key;uint64_t custody_id=0;Placed before,after;bool replayed=false;uint64_t committed_catalog_revision=0;std::optional<InteriorMoveContext> room;std::optional<Route> room_exterior_source;std::optional<OccupiedParentContext> parent_yard;};
// Trusted destination intent, with no client-supplied source/custody identity.
struct LayoutUnitDestination {
 std::string request_key;Lease lease;EstateId estate_id=0;uint32_t native_estate_id=0;
 std::string catalog_key;uint64_t catalog_revision=0;PlacementPolicy::Transform transform;
 std::optional<InteriorMoveContext> room;std::optional<Route> exterior_source;std::optional<OccupiedParentContext> parent_yard;
};
struct LayoutUnitPlacement {Stored stored;std::optional<InteriorReservation> room;};
// List-only non-placeable custody. No pose, world actor, house/crate binding,
// or packet-selected slot count is represented by this server context.
struct VaultContext {
 EstateId estate_id=0;
 uint32_t native_estate_id=0;uint64_t catalog_revision=0,house_id=0,house_revision=0,profile_revision=0;
 Route source;uint32_t expected_native_item_id=0;uint64_t expected_revision=0;
 // Optional typed containment, authenticated against the current placed crate.
 uint64_t crate_id=0,crate_revision=0,member_revision=0;
};
struct StackPartition;
struct VaultTransferRequest {std::string request_key;Lease lease;EstateId estate_id=0;uint64_t custody_id=0;VaultContext from,to;InventoryRow expected;uint32_t partition_quantity=0,partition_serial=0;};
struct VaultTransferReceipt {uint64_t custody_id=0,revision=0;uint32_t native_item_id=0,native_estate_id=0;bool replayed=false;std::shared_ptr<const StackPartition> partition;EstateId custody_estate_id=0;};
enum class Action:uint8_t { Store=0,Return=1 };
struct HouseIdentity {uint64_t house_id=0,expected_revision=0;};
struct CrateIdentity {uint64_t crate_id=0,expected_revision=0;};
struct CrateMemberContext {uint64_t crate_id=0,revision=0;uint32_t native_plot_id=0;uint64_t catalog_revision=0;Route source;};
// Server-created strict partition. Never populated from a client GUID/serial.
struct StackPartition {InventoryRow before,after;uint32_t quantity=0,serial=0,maximum=0;uint64_t storage_before=0,storage_after=0;bool stowed=false;std::optional<InteriorMoveContext> room_source;std::optional<VaultContext> vault_source;uint64_t vault_scope_revision=0;std::shared_ptr<const CrateBinding::Allocation> member_container_before;std::shared_ptr<const LayoutCrateMember::Record> member_before,member_after;};
struct Request {
 std::string request_key;Lease lease;EstateId estate_id=0;Action action=Action::Store;
 InventoryRow expected;int32_t verified_runtime_serial=0; // Adapter-owned current source proof, NOT DB GUID identity.
 uint64_t custody_id=0;uint32_t destination_slot=0;std::optional<Placed> placement;
 std::optional<HouseIdentity> house; // server-only expected ledger identity; never populated directly from wire
 std::optional<InteriorReservation> room; // explicit server room reservation; explicit Store/Return witness
 std::optional<InteriorMoveContext> room_return_context;uint64_t expected_room_pose_revision=0;
 bool initial_stowed=false; // Inventory -> storage in the same custody transaction; no world actor.
 std::optional<Route> room_exterior_source; // Actual yard access; room.source retains its target-room meaning.
 std::optional<CrateIdentity> crate; // Server ledger identity from a fresh scene, never a wire-provided ID.
 std::optional<CrateMemberContext> crate_member; // Explicit container admission, separate from moving the crate itself.
 std::optional<VaultContext> vault; // Fresh server-selected realm, not a placed item.
 uint32_t partition_quantity=0,partition_serial=0; // Positive strict partial only; exact whole uses legacy path.
};
struct Receipt { std::string request_key;Action action=Action::Store;Stored stored;InventoryRow inventory_after;bool replayed=false,requires_application=true;std::shared_ptr<const HouseBinding::Effect> house_effect;bool house_effect_required=false;std::optional<InteriorReservation> room;std::shared_ptr<const CrateBinding::Allocation> crate_allocation,crate_before;std::optional<CrateMemberContext> crate_member;std::shared_ptr<const StackPartition> partition; };
enum class Error { None,Disabled,InvalidArgument,UnheldFence,Unauthorized,StaleLease,PendingApplication,
 UnsupportedItem,NotFound,Conflict,DatabaseFailure,CommitUncertain,RealEstateFull,VaultFull,NpcFull };
template<class T>struct Result {T value{};Error error=Error::None;explicit operator bool()const{return error==Error::None;}};
// SERVER-ONLY contract. An implementation must hold a real exclusive inventory
// fence covering ALL saves/mutations/callbacks/zoning/logout and current session.
// No packet flag/token or DB ingame field establishes this invariant. Epoch0 is
// used only to bootstrap OpenLease under the world-authorized session fence.
// Owning TX derives this from its exact selected input/reservation, never wire.
struct MountSubject {
 enum class Kind:uint8_t {Unspecified,Add,Move};Kind kind=Kind::Unspecified;
 uint64_t custody_id=0;uint32_t native_item_id=0;uint64_t pose_revision=0;
 static MountSubject NewSource(uint32_t native=0){return {Kind::Add,0,native,0};}
 static MountSubject ExistingSource(uint64_t custody,uint32_t native,uint64_t revision){return {Kind::Move,custody,native,revision};}
 bool Valid()const{return native_item_id!=UINT32_MAX&&((kind==Kind::Add&&!custody_id&&!pose_revision)||(kind==Kind::Move&&custody_id&&native_item_id&&pose_revision));}
};
struct InteriorMountContext {uint64_t house_id=0;std::array<uint8_t,32> content_digest{};MountSubject subject{};};
class InventorySessionFence {
public:
 virtual ~InventorySessionFence()=default;
 virtual bool HeldFor(AuthenticatedPurchaser actor,uint64_t epoch,std::string_view session_key)const noexcept=0;
 // Cursor33 additionally requires exactly one memory cursor item, no queue/children.
 virtual bool SourceMatches(const InventoryRow&,int32_t current_runtime_serial)const noexcept=0;
 // Layout sources include possession bag contents. Existing single-item paths
 // retain their original slot rules; default implementations grant no extras.
 virtual bool LayoutSourceMatches(const InventoryRow& row,int32_t serial)const noexcept{return SourceMatches(row,serial);}
 virtual bool ReturnAllowed(const InventoryRow& exact_destination_row)const noexcept=0; // empty slot, lore, GUID reservation, client slot support; cursor requires empty queue
 // Placement needs current main-thread zone/instance proof, not stale character DB location.
 virtual bool PlacementContextMatches(EstateId,uint32_t native_estate_id,uint16_t neighborhood_instance)const noexcept{return false;}
 // Called only by an owning mutation TX after fresh source/ACL/catalog review.
 virtual bool MountSurfaceUnderTransaction(Database&,const Lease&,const InventoryRow&,const PlacementPolicy::Transform&,uint32_t)const noexcept{return false;}
 // Subject comes only from the owning locked mutation. Legacy implementations
 // retain their old proof; a new original-source implementation must use it.
 virtual bool MountSurfaceWithSubjectUnderTransaction(Database& db,const Lease& lease,const InventoryRow& row,const PlacementPolicy::Transform& pose,uint32_t native,const MountSubject& subject)const noexcept{return subject.Valid()&&MountSurfaceUnderTransaction(db,lease,row,pose,native);}
 // Fresh optional NPC source must also match the actual shared item cache used by the native actor encoder.
 virtual bool NpcProfileMatchesUnderTransaction(Database&,const Lease&,const NpcRuntime::Profile&)const noexcept{return false;}
 virtual bool Applied(const Receipt&)const noexcept=0; // m_inv AFTER matches, source removal/return completed under fence
 // Interior callers must bind the surface proof to their exact selected content.
 virtual bool MountInteriorSurfaceUnderTransaction(Database&,const Lease&,const InventoryRow&,const PlacementPolicy::Transform&,uint32_t,const InteriorMountContext&)const noexcept{return false;}
};
class Service {
public:
 explicit Service(std::unique_ptr<Database>,bool enabled=false,HouseBinding::CustodyContext* houses=nullptr,CrateBinding::CustodyContext* crates=nullptr);~Service();
 Result<Receipt> OperateVault(const Request&,InventorySessionFence&,const Scene::HouseScenePolicy*,const InteriorContent::ProfilePolicy*);
 Result<VaultTransferReceipt> TransferVault(const VaultTransferRequest&,InventorySessionFence&,const Scene::HouseScenePolicy*,const InteriorContent::ProfilePolicy*);
 Result<MoveReceipt> MovePlaced(const MoveRequest&,InventorySessionFence&);
 Result<MoveReceipt> MoveInterior(const MoveRequest&,InventorySessionFence&,const Scene::HouseScenePolicy&,const InteriorContent::ProfilePolicy&);
 Result<MoveReceipt> ResolveMove(const Lease&,const std::string& request_key,InventorySessionFence&);
 // Checked pose mutation for a mixed layout's caller-owned connection/TX.
 // Never begins/commits/rolls back, applies inventory or sends. Caller must
 // roll back the ENTIRE batch on any error, and resolve its aggregate outcome
 // after uncertain COMMIT. Policies/fence are real server-owned instances;
 // passing a request never supplies source/context or permission authority.
 static Result<MoveReceipt> MoveUnderTransaction(Database&,bool enabled,const MoveRequest&,InventorySessionFence&,
  HouseBinding::CustodyContext* =nullptr,CrateBinding::CustodyContext* =nullptr,
  const Scene::HouseScenePolicy* =nullptr,const InteriorContent::ProfilePolicy* =nullptr);
 // Internal batch member: verifies the original prepared inventory source,
 // creates one authorized placement, but never consumes inventory or sets
 // pending/receipts. Caller must consume sources and finish ONE aggregate
 // journal/pending in the same TX, then apply/ACK/recover before enabling loads.
 // Any error requires whole parent rollback; duplicate member keys conflict.
 // Retained token layout uses the real Store path in the caller's aggregate
 // transaction. It never clones a token or creates an independent commit.
 // Caller MUST publish its typed aggregate outcome/pending before COMMIT;
 // every failure requires whole-transaction rollback. No other caller.
 static Result<Receipt> PlaceRetainedLayoutUnderTransaction(Database&,bool,const LayoutUnitDestination&,
  const InventoryRow&,int32_t,uint64_t house_id,uint64_t crate_id,InventorySessionFence&,
  HouseBinding::CustodyContext*,CrateBinding::CustodyContext*,const Scene::HouseScenePolicy*,const InteriorContent::ProfilePolicy*);
 static Result<LayoutUnitPlacement> PlaceLayoutUnitUnderTransaction(Database&,bool enabled,
  const LayoutUnitDestination&,const LayoutInventory::Plan&,const LayoutInventory::Prepared&,size_t unit_index,
  EQ::InventoryProfile&,InventorySessionFence&,const Scene::HouseScenePolicy* =nullptr,
  const InteriorContent::ProfilePolicy* =nullptr,const CrateBinding::CustodyContext* =nullptr);
 // Authenticate/lock the actual current actor and lease without fresh estate
 // rights. Used by immutable batch resolution after a known/uncertain commit.
 // No lifecycle or mutation; caller owns active TX and checks pending_key
 // before any NEW operation. This alone supplies no property authority.
 // Read-only publication barrier for an authenticated scene transaction. Uses
 // the actual immutable pending item receipt codec; never applies/recover/ACKs
 // and does not borrow writer-death recovery authority. No route/access grant.
 static Result<std::optional<Reservation>> PendingNpcPlacementUnderTransaction(
  Database&,bool enabled,const Lease& viewer,uint32_t creator,InventorySessionFence&);
 // Read-only publication barrier; caller owns the room-head locks and TX.
 // Opaque pending receipts are refusal, never evidence of unrelated sources.
 static Result<bool> NoPendingInteriorPublicationUnderTransaction(Database&,const Lease&,uint32_t native_interior,uint64_t house_id,InventorySessionFence&);
 static Result<Lease> AuthenticateUnderTransaction(Database&,bool enabled,const Lease&,InventorySessionFence&);
 // Separate checked room preview; never implicit exterior v1/v2 support.
 Result<InteriorReservation> ReserveInteriorPlacement(const InteriorPreviewRequest&,InventorySessionFence&,const Scene::HouseScenePolicy&,const InteriorContent::ProfilePolicy&,const CrateMemberContext* destination=nullptr);
 Result<Reservation> ReservePlacement(const PreviewRequest&,InventorySessionFence&);
 // Destination check only; this preview reserves neither crate capacity nor membership.
 Result<Reservation> ReserveCratePlacement(const PreviewRequest&,const CrateMemberContext&,InventorySessionFence&,const Scene::HouseScenePolicy* =nullptr);
 Result<bool> CancelPlacement(const Lease&,uint32_t native_item_id,InventorySessionFence&);
 Result<std::vector<Stored>> ReadPlaced(const Lease&,EstateId,InventorySessionFence&);
 Result<Lease> OpenLease(AuthenticatedPurchaser,const std::string& server_session_key,InventorySessionFence&);
 Result<bool> ReleaseLease(const Lease&,InventorySessionFence&); // No pending operation; no expiration/takeover.
 // Exact existing typed Return mutation/journal, inside caller-owned TX.
 // No BEGIN/COMMIT/ROLLBACK, memory application or ACK. Any failure requires
 // whole parent rollback; the caller must retain the real fence until recovery.
 static Result<Receipt> ReturnUnderTransaction(Database&,bool enabled,const Request&,InventorySessionFence&,
  HouseBinding::CustodyContext* =nullptr,CrateBinding::CustodyContext* =nullptr,
  const Scene::HouseScenePolicy* =nullptr,const InteriorContent::ProfilePolicy* =nullptr);
 // Exact crate return only, authorized by the locked occupied-retirement
 // aggregate. No standalone constructor or general expired-operation flag.
 static Result<Receipt> ReturnCrateForRetirementUnderTransaction(Database&,bool,const Request&,InventorySessionFence&,
  const OccupiedRetirement::Authority&,HouseBinding::CustodyContext* =nullptr,CrateBinding::CustodyContext* =nullptr,
  const Scene::HouseScenePolicy* =nullptr,const InteriorContent::ProfilePolicy* =nullptr);
 Result<Receipt> Operate(const Request&,InventorySessionFence&);
 Result<Receipt> OperateCrate(const Request&,InventorySessionFence&,const Scene::HouseScenePolicy* =nullptr);
 Result<Receipt> OperateInterior(const Request&,InventorySessionFence&,const Scene::HouseScenePolicy&,const InteriorContent::ProfilePolicy&);
 Result<Receipt> Resolve(const Lease&,const std::string& request_key,InventorySessionFence&); // lookup only; fresh connection after uncertain COMMIT
 Result<bool> AcknowledgeApplied(const Lease&,const std::string& request_key,InventorySessionFence&);
 Result<Stored> ReadStored(const Lease&,uint64_t custody_id,InventorySessionFence&);
 // Snapshot only: checked real source row/fence and current own inventory
 // binding. No adoption, property hydration, placement or route authority.
 Result<std::shared_ptr<const HouseBinding::Binding>> ReadInventoryHouse(const Lease&,
  const InventoryRow&,int32_t runtime_serial,InventorySessionFence&);
 Result<std::shared_ptr<const CrateBinding::Allocation>> ReadInventoryCrate(const Lease&,
  const InventoryRow&,int32_t runtime_serial,InventorySessionFence&);
 // Structural custody snapshot only; no access or delivery authority. Caller owns TX.
 static Result<Stored> InspectCustodyUnderTransaction(Database&,uint64_t custody_id);
 // Structural conservation only; contained origins require their owning graph
 // reader. This overload grants no access, visibility or mutation authority.
 static Result<Stored> InspectCustodyUnderTransaction(Database&,uint64_t custody_id,bool allow_contained);
 static std::string SchemaDescriptor();
 // Read-only proof for the allocator inside the CURRENT parent Store tx. Uses
 // the real request codec, held lease, untouched memory source and placed
 // custody postimage; never grants allocation/admission or starts a transaction.
 static Result<bool> VerifyHouseAllocationParent(Database&,const Request&,InventorySessionFence&);
 // Same exact parent receipt/custody/pose/fence proof, without asserting a
 // house route. Container allocators must separately verify their type,
 // content, current rights and allocation; this proof grants none of them.
 static Result<bool> VerifyPlacedStoreAllocationParent(Database&,const Request&,InventorySessionFence&);
 // Validation only, inside a caller-owned recovery transaction after certified
 // exact writer death. Never applies memory or marks receipt.applied.
 static Result<Receipt> ValidateRestartOutcome(Database&,const Lease&);
 // Only paired refund death recovery may revalidate an already ACKed item.
 // The default pending-only entry point and symbol remain unchanged.
 static Result<Receipt> ValidateRestartOutcome(Database&,const Lease&,bool allow_acknowledged);
 // Internal retained-layout child: caller validates immutable aggregate/key,
 // exact token membership and postimage; nullptr retains the original contract.
 static Result<Receipt> ValidateRestartOutcome(Database&,const Lease&,const std::string* aggregate_pending);
private:
 // Recovery modes are intentionally separate: a retained-layout child cannot
 // borrow the funded-refund acknowledged-item exception.
 static Result<Receipt> ValidateRestartOutcomeCurrent(Database&,const Lease&,bool allow_acknowledged,const std::string* aggregate_pending);
 Result<Reservation> ReservePlacementCurrent(const PreviewRequest&,InventorySessionFence&,const CrateMemberContext*,const Scene::HouseScenePolicy*,Database* parent_transaction=nullptr);
 Result<MoveReceipt> MoveCurrent(const MoveRequest&,InventorySessionFence&,const Scene::HouseScenePolicy*,const InteriorContent::ProfilePolicy*);
 Result<InteriorReservation> ReserveInteriorPlacementCurrent(const InteriorPreviewRequest&,InventorySessionFence&,const Scene::HouseScenePolicy&,const InteriorContent::ProfilePolicy&,const CrateMemberContext*,Database* parent_transaction=nullptr);
 Result<Receipt> OperateCurrent(const Request&,InventorySessionFence&,const Scene::HouseScenePolicy*,const InteriorContent::ProfilePolicy*,Database* parent_transaction=nullptr,const OccupiedRetirement::Authority* retirement=nullptr);
 std::unique_ptr<Database> database_;bool enabled_=false;HouseBinding::CustodyContext* houses_=nullptr;CrateBinding::CustodyContext* crates_=nullptr;std::mutex mutex_;
};
}
#endif
