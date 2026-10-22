#ifndef EQEMU_HOUSING_INVENTORY_PROJECTION_H
#define EQEMU_HOUSING_INVENTORY_PROJECTION_H
#include "custody_codec.h"
#include "evolving_item_custody.h"
#include "legacy_scaling_state.h"
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
  (!(row.null_fields&LiveItems::NullCustomData)||r.custom_data.empty())&&r.custom_data.size()<=65535&&CustodyCodec::Valid(row);
}
inline std::optional<std::map<std::string,std::string>> Custom(const std::string& raw){return LegacyScaling::Fields(raw);}

inline std::string Canonical(const std::map<std::string,std::string>& fields){
 std::string out;for(const auto& [key,value]:fields){if(!out.empty())out+='^';out+=key;out+='^';out+=value;out+='^';}return out;
}
inline int16_t Charges(const LiveItems::InventoryRow& row,const EQ::ItemData& data){
 const auto raw=row.values.charges;if(raw==32767)return -1;
 if(!raw&&data.Stackable)return 1;
 int16_t value;std::memcpy(&value,&raw,2);return value;
}
// Match CreateBaseItem/GetInventory: only common items inherit a definition
// color when the persisted instance color is zero. Bags/books default to zero.
inline uint32_t Color(const LiveItems::InventoryRow& row,const EQ::ItemData& data){return row.values.color?row.values.color:(data.IsClassCommon()?data.Color:0u);}
inline bool VaultCandidate(const EQ::ItemData& data){return !data.Placeable&&!data.NoTransfer&&data.ItemClass<=2&&(data.ItemClass==1||!data.BagSlots);}
inline bool Matches(const LiveItems::InventoryRow& row,EQ::ItemInstance& item){
 const auto& r=row.values;const auto* data=item.GetItem();
 if(!Shape(row)||!data||data->ID!=r.item_id||((data->ItemClass!=0||data->BagSlots)&&!VaultCandidate(*data))||!LegacyScaling::Matches(r.custom_data,item)||item.GetSerialNumber()<=0||!EvolvingItemCustody::Matches(row.evolving_payload,item,r.character_id))return false;
 const std::array<uint32_t,6> augments{r.augment_one,r.augment_two,r.augment_three,r.augment_four,r.augment_five,r.augment_six};
 size_t augment_count=0;for(size_t i=0;i<augments.size();++i){if(data->ItemClass!=EQ::item::ItemClassBag&&item.GetAugmentItemID(uint8_t(i))!=augments[i])return false;if(augments[i])++augment_count;}
 if(data->ItemClass==EQ::item::ItemClassBag){
  if(augment_count||item.GetContents()->size()!=row.children.size()||data->BagSlots>CustodyCodec::MaximumBagSlots)return false;
  for(const auto& child:row.children){auto found=item.GetContents()->find(uint8_t(child.values.slot_id));if(child.values.slot_id>=data->BagSlots||found==item.GetContents()->end()||!found->second||!Matches(child,*found->second))return false;}
 }else if(!row.children.empty()||item.GetContents()->size()!=augment_count)return false;
 auto fields=Custom(r.custom_data);if(!fields)return false;
 return item.GetCharges()==Charges(row,*data)&&item.GetColor()==Color(row,*data)&&item.IsAttuned()==bool(r.instnodrop)&&
  item.GetCustomDataString()==Canonical(*fields)&&item.GetOrnamentationIcon()==r.ornament_icon&&item.GetOrnamentationIDFile()==r.ornament_idfile&&item.GetOrnamentHeroModel()==uint32_t(r.ornament_hero_model);
}
inline bool RestoreEvolution(const LiveItems::InventoryRow& row,EQ::ItemInstance& item){
 if(!EvolvingItemCustody::Restore(row.evolving_payload,item,row.values.character_id))return false;
 for(const auto& child:row.children){auto found=item.GetContents()->find(uint8_t(child.values.slot_id));if(found==item.GetContents()->end()||!found->second||!RestoreEvolution(child,*found->second))return false;}return true;
}

}
#endif
