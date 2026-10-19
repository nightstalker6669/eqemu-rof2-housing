#ifndef EQEMU_HOUSING_OCCUPIED_RETIREMENT_AUTHORITY_H
#define EQEMU_HOUSING_OCCUPIED_RETIREMENT_AUTHORITY_H
#include "crate_storage_service.h"
#include "crate_binding.h"
#include "live_escrow_service.h"
#include "custody_codec.h"
#include <map>
namespace Housing::OccupiedMoveOut {
struct Request;
LiveItems::Result<LiveItems::Receipt> Execute(Database&,bool,const Request&,LiveItems::InventorySessionFence&,
 CrateBinding::CustodyContext&,HouseBinding::CustodyContext*,const Scene::HouseScenePolicy*,
 const InteriorContent::ProfilePolicy*,LiveEscrow::CoinSessionFence*,std::optional<LiveEscrow::Receipt>*);
}
namespace Housing::OccupiedRetirement {
// A stack-only, caller-owned transaction authority. Only the aggregate Move Out
// may construct it after locking the actual current primary owner/generation.
// It grants neither an ordinary expired mutation nor room access/unpacking.
class Authority final {
 struct Member {std::string key;uint64_t crate_revision,pose_revision,storage_revision;bool vault=false;};
 Database* database_;LiveItems::Lease lease_;std::string key_;uint32_t estate_,native_,destination_;
 uint64_t crate_,custody_,return_revision_,catalog_;Route route_;LiveItems::InventoryRow crate_row_;
 std::map<uint64_t,Member> members_;
 static bool Same(const LiveItems::Lease&a,const LiveItems::Lease&b){return a.actor.character_id==b.actor.character_id&&a.actor.account_id==b.actor.account_id&&a.epoch==b.epoch&&a.session_key==b.session_key&&a.token==b.token;}
 bool Held(Database& db,const LiveItems::Lease& lease,LiveItems::InventorySessionFence& fence)const{
  if(database_!=&db||!Same(lease_,lease)||!fence.HeldFor(lease.actor,lease.epoch,lease.session_key))return false;
  auto tx=db.QueryDatabase(std::string("SELECT @@in_transaction"),false);return tx.Success()&&tx.RowCount()==1&&tx.begin()[0]&&std::string(tx.begin()[0])=="1";
 }
 Authority(Database& db,const LiveItems::Lease& lease,const std::string& key,uint32_t estate,uint32_t native,uint32_t destination,const Scene::Snapshot& scene,const Scene::Item& crate)
 :database_(&db),lease_(lease),key_(key),estate_(estate),native_(native),destination_(destination),crate_(crate.crate->binding.crate_id),custody_(crate.stored.custody_id),return_revision_(crate.crate->binding.revision),catalog_(scene.current_catalog_revision),route_(scene.route),crate_row_(crate.stored.row){
  unsigned index=0;for(const auto& item:scene.items){if(item.stored.custody_id==custody_)continue;
   if(!item.stored.placement||return_revision_==UINT64_MAX)throw std::invalid_argument("Invalid retirement member");
   if(!members_.emplace(item.stored.custody_id,Member{key+":pack"+std::to_string(index++),return_revision_++,item.stored.placement->pose_revision,item.stored.storage_revision}).second)throw std::invalid_argument("Duplicate retirement member");
  }
  for(const auto& item:scene.vault_items){if(!item.revision||return_revision_==UINT64_MAX||!members_.emplace(item.stored.custody_id,Member{key+":pack"+std::to_string(index++),return_revision_++,0,item.revision,true}).second)throw std::invalid_argument("Invalid retirement vault member");}
 }
 friend LiveItems::Result<LiveItems::Receipt> OccupiedMoveOut::Execute(Database&,bool,const OccupiedMoveOut::Request&,LiveItems::InventorySessionFence&,CrateBinding::CustodyContext&,HouseBinding::CustodyContext*,const Scene::HouseScenePolicy*,const InteriorContent::ProfilePolicy*,LiveEscrow::CoinSessionFence*,std::optional<LiveEscrow::Receipt>*);
public:
 Authority(const Authority&)=delete;Authority& operator=(const Authority&)=delete;
 Authority(Authority&&)=delete;Authority& operator=(Authority&&)=delete;
 bool AllowsPack(Database& db,const CrateStorage::Request& r,LiveItems::InventorySessionFence& fence,const Scene::Snapshot& scene)const noexcept{
  try{auto member=members_.find(r.custody_id);return Held(db,r.lease,fence)&&member!=members_.end()&&r.vault==member->second.vault&&r.to_crate&&r.restore_original&&!r.room&&!r.parent_yard&&r.request_key==member->second.key&&r.estate_id==estate_&&r.native_plot_id==native_&&r.catalog_revision==catalog_&&r.crate_id==crate_&&r.crate_revision==member->second.crate_revision&&r.pose_revision==member->second.pose_revision&&r.storage_revision==member->second.storage_revision&&r.source.zone_id==route_.zone_id&&r.source.zone_version==route_.zone_version&&r.source.instance_id==route_.instance_id&&scene.estate_id==estate_&&scene.native_plot_id==native_&&scene.current_catalog_revision==catalog_&&scene.definition.type==4&&scene.access.implicit_owner&&scene.route.zone_id==route_.zone_id&&scene.route.zone_version==route_.zone_version&&scene.route.instance_id==route_.instance_id;}catch(...){return false;}
 }
 bool AllowsReturn(Database& db,const LiveItems::Request& r,LiveItems::InventorySessionFence& fence)const noexcept{
  try{return Held(db,r.lease,fence)&&r.request_key==key_&&r.estate_id==estate_&&r.action==LiveItems::Action::Return&&r.custody_id==custody_&&r.destination_slot==destination_&&r.crate&&r.crate->crate_id==crate_&&r.crate->expected_revision==return_revision_&&!r.placement&&!r.house&&!r.room&&!r.room_return_context&&!r.room_exterior_source&&!r.expected_room_pose_revision&&!r.verified_runtime_serial&&!r.vault&&!r.crate_member&&!r.partition_quantity&&!r.partition_serial&&!r.initial_stowed&&CustodyCodec::Encode(r.expected)==CustodyCodec::Encode(crate_row_);}catch(...){return false;}
 }
};
}
#endif
