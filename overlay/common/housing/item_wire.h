#ifndef EQEMU_HOUSING_ITEM_WIRE_H
#define EQEMU_HOUSING_ITEM_WIRE_H

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace Housing::ItemWire {
// May 10 2013 reader 0x7f6bf0, tail 0x7f7bbb..0x7f7e67; wrapper 0x5557d4..0x555858.
// Starts at quaternary offset 96; excludes the following uint32 SubItemCount.
inline constexpr size_t EmptyTailSize = 72;
inline constexpr size_t MaximumFilename = 31;
inline constexpr size_t MaximumSockets = 10;
inline constexpr size_t MaximumPayload = 65535; // Server policy, not a native limit.
// Original ItemDefinition+0x5b4 / placedItem+0x9c, corroborated by
// 0x7b0b40 and 0x5ae010/0x5ae160. This class is distinct from estate masks
// and from the client placement mode (including Free). Decoding remains
// lossless for unknown values; recognizing an enum grants no placement authority.
enum class PlacementType : uint32_t { Floor = 0, Wall = 1, Ceiling = 2 };
inline constexpr bool KnownPlacementType(uint32_t value) {
	return value <= static_cast<uint32_t>(PlacementType::Ceiling);
}
struct Metadata {
	uint32_t placeable = 0;
	uint8_t ignore_collisions = 0;
	uint32_t placement_type = static_cast<uint32_t>(PlacementType::Floor);
	uint32_t definition_id = 0;
	float scale_min = 0;
	float scale_max = 0;
	uint32_t upkeep_id = 0;
	int32_t max_per_estate = 0;
	std::string housepet_filename;
	bool interactive = false;
	uint32_t trophy_benefit_id = 0xffffffff;
	bool disable_rotation = false;
	bool disable_free_placement = false;
	uint32_t respawn_interval = 0;
	float default_scale = 0;
	float default_heading = 0;
	float default_pitch = 0;
	float default_roll = 0;
	std::vector<uint32_t> socket_subclasses;
	bool collectible = false;
	bool no_destroy = false;
	bool marketplace = false;
	bool no_zone = false;
	uint32_t maker_id = 0;
	bool no_ground = false;
	std::vector<uint32_t> estate_ids;
	bool instance_placeable = true;
};
// Original7b1640/7fcb00: interactive services are not NPC-item count entries.
inline bool IsNpcItem(const Metadata& value){return !value.housepet_filename.empty()&&!value.interactive;}
namespace Detail {
inline void U32(std::vector<uint8_t> &out, uint32_t value) {
	for (size_t i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(value >> (8 * i)));
}
inline void Float(std::vector<uint8_t> &out, float value) {
	static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
	uint32_t bits; std::memcpy(&bits, &value, sizeof(bits)); U32(out, bits);
}
inline bool Finite(const Metadata &m) {
	return std::isfinite(m.scale_min) && std::isfinite(m.scale_max) &&
		std::isfinite(m.default_scale) && std::isfinite(m.default_heading) &&
		std::isfinite(m.default_pitch) && std::isfinite(m.default_roll);
}
struct Reader {
	const uint8_t *data;
	size_t size;
	size_t offset = 0;
	bool valid = true;
	uint8_t Byte() {
		if (offset >= size) { valid = false; return 0; }
		return data[offset++];
	}
	uint32_t U32() {
		if (size - offset < 4) { valid = false; return 0; }
		uint32_t value = 0;
		for (size_t i = 0; i < 4; ++i) value |= uint32_t(data[offset++]) << (8 * i);
		return value;
	}
	float Float() { uint32_t bits = U32(); float value; std::memcpy(&value, &bits, 4); return value; }
	bool Bool() { const auto value = Byte(); if (value > 1) valid = false; return value != 0; }
};
} // namespace Detail

inline std::vector<uint8_t> EncodeTail(const Metadata &m) {
	if (m.housepet_filename.size() > MaximumFilename || m.housepet_filename.find('\0') != std::string::npos ||
		m.socket_subclasses.size() > MaximumSockets || !Detail::Finite(m))
		throw std::invalid_argument("Invalid native housing item metadata");
	const size_t base = EmptyTailSize + m.housepet_filename.size() + 4 * m.socket_subclasses.size();
	if (m.estate_ids.size() > (MaximumPayload - base) / 4)
		throw std::invalid_argument("Housing item tail exceeds server bound");
	std::vector<uint8_t> out; out.reserve(base + 4 * m.estate_ids.size());
	Detail::U32(out, m.placeable); out.push_back(m.ignore_collisions);
	Detail::U32(out, m.placement_type); Detail::U32(out, m.definition_id);
	Detail::Float(out, m.scale_min); Detail::Float(out, m.scale_max);
	Detail::U32(out, m.upkeep_id); Detail::U32(out, static_cast<uint32_t>(m.max_per_estate));
	out.insert(out.end(), m.housepet_filename.begin(), m.housepet_filename.end()); out.push_back(0);
	out.push_back(m.interactive); Detail::U32(out, m.trophy_benefit_id);
	out.push_back(m.disable_rotation); out.push_back(m.disable_free_placement);
	Detail::U32(out, m.respawn_interval);
	Detail::Float(out, m.default_scale); Detail::Float(out, m.default_heading);
	Detail::Float(out, m.default_pitch); Detail::Float(out, m.default_roll);
	out.push_back(static_cast<uint8_t>(m.socket_subclasses.size()));
	for (auto id : m.socket_subclasses) Detail::U32(out, id);
	out.push_back(m.collectible); out.push_back(m.no_destroy); out.push_back(m.marketplace); out.push_back(m.no_zone);
	Detail::U32(out, m.maker_id); out.push_back(m.no_ground);
	Detail::U32(out, static_cast<uint32_t>(m.estate_ids.size()));
	for (auto id : m.estate_ids) Detail::U32(out, id);
	out.push_back(m.instance_placeable);
	return out;
}

// Diagnostic decoder requires exactly the tail bytes, excluding SubItemCount.
inline std::optional<Metadata> DecodeTail(const uint8_t *data, size_t size) {
	if (!data || size < EmptyTailSize || size > MaximumPayload) return std::nullopt;
	Detail::Reader r{data, size}; Metadata m;
	m.placeable = r.U32(); m.ignore_collisions = r.Byte(); m.placement_type = r.U32(); m.definition_id = r.U32();
	m.scale_min = r.Float(); m.scale_max = r.Float(); m.upkeep_id = r.U32();
	const uint32_t max_bits = r.U32(); std::memcpy(&m.max_per_estate, &max_bits, 4);
	const size_t start = r.offset;
	while (r.offset < size && data[r.offset] != 0 && r.offset - start <= MaximumFilename) ++r.offset;
	if (r.offset == size || r.offset - start > MaximumFilename) return std::nullopt;
	m.housepet_filename.assign(reinterpret_cast<const char *>(data + start), r.offset - start); ++r.offset;
	m.interactive = r.Bool(); m.trophy_benefit_id = r.U32(); m.disable_rotation = r.Bool(); m.disable_free_placement = r.Bool();
	m.respawn_interval = r.U32(); m.default_scale = r.Float(); m.default_heading = r.Float(); m.default_pitch = r.Float(); m.default_roll = r.Float();
	const auto sockets = r.Byte();
	if (!r.valid || sockets > MaximumSockets || sockets > (size - r.offset) / 4) return std::nullopt;
	for (size_t i = 0; i < sockets; ++i) m.socket_subclasses.push_back(r.U32());
	m.collectible = r.Bool(); m.no_destroy = r.Bool(); m.marketplace = r.Bool(); m.no_zone = r.Bool();
	m.maker_id = r.U32(); m.no_ground = r.Bool();
	const auto estates = r.U32();
	if (!r.valid || estates > (size - r.offset) / 4) return std::nullopt;
	for (size_t i = 0; i < estates; ++i) m.estate_ids.push_back(r.U32());
	m.instance_placeable = r.Bool();
	if (!r.valid || r.offset != size || !Detail::Finite(m)) return std::nullopt;
	return m;
}
} // namespace Housing::ItemWire
#endif
