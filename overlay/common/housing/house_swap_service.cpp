#include "house_swap_service.h"
#include "custody_codec.h"
#include "interior_item_content_db.h"
#include "crate_binding.h"
#include "crate_member_scope_db.h"
#include "npc_native_frame_db.h"
#include "npc_runtime_db.h"
#include "rent_state_db.h"
#include "upkeep_cycle_db.h"
#include "vault_storage_db.h"
#include <map>
#include <algorithm>
#include <cctype>
namespace Housing::HouseSwap {
namespace {
using Error=LiveItems::Error;using Bytes=std::vector<uint8_t>;
struct Failure {Error error;};
[[noreturn]] void Fail(Error e=Error::Conflict){throw Failure{e};}
auto Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())Fail(Error::DatabaseFailure);return r;}
auto One(Database& db,const std::string& sql){auto r=Q(db,sql);if(r.RowsAffected()!=1)Fail();return r;}
// Once COMMIT is attempted, an exception is as ambiguous as a failed driver
// response. Close that owned connection before fresh-connection resolution so
// an uncommitted transaction cannot keep its locks and block the resolver.
bool Commit(std::unique_ptr<Database>& connection)noexcept{
 try{auto result=connection->QueryDatabase(std::string("COMMIT"),false);if(result.Success())return true;}catch(...){}
 connection.reset();return false;
}
std::string Hex(const Bytes& b){return VaultStorage::Hex(b);}
std::string Literal(const std::string& s){return "x'"+Hex(Bytes(s.begin(),s.end()))+"'";}
bool Shape(const Request& r){
 if(r.key.rfind("houseswap:",0)!=0||r.key.size()>80||!r.estate||!r.lease.actor.character_id||!r.lease.actor.account_id||!r.lease.epoch||r.lease.session_key.empty()||r.lease.session_key.size()>96||!InteractionWire::DecodeHouseSwap(r.wire.data(),r.wire.size()))return false;
 for(unsigned char c:r.key)if(!std::isalnum(c)&&c!=':'&&c!='-'&&c!='_'&&c!='.')return false;return true;
}
Bytes Binding(const Request& r){
 Bytes b;CustodyCodec::W(b,1,4);CustodyCodec::S(b,r.key);CustodyCodec::W(b,r.estate,4);
 const auto& l=r.lease;CustodyCodec::W(b,l.actor.character_id,4);CustodyCodec::W(b,l.actor.account_id,4);CustodyCodec::W(b,l.epoch,8);CustodyCodec::S(b,l.session_key);b.insert(b.end(),l.token.begin(),l.token.end());b.insert(b.end(),r.wire.begin(),r.wire.end());return b;
}
Bytes Encode(const Receipt& r){
 if(!r.old_house||!r.new_house||r.old_house==r.new_house||r.changes.size()<2||r.changes.size()>10002)Fail(Error::DatabaseFailure);
 Bytes b;CustodyCodec::W(b,1,4);CustodyCodec::W(b,r.old_house,8);CustodyCodec::W(b,r.new_house,8);CustodyCodec::W(b,r.changes.size(),4);std::set<uint64_t> ids;std::set<uint32_t> before,after;
 size_t index=0;for(const auto& c:r.changes){
  const auto kind=uint8_t(c.kind);
  if(kind<1||kind>5||((index++<2)!=(c.kind==Kind::House))||!c.custody||!ids.insert(c.custody).second||!InteractionWire::Identity(c.before_native)||!InteractionWire::Identity(c.after_native)||!before.insert(c.before_native).second||!after.insert(c.after_native).second||c.before_storage==UINT64_MAX||c.after_storage!=c.before_storage+1||!CustodyCodec::Valid(c.payload)||!c.payload.values.character_id||!c.payload.values.item_id||(c.kind==Kind::House?c.before_native!=c.after_native:c.before_native==c.after_native))Fail(Error::DatabaseFailure);
  CustodyCodec::W(b,kind,1);for(auto v:{c.custody,c.before_storage,c.after_storage})CustodyCodec::W(b,v,8);for(auto v:{c.before_native,c.after_native})CustodyCodec::W(b,v,4);auto row=CustodyCodec::Encode(c.payload);CustodyCodec::W(b,row.size(),4);b.insert(b.end(),row.begin(),row.end());
 }
 if(r.changes[0].kind!=Kind::House||r.changes[1].kind!=Kind::House||b.size()>4*1024*1024)Fail(Error::DatabaseFailure);return b;
}
Receipt Decode(const Bytes& b){
 CustodyCodec::Reader reader{b};if(reader.N(4)!=1)Fail(Error::DatabaseFailure);Receipt r;r.old_house=reader.N(8);r.new_house=reader.N(8);const auto count=reader.N(4);if(count<2||count>10002)Fail(Error::DatabaseFailure);
 for(size_t i=0;i<count;++i){Change c;c.kind=Kind(reader.N(1));c.custody=reader.N(8);c.before_storage=reader.N(8);c.after_storage=reader.N(8);c.before_native=reader.N(4);c.after_native=reader.N(4);c.payload=CustodyCodec::Decode(reader.Blob(66000));r.changes.push_back(std::move(c));}
 if(reader.at!=b.size()||Encode(r)!=b)Fail(Error::DatabaseFailure);r.replayed=true;return r;
}
void Schema(Database& db){
 // These tables are all mutated or provide locks on writer/occupancy state.
 const auto tables={"housing_house_swap_receipts","housing_house_instances","housing_house_locations","housing_house_interiors","housing_house_room_profiles","housing_house_item_scopes","housing_live_item_custody","housing_live_placement_reservations","housing_live_placed_items","housing_interior_placement_reservations","housing_live_item_storage","housing_live_item_leases","housing_inventory_sessions","housing_native_estates","housing_catalogs","housing_crate_instances","housing_crate_locations","housing_crate_estates","housing_crate_members","housing_live_vault_items","items"};
 std::string names;for(auto name:tables){if(!names.empty())names+=",";names+=Literal(name);}auto r=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ("+names+")");if(r.RowCount()!=1||!r.begin()[0]||std::stoul(r.begin()[0])!=tables.size())Fail(Error::Disabled);
}
LiveItems::Result<Receipt> Lookup(Database& db,const Request& r){
 auto rows=Q(db,"SELECT format_version,character_id,account_id,HEX(binding_blob),HEX(outcome_blob) FROM housing_house_swap_receipts WHERE request_key="+Literal(r.key)+" FOR UPDATE");if(!rows.RowCount())return {{},Error::NotFound};if(rows.RowCount()!=1)Fail(Error::DatabaseFailure);auto row=rows.begin();for(unsigned i=0;i<5;++i)if(!row[i])Fail(Error::DatabaseFailure);
 if(std::stoul(row[0])!=1)Fail(Error::DatabaseFailure);
 if(std::stoul(row[1])!=r.lease.actor.character_id||std::stoul(row[2])!=r.lease.actor.account_id)Fail(Error::Unauthorized);
 if(VaultStorage::Unhex(row[3])!=Binding(r))Fail();
 // Vault's field decoder has a smaller item-specific bound; the aggregate
 // outcome is deliberately bounded separately before decoding its payloads.
 const std::string hex=row[4];if(hex.size()>8*1024*1024||hex.size()%2)Fail(Error::DatabaseFailure);Bytes bytes;bytes.reserve(hex.size()/2);auto digit=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;Fail(Error::DatabaseFailure);};for(size_t i=0;i<hex.size();i+=2)bytes.push_back(uint8_t(digit(hex[i])*16+digit(hex[i+1])));auto result=Decode(bytes);auto wire=*InteractionWire::DecodeHouseSwap(r.wire.data(),r.wire.size());if(result.changes[0].before_native!=wire.current.item_id||result.changes[1].before_native!=wire.replacement.item_id)Fail(Error::DatabaseFailure);return {std::move(result),Error::None};
}
Bytes Pose(const PlacementPolicy::Transform& pose){Bytes out;for(float v:{pose.heading,pose.pitch,pose.roll,pose.scale,pose.position[0],pose.position[1],pose.position[2]}){uint32_t bits;std::memcpy(&bits,&v,4);CustodyCodec::W(out,bits,4);}return out;}
void Storage(Database& db,const LiveItems::Stored& s,uint32_t from,uint32_t to,bool stowed){
 const auto id=std::to_string(s.custody_id);
 if(s.storage_revision)One(db,"UPDATE housing_live_item_storage SET revision=revision+1,stowed="+std::to_string(stowed)+",native_estate_id="+std::to_string(to)+" WHERE custody_id="+id+" AND revision="+std::to_string(s.storage_revision)+" AND native_estate_id="+std::to_string(from)+" AND stowed="+std::to_string(s.stowed));
 else One(db,"INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+id+",1,"+std::to_string(stowed)+","+std::to_string(to)+")");
}
struct Planned {const Scene::InteriorContent* source=nullptr;const VaultStorage::Item* vault=nullptr;std::optional<InteriorContent::Input> input;std::optional<InteriorContent::Fingerprint> fingerprint;};
std::vector<Planned> Plan(Database& db,const HouseSwapPreflight::Snapshot& s,LiveItems::InventorySessionFence& fence,const LiveItems::Lease& lease,const CrateBinding::CustodyContext* crates){
 const auto& old=s.current_room;const auto& next=s.replacement_room;
 if(!old.house||!next.house||!old.room_profile||!next.room_profile||!old.profile_revision||!next.profile_revision||old.profile_revision==UINT64_MAX||next.profile_revision==UINT64_MAX)Fail();
 // Original eqstr5338 refuses a placed crate in the old interior.
 for(const auto& item:old.contents)if(item.crate&&!item.stored.stowed)Fail(Error::UnsupportedItem);
 const auto& def=next.room_profile->definition;
 const auto vault=old.vault_items.size()+next.vault_items.size();
 // Direct container members retain their own container capacity; they are not
 // counted a second time against the room's direct item slots.
 const auto direct=old.contents.size()+next.contents.size();
 if(direct+vault>def.max_items)Fail(Error::RealEstateFull);if(vault>def.max_non_estate_items)Fail(Error::VaultFull);
 std::map<uint32_t,uint64_t> counts;uint64_t npcs=0;
 auto count=[&](const Scene::InteriorContent& item,const InteriorContent::Profile& profile){
  ++counts[item.stored.row.values.item_id];auto input=InteriorContent::ItemInputUnderTransaction(db,profile,item.stored.row.values.item_id);if(!input)Fail(input.error);if(input.value.npc)++npcs;
 };
 for(const auto& i:next.contents)count(i,*next.room_profile);
 for(const auto& i:old.contents)count(i,*next.room_profile);
 if(int32_t(def.max_npc_items)>=0&&npcs>def.max_npc_items)Fail(Error::NpcFull);
 for(auto [item,count]:counts){auto rows=Q(db,"SELECT UNK231 FROM items WHERE id="+std::to_string(item)+" FOR UPDATE");if(rows.RowCount()!=1||!rows.begin()[0])Fail(Error::UnsupportedItem);auto max=std::stoll(rows.begin()[0]);if(max<INT32_MIN||max>INT32_MAX)Fail();if(max>0&&count>uint64_t(max))Fail(Error::RealEstateFull);}
 std::vector<Planned> plan;std::set<uint64_t> containers;
 for(const auto& i:old.contents)if(i.crate){
  if(!crates||i.crate->binding.revision==UINT64_MAX||i.crate->content_manifest_sha256!=crates->ManifestDigest())Fail(Error::UnsupportedItem);
  auto definition=CrateBinding::ReadDefinitionUnderTransaction(db,*i.crate);if(!definition)Fail(definition.error);
  const auto& children=next.room_profile->child_definitions;auto target=std::find_if(children.begin(),children.end(),[&](const auto& d){return d.id==i.crate->binding.definition_id;});
  if(target==children.end()||EstateWire::EncodeBootstrap({*target},{})!=EstateWire::EncodeBootstrap({definition.value},{}))Fail(Error::UnsupportedItem);
  auto members=CrateBinding::ValidateMembersUnderTransaction(db,*i.crate);if(!members)Fail(members.error);size_t visible=0;for(const auto& m:old.crate_contents)if(m.contained_in_crate==i.crate->binding.crate_id)++visible;if(visible!=members.value)Fail();
  if(!containers.insert(i.crate->binding.crate_id).second)Fail();
 }
 auto add=[&](const Scene::InteriorContent& i){
  if(!i.room||!i.scope_revision||i.scope_revision==UINT64_MAX||!i.stored.placement||i.stored.placement->pose_revision==UINT64_MAX||i.stored.storage_revision==UINT64_MAX)Fail();
  if(!RentState::MutableItem(db,i.stored.estate_id,0,i.stored.custody_id))Fail(Error::Unauthorized);
  if(i.contained_in_crate&&(!i.crate_membership||!containers.count(i.contained_in_crate)||i.crate_membership->revision==UINT64_MAX))Fail(Error::UnsupportedItem);
  auto input=InteriorContent::ItemInputUnderTransaction(db,*next.room_profile,i.stored.row.values.item_id,{},true);if(!input)Fail(input.error);if(input.value.npc&&!fence.NpcProfileMatchesUnderTransaction(db,lease,*input.value.npc))Fail();auto fingerprint=InteriorContent::Build(input.value);if(!fingerprint)Fail(Error::UnsupportedItem);plan.push_back({&i,nullptr,std::move(input.value),std::move(fingerprint.value)});
 };
 for(const auto& i:old.contents)add(i);for(const auto& i:old.crate_contents)add(i);
 for(const auto& i:old.vault_items){if(i.revision==UINT64_MAX||!RentState::MutableItem(db,i.stored.estate_id,0,i.stored.custody_id))Fail(Error::Unauthorized);plan.push_back({nullptr,&i,{},{}});}
 if(plan.size()>10000)Fail(Error::RealEstateFull);return plan;
}
// Internal writer. Its inputs have been freshly read/locked by the SAME owner
// transaction; it is never an externally supplied snapshot mutation API.
Receipt Apply(Database& db,const Request& r,const HouseSwapPreflight::Snapshot& s,const std::vector<Planned>& plan){
 Receipt receipt;receipt.old_house=s.houses.current.house->house_id;receipt.new_house=s.houses.replacement.house->house_id;
 const auto& destination=s.replacement_room;const auto target=destination.house->interior->native_estate_id;
 // House bindings/interior tuples and custody blobs do not change. Only the
 // exterior pose and storage markers change, in the same TX as all contents.
 for(const auto* item:{&s.houses.current,&s.houses.replacement}){
  const auto& stored=item->stored;const auto native=stored.placement->reservation.native_item_id;
  Storage(db,stored,s.yard.native_plot_id,s.yard.native_plot_id,item==&s.houses.current);
  if(item==&s.houses.replacement)One(db,"UPDATE housing_live_placed_items SET pose_revision=pose_revision+1,pose_blob=x'"+Hex(Pose(s.houses.current.stored.placement->transform))+"' WHERE custody_id="+std::to_string(stored.custody_id)+" AND native_item_id="+std::to_string(native)+" AND pose_revision="+std::to_string(stored.placement->pose_revision));
  receipt.changes.push_back({Kind::House,stored.custody_id,native,native,stored.storage_revision,stored.storage_revision+1,stored.row});
 }
 size_t ordinal=0;
 for(const auto& planned:plan){
  const auto& stored=planned.source?planned.source->stored:planned.vault->stored;
  const auto custody=std::to_string(stored.custody_id);const uint32_t native=planned.source?stored.placement->reservation.native_item_id:planned.vault->native_item_id;
  const auto from=planned.source?stored.placement->reservation.native_estate_id:planned.vault->native_estate_id;
  const auto& next=*destination.house->interior;const auto key=Literal(r.key+":"+std::to_string(ordinal++));
  auto prior=Q(db,"SELECT character_id,account_id,lease_epoch,HEX(session_key),HEX(source_row),runtime_serial FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(native)+" AND state=1 AND custody_id="+custody+" FOR UPDATE");if(prior.RowCount()!=1)Fail();auto row=prior.begin();for(unsigned i=0;i<6;++i)if(!row[i])Fail(Error::DatabaseFailure);
  if(std::stoul(row[0])!=stored.row.values.character_id||VaultStorage::Unhex(row[4])!=CustodyCodec::Encode(stored.row)||std::stoll(row[5])<=0)Fail();
  One(db,"UPDATE housing_live_placement_reservations SET state=2,custody_id=NULL WHERE native_item_id="+std::to_string(native)+" AND state=1 AND custody_id="+custody);
  auto created=One(db,"INSERT INTO housing_live_placement_reservations(request_key,character_id,account_id,lease_epoch,session_key,estate_id,native_estate_id,catalog_key,catalog_revision,source_row,runtime_serial,state,custody_id) VALUES("+key+","+row[0]+","+row[1]+","+row[2]+",x'"+row[3]+"',"+std::to_string(stored.estate_id)+","+std::to_string(target)+","+Literal(next.catalog_key)+","+std::to_string(next.catalog_revision)+",x'"+row[4]+"',"+row[5]+",1,"+custody+")");auto id=created.LastInsertedID();if(!id||id>=UINT32_MAX)Fail();
  auto scope=HouseItemScope::Locked(db,stored.custody_id);if(!scope||scope->house_id!=receipt.old_house||scope->revision==UINT64_MAX||(planned.source&&scope->revision!=planned.source->scope_revision))Fail();
  One(db,"UPDATE housing_house_item_scopes SET house_id="+std::to_string(receipt.new_house)+",revision=revision+1 WHERE custody_id="+custody+" AND house_id="+std::to_string(receipt.old_house)+" AND revision="+std::to_string(scope->revision));
  Kind kind=Kind::Vault;uint64_t before_storage=planned.vault?planned.vault->revision:stored.storage_revision;
  if(planned.vault){
   One(db,"UPDATE housing_live_vault_items SET native_item_id="+std::to_string(id)+",house_id="+std::to_string(receipt.new_house)+",realm_estate_id="+std::to_string(s.yard.estate_id)+",revision=revision+1 WHERE custody_id="+custody+" AND native_item_id="+std::to_string(native)+" AND revision="+std::to_string(planned.vault->revision)+" AND house_id="+std::to_string(receipt.old_house));
  }else{
   const auto& source=*planned.source;const auto& old=*stored.placement;PlacementPolicy::Transform neutral{0,0,0,1,{0,0,0}};
   const auto digest=Hex(Bytes(planned.fingerprint->digest.begin(),planned.fingerprint->digest.end()));
   One(db,"INSERT INTO housing_interior_placement_reservations(native_item_id,house_id,house_revision,profile_revision,source_zone,source_version,source_instance,content_version,content_digest) VALUES("+std::to_string(id)+","+std::to_string(receipt.new_house)+","+std::to_string(destination.house->revision)+","+std::to_string(destination.profile_revision)+","+std::to_string(next.zone_id)+","+std::to_string(next.zone_version)+","+std::to_string(next.instance_id)+",3,x'"+digest+"')");
   One(db,"UPDATE housing_live_placed_items SET native_item_id="+std::to_string(id)+",pose_revision=1,pose_blob=x'"+Hex(Pose(neutral))+"' WHERE custody_id="+custody+" AND native_item_id="+std::to_string(native)+" AND pose_revision="+std::to_string(old.pose_revision));
   auto frame=old.npc_frame;if(frame){frame=NpcNativeFrame::Reframe(*frame,neutral);if(!frame)Fail();}
   NpcNativeFrame::ReplaceUnderTransaction(db,stored.custody_id,native,old.pose_revision,uint32_t(id),1,neutral,old.npc_frame,frame);
   Storage(db,stored,from,target,true);kind=Kind::Furniture;
   if(source.crate){
    const auto& b=source.crate->binding;kind=Kind::Crate;
    One(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+std::to_string(b.crate_id)+" AND revision="+std::to_string(b.revision));
    One(db,"UPDATE housing_crate_estates SET parent_native_estate_id="+std::to_string(target)+" WHERE crate_id="+std::to_string(b.crate_id)+" AND parent_native_estate_id="+std::to_string(from));
    One(db,"UPDATE housing_native_estates SET parent_native_id="+std::to_string(target)+" WHERE native_id="+std::to_string(b.native_estate_id)+" AND parent_native_id="+std::to_string(from)+" AND active=1");
   }
   if(source.crate_membership){
    const auto& m=*source.crate_membership;kind=Kind::Member;
    One(db,"UPDATE housing_crate_members SET revision=revision+1,origin_native_estate_id="+std::to_string(target)+",origin_house_id="+std::to_string(receipt.new_house)+",origin_stowed=1,origin_pose_blob=x'"+Hex(Pose(neutral))+"' WHERE custody_id="+custody+" AND crate_id="+std::to_string(m.crate_id)+" AND revision="+std::to_string(m.revision)+" AND origin_house_id="+std::to_string(receipt.old_house)+" AND origin_native_estate_id="+std::to_string(from));
   }
  }
  receipt.changes.push_back({kind,stored.custody_id,native,uint32_t(id),before_storage,before_storage+1,stored.row});
 }
 return receipt;
}
void HousePose(const HouseSwapPreflight::Snapshot& s){
 const auto& selected=s.houses.replacement;if(!selected.metadata||!selected.geometry)Fail(Error::UnsupportedItem);
 auto context=PlacementPolicy::Context{s.yard.native_plot_id,s.yard.definition.type,s.yard.region.id,s.yard.access,true};
 if(!PlacementPolicy::CheckAny(context,*selected.metadata,*selected.geometry,s.yard.region,s.yard.additional_regions,s.houses.current.stored.placement->transform,PlacementPolicy::Operation::Add))Fail(Error::Conflict);
 uint64_t placed=0;for(const auto& item:s.yard.items)if(item.house&&!item.stored.stowed)++placed;
 if(!placed||!s.yard.definition.max_children||placed>s.yard.definition.max_children)Fail(Error::RealEstateFull);
}
}
LiveItems::Result<Receipt> Service::Swap(const Request& r,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy& houses,const InteriorContent::ProfilePolicy& rooms,const CrateBinding::CustodyContext* crates){
 std::lock_guard<std::mutex> lock(mutex_);if(!options_.enabled||!options_.old_house_to_plot_storage)return {{},Error::Disabled};if(!Shape(r))return {{},Error::InvalidArgument};if(!fence.HeldFor(r.lease.actor,r.lease.epoch,r.lease.session_key))return {{},Error::UnheldFence};if(!database_)return {{},Error::DatabaseFailure};auto& db=*database_;bool tx=false;
 try{
  Q(db,"START TRANSACTION");tx=true;Schema(db);auto authenticated=LiveItems::Service::AuthenticateUnderTransaction(db,true,r.lease,fence);if(!authenticated)Fail(authenticated.error);
  auto prior=Lookup(db,r);Receipt receipt;
  if(prior)receipt=std::move(prior.value);
  else{
   if(prior.error!=Error::NotFound)Fail(prior.error);
   auto selected=HouseSwapPreflight::ReadUnderTransaction(db,true,r.lease,r.estate,*InteractionWire::DecodeHouseSwap(r.wire.data(),24),fence,houses,rooms,crates);if(!selected)Fail(selected.error);
   const auto& snapshot=selected.value;
   // Preserve canonical guild implicit-owner restrictions. Narrow policy also
   // requires the actor to be the immutable creator of BOTH tokens.
   if(snapshot.current_room.access.owner_kind==OwnerKind::Guild&&snapshot.current_room.access.effective.implicit_owner){auto guild=Q(db,"SELECT leader FROM guilds WHERE id="+std::to_string(snapshot.current_room.access.owner_id)+" FOR UPDATE");if(guild.RowCount()!=1||!guild.begin()[0]||std::stoul(guild.begin()[0])!=r.lease.actor.character_id)Fail(Error::Unauthorized);}
   HousePose(snapshot);auto plan=Plan(db,snapshot,fence,r.lease,crates);receipt=Apply(db,r,snapshot,plan);
   // Re-read using ordinary production scene validation, not a permissive
   // postimage decoder. This catches broken tags/crate graphs before commit.
   for(auto house:{receipt.old_house,receipt.new_house}){
    auto room=Scene::Service::ReadAttachedInteriorUnderTransaction(db,r.lease,r.estate,house,fence,houses,&rooms,true,crates);if(!room)Fail(room.error);auto complete=HouseSwapPreflight::CompleteGraphUnderTransaction(db,room.value);if(complete!=Error::None)Fail(complete);
    if(house==receipt.old_house&&(!room.value.contents.empty()||!room.value.crate_contents.empty()||!room.value.vault_items.empty()))Fail();
   }
   const auto outcome=Encode(receipt);
   One(db,"INSERT INTO housing_house_swap_receipts(request_key,format_version,character_id,account_id,binding_blob,outcome_blob) VALUES("+Literal(r.key)+",1,"+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+",x'"+Hex(Binding(r))+"',x'"+Hex(outcome)+"')");
  }
  tx=false;if(!Commit(database_))return {{},Error::CommitUncertain};return {std::move(receipt),Error::None};
 }catch(const Failure& e){if(tx)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},e.error};}catch(...){if(tx)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
LiveItems::Result<Receipt> Service::Resolve(const Request& r,LiveItems::InventorySessionFence& fence){
 std::lock_guard<std::mutex> lock(mutex_);if(!options_.enabled)return {{},Error::Disabled};if(!Shape(r))return {{},Error::InvalidArgument};if(!database_)return {{},Error::DatabaseFailure};bool tx=false;auto& db=*database_;
 try{Q(db,"START TRANSACTION");tx=true;Schema(db);auto lease=LiveItems::Service::AuthenticateUnderTransaction(db,true,r.lease,fence);if(!lease)Fail(lease.error);auto result=Lookup(db,r);tx=false;if(!Commit(database_))return {{},Error::CommitUncertain};return result;}
 catch(const Failure& e){if(tx)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},e.error};}catch(...){if(tx)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},Error::DatabaseFailure};}
}
}
