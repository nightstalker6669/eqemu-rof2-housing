#ifndef EQEMU_HOUSING_NPC_RUNTIME_DB_H
#define EQEMU_HOUSING_NPC_RUNTIME_DB_H
#include "npc_runtime.h"
#include "npc_item_metadata_db.h"
#include "crate_member_scope_db.h"
#include "custody_codec.h"
namespace Housing::NpcRuntime {
inline std::optional<Profile> Current(Database& db,uint32_t item){auto source=NpcItemMetadata::ReadUnderTransaction(db,item);return source?Review(*source):std::nullopt;}
inline bool Capacity(uint32_t maximum,uint64_t current){return int32_t(maximum)<0||current<maximum;} // Original5abc60 signed sentinel.
inline uint64_t Count(Database& db,uint32_t native,uint64_t house=0){
 auto tx=NpcItemMetadata::Query(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||NpcItemMetadata::Number(tx.begin()[0])!=1)throw std::runtime_error("NPC count requires owning transaction");
 std::string sql=house?"SELECT HEX(c.row_blob),c.owner_character_id,c.estate_id,0 FROM housing_house_item_scopes p JOIN housing_live_item_custody c ON c.custody_id=p.custody_id WHERE p.house_id="+std::to_string(house):"SELECT HEX(c.row_blob),c.owner_character_id,c.estate_id,r.content_binding_version FROM housing_live_placed_items p JOIN housing_live_item_custody c ON c.custody_id=p.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE r.native_estate_id="+std::to_string(native)+" AND r.state=1";
 if(CrateMemberScope::Installed(db))sql+=" AND NOT EXISTS (SELECT 1 FROM housing_crate_members packed WHERE packed.custody_id=p.custody_id)";
 sql+=CrateMemberScope::VaultPredicate(db,"p");
 auto rows=NpcItemMetadata::Query(db,sql+" ORDER BY p.custody_id LIMIT 16385 FOR UPDATE");if(rows.RowCount()>16384)throw std::runtime_error("NPC count envelope");uint64_t count=0;
 for(auto r=rows.begin();r.operator!=(rows.end());++r){
  if(!r[0])throw std::runtime_error("NPC count raw custody missing");std::string hex=r[0];if(hex.size()%2||hex.size()>2*CustodyCodec::MaximumPayload)throw std::runtime_error("NPC count custody framing");
  std::vector<uint8_t> bytes;auto digit=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw std::runtime_error("NPC count custody hex");};for(size_t i=0;i<hex.size();i+=2)bytes.push_back((digit(hex[i])<<4)|digit(hex[i+1]));
  auto row=CustodyCodec::Decode(bytes);if((row.null_fields&LiveItems::NullItemId)||!row.values.item_id||!row.values.character_id||!r[1]||!r[2]||NpcItemMetadata::Number(r[1])!=row.values.character_id||!NpcItemMetadata::Number(r[2]))throw std::runtime_error("NPC count custody identity mismatch");auto source=NpcItemMetadata::ReadUnderTransaction(db,row.values.item_id);if(r[3]&&(NpcItemMetadata::Number(r[3])==ContentBinding::NpcClassVersion||NpcItemMetadata::Number(r[3])==ContentBinding::GenericNpcClassVersion)&&(!source||!Review(*source)))throw std::runtime_error("NPC count source changed under retained class");if(source&&!source->filename.empty()&&!source->interactive)++count;
 }
 return count;
}
}
#endif
