#ifndef EQEMU_HOUSING_GUILD_PORTAL_DB_H
#define EQEMU_HOUSING_GUILD_PORTAL_DB_H
#include "guild_portal_service.h"
#include "house_item_scope_db.h"
#include <limits>
namespace Housing::GuildPortal {
inline uint64_t Number(const char* text){if(!text||!*text)throw std::invalid_argument("Missing guild portal number");std::string s(text);if(s[0]<'0'||s[0]>'9')throw std::invalid_argument("Invalid guild portal number");size_t used=0;auto value=std::stoull(s,&used);if(used!=s.size())throw std::invalid_argument("Invalid guild portal number");return value;}
inline uint32_t Number32(const char* text){auto n=Number(text);if(n>UINT32_MAX)throw std::invalid_argument("Guild portal number overflow");return uint32_t(n);}
inline MySQLRequestResult Query(Database& db,const std::string& sql){return HouseItemScope::Query(db,sql);}
// A scope proves a currently placed supported guild hall and actual owning
// estate. Caller additionally proves entry/interaction permission and exact
// committed service item/pad identity; neither client GuildID nor a native
// estate ID supplied by a packet may manufacture this scope.
inline std::optional<Scope> Current(Database& db,uint32_t zone,uint32_t instance,bool lock=true){
 if(!zone||!instance||instance>32000)return {};
 auto rows=Query(db,"SELECT h.house_id,e.id,e.owner_id FROM housing_house_interiors i JOIN housing_house_instances h ON h.house_id=i.house_id JOIN housing_house_locations l ON l.house_id=h.house_id AND l.location_kind=2 JOIN housing_live_item_custody c ON c.custody_id=l.custody_id JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_estates e ON e.id=c.estate_id AND e.active=1 AND e.owner_kind=3 WHERE h.item_template_id IN (66618,66619,66620,66660) AND i.zone_id="+std::to_string(zone)+" AND i.instance_id="+std::to_string(instance)+(lock?" FOR UPDATE":""));
 if(!rows.RowCount())return {};if(rows.RowCount()!=1)throw std::invalid_argument("Ambiguous guild portal scope");auto r=rows.begin();Scope s{Number(r[0]),Number32(r[1]),Number32(r[2]),zone,instance};if(!Valid(s))throw std::invalid_argument("Invalid guild portal scope");return s;
}
inline uint64_t DatabaseNow(Database& db){auto row=Query(db,"SELECT UNIX_TIMESTAMP()");if(row.RowCount()!=1)throw std::invalid_argument("Missing guild portal clock");return Number(row.begin()[0]);}
inline bool SameCurrent(Database& db,const Scope& s,bool lock=true){if(!Valid(s))return false;auto current=Current(db,s.zone,s.instance,lock);return current&&Same(*current,s);}
inline std::optional<Alignment> Read(Database& db,const Scope& s,bool lock=false){
 if(!SameCurrent(db,s,lock))return {};
 auto rows=Query(db,"SELECT estate_id,guild_id,zone_id,instance_id,stone_item_id,expires_epoch,revision FROM housing_guild_portal_alignments WHERE house_id="+std::to_string(s.house)+(lock?" FOR UPDATE":""));
 if(!rows.RowCount())return {};if(rows.RowCount()!=1)throw std::invalid_argument("Ambiguous guild portal alignment");auto r=rows.begin();Alignment a{{s.house,Number32(r[0]),Number32(r[1]),Number32(r[2]),Number32(r[3])},Number32(r[4]),Number(r[5]),Number(r[6])};if(!a.revision||!a.expires||!Lookup(a.stone))throw std::invalid_argument("Invalid persisted guild portal destination");if(!Same(a.scope,s))return {};return a;
}
// Caller MUST own the same InnoDB transaction used to durably remove exactly
// one paid stone from a verified hand-in. No money is taken here; stones use
// the ordinary merchant price/stock system. Commit both effects together, and
// return all rejected stones through the ordinary trade return path.
inline Alignment AlignLocked(Database& db,const Scope& s,uint32_t stone,uint64_t now){
 if(!Purchasable(stone)||!now||now>UINT64_MAX-AlignmentSeconds||!SameCurrent(db,s,true))throw std::invalid_argument("Invalid guild portal activation");
 auto old=Query(db,"SELECT revision FROM housing_guild_portal_alignments WHERE house_id="+std::to_string(s.house)+" FOR UPDATE");if(old.RowCount()>1||(old.RowCount()&&(!Number(old.begin()[0])||Number(old.begin()[0])==UINT64_MAX)))throw std::invalid_argument("Guild portal revision exhausted");
 const uint64_t revision=old.RowCount()?Number(old.begin()[0])+1:1;const auto expires=now+AlignmentSeconds;
 Query(db,"INSERT INTO housing_guild_portal_alignments (house_id,estate_id,guild_id,zone_id,instance_id,stone_item_id,expires_epoch,revision) VALUES ("+std::to_string(s.house)+","+std::to_string(s.estate)+","+std::to_string(s.guild)+","+std::to_string(s.zone)+","+std::to_string(s.instance)+","+std::to_string(stone)+","+std::to_string(expires)+","+std::to_string(revision)+") ON DUPLICATE KEY UPDATE estate_id=VALUES(estate_id),guild_id=VALUES(guild_id),zone_id=VALUES(zone_id),instance_id=VALUES(instance_id),stone_item_id=VALUES(stone_item_id),expires_epoch=VALUES(expires_epoch),revision=VALUES(revision)");
 return {s,stone,expires,revision};
}
}
#endif
