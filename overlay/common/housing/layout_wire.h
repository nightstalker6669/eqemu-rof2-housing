#ifndef EQEMU_HOUSING_LAYOUT_WIRE_H
#define EQEMU_HOUSING_LAYOUT_WIRE_H
#include "neighborhood_wire.h"
#include "placement_wire.h"
#include "placement_policy.h"
#include <array>
#include <cmath>
#include <cstring>
#include <optional>
#include <variant>
namespace Housing::LayoutWire {
// Original May10 2013 sender73bec0; serializers733370/738de0. Evidence:
// housing/protocol/LAYOUT-LOAD-PROTOCOL-1014.md. Decode supplies no authority.
inline constexpr uint16_t LoadOpcode=0x5b41;
struct ItemOperation {
 uint32_t source_estate_id=0,native_item_id=0,quantity=0,target_estate_id=0;
 // 733370 writes a quantity, not an action enum. 7388ce supplies remaining
 // target stack capacity; other producer paths pass zero. Zero's execution
 // semantics must be resolved from its source/target context, not guessed here.
};
struct PlacementOperation {
 std::string guid;
 uint8_t source_kind=0;
 std::array<uint16_t,3> source_slots{}; // Compact ItemPtr location; ffff is -1.
 uint32_t source_estate_id=0,native_item_id=0;
 std::array<float,7> heading_pitch_roll_scale_x_y_z{};
 uint32_t target_estate_id=0;
 // Already transformed by the client. Server placement position requires Y/X/Z.
};
using Operation=std::variant<ItemOperation,PlacementOperation>;
struct Load {uint32_t yard_estate_id=0,house_estate_id=0;std::vector<Operation> operations;};
// Item-window reset734e89..734eb5 initializes absent yard/house/crate IDs
// to ffffffff. Keep the exact header on wire/journal; use zero only for the
// server's optional attached-house context. Zero remains accepted internally.
inline uint32_t ContextHouseEstateId(const Load& load){return load.house_estate_id==UINT32_MAX?0:load.house_estate_id;}
// Explicit adapters only: neither grants possession, custody, geometry or ACL
// authority. Preserve compact source sentinels and already-transformed poses.
inline PlacementWire::Source PlacementSource(const PlacementOperation& op){
 if(op.guid.size()!=16||op.guid.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-")!=std::string::npos)
  throw std::invalid_argument("Invalid layout source GUID");
 PlacementWire::Source source;
 std::copy(op.guid.begin(),op.guid.end(),source.guid.begin());
 source.location={op.source_kind==0xff?UINT32_MAX:uint32_t(op.source_kind),
  uint32_t(op.source_slots[0])|(uint32_t(op.source_slots[1])<<16),
  uint32_t(op.source_slots[2])};
 source.estate_id=op.source_estate_id;source.item_id=op.native_item_id;
 return source;
}
inline PlacementPolicy::Transform PlacementTransform(const PlacementOperation& op){
 const auto& p=op.heading_pitch_roll_scale_x_y_z;
 return {p[0],p[1],p[2],p[3],{p[5],p[4],p[6]}};
}
inline std::optional<Load> DecodeLoad(const uint8_t* data,size_t size){
 if(!data||size<10||size>NeighborhoodWire::MaximumPayload)return {};
 size_t at=0;
 auto number=[&](size_t width)->uint32_t{
  if(width>size-at)throw std::invalid_argument("Truncated housing layout");
  uint32_t value=0;for(size_t n=0;n<width;++n)value|=uint32_t(data[at++])<<(8*n);return value;
 };
 try{
  Load out;out.yard_estate_id=number(4);out.house_estate_id=number(4);const auto count=number(2);
  // Smallest observed record is tag0+four DWORDs. Bound allocation before reserve.
  if(count>(size-at)/17)return {};
  out.operations.reserve(count);
  for(uint32_t i=0;i<count;++i){
   const auto tag=number(1);
   if(tag==0){ItemOperation op;op.source_estate_id=number(4);op.native_item_id=number(4);op.quantity=number(4);op.target_estate_id=number(4);out.operations.emplace_back(op);}
   else if(tag==1){
    PlacementOperation op;
    // EqItemGuid accepts exactly16 characters from this original alphabet.
    if(size-at<17)return {};
    op.guid.assign(reinterpret_cast<const char*>(data+at),16);at+=16;
    if(number(1)!=0||op.guid.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-")!=std::string::npos)return {};
    op.source_kind=uint8_t(number(1));for(auto& slot:op.source_slots)slot=uint16_t(number(2));
    op.source_estate_id=number(4);op.native_item_id=number(4);
    for(float& value:op.heading_pitch_roll_scale_x_y_z){const uint32_t bits=number(4);std::memcpy(&value,&bits,4);if(!std::isfinite(value))return {};}
    if(!(op.heading_pitch_roll_scale_x_y_z[3]>0))return {};
    op.target_estate_id=number(4);out.operations.emplace_back(std::move(op));
   }else return {}; // Unknown tags must not be guessed or silently skipped.
  }
  if(at!=size)return {};
  return out;
 }catch(const std::invalid_argument&){return {};}
}
}
#endif
