#ifndef EQEMU_HOUSING_ESTATE_WIRE_H
#define EQEMU_HOUSING_ESTATE_WIRE_H

#include "neighborhood_wire.h"
#include <cmath>
#include <algorithm>
#include <set>

namespace Housing::EstateWire {
// May10 2013 native readers; provenance: housing/protocol/PLOT-BOOTSTRAP.md
// and PURCHASE-PROTOCOL.md. These codecs do not supply a Live content catalog.
inline constexpr uint16_t DefinitionOpcode = 0x2698;
inline constexpr uint16_t InspectOpcode = 0x796c;
inline constexpr uint16_t PurchaseOpcode = 0x45be;
inline constexpr size_t PurchaseSize = 188;
struct Definition {
 uint32_t id{}, area_id{}, purchase_scheme{}, upkeep_scheme{}, max_items{}, max_non_estate_items{},
          max_children{}, zone_id{}, dynamic_zone_id{}, type{}, group_id{};
 std::string address;
 uint32_t icon{}, max_npc_items{};
 std::array<float,3> switch_coordinates{};
 float switch_heading{};
 std::vector<uint32_t> children;
};
struct CostValue { int32_t type_id{}; uint32_t subtype_id{}; int64_t amount{}; };
struct CostComponent { uint32_t id{}; std::vector<CostValue> values; };
struct CostScheme { uint32_t id{}; std::vector<CostComponent> components; };
struct Inspection {
 uint32_t estate_id{}, definition_id{}, parent_id{}, guild_id{0xffffffff}, fellowship_id{};
 uint8_t type{};
 std::array<std::string,4> strings{}; // context, display name, owner name, owner handle
 std::array<uint32_t,4> opaque_tail{};
};
struct EscrowBalance { uint32_t currency_key{}; double balance{}; };
struct EstateSnapshot {
 uint32_t estate_id{}, static_id{}, parent_id{}, definition_id{}, guild_id{0xffffffff}, fellowship_id{};
 std::string owner_name, owner_handle;
 uint32_t default_permission_level{};
 std::string dynamic_name;
 std::vector<uint32_t> children;
 std::vector<EscrowBalance> escrow;
 uint32_t default_upkeep_component{}, expiry_time{};
 uint8_t voting_opted_out{};
};
struct PlotSearchRow {
 uint32_t numeric_icon_value{}, type{}, estate_id{};
 std::array<std::string,2> strings{}; // Native fourth/second columns; names unresolved.
};
struct PlotSearchRequest {
 uint32_t mode{},context{};std::string owner_query;
 std::array<uint8_t,NeighborhoodWire::SearchSize> source{}; // Includes opaque native filters/padding.
};
struct VotingOptOutRequest { uint32_t estate_id{}; bool opted_out{}; };
inline std::optional<VotingOptOutRequest> DecodeVotingOptOut(const uint8_t* p,size_t size) {
 if(!p||size!=8||p[4]>1)return {};
 const auto id=NeighborhoodWire::Detail::Read32(p);if(!id||id==UINT32_MAX)return {};
 // Native6573 initializes only byte4; bytes5..7 are stale, not a DWORD Boolean.
 return VotingOptOutRequest{id,p[4]!=0};
}
struct PurchaseRequest {
 uint32_t estate_id{}, sentinel_a{}, sentinel_b{}, price_component{}, upkeep_component{}, client_cached_guild_roster_count{};
 std::array<uint8_t,160> reserved{};
 std::array<uint8_t,3> flags{};
 uint8_t padding{};
};
namespace Detail {
using NeighborhoodWire::Detail::Read32;
using NeighborhoodWire::Detail::Write32;
class Writer {
public:
 std::vector<uint8_t> bytes;
 void Room(size_t n) const {
  if(n>NeighborhoodWire::MaximumPayload-bytes.size()) throw std::invalid_argument("Estate packet too large");
 }
 void Byte(uint8_t v) { Room(1);bytes.push_back(v); }
 void Word(uint32_t v) { Room(4);Write32(bytes,v); }
 void Wide(int64_t v) { Room(8);for(size_t i=0;i<8;++i)bytes.push_back(uint8_t(uint64_t(v)>>(8*i))); }
 void Float(float v) {
  if(!std::isfinite(v))throw std::invalid_argument("Nonfinite estate coordinate");
  uint32_t bits;static_assert(sizeof(bits)==sizeof(v));std::memcpy(&bits,&v,4);Word(bits);
 }
 void Text(const std::string& s) {
  if(s.size()>255 || s.find('\0')!=std::string::npos)throw std::invalid_argument("Invalid estate string");
  for(unsigned char c:s)if(c<32||c>126)throw std::invalid_argument("Invalid estate text byte");
  Room(4+s.size());Word(uint32_t(s.size()));bytes.insert(bytes.end(),s.begin(),s.end());
 }
};
inline void Identity(uint32_t id) {
 if(!id||id==0xffffffff)throw std::invalid_argument("Invalid estate identity");
}
} // namespace Detail

inline std::vector<uint8_t> EncodeBootstrap(const std::vector<Definition>& definitions,
 const std::vector<CostScheme>& schemes) {
 if(definitions.size()>NeighborhoodWire::MaximumPayload/76 || schemes.size()>NeighborhoodWire::MaximumPayload/8)
  throw std::invalid_argument("Estate catalog count exceeds payload limit");
 Detail::Writer w;std::set<uint32_t> definition_ids,scheme_ids;
 for(const auto& d:definitions){Detail::Identity(d.id);if(!definition_ids.insert(d.id).second)throw std::invalid_argument("Duplicate estate definition");}
 for(const auto& s:schemes){Detail::Identity(s.id);if(!scheme_ids.insert(s.id).second)throw std::invalid_argument("Duplicate cost scheme");}
 // This opcode clears both native catalogs. Publish a complete, consistent batch.
 w.Word(uint32_t(definitions.size()));
 for(const auto& d:definitions){
  if(d.type<1||d.type>8)throw std::invalid_argument("Unknown estate type");
  for(uint32_t id:{d.purchase_scheme,d.upkeep_scheme})
   if(id && !scheme_ids.count(id))throw std::invalid_argument("Missing referenced cost scheme");
  for(uint32_t id:{d.id,d.area_id,d.purchase_scheme,d.upkeep_scheme,d.max_items,d.max_non_estate_items,
    d.max_children,d.zone_id,d.dynamic_zone_id,d.type,d.group_id})w.Word(id);
  w.Text(d.address);w.Word(d.icon);w.Word(d.max_npc_items);
  for(float coordinate:d.switch_coordinates)w.Float(coordinate);w.Float(d.switch_heading);
  w.Word(uint32_t(d.children.size()));std::set<uint32_t> child_ids;
  for(uint32_t id:d.children){
   if(id==d.id || !definition_ids.count(id) || !child_ids.insert(id).second)
    throw std::invalid_argument("Invalid child definition reference");
   w.Word(id);
  }
 }
 w.Word(uint32_t(schemes.size()));
 for(const auto& s:schemes){
  w.Word(s.id);w.Word(uint32_t(s.components.size()));std::set<uint32_t> components;
  for(const auto& c:s.components){
   Detail::Identity(c.id);if(!components.insert(c.id).second)throw std::invalid_argument("Duplicate cost component");
   w.Word(c.id);w.Word(uint32_t(c.values.size()));
   for(const auto& v:c.values){w.Word(uint32_t(v.type_id));w.Word(v.subtype_id);w.Wide(v.amount);}
  }
 }
 return std::move(w.bytes);
}
inline std::vector<uint8_t> EncodeInspection(const Inspection& inspection, bool show=true, uint32_t request=0) {
 Detail::Identity(inspection.estate_id);Detail::Identity(inspection.definition_id);
 if(inspection.type!=4 && inspection.type!=8)throw std::invalid_argument("Inspection requires a plot type");
 Detail::Writer w;w.Word(request);w.Byte(inspection.type);w.Word(0);w.Byte(1);w.Byte(0);
 for(uint32_t id:{inspection.estate_id,inspection.parent_id,inspection.definition_id,
   inspection.guild_id,inspection.fellowship_id})w.Word(id);
 for(const auto& s:inspection.strings)w.Text(s);
 for(uint32_t v:inspection.opaque_tail)w.Word(v);
 w.Byte(show?1:0);return std::move(w.bytes);
}
inline std::vector<uint8_t> EncodeEstateSnapshot(const EstateSnapshot& state) {
 Detail::Identity(state.estate_id);Detail::Identity(state.definition_id);
 if(state.escrow.size()>255)throw std::invalid_argument("Too many escrow balances");
 Detail::Writer w;
 for(uint32_t v:{state.estate_id,state.static_id,state.parent_id,state.definition_id,state.guild_id,state.fellowship_id})w.Word(v);
 w.Text(state.owner_name);w.Text(state.owner_handle);w.Word(state.default_permission_level);
 w.Word(uint32_t(state.children.size()));w.Text(state.dynamic_name);
 std::set<uint32_t> child_ids;
 for(uint32_t v:state.children){Detail::Identity(v);if(v==state.estate_id || !child_ids.insert(v).second)throw std::invalid_argument("Invalid estate child");w.Word(v);}
 w.Byte(uint8_t(state.escrow.size()));std::set<uint32_t> currency_keys;
 for(const auto& balance:state.escrow){
  if(!std::isfinite(balance.balance) || !currency_keys.insert(balance.currency_key).second)
   throw std::invalid_argument("Invalid estate escrow balance");
  w.Word(balance.currency_key);uint64_t bits;static_assert(sizeof(bits)==sizeof(balance.balance));
  std::memcpy(&bits,&balance.balance,8);w.Room(8);
  for(size_t i=0;i<8;++i)w.bytes.push_back(uint8_t(bits>>(8*i)));
 }
 w.Word(state.default_upkeep_component);w.Word(state.expiry_time);w.Byte(state.voting_opted_out);
 return std::move(w.bytes);
}
inline std::vector<uint8_t> EncodePlotSearch(uint32_t mode,const std::string& title,
 const std::vector<PlotSearchRow>& rows,uint32_t request=0) {
 if((mode!=4&&mode!=5)||rows.size()>65535)throw std::invalid_argument("Invalid plot search response");
 Detail::Writer w;w.Word(request);w.Byte(5);w.Word(mode);w.Byte(uint8_t(rows.size()));w.Byte(uint8_t(rows.size()>>8));w.Text(title);
 for(const auto& row:rows){
  Detail::Identity(row.estate_id);if(row.type!=4&&row.type!=8)throw std::invalid_argument("Plot search requires plot type");
  w.Word(row.numeric_icon_value);w.Word(row.type);w.Word(row.estate_id);
  for(const auto& text:row.strings)w.Text(text);
 }
 return std::move(w.bytes);
}
inline std::optional<uint32_t> DecodeInspect(const uint8_t* p,size_t size) {
 if(!p||size!=4)return std::nullopt;
 uint32_t id=Detail::Read32(p);if(!id||id==0xffffffff)return std::nullopt;return id;
}
inline std::optional<PlotSearchRequest> DecodePlotSearch(const uint8_t* p,size_t size){
 if(!p||size!=NeighborhoodWire::SearchSize)return std::nullopt;
 PlotSearchRequest r;r.mode=Detail::Read32(p);if(r.mode!=4&&r.mode!=5)return std::nullopt;
 auto owner=NeighborhoodWire::Detail::FixedName(p+92,64);if(!owner)return std::nullopt;
 r.context=Detail::Read32(p+4);r.owner_query=std::move(*owner);
 std::copy(p,p+size,r.source.begin());return r;
}
inline std::optional<PurchaseRequest> DecodePurchase(const uint8_t* p,size_t size) {
 if(!p||size!=PurchaseSize)return std::nullopt;
 PurchaseRequest r{};r.estate_id=Detail::Read32(p);
 if(!r.estate_id||r.estate_id==0xffffffff)return std::nullopt;
 r.sentinel_a=Detail::Read32(p+4);r.sentinel_b=Detail::Read32(p+8);
 std::memcpy(r.reserved.data(),p+12,r.reserved.size());
 r.price_component=Detail::Read32(p+172);r.upkeep_component=Detail::Read32(p+176);
 // Native guild-manager cached roster count. Never use it as guild authority or a funding selector.
 r.client_cached_guild_roster_count=Detail::Read32(p+180);std::memcpy(r.flags.data(),p+184,3);r.padding=p[187];
 return r;
}
} // namespace Housing::EstateWire
#endif
