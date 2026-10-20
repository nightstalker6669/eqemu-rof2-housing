#ifndef EQEMU_HOUSING_SCENE_SERVICE_H
#define EQEMU_HOUSING_SCENE_SERVICE_H
#include "live_item_service.h"
#include "vault_storage.h"
#include "anchor_destination.h"
#include "current_estate_access.h"
#include "exterior_exit_geometry.h"
namespace Housing::HouseBinding { struct Binding; }
namespace Housing::InteriorContent {struct Profile;}
namespace Housing::CrateMemberScope {struct Membership;}
namespace Housing::Scene {
struct Item { LiveItems::Stored stored; std::string creator_name,estate_owner_name;std::shared_ptr<const HouseBinding::Binding> house;std::optional<PlacementPolicy::ItemMetadata> metadata;std::optional<PlacementPolicy::Geometry> geometry;std::shared_ptr<const CrateBinding::Allocation> crate;uint64_t contained_in_crate=0;std::shared_ptr<const CrateMemberScope::Membership> crate_membership; };
struct Snapshot;
// Transfer conservation can inspect a packed house without opening it for
// storage, placement, anchor use or admission. Ordinary readers stay strict.
enum class AttachedRead { Storage, TransferConservation };
// SERVER source proof only; never decoded from a client estate/instance request.
struct LobbyRequest { Route source;uint8_t door_id=0; };

struct EntryRequest { uint32_t native_plot_id=0,native_item_id=0;uint64_t house_id=0,house_revision=0,custody_id=0; };
struct ExitContext { EstateId estate_id=0;uint32_t native_plot_id=0,creator_character_id=0;Route source,neighborhood;uint64_t catalog_revision=0;EstateWire::Definition house_definition; };
struct ExteriorExitContext {ExitContext parent;RegionWire::Region region;std::optional<LiveItems::Stored> current_house;std::vector<ExteriorExit::Box> blockers;};
struct Admission { uint64_t house_id=0;uint32_t native_house_id=0;Route destination;std::array<float,4> spawn{},exit{}; };
struct LobbyAdmission { uint32_t guild_id=0;std::optional<Admission> admission; };
struct InteriorContent {LiveItems::Stored stored;uint64_t scope_revision=0;std::string creator_name;std::optional<LiveItems::InteriorReservation> room;std::shared_ptr<const CrateBinding::Allocation> crate;uint64_t contained_in_crate=0;std::shared_ptr<const CrateMemberScope::Membership> crate_membership;};
struct InteriorSnapshot {std::shared_ptr<const HouseBinding::Binding> house;ExitContext context;CurrentEstateAccess access;std::optional<Admission> exit;std::vector<InteriorContent> contents,crate_contents;std::vector<VaultStorage::Item> vault_items,crate_vault_items;std::shared_ptr<const Housing::InteriorContent::Profile> room_profile;uint64_t profile_revision=0;std::optional<Route> exterior_source;};
class HouseScenePolicy {
public:
 virtual ~HouseScenePolicy()=default;
 // Remote anchor house review. Requires the actual current Client/held lease
 // and immutable reviewed house/template destination; no local doorway/range
 // assertion is substituted. Default denies. This hook grants no admission.
 virtual std::optional<ContentBinding::HouseRoute> ReviewedAnchorHouseUnderTransaction(
  Database&,LiveItems::InventorySessionFence&,const LiveItems::Lease&,
  const LiveItems::Stored&,const HouseBinding::Binding&,const EstateWire::Definition&)const{return std::nullopt;}

