#ifndef EQEMU_ZONE_HOUSING_TROPHY_STAT_ITEM_H
#define EQEMU_ZONE_HOUSING_TROPHY_STAT_ITEM_H
#include "client.h"
#include "../common/housing/trophy_stat_item.h"
namespace Housing::TrophyStatItem {
template<size_t N> void Send(Client& c,const std::array<uint32_t,N>& slots,bool guild){
 if(c.ClientVersion()!=EQ::versions::ClientVersion::RoF2)return;
 // Both original state snapshots clear their domain's item container. Always
 // resend current grants AFTER the snapshot, even when IDs have not changed.
 for(size_t i=0;i<N;++i)if(slots[i]){
  const auto* item=database.CreateItem(slots[i],1);
  if(item){c.SendItemPacket(int16(i),item,guild?GuildPacket:PersonalPacket);safe_delete(item);}
 }
}
}
#endif
