#ifndef EQEMU_HOUSING_UPKEEP_REFRESH_WIRE_H
#define EQEMU_HOUSING_UPKEEP_REFRESH_WIRE_H
#include <cstddef>
#include <cstdint>
namespace Housing::UpkeepRefreshWire {
// Modern Servertalk Send pads empty application messages with one zero byte.
// Accept that canonical invalidation and historical empty messages only; no
// client identity, estate authority, money amount or mutation is carried here.
inline constexpr bool Valid(const uint8_t* bytes,size_t size) noexcept {
 return size==0||(size==1&&bytes&&bytes[0]==0);
}
}
#endif
