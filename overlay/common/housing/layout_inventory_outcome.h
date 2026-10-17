#ifndef EQEMU_HOUSING_LAYOUT_INVENTORY_OUTCOME_H
#define EQEMU_HOUSING_LAYOUT_INVENTORY_OUTCOME_H
#include "layout_inventory_units.h"
#include <cmath>
namespace Housing::LayoutInventory {
// Durable INVENTORY COMPONENT of the future mixed layout outcome. Not a wire
// format, authorization proof, journal, pending marker or recovery release.
// Every consumed unit is linked to its persistent final property identity.
struct SourceFacts {uint32_t available=0,consumed=0;int32_t runtime_serial=0;bool stackable=false;};
struct OutcomePiece {size_t operation_index=0,source_index=0;int32_t runtime_serial=0;LiveItems::LayoutUnitPlacement placement;uint64_t room_scope_revision=0;};
struct Outcome {Consumption inventory;std::vector<SourceFacts> sources;std::vector<OutcomePiece> pieces;};
inline constexpr size_t MaximumInventoryOutcomeBytes=1048576;
namespace OutcomeDetail {
using Bytes=CustodyCodec::Bytes;using Reader=CustodyCodec::Reader;
inline void Require(bool yes){if(!yes)throw std::invalid_argument("Invalid layout inventory outcome");}
inline bool Id(uint32_t id){return id&&id!=UINT32_MAX;}
inline void Blob(Bytes& bytes,const Bytes& value){CustodyCodec::W(bytes,value.size(),4);bytes.insert(bytes.end(),value.begin(),value.end());}
inline void Row(Bytes& bytes,const LiveItems::InventoryRow& row){Blob(bytes,CustodyCodec::Encode(row));}
inline LiveItems::InventoryRow Row(Reader& reader){return CustodyCodec::Decode(reader.Blob(65700));}
inline bool Flag(Reader& reader){auto n=reader.N(1);Require(n<=1);return bool(n);}
inline void Pose(Bytes& b,const PlacementPolicy::Transform& pose){for(float v:{pose.heading,pose.pitch,pose.roll,pose.scale,pose.position[0],pose.position[1],pose.position[2]}){Require(std::isfinite(v));uint32_t bits;std::memcpy(&bits,&v,4);CustodyCodec::W(b,bits,4);}}
inline PlacementPolicy::Transform Pose(Reader& r){std::array<float,7> a;for(float& v:a){uint32_t bits=r.N(4);std::memcpy(&v,&bits,4);Require(std::isfinite(v));}return {a[0],a[1],a[2],a[3],{a[4],a[5],a[6]}};}
inline void Reservation(Bytes& b,const LiveItems::Reservation& p){
 CustodyCodec::W(b,p.native_item_id,4);CustodyCodec::W(b,p.native_estate_id,4);CustodyCodec::W(b,p.estate_id,4);CustodyCodec::S(b,p.catalog_key);CustodyCodec::W(b,p.catalog_revision,8);CustodyCodec::W(b,bool(p.content_binding),1);
 if(p.content_binding){CustodyCodec::W(b,p.content_binding->version,4);b.insert(b.end(),p.content_binding->digest.begin(),p.content_binding->digest.end());}
}
inline LiveItems::Reservation Reservation(Reader& r){
 LiveItems::Reservation p;p.native_item_id=r.N(4);p.native_estate_id=r.N(4);p.estate_id=r.N(4);p.catalog_key=r.Text(96);p.catalog_revision=r.N(8);
 if(Flag(r)){ContentBinding::Identity identity;identity.version=r.N(4);for(auto& byte:identity.digest)byte=r.N(1);p.content_binding=identity;}return p;
}
inline void Room(Bytes& b,const LiveItems::InteriorReservation& p){
 Reservation(b,p.placement);CustodyCodec::W(b,p.house_id,8);CustodyCodec::W(b,p.house_revision,8);CustodyCodec::W(b,p.profile_revision,8);CustodyCodec::W(b,p.source.zone_id,4);CustodyCodec::W(b,p.source.zone_version,1);CustodyCodec::W(b,p.source.instance_id,2);b.insert(b.end(),p.content_digest.begin(),p.content_digest.end());
}
inline LiveItems::InteriorReservation Room(Reader& r){
 LiveItems::InteriorReservation p;p.placement=Reservation(r);p.house_id=r.N(8);p.house_revision=r.N(8);p.profile_revision=r.N(8);p.source.zone_id=r.N(4);p.source.zone_version=r.N(1);p.source.instance_id=r.N(2);for(auto& byte:p.content_digest)byte=r.N(1);return p;
}
inline void Placement(Bytes& b,const LiveItems::LayoutUnitPlacement& p){
 const auto& s=p.stored;CustodyCodec::W(b,s.custody_id,8);CustodyCodec::W(b,s.estate_id,4);Row(b,s.row);
 for(int32_t v:{s.catalog.item_class,s.catalog.bag_slots,s.catalog.lore_group,s.catalog.no_drop})CustodyCodec::W(b,uint32_t(v),4);
 Require(bool(s.placement));Reservation(b,s.placement->reservation);Pose(b,s.placement->transform);CustodyCodec::W(b,s.placement->pose_revision,8);
 CustodyCodec::W(b,s.current_catalog_revision,8);CustodyCodec::W(b,s.storage_revision,8);CustodyCodec::W(b,s.stowed,1);CustodyCodec::W(b,bool(p.room),1);if(p.room)Room(b,*p.room);
}
inline LiveItems::LayoutUnitPlacement Placement(Reader& r){
 LiveItems::LayoutUnitPlacement p;auto& s=p.stored;s.custody_id=r.N(8);s.estate_id=r.N(4);s.row=Row(r);
 for(auto* v:{&s.catalog.item_class,&s.catalog.bag_slots,&s.catalog.lore_group,&s.catalog.no_drop}){uint32_t word=r.N(4);std::memcpy(v,&word,4);}
 LiveItems::Placed pose;pose.reservation=Reservation(r);pose.transform=Pose(r);pose.pose_revision=r.N(8);s.placement=pose;s.current_catalog_revision=r.N(8);s.storage_revision=r.N(8);s.stowed=Flag(r);if(Flag(r))p.room=Room(r);return p;
}
inline bool SameReservation(const LiveItems::Reservation& a,const LiveItems::Reservation& b){Bytes x,y;Reservation(x,a);Reservation(y,b);return x==y;}
inline void Shape(const Outcome& o){
 const auto count=o.sources.size();Require(count&&count<=UINT16_MAX&&o.inventory.changes.size()==count&&o.inventory.review.locked_rows.size()<=2*UINT16_MAX&&!o.pieces.empty()&&o.pieces.size()<=UINT16_MAX);
 std::map<uint32_t,const LiveItems::InventoryRow*> reviewed;uint32_t actor=0,last_slot=0;bool first=true;
 for(const auto& row:o.inventory.review.locked_rows){Require(CustodyCodec::Valid(row)&&row.values.character_id&&(!actor||row.values.character_id==actor)&&(first||row.values.slot_id>last_slot));actor=row.values.character_id;last_slot=row.values.slot_id;first=false;Require(reviewed.emplace(last_slot,&row).second);}
 std::vector<const SourceChange*> changes(count);first=true;std::set<int32_t> serials;
 for(const auto& c:o.inventory.changes){Require(c.source_index<count&&!changes[c.source_index]&&c.before.values.character_id==actor&&InventoryProjection::Shape(c.before)&&(first||c.before.values.slot_id>last_slot));first=false;last_slot=c.before.values.slot_id;changes[c.source_index]=&c;auto r=reviewed.find(last_slot);Require(r!=reviewed.end()&&CustodyCodec::Encode(*r->second)==CustodyCodec::Encode(c.before));}
 std::set<uint32_t> expected_slots;for(const auto* change:changes){const auto slot=change->before.values.slot_id;expected_slots.insert(slot);if(slot<23||slot>33){Require(slot<=INT16_MAX);const auto root=EQ::InventoryProfile::CalcSlotId(int16_t(slot));const auto index=EQ::InventoryProfile::CalcBagIdx(int16_t(slot));Require(root>=23&&root<=33&&index<EQ::invbag::SLOT_COUNT&&EQ::InventoryProfile::CalcSlotId(root,index)==slot);expected_slots.insert(uint32_t(root));}}
 Require(reviewed.size()==expected_slots.size());for(auto slot:expected_slots)Require(reviewed.count(slot));
 uint64_t total=0;
 for(size_t i=0;i<count;++i){const auto& f=o.sources[i];const auto& c=*changes[i];Require(f.runtime_serial>0&&serials.insert(f.runtime_serial).second&&f.available&&f.consumed&&f.consumed<=f.available);
  if(f.stackable){const auto raw=c.before.values.charges;Require(raw!=32767&&raw<32768&&f.available==uint32_t(raw?raw:1));}else Require(f.available==1);
  std::optional<LiveItems::InventoryRow> after;if(f.consumed<f.available){after=c.before;after->values.charges=f.available-f.consumed;after->null_fields&=~LiveItems::NullCharges;}
  Require(bool(after)==bool(c.after)&&(!after||CustodyCodec::Encode(*after)==CustodyCodec::Encode(*c.after)));total+=f.consumed;
 }
 Require(total==o.pieces.size());std::vector<uint32_t> uses(count);std::set<uint64_t> custody;std::set<uint32_t> native;
 size_t ordinal=0;first=true;
 for(const auto& p:o.pieces){Require(p.source_index<count&&p.operation_index<UINT16_MAX&&(first||p.operation_index>ordinal)&&p.runtime_serial>0&&serials.insert(p.runtime_serial).second);first=false;ordinal=p.operation_index;++uses[p.source_index];
  const auto& s=p.placement.stored;const auto& c=*changes[p.source_index];auto expected=c.before;expected.values.guid=uint32_t(p.runtime_serial);expected.null_fields&=~LiveItems::NullGuid;if(o.sources[p.source_index].stackable){expected.values.charges=1;expected.null_fields&=~LiveItems::NullCharges;}
  Require(CustodyCodec::Encode(expected)==CustodyCodec::Encode(s.row)&&s.custody_id&&custody.insert(s.custody_id).second&&Id(s.estate_id)&&s.catalog.item_class==0&&!s.catalog.bag_slots&&s.placement&&s.current_catalog_revision&&(!s.stowed||s.storage_revision));
  const auto& placed=*s.placement;const auto& r=placed.reservation;Require(Id(r.native_item_id)&&native.insert(r.native_item_id).second&&Id(r.native_estate_id)&&r.estate_id==s.estate_id&&!r.catalog_key.empty()&&r.catalog_key.size()<=96&&r.catalog_revision&&placed.pose_revision);
  if(r.content_binding)Require(ContentBinding::FurnitureVersion(r.content_binding->version));Bytes pose;Pose(pose,placed.transform);Require(placed.transform.scale>0);
  if(!p.placement.room)Require(!p.room_scope_revision);
  if(p.placement.room){const auto& room=*p.placement.room;Require(p.room_scope_revision&&!r.content_binding&&SameReservation(room.placement,r)&&room.house_id&&room.house_revision&&room.profile_revision&&room.source.zone_id&&room.source.instance_id);}
 }
 for(size_t i=0;i<count;++i)Require(uses[i]==o.sources[i].consumed);
}
}
inline CustodyCodec::Bytes EncodeOutcome(const Outcome& o){
 using namespace OutcomeDetail;Shape(o);Bytes b;CustodyCodec::W(b,1,4);CustodyCodec::W(b,o.sources.size(),2);
 for(const auto& f:o.sources){CustodyCodec::W(b,f.available,4);CustodyCodec::W(b,f.consumed,4);CustodyCodec::W(b,uint32_t(f.runtime_serial),4);CustodyCodec::W(b,f.stackable,1);}
 CustodyCodec::W(b,o.inventory.review.locked_rows.size(),4);for(const auto& row:o.inventory.review.locked_rows){Row(b,row);Require(b.size()<=MaximumInventoryOutcomeBytes);}
 for(const auto& c:o.inventory.changes){CustodyCodec::W(b,c.source_index,2);Row(b,c.before);CustodyCodec::W(b,bool(c.after),1);if(c.after)Row(b,*c.after);Require(b.size()<=MaximumInventoryOutcomeBytes);}
 CustodyCodec::W(b,o.pieces.size(),2);for(const auto& p:o.pieces){CustodyCodec::W(b,p.operation_index,2);CustodyCodec::W(b,p.source_index,2);CustodyCodec::W(b,uint32_t(p.runtime_serial),4);Placement(b,p.placement);CustodyCodec::W(b,p.room_scope_revision,8);Require(b.size()<=MaximumInventoryOutcomeBytes);}return b;
}
inline std::optional<Outcome> DecodeOutcome(const CustodyCodec::Bytes& b){
 using namespace OutcomeDetail;
 try{if(b.size()>MaximumInventoryOutcomeBytes)return {};Reader r{b};Require(r.N(4)==1);Outcome o;const auto count=r.N(2);Require(count&&count<=(b.size()-r.at)/13);o.sources.reserve(count);
  for(size_t i=0;i<count;++i){SourceFacts f;f.available=r.N(4);f.consumed=r.N(4);auto serial=uint32_t(r.N(4));Require(serial<=INT32_MAX);f.runtime_serial=serial;f.stackable=Flag(r);o.sources.push_back(f);}
  auto rows=r.N(4);Require(rows<=2*UINT16_MAX&&rows<=(b.size()-r.at)/4);for(size_t i=0;i<rows;++i)o.inventory.review.locked_rows.push_back(Row(r));
  for(size_t i=0;i<count;++i){SourceChange c;c.source_index=r.N(2);c.before=Row(r);if(Flag(r))c.after=Row(r);o.inventory.changes.push_back(std::move(c));}
  auto pieces=r.N(2);Require(pieces&&pieces<=(b.size()-r.at)/8);for(size_t i=0;i<pieces;++i){OutcomePiece p;p.operation_index=r.N(2);p.source_index=r.N(2);auto serial=uint32_t(r.N(4));Require(serial<=INT32_MAX);p.runtime_serial=serial;p.placement=Placement(r);p.room_scope_revision=r.N(8);o.pieces.push_back(std::move(p));}
  Require(r.at==b.size());Shape(o);return o;
 }catch(...){return {};}
}
inline Outcome MakeOutcome(const Plan& plan,const Prepared& prepared,const Consumption& consumed,const std::vector<LiveItems::LayoutUnitPlacement>& placements){
 OutcomeDetail::Require(Coherent(plan)&&placements.size()==prepared.units.size()&&prepared.units.size()==plan.uses.size());Outcome o;o.inventory=consumed;
 for(const auto& c:consumed.changes){OutcomeDetail::Require(c.source_index<plan.sources.size());const auto& source=plan.sources[c.source_index];OutcomeDetail::Require(CustodyCodec::Encode(c.before)==CustodyCodec::Encode(source.before)&&bool(c.after)==bool(source.after)&&(!c.after||CustodyCodec::Encode(*c.after)==CustodyCodec::Encode(*source.after)));}
 for(const auto& s:plan.sources)o.sources.push_back({s.available,s.consumed,s.serial,s.stackable});
 for(size_t i=0;i<prepared.units.size();++i){const auto& unit=prepared.units[i];OutcomeDetail::Require(unit.item&&unit.source_index==plan.uses[i].source_index&&unit.operation_index==plan.uses[i].operation_index&&CustodyCodec::Encode(unit.row)==CustodyCodec::Encode(placements[i].stored.row));o.pieces.push_back({unit.operation_index,unit.source_index,unit.item->GetSerialNumber(),placements[i],placements[i].room?1u:0u});}
 OutcomeDetail::Shape(o);return o;
}
}
#endif
