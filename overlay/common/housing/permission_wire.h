#ifndef EQEMU_HOUSING_PERMISSION_WIRE_H
#define EQEMU_HOUSING_PERMISSION_WIRE_H
#include "estate_wire.h"
#include <algorithm>

namespace Housing::PermissionWire {
// May10 2013: reader5b7a90/writer749420. Default masks5b5830/5b3d50.
inline constexpr uint16_t SnapshotOpcode=0x485d, EditOpcode=0x3146, OwnedOpcode=0x2aa7;
inline constexpr uint32_t KnownCapabilities=0x1fff;
inline constexpr std::array<uint32_t,7> NativeDefaults{3,3,0x803,0xe03,0x1f87,0x1fff,0};
struct Member { std::string name; uint32_t group{}; };
struct Snapshot {
 uint32_t estate_id{};
 std::array<uint32_t,7> masks=NativeDefaults;
 std::vector<Member> members;
};
namespace Detail {
inline std::string Key(const std::string& name){
 std::string s=name;for(char& c:s)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return s;
}
inline bool ValidMember(const Member& m){
 if(m.group>5 || m.name.empty() || m.name.size()>63)return false;
 for(unsigned char c:m.name)if(c<32||c>126)return false;
 return true;
}
inline bool ValidMasks(const std::array<uint32_t,7>& masks){
 return std::all_of(masks.begin(),masks.end(),[](uint32_t mask){return !(mask&~KnownCapabilities);});
}
}
inline std::vector<uint8_t> EncodeSnapshot(const Snapshot& snapshot){
 EstateWire::Detail::Identity(snapshot.estate_id);
 if(!Detail::ValidMasks(snapshot.masks) || snapshot.members.size()>(NeighborhoodWire::MaximumPayload-64)/9)
  throw std::invalid_argument("Invalid housing permission bounds");
 EstateWire::Detail::Writer w;w.Word(snapshot.estate_id);
 for(uint32_t group=0;group<7;++group){w.Word(group);w.Word(snapshot.masks[group]);}
 w.Word(uint32_t(snapshot.members.size()));std::set<std::string> names;
 for(const auto& member:snapshot.members){
  if(!Detail::ValidMember(member) || !names.insert(Detail::Key(member.name)).second)
   throw std::invalid_argument("Invalid housing permission member");
  w.Text(member.name);w.Word(member.group);
 }
 return std::move(w.bytes);
}
inline std::optional<Snapshot> DecodeEdit(const uint8_t* p,size_t size){
 if(!p||size<64||size>NeighborhoodWire::MaximumPayload)return std::nullopt;
 auto word=[&](size_t at){return NeighborhoodWire::Detail::Read32(p+at);};
 Snapshot result{};result.estate_id=word(0);if(!result.estate_id||result.estate_id==0xffffffff)return std::nullopt;
 for(uint32_t group=0;group<7;++group){
  if(word(4+8*group)!=group)return std::nullopt;result.masks[group]=word(8+8*group);
 }
 if(!Detail::ValidMasks(result.masks))return std::nullopt;
 uint32_t count=word(60);if(count>(size-64)/9)return std::nullopt;
 size_t at=64;std::set<std::string> names;
 for(uint32_t i=0;i<count;++i){
  if(size-at<4)return std::nullopt;uint32_t length=word(at);at+=4;
  if(!length||length>63||length>size-at||size-at-length<4)return std::nullopt;
  Member member{std::string(reinterpret_cast<const char*>(p+at),length),word(at+length)};at+=length+4;
  if(!Detail::ValidMember(member)||!names.insert(Detail::Key(member.name)).second)return std::nullopt;
  result.members.push_back(std::move(member));
 }
 if(at!=size)return std::nullopt;return result;
}
// Full replacement; never send just a new plot or mirror item/archive arrays.
inline std::vector<uint8_t> EncodeOwned(const std::vector<uint32_t>& ids){
 if(ids.size()>(NeighborhoodWire::MaximumPayload-4)/4)throw std::invalid_argument("Owned list too large");
 EstateWire::Detail::Writer w;w.Word(uint32_t(ids.size()));std::set<uint32_t> unique;
 for(uint32_t id:ids){EstateWire::Detail::Identity(id);if(!unique.insert(id).second)throw std::invalid_argument("Duplicate owned estate");w.Word(id);}
 return std::move(w.bytes);
}
} // namespace Housing::PermissionWire
#endif