 // Dedicated lobby admission: real source, configured door/range and held
 // session must be rechecked. Defaults deny; yard methods are unchanged.
 virtual bool LobbySourceUnderTransaction(Database&,LiveItems::InventorySessionFence&,
   const LiveItems::Lease&,const LobbyRequest&,uint32_t)const{return false;}
 virtual std::optional<ContentBinding::HouseRoute> ReviewedLobbyRouteUnderTransaction(
   Database&,LiveItems::InventorySessionFence&,const LiveItems::Lease&,
   const LiveItems::Stored&,const HouseBinding::Binding&,const EstateWire::Definition&,
   const LobbyRequest&,uint32_t)const{return std::nullopt;}
 // Optional companion policy for plots containing both a house and crates.
 // Shared by attached-room/parent reads; ownership stays with the caller.
 virtual const CrateBinding::CustodyContext* Crates()const{return nullptr;}
 // Required SERVER immutable-template/route review for the actual locked
 // current house scene. Read-only: no adoption, inventory mutation, allocation,
 // admission or transaction lifecycle. Default service configuration denies.
 virtual std::optional<ContentBinding::HouseRoute> ReviewedRouteUnderTransaction(
   Database&,LiveItems::InventorySessionFence&,const LiveItems::Lease&,
   const LiveItems::Stored&,const HouseBinding::Binding&,const EstateWire::Definition&)const=0;
 // Separate admission witness, under the same current scene/lease/ACL locks.
 // Visibility or an instance ID alone never permits admission.
 virtual std::optional<ContentBinding::HouseRoute> ReviewedEntryUnderTransaction(
   Database&,LiveItems::InventorySessionFence&,const LiveItems::Lease&,
   const Snapshot&,const Item&)const {return std::nullopt;}
 // Egress uses current server interior/parent context, never the stale native
 // four-byte body. Enter ACL is deliberately not an exit requirement.
 virtual std::optional<ContentBinding::HouseRoute> ReviewedExitUnderTransaction(
   Database&,LiveItems::InventorySessionFence&,const LiveItems::Lease&,
   const HouseBinding::Binding&,const ExitContext&)const {return std::nullopt;}
 // Called only after current interior membership/house/parent proof, and
 // fresh locked plot geometry. Immutable route bytes remain unchanged.
 virtual std::optional<ExteriorExit::Destination> ReviewedExteriorPointUnderTransaction(
   Database&,LiveItems::InventorySessionFence&,const LiveItems::Lease&,
   const HouseBinding::Binding&,const ExteriorExitContext&,const ContentBinding::HouseRoute& immutable)const{return immutable.exit;}
 // Interior read is independent of egress; must prove the actual live source,
 // immutable room/feature/upkeep witness. The service separately locks Enter.
 virtual std::optional<ContentBinding::HouseRoute> ReviewedInteriorUnderTransaction(
   Database&,LiveItems::InventorySessionFence&,const LiveItems::Lease&,
   const HouseBinding::Binding&,const ExitContext&)const {return std::nullopt;}
 // Attached-house storage access from a fresh, Enter-authorized yard scene.
 // Separate from admission and actual occupancy; default denies.
 virtual std::optional<ContentBinding::HouseRoute> ReviewedAttachedInteriorUnderTransaction(
   Database&,LiveItems::InventorySessionFence&,const LiveItems::Lease&,
   const Snapshot&,const Item&)const {return std::nullopt;}
};
struct Snapshot { EstateId estate_id=0;uint32_t native_plot_id=0;uint64_t acl_revision=0;Route route;AccessPolicy::EffectiveAccess access;std::vector<Item> items;uint64_t current_catalog_revision=0;std::optional<Admission> admission;EstateWire::Definition definition;RegionWire::Region region;std::vector<Item> crate_contents;std::vector<VaultStorage::Item> vault_items,crate_vault_items;std::vector<RegionWire::Region> additional_regions; };
// Current read only. Enter authorizes visibility; no inventory ownership transfer.
// Caller supplies genuine held lease and main-thread current route proof.
class Service {
public:
 explicit Service(std::unique_ptr<Database>,bool enabled=false,const HouseScenePolicy* houses=nullptr,const Housing::InteriorContent::ProfilePolicy* room_profile=nullptr,const CrateBinding::CustodyContext* crates=nullptr);
 ~Service();
 LiveItems::Result<Snapshot> ReadScene(const LiveItems::Lease&,EstateId,LiveItems::InventorySessionFence&);
 // Remote spell path: real recipient lease plus locked caster ownership/guild,
 // exact placed anchor, current ACL/content/pose and verified yard/house route.
 // admit installs only the already-validated destination membership. expected
 // must match a pending offer exactly; it never supplies destination authority.
 LiveItems::Result<AnchorDestination> ResolveAnchor(const LiveItems::Lease&,
  const AnchorRequest&,LiveItems::InventorySessionFence&,bool admit=false,
  const AnchorDestination* expected=nullptr);

