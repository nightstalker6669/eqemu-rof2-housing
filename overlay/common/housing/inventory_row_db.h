#ifndef EQEMU_HOUSING_INVENTORY_ROW_DB_H
#define EQEMU_HOUSING_INVENTORY_ROW_DB_H
#include "custody_codec.h"
#include "../mysql_request_row.h"
namespace Housing::InventoryRowDB {
inline constexpr const char* Columns="character_id,slot_id,item_id,charges,color,augment_one,augment_two,augment_three,augment_four,augment_five,augment_six,instnodrop,HEX(custom_data),ornament_icon,ornament_idfile,ornament_hero_model,guid";
inline constexpr const char* WriteColumns="character_id,slot_id,item_id,charges,color,augment_one,augment_two,augment_three,augment_four,augment_five,augment_six,instnodrop,custom_data,ornament_icon,ornament_idfile,ornament_hero_model,guid";
// Canonical SQL writer shared by online custody and trusted offline delivery.
// Preserve NULL flags, embedded custom-data bytes and all ornament/GUID fields.
inline std::string Values(const LiveItems::InventoryRow& r,uint32_t slot){
 using namespace LiveItems;const auto& v=r.values;
 auto hex=[](const std::string& raw){const char* h="0123456789ABCDEF";std::string out;for(unsigned char c:raw){out+=h[c>>4];out+=h[c&15];}return out;};
 std::string s=std::to_string(v.character_id)+","+std::to_string(slot)+","+((r.null_fields&NullItemId)?"NULL":std::to_string(v.item_id))+","+((r.null_fields&NullCharges)?"NULL":std::to_string(v.charges));
 for(auto n:{v.color,v.augment_one,v.augment_two,v.augment_three,v.augment_four,v.augment_five,v.augment_six})s+=","+std::to_string(n);
 s+=","+std::to_string(v.instnodrop)+","+((r.null_fields&NullCustomData)?"NULL":"x'"+hex(v.custom_data)+"'")+","+std::to_string(v.ornament_icon)+","+std::to_string(v.ornament_idfile)+","+std::to_string(v.ornament_hero_model)+","+((r.null_fields&NullGuid)?"NULL":std::to_string(v.guid));return s;
}
inline LiveItems::InventoryRow Decode(MySQLRequestRow row){
 LiveItems::InventoryRow out;auto number=[&](int n,uint64_t max)->uint64_t{if(!row[n])return uint64_t(0);size_t end=0;std::string s=row[n];auto v=std::stoull(s,&end);if(end!=s.size()||v>max)throw std::invalid_argument("Inventory row field range");return v;};auto& v=out.values;
 v.character_id=number(0,UINT32_MAX);v.slot_id=number(1,UINT32_MAX);v.item_id=number(2,UINT32_MAX);v.charges=number(3,UINT16_MAX);v.color=number(4,UINT32_MAX);
 v.augment_one=number(5,UINT32_MAX);v.augment_two=number(6,UINT32_MAX);v.augment_three=number(7,UINT32_MAX);v.augment_four=number(8,UINT32_MAX);v.augment_five=number(9,UINT32_MAX);v.augment_six=number(10,UINT32_MAX);v.instnodrop=number(11,UINT8_MAX);
 if(row[12]){std::string s=row[12];if(s.size()>131070||s.size()%2)throw std::invalid_argument("Inventory custom data bound");auto nibble=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;throw std::invalid_argument("Inventory custom data hex");};for(size_t i=0;i<s.size();i+=2)v.custom_data+=char((nibble(s[i])<<4)|nibble(s[i+1]));}
 v.ornament_icon=number(13,UINT32_MAX);v.ornament_idfile=number(14,UINT32_MAX);
 if(!row[15])throw std::invalid_argument("Missing inventory hero field");size_t end=0;std::string hero=row[15];auto h=std::stoll(hero,&end);if(end!=hero.size()||h<INT32_MIN||h>INT32_MAX)throw std::invalid_argument("Inventory hero range");v.ornament_hero_model=h;v.guid=number(16,UINT64_MAX);
 if(!row[2])out.null_fields|=LiveItems::NullItemId;if(!row[3])out.null_fields|=LiveItems::NullCharges;if(!row[12])out.null_fields|=LiveItems::NullCustomData;if(!row[16])out.null_fields|=LiveItems::NullGuid;return out;
}
}
#endif
