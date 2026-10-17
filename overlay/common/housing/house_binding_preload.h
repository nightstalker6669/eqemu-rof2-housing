#ifndef EQEMU_HOUSING_HOUSE_BINDING_PRELOAD_H
#define EQEMU_HOUSING_HOUSE_BINDING_PRELOAD_H
#include "house_binding_service.h"
#include "inventory_session_service.h"
#include "crate_binding.h"
namespace EQ {
    class InventoryProfile;
}
namespace Housing::HousePreload {
    enum class Error {
        None, Disabled, InvalidArgument, Unauthorized, UnheldReservation,
        PendingApplication, Conflict, UnsupportedItem, DatabaseFailure, CommitUncertain
    };
    template<class T> struct Result {
        T value{
        };
        Error error=Error::None;
        explicit operator bool()const{
            return error==Error::None;
        }
    };
    struct Entry {
        HouseBinding::Binding binding;
        LiveItems::InventoryRow source;
        uint32_t item_definition_id=0;
    };
    struct Capture {
        InventorySessions::Lease reservation;
        std::vector<Entry> entries;
        struct CrateEntry {CrateBinding::Allocation binding;LiveItems::InventoryRow source;uint32_t placement_mask=0,minimum_scale_bits=0,maximum_scale_bits=0;};
        std::vector<CrateEntry> crates;
    };
    struct Loaded {
        LiveItems::InventoryRow persisted;
        int32_t runtime_serial=0;
    };
    // Prepare the GUID-only postimage from an exact SQL load snapshot. This is
    // not commit or memory proof: export only after the checked loader commits,
    // then AfterLoad still verifies SQL and the real SessionFence's item pointer.
    Result<Loaded> ProjectGuidUpdate(const InventoryRepository::LoadRow&,int32_t serial);
    // GetInventory exports all character inventory rows. Select the exact house
    // subset captured before that load; this is framing, not SQL or memory proof.
    Result<std::vector<Loaded>> SelectBoundLoadWitnesses(const Capture&,const std::vector<Loaded>&);
    class SessionFence {
        public:
        virtual ~SessionFence()=default;
        // Real ZoneReservation identity/connection/thread held across capture, loader
        // and recheck. No packet flag or copied DB row is proof of local ownership.
        virtual bool HeldFor(const InventorySessions::Lease&)const noexcept=0;
        // Must prove the actual current item/pointer, all representable raw17
        // metadata, runtime serial and simple cursor/no-queue projection. NULL bits
        // remain SQL facts compared separately; GUID alone proves no item identity.
        virtual bool LoadedMatches(const LiveItems::InventoryRow&,int32_t serial)const noexcept=0;
    };
    class Validated {
        public:
        const std::vector<Entry>& Entries()const{
            return capture_.entries;
        }
        private:
        friend class Service;
        Capture capture_;
        std::vector<Loaded> loaded_;
    };
    // Read-only exclusive DB. Each read locks in its OWN checked transaction and
    // releases SQL locks before returning; durable009 and the local fence remain.
    // Do not hold these SQL locks while GetInventory writes on another connection.
    class Service {
        public:
        explicit Service(std::unique_ptr<Database>,bool enabled=false);
        Result<Capture> BeforeLoad(const InventorySessions::Lease&,SessionFence&,
        const HouseBinding::HouseTemplatePolicy&,const HouseBinding::InteriorRoutePolicy* = nullptr,const CrateBinding::ReloadPolicy* = nullptr);
        Result<Validated> AfterLoad(const Capture&,const std::vector<Loaded>&,SessionFence&,
        const HouseBinding::HouseTemplatePolicy&,const HouseBinding::InteriorRoutePolicy* = nullptr,const CrateBinding::ReloadPolicy* = nullptr);
        // Actual loader handoff, preserving AfterLoad's exact-subset contract.
        Result<Validated> AfterInventoryLoad(const Capture&,const std::vector<Loaded>&,SessionFence&,
        const HouseBinding::HouseTemplatePolicy&,const HouseBinding::InteriorRoutePolicy* = nullptr,const CrateBinding::ReloadPolicy* = nullptr);
        // All pointer/serial/template/property checks precede any property assignment.
        // Must immediately follow AfterLoad on the same held zone-thread reservation.
        static Result<bool> Hydrate(const Validated&,EQ::InventoryProfile&,SessionFence&);
        private:
        std::unique_ptr<Database> database_;
        bool enabled_;
        std::mutex mutex_;
    };
}
#endif
