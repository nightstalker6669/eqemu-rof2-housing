#ifndef EQEMU_HOUSING_GUILD_FAVOR_H
#define EQEMU_HOUSING_GUILD_FAVOR_H
#include "../database.h"
#include <optional>
#include <cstdint>
namespace Housing::GuildFavor {
// One shared guild favor row. Atomic arithmetic replaces cached absolute writes.
// MySQL LAST_INSERT_ID(expr) returns this UPDATE's committed/transactional result
// in its own result metadata, avoiding a second connection-scoped SELECT race.
inline std::optional<uint32_t> Read(Database& db,uint32_t guild){if(!guild||guild==UINT32_MAX)return {};auto r=db.QueryDatabase("SELECT favor FROM guilds WHERE id="+std::to_string(guild),false);if(!r.Success()||r.RowCount()!=1||!r.begin()[0])return {};try{auto v=std::stoull(r.begin()[0]);if(v<=UINT32_MAX)return uint32_t(v);}catch(...){}return {};}
inline std::optional<uint32_t> Adjust(Database& db,uint32_t guild,int64_t delta){
 if(!guild||guild==UINT32_MAX||delta>int64_t(UINT32_MAX)||delta<-int64_t(UINT32_MAX))return {};if(!delta)return Read(db,guild);
 const auto guard=delta>0?"favor<="+std::to_string(uint64_t(UINT32_MAX)-uint64_t(delta)):"favor>="+std::to_string(uint64_t(-delta));
 auto r=db.QueryDatabase("UPDATE guilds SET favor=LAST_INSERT_ID(favor"+std::string(delta>0?"+":"-")+std::to_string(uint64_t(delta>0?delta:-delta))+") WHERE id="+std::to_string(guild)+" AND "+guard,false);
 if(!r.Success()||r.RowsAffected()!=1||r.LastInsertedID()>UINT32_MAX)return {};return uint32_t(r.LastInsertedID());
}
}
#endif
