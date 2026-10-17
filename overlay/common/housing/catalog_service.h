#ifndef EQEMU_HOUSING_CATALOG_SERVICE_H
#define EQEMU_HOUSING_CATALOG_SERVICE_H
#include "persistence_service.h"
#include "estate_wire.h"
#include "region_wire.h"
namespace Housing::Catalog {
struct NativeEstateId { uint32_t value=0; };
enum class Origin:uint8_t { HistoricalVerified=1, Experimental=2 };
struct Provenance {
 Origin origin=Origin::Experimental;
 std::string source; // Required provenance reference; experiments begin EXPERIMENTAL:.
 std::string sha256; // Digest of supplied evidence/import source, not an authenticity claim.
 uint32_t client_build=20130510;
};
struct Node {
 std::string key,parent_key;
 NativeEstateId native_id; // Assigned by service, distinct from all domain IDs.
 uint32_t neighborhood_id=0,plot_id=0,ownership_estate_id=0;
 uint32_t definition_id=0,static_id=0;
};
struct Bundle {
 std::string key;
 uint64_t revision=0;
 Provenance provenance;
 std::vector<EstateWire::Definition> definitions;
 std::vector<EstateWire::CostScheme> costs;
 std::vector<RegionWire::Region> regions;
 std::vector<Node> nodes;
};
struct Export {
 Bundle catalog;
 std::vector<uint8_t> definition_payload,region_payload;
};
struct Settings { bool enabled=false; };
// Administrative importer only. No purchase, item custody or entry permission.
// Imports replace one complete revision; omitted native IDs are tombstoned and
// never recycled. Catalog/native estate IDs do NOT equal neighborhood/domain IDs.
class Service {
public:
 explicit Service(std::unique_ptr<Database> database,Settings settings={});
 ~Service();
 Result<Export> Import(const Bundle& bundle,uint64_t expected_revision);
 Result<Export> Load(const std::string& key);
private:
 std::unique_ptr<Database> database_;
 Settings settings_;
 std::mutex mutex_;
};
// Bounded canonical recovery from stored production wire blobs. Throws invalid_argument.
Bundle DecodePayloads(const std::vector<uint8_t>& definitions,const std::vector<uint8_t>& regions);
void Validate(const Bundle& bundle); // Pure graph/codec/provenance/capacity validation.
}
#endif
