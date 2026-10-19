#include "crate_storage_destination.h"
#include "occupied_retirement_authority.h"
#include "rent_state_db.h"
#include "occupied_parent_yard.h"
#include "crate_storage_service.h"
#include "crate_binding.h"
#include "crate_vault_members_db.h"
#include "crate_foreign_member_policy_db.h"
#include "crate_house_rules.h"
#include "house_binding_service.h"
#include "crate_member_scope_db.h"
#include "item_storage_db.h"
#include "catalog_service.h"
#include "custody_codec.h"
#include "interior_item_content_db.h"
namespace Housing::CrateStorage {
namespace {
using Error=LiveItems::Error;using Bytes=std::vector<uint8_t>;
// Moving between a plot and its crate retains title and stable house identity.
// A fresh reviewed house may be moved by an actor with house ACL capabilities;
// this does not expand the separate inventory-return/title-transfer policy.
bool ForeignMovement(Database& db,const Scene::Item& item,uint32_t crate_creator){
 if(!item.house)return CrateForeignMember::EligibleUnderTransaction(db,item.stored.row);
 const auto& stored=item.stored;const auto& h=*item.house;
 // The crate member reader still requires matching house/crate creators.
 // Preserve that structural invariant, including when packing a foreign house.
 if(!crate_creator||stored.row.values.character_id!=crate_creator||
    !ForeignFurnitureReturn::NormalMode()||!ForeignFurnitureReturn::Shape(stored.row,stored.catalog)||
    !stored.placement||!h.interior||h.item_template_id!=stored.row.values.item_id||
    h.location.kind!=HouseBinding::Location::Kind::Custody||h.location.custody_id!=stored.custody_id||
    !stored.placement->reservation.content_binding||stored.placement->reservation.content_binding->version!=ContentBinding::HouseVersion)return false;
 auto metadata=db.QueryDatabase("SELECT idfile,nodrop,loregroup,itemclass,bagslots,notransfer FROM items WHERE id="+std::to_string(h.item_template_id)+" FOR UPDATE",false);
 if(!metadata.Success()||metadata.RowCount()!=1)return false;auto row=metadata.begin();for(unsigned i=0;i<6;++i)if(!row[i])return false;
 try{return HouseGeometry::Lookup(h.item_template_id,row[0])&&std::stoll(row[1])==stored.catalog.no_drop&&
  std::stoll(row[2])==stored.catalog.lore_group&&std::stoll(row[3])==stored.catalog.item_class&&
  std::stoll(row[4])==stored.catalog.bag_slots&&std::stoll(row[5])==0;}catch(...){return false;}
}

struct Failure{Error error;};
std::string Hex(const Bytes& bytes){const char* h="0123456789abcdef";std::string out;for(auto c:bytes){out+=h[c>>4];out+=h[c&15];}return out;}
Bytes Unhex(const char* text){
 if(!text)throw Failure{Error::DatabaseFailure};std::string s=text;Bytes out;if(s.size()%2||s.size()>1048576)throw Failure{Error::DatabaseFailure};
 auto n=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw Failure{Error::DatabaseFailure};};
 for(size_t i=0;i<s.size();i+=2)out.push_back((n(s[i])<<4)|n(s[i+1]));return out;
}
bool Key(const std::string& k){return !k.empty()&&k.size()<=96&&k.find('\0')==std::string::npos;}
Bytes Binding(const Request& r){
 Bytes out;auto n=[&](uint64_t v,unsigned size){CustodyCodec::W(out,v,size);};
 n(r.vault?4:r.parent_yard?3:r.room?2:1,4);CustodyCodec::S(out,r.request_key);n(r.lease.actor.character_id,4);n(r.lease.actor.account_id,4);n(r.lease.epoch,8);CustodyCodec::S(out,r.lease.session_key);out.insert(out.end(),r.lease.token.begin(),r.lease.token.end());
 n(r.estate_id,4);n(r.native_plot_id,4);n(r.catalog_revision,8);n(r.source.zone_id,4);n(r.source.zone_version,1);n(r.source.instance_id,2);
 for(auto v:{r.crate_id,r.crate_revision,r.custody_id,r.pose_revision,r.storage_revision})n(v,8);n(r.to_crate,1);n(r.restore_original,1);if(r.room){n(r.room->house_id,8);n(r.room->house_revision,8);n(r.room->profile_revision,8);n(r.room->attached,1);}if(r.vault){n(bool(r.room),1);n(bool(r.parent_yard),1);}if(r.parent_yard){auto parent=OccupiedParentYard::Encode(*r.parent_yard);n(parent.size(),4);out.insert(out.end(),parent.begin(),parent.end());}return out;
}
Bytes Pose(const PlacementPolicy::Transform& p){
 Bytes out;for(float f:{p.heading,p.pitch,p.roll,p.scale,p.position[0],p.position[1],p.position[2]}){if(!std::isfinite(f))throw Failure{Error::Conflict};uint32_t bits;std::memcpy(&bits,&f,4);CustodyCodec::W(out,bits,4);}return out;
}
// Adapt only a freshly authenticated room scene; preserve historical raw rows.
LiveItems::Result<Scene::Snapshot> StorageScene(Database& db,const Request& r,LiveItems::InventorySessionFence& fence,const CrateBinding::CustodyContext& crates,const Scene::HouseScenePolicy* houses,const InteriorContent::ProfilePolicy* rooms){
 if(r.parent_yard){if(r.room)return {{},Error::InvalidArgument};return OccupiedParentYard::ReadUnderTransaction(db,r.lease,*r.parent_yard,fence,houses,rooms,&crates);}
 if(!r.room)return Scene::Service::ReadSceneUnderTransaction(db,r.lease,r.estate_id,fence,houses,&crates);
 if(!houses||!rooms||!r.room->house_id||!r.room->house_revision||!r.room->profile_revision)return {{},Error::Unauthorized};
 auto current=r.room->attached?Scene::Service::ReadAttachedInteriorUnderTransaction(db,r.lease,r.estate_id,r.room->house_id,fence,*houses,rooms,true,&crates):Scene::Service::ReadInteriorSceneUnderTransaction(db,r.lease,r.source,fence,*houses,*rooms,&crates);
 if(!current)return {{},current.error};const auto& room=current.value;
 if(!room.house||!room.house->interior||!room.room_profile||room.house->house_id!=r.room->house_id||room.house->revision!=r.room->house_revision||room.profile_revision!=r.room->profile_revision||room.context.estate_id!=r.estate_id||room.house->interior->native_estate_id!=r.native_plot_id)return {{},Error::Conflict};
 const auto source=r.room->attached?room.exterior_source:std::optional<Route>{room.context.source};if(!source||source->zone_id!=r.source.zone_id||source->zone_version!=r.source.zone_version||source->instance_id!=r.source.instance_id)return {{},Error::Conflict};
 Scene::Snapshot out;out.estate_id=room.context.estate_id;out.native_plot_id=room.house->interior->native_estate_id;out.current_catalog_revision=room.context.catalog_revision;out.access=room.access.effective;out.acl_revision=room.access.acl_revision;out.route=*source;out.definition=room.room_profile->definition;out.region=room.room_profile->region;out.additional_regions=room.room_profile->additional_regions;out.vault_items=room.vault_items;out.crate_vault_items=room.crate_vault_items;
 for(bool contained:{false,true})for(const auto& entry:contained?room.crate_contents:room.contents){
  if(!entry.room||entry.room->house_id!=r.room->house_id||!entry.stored.placement)return {{},Error::Conflict};auto input=InteriorContent::ItemInputForStoredUnderTransaction(db,*room.room_profile,entry.stored);if(!input)return {{},input.error};
  Scene::Item item;item.stored=entry.stored;item.creator_name=entry.creator_name;item.crate=entry.crate;item.metadata=input.value.item;item.geometry=input.value.geometry;item.contained_in_crate=entry.contained_in_crate;item.crate_membership=entry.crate_membership;(contained?out.crate_contents:out.items).push_back(std::move(item));
 }
 return {std::move(out),Error::None};
}
}
// Distinct typed list-only path. The ordinary real-pose member path is unchanged.
static LiveItems::Result<Receipt> VaultTransferCurrent(Database* database_,bool enabled_,const Request& r,LiveItems::InventorySessionFence& fence,const CrateBinding::CustodyContext& policy,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* rooms,bool externally_owned,const OccupiedRetirement::Authority* retirement){
 if(!enabled_||!database_)return {{},Error::Disabled};
 if(!r.vault||r.pose_revision||!r.storage_revision||r.storage_revision==UINT64_MAX||!r.custody_id||!r.crate_id||!r.crate_revision||r.crate_revision==UINT64_MAX||!Key(r.request_key)||!Key(r.lease.session_key)||!r.estate_id||!r.native_plot_id||!r.catalog_revision||!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::InvalidArgument};
 auto& db=*database_;bool active=false;
 try{
  if(!externally_owned){VaultStorage::Query(db,"START TRANSACTION");active=true;}else{auto tx=VaultStorage::Query(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||std::string(tx.begin()[0])!="1")throw Failure{Error::InvalidArgument};}
  if(!VaultStorage::Installed(db)||!VaultStorage::CrateMembersInstalled(db))throw Failure{Error::Disabled};
  auto read=StorageScene(db,r,fence,policy,house,rooms);if(!read)throw Failure{read.error};auto& scene=read.value;
  if(scene.native_plot_id!=r.native_plot_id||scene.current_catalog_revision!=r.catalog_revision||!VaultStorage::SameRoute(scene.route,r.source))throw Failure{Error::Conflict};
  const bool retiring=retirement&&externally_owned&&retirement->AllowsPack(db,r,fence,scene);if(retirement&&!retiring)throw Failure{Error::Unauthorized};
  if(!retiring&&(!scene.access.Allows(AccessPolicy::Capability::MoveItem)||!scene.access.Allows(AccessPolicy::Capability::AddItem)||!scene.access.Allows(AccessPolicy::Capability::RemoveItem)))throw Failure{Error::Unauthorized};
  const auto binding=Binding(r);const auto key="x'"+Hex(Bytes(r.request_key.begin(),r.request_key.end()))+"'";
  auto saved=VaultStorage::Query(db,"SELECT character_id,account_id,HEX(binding_blob),crate_id,custody_id,crate_revision,storage_revision FROM housing_crate_storage_receipts WHERE request_key="+key+" FOR UPDATE");Receipt out;
  if(saved.RowCount()){
   if(externally_owned||saved.RowCount()!=1)throw Failure{Error::Conflict};auto x=saved.begin();for(unsigned i=0;i<7;++i)if(!x[i])throw Failure{Error::Conflict};
   if(std::stoul(x[0])!=r.lease.actor.character_id||std::stoul(x[1])!=r.lease.actor.account_id||Unhex(x[2])!=binding)throw Failure{Error::Conflict};out={std::stoull(x[3]),std::stoull(x[4]),std::stoull(x[5]),std::stoull(x[6]),true};if(out.crate_id!=r.crate_id||out.custody_id!=r.custody_id||out.crate_revision!=r.crate_revision+1||out.storage_revision!=r.storage_revision+1)throw Failure{Error::Conflict};
  }else{
   auto container=std::find_if(scene.items.begin(),scene.items.end(),[&](const auto& i){return i.crate&&i.crate->binding.crate_id==r.crate_id;});
   if(container==scene.items.end()||container->stored.stowed||container->crate->packed||container->crate->binding.revision!=r.crate_revision||container->crate->content_manifest_sha256!=policy.ManifestDigest())throw Failure{Error::Conflict};
   const auto& candidates=r.to_crate?scene.vault_items:scene.crate_vault_items;auto found=std::find_if(candidates.begin(),candidates.end(),[&](const auto&i){return i.stored.custody_id==r.custody_id;});
   if(found==candidates.end()||found->revision!=r.storage_revision||found->stored.placement||found->native_estate_id!=r.native_plot_id||found->house_id!=(r.room?r.room->house_id:0)||found->stored.row.values.character_id!=r.lease.actor.character_id||found->stored.row.values.character_id!=container->crate->created_by_character_id||(r.to_crate?bool(found->contained_in_crate):found->contained_in_crate!=r.crate_id))throw Failure{Error::Unauthorized};
   (void)VaultStorage::Profile(db,found->stored.row);
   if(!retiring&&!RentState::MutableItem(db,found->stored.estate_id,0,found->stored.custody_id))throw Failure{Error::Unauthorized};
   if(r.to_crate){
    auto definition=CrateBinding::ReadDefinitionUnderTransaction(db,*container->crate);if(!definition)throw Failure{definition.error};auto members=CrateBinding::ValidateMembersUnderTransaction(db,*container->crate);if(!members)throw Failure{members.error};if(members.value>=definition.value.max_items||CrateVault::Count(db,r.crate_id)>=definition.value.max_non_estate_items)throw Failure{Error::VaultFull};
    Bytes cat;for(auto n:{found->stored.catalog.item_class,found->stored.catalog.bag_slots,found->stored.catalog.lore_group,found->stored.catalog.no_drop})CustodyCodec::W(cat,uint32_t(n),4);
    auto inserted=VaultStorage::Query(db,"INSERT INTO housing_crate_vault_members(custody_id,crate_id,revision,origin_native_estate_id,origin_house_id,origin_realm_estate_id,vault_revision,source_row,catalog_blob,schema_descriptor) VALUES("+std::to_string(r.custody_id)+","+std::to_string(r.crate_id)+",1,"+std::to_string(r.native_plot_id)+","+(found->house_id?std::to_string(found->house_id):"NULL")+","+std::to_string(found->realm_estate_id)+","+std::to_string(found->revision+1)+",UNHEX('"+Hex(CustodyCodec::Encode(found->stored.row))+"'),UNHEX('"+Hex(cat)+"'),'"+db.Escape(LiveItems::Service::SchemaDescriptor())+"')");if(inserted.RowsAffected()!=1)throw Failure{Error::Conflict};
   }else{
    // Unpack restores storage in the actual current realm, never a world pose.
    VaultStorage::Target target;target.definition=scene.definition;target.placed=scene.items.size();auto full=VaultStorage::CapacityError(target,VaultStorage::Count(db,r.native_plot_id));if(full!=Error::None)throw Failure{full};
    auto erased=VaultStorage::Query(db,"DELETE FROM housing_crate_vault_members WHERE custody_id="+std::to_string(r.custody_id)+" AND crate_id="+std::to_string(r.crate_id)+" AND revision="+std::to_string(found->membership_revision)+" AND vault_revision="+std::to_string(found->revision));if(erased.RowsAffected()!=1)throw Failure{Error::Conflict};
   }
   auto changed=VaultStorage::Query(db,"UPDATE housing_live_vault_items SET revision=revision+1 WHERE custody_id="+std::to_string(r.custody_id)+" AND revision="+std::to_string(found->revision));if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};
   auto crate=VaultStorage::Query(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+std::to_string(r.crate_id)+" AND revision="+std::to_string(r.crate_revision)+" AND packed=0");if(crate.RowsAffected()!=1)throw Failure{Error::Conflict};
   out={r.crate_id,r.custody_id,r.crate_revision+1,found->revision+1,false};
   auto receipt=VaultStorage::Query(db,"INSERT INTO housing_crate_storage_receipts(request_key,character_id,account_id,binding_blob,crate_id,custody_id,crate_revision,storage_revision) VALUES("+key+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+",UNHEX('"+Hex(binding)+"'),"+std::to_string(out.crate_id)+","+std::to_string(out.custody_id)+","+std::to_string(out.crate_revision)+","+std::to_string(out.storage_revision)+")");if(receipt.RowsAffected()!=1)throw Failure{Error::DatabaseFailure};
  }
  if(active){active=false;if(!db.QueryDatabase(std::string("COMMIT"),false).Success())return {{},Error::CommitUncertain};}return {out,Error::None};
 }catch(const Failure& e){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},e.error};}catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}

