#ifndef EQEMU_HOUSING_PET_COMMANDS_H
#define EQEMU_HOUSING_PET_COMMANDS_H
#include "interaction_wire.h"
namespace Housing::PetCommands {
// Original action9 / 51e3. The separate menu parameter is not a universal
// text-ID formula. These five values were observed on an owned Live spider;
// the same command labels are present in the original RoF2 category28 data.
enum class Command:uint32_t {Stay=16175,Walk=16176,Run=16177,Follow=16178,PlayDead=16179};
struct Request {uint32_t estate_id,item_id;Command command;};
static_assert(sizeof(Request)==12,"Housing custom interaction body is 12 bytes");
inline std::optional<Command> DecodeCommand(uint32_t value){switch(value){case 16175:case 16176:case 16177:case 16178:case 16179:return Command(value);default:return {};}}
inline std::optional<Request> Decode(const uint8_t* p,size_t n){
 if(!p||n!=12)return {};auto a=NeighborhoodWire::Detail::Read32(p),b=NeighborhoodWire::Detail::Read32(p+4);auto c=DecodeCommand(NeighborhoodWire::Detail::Read32(p+8));
 if(!InteractionWire::Identity(a)||!InteractionWire::Identity(b)||!c)return {};return Request{a,b,*c};
}
}
#endif
