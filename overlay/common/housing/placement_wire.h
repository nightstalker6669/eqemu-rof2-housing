#ifndef EQEMU_HOUSING_PLACEMENT_WIRE_H
#define EQEMU_HOUSING_PLACEMENT_WIRE_H
#include "neighborhood_wire.h"
#include <cmath>
#include <algorithm>
namespace Housing::PlacementWire {
// Exact May10 2013 constructors5424d0/5b3fb0 and callback5b1f90.
// These request codecs preserve opaque source data; they authorize no custody.
inline constexpr uint16_t BeginOpcode=0x626e,CommitOpcode=0x711a;
struct Source {
 std::array<uint8_t,18> guid{};
 std::array<uint8_t,2> padding{};
 std::array<uint32_t,3> location{};
 uint32_t estate_id{},item_id{},opaque{};
};
// Top-level possession framing. Equipment, bank and bag indices are not
// promoted to inventory authority here; the live fence validates possession
// support, exact row and runtime item independently.
inline std::optional<uint32_t> InventorySlot(const Source& s){
 const auto slot=uint16_t(s.location[1]);
 if(s.location[0]||uint16_t(s.location[1]>>16)!=0xffff||uint16_t(s.location[2])!=0xffff||!((slot>=23&&slot<=32)||slot==33))return {};
 return slot;
}
inline bool InventorySlotMatches(const Source& s,uint32_t slot){const auto actual=InventorySlot(s);return actual&&*actual==slot;}
struct Begin {
 Source source;
 uint32_t action{1};uint8_t inventory_source{};
 std::array<uint8_t,3> padding{};std::array<uint32_t,2> trailing{};
};
struct Commit {
 Source source;
 std::array<float,7> transform{0,0,0,1,0,0,0}; // heading,pitch,roll,scale,position3; axis names unresolved.
 std::array<float,3> vector_a{},vector_b{}; // Native collision-box extents/center; caller validates authoritative geometry.
};
namespace Detail {
inline void SourceBytes(std::vector<uint8_t>& b,const Source& s){b.insert(b.end(),s.guid.begin(),s.guid.end());b.insert(b.end(),s.padding.begin(),s.padding.end());for(auto v:s.location)NeighborhoodWire::Detail::Write32(b,v);for(auto v:{s.estate_id,s.item_id,s.opaque})NeighborhoodWire::Detail::Write32(b,v);}
inline Source ReadSource(const uint8_t*p){Source s;std::copy(p,p+18,s.guid.begin());std::copy(p+18,p+20,s.padding.begin());for(size_t i=0;i<3;++i)s.location[i]=NeighborhoodWire::Detail::Read32(p+20+4*i);s.estate_id=NeighborhoodWire::Detail::Read32(p+32);s.item_id=NeighborhoodWire::Detail::Read32(p+36);s.opaque=NeighborhoodWire::Detail::Read32(p+40);return s;}
inline void Float(std::vector<uint8_t>&b,float v){if(!std::isfinite(v))throw std::invalid_argument("Nonfinite placement transform");uint32_t bits;std::memcpy(&bits,&v,4);NeighborhoodWire::Detail::Write32(b,bits);}
}
inline std::vector<uint8_t> EncodeBegin(const Begin& m){if(m.inventory_source>1)throw std::invalid_argument("Unknown placement source flag");std::vector<uint8_t>b;b.reserve(60);Detail::SourceBytes(b,m.source);NeighborhoodWire::Detail::Write32(b,m.action);b.push_back(m.inventory_source);b.insert(b.end(),m.padding.begin(),m.padding.end());for(auto v:m.trailing)NeighborhoodWire::Detail::Write32(b,v);return b;}
inline std::optional<Begin> DecodeBegin(const uint8_t*p,size_t n){if(!p||n!=60||p[48]>1)return std::nullopt;Begin m;m.source=Detail::ReadSource(p);m.action=NeighborhoodWire::Detail::Read32(p+44);m.inventory_source=p[48];std::copy(p+49,p+52,m.padding.begin());for(size_t i=0;i<2;++i)m.trailing[i]=NeighborhoodWire::Detail::Read32(p+52+4*i);return m;}
inline std::vector<uint8_t> EncodeCommit(const Commit& m){if(!(m.transform[3]>0))throw std::invalid_argument("Invalid placement scale");std::vector<uint8_t>b;b.reserve(96);Detail::SourceBytes(b,m.source);for(float v:m.transform)Detail::Float(b,v);for(float v:m.vector_a)Detail::Float(b,v);for(float v:m.vector_b)Detail::Float(b,v);return b;}
inline std::optional<Commit> DecodeCommit(const uint8_t*p,size_t n){if(!p||n!=96)return std::nullopt;Commit m;m.source=Detail::ReadSource(p);size_t at=44;for(auto* a:{&m.transform[0],&m.vector_a[0],&m.vector_b[0]}){size_t count=a==&m.transform[0]?7:3;for(size_t i=0;i<count;++i){uint32_t bits=NeighborhoodWire::Detail::Read32(p+at);at+=4;std::memcpy(a+i,&bits,4);if(!std::isfinite(a[i]))return std::nullopt;}}if(!(m.transform[3]>0))return std::nullopt;return m;}
// Native505400 world record is scale-first; it is NOT the711a tuple.
struct Actor {
 std::string model,guid;uint8_t is_npc{};float npc_height{};uint32_t record_num{},estate_id{},item_id{};
 std::array<float,7> scale_heading_pitch_roll_y_x_z{1,0,0,0,0,0,0};
 uint8_t ignore_collisions{},disable_rotation{},disable_free{};uint32_t placement_type{};
 std::array<float,6> min_max_default_scale_heading_pitch_roll{1,1,1,0,0,0};uint32_t light_type{};
};
struct Preview {
 Actor actor;uint8_t source_kind{};std::array<int16_t,3> source_slots{33,-1,-1};
 std::array<uint8_t,3> flags{};std::array<uint32_t,3> words{};
 std::vector<std::pair<uint32_t,uint32_t>> region_results; // KEY(regionID),RESULT(0 permits).
 std::pair<uint32_t,uint32_t> fallback{}; // KEY(zone estate),RESULT. Not permission authority.
};
inline std::vector<uint8_t> EncodeActor(const Actor& a){
 if(a.model.empty()||a.model.size()>63||a.model.find('\0')!=std::string::npos||a.guid.size()!=16||a.guid.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-")!=std::string::npos)
  throw std::invalid_argument("Invalid placement model/GUID");
 if(!(a.scale_heading_pitch_roll_y_x_z[0]>0)||!(a.min_max_default_scale_heading_pitch_roll[0]>0)||a.min_max_default_scale_heading_pitch_roll[0]>a.min_max_default_scale_heading_pitch_roll[1]||a.min_max_default_scale_heading_pitch_roll[2]<a.min_max_default_scale_heading_pitch_roll[0]||a.min_max_default_scale_heading_pitch_roll[2]>a.min_max_default_scale_heading_pitch_roll[1])
  throw std::invalid_argument("Invalid actor scale bounds");
 std::vector<uint8_t>b;b.insert(b.end(),a.model.begin(),a.model.end());b.push_back(0);b.push_back(a.is_npc);Detail::Float(b,a.npc_height);NeighborhoodWire::Detail::Write32(b,a.record_num);b.insert(b.end(),a.guid.begin(),a.guid.end());b.push_back(0);
 NeighborhoodWire::Detail::Write32(b,a.estate_id);NeighborhoodWire::Detail::Write32(b,a.item_id);for(float v:a.scale_heading_pitch_roll_y_x_z)Detail::Float(b,v);
 for(auto v:{a.ignore_collisions,a.disable_rotation,a.disable_free})b.push_back(v);NeighborhoodWire::Detail::Write32(b,a.placement_type);
 for(float v:a.min_max_default_scale_heading_pitch_roll)Detail::Float(b,v);NeighborhoodWire::Detail::Write32(b,a.light_type);return b;
}
inline std::vector<uint8_t> EncodePreview(const Preview& p){
 auto b=EncodeActor(p.actor);constexpr size_t suffix=34;
 if(p.region_results.size()>(NeighborhoodWire::MaximumPayload-b.size()-suffix)/8)throw std::invalid_argument("Placement region map too large");
 b.push_back(p.source_kind);for(int16_t v:p.source_slots){b.push_back(uint8_t(uint16_t(v)));b.push_back(uint8_t(uint16_t(v)>>8));}
 for(auto v:p.flags)b.push_back(v);for(auto v:p.words)NeighborhoodWire::Detail::Write32(b,v);NeighborhoodWire::Detail::Write32(b,uint32_t(p.region_results.size()));
 for(const auto& kv:p.region_results){NeighborhoodWire::Detail::Write32(b,kv.first);NeighborhoodWire::Detail::Write32(b,kv.second);}
 NeighborhoodWire::Detail::Write32(b,p.fallback.first);NeighborhoodWire::Detail::Write32(b,p.fallback.second);return b;
}
}
#endif