static LiveItems::Result<Receipt> TransferCurrent(Database* database_,bool enabled_,const Request& r,LiveItems::InventorySessionFence& fence,const CrateBinding::CustodyContext& policy,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* rooms,bool externally_owned=false,Scene::Snapshot* shared=nullptr,const OccupiedRetirement::Authority* retirement=nullptr){
 if(r.vault)return VaultTransferCurrent(database_,enabled_,r,fence,policy,house,rooms,externally_owned,retirement);
 if(!enabled_||!database_)return {{},Error::Disabled};
 if(!Key(r.request_key)||!Key(r.lease.session_key)||!r.lease.actor.character_id||!r.lease.actor.account_id||!r.lease.epoch||!r.estate_id||!r.native_plot_id||r.native_plot_id==UINT32_MAX||!r.catalog_revision||!r.crate_id||!r.crate_revision||r.crate_revision==UINT64_MAX||!r.custody_id||!r.pose_revision||r.storage_revision==UINT64_MAX||!r.source.zone_id||!r.source.instance_id)return {{},Error::InvalidArgument};
 if(r.parent_yard&&(r.room||!OccupiedParentYard::Shape(*r.parent_yard)))return {{},Error::InvalidArgument};
 if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};
 auto& db=*database_;bool transaction=false;
 auto q=[&](const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())throw Failure{Error::DatabaseFailure};return result;};
 try{
  if(!externally_owned){q("START TRANSACTION");transaction=true;}
  else{auto tx=q("SELECT @@in_transaction");if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")throw Failure{Error::InvalidArgument};}
  if(!CrateBinding::Installed(db)||!ItemStorage::Installed(db))throw Failure{Error::Disabled};
  auto schema=q("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_crate_storage_receipts'");if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB")throw Failure{Error::Disabled};
  std::optional<Scene::Snapshot> local;
  if(!shared){auto scene=StorageScene(db,r,fence,policy,house,rooms);if(!scene)throw Failure{scene.error};local=std::move(scene.value);}
  auto& yard=shared?*shared:*local;
  if(yard.native_plot_id!=r.native_plot_id||yard.current_catalog_revision!=r.catalog_revision||yard.route.zone_id!=r.source.zone_id||yard.route.zone_version!=r.source.zone_version||yard.route.instance_id!=r.source.instance_id)throw Failure{Error::Conflict};
  const bool retiring=retirement&&externally_owned&&retirement->AllowsPack(db,r,fence,yard);
  if(retirement&&!retiring)throw Failure{Error::Unauthorized};
  if(!retiring&&(!yard.access.Allows(AccessPolicy::Capability::MoveItem)||!yard.access.Allows(AccessPolicy::Capability::AddItem)||!yard.access.Allows(AccessPolicy::Capability::RemoveItem)))throw Failure{Error::Unauthorized};
  const auto key="x'"+Hex(Bytes(r.request_key.begin(),r.request_key.end()))+"'";const auto binding=Binding(r);
  auto old=q("SELECT character_id,account_id,HEX(binding_blob),crate_id,custody_id,crate_revision,storage_revision FROM housing_crate_storage_receipts WHERE request_key="+key+" FOR UPDATE");
  Receipt out;
  if(old.RowCount()){
   if(externally_owned)throw Failure{Error::Conflict};
   if(old.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto row=old.begin();for(size_t i=0;i<7;++i)if(!row[i])throw Failure{Error::DatabaseFailure};
   if(std::stoul(row[0])!=r.lease.actor.character_id||std::stoul(row[1])!=r.lease.actor.account_id||Unhex(row[2])!=binding)throw Failure{Error::Conflict};
   out={std::stoull(row[3]),std::stoull(row[4]),std::stoull(row[5]),std::stoull(row[6]),true};
   if(out.crate_id!=r.crate_id||out.custody_id!=r.custody_id||out.crate_revision!=r.crate_revision+1||out.storage_revision!=r.storage_revision+1)throw Failure{Error::DatabaseFailure};
  }else{
   auto crate=std::find_if(yard.items.begin(),yard.items.end(),[&](const auto& item){return item.crate&&item.crate->binding.crate_id==r.crate_id;});
   if(crate==yard.items.end()||crate->stored.stowed||crate->crate->packed||crate->crate->binding.revision!=r.crate_revision||crate->crate->binding.parent_native_estate_id!=r.native_plot_id||crate->crate->content_manifest_sha256!=policy.ManifestDigest())throw Failure{Error::Conflict};
   const auto& members=r.to_crate?yard.items:yard.crate_contents;
   auto member=std::find_if(members.begin(),members.end(),[&](const auto& item){return item.stored.custody_id==r.custody_id;});
   if(member==members.end()||member->crate||!member->metadata||!member->geometry||!member->stored.placement||member->stored.placement->pose_revision!=r.pose_revision||member->stored.storage_revision!=r.storage_revision||(r.to_crate?bool(member->contained_in_crate):member->contained_in_crate!=r.crate_id))throw Failure{Error::Conflict};
   if((member->stored.row.values.character_id!=r.lease.actor.character_id||member->stored.row.values.character_id!=crate->crate->created_by_character_id)&&!ForeignMovement(db,*member,crate->crate->created_by_character_id))throw Failure{Error::UnsupportedItem};
   if(!retiring&&!RentState::MutableItem(db,r.estate_id,r.native_plot_id,member->stored.custody_id))throw Failure{Error::Unauthorized};
   if(member->house&&(!house||(!retiring&&(!yard.access.Allows(AccessPolicy::Capability::AddHouse)||!yard.access.Allows(AccessPolicy::Capability::RemoveHouse)))))throw Failure{Error::Unauthorized};
   auto scope=CrateMemberScope::Locked(db,r.custody_id,bool(r.room));if(bool(scope)==r.to_crate)throw Failure{Error::Conflict};
   auto state=ItemStorage::Locked(db,r.custody_id);if(state.revision!=r.storage_revision||(state.revision&&state.native_estate_id!=r.native_plot_id))throw Failure{Error::Conflict};
   Scene::Item moved=*member;
   const auto pose=Pose(member->stored.placement->transform);
   bool stowed=true;
   if(r.to_crate){
    if(member->house&&!CrateHouseRules::CanAdmit(db,r.crate_id,member->house->house_id))throw Failure{Error::Conflict};
    auto definition=CrateBinding::ReadDefinitionUnderTransaction(db,*crate->crate);if(!definition)throw Failure{definition.error};
    size_t used=CrateVault::Count(db,r.crate_id);for(const auto& item:yard.crate_contents)if(item.contained_in_crate==r.crate_id)++used;
    if(used>=definition.value.max_items)throw Failure{Error::Conflict};
    auto inserted=q("INSERT INTO housing_crate_members(custody_id,crate_id,revision,origin_native_estate_id,origin_house_id,origin_pose_blob,origin_stowed) VALUES("+std::to_string(r.custody_id)+","+std::to_string(r.crate_id)+",1,"+std::to_string(r.native_plot_id)+","+(r.room?std::to_string(r.room->house_id):"NULL")+",x'"+Hex(pose)+"',"+std::to_string(state.stowed)+")");if(inserted.RowsAffected()!=1)throw Failure{Error::Conflict};
   }else{
    if(!CrateStorageDestination::Allowed(*member->metadata,yard.definition.type,member->house&&member->house->interior?member->house->interior->definition_id:0,yard.definition.children))throw Failure{Error::UnsupportedItem};
    if(!yard.definition.max_items||yard.items.size()>=yard.definition.max_items)throw Failure{Error::Conflict};
    if(scope->origin_house_id!=(r.room?r.room->house_id:0)||scope->crate_id!=r.crate_id||scope->origin_native_estate_id!=r.native_plot_id||scope->origin_pose!=pose||!state.stowed)throw Failure{Error::Conflict};stowed=r.restore_original?scope->origin_stowed:true;
    if(!stowed){
     PlacementPolicy::Context destination{yard.native_plot_id,yard.definition.type,yard.region.id,yard.access,true};
     if(!PlacementPolicy::CheckAny(destination,*member->metadata,*member->geometry,yard.region,yard.additional_regions,member->stored.placement->transform,PlacementPolicy::Operation::Add))throw Failure{Error::Conflict};
    }
    auto removed=q("DELETE FROM housing_crate_members WHERE custody_id="+std::to_string(r.custody_id)+" AND crate_id="+std::to_string(r.crate_id)+" AND revision="+std::to_string(scope->revision));if(removed.RowsAffected()!=1)throw Failure{Error::Conflict};
   }
   if(!state.revision){auto changed=q("INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+std::to_string(r.custody_id)+",1,"+std::to_string(stowed)+","+std::to_string(r.native_plot_id)+")");if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};}
   else{auto changed=q("UPDATE housing_live_item_storage SET revision=revision+1,stowed="+std::to_string(stowed)+" WHERE custody_id="+std::to_string(r.custody_id)+" AND revision="+std::to_string(state.revision));if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};}
   auto changed=q("UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+std::to_string(r.crate_id)+" AND revision="+std::to_string(r.crate_revision)+" AND packed=0");if(changed.RowsAffected()!=1)throw Failure{Error::Conflict};
   out={r.crate_id,r.custody_id,r.crate_revision+1,state.revision+1,false};
   q("INSERT INTO housing_crate_storage_receipts(request_key,character_id,account_id,binding_blob,crate_id,custody_id,crate_revision,storage_revision) VALUES("+key+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+",x'"+Hex(binding)+"',"+std::to_string(out.crate_id)+","+std::to_string(out.custody_id)+","+std::to_string(out.crate_revision)+","+std::to_string(out.storage_revision)+")");
   if(shared){
    auto allocation=std::make_shared<CrateBinding::Allocation>(*crate->crate);allocation->binding.revision=out.crate_revision;crate->crate=std::move(allocation);
    moved.stored.storage_revision=out.storage_revision;moved.stored.stowed=stowed;moved.contained_in_crate=r.to_crate?r.crate_id:0;
    if(r.to_crate){CrateMemberScope::Membership membership;membership.crate_id=r.crate_id;membership.revision=1;membership.origin_native_estate_id=r.native_plot_id;membership.origin_house_id=r.room?r.room->house_id:0;membership.origin_pose=pose;membership.origin_stowed=state.stowed;moved.crate_membership=std::make_shared<const CrateMemberScope::Membership>(std::move(membership));}else moved.crate_membership.reset();
    auto& before=r.to_crate?yard.items:yard.crate_contents;before.erase(std::remove_if(before.begin(),before.end(),[&](const auto& item){return item.stored.custody_id==r.custody_id;}),before.end());
    auto& after=r.to_crate?yard.crate_contents:yard.items;after.push_back(std::move(moved));
   }
  }
  if(!externally_owned){transaction=false;if(!db.QueryDatabase(std::string("COMMIT"),false).Success())return {{},Error::CommitUncertain};}return {out,Error::None};
 }catch(const Failure& f){if(transaction)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},f.error};}catch(...){if(transaction)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Receipt> Service::Transfer(const Request& r,LiveItems::InventorySessionFence& fence,const CrateBinding::CustodyContext& policy,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* rooms){
 std::lock_guard<std::mutex> lock(mutex_);return TransferCurrent(database_.get(),enabled_,r,fence,policy,house,rooms);
}
LiveItems::Result<Receipt> Service::PackForRetirementUnderTransaction(Database& db,bool enabled,const Request& r,LiveItems::InventorySessionFence& fence,const OccupiedRetirement::Authority& retirement,const CrateBinding::CustodyContext& policy,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* rooms){return TransferCurrent(&db,enabled,r,fence,policy,house,rooms,true,nullptr,&retirement);}
LiveItems::Result<Receipt> Service::TransferUnderTransaction(Database& db,bool enabled,const Request& r,LiveItems::InventorySessionFence& fence,const CrateBinding::CustodyContext& policy,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* rooms){
 return TransferCurrent(&db,enabled,r,fence,policy,house,rooms,true);
}

LiveItems::Result<BatchReceipt> Service::Pack(const BatchRequest& r,LiveItems::InventorySessionFence& fence,const CrateBinding::CustodyContext& policy,const Scene::HouseScenePolicy* house,const InteriorContent::ProfilePolicy* rooms){
 if(!enabled_||!database_)return {{},Error::Disabled};
 if(!Key(r.request_key)||r.request_key.size()>80||!Key(r.lease.session_key)||!r.lease.actor.character_id||!r.lease.actor.account_id||!r.lease.epoch||r.native_plot_id==UINT32_MAX||!r.crate_id||!r.crate_revision||r.members.empty()||r.members.size()>1000||r.crate_revision>=UINT64_MAX-r.members.size()||!r.estate_id||!r.native_plot_id||!r.catalog_revision||!r.source.zone_id||!r.source.instance_id)return {{},Error::InvalidArgument};
 for(size_t i=0;i<r.members.size();++i){const auto& m=r.members[i];if(!m.custody_id||(m.vault?bool(m.pose_revision)||!m.storage_revision:!m.pose_revision)||m.storage_revision==UINT64_MAX||(i&&r.members[i-1].custody_id>=m.custody_id))return {{},Error::InvalidArgument};}
 if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};
 Request prototype;prototype.request_key=r.request_key;prototype.lease=r.lease;prototype.estate_id=r.estate_id;prototype.native_plot_id=r.native_plot_id;prototype.catalog_revision=r.catalog_revision;prototype.source=r.source;prototype.crate_id=r.crate_id;prototype.crate_revision=r.crate_revision;prototype.to_crate=r.packing;prototype.restore_original=r.packing;prototype.room=r.room;
 auto binding=Binding(prototype);CustodyCodec::W(binding,0x42554c4b,4);CustodyCodec::W(binding,r.members.size(),4);for(const auto& m:r.members){CustodyCodec::W(binding,m.custody_id,8);CustodyCodec::W(binding,m.pose_revision,8);CustodyCodec::W(binding,m.storage_revision,8);}if(std::any_of(r.members.begin(),r.members.end(),[](const auto& m){return m.vault;})){CustodyCodec::W(binding,0x564c5431,4);for(const auto& m:r.members)CustodyCodec::W(binding,m.vault,1);}
 const auto key="x'"+Hex(Bytes(r.request_key.begin(),r.request_key.end()))+"'";
 std::lock_guard<std::mutex> lock(mutex_);auto& db=*database_;bool active=false;
 auto q=[&](const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())throw Failure{Error::DatabaseFailure};return result;};
 try{
  q("START TRANSACTION");active=true;
  auto schema=q("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_crate_batch_receipts'");if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB")throw Failure{Error::Disabled};
  auto scene=StorageScene(db,prototype,fence,policy,house,rooms);if(!scene)throw Failure{scene.error};auto& yard=scene.value;
  if(yard.native_plot_id!=r.native_plot_id||yard.current_catalog_revision!=r.catalog_revision||yard.route.zone_id!=r.source.zone_id||yard.route.zone_version!=r.source.zone_version||yard.route.instance_id!=r.source.instance_id)throw Failure{Error::Conflict};
  if(!yard.access.Allows(AccessPolicy::Capability::MoveItem)||!yard.access.Allows(AccessPolicy::Capability::AddItem)||!yard.access.Allows(AccessPolicy::Capability::RemoveItem))throw Failure{Error::Unauthorized};
  auto prior=q("SELECT character_id,account_id,HEX(binding_blob),crate_id,crate_revision,member_count FROM housing_crate_batch_receipts WHERE request_key="+key+" FOR UPDATE");BatchReceipt out;
  if(prior.RowCount()){
   if(prior.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto row=prior.begin();for(size_t i=0;i<6;++i)if(!row[i])throw Failure{Error::DatabaseFailure};
   if(std::stoul(row[0])!=r.lease.actor.character_id||std::stoul(row[1])!=r.lease.actor.account_id||Unhex(row[2])!=binding)throw Failure{Error::Conflict};out={std::stoull(row[3]),std::stoull(row[4]),uint32_t(std::stoul(row[5])),true};
   if(out.crate_id!=r.crate_id||!out.member_count||out.member_count>r.members.size()||out.crate_revision!=r.crate_revision+out.member_count)throw Failure{Error::DatabaseFailure};
  }else{
   auto container=std::find_if(yard.items.begin(),yard.items.end(),[&](const auto& item){return item.crate&&item.crate->binding.crate_id==r.crate_id;});if(container==yard.items.end()||container->stored.stowed||container->crate->packed||container->crate->binding.revision!=r.crate_revision)throw Failure{Error::Conflict};
   std::vector<BatchMember> exact;
   const auto& selected=r.packing?yard.items:yard.crate_contents;
   for(const auto& item:selected){
    if(!r.packing&&item.contained_in_crate!=r.crate_id)continue;
    if(item.crate)continue;
    if(!item.stored.placement)throw Failure{Error::UnsupportedItem};
    exact.push_back({item.stored.custody_id,item.stored.placement->pose_revision,item.stored.storage_revision});
   }
   for(const auto& item:r.packing?yard.vault_items:yard.crate_vault_items){if(!r.packing&&item.contained_in_crate!=r.crate_id)continue;exact.push_back({item.stored.custody_id,0,item.revision,true});}
   std::sort(exact.begin(),exact.end(),[](const auto& a,const auto& b){return a.custody_id<b.custody_id;});if(exact.size()!=r.members.size())throw Failure{Error::Conflict};
   for(size_t i=0;i<exact.size();++i)if(exact[i].custody_id!=r.members[i].custody_id||exact[i].pose_revision!=r.members[i].pose_revision||exact[i].storage_revision!=r.members[i].storage_revision||exact[i].vault!=r.members[i].vault)throw Failure{Error::Conflict};
   auto definition=CrateBinding::ReadDefinitionUnderTransaction(db,*container->crate);if(!definition)throw Failure{definition.error};
   size_t used=r.packing?CrateVault::Count(db,r.crate_id)+size_t(std::count_if(yard.crate_contents.begin(),yard.crate_contents.end(),[&](const auto& item){return item.contained_in_crate==r.crate_id;})):yard.items.size()+yard.vault_items.size();
   const size_t maximum=r.packing?definition.value.max_items:yard.definition.max_items;
   std::vector<BatchMember> chosen;size_t chosen_vaults=0;const auto vault_maximum=r.packing?definition.value.max_non_estate_items:yard.definition.max_non_estate_items;const auto used_vaults=r.packing?CrateVault::Count(db,r.crate_id):yard.vault_items.size();auto planned_houses=r.packing?CrateHouseRules::LockedMembers(db,r.crate_id):std::vector<CrateHouseRules::House>{};
   // Original RoF2 strings5238/5239 specify fitting/allowed items. Bind the
   // complete candidate snapshot, then select its eligible subset under locks.
   // Custody order is deterministic; exact historical priority is unverified.
   for(const auto& candidate:r.members){
    if(used+chosen.size()>=maximum)break;
    if(candidate.vault){if(used_vaults+chosen_vaults>=vault_maximum)continue;const auto& vaults=r.packing?yard.vault_items:yard.crate_vault_items;auto item=std::find_if(vaults.begin(),vaults.end(),[&](const auto& i){return i.stored.custody_id==candidate.custody_id;});if(item==vaults.end()||item->stored.row.values.character_id!=r.lease.actor.character_id||item->stored.row.values.character_id!=container->crate->created_by_character_id)continue;if(!RentState::MutableItem(db,item->stored.estate_id,0,item->stored.custody_id))throw Failure{Error::Unauthorized};chosen.push_back(candidate);++chosen_vaults;continue;}

    auto item=std::find_if(selected.begin(),selected.end(),[&](const auto& entry){return entry.stored.custody_id==candidate.custody_id;});
    if(item==selected.end()||!item->metadata||!item->geometry)continue;
    if((item->stored.row.values.character_id!=r.lease.actor.character_id||item->stored.row.values.character_id!=container->crate->created_by_character_id)&&!ForeignMovement(db,*item,container->crate->created_by_character_id))continue;
    if(!RentState::MutableItem(db,r.estate_id,r.native_plot_id,item->stored.custody_id))throw Failure{Error::Unauthorized};
    if(item->house&&(!house||!yard.access.Allows(AccessPolicy::Capability::AddHouse)||!yard.access.Allows(AccessPolicy::Capability::RemoveHouse)))continue;
    if(r.packing&&item->house){auto houses=planned_houses;houses.push_back(CrateHouseRules::LockedHouse(db,item->house->house_id));if(!CrateHouseRules::Allowed(houses))continue;planned_houses=std::move(houses);}
    if(!r.packing){auto membership=CrateMemberScope::Locked(db,candidate.custody_id,bool(r.room));if(!membership||membership->crate_id!=r.crate_id)throw Failure{Error::Conflict};
     // Bulk Unpack moves eligible items to Shed/Closet. A saved layout or
     // native Place supplies their destination pose afterward; stale source
     // coordinates cannot exclude an otherwise valid storage item.
     if(!CrateStorageDestination::Allowed(*item->metadata,yard.definition.type,item->house&&item->house->interior?item->house->interior->definition_id:0,yard.definition.children))continue;
    }
    chosen.push_back(candidate);
   }
   if(chosen.empty())throw Failure{Error::NotFound};
   for(size_t i=0;i<chosen.size();++i){auto request=prototype;request.request_key=r.request_key+":"+std::to_string(i);request.crate_revision=r.crate_revision+i;request.custody_id=chosen[i].custody_id;request.pose_revision=chosen[i].pose_revision;request.storage_revision=chosen[i].storage_revision;request.vault=chosen[i].vault;
    auto moved=TransferCurrent(&db,true,request,fence,policy,house,rooms,true,nullptr);if(!moved)throw Failure{moved.error};
   }
   out={r.crate_id,r.crate_revision+chosen.size(),uint32_t(chosen.size()),false};
   q("INSERT INTO housing_crate_batch_receipts(request_key,character_id,account_id,binding_blob,crate_id,crate_revision,member_count) VALUES("+key+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+",x'"+Hex(binding)+"',"+std::to_string(out.crate_id)+","+std::to_string(out.crate_revision)+","+std::to_string(out.member_count)+")");
  }
  active=false;if(!db.QueryDatabase(std::string("COMMIT"),false).Success())return {{},Error::CommitUncertain};return {out,Error::None};
 }catch(const Failure& f){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},f.error};}catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}

}
