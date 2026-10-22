#include "runtime_activation_db.h"
#include "house_binding_service.h"
#include "house_relocation_preflight.h"
#include "rent_state_db.h"
#include "house_inventory_transition.h"
#include "custody_codec.h"
#include "inventory_process_recovery.h"
#include "catalog_service.h"
#include "../database.h"
#include "../inventory_profile.h"
#include <algorithm>
#include <limits>

namespace Housing::HouseBinding {
namespace {
using Row = LiveItems::InventoryRow;
using Bytes = CustodyCodec::Bytes;
using Lease = LiveItems::Lease;
using Action = LiveItems::Action;
[[noreturn]] void Reject(Error error) { throw Failure(error); }
MySQLRequestResult Q(Database& db, const std::string& sql) {
    auto result = db.QueryDatabase(sql, false);
    if (!result.Success()) Reject(Error::DatabaseFailure);
    return result;
}
std::string Hex(const Bytes& bytes) {
    static constexpr char alphabet[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (auto byte : bytes) { out += alphabet[byte >> 4]; out += alphabet[byte & 15]; }
    return out;
}
Bytes Unhex(const char* input) {
    if (!input) Reject(Error::DatabaseFailure);
    const std::string text(input);
    if (text.size() % 2 || text.size() > 600000) Reject(Error::DatabaseFailure);
    auto nibble = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        Reject(Error::DatabaseFailure);
    };
    Bytes out;
    for (size_t at = 0; at < text.size(); at += 2)
        out.push_back(uint8_t(nibble(text[at]) * 16 + nibble(text[at + 1])));
    return out;
}
std::string Literal(const std::string& text) {
    return "x'" + Hex(Bytes(text.begin(), text.end())) + "'";
}
bool Key(const std::string& key) { return !key.empty() && key.size() <= 96; }
bool Slot(uint32_t slot) { return (slot >= 23 && slot <= 32) || slot == 33; }
void Transaction(Database& db) {
    auto state = Q(db, "SELECT @@in_transaction");
    if (state.RowCount() != 1 || !state.begin()[0] || std::string(state.begin()[0]) != "1")
        Reject(Error::NoTransaction);
}
void Schema(Database& db) {
    auto result = Q(db, "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND engine='InnoDB' AND table_name IN ('housing_house_instances','housing_house_locations',"
        "'housing_house_interiors','housing_house_receipt_effects','housing_live_item_leases',"
        "'housing_live_item_receipts','housing_live_item_custody','housing_inventory_sessions',"
        "'inventory','character_data','account','items','housing_estates','housing_native_estates',"
        "'housing_catalogs','instance_list','items_evolving_details','character_evolving_items')");
    if (result.RowCount() != 1 || std::stoul(result.begin()[0]) != 18) Reject(Error::DatabaseFailure);
}
uint64_t Number(const char* value, uint64_t max) {
    if (!value || !*value || *value == '-') Reject(Error::DatabaseFailure);
    size_t end = 0;
    auto out = std::stoull(value, &end);
    if (end != std::string(value).size() || out > max) Reject(Error::DatabaseFailure);
    return out;
}
std::string LockLease(Database& db, const Lease& lease, LiveItems::InventorySessionFence& fence) {
    Transaction(db);
    if (!lease.actor.character_id || !lease.actor.account_id || !lease.epoch || !Key(lease.session_key))
        Reject(Error::InvalidArgument);
    if (!fence.HeldFor(lease.actor, lease.epoch, lease.session_key)) Reject(Error::UnheldFence);
    const auto character = std::to_string(lease.actor.character_id);
    auto actor = Q(db, "SELECT account_id,deleted_at FROM character_data WHERE id=" + character + " FOR UPDATE");
    if (actor.RowCount() != 1 || Number(actor.begin()[0], UINT32_MAX) != lease.actor.account_id
        || actor.begin()[1]) Reject(Error::Unauthorized);
    auto account = Q(db, "SELECT id FROM account WHERE id=" + std::to_string(lease.actor.account_id) + " FOR UPDATE");
    if (account.RowCount() != 1) Reject(Error::Unauthorized);
    if (!InventoryRecovery::LockCustodyReservation(db, lease.actor)) Reject(Error::UnheldFence);
    const auto ordinary_pending = HouseInventory::InspectPending(db, lease.actor.character_id, true);
    if (ordinary_pending != HouseInventory::PendingState::Clear)
        Reject(ordinary_pending == HouseInventory::PendingState::Pending ? Error::PendingApplication : Error::DatabaseFailure);
    auto session = Q(db, "SELECT account_id,active,epoch,HEX(token),HEX(process_key),HEX(client_key) FROM housing_inventory_sessions WHERE character_id=" + character + " FOR UPDATE");
    if (session.RowCount() != 1 || Number(session.begin()[0], UINT32_MAX) != lease.actor.account_id
        || Number(session.begin()[1], 1) != 1) Reject(Error::UnheldFence);
    auto current = Q(db, "SELECT account_id,epoch,HEX(session_key),HEX(token),active,HEX(pending_key) "
        "FROM housing_live_item_leases WHERE character_id=" + character + " FOR UPDATE");
    if (current.RowCount() != 1) Reject(Error::UnheldFence);
    auto row = current.begin();
    if (Number(row[0], UINT32_MAX) != lease.actor.account_id || Number(row[1], UINT64_MAX) != lease.epoch
        || Unhex(row[2]) != Bytes(lease.session_key.begin(), lease.session_key.end())
        || Unhex(row[3]) != Bytes(lease.token.begin(), lease.token.end()) || Number(row[4], 1) != 1)
        Reject(Error::UnheldFence);
    auto optional = Q(db, "SELECT COUNT(*),SUM(engine='InnoDB') FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND table_name IN ('housing_inventory_processes','housing_inventory_custody_links','housing_inventory_death_reconciliations')");
    const auto tables = Number(optional.begin()[0], 3);
    if (tables) {
        if (tables != 3 || Number(optional.begin()[1], 3) != 3) Reject(Error::DatabaseFailure);
        auto owner = session.begin();
        auto process_bytes = Unhex(owner[4]);
        auto process = Q(db, "SELECT reaped FROM housing_inventory_processes WHERE process_key=x'" + Hex(process_bytes) + "' FOR UPDATE");
        auto links = Q(db, "SELECT account_id,inventory_epoch,HEX(inventory_token),HEX(process_key),HEX(client_key),"
            "HEX(custody_session),HEX(custody_token) FROM housing_inventory_custody_links WHERE character_id=" + character
            + " AND custody_epoch=" + std::to_string(lease.epoch) + " FOR UPDATE");
        if (process.RowCount()) {
            if (process.RowCount() != 1 || Number(process.begin()[0], 1) != 0 || links.RowCount() != 1) Reject(Error::UnheldFence);
            auto link = links.begin();
            if (Number(link[0], UINT32_MAX) != lease.actor.account_id || Number(link[1], UINT64_MAX) != Number(owner[2], UINT64_MAX)
                || Unhex(link[2]) != Unhex(owner[3]) || Unhex(link[3]) != process_bytes || Unhex(link[4]) != Unhex(owner[5])
                || Unhex(link[5]) != Bytes(lease.session_key.begin(), lease.session_key.end())
                || Unhex(link[6]) != Bytes(lease.token.begin(), lease.token.end())) Reject(Error::UnheldFence);
        } else if (links.RowCount()) Reject(Error::UnheldFence);
        // Unregistered processes remain unsupervised/unrecoverable, matching
        // core OpenLease. A real held fence is still mandatory in that path.
    }
    auto bytes = row[5] ? Unhex(row[5]) : Bytes{};
    return std::string(bytes.begin(), bytes.end());
}
// Return SQL parent flags after checking immutable receipt session ownership.
std::pair<bool, bool> Parent(Database& db, const Lease& lease, const std::string& key) {
    if (!Key(key)) Reject(Error::InvalidArgument);
    auto result = Q(db, "SELECT character_id,account_id,lease_epoch,HEX(session_key),completed,applied,"
        "after_blob IS NOT NULL FROM housing_live_item_receipts WHERE request_key=" + Literal(key) + " FOR UPDATE");
    if (result.RowCount() != 1) Reject(Error::NotFound);
    auto row = result.begin();
    if (Number(row[0], UINT32_MAX) != lease.actor.character_id || Number(row[1], UINT32_MAX) != lease.actor.account_id
        || Number(row[2], UINT64_MAX) != lease.epoch
        || Unhex(row[3]) != Bytes(lease.session_key.begin(), lease.session_key.end())) Reject(Error::Unauthorized);
    const bool complete = Number(row[4], 1), applied = Number(row[5], 1);
    if (complete && !Number(row[6], 1)) Reject(Error::DatabaseFailure);
    return {complete, applied};
}
void NewParent(Database& db, const Lease& lease, const std::string& key,
    LiveItems::InventorySessionFence& fence) {
    if (!LockLease(db, lease, fence).empty()) Reject(Error::PendingApplication);
    auto state = Parent(db, lease, key);
    if (state.first || state.second) Reject(Error::Conflict);
    auto prior = Q(db, "SELECT request_key FROM housing_house_receipt_effects WHERE request_key=" + Literal(key) + " FOR UPDATE");
    if (prior.RowCount()) Reject(Error::Conflict);
}
const std::string InventoryColumns = "character_id,slot_id,item_id,charges,color,augment_one,augment_two,"
    "augment_three,augment_four,augment_five,augment_six,instnodrop,HEX(custom_data),ornament_icon,"
    "ornament_idfile,ornament_hero_model,guid";
Row InventoryRow(MySQLRequestRow row) {
    Row out;
    auto number = [&](size_t column, uint64_t max) { return row[column] ? Number(row[column], max) : 0; };
    auto& v = out.values;
    v.character_id = number(0, UINT32_MAX); v.slot_id = number(1, UINT32_MAX);
    v.item_id = number(2, UINT32_MAX); v.charges = number(3, UINT16_MAX); v.color = number(4, UINT32_MAX);
    v.augment_one = number(5, UINT32_MAX); v.augment_two = number(6, UINT32_MAX);
    v.augment_three = number(7, UINT32_MAX); v.augment_four = number(8, UINT32_MAX);
    v.augment_five = number(9, UINT32_MAX); v.augment_six = number(10, UINT32_MAX);
    v.instnodrop = number(11, UINT8_MAX);
    if (row[12]) { auto text = Unhex(row[12]); v.custom_data.assign(text.begin(), text.end()); }
    v.ornament_icon = number(13, UINT32_MAX); v.ornament_idfile = number(14, UINT32_MAX);
    if (!row[15]) Reject(Error::DatabaseFailure);
    size_t end = 0; auto hero = std::stoll(row[15], &end);
    if (end != std::string(row[15]).size() || hero < INT32_MIN || hero > INT32_MAX) Reject(Error::DatabaseFailure);
    v.ornament_hero_model = int32_t(hero); v.guid = number(16, UINT64_MAX);
    if (!row[2]) out.null_fields |= LiveItems::NullItemId;
    if (!row[3]) out.null_fields |= LiveItems::NullCharges;
    if (!row[12]) out.null_fields |= LiveItems::NullCustomData;
    if (!row[16]) out.null_fields |= LiveItems::NullGuid;
    // Non-null fields must never silently become zero on malformed legacy rows.
    for (auto i : {0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14}) if (!row[i]) Reject(Error::DatabaseFailure);
    if (!CustodyCodec::Valid(out)) Reject(Error::UnsupportedItem);
    return out;
}
std::string InventoryWhere(const Row& row) {
    return "character_id=" + std::to_string(row.values.character_id) + " AND slot_id=" + std::to_string(row.values.slot_id);
}
void InventoryExact(Database& db, const Row& expected, bool absent = false) {
    auto result = Q(db, "SELECT " + InventoryColumns + " FROM inventory WHERE " + InventoryWhere(expected) + " FOR UPDATE");
    if (absent) { if (result.RowCount()) Reject(Error::Conflict); return; }
    if (result.RowCount() != 1 || CustodyCodec::Encode(InventoryRow(result.begin())) != CustodyCodec::Encode(expected))
        Reject(Error::Conflict);
}
void Simple(Database& db, const Row& row) {
    const auto& v = row.values;
    if (!CustodyCodec::Valid(row) || !v.character_id || !Slot(v.slot_id) || !v.item_id
        || v.augment_one || v.augment_two || v.augment_three || v.augment_four || v.augment_five || v.augment_six)
        Reject(Error::UnsupportedItem);
    auto catalog = Q(db, "SELECT itemclass,bagslots,stackable FROM items WHERE id=" + std::to_string(v.item_id) + " FOR UPDATE");
    if (catalog.RowCount() != 1 || Number(catalog.begin()[0], UINT32_MAX)
        || Number(catalog.begin()[1], UINT32_MAX) || Number(catalog.begin()[2], UINT32_MAX)) Reject(Error::UnsupportedItem);
    auto evolving = Q(db, "SELECT id FROM items_evolving_details WHERE item_id=" + std::to_string(v.item_id) + " FOR UPDATE");
    auto actor_evolving = Q(db, "SELECT id FROM character_evolving_items WHERE character_id=" + std::to_string(v.character_id)
        + " AND item_id=" + std::to_string(v.item_id) + " AND deleted_at IS NULL FOR UPDATE");
    if (evolving.RowCount() || actor_evolving.RowCount()) Reject(Error::UnsupportedItem);
}
void NoChildren(Database& db, const Row& row) {
    auto first = EQ::InventoryProfile::CalcSlotId(int16_t(row.values.slot_id), EQ::invbag::SLOT_BEGIN);
    auto last = EQ::InventoryProfile::CalcSlotId(int16_t(row.values.slot_id), EQ::invbag::SLOT_END);
    if (first < 0 || last < first) Reject(Error::UnsupportedItem);
    auto result = Q(db, "SELECT slot_id FROM inventory WHERE character_id=" + std::to_string(row.values.character_id)
        + " AND slot_id BETWEEN " + std::to_string(first) + " AND " + std::to_string(last) + " FOR UPDATE");
    if (result.RowCount()) Reject(Error::UnsupportedItem);
}
void PersonalEstate(Database& db, uint32_t estate, uint32_t actor) {
    auto result = Q(db, "SELECT owner_kind,owner_id FROM housing_current_estates WHERE id=" + std::to_string(estate) + " FOR UPDATE");
    if (result.RowCount() != 1 || Number(result.begin()[0], UINT8_MAX) != uint8_t(OwnerKind::Character)
        || Number(result.begin()[1], UINT32_MAX) != actor) Reject(Error::Unauthorized);
}
EstateId CustodyExact(Database& db, uint64_t custody, const Row& expected, uint32_t actor) {
    auto result = Q(db, "SELECT estate_id,owner_character_id,HEX(row_blob),HEX(schema_descriptor) "
        "FROM housing_live_item_custody WHERE custody_id=" + std::to_string(custody) + " FOR UPDATE");
    if (result.RowCount() != 1) Reject(Error::NotFound);
    auto row = result.begin();
    if (Number(row[1], UINT32_MAX) != actor || Unhex(row[2]) != CustodyCodec::Encode(expected)) Reject(Error::Conflict);
    auto descriptor = LiveItems::Service::SchemaDescriptor();
    if (Unhex(row[3]) != Bytes(descriptor.begin(), descriptor.end())) Reject(Error::DatabaseFailure);
    const auto estate = Number(row[0], UINT32_MAX);
    if (!estate) Reject(Error::Conflict);
    return estate;
}
bool Same(const Location& a, const Location& b) {
    return a.kind == b.kind && a.character_id == b.character_id && a.slot_id == b.slot_id && a.custody_id == b.custody_id;
}
bool Same(const Interior& a, const Interior& b) {
    return a.native_estate_id == b.native_estate_id && a.definition_id == b.definition_id && a.zone_id == b.zone_id
        && a.instance_id == b.instance_id && a.zone_version == b.zone_version && a.catalog_key == b.catalog_key && a.catalog_revision == b.catalog_revision;
}
Binding Read(Database& db, const std::string& condition) {
    auto result = Q(db, "SELECT h.house_id,h.revision,h.item_template_id,HEX(h.pending_request_key),"
        "l.location_kind,l.character_id,l.slot_id,l.custody_id FROM housing_house_instances h "
        "JOIN housing_house_locations l ON l.house_id=h.house_id WHERE " + condition + " FOR UPDATE");
    if (result.RowCount() != 1) Reject(result.RowCount() ? Error::DatabaseFailure : Error::NotFound);
    auto row = result.begin(); Binding out;
    out.house_id = Number(row[0], UINT64_MAX); out.revision = Number(row[1], UINT64_MAX);
    out.item_template_id = Number(row[2], UINT32_MAX);
    auto key = row[3] ? Unhex(row[3]) : Bytes{}; out.pending_key.assign(key.begin(), key.end());
    out.location.kind = Location::Kind(Number(row[4], 2));
    out.location.character_id = row[5] ? Number(row[5], UINT32_MAX) : 0;
    out.location.slot_id = row[6] ? Number(row[6], UINT32_MAX) : 0;
    out.location.custody_id = row[7] ? Number(row[7], UINT64_MAX) : 0;
    if (!out.house_id || !out.revision || !out.item_template_id) Reject(Error::DatabaseFailure);
    if (out.location.kind == Location::Kind::Inventory) {
        if (!out.location.character_id || !row[6] || out.location.custody_id) Reject(Error::DatabaseFailure);
    } else if (out.location.kind == Location::Kind::Custody) {
        if (!out.location.custody_id || row[5] || row[6]) Reject(Error::DatabaseFailure);
    } else Reject(Error::DatabaseFailure);
    auto interior = Q(db, "SELECT native_estate_id,definition_id,zone_id,instance_id,zone_version,HEX(catalog_key),catalog_revision "
        "FROM housing_house_interiors WHERE house_id=" + std::to_string(out.house_id) + " FOR UPDATE");
    if (interior.RowCount() > 1) Reject(Error::DatabaseFailure);
    if (interior.RowCount()) {
        auto i = interior.begin(); Interior value;
        value.native_estate_id = Number(i[0], UINT32_MAX); value.definition_id = Number(i[1], UINT32_MAX);
        value.zone_id = Number(i[2], UINT32_MAX); value.instance_id = Number(i[3], 32000);
        value.zone_version = Number(i[4], UINT8_MAX); auto text = Unhex(i[5]); value.catalog_key.assign(text.begin(), text.end());
        value.catalog_revision = Number(i[6], UINT64_MAX); out.interior = value;
    }
    return out;
}
void Expected(const Binding& actual, const Binding& expected) {
    if (actual.house_id != expected.house_id || actual.revision != expected.revision
        || actual.item_template_id != expected.item_template_id || !Same(actual.location, expected.location)
        || actual.interior.has_value() != expected.interior.has_value()
        || (actual.interior && !Same(*actual.interior, *expected.interior))) Reject(Error::Conflict);
}
EstateWire::Definition InteriorFacts(Database& db, const Interior& interior) {
    if (!interior.native_estate_id || interior.native_estate_id == UINT32_MAX || !interior.definition_id
        || !interior.catalog_revision || !interior.zone_id || !interior.instance_id || interior.instance_id > 32000 || !Key(interior.catalog_key))
        Reject(Error::InvalidArgument);
    auto native = Q(db, "SELECT definition_id,HEX(catalog_key),active FROM housing_native_estates WHERE native_id="
        + std::to_string(interior.native_estate_id) + " FOR UPDATE");
    if (native.RowCount() != 1 || Number(native.begin()[0], UINT32_MAX) != interior.definition_id
        || Unhex(native.begin()[1]) != Bytes(interior.catalog_key.begin(), interior.catalog_key.end())
        || Number(native.begin()[2], 1) != 1) Reject(Error::Conflict);
    auto instance = Q(db, "SELECT zone,version,never_expires FROM instance_list WHERE id=" + std::to_string(interior.instance_id) + " FOR UPDATE");
    if (instance.RowCount() != 1 || Number(instance.begin()[0], UINT32_MAX) != interior.zone_id
        || Number(instance.begin()[1], UINT8_MAX) != interior.zone_version || Number(instance.begin()[2], 1) != 1)
        Reject(Error::Conflict);
    auto catalog = Q(db, "SELECT revision,origin,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs "
        "WHERE catalog_key=" + Literal(interior.catalog_key) + " FOR UPDATE");
    if (catalog.RowCount() != 1 || Number(catalog.begin()[0], UINT64_MAX) != interior.catalog_revision
        || Number(catalog.begin()[2], UINT32_MAX) != 20130510) Reject(Error::Conflict);
    const auto origin = Number(catalog.begin()[1], 2);
    if (origin == 2) {
        auto database = Q(db, "SELECT DATABASE()");
        const std::string name = database.begin()[0] ? database.begin()[0] : "";
        if (!Housing::RuntimeActivation::AllowsReconstructedContent(db)) Reject(Error::Unauthorized);
    } else if (origin != 1) Reject(Error::Conflict);
    Catalog::Bundle decoded;
    try { decoded = Catalog::DecodePayloads(Unhex(catalog.begin()[3]), Unhex(catalog.begin()[4])); }
    catch (...) { Reject(Error::DatabaseFailure); }
    const EstateWire::Definition* definition = nullptr;
    for (const auto& value : decoded.definitions) if (value.id == interior.definition_id) {
        if (definition) Reject(Error::DatabaseFailure);
        definition = &value;
    }
    if (!definition || (definition->type != 2 && definition->type != 3) || definition->zone_id != interior.zone_id)
        Reject(Error::Conflict);
    return *definition;
}
void InteriorValid(Database& db,const Interior& interior,const InteriorRoutePolicy* policy,uint32_t item_template_id) {
    const auto definition=InteriorFacts(db,interior);
    if(!policy||!policy->VerifiedDestination(interior,definition,item_template_id))Reject(Error::UnsupportedItem);
    // Route approval remains mandatory for new allocation. Dead-owner validation
    // checks committed routing facts, without granting new placement/entry rights.
}
void PutLocation(Database& db, HouseId house, const Location& location) {
    const auto sql = "UPDATE housing_house_locations SET location_kind=" + std::to_string(uint8_t(location.kind))
        + ",character_id=" + (location.kind == Location::Kind::Inventory ? std::to_string(location.character_id) : "NULL")
        + ",slot_id=" + (location.kind == Location::Kind::Inventory ? std::to_string(location.slot_id) : "NULL")
        + ",custody_id=" + (location.kind == Location::Kind::Custody ? std::to_string(location.custody_id) : "NULL")
        + " WHERE house_id=" + std::to_string(house);
    if (Q(db, sql).RowsAffected() != 1) Reject(Error::Conflict);
}
void Blob(Bytes& bytes, const Bytes& value) { CustodyCodec::W(bytes, value.size(), 4); bytes.insert(bytes.end(), value.begin(), value.end()); }
void EncodeBinding(Bytes& bytes, const Binding& value) {
    CustodyCodec::W(bytes, value.house_id, 8); CustodyCodec::W(bytes, value.revision, 8);
    CustodyCodec::W(bytes, value.item_template_id, 4); CustodyCodec::W(bytes, uint8_t(value.location.kind), 1);
    CustodyCodec::W(bytes, value.location.character_id, 4); CustodyCodec::W(bytes, value.location.slot_id, 4);
    CustodyCodec::W(bytes, value.location.custody_id, 8); CustodyCodec::W(bytes, value.interior ? 1 : 0, 1);
    if (value.interior) {
        auto& i = *value.interior;
        CustodyCodec::W(bytes, i.native_estate_id, 4); CustodyCodec::W(bytes, i.definition_id, 4);
        CustodyCodec::W(bytes, i.zone_id, 4); CustodyCodec::W(bytes, i.instance_id, 2);
        CustodyCodec::W(bytes, i.zone_version, 1); CustodyCodec::S(bytes, i.catalog_key); CustodyCodec::W(bytes, i.catalog_revision, 8);
    }
}
Binding DecodeBinding(CustodyCodec::Reader& reader) {
    Binding out;
    out.house_id = reader.N(8); out.revision = reader.N(8); out.item_template_id = reader.N(4);
    out.location.kind = Location::Kind(reader.N(1)); out.location.character_id = reader.N(4);
    out.location.slot_id = reader.N(4); out.location.custody_id = reader.N(8);
    const auto present = reader.N(1); if (present > 1) Reject(Error::DatabaseFailure);
    if (present) {
        Interior i; i.native_estate_id = reader.N(4); i.definition_id = reader.N(4);
        i.zone_id = reader.N(4); i.instance_id = reader.N(2); i.zone_version = reader.N(1); i.catalog_key = reader.Text(96); i.catalog_revision = reader.N(8);
        out.interior = i;
    }
    return out;
}
Bytes EncodeEffect(const Effect& effect) {
    Bytes out; CustodyCodec::W(out, effect.encoding_version, 4); CustodyCodec::S(out, effect.request_key);
    CustodyCodec::W(out, uint8_t(effect.action), 1); CustodyCodec::W(out, effect.minted ? 1 : 0, 1);
    EncodeBinding(out, effect.before); EncodeBinding(out, effect.after);
    Blob(out, CustodyCodec::Encode(effect.source)); Blob(out, CustodyCodec::Encode(effect.destination));
    return out;
}
Effect DecodeEffect(const Bytes& bytes) {
    try {
        CustodyCodec::Reader reader{bytes}; Effect out;
        out.encoding_version = reader.N(4);
        if (out.encoding_version != 1 && out.encoding_version != 2) Reject(Error::DatabaseFailure);
        out.request_key = reader.Text(96); out.action = Action(reader.N(1));
        auto minted = reader.N(1); if (minted > 1) Reject(Error::DatabaseFailure); out.minted = minted;
        out.before = DecodeBinding(reader); out.after = DecodeBinding(reader);
        out.source = CustodyCodec::Decode(reader.Blob(CustodyCodec::MaximumPayload)); out.destination = CustodyCodec::Decode(reader.Blob(CustodyCodec::MaximumPayload));
        if (reader.at != bytes.size() || EncodeEffect(out) != bytes || !Key(out.request_key)
            || (out.action != Action::Store && out.action != Action::Return) || !out.before.house_id
            || out.before.house_id != out.after.house_id || out.before.revision == UINT64_MAX
            || out.after.revision != out.before.revision + 1 || out.before.item_template_id != out.source.values.item_id
            || out.after.item_template_id != out.before.item_template_id) Reject(Error::DatabaseFailure);
        if (!out.before.revision || !Slot(out.source.values.slot_id) || !Slot(out.destination.values.slot_id)
            || !out.source.values.character_id || out.before.location.custody_id == UINT64_MAX)
            Reject(Error::DatabaseFailure);
        if (!ValidInteriorTransition(out))
            Reject(Error::DatabaseFailure);
        if (out.action == Action::Return && out.before.interior.has_value() != out.after.interior.has_value())
            Reject(Error::DatabaseFailure);
        if (out.action == Action::Store) {
            if (out.before.location.character_id != out.source.values.character_id
                || out.before.location.slot_id != out.source.values.slot_id || out.before.location.custody_id
                || out.after.location.character_id || out.after.location.slot_id
                || out.before.location.kind != Location::Kind::Inventory || out.after.location.kind != Location::Kind::Custody
                || !out.after.location.custody_id || CustodyCodec::Encode(out.source) != CustodyCodec::Encode(out.destination))
                Reject(Error::DatabaseFailure);
        } else {
            auto expected = out.source; expected.values.slot_id = out.destination.values.slot_id;
            if (out.before.location.character_id || out.before.location.slot_id || !out.before.location.custody_id
                || out.after.location.character_id != out.destination.values.character_id
                || out.after.location.slot_id != out.destination.values.slot_id || out.after.location.custody_id
                || out.before.location.kind != Location::Kind::Custody || out.after.location.kind != Location::Kind::Inventory
                || out.minted || CustodyCodec::Encode(expected) != CustodyCodec::Encode(out.destination)) Reject(Error::DatabaseFailure);
        }
        return out;
    } catch (const Failure&) { throw; } catch (...) { Reject(Error::DatabaseFailure); }
}
void Persist(Database& db, Effect& effect) {
    auto bytes = EncodeEffect(effect);
    auto inserted = Q(db, "INSERT INTO housing_house_receipt_effects(request_key,house_id,effects_version,effects_blob) VALUES ("
        + Literal(effect.request_key) + "," + std::to_string(effect.after.house_id) + "," + std::to_string(effect.encoding_version) + ",x'" + Hex(bytes) + "')");
    if (inserted.RowsAffected() != 1) Reject(Error::Conflict);
    if (effect.before.revision == UINT64_MAX) Reject(Error::Conflict);
    PutLocation(db, effect.after.house_id, effect.after.location);
    auto changed = Q(db, "UPDATE housing_house_instances SET revision=revision+1,pending_request_key=" + Literal(effect.request_key)
        + " WHERE house_id=" + std::to_string(effect.after.house_id) + " AND revision=" + std::to_string(effect.before.revision)
        + " AND pending_request_key IS NULL");
    if (changed.RowsAffected() != 1) Reject(Error::Conflict);
    effect.after.pending_key = effect.request_key;
}
}

Effect ValidateRestartOutcome(Database& db,const Lease& lease,const LiveItems::Receipt& parent){return ValidateRestartOutcome(db,lease,parent,nullptr);}
Effect ValidateRestartOutcome(Database& db,const Lease& lease,const LiveItems::Receipt& parent,const std::string* aggregate_pending) {
    Transaction(db);Schema(db);
    const auto state=Parent(db,lease,parent.request_key);
    if(!state.first||state.second||!parent.requires_application||lease.pending_key!=parent.request_key)Reject(Error::PendingApplication);
    const auto expected_pending=aggregate_pending?*aggregate_pending:parent.request_key;
    auto current_lease=Q(db,"SELECT account_id,epoch,HEX(session_key),HEX(token),active,HEX(pending_key) FROM housing_live_item_leases WHERE character_id="+std::to_string(lease.actor.character_id)+" FOR UPDATE");
    if(current_lease.RowCount()!=1)Reject(Error::UnheldFence);
    auto owner=current_lease.begin();
    if(Number(owner[0],UINT32_MAX)!=lease.actor.account_id||Number(owner[1],UINT64_MAX)!=lease.epoch||Unhex(owner[2])!=Bytes(lease.session_key.begin(),lease.session_key.end())||Unhex(owner[3])!=Bytes(lease.token.begin(),lease.token.end())||Number(owner[4],1)!=1||(!aggregate_pending?Unhex(owner[5])!=Bytes(parent.request_key.begin(),parent.request_key.end()):(owner[5]&&Unhex(owner[5])!=Bytes(expected_pending.begin(),expected_pending.end()))))Reject(Error::UnheldFence);
    auto rows=Q(db,"SELECT house_id,effects_version,HEX(effects_blob) FROM housing_house_receipt_effects WHERE request_key="+Literal(parent.request_key)+" FOR UPDATE");
    if(rows.RowCount()!=1)Reject(Error::NotFound);
    auto row=rows.begin();
    auto effect=DecodeEffect(Unhex(row[2]));
    if(Number(row[1],UINT32_MAX)!=effect.encoding_version)Reject(Error::DatabaseFailure);
    if(effect.request_key!=parent.request_key||effect.action!=parent.action||effect.after.house_id!=Number(row[0],UINT64_MAX)||effect.source.values.character_id!=lease.actor.character_id||CustodyCodec::Encode(effect.source)!=CustodyCodec::Encode(parent.stored.row)||(effect.action==Action::Store?effect.after.location.custody_id:effect.before.location.custody_id)!=parent.stored.custody_id||(effect.action==Action::Return&&CustodyCodec::Encode(effect.destination)!=CustodyCodec::Encode(parent.inventory_after)))Reject(Error::Conflict);
    auto actual=Read(db,"h.house_id="+std::to_string(effect.after.house_id));Expected(actual,effect.after);
    if(actual.pending_key!=expected_pending)Reject(Error::PendingApplication);
    if(actual.interior){
        InteriorFacts(db,*actual.interior);
        auto metadata=Q(db,"SELECT UNK227 FROM items WHERE id="+std::to_string(actual.item_template_id)+" FOR UPDATE");
        if(metadata.RowCount()!=1||Number(metadata.begin()[0],UINT32_MAX)!=actual.interior->definition_id)Reject(Error::Conflict);
    }
    if(effect.action==Action::Store){CustodyExact(db,effect.after.location.custody_id,effect.destination,lease.actor.character_id);InventoryExact(db,effect.source,true);}
    else {InventoryExact(db,effect.destination);if(Q(db,"SELECT custody_id FROM housing_live_item_custody WHERE custody_id="+std::to_string(effect.before.location.custody_id)+" FOR UPDATE").RowCount())Reject(Error::Conflict);}
    auto pending_houses=Q(db,"SELECT house_id FROM housing_house_instances WHERE pending_request_key="+Literal(expected_pending)+(aggregate_pending?" AND house_id="+std::to_string(effect.after.house_id):std::string{})+" FOR UPDATE");
    if(pending_houses.RowCount()!=1||Number(pending_houses.begin()[0],UINT64_MAX)!=effect.after.house_id)Reject(Error::Conflict);
    effect.replayed=true;effect.requires_application=true;return effect;
}

TransactionHelpers::TransactionHelpers(Database& db, LiveItems::InventorySessionFence& fence,
    const CustodyAuthorization* authorization) : db_(db), fence_(fence), authorization_(authorization) {
    Transaction(db_); Schema(db_);
}
void TransactionHelpers::Authorize(const AuthorizationRequest& request) {
    if (authorization_) {
        if (!authorization_->AllowedUnderTransaction(db_, fence_, request)) Reject(Error::Unauthorized);
    } else {
        // Preserve the existing strict personal-owner policy until a reviewed
        // house-specific integration is explicitly supplied. No automatic guild
        // grant, item-creator impersonation or mapping-only bypass is added.
        PersonalEstate(db_, request.estate_id, request.lease.actor.character_id);
    }
}
Prepared TransactionHelpers::PrepareStore(const Lease& lease, const std::string& key, const Row& source,
    int32_t serial, HouseId house, uint64_t revision, const HouseTemplatePolicy& policy) {
    NewParent(db_, lease, key, fence_);
    if (source.values.character_id != lease.actor.character_id || serial <= 0 || (!house != !revision)) Reject(Error::InvalidArgument);
    Simple(db_, source);
    if (!policy.VerifiedHouse(source)) Reject(Error::UnsupportedItem);
    if (!fence_.SourceMatches(source, serial)) Reject(Error::UnheldFence);
    InventoryExact(db_, source); NoChildren(db_, source);
    Prepared out; out.lease_ = lease; out.key_ = key; out.source_ = source; out.runtime_serial_ = serial;
    if (!house) {
        if (!policy.PermitInitialBinding(lease.actor, source)) Reject(Error::UnsupportedItem);
        auto prior = Q(db_, "SELECT house_id FROM housing_house_locations WHERE character_id=" + std::to_string(source.values.character_id)
            + " AND slot_id=" + std::to_string(source.values.slot_id) + " FOR UPDATE");
        if (prior.RowCount()) Reject(Error::Conflict);
        auto minted = Q(db_, "INSERT INTO housing_house_instances(item_template_id,created_by_character_id) VALUES ("
            + std::to_string(source.values.item_id) + "," + std::to_string(lease.actor.character_id) + ")");
        if (minted.RowsAffected() != 1 || !minted.LastInsertedID()) Reject(Error::DatabaseFailure);
        house = minted.LastInsertedID();
        Q(db_, "INSERT INTO housing_house_locations(house_id,location_kind,character_id,slot_id) VALUES ("
            + std::to_string(house) + ",1," + std::to_string(source.values.character_id) + "," + std::to_string(source.values.slot_id) + ")");
        out.minted_ = true; revision = 1;
    }
    out.before_ = Read(db_, "h.house_id=" + std::to_string(house));
    const Location expected{Location::Kind::Inventory, source.values.character_id, source.values.slot_id, 0};
    if (!out.before_.pending_key.empty()) Reject(Error::PendingApplication);
    if (out.before_.revision != revision || out.before_.item_template_id != source.values.item_id
        || !Same(out.before_.location, expected)) Reject(Error::Conflict);
    return out;
}
Prepared TransactionHelpers::PrepareReturn(const Lease& lease, const std::string& key, HouseId house,
    uint64_t revision, uint64_t custody, const Row& source, uint32_t destination_slot) {
    NewParent(db_, lease, key, fence_);
    if (!house || !revision || !custody || !Slot(destination_slot) || source.values.character_id != lease.actor.character_id) Reject(Error::InvalidArgument);
    Simple(db_, source); auto estate = CustodyExact(db_, custody, source, lease.actor.character_id);
    if(!RentState::MutableItem(db_,estate,0,custody))Reject(Error::Unauthorized);
    auto destination = source; destination.values.slot_id = destination_slot;
    if (!fence_.ReturnAllowed(destination)) Reject(Error::UnheldFence);
    InventoryExact(db_, destination, true); NoChildren(db_, destination);
    Prepared out; out.lease_ = lease; out.key_ = key; out.source_ = source; out.action_ = Action::Return; out.destination_slot_ = destination_slot;
    out.before_ = Read(db_, "h.house_id=" + std::to_string(house));
    if (!out.before_.pending_key.empty()) Reject(Error::PendingApplication);
    if (out.before_.revision != revision || out.before_.item_template_id != source.values.item_id
        || !Same(out.before_.location, Location{Location::Kind::Custody, 0, 0, custody})) Reject(Error::Conflict);
    Authorize({AuthorizationPhase::ReturnBeforeInsert, lease, key, out.before_, source,
        custody, estate, destination_slot, 0, std::nullopt});
    const auto quiet=HouseRelocationPreflight::UnderTransaction(db_,out.before_,lease,fence_);
    if(quiet!=LiveItems::Error::None)Reject(quiet==LiveItems::Error::PendingApplication?Error::PendingApplication:quiet==LiveItems::Error::DatabaseFailure?Error::DatabaseFailure:Error::Conflict);
    return out;
}
Effect TransactionHelpers::FinishStore(const Prepared& prepared, uint64_t custody, const std::optional<Interior>& first, const InteriorRoutePolicy* policy) {
    if (prepared.action_ != Action::Store || !custody) Reject(Error::InvalidArgument);
    NewParent(db_, prepared.lease_, prepared.key_, fence_);
    auto current = Read(db_, "h.house_id=" + std::to_string(prepared.before_.house_id));
    Expected(current, prepared.before_);
    if (!current.pending_key.empty()) Reject(Error::PendingApplication);
    InventoryExact(db_, prepared.source_, true);
    auto estate = CustodyExact(db_, custody, prepared.source_, prepared.lease_.actor.character_id);
    Authorize({AuthorizationPhase::StoreAfterCustodyInsert, prepared.lease_, prepared.key_, prepared.before_,
        prepared.source_, custody, estate, 0, prepared.runtime_serial_, first});
    Effect out{prepared.before_, prepared.before_, Action::Store, prepared.source_, prepared.source_, prepared.key_, prepared.minted_};
    out.after.location = {Location::Kind::Custody, 0, 0, custody}; ++out.after.revision;
    if (first) {
        InteriorValid(db_, *first, policy, prepared.source_.values.item_id);
        if (out.before.interior) {
            if (!SamePhysicalInterior(*out.before.interior, *first)) Reject(Error::Conflict);
            if (!Same(*out.before.interior, *first)) {
                const auto& old = *out.before.interior;
                auto updated = Q(db_, "UPDATE housing_house_interiors SET catalog_key=" + Literal(first->catalog_key)
                    + ",catalog_revision=" + std::to_string(first->catalog_revision)
                    + " WHERE house_id=" + std::to_string(out.before.house_id)
                    + " AND catalog_key=" + Literal(old.catalog_key) + " AND catalog_revision=" + std::to_string(old.catalog_revision)
                    + " AND native_estate_id=" + std::to_string(old.native_estate_id)
                    + " AND instance_id=" + std::to_string(old.instance_id));
                if (updated.RowsAffected() != 1) Reject(Error::Conflict);
                out.after.interior = first;
                out.encoding_version = 2;
            }
        }
        else {
            auto& i = *first;
            auto inserted = Q(db_, "INSERT INTO housing_house_interiors(house_id,native_estate_id,instance_id,catalog_key,definition_id,zone_id,zone_version,catalog_revision) VALUES ("
                + std::to_string(out.after.house_id) + "," + std::to_string(i.native_estate_id) + "," + std::to_string(i.instance_id)
                + "," + Literal(i.catalog_key) + "," + std::to_string(i.definition_id) + "," + std::to_string(i.zone_id)
                + "," + std::to_string(i.zone_version) + "," + std::to_string(i.catalog_revision) + ")");
            if (inserted.RowsAffected() != 1) Reject(Error::Conflict);
            out.after.interior = first;
        }
    }
    Persist(db_, out); return out;
}
Effect TransactionHelpers::FinishReturn(const Prepared& prepared, const Row& destination) {
    if (prepared.action_ != Action::Return || !Slot(destination.values.slot_id) || destination.values.slot_id != prepared.destination_slot_) Reject(Error::InvalidArgument);
    NewParent(db_, prepared.lease_, prepared.key_, fence_);
    auto expected = prepared.source_; expected.values.slot_id = destination.values.slot_id;
    if (CustodyCodec::Encode(expected) != CustodyCodec::Encode(destination)) Reject(Error::Conflict);
    if (!fence_.ReturnAllowed(destination)) Reject(Error::UnheldFence);
    auto current = Read(db_, "h.house_id=" + std::to_string(prepared.before_.house_id));
    Expected(current, prepared.before_);
    if (!current.pending_key.empty()) Reject(Error::PendingApplication);
    auto estate = CustodyExact(db_, prepared.before_.location.custody_id, prepared.source_, prepared.lease_.actor.character_id);
    InventoryExact(db_, destination); NoChildren(db_, destination);
    Authorize({AuthorizationPhase::ReturnAfterInsert, prepared.lease_, prepared.key_, prepared.before_,
        prepared.source_, prepared.before_.location.custody_id, estate, destination.values.slot_id, 0, std::nullopt});
    Effect out{prepared.before_, prepared.before_, Action::Return, prepared.source_, destination, prepared.key_};
    out.after.location = {Location::Kind::Inventory, destination.values.character_id, destination.values.slot_id, 0};
    ++out.after.revision; Persist(db_, out); return out;
}
Effect TransactionHelpers::Resolve(const Lease& lease, const LiveItems::Receipt& parent) {
    auto pending = LockLease(db_, lease, fence_);
    auto state = Parent(db_, lease, parent.request_key);
    if (!state.first) Reject(Error::PendingApplication);
    auto result = Q(db_, "SELECT house_id,effects_version,HEX(effects_blob) FROM housing_house_receipt_effects WHERE request_key="
        + Literal(parent.request_key) + " FOR UPDATE");
    if (result.RowCount() != 1) Reject(Error::NotFound);
    auto row = result.begin();
    auto out = DecodeEffect(Unhex(row[2]));
    if (Number(row[1], UINT32_MAX) != out.encoding_version) Reject(Error::DatabaseFailure);
    if (out.request_key != parent.request_key || out.action != parent.action || out.after.house_id != Number(row[0], UINT64_MAX)
        || CustodyCodec::Encode(out.source) != CustodyCodec::Encode(parent.stored.row)
        || (out.action == Action::Store ? out.after.location.custody_id : out.before.location.custody_id) != parent.stored.custody_id
        || (out.action == Action::Return && CustodyCodec::Encode(out.destination) != CustodyCodec::Encode(parent.inventory_after)))
        Reject(Error::Conflict);
    out.replayed = true; out.requires_application = !state.second;
    if (state.second) return out; // Immutable historical outcome; NEVER apply it.
    if (pending != parent.request_key) Reject(Error::PendingApplication);
    auto current = Read(db_, "h.house_id=" + std::to_string(out.after.house_id));
    Expected(current, out.after);
    if (current.pending_key != parent.request_key) Reject(Error::PendingApplication);
    if (out.action == Action::Store) {
        CustodyExact(db_, out.after.location.custody_id, out.destination, lease.actor.character_id);
        InventoryExact(db_, out.source, true);
    } else {
        InventoryExact(db_, out.destination);
        auto old = Q(db_, "SELECT custody_id FROM housing_live_item_custody WHERE custody_id=" + std::to_string(out.before.location.custody_id) + " FOR UPDATE");
        if (old.RowCount()) Reject(Error::Conflict);
    }
    return out;
}
void TransactionHelpers::ClearAfterParentAcknowledgement(const Lease& lease, const LiveItems::Receipt& parent, HouseApplicationProof& proof) {
    auto pending = LockLease(db_, lease, fence_);
    auto state = Parent(db_, lease, parent.request_key);
    if (!state.first || !state.second || !pending.empty()) Reject(Error::PendingApplication);
    auto effect = Resolve(lease, parent); // historical branch skips postimage check; check it below.
    auto current = Read(db_, "h.house_id=" + std::to_string(effect.after.house_id));
    if (current.pending_key.empty()) return; // Prior ACK is idempotent even after later operations.
    if (current.pending_key != parent.request_key) Reject(Error::PendingApplication);
    Expected(current, effect.after);
    if (!fence_.Applied(parent) || !proof.AppliedHouseEffect(effect)) Reject(Error::UnheldFence);
    if (effect.action == Action::Store) CustodyExact(db_, effect.after.location.custody_id, effect.destination, lease.actor.character_id);
    else InventoryExact(db_, effect.destination);
    auto cleared = Q(db_, "UPDATE housing_house_instances SET pending_request_key=NULL WHERE house_id="
        + std::to_string(current.house_id) + " AND pending_request_key=" + Literal(parent.request_key));
    if (cleared.RowsAffected() != 1) Reject(Error::Conflict);
}
Binding TransactionHelpers::ReadInventory(const Lease& lease, uint32_t slot) {
    if (!Slot(slot)) Reject(Error::InvalidArgument);
    auto pending = LockLease(db_, lease, fence_);
    if (!pending.empty()) Reject(Error::PendingApplication);
    auto result = Read(db_, "l.character_id=" + std::to_string(lease.actor.character_id) + " AND l.slot_id=" + std::to_string(slot));
    if (!result.pending_key.empty()) Reject(Error::PendingApplication);
    auto row = Q(db_, "SELECT " + InventoryColumns + " FROM inventory WHERE character_id=" + std::to_string(lease.actor.character_id)
        + " AND slot_id=" + std::to_string(slot) + " FOR UPDATE");
    if (row.RowCount() != 1 || InventoryRow(row.begin()).values.item_id != result.item_template_id) Reject(Error::Conflict);
    return result;
}
Binding TransactionHelpers::ReadCustody(const Lease& lease, uint64_t custody) {
    if (!custody) Reject(Error::InvalidArgument);
    auto pending = LockLease(db_, lease, fence_);
    if (!pending.empty()) Reject(Error::PendingApplication);
    auto result = Read(db_, "l.custody_id=" + std::to_string(custody));
    if (!result.pending_key.empty()) Reject(Error::PendingApplication);
    auto row = Q(db_, "SELECT HEX(row_blob) FROM housing_live_item_custody WHERE custody_id=" + std::to_string(custody) + " FOR UPDATE");
    if (row.RowCount() != 1) Reject(Error::NotFound);
    Row source;
    try { source = CustodyCodec::Decode(Unhex(row.begin()[0])); } catch (...) { Reject(Error::DatabaseFailure); }
    if (source.values.item_id != result.item_template_id || source.values.character_id != lease.actor.character_id) Reject(Error::Unauthorized);
    auto estate = CustodyExact(db_, custody, source, lease.actor.character_id);
    Authorize({AuthorizationPhase::ReadStored, lease, {}, result, source, custody, estate, 0, 0, std::nullopt});
    return result;
}
}
