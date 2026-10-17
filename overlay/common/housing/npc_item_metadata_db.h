#ifndef EQEMU_HOUSING_NPC_ITEM_METADATA_DB_H
#define EQEMU_HOUSING_NPC_ITEM_METADATA_DB_H
#include "npc_item_metadata.h"
#include "../database.h"
#include <optional>
namespace Housing::NpcItemMetadata {
inline MySQLRequestResult Query(Database& db,const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())throw std::runtime_error("NPC item metadata query failed");return result;}
inline uint32_t Number(const char* text,uint32_t maximum=UINT32_MAX){if(!text||!*text||std::string(text).find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid NPC item metadata number");auto n=std::stoull(text);if(n>maximum)throw std::runtime_error("Invalid NPC item metadata range");return uint32_t(n);}
inline bool Installed(Database& db){auto rows=Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_native_npc_item_sources'");if(!rows.RowCount())return false;if(rows.RowCount()!=1||!rows.begin()[0]||std::string(rows.begin()[0])!="InnoDB")throw std::runtime_error("Invalid NPC item metadata schema");return true;}
template<class Row> inline Source Decode(Row row){
 Source result;result.item_id=Number(row[0],INT32_MAX);if(!row[1]||!row[4])throw std::runtime_error("Incomplete NPC item metadata source");std::string hex=row[1];if(hex.size()>2*ItemWire::MaximumFilename||hex.size()%2)throw std::runtime_error("Invalid NPC filename width");auto digit=[](char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw std::runtime_error("Invalid NPC filename byte");};for(size_t n=0;n<hex.size();n+=2)result.filename.push_back(char((digit(hex[n])<<4)|digit(hex[n+1])));result.interactive=Number(row[2],1)!=0;result.respawn_interval=Number(row[3]);result.source_sha256=row[4];if(!Valid(result))throw std::runtime_error("Invalid reviewed NPC item metadata");return result;
}
inline bool RuntimeColumn(Database& db){auto rows=Query(db,"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_native_npc_item_sources' AND column_name='actor_height_bits'");if(rows.RowCount()!=1)throw std::runtime_error("NPC runtime column query");return Number(rows.begin()[0],1)==1;}
inline void Runtime(Source& source,const char* value){auto bits=Number(value);std::memcpy(&source.actor_height,&bits,4);if(!Valid(source))throw std::runtime_error("Invalid NPC runtime actor height");}
inline std::map<uint32_t,Source> LoadAll(Database& db){
 std::map<uint32_t,Source> sources;if(!Installed(db))return sources;
 const bool runtime=RuntimeColumn(db);auto rows=Query(db,"SELECT m.item_id,HEX(m.housepet_filename),m.interactive_object,m.npc_respawn_interval,LOWER(HEX(m.source_sha256)),i.id,"+std::string(runtime?"m.actor_height_bits":"0")+" FROM housing_native_npc_item_sources m LEFT JOIN items i ON i.id=m.item_id ORDER BY m.item_id");
 for(auto row=rows.begin();row.operator!=(rows.end());++row){auto source=Decode(row);Runtime(source,row[6]);if(!row[5]||Number(row[5])!=source.item_id||!sources.emplace(source.item_id,std::move(source)).second)throw std::runtime_error("Orphan or duplicate NPC item metadata");}return sources;
}
inline std::optional<Source> ReadUnderTransaction(Database& db,uint32_t item){
 if(!Installed(db))return {};auto transaction=Query(db,"SELECT @@in_transaction");if(transaction.RowCount()!=1||!transaction.begin()[0]||std::string(transaction.begin()[0])!="1")throw std::runtime_error("NPC metadata requires the parent transaction");
 const bool runtime=RuntimeColumn(db);auto rows=Query(db,"SELECT item_id,HEX(housepet_filename),interactive_object,npc_respawn_interval,LOWER(HEX(source_sha256)),"+std::string(runtime?"actor_height_bits":"0")+" FROM housing_native_npc_item_sources WHERE item_id="+std::to_string(item)+" FOR UPDATE");if(!rows.RowCount())return {};if(rows.RowCount()!=1)throw std::runtime_error("Ambiguous NPC item metadata source");auto source=Decode(rows.begin());Runtime(source,rows.begin()[5]);if(source.item_id!=item)throw std::runtime_error("NPC item metadata identity mismatch");return source;
}
inline bool OrdinaryUnderTransaction(Database& db,uint32_t item){auto source=ReadUnderTransaction(db,item);return !source||Ordinary(*source);}
}
#endif
