#ifndef EQEMU_HOUSING_TROPHY_ANCHOR_WIRE_H
#define EQEMU_HOUSING_TROPHY_ANCHOR_WIRE_H
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

// Exact May 10 2013 client, SHA256 2a8702ad9f722704f01355c0750be7d6f164a8b9c9128ba0cf286ea32b405b0e.
// Evidence: TROPHY-ANCHOR-PROTOCOL.md, dispatch485990 and reader4d5940,
// confirmation52dfd0. Framing only: no client field grants buffs or zoning.
namespace Housing::TrophyAnchorWire {
inline constexpr uint16_t SelectionOpcode=0x4b65, ListOpcode=0x7697,
 TimerOpcode=0x51ae, ClearOpcode=0x08bf, AnchorOpcode=0x7bb1;
inline constexpr size_t Slots=10, ConfirmationSize=84;
inline constexpr uint32_t EmptyBenefit=0xffffffff;
namespace Detail {
inline uint32_t Read(const uint8_t* p) {
 return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
inline void Write(std::vector<uint8_t>& b,uint32_t v) {
 for(unsigned s=0;s<32;s+=8)b.push_back(uint8_t(v>>s));
}
inline int32_t Signed(uint32_t bits) {
 int32_t result;std::memcpy(&result,&bits,4);return result;
}
inline bool Name(const std::string& s) {return !s.empty()&&s.size()<=63&&s.find('\0')==std::string::npos;}
}
struct Selection {uint32_t slot=0,benefit_id=EmptyBenefit;int32_t tier=-1;};
inline std::optional<Selection> DecodeSelection(const uint8_t* p,size_t n) {
 if(!p||n!=12||Detail::Read(p)>=Slots)return {};
 return Selection{Detail::Read(p),Detail::Read(p+4),Detail::Signed(Detail::Read(p+8))};
}
inline std::vector<uint8_t> EncodeSelection(const Selection& s) {
 if(s.slot>=Slots)throw std::invalid_argument("Trophy slot outside native ten slots");
 std::vector<uint8_t> b;Detail::Write(b,s.slot);Detail::Write(b,s.benefit_id);Detail::Write(b,uint32_t(s.tier));return b;
}
struct Benefits {
 uint32_t enabled=0;
 std::array<uint32_t,Slots> ids=[] {std::array<uint32_t,Slots> a{};a.fill(EmptyBenefit);return a;}();
 std::array<int32_t,Slots> tiers=[] {std::array<int32_t,Slots> a{};a.fill(-1);return a;}();
};
inline std::vector<uint8_t> EncodeBenefits(const Benefits& s) {
 std::vector<uint8_t> b;Detail::Write(b,s.enabled);
 // The native reader copies TWO arrays, not ten interleaved selections.
 for(auto id:s.ids)Detail::Write(b,id);
 for(auto tier:s.tiers)Detail::Write(b,uint32_t(tier));return b;
}
inline std::optional<Benefits> DecodeBenefits(const uint8_t* p,size_t n) {
 if(!p||n!=84)return {};Benefits s;s.enabled=Detail::Read(p);
 for(size_t i=0;i<Slots;++i){s.ids[i]=Detail::Read(p+4+4*i);s.tiers[i]=Detail::Signed(Detail::Read(p+44+4*i));}return s;
}
inline std::vector<uint8_t> EncodeTimer(uint32_t remaining_ms) {std::vector<uint8_t> b;Detail::Write(b,remaining_ms);return b;}
inline std::vector<uint8_t> EncodeClear() {return {};}

struct Anchor {
 std::string caster;
 // Preserve all five DWORDs. Only word[1]'s estate lookup and word[4]'s
 // personal(ffffffff)/guild discriminator are proven; no spell/zone guesses.
 std::array<uint32_t,5> words{0,0xffffffff,0xffffffff,0xffffffff,0xffffffff};
};
inline std::vector<uint8_t> EncodeAnchorOffer(const Anchor& s,const std::string& destination) {
 if(!Detail::Name(s.caster)||destination.find('\0')!=std::string::npos||destination.size()>4096)
  throw std::invalid_argument("Invalid bounded anchor display text");
 std::vector<uint8_t> b(s.caster.begin(),s.caster.end());b.push_back(0);
 for(auto word:s.words)Detail::Write(b,word);
 Detail::Write(b,uint32_t(destination.size()));b.insert(b.end(),destination.begin(),destination.end());return b;
}
inline std::optional<Anchor> DecodeAnchorConfirmation(const uint8_t* p,size_t n) {
 if(!p||n!=ConfirmationSize)return {};
 const auto end=std::find(p,p+64,uint8_t(0));if(end==p||end==p+64)return {};
 Anchor s;s.caster.assign(reinterpret_cast<const char*>(p),size_t(end-p));
 // Constructor4b3910 zeroes the complete name buffer. Strict zero padding
 // prevents alternate encodings; names and all five words match pending offer.
 if(!std::all_of(end,p+64,[](uint8_t v){return v==0;}))return {};
 for(size_t i=0;i<5;++i)s.words[i]=Detail::Read(p+64+4*i);return s;
}
inline bool Matches(const Anchor& offered,const Anchor& accepted) {
 return offered.caster==accepted.caster&&offered.words==accepted.words;
}
} // namespace Housing::TrophyAnchorWire
#endif
