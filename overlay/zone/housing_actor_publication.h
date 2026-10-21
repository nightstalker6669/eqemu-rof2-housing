#ifndef EQEMU_ZONE_HOUSING_ACTOR_PUBLICATION_H
#define EQEMU_ZONE_HOUSING_ACTOR_PUBLICATION_H
#include "../common/eq_packet_structs.h"
#include "../common/housing/placed_height_filters.h"
#include <map>
#include <optional>
class Client;
class EQApplicationPacket;
namespace Housing::NpcRuntime {struct Profile;}
namespace Housing::ActorPublication {
// Per-recipient ledger of packets actually handed to the stream, after delayed
// packet/filter handling. A spawn-ID reuse obtains a new publication generation.
// Appearance transitions invalidate the original ordinary-construction proof;
// only a new complete spawn publication can restore it.
struct Record {Spawn_Struct spawn{};uint64_t generation{};bool ordinary{};};
struct DoorRecord {Door_Struct door{};uint64_t generation{};bool closed_resource=false;};
class Ledger {
 std::map<uint32_t,Record> records_;
 std::map<uint32_t,DoorRecord> doors_;
 bool doors_reset_=true;
 uint64_t revision_=1;
 bool valid_=true;
 void Advance() noexcept;
 void Dirty(uint32_t) noexcept;
public:
 explicit Ledger(bool enabled=true):valid_(enabled){}
 void Observe(const EQApplicationPacket*) noexcept;
 uint64_t Revision()const noexcept{return valid_?revision_:0;}
 const std::map<uint32_t,Record>& Records()const noexcept{return records_;}
 bool DoorsPublished()const noexcept{return valid_;}
 const std::map<uint32_t,DoorRecord>& PublishedDoors()const noexcept{return doors_;}
 const DoorRecord* FindDoor(uint32_t id)const noexcept{auto p=doors_.find(id);return valid_&&p!=doors_.end()?&p->second:nullptr;}
 const Record* Find(uint32_t id)const noexcept{auto p=records_.find(id);return valid_&&p!=records_.end()?&p->second:nullptr;}
};
uint64_t Revision(const Client&) noexcept;
const Ledger& Publication(const Client&) noexcept;
void Observe(const Client&,const EQApplicationPacket*) noexcept;
void Forget(const Client&) noexcept;
std::optional<PlacedHeightFilters::Actor> ProjectHeightActor(Client&,uint16_t) noexcept;
// A source-linked real network NPC, never the noninteractive CPlacedNPC local
// construction. Caller binds fresh SQL metadata/cache/pose and service authority
// under its existing custody transaction before using this projection.
std::optional<PlacedHeightFilters::Actor> ProjectServiceHeightActor(Client&,uint16_t,
 const NpcRuntime::Profile&,uint64_t custody_id,uint32_t native_estate_id,uint32_t native_item_id) noexcept;
}
#endif
