#ifndef EQEMU_HOUSING_LAYOUT_HOUSE_IDENTITY_H
#define EQEMU_HOUSING_LAYOUT_HOUSE_IDENTITY_H
#include "house_scene_presentation.h"
#include "custody_codec.h"
namespace Housing::LayoutHouse {
using Identity=std::shared_ptr<const HouseBinding::Binding>;
inline bool Valid(const HouseBinding::Binding& h){
 return h.house_id&&h.revision&&h.item_template_id&&h.pending_key.empty()&&
  h.location.kind==HouseBinding::Location::Kind::Custody&&h.location.custody_id&&!h.location.character_id&&!h.location.slot_id&&
  h.interior&&h.interior->native_estate_id&&h.interior->native_estate_id!=UINT32_MAX&&h.interior->definition_id&&h.interior->zone_id&&
  h.interior->instance_id&&h.interior->instance_id<=32000&&!h.interior->catalog_key.empty()&&h.interior->catalog_key.size()<=96&&h.interior->catalog_revision;
}
inline CustodyCodec::Bytes Encode(const HouseBinding::Binding& h){
 if(!Valid(h))throw std::invalid_argument("Invalid layout house identity");
 CustodyCodec::Bytes b;CustodyCodec::W(b,h.house_id,8);CustodyCodec::W(b,h.revision,8);CustodyCodec::W(b,h.item_template_id,4);CustodyCodec::W(b,h.location.custody_id,8);
 const auto& i=*h.interior;for(auto n:{i.native_estate_id,i.definition_id,i.zone_id})CustodyCodec::W(b,n,4);CustodyCodec::W(b,i.instance_id,2);CustodyCodec::W(b,i.zone_version,1);CustodyCodec::S(b,i.catalog_key);CustodyCodec::W(b,i.catalog_revision,8);return b;
}
inline Identity Decode(const CustodyCodec::Bytes& b){
 CustodyCodec::Reader r{b};auto h=std::make_shared<HouseBinding::Binding>();h->house_id=r.N(8);h->revision=r.N(8);h->item_template_id=r.N(4);h->location={HouseBinding::Location::Kind::Custody,0,0,r.N(8)};
 HouseBinding::Interior i;i.native_estate_id=r.N(4);i.definition_id=r.N(4);i.zone_id=r.N(4);i.instance_id=r.N(2);i.zone_version=r.N(1);i.catalog_key=r.Text(96);i.catalog_revision=r.N(8);h->interior=std::move(i);
 if(r.at!=b.size()||!Valid(*h)||Encode(*h)!=b)throw std::invalid_argument("Invalid layout house bytes");return h;
}
inline bool Same(const Identity& a,const Identity& b){return HousePresentation::SameBinding(a,b);}
inline bool Shape(const HouseBinding::Binding& h,const LiveItems::Stored& s){
 if(!Valid(h)||!s.placement||(s.stowed&&!s.storage_revision)||h.location.custody_id!=s.custody_id||h.item_template_id!=s.row.values.item_id)return false;
 bool recognized=false;for(const auto& record:HouseGeometry::Records)recognized|=record.item_id==h.item_template_id;
 const auto& r=s.placement->reservation;return recognized&&r.content_binding&&r.content_binding->version==ContentBinding::HouseVersion&&
  h.interior->catalog_key==r.catalog_key&&h.interior->catalog_revision==s.current_catalog_revision;
}
inline bool Matches(const Identity& h,const EQ::ItemInstance* item){
 return !h||(Valid(*h)&&item&&!item->IsStackable()&&!item->GetHousingCrateID()&&HousePlacement::Model(*item)&&
  item->GetID()==h->item_template_id&&item->GetUnscaledItem()->HousingDefinitionID==h->interior->definition_id&&item->GetHousingHouseID()==h->house_id&&item->GetHousingEstateID()==h->interior->native_estate_id);
}
// Structural custody/route snapshot, not scene or mutation authority. The
// executor obtains fresh authorized scenes and checks MoveItem separately.
inline LiveItems::Result<Identity> ReadUnderTransaction(Database& db,const LiveItems::Stored& s){
 using Error=LiveItems::Error;try{
  auto q=db.QueryDatabase("SELECT h.house_id,h.revision,h.item_template_id,h.created_by_character_id,h.pending_request_key,l.location_kind,l.character_id,l.slot_id,l.custody_id,i.native_estate_id,i.definition_id,i.zone_id,i.instance_id,i.zone_version,i.catalog_key,i.catalog_revision,n.parent_native_id,n.definition_id,n.active,n.catalog_key,v.zone,v.version,v.never_expires FROM housing_house_locations l JOIN housing_house_instances h ON h.house_id=l.house_id JOIN housing_house_interiors i ON i.house_id=h.house_id JOIN housing_native_estates n ON n.native_id=i.native_estate_id JOIN instance_list v ON v.id=i.instance_id WHERE l.custody_id="+std::to_string(s.custody_id)+" FOR UPDATE",false);
  if(!q.Success())return {{},Error::DatabaseFailure};if(q.RowCount()!=1)return {{},Error::Conflict};auto r=q.begin();
  auto n=[&](size_t at){if(!r[at])throw std::invalid_argument("Missing house fact");size_t end=0;const auto v=std::stoull(r[at],&end);if(end!=std::strlen(r[at]))throw std::invalid_argument("Invalid house number");return v;};
  if(!s.placement||n(2)!=s.row.values.item_id||n(3)!=s.row.values.character_id||r[4]||n(5)!=2||r[6]||r[7]||n(8)!=s.custody_id||n(12)>32000||n(13)>255||n(16)!=s.placement->reservation.native_estate_id||n(17)!=n(10)||n(18)!=1||!r[14]||!r[19]||std::string(r[14])!=r[19]||n(20)!=n(11)||n(21)!=n(13)||n(22)!=1)return {{},Error::Conflict};
  auto h=std::make_shared<HouseBinding::Binding>();h->house_id=n(0);h->revision=n(1);h->item_template_id=n(2);h->location={HouseBinding::Location::Kind::Custody,0,0,n(8)};h->interior=HouseBinding::Interior{uint32_t(n(9)),uint32_t(n(10)),uint32_t(n(11)),uint16_t(n(12)),uint8_t(n(13)),r[14],n(15)};
  if(!Shape(*h,s))return {{},Error::Conflict};return {std::move(h),Error::None};
 }catch(const std::invalid_argument&){return {{},Error::Conflict};}catch(...){return {{},Error::DatabaseFailure};}
}
}
#endif
