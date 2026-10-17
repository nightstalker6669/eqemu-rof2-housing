#ifndef EQEMU_HOUSING_SALE_WIRE_H
#define EQEMU_HOUSING_SALE_WIRE_H
#include "estate_wire.h"
namespace Housing::SaleWire {
// Original confirmation100 callback9f91fc->74c8d0. Confirmation101 and507f
// transfer ownership are a DIFFERENT operation and remain unsupported.
inline constexpr uint16_t Opcode=0x792c,TransferOpcode=0x507f;
inline constexpr size_t Size=188;
struct Request{uint32_t estate_id=0;uint8_t padding=0;};
inline std::optional<Request> Decode(const uint8_t* bytes,size_t size){
 if(!bytes||size!=Size)return {};
 const auto id=NeighborhoodWire::Detail::Read32(bytes);
 if(!id||id==UINT32_MAX||NeighborhoodWire::Detail::Read32(bytes+4)!=UINT32_MAX||NeighborhoodWire::Detail::Read32(bytes+8)!=UINT32_MAX)return {};
 // The sale callback only fillsestateID after the shared default constructor.
 // Byte187 is alignment padding not initialized by that constructor.
 for(size_t i=12;i<187;++i)if(bytes[i])return {};
 return Request{id,bytes[187]};
}
}
#endif
