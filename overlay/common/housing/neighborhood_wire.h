#ifndef EQEMU_HOUSING_NEIGHBORHOOD_WIRE_H
#define EQEMU_HOUSING_NEIGHBORHOOD_WIRE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Housing::NeighborhoodWire {

// Recovered for the May 10 2013 client, SHA256 2a8702ad...b405b0e.
// Manager 0x5b2500 -> reader 0x5b0f80 -> neighborhood UI 0x74ab50.
inline constexpr uint16_t SearchOpcode = 0x2691;
inline constexpr uint16_t GoOpcode = 0x218c;
inline constexpr uint8_t NeighborhoodType = 5;
inline constexpr size_t SearchSize = 224;
inline constexpr size_t GoSize = 12;
// Application policy, not a claim about the native protocol's maximum size.
inline constexpr size_t MaximumPayload = 65535;
inline constexpr size_t MaximumNeighborhoodName = 79;
inline constexpr size_t MaximumPlayerName = 63;
inline constexpr uint32_t NoGuild = 0xffffffff;

struct Row {
	uint32_t id = 0;
	std::string name;
	uint32_t guild_id = NoGuild;
	uint32_t player_plots = 0;
	uint32_t used_player_plots = 0;
	uint32_t guild_plots = 0;
	uint32_t used_guild_plots = 0;
};

struct SearchRequest {
	uint32_t mode = 0; // 0 search/My Plots; 1 My Items; 2 selected-row owners.
	uint32_t context_or_selected_id = 0;
	uint32_t reserved = 0;
	std::string neighborhood_name;
	std::string player_name;
	std::string guild_name;
	uint8_t flag220 = 0;
	uint8_t flag221 = 0;
	std::array<uint8_t, 2> padding{}; // Semantics unknown; preserve rather than infer.
};

struct GoRequest {
	uint32_t selected_id = 0;
	uint32_t sentinel = 0xffffffff;
	uint32_t reserved = 0;
};

