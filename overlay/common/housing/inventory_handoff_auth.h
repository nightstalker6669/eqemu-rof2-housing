#ifndef EQEMU_HOUSING_INVENTORY_HANDOFF_AUTH_H
#define EQEMU_HOUSING_INVENTORY_HANDOFF_AUTH_H
#include "../database.h"
namespace Housing {
enum class HandoffActorState {Authenticated,Unauthorized,Unavailable};
struct HandoffActor {HandoffActorState state=HandoffActorState::Unavailable;std::string name;};
// Revalidate a server-authenticated queued actor. SQL failure never proves that
// its previous writer has exited, and never changes a durable reservation.
inline HandoffActor CheckHandoffActor(Database& db,uint32_t character,uint32_t account) noexcept {
 if(!character||!account)return {HandoffActorState::Unauthorized,{}};
 try {
  auto rows=db.QueryDatabase("SELECT c.name FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+
   std::to_string(character)+" AND c.account_id="+std::to_string(account)+" AND c.deleted_at IS NULL AND a.status>=0 LIMIT 1",false);
  if(!rows.Success())return {};
  if(rows.RowCount()!=1||!rows.begin()[0])return {HandoffActorState::Unauthorized,{}};
  return {HandoffActorState::Authenticated,rows.begin()[0]};
 }catch(...){return {};}
}
}
#endif
