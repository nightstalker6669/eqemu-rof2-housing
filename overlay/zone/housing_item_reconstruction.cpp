#include "housing_item_reconstruction.h"
#include "../common/housing/inventory_projection.h"
#include <map>
#include <optional>
#include "../common/strings.h"
namespace Housing::InventoryBridge {
using InventoryProjection::Shape;
using InventoryProjection::Custom;
using InventoryProjection::Charges;
bool MatchesProjection(const LiveItems::InventoryRow& row,EQ::ItemInstance& item){return InventoryProjection::Matches(row,item);}
LiveItems::Result<ReconstructedItem> Reconstruct(SharedDatabase& db,const LiveItems::InventoryRow& row){
 try{
  if(!Shape(row))return {{},LiveItems::Error::UnsupportedItem};auto fields=Custom(row.values.custom_data);if(!fields)return {{},LiveItems::Error::UnsupportedItem};
  const auto* data=db.GetItem(row.values.item_id);
  if(!data||((data->ItemClass!=0||data->BagSlots)&&!InventoryProjection::VaultCandidate(*data)))return {{},LiveItems::Error::UnsupportedItem};
  // Reserve legacy GUID before allocating a fresh native serial; GUID may be
  // NULL,0 or wider than signed32 and remains verbatim in the original row.
  if(!(row.null_fields&LiveItems::NullGuid)&&row.values.guid)EQ::ItemInstance::AddGUIDToMap(row.values.guid);
  std::unique_ptr<EQ::ItemInstance> item(db.CreateItem(data,Charges(row,*data),row.values.augment_one,row.values.augment_two,row.values.augment_three,row.values.augment_four,row.values.augment_five,row.values.augment_six,bool(row.values.instnodrop),"",row.values.ornament_icon,row.values.ornament_idfile,uint32_t(row.values.ornament_hero_model)));
  if(!item)return {{},LiveItems::Error::UnsupportedItem};
  // CreateBaseItem normalizes stack charges; restore exactly the legacy load
  // projection after creation. Never silently normalize the stored row.
  item->SetCharges(Charges(row,*data));item->SetColor(InventoryProjection::Color(row,*data));
  for(const auto& [key,value]:*fields)item->SetCustomData(key,value);
  if(!LegacyScaling::Restore(*item,row.values.custom_data))return {{},LiveItems::Error::UnsupportedItem};
  if(!EvolvingItemCustody::Restore(row.evolving_payload,*item,row.values.character_id))return {{},LiveItems::Error::UnsupportedItem};
  for(const auto& child:row.children){auto rebuilt=Reconstruct(db,child);if(!rebuilt)return {{},rebuilt.error};item->PutItem(uint8_t(child.values.slot_id),*rebuilt.value.item);}
  if(!MatchesProjection(row,*item))return {{},LiveItems::Error::UnsupportedItem};
  ReconstructedItem output;output.original=row;output.runtime_serial=item->GetSerialNumber();output.item=std::move(item);
  LiveItems::Result<ReconstructedItem> result;result.value=std::move(output);return result;
 }catch(...){return {{},LiveItems::Error::UnsupportedItem};}
}
}