 // Successful empty admission means no currently placed own-guild hall: classic
 // fallback. Every error denies; unavailable/ambiguous state never falls back.
 LiveItems::Result<LobbyAdmission> AdmitOwnGuildHallFromLobby(
   const LiveItems::Lease&,const LobbyRequest&,LiveItems::InventorySessionFence&);
 // Caller owns transaction. No admission or transaction lifecycle changes.
 static LiveItems::Result<Snapshot> ReadSceneUnderTransaction(Database&,const LiveItems::Lease&,EstateId,LiveItems::InventorySessionFence&,const HouseScenePolicy* =nullptr,const CrateBinding::CustodyContext* =nullptr);
 // Parent item visibility from a genuinely occupied, reviewed house. This does
 // not grant a neighborhood position proof, admission, or inventory ownership.
 static LiveItems::Result<Snapshot> ReadParentSceneUnderTransaction(Database&,const LiveItems::Lease&,Route current_server_source,LiveItems::InventorySessionFence&,const HouseScenePolicy&,const CrateBinding::CustodyContext* =nullptr);
 LiveItems::Result<Snapshot> ReadParentScene(const LiveItems::Lease&,Route current_server_source,LiveItems::InventorySessionFence&);
 // Current, remotely reviewed hosting house context for the anchor profile
 // reader. Visibility only; ResolveAnchor separately validates the anchor and
 // inserts instance membership only after complete content/pose authorization.
 static LiveItems::Result<Snapshot> ReadTrophyYardSourceUnderTransaction(Database&,const LiveItems::Lease&,EstateId,uint64_t custody_id,LiveItems::InventorySessionFence&,const HouseScenePolicy* =nullptr,const CrateBinding::CustodyContext* =nullptr);
 // Remote read of ONE actual room source for ongoing trophy benefits. This
 // validates the existing reviewed house/profile/placement and Enter ACL only;
 // it installs no admission and confers no inventory or movement capability.
 static LiveItems::Result<InteriorSnapshot> ReadTrophySourceUnderTransaction(Database&,const LiveItems::Lease&,EstateId,uint64_t house_id,uint64_t custody_id,LiveItems::InventorySessionFence&,const HouseScenePolicy&,const Housing::InteriorContent::ProfilePolicy&,const CrateBinding::CustodyContext* =nullptr);
 static LiveItems::Result<InteriorSnapshot> ReadAnchorAttachedInteriorUnderTransaction(
  Database&,const LiveItems::Lease&,EstateId,uint64_t house_id,LiveItems::InventorySessionFence&,const HouseScenePolicy&);
 static LiveItems::Result<InteriorSnapshot> ReadAttachedInteriorUnderTransaction(Database&,const LiveItems::Lease&,EstateId,uint64_t house_id,LiveItems::InventorySessionFence&,const HouseScenePolicy&,const Housing::InteriorContent::ProfilePolicy* =nullptr,bool include_contents=false,const CrateBinding::CustodyContext* crates=nullptr,AttachedRead read=AttachedRead::Storage);
 LiveItems::Result<InteriorSnapshot> ReadAttachedInterior(const LiveItems::Lease&,EstateId,uint64_t house_id,LiveItems::InventorySessionFence&);
 // Resolve the current hosting estate from the placed house, then perform the
 // full attached read. Historical room item custody may retain an old estate.
 // Complete publication reads for collision authority: same-TX locked realm,
 // no pending-source omission. Returned contents grant no management rights.
 static LiveItems::Result<InteriorSnapshot> ReadPublishedInteriorCollisionUnderTransaction(Database&,const LiveItems::Lease&,Route,LiveItems::InventorySessionFence&,const HouseScenePolicy&,const Housing::InteriorContent::ProfilePolicy&,const CrateBinding::CustodyContext* crates=nullptr);
 static LiveItems::Result<InteriorSnapshot> ReadPublishedAttachedInteriorCollisionUnderTransaction(Database&,const LiveItems::Lease&,uint64_t house_id,LiveItems::InventorySessionFence&,const HouseScenePolicy&,const Housing::InteriorContent::ProfilePolicy&,const CrateBinding::CustodyContext* crates=nullptr);
 static LiveItems::Result<InteriorSnapshot> ReadCurrentAttachedInteriorUnderTransaction(Database&,const LiveItems::Lease&,uint64_t house_id,LiveItems::InventorySessionFence&,const HouseScenePolicy&,const Housing::InteriorContent::ProfilePolicy*,const CrateBinding::CustodyContext* crates=nullptr);
 LiveItems::Result<InteriorSnapshot> ReadCurrentAttachedInterior(const LiveItems::Lease&,uint64_t house_id,LiveItems::InventorySessionFence&);
 // Authorize and install instance membership atomically with current placed
 // custody, binding, route and Enter ACL. The caller zones only after return.
 // Same checked read for item transactions. Caller MUST own an active
 // database transaction and real held lease/fence; on failure it must roll back.
 // Never begins/commits/rolls back, allocates or installs admission. Returned
 // snapshot/locks are only valid inside that caller transaction. Contents are
 // omitted: each mutation validates its own item/scope/pose. Enter visibility
 // does not grant Add/Move/Remove; mutation must separately check its capability.
 static LiveItems::Result<InteriorSnapshot> ReadInteriorUnderTransaction(Database&,const LiveItems::Lease&,Route current_server_source,LiveItems::InventorySessionFence&,const HouseScenePolicy&);
 // Full authorized scene on the caller transaction; no lifecycle ownership
 // or mutation rights. Each write still requires its own capability and CAS.
 static LiveItems::Result<InteriorSnapshot> ReadInteriorSceneUnderTransaction(Database&,const LiveItems::Lease&,Route current_server_source,LiveItems::InventorySessionFence&,const HouseScenePolicy&,const Housing::InteriorContent::ProfilePolicy&,const CrateBinding::CustodyContext* crates=nullptr);
 LiveItems::Result<InteriorSnapshot> ReadInterior(const LiveItems::Lease&,Route current_server_source,LiveItems::InventorySessionFence&);
 // Metadata-only occupied view through verified egress context; no admission,
 // contents, room review or membership insertion. Rent does not trap management.
 // Strict currently placed host proof, even when rent has expired; no contents,
 // room review, admission, exit, membership effect or transaction lifecycle.
 static LiveItems::Result<InteriorSnapshot> ReadInteriorManagementUnderTransaction(Database&,const LiveItems::Lease&,Route,LiveItems::InventorySessionFence&,const HouseScenePolicy&);
 LiveItems::Result<InteriorSnapshot> ReadInteriorForManagement(const LiveItems::Lease&,Route,LiveItems::InventorySessionFence&);
 LiveItems::Result<Admission> ExitHouse(const LiveItems::Lease&,Route current_server_source,LiveItems::InventorySessionFence&);
 LiveItems::Result<Snapshot> AdmitHouse(const LiveItems::Lease&,EstateId,const EntryRequest&,LiveItems::InventorySessionFence&);
private:
 LiveItems::Result<InteriorSnapshot> ResolveInterior(const LiveItems::Lease&,Route,LiveItems::InventorySessionFence&,bool egress,bool management_view=false);
 LiveItems::Result<Snapshot> ReadCurrent(const LiveItems::Lease&,EstateId,LiveItems::InventorySessionFence&,const EntryRequest*);
 std::unique_ptr<Database> database_;bool enabled_;const HouseScenePolicy* houses_;const Housing::InteriorContent::ProfilePolicy* room_profile_;const CrateBinding::CustodyContext* crates_;std::mutex mutex_;
};
}
#endif
