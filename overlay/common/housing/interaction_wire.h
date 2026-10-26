#ifndef EQEMU_HOUSING_INTERACTION_WIRE_H
#define EQEMU_HOUSING_INTERACTION_WIRE_H
#include "placement_wire.h"
namespace Housing::InteractionWire {
// Exact542a90 query constructor/7375f0 reader: fixed body768 bytes.
inline constexpr size_t MenuSize=768;
enum class QueryProfile { List, ActorRightClick };
struct Query { QueryProfile profile{QueryProfile::List};PlacementWire::Source source;std::array<uint32_t,2> mouse{};uint32_t opaque760{}; };
struct ActorQuery {uint32_t estate_id{},item_id{};};
inline bool Identity(uint32_t id){return id&&id!=UINT32_MAX;}
// Original735743 confirmation67: exact24-byte body. The prefix's roles are
// unknown; preserve the witnessed constants without treating them as slots.
// Both identity pairs are looked up again when the native popup is accepted.
struct HouseSwap {ActorQuery current,replacement;};
inline std::optional<HouseSwap> DecodeHouseSwap(const uint8_t* p,size_t size){
 if(!p||size!=24)return {};auto w=[&](size_t at){return NeighborhoodWire::Detail::Read32(p+at);};
 if(w(0)!=UINT32_MAX||w(4)!=0)return {};
 // This decoder covers only the evidenced estate-item source profile. No
 // carried-inventory identity convention has been recovered for this packet.
 if(!Identity(w(8))||!Identity(w(12))||!Identity(w(16))||!Identity(w(20)))return {};
 return HouseSwap{{w(8),w(12)},{w(16),w(20)}};
}
inline std::optional<ActorQuery> DecodeItemInspect(const uint8_t* p,size_t size){
 if(!p||size!=8)return {};const auto estate=NeighborhoodWire::Detail::Read32(p),item=NeighborhoodWire::Detail::Read32(p+4);
 if(!Identity(estate)||!Identity(item))return {};return ActorQuery{estate,item};
}
// Original218c house Go body: estate/item/zero. Neighborhood Go remains
// a distinct profile with itemFFFFFFFF; never accept that sentinel here.
inline std::optional<ActorQuery> DecodeHouseGo(const uint8_t* p,size_t size){
 if(!p||size!=12)return {};auto w=[&](size_t at){return NeighborhoodWire::Detail::Read32(p+at);};
 if(!Identity(w(0))||!Identity(w(4))||w(8))return {};return ActorQuery{w(0),w(4)};
}
// Native /leaverealestate writes only its opcode; all four body bytes are
// stale shared-buffer contents. They select no house or destination.
inline bool DecodeHouseExit(const uint8_t* p,size_t size){return p&&size==4;}
inline bool Guid(const std::array<uint8_t,18>& text){
 if(text[16]||text[17])return false;for(size_t i=0;i<16;++i){auto c=text[i];if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;}return true;
}
inline bool StoredSource(const PlacementWire::Source& source){return source.location[0]==UINT32_MAX&&source.location[1]==UINT32_MAX&&uint16_t(source.location[2])==0xffff;}
inline std::optional<ActorQuery> DecodeActorQuery(const uint8_t* p,size_t size){
 if(!p||size!=12)return {};auto w=[&](size_t at){return NeighborhoodWire::Detail::Read32(p+at);};
 if(!Identity(w(0))||!Identity(w(4))||w(8)!=UINT32_MAX)return {};return ActorQuery{w(0),w(4)};
}
inline std::optional<Query> DecodeQuery(const uint8_t* p,size_t size){
 if(!p||size!=MenuSize)return {};Query q;q.source=PlacementWire::Detail::ReadSource(p);
 if(!Guid(q.source.guid)||!StoredSource(q.source)||!Identity(q.source.estate_id)||!Identity(q.source.item_id)||q.source.opaque)return {};
 // Shared constructor initializes title/actions/opaque764; padding remains
 // non-authoritative. RMouseUp537f49 uses a distinct default-GUID profile.
 if(!std::all_of(p+44,p+108,[](uint8_t b){return b==0;})||!std::all_of(p+108,p+748,[](uint8_t b){return b==0xff;})||NeighborhoodWire::Detail::Read32(p+764))return {};
 if(p[748]==1)q.profile=QueryProfile::List;
 else if(p[748]==0&&NeighborhoodWire::Detail::Read32(p+760)==UINT32_MAX&&std::all_of(p,p+16,[](uint8_t b){return b=='0';}))q.profile=QueryProfile::ActorRightClick;
 else return {};
 q.mouse={NeighborhoodWire::Detail::Read32(p+752),NeighborhoodWire::Detail::Read32(p+756)};q.opaque760=NeighborhoodWire::Detail::Read32(p+760);return q;
}
inline std::vector<uint8_t> EncodeChairMenu(uint32_t estate,uint32_t item,const std::string& guid,const std::string& title,std::array<uint32_t,2> mouse,bool move,bool pickup,bool enter=false,bool inspect=false,bool swap=false,bool pet=false){
 PlacementWire::Source source;source.location={UINT32_MAX,UINT32_MAX,UINT32_MAX};source.estate_id=estate;source.item_id=item;
 if(guid.size()!=16||!Identity(estate)||!Identity(item)||title.size()>63||title.find('\0')!=std::string::npos)throw std::invalid_argument("Invalid chair menu identity/title");
 std::copy(guid.begin(),guid.end(),source.guid.begin());if(!Guid(source.guid))throw std::invalid_argument("Invalid chair menu GUID");
 for(unsigned char c:title)if(c<32||c>126)throw std::invalid_argument("Invalid chair menu title");
 std::vector<uint8_t> bytes;PlacementWire::Detail::SourceBytes(bytes,source);bytes.resize(MenuSize,0);std::copy(title.begin(),title.end(),bytes.begin()+44);std::fill(bytes.begin()+108,bytes.begin()+748,0xff);
 auto put=[&](size_t at,uint32_t word){for(size_t i=0;i<4;++i)bytes[at+i]=uint8_t(word>>(8*i));};size_t record=108;
 auto action=[&](uint32_t type,uint32_t parameter,uint32_t label){put(record,type);put(record+4,parameter);put(record+8,label);put(record+12,UINT32_MAX);record+=16;};
 if(pet)for(uint32_t id=16175;id<=16179;++id)action(9,id,id);
 if(enter)action(1,0,723);if(move)action(2,1,5244);if(pickup)action(4,0,724);
 if(inspect)action(7,0,720); // Client action7 displays cached ItemPtr or requests58c7; eqstr720=Inspect.
 if(swap)action(17,0,5336); // Original eqstr5336: Swap Houses; confirmation67 emits21ba.
 bytes[748]=(move||pickup||enter||inspect||swap||pet)?0:1;put(752,mouse[0]);put(756,mouse[1]);return bytes;
}
}
#endif
