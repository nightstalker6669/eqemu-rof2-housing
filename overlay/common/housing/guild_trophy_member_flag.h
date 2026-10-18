#ifndef EQEMU_HOUSING_GUILD_TROPHY_MEMBER_FLAG_H
#define EQEMU_HOUSING_GUILD_TROPHY_MEMBER_FLAG_H
#include "../database.h"
#include "../rulesys.h"
#include <string>
namespace Housing {
// Display state for an already-authorized ordinary tribute notification. This
// confers no membership or trophy authority and never changes either toggle.
inline uint32_t GuildTrophyMemberFlag(Database& db,uint32_t guild,uint32_t character,const char* name=nullptr){
 if(!RuleB(Housing,EnablePlayerHousing)||!guild||guild==UINT32_MAX)return 0;
 std::string identity;
 if(character)identity="m.char_id="+std::to_string(character);
 else{
  if(!name)return 0;size_t length=0;while(length<64&&name[length])++length;
  if(!length||length==64)return 0;
  identity="c.name='"+db.Escape(std::string(name,length))+"'";
 }
 auto rows=db.QueryDatabase("SELECT m.trophy_enable FROM guild_members m JOIN character_data c ON c.id=m.char_id WHERE c.deleted_at IS NULL AND m.guild_id="+std::to_string(guild)+" AND "+identity,false);
 if(!rows.Success()||rows.RowCount()!=1||!rows.begin()[0])return 0;
 return std::string(rows.begin()[0])=="1"?1:0;
}
}
#endif
