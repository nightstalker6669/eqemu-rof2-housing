#include "current_estate_access.h"
#include "rent_state_db.h"
#include "../database.h"
#include <stdexcept>
namespace Housing {
namespace {
struct Failure {Error error;};
MySQLRequestResult Query(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw Failure{Error::DatabaseFailure};return r;}
uint32_t Number(const char* text){if(!text)throw Failure{Error::DatabaseFailure};auto n=std::stoull(text);if(n>UINT32_MAX)throw Failure{Error::DatabaseFailure};return uint32_t(n);}
void Character(Database& db,uint32_t id){auto r=Query(db,"SELECT c.id FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(id)+" AND c.deleted_at IS NULL FOR UPDATE");if(r.RowCount()!=1)throw Failure{Error::Unauthorized};}
AccessPolicy::Acl Acl(Database& db,EstateId estate,uint64_t& revision){
 AccessPolicy::Acl acl;auto id=std::to_string(estate);
 auto state=Query(db,"SELECT revision FROM housing_acl_state WHERE estate_id="+id+" FOR UPDATE");
 if(!state.RowCount())return acl;
 if(state.RowCount()!=1||!state.begin()[0]||!(revision=std::stoull(state.begin()[0])))throw Failure{Error::DatabaseFailure};
 auto groups=Query(db,"SELECT group_id,capabilities FROM housing_acl_groups WHERE estate_id="+id+" ORDER BY group_id FOR UPDATE");if(groups.RowCount()!=7)throw Failure{Error::DatabaseFailure};uint32_t i=0;
 for(auto r=groups.begin();r.operator!=(groups.end());++r){if(Number(r[0])!=i)throw Failure{Error::DatabaseFailure};acl.masks[i++]=Number(r[1]);}
 auto members=Query(db,"SELECT character_id,group_id FROM housing_acl_members WHERE estate_id="+id+" ORDER BY character_id FOR UPDATE");if(members.RowCount()>1000)throw Failure{Error::DatabaseFailure};
 for(auto r=members.begin();r.operator!=(members.end());++r){auto member=Number(r[0]),group=Number(r[1]);Character(db,member);if(group>5)throw Failure{Error::DatabaseFailure};acl.assignments.push_back({{AccessPolicy::SubjectKind::Character,member},uint8_t(group)});}
 auto special=Query(db,"SELECT subject_kind,subject_id,group_id FROM housing_acl_special_members WHERE estate_id="+id+" ORDER BY subject_kind,subject_id FOR UPDATE");if(special.RowCount()>9)throw Failure{Error::DatabaseFailure};
 for(auto r=special.begin();r.operator!=(special.end());++r){auto kind=Number(r[0]),group=Number(r[2]);if((kind!=1&&kind!=2)||group>5)throw Failure{Error::DatabaseFailure};acl.assignments.push_back({{kind==1?AccessPolicy::SubjectKind::GuildRank:AccessPolicy::SubjectKind::Fellowship,Number(r[1])},uint8_t(group)});}
 if(!AccessPolicy::ValidAcl(acl))throw Failure{Error::DatabaseFailure};return acl;
}
}
static Result<CurrentEstateAccess> LoadEstateAccessCoreUnderTransaction(Database& db,EstateId estate,uint32_t native,AuthenticatedPurchaser actor){
 if(!estate||!AccessPolicy::PositiveIdentity(native)||!actor.character_id||!actor.account_id)return {{},Error::InvalidArgument};
 if(db.GetStatus()!=DBcore::Connected)return {{},Error::DatabaseFailure};
 try{
  auto transaction=Query(db,"SELECT @@in_transaction");if(transaction.RowCount()!=1||Number(transaction.begin()[0])!=1)throw Failure{Error::InvalidArgument};
  auto engines=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_estates','housing_native_estates','housing_acl_state','housing_acl_groups','housing_acl_members','housing_acl_special_members','character_data','account','guild_members','guilds')");if(engines.RowCount()!=1||Number(engines.begin()[0])!=10)throw Failure{Error::DatabaseFailure};
  auto domain=Query(db,"SELECT owner_kind,owner_id,plot_id FROM housing_current_estates WHERE id="+std::to_string(estate)+" FOR UPDATE");if(domain.RowCount()!=1)throw Failure{Error::NotFound};auto d=domain.begin();auto kind=Number(d[0]),owner=Number(d[1]),plot=Number(d[2]);if((kind!=1&&kind!=3)||!AccessPolicy::PositiveIdentity(owner)||!plot)throw Failure{Error::Unauthorized};
  auto mapping=Query(db,"SELECT native_id FROM housing_native_estates WHERE native_id="+std::to_string(native)+" AND ownership_estate_id="+std::to_string(estate)+" AND plot_id="+std::to_string(plot)+" AND active=1 FOR UPDATE");if(mapping.RowCount()!=1)throw Failure{Error::Unauthorized};
  auto character=Query(db,"SELECT c.account_id,c.deleted_at,a.id FROM character_data c LEFT JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(actor.character_id)+" FOR UPDATE");if(character.RowCount()!=1||Number(character.begin()[0])!=actor.account_id||character.begin()[1]||!character.begin()[2])throw Failure{Error::Unauthorized};
  AccessPolicy::TrustedActor trusted;trusted.character_id=actor.character_id;trusted.authenticated=trusted.feature_allowed=true;
  auto membership=Query(db,"SELECT guild_id,`rank` FROM guild_members WHERE char_id="+std::to_string(actor.character_id)+" FOR UPDATE");if(membership.RowCount()>1)throw Failure{Error::DatabaseFailure};
  if(membership.RowCount()){trusted.guild_id=Number(membership.begin()[0]);auto rank=AccessPolicy::NativeGuildRankFromServer(Number(membership.begin()[1]));if(!rank||!AccessPolicy::PositiveIdentity(trusted.guild_id))throw Failure{Error::Unauthorized};trusted.guild_rank=*rank;auto guild=Query(db,"SELECT leader FROM guilds WHERE id="+std::to_string(trusted.guild_id)+" FOR UPDATE");if(guild.RowCount()!=1||(trusted.guild_rank==1&&Number(guild.begin()[0])!=actor.character_id))throw Failure{Error::Unauthorized};}
  AccessPolicy::ResolvedPlot resolved;resolved.estate_id=native;resolved.type=kind==1?AccessPolicy::PlotType::Personal:AccessPolicy::PlotType::Guild;
  if(kind==1){resolved.owner_character_id=owner;Character(db,owner);}else{resolved.guild_id=owner;auto guild=Query(db,"SELECT id FROM guilds WHERE id="+std::to_string(owner)+" FOR UPDATE");if(guild.RowCount()!=1)throw Failure{Error::Unauthorized};}
  CurrentEstateAccess out;out.owner_kind=OwnerKind(kind);out.owner_id=owner;out.plot_id=plot;auto acl=Acl(db,estate,out.acl_revision);auto access=AccessPolicy::Resolve(resolved,trusted,&acl);if(!access)throw Failure{Error::Unauthorized};out.effective=*access;RentState::Restrict(db,estate,native,out.effective);return {out,Error::None};
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}
Result<CurrentEstateAccess> LoadCurrentEstateAccessUnderTransaction(Database& db,EstateId estate,uint32_t native,AuthenticatedPurchaser actor){return LoadEstateAccessCoreUnderTransaction(db,estate,native,actor);}
Result<CurrentEstateAccess> LoadGuildTrophyRecipientAccessUnderTransaction(Database& db,EstateId estate,uint32_t native,AuthenticatedPurchaser actor){auto result=LoadEstateAccessCoreUnderTransaction(db,estate,native,actor);if(!result)return result;if(uint32_t(result.value.owner_kind)!=3)return {{},Error::Unauthorized};try{if(!RentState::PlotActive(db,estate,native))return {{},Error::Unauthorized};auto member=Query(db,"SELECT guild_id FROM guild_members WHERE char_id="+std::to_string(actor.character_id)+" FOR UPDATE");if(member.RowCount()!=1||Number(member.begin()[0])!=result.value.owner_id)return {{},Error::Unauthorized};return result;}catch(...){return {{},Error::DatabaseFailure};}}

}
