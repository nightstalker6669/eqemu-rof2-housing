#include "../common/housing/house_swap_service.h"
#ifndef EQEMU_HOUSING_INVENTORY_BRIDGE_H
#define EQEMU_HOUSING_INVENTORY_BRIDGE_H
#include "housing_inventory_identity.h"
#include "housing_guild_portal.h"
#include "../common/housing/ownership_transfer_service.h"
#include "../common/housing/trophy_benefits_db.h"
#include "../common/housing/guild_trophies_db.h"
#include "../common/housing/live_escrow_service.h"
#include "../common/housing/native_plot_purchase.h"
#include "../common/housing/scene_service.h"
#include "../common/housing/interior_profile_db.h"
#include "../common/housing/native_access_service.h"
#include "../common/housing/house_binding_service.h"
#include "../common/housing/crate_binding.h"
#include "../common/housing/crate_storage_service.h"
#include <set>
#include <functional>
#include <map>
namespace Housing::InteriorStorage {struct StowRequest;struct Request;struct Receipt;struct TransferRequest;struct TransferReceipt;}
namespace Housing::LayoutInventoryJournal {struct Receipt;}
namespace Housing::LayoutMixedJournal {struct Receipt;}
namespace Housing::LayoutMixedCoordinator {struct Intent;}
class Client;
class EQStreamInterface;
class EQApplicationPacket;
namespace Housing::InventoryBridge {
// Call ONLY at zone main-thread startup AFTER CatchSignal has been changed to
// defer shutdown to the normal loop checkpoint. Never call from packet input.
void RegisterZoneThreadAtSafeStartup();
bool IsRegisteredZoneThread() noexcept;
// Synchronous bridge. No event-loop pumping, script calls, sends, retry callbacks
// or Client lifecycle transitions while its barrier is held. A DB outage after
// uncertain COMMIT intentionally blocks the simulation until durable resolution.
class Bridge final : private NativeAccess::InteriorSourcePolicy, private NativePlotPurchase::Fence, private OwnershipTransfer::Authority, public HouseBinding::HouseApplicationProof, private TrophyBenefits::FavorAuthority {
public:
 OwnershipTransfer::Result<OwnershipTransfer::Receipt> InspectPersonalPlotTransfer(const std::string&,const std::array<uint8_t,EstateWire::PurchaseSize>&);
 OwnershipTransfer::Result<OwnershipTransfer::Receipt> TransferPersonalPlot(const std::string&,const std::array<uint8_t,EstateWire::PurchaseSize>&,const OwnershipTransfer::Source&);
 struct HousePolicies {
  std::unique_ptr<HouseBinding::CustodyContext> custody;
  std::unique_ptr<Scene::HouseScenePolicy> scene;
  std::unique_ptr<InteriorContent::ProfilePolicy> room;
  std::unique_ptr<CrateBinding::CustodyContext> crates;
 };
 // Policies are owned by this exact Client bridge and remain alive throughout
 // every exclusive service operation. Construction must not send, run scripts,
 // yield, adopt items or change database state. No default house policy exists.
 using HousePolicyFactory=std::function<HousePolicies(Client&,LiveItems::InventorySessionFence&,HouseBinding::HouseApplicationProof&)>;
 using ConnectionFactory=std::function<std::unique_ptr<LiveItems::Service>(HouseBinding::CustodyContext*,CrateBinding::CustodyContext*)>;
 static bool CanCreateForClient(Client&) noexcept;
 Bridge(Client&,ConnectionFactory,HousePolicyFactory={});
 ~Bridge();
 LiveItems::Result<LiveItems::Receipt> Store(const std::string& request_key,EstateId,const LiveItems::InventoryRow& trusted_db_snapshot,std::optional<LiveItems::Placed> placement=std::nullopt,std::optional<LiveItems::HouseIdentity> house=std::nullopt,std::optional<LiveItems::InteriorReservation> room=std::nullopt,bool initial_stowed=false,std::optional<Route> room_exterior_source=std::nullopt,std::optional<LiveItems::CrateIdentity> crate=std::nullopt,std::optional<LiveItems::CrateMemberContext> crate_member=std::nullopt,std::optional<LiveItems::VaultContext> vault=std::nullopt,uint32_t quantity=0);
 LiveItems::Result<LiveItems::Receipt> ReturnVault(const std::string&,const VaultStorage::Item&,const LiveItems::VaultContext&,uint32_t destination,uint32_t quantity=0);
 LiveItems::Result<LiveItems::VaultTransferReceipt> TransferVault(LiveItems::VaultTransferRequest&);
 LiveItems::Result<LiveItems::Reservation> ReservePlacement(LiveItems::PreviewRequest);
 LiveItems::Result<LiveItems::Reservation> ReserveCratePlacement(LiveItems::PreviewRequest,const LiveItems::CrateMemberContext&);
 LiveItems::Result<LiveItems::InteriorReservation> ReserveInteriorPlacement(LiveItems::InteriorPreviewRequest,std::optional<LiveItems::CrateMemberContext> destination=std::nullopt);
 LiveItems::Result<bool> CancelPlacement(uint32_t native_item_id);
 // One current inspection binds a native plot to its server generation.
 // Snapshot refresh/replayed packets never arm a new ownership generation.
 void ForgetPurchaseInspection() noexcept {purchase_inspection_.reset();}
 bool RememberPurchaseInspection(uint32_t native,const Catalog::Export& published);
 NativePlotPurchase::Result<NativePlotPurchase::Receipt> BuyPlot(const std::string&,const std::array<uint8_t,EstateWire::PurchaseSize>&);
 void RememberRetirementInspection(EstateId estate,uint32_t native){retirement_inspection_=estate?std::optional<std::pair<EstateId,uint32_t>>{{estate,native}}:std::nullopt;}
 std::optional<EstateId> ConsumeRetirementInspection(uint32_t native){auto prior=retirement_inspection_;retirement_inspection_.reset();return prior&&prior->second==native?std::optional<EstateId>{prior->first}:std::nullopt;}
 // Native Move Out occupied slice: stage exact returned inventory token,
 // retain barrier through COMMIT resolution, application and existing typed ACK.
 struct MoveOutResult {LiveItems::Receipt receipt;std::unique_ptr<EQApplicationPacket> inventory_packet;int32_t runtime_serial=0;bool refunded=false;};
 LiveItems::Result<MoveOutResult> MoveOutOccupied(const std::string& key,EstateId,uint32_t native_plot_id,bool enabled=false);
 struct ParcelClaimResult {std::unique_ptr<EQApplicationPacket> inventory_packet;};
 LiveItems::Result<ParcelClaimResult> ClaimHousingParcel(uint32_t parcel_id,uint32_t native_slot,uint32_t item_id);
 LiveItems::Result<uint32_t> RetireEmptyPlot(const std::string& key,EstateId,uint32_t native_plot_id);
 LiveItems::Result<uint64_t> SelectUpkeep(const std::string& key,EstateId,uint32_t native_plot_id,uint32_t component_id);
 LiveItems::Result<std::vector<LiveItems::Stored>> ReadPlaced(EstateId);
 LiveItems::Result<std::shared_ptr<const HouseBinding::Binding>> ReadInventoryHouse(const LiveItems::InventoryRow&);
 LiveItems::Result<std::shared_ptr<const CrateBinding::Allocation>> ReadInventoryCrate(const LiveItems::InventoryRow&);
 LiveItems::Result<LiveItems::MoveReceipt> MovePlaced(LiveItems::MoveRequest);
 LiveItems::Result<std::vector<InteriorStorage::Receipt>> StowAll(InteriorStorage::StowRequest);
 LiveItems::Result<InteriorStorage::Receipt> ChangeStorage(InteriorStorage::Request);
 LiveItems::Result<HouseSwap::Receipt> SwapHouse(const std::string&,EstateId,const std::array<uint8_t,24>&);
 LiveItems::Result<InteriorStorage::TransferReceipt> TransferStorage(InteriorStorage::TransferRequest);
 LiveItems::Result<CrateStorage::Receipt> TransferCrate(CrateStorage::Request);
 LiveItems::Result<CrateStorage::BatchReceipt> PackCrate(CrateStorage::BatchRequest);
 LiveItems::Result<Scene::Snapshot> ReadParentScene();
 LiveItems::Result<Scene::InteriorSnapshot> ReadAttachedInterior(EstateId,uint64_t house_id);
 LiveItems::Result<Scene::Snapshot> ReadScene(EstateId);
 LiveItems::Result<GuildTrophies::View> GuildTrophiesView(const GuildTrophies::Request&);
 LiveItems::Result<TrophyBenefits::View> Trophies(const TrophyBenefits::Request&);
 LiveItems::Result<Scene::AnchorDestination> ResolveAnchor(const Scene::AnchorRequest&,bool admit=false,const Scene::AnchorDestination* expected=nullptr);
 LiveItems::Result<Scene::Snapshot> AdmitHouse(EstateId,const Scene::EntryRequest&);
 LiveItems::Result<Scene::LobbyAdmission> AdmitOwnGuildHallFromLobby(const Scene::LobbyRequest&);
 LiveItems::Result<Scene::Admission> ExitHouse();
 LiveItems::Result<Scene::InteriorSnapshot> ReadInterior();
 LiveItems::Result<Scene::InteriorSnapshot> ReadInteriorForManagement();
 NativeAccess::Result<NativeAccess::Snapshot> ReadAccess(EstateId,uint32_t native_plot_id);
 NativeAccess::Result<NativeAccess::Receipt> ReplaceAccess(NativeAccess::Request);
 NativeAccess::Result<bool> SetVotingOptOut(EstateId,uint32_t native_plot_id,bool);
 LiveItems::Result<LiveItems::Receipt> ReturnCrateMember(const std::string& request_key,const LiveItems::Stored& server_cache,const LiveItems::CrateMemberContext&,uint32_t destination,std::optional<LiveItems::HouseIdentity> house=std::nullopt,uint32_t quantity=0);
 LiveItems::Result<LiveItems::Receipt> ReturnInterior(const std::string& request_key,const Scene::InteriorContent& server_cache,const LiveItems::InteriorMoveContext& current_context,uint32_t destination,std::optional<Route> room_exterior_source=std::nullopt,std::optional<LiveItems::CrateMemberContext> crate_member=std::nullopt,uint32_t quantity=0);
 LiveItems::Result<LiveItems::Receipt> Return(const std::string& request_key,EstateId,uint64_t custody_id,uint32_t destination,std::optional<LiveItems::HouseIdentity> house=std::nullopt,std::optional<LiveItems::CrateIdentity> crate=std::nullopt,uint32_t quantity=0);
 bool AppliedHouseEffect(const HouseBinding::Effect&)const noexcept override;
 // Server-issued key and canonical estate; no native sends while held. Explicit
 // backend enablement defaults false until the native response path is verified.
 LiveEscrow::Result<LiveEscrow::Receipt> TransferCoin(const std::string& request_key,
  EstateId,const EscrowWire::Message&,bool enabled=false);
 // Server-issued key and freshly resolved destination intents, never raw
 // client estate/lease authority. Inventory member stage only; native mapping
 // stays disabled until mixed property coordination and response gates pass.
 LiveItems::Result<LayoutInventoryJournal::Receipt> LoadLayoutInventory(const std::string&,
  const std::vector<uint8_t>&,const std::vector<LiveItems::LayoutUnitDestination>&,bool enabled=false);
 // Exact native scene owner slots stay alive/unmoved through durable ACK.
 // Caller refreshes world/items and clears native busy only after this returns.
 LiveItems::Result<LayoutMixedJournal::Receipt> LoadLayoutMixed(const std::string&,
  const std::vector<uint8_t>&,const LayoutMixedCoordinator::Intent&,
  const std::map<uint64_t,std::unique_ptr<EQ::ItemInstance>*>&,bool enabled=false);
 LiveItems::Result<GuildPortal::Alignment> AlignGuildPortal(const GuildPortalRuntime::Source&,uint32_t trade_slot,int32_t runtime_serial,const GuildPortalRuntime::Validator&);
 LiveItems::Result<bool> CloseSession(); // Lifecycle owner must call before Client destruction.
private:
 OwnershipTransfer::Result<OwnershipTransfer::Receipt> ExecutePersonalPlotTransfer(const std::string&,const std::array<uint8_t,EstateWire::PurchaseSize>&,bool,const OwnershipTransfer::Source*);
 const OwnershipTransfer::Source* transfer_expected_source_=nullptr;
 bool TransferContentsReviewedUnderTransaction(Database&,const OwnershipTransfer::Request&,const OwnershipTransfer::Source&,LiveItems::InventorySessionFence&)const override;
 bool TransferFrameMatches(const OwnershipTransfer::Request&,const OwnershipTransfer::Source&)const noexcept override;
 std::optional<uint32_t> CurrentFavorUnderFence(const LiveItems::Lease&)const noexcept override;
 bool HeldFor(AuthenticatedPurchaser,uint64_t,std::string_view)const noexcept override;
 bool SourceMatches(const LiveItems::InventoryRow&,int32_t)const noexcept override;
 bool LayoutSourceMatches(const LiveItems::InventoryRow&,int32_t)const noexcept override;
 bool NpcProfileMatchesUnderTransaction(Database&,const LiveItems::Lease&,const NpcRuntime::Profile&)const noexcept override;
 bool MountSurfaceUnderTransaction(Database&,const LiveItems::Lease&,const LiveItems::InventoryRow&,const PlacementPolicy::Transform&,uint32_t)const noexcept override;
 bool MountSurfaceWithSubjectUnderTransaction(Database&,const LiveItems::Lease&,const LiveItems::InventoryRow&,const PlacementPolicy::Transform&,uint32_t,const LiveItems::MountSubject&)const noexcept override;
 bool MountInteriorSurfaceUnderTransaction(Database&,const LiveItems::Lease&,const LiveItems::InventoryRow&,const PlacementPolicy::Transform&,uint32_t,const LiveItems::InteriorMountContext&)const noexcept override;
 bool MountSurfaceSelectedUnderTransaction(Database&,const LiveItems::Lease&,const LiveItems::InventoryRow&,const PlacementPolicy::Transform&,uint32_t,const LiveItems::InteriorMountContext*,const LiveItems::MountSubject& = {})const noexcept;
 bool ReturnAllowed(const LiveItems::InventoryRow&)const noexcept override;
 bool PlacementContextMatches(EstateId,uint32_t,uint16_t)const noexcept override;
 bool ParentMatchesUnderTransaction(Database&,const LiveItems::Lease&,LiveItems::InventorySessionFence&,EstateId,uint32_t,uint16_t)const noexcept override;
 bool InteriorCoinSourceMatches(const LiveEscrow::InteriorSource&)const noexcept override;
 bool Applied(const LiveItems::Receipt&)const noexcept override;
 bool CurrencyMatches(const LiveEscrow::Currency&)const noexcept override;
 bool CursorItemsEmpty()const noexcept override;
 bool GuildExpansionAllowed()const noexcept override;
 bool InspectedOfferMatches(const NativePlotPurchase::Offer&)const noexcept override;
 bool PurchaseApplied(const NativePlotPurchase::Receipt&)const noexcept override;
 bool ApplyPurchase(const NativePlotPurchase::Receipt&);
 bool CoinApplied(const LiveEscrow::Receipt&)const noexcept override;
 bool ApplyCoin(const LiveEscrow::Receipt&);
 LiveEscrow::Result<LiveEscrow::Receipt> ExecuteCoin(LiveEscrow::Request);
 bool Apply(const LiveItems::Receipt&);
 LiveItems::Result<LiveItems::Receipt> Execute(LiveItems::Request);
 LiveItems::Result<Scene::InteriorSnapshot> ResolveInterior(bool egress,bool management_view=false);
 LiveItems::Result<Scene::Snapshot> ReadCurrentScene(EstateId,const Scene::EntryRequest*);
 bool Enter(bool closing=false);void Leave();bool Open();
 std::unique_ptr<LiveItems::Service> Fresh() const;
 [[noreturn]] void FreezeInvariantFailure()const;
 Client& client_;ConnectionFactory factory_;AuthenticatedPurchaser actor_;EQStreamInterface* connection_=nullptr;
 std::string session_key_;LiveItems::Lease lease_;bool held_=false;bool trophy_sources_changed_=false;
 std::map<uint64_t,std::unique_ptr<EQ::ItemInstance>> retained_;
 std::map<std::string,std::unique_ptr<EQ::ItemInstance>> partition_pieces_;
 // Only actual server Reconstruct calls may authorize unbound property hydration
 // on a recovered pickup. A matching row or packet never supplies this proof.
 std::set<uint64_t> reconstructed_custody_;
 HousePolicies house_policies_;
 std::optional<NativePlotPurchase::Offer> purchase_inspection_;
 std::optional<std::pair<EstateId,uint32_t>> retirement_inspection_;
};
}
#endif
