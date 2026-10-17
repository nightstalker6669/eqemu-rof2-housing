#ifndef EQEMU_HOUSING_ITEM_SERVICE_H
#define EQEMU_HOUSING_ITEM_SERVICE_H
#include "persistence_service.h"
#include <array>
#include <optional>
#include <utility>
namespace Housing::Items {
// Synthetic domain identities/slot numbers are NOT native item GUIDs/slots/opcodes.
struct IsolatedFixture {};
struct CallerVerifiedGeometry {};
struct Transform {
    std::array<double,3> position{};
    std::array<double,3> rotation{}; // Units/bounds supplied by caller; never assumed.
    double scale=1;
};
struct VerifiedBounds {
    EstateId estate_id;
    std::string provenance;
    std::array<double,3> minimum_position,maximum_position,minimum_rotation,maximum_rotation;
    double minimum_scale,maximum_scale;
    VerifiedBounds(CallerVerifiedGeometry, EstateId estate,std::string proof,
        std::array<double,3> pmin,std::array<double,3> pmax,
        std::array<double,3> rmin,std::array<double,3> rmax,double smin,double smax)
        :estate_id(estate),provenance(std::move(proof)),minimum_position(pmin),maximum_position(pmax),
         minimum_rotation(rmin),maximum_rotation(rmax),minimum_scale(smin),maximum_scale(smax) {}
};
struct Identity {
    std::string instance_key; // Globally unique within this isolated custody domain.
    uint32_t catalog_item_id=0;
    uint32_t original_owner_character_id=0;
    int32_t lore_group=0; // Captured opaque metadata; no invented lore evaluator.
    bool no_drop=false;
    std::string format,provenance;
    std::vector<uint8_t> payload; // Exact bytes; NEVER ItemInstance::Serialize pointer data.
};
enum class Location:uint8_t { Inventory=0, Stored=1, Placed=2 };
struct Snapshot {
    Identity identity;
    uint64_t revision=0;
    Location location=Location::Inventory;
    EstateId estate_id=0;
    uint32_t inventory_slot=0; // Synthetic positive slot identity;0 outside inventory.
    Transform transform;
    std::string bounds_provenance;
};
enum class Action:uint8_t { ToEstate=0, Place=1, Store=2, MovePlaced=3, ReturnInventory=4 };
struct Request {
    std::string request_key;
    AuthenticatedPurchaser actor;
    EstateId estate_id=0;
    Action action=Action::ToEstate;
    Snapshot expected; // Entire exact record/bytes and revision CAS, not just item ID.
    uint32_t destination_slot=0;
    Transform transform;
    std::optional<VerifiedBounds> bounds;
};
struct Receipt { Snapshot after; bool replayed=false; };
enum class Error { None, Disabled, UnsupportedFunding, InvalidArgument, InvalidGeometry,
    Unauthorized, NotFound, Conflict, DatabaseFailure, CommitUncertain };
template<class T> struct Result { T value{};Error error=Error::None;explicit operator bool()const{return error==Error::None;} };
class Service {
public:
    explicit Service(std::unique_ptr<Database> db,bool enabled=false);
    Service(std::unique_ptr<Database> db,bool enabled,IsolatedFixture);
    ~Service();
    Result<Receipt> Operate(const Request&); // Production adapter absent, fail closed.
    Result<Snapshot> SeedIsolatedInventory(AuthenticatedPurchaser,const Identity&,uint32_t slot);
    Result<Snapshot> ReadIsolatedItem(AuthenticatedPurchaser,const std::string& instance_key);
    Result<Receipt> OperateIsolated(const Request&);
    // Historical AFTER receipt is not a current inventory synchronization response.
private:
    std::unique_ptr<Database> database_;bool enabled_=false,fixture_=false;std::mutex mutex_;
};
}
#endif
