#ifndef EQEMU_HOUSING_OFFLINE_RETURN_BATCH_DB_H
#define EQEMU_HOUSING_OFFLINE_RETURN_BATCH_DB_H
#include "offline_property_delivery_db.h"
#include "offline_direct_return_db.h"
#include "return_settlement_db.h"
#include "crate_vault_members_db.h"
#include "house_relocation_preflight.h"
#include <map>
namespace Housing::OfflineReturnBatch {
using Status=InactivityRetirement::Status;
namespace Detail {
using OfflinePropertyDelivery::Detail::Q;using OfflinePropertyDelivery::Detail::N;
using OfflinePropertyDelivery::Detail::Id;using OfflinePropertyDelivery::Detail::Hex;
using OfflinePropertyDelivery::Detail::Quote;using OfflinePropertyDelivery::Detail::Empty;
using OfflinePropertyDelivery::Detail::One;using OfflinePropertyDelivery::Detail::Stop;
inline void Require(bool value,Status status=Status::Conflict){if(!value)throw Stop{status};}
struct Authority {
 uint32_t estate=0,plot=0,native=0,kind=0,owner=0,refunder=0,account=0,last=0;
 uint64_t revision=0;uint32_t max_recipients=0,max_crates=0,capacity=0;
 bool parcel_overflow=false;std::string catalog,manifest,review,inactivity_review;Catalog::Bundle bundle;
};
inline bool Enabled(Database& db,uint32_t estate){
 auto table=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_offline_return_batch_policy'");if(!table.RowCount())return false;
 Require(table.RowCount()==1&&table.begin()[0]&&std::string(table.begin()[0])=="InnoDB",Status::DatabaseFailure);
 return Q(db,"SELECT estate_id FROM housing_offline_return_batch_policy WHERE estate_id="+Id(estate)).RowCount()==1;
}
inline void Installed(Database& db,bool guild){
 Require(OfflinePropertyDelivery::Detail::Installed(db),Status::DatabaseFailure);
 std::vector<std::string> tables={"housing_offline_return_batch_policy","housing_offline_return_finance","housing_offline_return_items","housing_return_settlements","housing_return_settlement_items","housing_return_settlement_crates","housing_crate_creation_origins"};
 if(guild)for(const auto* table:{"guilds","guild_members","housing_guild_inactivity_policy","housing_guild_inactivity_retirements"})tables.push_back(table);
 std::string names;for(const auto& table:tables){if(!names.empty())names+=",";names+=Quote(db,table);}auto rows=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN("+names+")");Require(rows.RowCount()==1&&N(rows.begin()[0])==tables.size(),Status::DatabaseFailure);
}
inline Authority Authorize(Database& db,uint32_t estate,uint32_t now,bool guild){
 const uint32_t interval=guild?2592000u:InactivityRetirement::InactiveSeconds;Require(now>=interval,Status::NotDue);
 if(guild){auto id=Q(db,"SELECT owner_id FROM housing_current_estates WHERE id="+Id(estate)+" AND owner_kind=3");Require(id.RowCount()==1,Status::NotDue);Require(Q(db,"SELECT id FROM guilds WHERE id="+Id(N(id.begin()[0]))+" FOR UPDATE").RowCount()==1,Status::Deferred);}
 auto e=Q(db,"SELECT e.plot_id,e.owner_kind,e.owner_id,n.native_id,n.catalog_key,n.definition_id,c.revision,HEX(c.definitions_blob),HEX(c.regions_blob),HEX(p.crate_manifest_sha256),p.max_recipients,p.max_crates,p.items_per_crate,p.review_reference FROM housing_current_estates e JOIN housing_native_estates n ON n.ownership_estate_id=e.id AND n.plot_id=e.plot_id AND n.active=1 JOIN housing_catalogs c ON c.catalog_key=n.catalog_key JOIN housing_offline_return_batch_policy p ON p.estate_id=e.id AND p.native_plot_id=n.native_id AND p.catalog_revision=c.revision AND p.definitions_sha256=UNHEX(SHA2(c.definitions_blob,256)) AND p.regions_sha256=UNHEX(SHA2(c.regions_blob,256)) WHERE e.id="+Id(estate)+" AND e.owner_kind="+Id(guild?3:1)+" AND c.client_build=20130510 AND c.origin IN(1,2) FOR UPDATE");
 Require(e.RowCount()==1,Status::NotDue);auto r=e.begin();Authority a;a.estate=estate;a.plot=N(r[0]);a.kind=N(r[1]);a.owner=N(r[2]);a.native=N(r[3]);Require(r[4]&&r[9]&&r[13]);a.catalog=r[4];a.revision=N(r[6]);a.manifest=r[9];a.max_recipients=N(r[10]);a.max_crates=N(r[11]);a.capacity=N(r[12]);a.review=r[13];auto column=Q(db,"SELECT column_name FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_offline_return_batch_policy' AND column_name='allow_parcel_overflow'");if(column.RowCount()){auto option=Q(db,"SELECT allow_parcel_overflow FROM housing_offline_return_batch_policy WHERE estate_id="+Id(estate)+" FOR UPDATE");Require(option.RowCount()==1&&N(option.begin()[0])<=1);a.parcel_overflow=N(option.begin()[0]);if(a.parcel_overflow)Require(ReturnParcels::Installed(db),Status::DatabaseFailure);}
 Require(a.plot&&a.owner&&a.native&&a.native<UINT32_MAX&&a.revision&&a.manifest.size()==64&&a.max_recipients&&a.max_recipients<=32&&a.max_crates&&a.max_crates<=32&&a.capacity&&a.capacity<=1000&&!a.review.empty());
 a.bundle=Catalog::DecodePayloads(UpkeepSelection::Detail::Unhex(r[7]),UpkeepSelection::Detail::Unhex(r[8]));
 auto plot=std::find_if(a.bundle.definitions.begin(),a.bundle.definitions.end(),[&](const auto& d){return d.id==N(r[5]);});Require(plot!=a.bundle.definitions.end()&&plot->type==(guild?8u:4u));
 auto box=std::find_if(a.bundle.definitions.begin(),a.bundle.definitions.end(),[](const auto& d){return d.id==174&&d.type==7;});Require(box!=a.bundle.definitions.end()&&box->max_items&&a.capacity<=box->max_items,Status::Deferred);
 if(guild){
  auto policy=Q(db,"SELECT g.leader,p.recipient_character_id,p.review_reference FROM guilds g JOIN housing_guild_inactivity_policy p ON p.estate_id="+Id(estate)+" AND p.native_plot_id="+Id(a.native)+" AND p.catalog_revision="+Id(a.revision)+" JOIN housing_catalogs c ON c.catalog_key="+Quote(db,a.catalog)+" AND p.definitions_sha256=UNHEX(SHA2(c.definitions_blob,256)) AND p.regions_sha256=UNHEX(SHA2(c.regions_blob,256)) JOIN guild_members leader ON leader.guild_id=g.id AND leader.char_id=g.leader WHERE g.id="+Id(a.owner)+" AND p.inactive_seconds=2592000 FOR UPDATE");Require(policy.RowCount()==1&&N(policy.begin()[0])==N(policy.begin()[1]),Status::NotDue);a.refunder=N(policy.begin()[0]);Require(policy.begin()[2]&&policy.begin()[2][0]);a.inactivity_review=policy.begin()[2];
  auto members=Q(db,"SELECT m.char_id,c.last_login,c.deleted_at FROM guild_members m LEFT JOIN character_data c ON c.id=m.char_id WHERE m.guild_id="+Id(a.owner)+" ORDER BY m.char_id FOR UPDATE");Require(members.RowCount(),Status::Deferred);for(auto m=members.begin();m!=members.end();++m){Require(m[1]&&!m[2]&&N(m[1])&&N(m[1])<=UINT32_MAX,Status::Deferred);a.last=std::max(a.last,uint32_t(N(m[1])));}
  Empty(db,"SELECT s.character_id FROM housing_inventory_sessions s JOIN guild_members m ON m.char_id=s.character_id WHERE s.active=1 AND m.guild_id="+Id(a.owner));
 }else{
  auto policy=Q(db,"SELECT c.last_login,p.review_reference FROM housing_inactivity_policy p JOIN character_data c ON c.id="+Id(a.owner)+" AND c.deleted_at IS NULL WHERE p.estate_id="+Id(estate)+" AND p.native_plot_id="+Id(a.native)+" AND p.catalog_revision="+Id(a.revision)+" AND p.inactive_seconds=7776000 FOR UPDATE");Require(policy.RowCount()==1&&policy.begin()[1]&&policy.begin()[1][0],Status::NotDue);Require(N(policy.begin()[0])<=UINT32_MAX);a.last=N(policy.begin()[0]);a.refunder=a.owner;a.inactivity_review=policy.begin()[1];
 }
 Require(a.last&&a.last<=now-interval,Status::NotDue);
 auto recipient=Q(db,"SELECT c.account_id FROM character_data c JOIN account ac ON ac.id=c.account_id WHERE c.id="+Id(a.refunder)+" AND c.deleted_at IS NULL FOR UPDATE");Require(recipient.RowCount()==1&&N(recipient.begin()[0])&&N(recipient.begin()[0])<=UINT32_MAX,Status::Deferred);a.account=N(recipient.begin()[0]);
 Require(Q(db,"SELECT id FROM housing_plot_definitions WHERE id="+Id(a.plot)+" FOR UPDATE").RowCount()==1);
 Empty(db,"SELECT instance_key FROM housing_fixture_item_custody WHERE estate_id="+Id(estate));
 Empty(db,"SELECT native_item_id FROM housing_live_placement_reservations WHERE (estate_id="+Id(estate)+" OR native_estate_id="+Id(a.native)+") AND state=0");
 Empty(db,"SELECT character_id FROM housing_live_item_leases WHERE pending_key IS NOT NULL");
 Empty(db,"SELECT p.request_key FROM housing_live_escrow_pending p JOIN housing_live_escrow_receipts r ON r.request_key=p.request_key WHERE r.estate_id="+Id(estate));
 return a;
}
inline std::map<uint32_t,uint32_t> Recipients(Database& db,const Authority& a,const ReturnSettlement::Snapshot& captured){
 std::set<uint32_t> ids{a.refunder};for(const auto& s:captured.sources)ids.insert(s.owner_character_id);Require(ids.size()<=a.max_recipients,Status::Deferred);std::map<uint32_t,uint32_t> accounts;
 for(auto owner:ids){Require(owner);auto row=Q(db,"SELECT c.account_id FROM character_data c JOIN account ac ON ac.id=c.account_id WHERE c.id="+Id(owner)+" AND c.deleted_at IS NULL FOR UPDATE");Require(row.RowCount()==1&&N(row.begin()[0])&&N(row.begin()[0])<=UINT32_MAX,Status::Deferred);accounts[owner]=N(row.begin()[0]);
  Empty(db,"SELECT character_id FROM housing_inventory_sessions WHERE character_id="+Id(owner)+" AND active=1");Empty(db,"SELECT character_id FROM housing_live_item_leases WHERE character_id="+Id(owner)+" AND (active=1 OR pending_key IS NOT NULL)");Require(CoinPending::Inspect(db,owner,true)==CoinPending::State::Clear,Status::Deferred);Require(HouseInventory::InspectPending(db,owner,true)==HouseInventory::PendingState::Clear,Status::Deferred);
 }
 return accounts;
}
struct Bin {CrateBinding::Allocation allocation;LiveItems::InventoryRow row;uint32_t count=0;bool house=false;};
inline Bin Create(Database& db,const Authority& a,uint32_t recipient,uint32_t ordinal,uint32_t account){
 auto meta=Q(db,"SELECT idfile,itemclass,bagslots,stackable,UNK227,UNK226,maxcharges,evoitem,loregroup FROM items WHERE id=62586 FOR UPDATE");Require(meta.RowCount()==1,Status::Deferred);auto m=meta.begin();Require(m[0]&&std::string(m[0])=="IT20028"&&!N(m[1])&&!N(m[2])&&!N(m[3])&&N(m[4])==174&&!N(m[5])&&!std::stoll(m[6])&&!N(m[7])&&!std::stoll(m[8]),Status::Deferred);Empty(db,"SELECT id FROM items_evolving_details WHERE item_id=62586");
 Bin out;out.row.values=InventoryRepository::NewEntity();out.row.values.character_id=recipient;out.row.values.slot_id=OfflineDirectReturns::Detail::Slot(db,recipient,a.parcel_overflow);out.row.values.item_id=62586;out.row.values.charges=0;out.row.values.guid=0;out.row.null_fields=0;
 const auto key="offline-return:"+Id(a.estate)+":"+Id(recipient)+":"+Id(ordinal);CrateCreationOrigin::OfflineReturn(db,key,a.estate,recipient,ordinal);
 auto made=Q(db,"INSERT INTO housing_crate_instances(item_template_id,created_by_character_id,creation_request_key,content_manifest_sha256) VALUES(62586,"+Id(recipient)+","+Quote(db,key)+",x'"+a.manifest+"')");Require(made.RowsAffected()==1&&made.LastInsertedID());auto crate=made.LastInsertedID();
 auto parent=Q(db,"SELECT neighborhood_id FROM housing_native_estates WHERE native_id="+Id(a.native)+" AND ownership_estate_id="+Id(a.estate)+" AND active=1 FOR UPDATE");Require(parent.RowCount()==1);
 auto node=Q(db,"INSERT INTO housing_native_estates(catalog_key,node_key,parent_native_id,neighborhood_id,definition_id,static_id,active) VALUES("+Quote(db,a.catalog)+","+Quote(db,"crate:"+Id(crate))+","+Id(a.native)+","+Id(N(parent.begin()[0]))+",174,0,0)");Require(node.RowsAffected()==1&&node.LastInsertedID()&&node.LastInsertedID()<UINT32_MAX);
 uint32_t parcel=0;if(out.row.values.slot_id){One(db,"INSERT INTO inventory("+std::string(InventoryRowDB::WriteColumns)+") VALUES("+InventoryRowDB::Values(out.row,out.row.values.slot_id)+")");One(db,"INSERT INTO housing_crate_locations(crate_id,location_kind,character_id,slot_id,custody_id) VALUES("+Id(crate)+",1,"+Id(recipient)+","+Id(out.row.values.slot_id)+",NULL)");}else{std::vector<uint8_t> state;CustodyCodec::S(state,key);CustodyCodec::S(state,a.manifest);parcel=ReturnParcels::QueueUnderTransaction(db,a.estate,recipient,account,out.row,0,crate,state,true).parcel_id;One(db,"INSERT INTO housing_crate_locations(crate_id,location_kind,parcel_id) VALUES("+Id(crate)+",3,"+Id(parcel)+")");}
 One(db,"INSERT INTO housing_crate_estates(crate_id,native_estate_id,parent_native_estate_id,ownership_estate_id,definition_id,catalog_key,catalog_revision) VALUES("+Id(crate)+","+Id(node.LastInsertedID())+","+Id(a.native)+","+Id(a.estate)+",174,"+Quote(db,a.catalog)+","+Id(a.revision)+")");
 if(parcel)out.allocation=ReturnParcels::ReadCrateUnderTransaction(db,parcel);else{auto read=CrateBinding::ReadCarriedUnderTransaction(db,out.row);Require(bool(read),Status::Deferred);out.allocation=read.value;}Require(bool(CrateBinding::ReadDefinitionUnderTransaction(db,out.allocation)),Status::Deferred);return out;
}
// A carried house keeps its permanent interior alive after whole Return. Its
// old yard parent is history, not an unaccounted currently placed house. Admit
// only an exact creator-owned inventoried token and a complete quiescent room.
inline bool DetachedCarriedRoom(Database& db,uint32_t native,uint32_t parent){
 try{
 auto rows=Q(db,"SELECT h.house_id,h.revision,h.item_template_id,h.created_by_character_id,h.pending_request_key,l.location_kind,l.character_id,l.slot_id,l.custody_id,i.definition_id,i.zone_id,i.instance_id,i.zone_version,i.catalog_key,i.catalog_revision,n.parent_native_id,n.active,n.definition_id,n.catalog_key FROM housing_house_interiors i JOIN housing_house_instances h USING(house_id) JOIN housing_house_locations l USING(house_id) JOIN housing_native_estates n ON n.native_id=i.native_estate_id WHERE i.native_estate_id="+Id(native)+" FOR UPDATE");
 if(rows.RowCount()!=1)return false;auto r=rows.begin();for(unsigned i=0;i<19;++i)if(i!=4&&i!=8&&!r[i])return false;
 const auto owner=N(r[3]),slot=N(r[7]);if(!N(r[0])||!N(r[1])||r[4]||N(r[5])!=1||N(r[6])!=owner||r[8]||slot<23||slot>33||N(r[15])!=parent||N(r[16])!=1||N(r[17])!=N(r[9])||std::string(r[18])!=r[13]||!N(r[14])||!N(r[11])||N(r[11])>32000)return false;
 auto actor=Q(db,"SELECT c.account_id,c.deleted_at,a.id FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+Id(owner)+" FOR UPDATE");if(actor.RowCount()!=1||!actor.begin()[0]||actor.begin()[1]||!actor.begin()[2]||N(actor.begin()[0])!=N(actor.begin()[2]))return false;
 if(Q(db,"SELECT character_id FROM housing_inventory_sessions WHERE character_id="+Id(owner)+" AND active=1 LIMIT 1 FOR UPDATE").RowCount()||Q(db,"SELECT character_id FROM housing_live_item_leases WHERE character_id="+Id(owner)+" AND (active=1 OR pending_key IS NOT NULL) LIMIT 1 FOR UPDATE").RowCount()||CoinPending::Inspect(db,uint32_t(owner),true)!=CoinPending::State::Clear||HouseInventory::InspectPending(db,uint32_t(owner),true)!=HouseInventory::PendingState::Clear)return false;
 auto inv=Q(db,"SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+Id(owner)+" AND slot_id="+Id(slot)+" FOR UPDATE");if(inv.RowCount()!=1)return false;auto item=InventoryRowDB::Decode(inv.begin());
 if(!CustodyCodec::Valid(item)||item.values.character_id!=owner||item.values.slot_id!=slot||item.values.item_id!=N(r[2])||item.values.augment_one||item.values.augment_two||item.values.augment_three||item.values.augment_four||item.values.augment_five||item.values.augment_six)return false;
 auto meta=Q(db,"SELECT idfile,itemclass,bagslots,stackable,UNK227,evoitem FROM items WHERE id="+Id(N(r[2]))+" FOR UPDATE");if(meta.RowCount()!=1||!meta.begin()[0]||N(meta.begin()[1])||N(meta.begin()[2])||N(meta.begin()[3])||N(meta.begin()[4])!=N(r[9])||N(meta.begin()[5])||!HouseGeometry::Lookup(uint32_t(N(r[2])),meta.begin()[0])||!HouseTemplates::Type(uint32_t(N(r[2]))))return false;
 Empty(db,"SELECT id FROM items_evolving_details WHERE item_id="+Id(N(r[2])));Empty(db,"SELECT id FROM character_evolving_items WHERE character_id="+Id(owner)+" AND item_id="+Id(N(r[2]))+" AND deleted_at IS NULL");
 auto instance=Q(db,"SELECT zone,version,never_expires FROM instance_list WHERE id="+Id(N(r[11]))+" FOR UPDATE");if(instance.RowCount()!=1||N(instance.begin()[0])!=N(r[10])||N(instance.begin()[1])!=N(r[12])||N(instance.begin()[2])!=1)return false;
 auto catalog=Q(db,"SELECT revision,client_build,origin,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,r[13])+" FOR UPDATE");if(catalog.RowCount()!=1||N(catalog.begin()[0])!=N(r[14])||N(catalog.begin()[1])!=20130510||(N(catalog.begin()[2])!=1&&N(catalog.begin()[2])!=2))return false;
 auto bundle=Catalog::DecodePayloads(UpkeepSelection::Detail::Unhex(catalog.begin()[3]),UpkeepSelection::Detail::Unhex(catalog.begin()[4]));auto def=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==N(r[9]);});if(def==bundle.definitions.end()||(def->type!=2&&def->type!=3)||def->zone_id!=N(r[10]))return false;
 HouseBinding::Binding binding;binding.house_id=N(r[0]);binding.revision=N(r[1]);binding.item_template_id=N(r[2]);binding.location={HouseBinding::Location::Kind::Inventory,uint32_t(owner),uint32_t(slot),0};binding.interior=HouseBinding::Interior{native,uint32_t(N(r[9])),uint32_t(N(r[10])),uint16_t(N(r[11])),uint8_t(N(r[12])),r[13],N(r[14])};
 return HouseRelocationPreflight::OfflineStructuralUnderTransaction(db,binding)==LiveItems::Error::None;
 }catch(...){return false;}
}

struct Unit {uint64_t custody=0,house=0;uint32_t owner=0;LiveItems::Stored stored;bool original_stowed=false;};
inline void Pack(Database& db,const Authority& a,const Unit& unit,Bin& bin){
 const auto& b=bin.allocation.binding;Require(unit.owner==bin.allocation.created_by_character_id&&bin.count<a.capacity&&(!unit.house||!bin.house));const auto& s=unit.stored;Require(s.placement&&s.estate_id==a.estate&&s.placement->reservation.native_estate_id==a.native&&s.placement->reservation.estate_id==a.estate&&s.placement->reservation.catalog_key==a.catalog&&s.placement->reservation.catalog_revision==a.revision,Status::Deferred);
 auto storage=ItemStorage::Locked(db,unit.custody);Require(storage.revision<UINT64_MAX&&(!storage.revision||storage.native_estate_id==a.native));
 auto pose=Q(db,"SELECT HEX(pose_blob) FROM housing_live_placed_items WHERE custody_id="+Id(unit.custody)+" FOR UPDATE");Require(pose.RowCount()==1&&pose.begin()[0]&&std::string(pose.begin()[0]).size()==56);
 One(db,"INSERT INTO housing_crate_members(custody_id,crate_id,revision,origin_native_estate_id,origin_house_id,origin_pose_blob,origin_stowed) VALUES("+Id(unit.custody)+","+Id(b.crate_id)+",1,"+Id(a.native)+",NULL,x'"+std::string(pose.begin()[0])+"',"+Id(unit.original_stowed)+")");
 if(storage.revision)One(db,"UPDATE housing_live_item_storage SET stowed=1,revision=revision+1 WHERE custody_id="+Id(unit.custody)+" AND revision="+Id(storage.revision));else One(db,"INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+Id(unit.custody)+",1,1,"+Id(a.native)+")");
 One(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+Id(b.crate_id)+" AND revision="+Id(b.revision)+" AND packed=0");bin.allocation.binding.revision++;bin.count++;bin.house=bin.house||unit.house;
}
}
// Entire estate disposition is ONE transaction across all rightful recipients.
// Legacy policies keep their prior single-recipient behavior when not opted in.
inline Status RetireBatch(Database& db,uint32_t estate,uint32_t now,bool guild){
 using namespace Detail;bool active=false,committing=false;
 try{
  auto tx=Q(db,"SELECT @@in_transaction");Require(tx.RowCount()==1&&!N(tx.begin()[0]));Q(db,"START TRANSACTION");active=true;Installed(db,guild);auto a=Authorize(db,estate,now,guild);
  auto captured=ReturnSettlement::Capture(db,estate);Require(captured.native_plot_id==a.native&&captured.owner_kind==a.kind&&captured.owner_id==a.owner&&captured.catalog_key==a.catalog&&captured.catalog_revision==a.revision&&captured.refund_recipient_character_id==a.refunder&&captured.refund_recipient_account_id==a.account);const auto accounts=Recipients(db,a,captured);
  std::map<uint64_t,uint32_t> house_owners;for(const auto& s:captured.sources)if(s.house_id){auto h=Q(db,"SELECT created_by_character_id,pending_request_key FROM housing_house_instances WHERE house_id="+Id(s.house_id)+" FOR UPDATE");Require(h.RowCount()==1&&N(h.begin()[0])==s.owner_character_id&&!h.begin()[1],Status::Deferred);house_owners[s.house_id]=s.owner_character_id;Empty(db,"SELECT sessions.character_id FROM housing_house_interiors i JOIN housing_inventory_sessions sessions ON sessions.zone_id=i.zone_id AND sessions.instance_id=i.instance_id AND sessions.active=1 WHERE i.house_id="+Id(s.house_id));Empty(db,"SELECT r.native_item_id FROM housing_live_placement_reservations r JOIN housing_house_interiors i ON i.native_estate_id=r.native_estate_id WHERE i.house_id="+Id(s.house_id)+" AND r.state=0");}
  std::map<uint64_t,InteriorContent::Profile> rooms;auto reviewed_room=[&](uint64_t house)->const InteriorContent::Profile&{auto found=rooms.find(house);if(found==rooms.end())found=rooms.emplace(house,OfflineRoomContent::ReadUnderTransaction(db,house,estate)).first;return found->second;};
  // Validate old typed containers before changing any membership, then
  // redistribute each original member to its own creator's return bin.
  std::map<uint64_t,bool> original_stowed;
  for(const auto& s:captured.sources)if(s.crate_id){auto inspected=LiveItems::Service::InspectCustodyUnderTransaction(db,s.custody_id);Require(bool(inspected),Status::Deferred);auto scope=HouseItemScope::Locked(db,s.custody_id);auto box=CrateBinding::ReadPlacedUnderTransaction(db,inspected.value);Require(box&&box.value.binding.parent_native_estate_id==(scope?reviewed_room(scope->house_id).native_estate_id:a.native)&&box.value.binding.estate_id==estate&&box.value.created_by_character_id==s.owner_character_id,Status::Deferred);auto valid=CrateBinding::ValidateMembersUnderTransaction(db,box.value);Require(bool(valid),Status::Deferred);auto vaults=CrateVault::Read(db,box.value);
   // Validate every typed member before detaching either kind. Direct returns
   // may consume a vault source only after its containing crate is accounted for.
   for(const auto& v:vaults){auto source=std::find_if(captured.sources.begin(),captured.sources.end(),[&](const auto& x){return x.custody_id==v.stored.custody_id;});Require(source!=captured.sources.end()&&source->owner_character_id==s.owner_character_id&&source->row_blob==CustodyCodec::Encode(v.stored.row),Status::Deferred);}
   if(!vaults.empty()){auto detached=Q(db,"DELETE FROM housing_crate_vault_members WHERE crate_id="+Id(s.crate_id));Require(detached.RowsAffected()==vaults.size(),Status::Conflict);}
   auto members=Q(db,"SELECT custody_id,origin_house_id,origin_stowed FROM housing_crate_members WHERE crate_id="+Id(s.crate_id)+" ORDER BY custody_id FOR UPDATE");for(auto m=members.begin();m!=members.end();++m){Require(scope?(m[1]&&N(m[1])==scope->house_id):!m[1],Status::Deferred);original_stowed[N(m[0])]=N(m[2])!=0;}Q(db,"DELETE FROM housing_crate_members WHERE crate_id="+Id(s.crate_id));if(valid.value)One(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+Id(s.crate_id)+" AND revision="+Id(box.value.binding.revision));}
  std::vector<ReturnSettlement::Disposition> dispositions;std::vector<Unit> units;
  for(const auto& s:captured.sources){
   auto row=CustodyCodec::Decode(s.row_blob);Require(CustodyCodec::Valid(row)&&row.values.character_id==s.owner_character_id);
   auto vault=Q(db,"SELECT custody_id FROM housing_live_vault_items WHERE custody_id="+Id(s.custody_id)+" FOR UPDATE");auto room=Q(db,"SELECT house_id FROM housing_house_item_scopes WHERE custody_id="+Id(s.custody_id)+" FOR UPDATE");Require(vault.RowCount()<=1&&room.RowCount()<=1);uint64_t room_house=room.RowCount()?N(room.begin()[0]):0;
   if(vault.RowCount()||(room_house&&house_owners.at(room_house)!=s.owner_character_id)||s.crate_id){
    auto out=s.crate_id?OfflineDirectReturns::ReturnCrate(db,estate,s.custody_id,s.owner_character_id,a.parcel_overflow):OfflineDirectReturns::Return(db,estate,s.custody_id,s.owner_character_id,a.parcel_overflow);
    dispositions.push_back({s.custody_id,out.parcel_id?ReturnSettlement::Route::Parcel:ReturnSettlement::Route::Direct,out.owner,out.account,out.parcel_id?out.parcel_id:out.destination_slot,out.crate_id,0});continue;
   }
   auto npc=OfflineNpcReturn::CurrentProfileUnderTransaction(db,row.values.item_id);if(npc){
    if(room_house){auto inspected=LiveItems::Service::InspectCustodyUnderTransaction(db,s.custody_id);Require(bool(inspected)&&house_owners.at(room_house)==s.owner_character_id,Status::Deferred);OfflineNpcReturn::InspectUnderTransaction(db,inspected.value,estate,s.owner_character_id,&reviewed_room(room_house));dispositions.push_back({s.custody_id,ReturnSettlement::Route::RetainedHouse,s.owner_character_id,accounts.at(s.owner_character_id),0,0,room_house});}
    else{auto out=OfflineDirectReturns::Return(db,estate,s.custody_id,s.owner_character_id,a.parcel_overflow);dispositions.push_back({s.custody_id,out.parcel_id?ReturnSettlement::Route::Parcel:ReturnSettlement::Route::Direct,out.owner,out.account,out.parcel_id?out.parcel_id:out.destination_slot,0,0});}continue;
   }
   Empty(db,"SELECT custody_id FROM housing_live_npc_frames WHERE custody_id="+Id(s.custody_id));auto meta=Q(db,"SELECT idfile,evoitem FROM items WHERE id="+Id(row.values.item_id)+" FOR UPDATE");Require(meta.RowCount()==1&&meta.begin()[0],Status::Deferred);ItemPayload::Metadata(db,row);ItemPayload::ValidateHeld(db,row,s.custody_id);
   if(room_house){Require(house_owners.at(room_house)==s.owner_character_id&&StaticFurniture::Lookup(row.values.item_id,meta.begin()[0]),Status::Deferred);dispositions.push_back({s.custody_id,ReturnSettlement::Route::RetainedHouse,s.owner_character_id,accounts.at(s.owner_character_id),0,0,room_house});continue;}
   auto inspected=LiveItems::Service::InspectCustodyUnderTransaction(db,s.custody_id);Require(bool(inspected)&&CustodyCodec::Encode(inspected.value.row)==s.row_blob,Status::Deferred);auto house=HouseGeometry::Lookup(row.values.item_id,meta.begin()[0]);Require((s.house_id&&house&&HouseTemplates::Type(row.values.item_id)==(guild?2u:3u))||(!s.house_id&&!house&&StaticFurniture::Lookup(row.values.item_id,meta.begin()[0])),Status::Deferred);
   units.push_back({s.custody_id,s.house_id,s.owner_character_id,inspected.value,original_stowed.count(s.custody_id)?original_stowed.at(s.custody_id):inspected.value.stowed});
  }
  std::vector<Bin> bins;std::vector<ReturnSettlement::CreatedCrate> created;
  for(const auto& unit:units){size_t i=0;for(;i<bins.size();++i)if(bins[i].allocation.created_by_character_id==unit.owner&&bins[i].count<a.capacity&&(!unit.house||!bins[i].house))break;
   if(i==bins.size()){Require(bins.size()<a.max_crates,Status::Deferred);bins.push_back(Create(db,a,unit.owner,uint32_t(bins.size()+1),accounts.at(unit.owner)));created.push_back({bins.back().allocation.binding.crate_id,unit.owner,accounts.at(unit.owner)});}
   Pack(db,a,unit,bins[i]);dispositions.push_back({unit.custody,ReturnSettlement::Route::Crate,unit.owner,accounts.at(unit.owner),0,bins[i].allocation.binding.crate_id,0});
  }
  for(const auto& bin:bins){CrateBinding::Allocation final;if(bin.allocation.parcel_id)final=ReturnParcels::ReadCrateUnderTransaction(db,bin.allocation.parcel_id);else{auto read=CrateBinding::ReadCarriedUnderTransaction(db,bin.row);Require(bool(read),Status::Deferred);final=read.value;}Require(CrateBinding::EncodeAllocation(final)==CrateBinding::EncodeAllocation(bin.allocation),Status::Deferred);auto count=CrateBinding::ValidateMembersUnderTransaction(db,final);Require(count&&count.value==bin.count,Status::Deferred);OfflinePropertyDelivery::Detail::Cargo(db,bin.allocation.binding.crate_id,bin.allocation.created_by_character_id);}
  // No unaccounted active child may survive outside a delivered retained hall.
  auto children=Q(db,"SELECT native_id FROM housing_native_estates WHERE parent_native_id="+Id(a.native)+" AND active=1 FOR UPDATE");for(auto c=children.begin();c!=children.end();++c){auto host=Q(db,"SELECT house_id FROM housing_house_interiors WHERE native_estate_id="+Id(N(c[0]))+" FOR UPDATE");Require(host.RowCount()==1&&(house_owners.count(N(host.begin()[0]))||DetachedCarriedRoom(db,uint32_t(N(c[0])),a.native)),Status::Deferred);}
  auto before=OfflinePropertyDelivery::Detail::Currency(db,a.refunder),after=before;Require(captured.escrow_copper<=uint64_t(INT32_MAX)-before[3],Status::Deferred);after[3]+=uint32_t(captured.escrow_copper);
  if(captured.escrow_copper){One(db,"UPDATE character_currency SET copper="+Id(after[3])+" WHERE id="+Id(a.refunder)+" AND copper="+Id(before[3]));One(db,"UPDATE housing_escrow_balances SET copper=0 WHERE estate_id="+Id(estate)+" AND copper="+Id(captured.escrow_copper));}
  One(db,"INSERT INTO housing_offline_return_finance(estate_id,recipient_character_id,recipient_account_id,refund_copper,currency_before,currency_after,review_reference) VALUES("+Id(estate)+","+Id(a.refunder)+","+Id(a.account)+","+Id(captured.escrow_copper)+",x'"+Hex(OfflinePropertyDelivery::Detail::CurrencyBytes(before))+"',x'"+Hex(OfflinePropertyDelivery::Detail::CurrencyBytes(after))+"',"+Quote(db,a.review)+")");
  One(db,"UPDATE housing_native_estates SET ownership_estate_id=NULL WHERE native_id="+Id(a.native)+" AND ownership_estate_id="+Id(estate)+" AND active=1");One(db,"UPDATE housing_estates SET active=0 WHERE id="+Id(estate)+" AND active=1 AND owner_kind="+Id(a.kind)+" AND owner_id="+Id(a.owner));
  One(db,"INSERT INTO "+std::string(guild?"housing_guild_inactivity_retirements":"housing_inactivity_retirements")+"("+std::string(guild?"guild_id,":"")+"estate_id,native_plot_id,plot_id,character_id,account_id,last_login,retired_epoch,catalog_revision,review_reference) VALUES("+std::string(guild?Id(a.owner)+",":"")+Id(estate)+","+Id(a.native)+","+Id(a.plot)+","+Id(a.refunder)+","+Id(a.account)+","+Id(a.last)+","+Id(now)+","+Id(a.revision)+","+Quote(db,a.inactivity_review)+")");
  ReturnSettlement::Seal(db,captured,dispositions,created,a.review);
  committing=true;auto commit=db.QueryDatabase(std::string("COMMIT"),false);active=false;return commit.Success()?Status::Retired:Status::CommitUncertain;
 }catch(const ReturnParcels::Capacity&){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return committing?Status::CommitUncertain:Status::Deferred;}
 catch(const Stop& e){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return committing?Status::CommitUncertain:e.status;}
 catch(const OfflineDirectReturns::Failure& e){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return committing?Status::CommitUncertain:(e.error==OfflineDirectReturns::Error::DatabaseFailure?Status::DatabaseFailure:Status::Deferred);}
 catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return committing?Status::CommitUncertain:Status::DatabaseFailure;}
}
inline Status Dispatch(Database& db,uint32_t estate,uint32_t now,bool guild){try{if(Detail::Enabled(db,estate))return RetireBatch(db,estate,now,guild);return guild?OfflinePropertyDelivery::RetireGuild(db,estate,now):OfflinePropertyDelivery::Retire(db,estate,now);}catch(...){return Status::DatabaseFailure;}}
inline Status Retire(Database& db,uint32_t estate,uint32_t now){return Dispatch(db,estate,now,false);}
inline Status RetireGuild(Database& db,uint32_t estate,uint32_t now){return Dispatch(db,estate,now,true);}
}
#endif
