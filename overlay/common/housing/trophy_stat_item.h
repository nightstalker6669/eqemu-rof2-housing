#ifndef EQEMU_HOUSING_TROPHY_STAT_ITEM_H
#define EQEMU_HOUSING_TROPHY_STAT_ITEM_H
#include "../eq_packet_structs.h"
namespace Housing::TrophyStatItem {
// Internal-only tags within ItemPacketType's defined 0..255 range. RoF2 emits the original ItemPacket type108 and embeds
// the native trophy container/index in the serialized item location.
inline constexpr ItemPacketType PersonalPacket=static_cast<ItemPacketType>(0xf0);
inline constexpr ItemPacketType GuildPacket=static_cast<ItemPacketType>(0xf1);
}
#endif
