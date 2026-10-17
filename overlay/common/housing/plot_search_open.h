#ifndef EQEMU_HOUSING_PLOT_SEARCH_OPEN_H
#define EQEMU_HOUSING_PLOT_SEARCH_OPEN_H
#include <array>
#include <cstdint>
namespace Housing::PlotSearchOpen {
// Original S2C2b35/4ca8bf consumes exactly one mode byte. It opens the
// native PlotSearch window, then enables selected-row796c inspection.
inline constexpr std::array<uint8_t,1> InspectMode(){return {1};}
}
#endif
