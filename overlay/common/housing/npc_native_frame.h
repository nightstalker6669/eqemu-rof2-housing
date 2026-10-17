#ifndef EQEMU_HOUSING_NPC_NATIVE_FRAME_H
#define EQEMU_HOUSING_NPC_NATIVE_FRAME_H
#include "npc_native_height.h"
namespace Housing::NpcNativeFrame {
struct Frame {uint32_t height_bits=0,origin_z_bits=0;};
inline bool Same(const Frame& a,const Frame& b){return a.height_bits==b.height_bits&&a.origin_z_bits==b.origin_z_bits;}
inline std::optional<uint32_t> Adjacent(uint32_t bits,bool up){
 if(!(bits&0x7fffffff))return up?uint32_t(1):uint32_t(0x80000001);
 uint32_t next=bits+((bool(bits>>31)==up)?uint32_t(-1):uint32_t(1));
 if(((next>>23)&255)==255)return {};return next;
}
inline bool Valid(const NpcNativeHeight::Variants& allowed,float ground,const Frame& frame){
 if(!NpcNativeHeight::Decode(frame.height_bits)||!NpcNativeHeight::Contains(allowed,NpcNativeHeight::Float(frame.height_bits)))return false;
 auto replay=NpcNativeHeight::Subtract(frame.origin_z_bits,frame.height_bits);return replay&&*replay==NpcNativeHeight::Bits(ground);
}
inline std::optional<Frame> FromSubmitted(const NpcNativeHeight::Variants& allowed,float ground,float actual_height){
 if(!NpcNativeHeight::Contains(allowed,actual_height))return {};const auto h=NpcNativeHeight::Bits(actual_height);if(!NpcNativeHeight::Decode(h))return {};
 auto origin=NpcNativeHeight::Add(NpcNativeHeight::Bits(ground),h);if(!origin)return {};
 // Packet supplies only rounded groundZ, not the sender origin's lost low
 // bits. Select a deterministic representable inverse, checking both neighbors
 // of the chopped exact sum; persist it rather than reinterpret on model edits.
 for(auto bits:{origin,Adjacent(*origin,true),Adjacent(*origin,false)})if(bits){Frame frame{h,*bits};if(Valid(allowed,ground,frame))return frame;}
 return {};
}
}
#endif
