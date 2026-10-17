#ifndef EQEMU_HOUSING_INVENTORY_PROJECTION_H
#define EQEMU_HOUSING_INVENTORY_PROJECTION_H
#include "live_item_service.h"
#include "../item_instance.h"
#include "../strings.h"
#include <map>
#include <optional>
namespace Housing::InventoryProjection {
inline bool Shape(const LiveItems::InventoryRow& row){
 const auto& r=row.values;
 return !(row.null_fields&~uint8_t(15))&&!(row.null_fields&LiveItems::NullItemId)&&r.item_id&&
  (!(row.null_fields&LiveItems::NullCharges)||!r.charges)&&
  (!(row.null_fields&LiveItems::NullGuid)||!r.guid)&&
  (!(row.null_fields&LiveItems::NullCustomData)||r.custom_data.empty())&&r.custom_data.size()<=65535&&
  !r.augment_one&&!r.augment_two&&!r.augment_three&&!r.augment_four&&!r.augment_five&&!r.augment_six;
}
inline std::optional<std::map<std::string,std::string>> Custom(const std::string& raw){
 std::map<std::string,std::string> result;if(raw.empty())return result;
 const auto parts=Strings::Split(raw,"^");
 // A final empty token is the writer's optional terminator. No trailing data
 // may be discarded, and duplicate keys cannot preserve all source records.
 size_t count=parts.size();if(count%2){if(!parts.back().empty())return std::nullopt;--count;}
 for(size_t i=0;i<count;i+=2)if(parts[i].empty()||!result.emplace(parts[i],parts[i+1]).second)return std::nullopt;
 return result;
}
inline std::string Canonical(const std::map<std::string,std::string>& fields){
 std::string out;for(const auto& [key,value]:fields){if(!out.empty())out+='^';out+=key;out+='^';out+=value;out+='^';}return out;
}
inline int16_t Charges(const LiveItems::InventoryRow& row,const EQ::ItemData& data){
 const auto raw=row.values.charges;if(raw==32767)return -1;
 if(!raw&&data.Stackable)return 1;
 int16_t value;std::memcpy(&value,&raw,2);return value;
}
inline bool VaultCandidate(const EQ::ItemData& data){return !data.Placeable&&!data.NoTransfer&&data.ItemClass<=2&&(data.ItemClass==1||!data.BagSlots);}
inline bool Matches(const LiveItems::InventoryRow& row,EQ::ItemInstance& item){
 const auto& r=row.values;const auto* data=item.GetItem();
 if(!Shape(row)||!data||data->ID!=r.item_id||((data->ItemClass!=0||data->BagSlots)&&!VaultCandidate(*data))||item.IsEvolving()||!item.GetContents()->empty()||item.IsAugmented()||item.GetSerialNumber()<=0)return false;
 auto fields=Custom(r.custom_data);if(!fields)return false;
 return item.GetCharges()==Charges(row,*data)&&item.GetColor()==(r.color?r.color:data->Color)&&item.IsAttuned()==bool(r.instnodrop)&&
  item.GetCustomDataString()==Canonical(*fields)&&item.GetOrnamentationIcon()==r.ornament_icon&&item.GetOrnamentationIDFile()==r.ornament_idfile&&item.GetOrnamentHeroModel()==uint32_t(r.ornament_hero_model);
}
}
#endif