namespace Detail {
inline uint32_t Read32(const uint8_t *p) {
	return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
		(uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline void Write16(std::vector<uint8_t> &out, uint16_t value) {
	out.push_back(static_cast<uint8_t>(value));
	out.push_back(static_cast<uint8_t>(value >> 8));
}
inline void Write32(std::vector<uint8_t> &out, uint32_t value) {
	for (size_t i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(value >> (8 * i)));
}
inline void AddSize(size_t &size, size_t increment) {
	if (increment > MaximumPayload - size) throw std::invalid_argument("Housing payload exceeds server bound");
	size += increment;
}
inline void ValidateName(const std::string &name, size_t maximum) {
	if (name.empty() || name.size() > maximum || name.find('\0') != std::string::npos)
		throw std::invalid_argument("Invalid housing name length or embedded NUL");
	// The recovered fixture is byte-oriented; only printable ASCII is validated.
	for (unsigned char c : name) if (c < 32 || c > 126)
		throw std::invalid_argument("Housing name requires printable ASCII");
}
inline void WriteName(std::vector<uint8_t> &out, const std::string &name) {
	Write32(out, static_cast<uint32_t>(name.size()));
	out.insert(out.end(), name.begin(), name.end()); // Native string reader excludes wire NUL.
}
inline void Header(std::vector<uint8_t> &out, uint32_t request, uint32_t subtype) {
	Write32(out, request); // Native reader skips this value.
	// Byte4 is estate type, not status: 0x7f6030 classifies PlayerPlot4/GuildPlot8.
	out.push_back(NeighborhoodType);
	Write32(out, subtype);
}
inline std::optional<std::string> FixedName(const uint8_t *p, size_t capacity) {
	const auto *end = static_cast<const uint8_t *>(std::memchr(p, 0, capacity));
	if (!end) return std::nullopt;
	return std::string(reinterpret_cast<const char *>(p), static_cast<size_t>(end - p));
}
} // namespace Detail

inline std::vector<uint8_t> EncodeList(const std::vector<Row> &rows, uint32_t context,
	bool show = true, uint32_t request = 0) {
	if (rows.size() > std::numeric_limits<uint16_t>::max())
		throw std::invalid_argument("Native housing row count exceeds uint16");
	size_t size = rows.empty() ? 12 : 16;
	for (const auto &row : rows) {
		Detail::ValidateName(row.name, MaximumNeighborhoodName);
		if (row.id == 0xffffffff || row.used_player_plots > row.player_plots ||
			row.used_guild_plots > row.guild_plots)
			throw std::invalid_argument("Invalid housing row identity or plot counts");
		Detail::AddSize(size, 28 + row.name.size());
	}
	std::vector<uint8_t> out;
	out.reserve(size);
	Detail::Header(out, request, 0);
	Detail::Write16(out, static_cast<uint16_t>(rows.size()));
	out.push_back(show ? 1 : 0);
	if (!rows.empty()) Detail::Write32(out, context);
	for (const auto &row : rows) {
		Detail::Write32(out, row.id);
		Detail::WriteName(out, row.name);
		Detail::Write32(out, row.guild_id);
		Detail::Write32(out, row.player_plots);
		Detail::Write32(out, row.used_player_plots);
		Detail::Write32(out, row.guild_plots);
		Detail::Write32(out, row.used_guild_plots);
	}
	return out;
}

inline std::vector<uint8_t> EncodeOwners(const std::vector<std::string> &players,
	const std::vector<uint32_t> &guild_ids, uint32_t request = 0) {
	if (players.size() > std::numeric_limits<uint16_t>::max() ||
		guild_ids.size() > std::numeric_limits<uint16_t>::max())
		throw std::invalid_argument("Native housing owner count exceeds uint16");
	size_t size = 13;
	for (const auto &name : players) {
		Detail::ValidateName(name, MaximumPlayerName);
		Detail::AddSize(size, 4 + name.size());
	}
	Detail::AddSize(size, 4 * guild_ids.size());
	std::vector<uint8_t> out;
	out.reserve(size);
	Detail::Header(out, request, 2);
	Detail::Write16(out, static_cast<uint16_t>(players.size()));
	for (const auto &name : players) Detail::WriteName(out, name);
	Detail::Write16(out, static_cast<uint16_t>(guild_ids.size()));
	for (uint32_t id : guild_ids) Detail::Write32(out, id);
	return out;
}

inline std::optional<SearchRequest> DecodeSearch(const uint8_t *buffer, size_t size) {
	if (!buffer || size != SearchSize) return std::nullopt;
	auto neighborhood = Detail::FixedName(buffer + 12, 80);
	auto player = Detail::FixedName(buffer + 92, 64);
	auto guild = Detail::FixedName(buffer + 156, 64);
	if (!neighborhood || !player || !guild) return std::nullopt;
	SearchRequest result;
	result.mode = Detail::Read32(buffer);
	if (result.mode > 2) return std::nullopt;
	result.context_or_selected_id = Detail::Read32(buffer + 4);
	result.reserved = Detail::Read32(buffer + 8);
	result.neighborhood_name = std::move(*neighborhood);
	result.player_name = std::move(*player);
	result.guild_name = std::move(*guild);
	result.flag220 = buffer[220];
	result.flag221 = buffer[221];
	result.padding = {buffer[222], buffer[223]};
	return result;
}

inline std::optional<GoRequest> DecodeGo(const uint8_t *buffer, size_t size) {
	if (!buffer || size != GoSize) return std::nullopt;
	GoRequest result{Detail::Read32(buffer), Detail::Read32(buffer + 4), Detail::Read32(buffer + 8)};
	// Native sender rejects -1 selection and always sends this exact suffix.
	if (result.selected_id == 0xffffffff || result.sentinel != 0xffffffff || result.reserved != 0)
		return std::nullopt;
	return result;
}
} // namespace Housing::NeighborhoodWire
#endif
