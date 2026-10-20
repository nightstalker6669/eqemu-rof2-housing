#ifndef EQEMU_HOUSING_CRATE_BINDING_H
#define EQEMU_HOUSING_CRATE_BINDING_H
#include "live_item_service.h"
#include "crate_geometry_registry.h"
#include "current_estate_access.h"
namespace Housing::InventorySessions {struct Lease;}
namespace Housing::InteriorContent {struct PlacementReview;}
namespace Housing::Scene {struct InteriorSnapshot;}
namespace Housing::CrateBinding {
struct Binding {
 uint64_t crate_id=0,revision=0,custody_id=0,catalog_revision=0;
 uint32_t item_template_id=0,native_estate_id=0,parent_native_estate_id=0,definition_id=0;
 EstateId estate_id=0;
 std::string catalog_key;
};
// Immutable location snapshot in a parent receipt. Later movement must not
// rewrite a historical allocation/transition witness.
struct Allocation {
 Binding binding;
 uint32_t created_by_character_id=0;
 std::array<uint8_t,32> content_manifest_sha256{};
 std::string creation_request_key;
 bool packed=false;
 uint32_t inventory_character_id=0,inventory_slot_id=0; // Nonzero character only when carried; custody_id then0.
 uint32_t parcel_id=0; // Durable housing parcel; no inventory slot or custody root.
};
struct PlacementFacts {
 const LiveItems::Request& parent;
 uint64_t custody_id=0;
 const EstateWire::Definition& plot;
 const EstateWire::Definition& crate;
 const RegionWire::Region& region;
 const CrateGeometry::Record& model;
 const CurrentEstateAccess& access;
 uint64_t catalog_revision=0;
 const Allocation* carried=nullptr;
};
// Actual held inventory/template/model/native metadata, unbound source identity,
// reviewed geometry containment/capacity and feature/economy support must be
// attested by a server-owned implementation. SQL classification is insufficient.
// This callback supplies no transaction lifecycle and cannot pump game events.
class ReviewedPlacement {
public:
 virtual ~ReviewedPlacement()=default;
 virtual bool VerifiedUnderTransaction(Database&,LiveItems::InventorySessionFence&,const PlacementFacts&)const=0;
 virtual bool ReparentReviewedUnderTransaction(Database&,LiveItems::InventorySessionFence&,const PlacementFacts&)const{return false;}
 virtual bool CatalogRelocationReviewedUnderTransaction(Database&,LiveItems::InventorySessionFence&,const PlacementFacts&)const{return false;}
 virtual std::array<uint8_t,32> ManifestDigest()const noexcept=0;
};
enum class Stage { Preview,Store,Read,Move };
struct ContentFacts {
 LiveItems::Lease lease;LiveItems::Reservation reservation;LiveItems::InventoryRow source;
 int32_t runtime_serial=0;
 std::optional<PlacementPolicy::Transform> pose;
 Stage stage=Stage::Preview;
 uint64_t catalog_revision=0;
 uint16_t neighborhood_instance=0;
 CurrentEstateAccess access;
 EstateWire::Definition plot,crate;
 RegionWire::Region region;
};
class CustodyContext:public ReviewedPlacement {
public:
 virtual bool ReviewInteriorPlacementUnderTransaction(Database&,LiveItems::InventorySessionFence&,
  const LiveItems::Request&,const InteriorContent::PlacementReview&,const Allocation*)const{return false;}
 virtual bool ReviewInteriorSceneUnderTransaction(Database&,LiveItems::InventorySessionFence&,
  const LiveItems::Lease&,const Scene::InteriorSnapshot&,const LiveItems::Stored&,const Allocation&)const{return false;}
 // Read-only parent yard of an independently authenticated occupied house.
 // Default deny; never substitutes a neighborhood placement fence for mutations.
 virtual std::optional<ContentBinding::CrateProfile> ReviewParentSceneUnderTransaction(
  Database&,LiveItems::InventorySessionFence&,const ContentFacts&,const Scene::InteriorSnapshot&)const{return {};}
 virtual std::optional<ContentBinding::CrateProfile> ReviewContentUnderTransaction(
  Database&,LiveItems::InventorySessionFence&,const ContentFacts&)const=0;
 virtual bool ReviewPickupUnderTransaction(Database&,LiveItems::InventorySessionFence&,
  const LiveItems::Lease&,const LiveItems::Stored&,const Allocation&)const{return false;}
};
class ReloadPolicy {
public:
 virtual ~ReloadPolicy()=default;
 virtual std::array<uint8_t,32> ManifestDigest()const noexcept=0;
 virtual bool ReviewedUnderTransaction(Database&,const InventorySessions::Lease&,
  const LiveItems::InventoryRow&,const Allocation&)const=0;
};
// Caller owns the SAME live Store transaction. Success does not commit it;
// failure requires caller rollback. This allocates a native type7 estate only,
// never a zone instance. No runtime hook is enabled by this helper.
LiveItems::Result<Binding> BindNewPlacedUnderTransaction(Database&,const LiveItems::Request&,
 LiveItems::InventorySessionFence&,const ReviewedPlacement&);
LiveItems::Result<Binding> RebindPlacedUnderTransaction(Database&,const LiveItems::Request&,
 LiveItems::InventorySessionFence&,const ReviewedPlacement&,const Allocation& carried,HouseBinding::CustodyContext* houses=nullptr);
LiveItems::Result<Binding> BindInteriorPlacedUnderTransaction(Database&,const LiveItems::Request&,
 LiveItems::InventorySessionFence&,const CustodyContext&,const InteriorContent::PlacementReview&,const Allocation* carried=nullptr);
bool Installed(Database&);
// Read-only structural/identity validation under the caller's transaction.
// This supplies no ACL or live simulation authority.
LiveItems::Result<Allocation> ReadPlacedUnderTransaction(Database&,const LiveItems::Stored&);
// Structural room-native lookup for catalog visibility. Caller owns the
// transaction; this grants no room Enter, item visibility or mutation rights.
LiveItems::Result<Allocation> ReadInteriorNativeUnderTransaction(Database&,uint32_t native_estate_id);
LiveItems::Result<Allocation> ReadCarriedUnderTransaction(Database&,const LiveItems::InventoryRow&);
// Structural conservation check only; no authority to move or expose contents.
// Capacity metadata only. Room children use their own retained room profile,
// which need not publish a crate definition in the neighborhood catalog.
LiveItems::Result<EstateWire::Definition> ReadDefinitionUnderTransaction(Database&,const Allocation&);
LiveItems::Result<uint32_t> ValidateMembersUnderTransaction(Database&,const Allocation&);
// Parent Return transaction only: release room membership of packed contents;
// retain typed origin reservations until a reviewed destination rebinds them.
LiveItems::Result<uint32_t> ReleaseInteriorMemberScopesUnderTransaction(Database&,const Allocation&);
std::vector<uint8_t> EncodeAllocation(const Allocation&);
LiveItems::Result<Allocation> DecodeAllocation(const std::vector<uint8_t>&);
bool CoherentOutcome(const LiveItems::Receipt&)noexcept;
// Pending parent receipt has already been authenticated/decoded by LiveItems.
// Historical applied replay must not compare a later current house location.
bool RelocationEffectsMatch(Database&,const LiveItems::Receipt&);
}
#endif
