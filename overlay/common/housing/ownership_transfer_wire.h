#ifndef EQEMU_HOUSING_OWNERSHIP_TRANSFER_WIRE_H
#define EQEMU_HOUSING_OWNERSHIP_TRANSFER_WIRE_H
#include "estate_wire.h"
#include "access_policy.h"
namespace Housing::OwnershipTransfer {
inline constexpr uint16_t Opcode=0x507f;
struct Wire {uint32_t native_plot_id=0;std::string target_name;std::array<uint8_t,EstateWire::PurchaseSize> source{};};
// Exact188-byte5b1470 constructor family; target name108 is NOT actor authority.
// Opaque guild singleton180 and copied padding187 are preserved, never trusted.
inline std::optional<Wire> Decode(const uint8_t*data,size_t size){
 if(!data||size!=EstateWire::PurchaseSize)return {};auto base=EstateWire::DecodePurchase(data,size);if(!base||!AccessPolicy::PositiveIdentity(base->estate_id)||base->sentinel_a!=UINT32_MAX||base->sentinel_b!=UINT32_MAX||base->price_component||base->upkeep_component)return {};
 for(size_t i=12;i<108;++i)if(data[i])return {};for(size_t i=184;i<187;++i)if(data[i])return {};
 size_t end=108;while(end<172&&data[end]){if(!((data[end]>='A'&&data[end]<='Z')||(data[end]>='a'&&data[end]<='z')))return {};++end;}
 if(end==108||end==172)return {};for(size_t i=end;i<172;++i)if(data[i])return {};
 Wire out;out.native_plot_id=base->estate_id;out.target_name.assign(reinterpret_cast<const char*>(data+108),end-108);std::copy(data,data+size,out.source.begin());return out;
}
}
#endif
