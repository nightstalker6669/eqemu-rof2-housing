#ifndef EQEMU_HOUSING_STACK_PARTITION_APPLICATION_H
#define EQEMU_HOUSING_STACK_PARTITION_APPLICATION_H
#include "inventory_projection.h"
#include "custody_codec.h"
#include "npc_item_metadata.h"
#include "crate_stack_partition.h"
#include "../inventory_profile.h"
#include <map>
namespace Housing::StackPartitionApplication {
using Owners=std::map<uint64_t,std::unique_ptr<EQ::ItemInstance>>;
using Pieces=std::map<std::string,std::unique_ptr<EQ::ItemInstance>>;
inline uint32_t Requested(const EQ::ItemInstance& item,uint32_t requested){
 const auto* data=item.GetItem();if(!data)throw std::invalid_argument("Missing quantity source");const auto count=data->Stackable?uint32_t(std::max<int32_t>(1,item.GetCharges())):1u;
 if(count>=32767||requested>count)throw std::invalid_argument("Invalid housing quantity");const auto part=requested&&requested<count?requested:0;
 if(part&&(!NpcItemMetadata::Ordinary(*data)||!data->Stackable||data->StackSize<count||data->HousingDefinitionID||item.GetHousingHouseID()||item.GetHousingCrateID()||item.GetHousingEstateID()!=UINT32_MAX||item.IsScaling()))throw std::invalid_argument("Unsupported housing partition");return part;
}
inline std::unique_ptr<EQ::ItemInstance> Prepare(const LiveItems::InventoryRow& before,EQ::ItemInstance& source,uint32_t quantity){
 if(!quantity||Requested(source,quantity)!=quantity||!InventoryProjection::Matches(before,source))return {};
 EQ::ItemInstance::AddGUIDToMap(source.GetSerialNumber());if(!(before.null_fields&LiveItems::NullGuid))EQ::ItemInstance::AddGUIDToMap(before.values.guid);
 auto item=std::unique_ptr<EQ::ItemInstance>(source.CloneWithNewSerial());if(!item||item->GetSerialNumber()<=0||item->GetSerialNumber()==source.GetSerialNumber()||uint64_t(item->GetSerialNumber())==before.values.guid)return {};item->SetCharges(quantity);return item;
}
inline bool Coherent(const LiveItems::Receipt& r){
 if(!r.partition||!CrateStackPartition::Coherent(r))return false;const auto& p=*r.partition;const auto count=p.before.values.charges;if((p.before.null_fields&LiveItems::NullCharges)||!count||count>=32767||!p.quantity||p.quantity>=count||!p.serial||p.serial>INT32_MAX||p.serial==p.before.values.guid||p.maximum<count||p.maximum>=32767)return false;
 if(!p.vault_source&&p.vault_scope_revision)return false;if(p.room_source&&p.vault_source)return false;if(bool(r.room)!=bool(p.room_source))return false;if(p.room_source&&(!r.stored.placement||p.room_source->house_id!=r.room->house_id||p.room_source->house_revision!=r.room->house_revision||p.room_source->profile_revision!=r.room->profile_revision||!p.room_source->scope_revision))return false;if(p.vault_source&&(r.stored.placement||r.room||p.stowed||bool(p.vault_source->house_id)!=bool(p.vault_scope_revision)||!p.vault_source->native_estate_id||!p.vault_source->catalog_revision||(r.action==LiveItems::Action::Return&&p.vault_source->expected_revision!=p.storage_before)))return false;
 auto after=p.before;after.values.charges=count-p.quantity;after.null_fields&=~LiveItems::NullCharges;if(CustodyCodec::Encode(after)!=CustodyCodec::Encode(p.after))return false;auto piece=p.before;piece.values.charges=p.quantity;piece.values.guid=p.serial;piece.null_fields&=~uint8_t(LiveItems::NullCharges|LiveItems::NullGuid);
 if(r.action==LiveItems::Action::Return){piece.values.character_id=r.inventory_after.values.character_id;piece.values.slot_id=r.inventory_after.values.slot_id;if(CustodyCodec::Encode(r.stored.row)!=CustodyCodec::Encode(p.before)||p.storage_before==UINT64_MAX||p.storage_after!=p.storage_before+1)return false;}
 else if(r.action!=LiveItems::Action::Store||p.storage_before||p.storage_after||p.stowed)return false;return CustodyCodec::Encode(piece)==CustodyCodec::Encode(r.action==LiveItems::Action::Store?r.stored.row:r.inventory_after);
}
inline bool Applied(const LiveItems::Receipt& r,EQ::InventoryProfile& inv,const Owners& owners){
 if(!Coherent(r))return false;const auto& p=*r.partition;auto retained=owners.find(r.stored.custody_id);auto* source=r.action==LiveItems::Action::Store?inv.GetItem(int16_t(p.after.values.slot_id)):(retained!=owners.end()?retained->second.get():nullptr);auto* piece=r.action==LiveItems::Action::Store?(retained!=owners.end()?retained->second.get():nullptr):inv.GetItem(int16_t(r.inventory_after.values.slot_id));return source&&piece&&source!=piece&&InventoryProjection::Matches(p.after,*source)&&InventoryProjection::Matches(r.action==LiveItems::Action::Store?r.stored.row:r.inventory_after,*piece)&&uint32_t(piece->GetSerialNumber())==p.serial;
}
inline bool Apply(const LiveItems::Receipt& r,EQ::InventoryProfile& inv,Owners& owners,Pieces& pieces){
 if(Applied(r,inv,owners))return true;if(!Coherent(r))return false;const auto& p=*r.partition;
 auto retained=owners.find(r.stored.custody_id);auto* source=r.action==LiveItems::Action::Store?inv.GetItem(int16_t(p.before.values.slot_id)):(retained!=owners.end()?retained->second.get():nullptr);auto unit=pieces.find(r.request_key);
 if(!source||unit==pieces.end()||!unit->second||(r.action==LiveItems::Action::Store&&retained!=owners.end()&&retained->second)||(!InventoryProjection::Matches(p.before,*source)&&!InventoryProjection::Matches(p.after,*source))||uint32_t(unit->second->GetSerialNumber())!=p.serial||!InventoryProjection::Matches(r.action==LiveItems::Action::Store?r.stored.row:r.inventory_after,*unit->second))return false;
 if(r.action==LiveItems::Action::Return&&(inv.GetItem(int16_t(r.inventory_after.values.slot_id))||(r.inventory_after.values.slot_id==EQ::invslot::slotCursor&&inv.CursorSize())))return false;
 if(r.action==LiveItems::Action::Store&&retained==owners.end())retained=owners.emplace(r.stored.custody_id,nullptr).first;
 source->SetCharges(p.after.values.charges);
 if(r.action==LiveItems::Action::Store)retained->second=std::move(unit->second);
 else{auto* pointer=unit->second.get();if(inv.PushItem(int16_t(r.inventory_after.values.slot_id),pointer)!=int16_t(r.inventory_after.values.slot_id))return false;unit->second.release();}
 pieces.erase(unit);return Applied(r,inv,owners);
}
}
#endif
