#ifndef EQEMU_HOUSING_PLOT_FIND_WIRE_H
#define EQEMU_HOUSING_PLOT_FIND_WIRE_H
#include "estate_wire.h"
#include <cmath>
namespace Housing::PlotFindWire {
inline constexpr size_t Size=40;
inline bool IsPlot(const uint8_t*p,size_t size){return p&&size==Size&&EstateWire::Detail::Read32(p)==4;}
struct Request {uint32_t estate_id=0;};
inline std::optional<Request> Decode(const uint8_t*p,size_t size){
 if(!IsPlot(p,size)||p[4]!=0xff||p[5]!=0xff)return {};
 // Native padding6/7 and secondary ID12 are preserved but grant no authority.
 const auto estate=EstateWire::Detail::Read32(p+8);if(!estate||estate==UINT32_MAX)return {};
 for(size_t at=16;at<Size;at+=4){const uint32_t bits=EstateWire::Detail::Read32(p+at);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value))return {};}
 return Request{estate};
}
}
#endif
