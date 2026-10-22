#ifndef EQEMU_HOUSING_OFFLINE_PROPERTY_DELIVERY_DB_H
#define EQEMU_HOUSING_OFFLINE_PROPERTY_DELIVERY_DB_H
#include "inactivity_retirement_db.h"
#include "crate_binding.h"
#include "crate_creation_origin_db.h"
#include "crate_house_rules.h"
#include "static_furniture_registry.h"
#include "house_geometry_registry.h"
#include "inventory_row_db.h"
#include "item_payload_db.h"
#include "house_inventory_transition.h"
#include "item_storage_db.h"
#include "../inventory_profile.h"
#include <set>
namespace Housing::OfflinePropertyDelivery {
using Status=InactivityRetirement::Status;
namespace Detail {
using InactivityRetirement::Detail::Q;using InactivityRetirement::Detail::N;
using InactivityRetirement::Detail::Quote;using InactivityRetirement::Detail::Empty;
using InactivityRetirement::Detail::Stop;
inline std::string Id(uint64_t n){return std::to_string(n);}
inline std::string Hex(const std::vector<uint8_t>& b){const char* h="0123456789ABCDEF";std::string s;for(auto c:b){s+=h[c>>4];s+=h[c&15];}return s;}
inline std::string Hex(const std::string& b){return Hex(std::vector<uint8_t>(b.begin(),b.end()));}
inline void One(Database& db,const std::string& q){if(Q(db,q).RowsAffected()!=1)throw Stop{Status::Conflict};}
inline bool Installed(Database& db){
 auto optional=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_inactivity_delivery_policy','housing_inactivity_deliveries')");
 if(!optional.RowCount())return false;if(optional.RowCount()!=2)throw Stop{Status::DatabaseFailure};
 for(auto r=optional.begin();r!=optional.end();++r)if(!r[0]||std::string(r[0])!="InnoDB")throw Stop{Status::DatabaseFailure};
 if(!InactivityRetirement::Detail::Installed(db))throw Stop{Status::DatabaseFailure};
 const std::vector<std::string> tables={"inventory","character_currency","items","housing_crate_instances","housing_crate_locations","housing_crate_estates","housing_crate_members","housing_house_instances","housing_house_locations","housing_house_interiors","housing_house_item_scopes","housing_live_item_custody","housing_live_item_storage","housing_live_placed_items","housing_live_placement_reservations","housing_live_vault_items","housing_live_npc_frames","housing_inactivity_retirements","housing_interior_placement_reservations","housing_house_room_profiles","instance_list","items_evolving_details","character_evolving_items"};
 std::string names;for(const auto& t:tables){if(!names.empty())names+=",";names+=Quote(db,t);}auto r=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ("+names+")");if(r.RowCount()!=1||N(r.begin()[0])!=tables.size())throw Stop{Status::DatabaseFailure};
 return true;
}
inline constexpr const char* CurrencyColumns="platinum,gold,silver,copper,platinum_bank,gold_bank,silver_bank,copper_bank,platinum_cursor,gold_cursor,silver_cursor,copper_cursor,radiant_crystals,career_radiant_crystals,ebon_crystals,career_ebon_crystals";
inline std::array<uint32_t,16> Currency(Database& db,uint32_t owner){auto r=Q(db,"SELECT "+std::string(CurrencyColumns)+" FROM character_currency WHERE id="+Id(owner)+" FOR UPDATE");if(r.RowCount()!=1)throw Stop{Status::Deferred};std::array<uint32_t,16> a{};for(size_t i=0;i<a.size();++i){auto n=N(r.begin()[i]);if(n>(i<12?uint64_t(INT32_MAX):uint64_t(UINT32_MAX)))throw Stop{Status::Deferred};a[i]=uint32_t(n);}return a;}
inline std::vector<uint8_t> CurrencyBytes(const std::array<uint32_t,16>& a){std::vector<uint8_t> b;for(auto n:a)for(unsigned i=0;i<4;++i)b.push_back(uint8_t(n>>(8*i)));return b;}
// Bounded, deterministic length-prefixed text/HEX witness. No delimiter or NULL
// ambiguity; all binary columns are selected as HEX. Order is primary-key order.
inline void Append(Database& db,std::string& out,const std::string& label,const std::string& query,unsigned columns){
 auto add=[&](const std::string& s){out+=Id(s.size())+":"+s;if(out.size()>64*1024*1024)throw Stop{Status::Deferred};};
 add(label);auto r=Q(db,query+" FOR UPDATE");add(Id(r.RowCount()));
 for(auto p=r.begin();p!=r.end();++p)for(unsigned c=0;c<columns;++c){if(!p[c])out+="N";else{out+="V";add(p[c]);}}
}
inline std::string Cargo(Database& db,uint64_t crate,uint32_t owner,const std::vector<uint64_t>& additional={},bool payload_only=false){
 const auto id=Id(crate);std::string members="SELECT m.custody_id FROM housing_crate_members m WHERE m.crate_id="+id;for(auto custody:additional)members+=" UNION SELECT "+Id(custody);
 const std::string houses="SELECT h.house_id FROM housing_house_locations h WHERE h.location_kind=2 AND h.custody_id IN ("+members+")";
 const std::string custody="("+members+" UNION SELECT s.custody_id FROM housing_house_item_scopes s WHERE s.house_id IN ("+houses+"))";
 Empty(db,"SELECT custody_id FROM housing_live_item_custody WHERE custody_id IN "+custody+" AND owner_character_id<>"+Id(owner));
 Empty(db,"SELECT house_id FROM housing_house_instances WHERE house_id IN ("+houses+") AND (created_by_character_id<>"+Id(owner)+" OR pending_request_key IS NOT NULL)");
 Empty(db,"SELECT v.custody_id FROM housing_live_vault_items v WHERE v.house_id IN ("+houses+")");
 Empty(db,"SELECT sessions.character_id FROM housing_house_interiors i JOIN housing_inventory_sessions sessions ON sessions.zone_id=i.zone_id AND sessions.instance_id=i.instance_id AND sessions.active=1 WHERE i.house_id IN ("+houses+")");
 std::string out="housing-offline-cargo-v1;";
 if(!payload_only)Append(db,out,"members","SELECT custody_id,crate_id,revision,origin_native_estate_id,origin_house_id,HEX(origin_pose_blob),origin_stowed FROM housing_crate_members WHERE crate_id="+id+" ORDER BY custody_id",7);
 Append(db,out,"custody","SELECT custody_id,estate_id,owner_character_id,HEX(schema_descriptor),HEX(row_blob),HEX(catalog_blob) FROM housing_live_item_custody WHERE custody_id IN "+custody+" ORDER BY custody_id",6);
 auto payloads=Q(db,"SELECT custody_id,HEX(row_blob) FROM housing_live_item_custody WHERE custody_id IN "+custody+" ORDER BY custody_id LIMIT 1001 FOR UPDATE");if(payloads.RowCount()>1000)throw Stop{Status::Deferred};for(auto p=payloads.begin();p!=payloads.end();++p){auto row=CustodyCodec::Decode(ItemPayload::Unhex(p[1]));ItemPayload::Metadata(db,row);ItemPayload::ValidateHeld(db,row,N(p[0]));}
 if(!payload_only)Append(db,out,"storage","SELECT custody_id,revision,stowed,native_estate_id FROM housing_live_item_storage WHERE custody_id IN "+custody+" ORDER BY custody_id",4);
 Append(db,out,"poses","SELECT custody_id,native_item_id,pose_revision,HEX(pose_blob) FROM housing_live_placed_items WHERE custody_id IN "+custody+" ORDER BY custody_id",4);
 Append(db,out,"reservations","SELECT native_item_id,native_estate_id,estate_id,HEX(catalog_key),catalog_revision,character_id,account_id,lease_epoch,HEX(session_key),HEX(source_row),runtime_serial,state,custody_id,content_binding_version,HEX(content_binding_digest) FROM housing_live_placement_reservations WHERE custody_id IN "+custody+" ORDER BY native_item_id",15);
 Append(db,out,"room-scopes","SELECT custody_id,house_id,revision FROM housing_house_item_scopes WHERE house_id IN ("+houses+") ORDER BY custody_id",3);
 Append(db,out,"house-locations","SELECT house_id,location_kind,character_id,slot_id,custody_id FROM housing_house_locations WHERE house_id IN ("+houses+") ORDER BY house_id",5);
 Append(db,out,"house-identities","SELECT house_id,revision,item_template_id,created_by_character_id,HEX(pending_request_key) FROM housing_house_instances WHERE house_id IN ("+houses+") ORDER BY house_id",5);
 Append(db,out,"room-profiles","SELECT house_id,revision,origin,client_build,HEX(zone_short_name),HEX(client_asset_sha256),HEX(collision_map_sha256),HEX(manifest_sha256),HEX(definitions_blob),HEX(regions_blob) FROM housing_house_room_profiles WHERE house_id IN ("+houses+") ORDER BY house_id",10);
 Append(db,out,"interiors","SELECT house_id,native_estate_id,zone_id,zone_version,instance_id,HEX(catalog_key),catalog_revision FROM housing_house_interiors WHERE house_id IN ("+houses+") ORDER BY house_id",7);
 return out;
}
// A picked-up house retains its permanent interior/native identity for later
// reparenting. It is not an installed child of its historical plot. Recognize
// only a complete, empty, quiescent carried binding; never deactivate the node.
inline void CheckRetirementChildren(Database& db,uint32_t parent,const std::string& allowed_custody={}){
 auto children=Q(db,"SELECT native_id,node_key,definition_id,catalog_key FROM housing_native_estates WHERE parent_native_id="+Id(parent)+" AND active=1 ORDER BY native_id FOR UPDATE");
 for(auto child=children.begin();child!=children.end();++child){
  const auto native=N(child[0]);
  if(!allowed_custody.empty()&&Q(db,"SELECT i.house_id FROM housing_house_interiors i JOIN housing_house_locations l ON l.house_id=i.house_id AND l.location_kind=2 WHERE i.native_estate_id="+Id(native)+" AND l.custody_id IN ("+allowed_custody+") FOR UPDATE").RowCount()==1)continue;
  auto found=Q(db,"SELECT h.house_id,h.item_template_id,h.pending_request_key,l.location_kind,l.character_id,l.slot_id,l.custody_id,i.definition_id,i.zone_id,i.zone_version,i.instance_id,i.catalog_key,i.catalog_revision FROM housing_house_interiors i JOIN housing_house_instances h USING(house_id) JOIN housing_house_locations l USING(house_id) WHERE i.native_estate_id="+Id(native)+" FOR UPDATE");
  if(found.RowCount()!=1)throw Stop{Status::Deferred};auto r=found.begin();const auto house=N(r[0]),item=N(r[1]),owner=N(r[4]),slot=N(r[5]);
  if(r[2]||N(r[3])!=1||r[6]||!owner||owner>UINT32_MAX||slot>UINT32_MAX||!child[1]||std::string(child[1])!="house:"+Id(house)||N(child[2])!=N(r[7])||!child[3]||!r[11]||std::string(child[3])!=r[11]||!N(r[12]))throw Stop{Status::Deferred};
  // Session acquisition locks this character row; fence a foreign carried owner too.
  if(Q(db,"SELECT id FROM character_data WHERE id="+Id(owner)+" AND deleted_at IS NULL FOR UPDATE").RowCount()!=1)throw Stop{Status::Deferred};
  auto inventory=Q(db,"SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+Id(owner)+" AND slot_id="+Id(slot)+" FOR UPDATE");if(inventory.RowCount()!=1)throw Stop{Status::Deferred};auto carried=InventoryRowDB::Decode(inventory.begin());
  if(!CustodyCodec::Valid(carried)||carried.values.item_id!=item||(carried.values.charges!=0&&carried.values.charges!=1))throw Stop{Status::Deferred};
  auto metadata=Q(db,"SELECT idfile,UNK227,evoitem FROM items WHERE id="+Id(item)+" FOR UPDATE");if(metadata.RowCount()!=1||!metadata.begin()[0]||!HouseGeometry::Lookup(uint32_t(item),metadata.begin()[0])||N(metadata.begin()[1])!=N(r[7])||N(metadata.begin()[2]))throw Stop{Status::Deferred};
  if(Q(db,"SELECT house_id FROM housing_house_room_profiles WHERE house_id="+Id(house)+" FOR UPDATE").RowCount()!=1)throw Stop{Status::Deferred};
  auto instance=Q(db,"SELECT id FROM instance_list WHERE id="+Id(N(r[10]))+" AND zone="+Id(N(r[8]))+" AND version="+Id(N(r[9]))+" AND never_expires=1 FOR UPDATE");if(instance.RowCount()!=1)throw Stop{Status::Deferred};
  for(const auto& query:{"SELECT native_item_id FROM housing_live_placement_reservations WHERE native_estate_id="+Id(native)+" AND state=0","SELECT custody_id FROM housing_live_item_storage WHERE native_estate_id="+Id(native),"SELECT custody_id FROM housing_house_item_scopes WHERE house_id="+Id(house),"SELECT custody_id FROM housing_live_vault_items WHERE house_id="+Id(house),"SELECT p.native_item_id FROM housing_interior_placement_reservations p LEFT JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE p.house_id="+Id(house)+" AND (r.native_item_id IS NULL OR r.state<>2 OR r.custody_id IS NOT NULL)","SELECT native_id FROM housing_native_estates WHERE parent_native_id="+Id(native)+" AND active=1","SELECT character_id FROM housing_inventory_sessions WHERE (character_id="+Id(owner)+" OR (zone_id="+Id(N(r[8]))+" AND instance_id="+Id(N(r[10]))+")) AND active=1","SELECT character_id FROM housing_live_item_leases WHERE character_id="+Id(owner)+" AND (active=1 OR pending_key IS NOT NULL)","SELECT character_id FROM housing_house_inventory_pending WHERE character_id="+Id(owner)})Empty(db,query);
 }
}
// Only the trusted offline retirement transaction calls this helper, after
// locking its actual inactive owner, generation and exact delivery review.
// Review absence retains the prior packed-crate-only behavior.
struct Packing {uint32_t count=0;std::string before,after,before_allocation,after_allocation,review;};
inline Packing PackLoose(Database& db,uint32_t estate,uint32_t owner,const LiveItems::Stored& root,const CrateBinding::Allocation& crate,bool guild=false){
 Packing out;auto schema=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_inactivity_packing_policy','housing_inactivity_packings')");
 if(!schema.RowCount())return out;if(schema.RowCount()!=2)throw Stop{Status::DatabaseFailure};for(auto r=schema.begin();r!=schema.end();++r)if(!r[0]||std::string(r[0])!="InnoDB")throw Stop{Status::DatabaseFailure};
 const auto& b=crate.binding;
 auto policy=Q(db,"SELECT review_reference FROM housing_inactivity_packing_policy WHERE estate_id="+Id(estate)+" AND native_plot_id="+Id(b.parent_native_estate_id)+" AND catalog_revision="+Id(b.catalog_revision)+" AND mode=1 FOR UPDATE");
 if(!policy.RowCount())return out;if(policy.RowCount()!=1||!policy.begin()[0]||!policy.begin()[0][0])throw Stop{Status::Conflict};out.review=policy.begin()[0];
 auto candidates=Q(db,"SELECT p.custody_id,HEX(p.pose_blob) FROM housing_live_placed_items p JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id AND r.state=1 AND r.custody_id=p.custody_id WHERE r.native_estate_id="+Id(b.parent_native_estate_id)+" AND p.custody_id<>"+Id(root.custody_id)+" AND NOT EXISTS (SELECT 1 FROM housing_crate_members m WHERE m.custody_id=p.custody_id) ORDER BY p.custody_id LIMIT 1001 FOR UPDATE");
 if(!candidates.RowCount())return out;if(candidates.RowCount()>1000||b.revision>=UINT64_MAX-candidates.RowCount())throw Stop{Status::Deferred};
 auto definition=CrateBinding::ReadDefinitionUnderTransaction(db,crate);auto members=CrateBinding::ValidateMembersUnderTransaction(db,crate);
 if(!definition||!members||uint64_t(members.value)+candidates.RowCount()>definition.value.max_items)throw Stop{Status::Deferred};
 struct Candidate {uint64_t custody,storage_revision;bool stowed;std::string pose;};std::vector<Candidate> plan;std::vector<uint64_t> ids;
 auto houses=CrateHouseRules::LockedMembers(db,b.crate_id);
 for(auto r=candidates.begin();r!=candidates.end();++r){
  const auto id=N(r[0]);auto inspected=LiveItems::Service::InspectCustodyUnderTransaction(db,id);if(!inspected)throw Stop{Status::Deferred};const auto& s=inspected.value;
  if(s.estate_id!=estate||s.row.values.character_id!=owner||!s.placement||s.placement->npc_frame||s.placement->reservation.native_estate_id!=b.parent_native_estate_id||s.placement->reservation.estate_id!=estate||s.placement->reservation.catalog_key!=b.catalog_key||s.placement->reservation.catalog_revision!=b.catalog_revision||!CustodyCodec::Valid(s.row))throw Stop{Status::Deferred};
  const auto& v=s.row.values;if(s.row.null_fields&LiveItems::NullItemId||CrateGeometry::TemplateDefinition(v.item_id))throw Stop{Status::Deferred};
  Empty(db,"SELECT custody_id FROM housing_house_item_scopes WHERE custody_id="+Id(id));Empty(db,"SELECT custody_id FROM housing_live_npc_frames WHERE custody_id="+Id(id));Empty(db,"SELECT custody_id FROM housing_live_vault_items WHERE custody_id="+Id(id));
  auto metadata=Q(db,"SELECT idfile,itemclass,bagslots,loregroup,nodrop,evoitem FROM items WHERE id="+Id(v.item_id)+" FOR UPDATE");if(metadata.RowCount()!=1)throw Stop{Status::Deferred};auto m=metadata.begin();
  if(!m[0]||N(m[1])||N(m[2])||std::stoll(m[3])!=s.catalog.lore_group||std::stoll(m[4])!=s.catalog.no_drop||s.catalog.item_class||s.catalog.bag_slots)throw Stop{Status::Deferred};
  const auto* house=HouseGeometry::Lookup(v.item_id,m[0]);if((house&&HouseTemplates::Type(v.item_id)!=(guild?2u:3u))||(!house&&!StaticFurniture::Lookup(v.item_id,m[0])))throw Stop{Status::Deferred};
  ItemPayload::Metadata(db,s.row);ItemPayload::ValidateHeld(db,s.row,id);
  auto house_row=Q(db,"SELECT h.house_id,h.created_by_character_id,h.item_template_id,h.pending_request_key FROM housing_house_instances h JOIN housing_house_locations l ON l.house_id=h.house_id WHERE l.location_kind=2 AND l.custody_id="+Id(id)+" FOR UPDATE");
  if(house){if(house_row.RowCount()!=1||N(house_row.begin()[1])!=owner||N(house_row.begin()[2])!=v.item_id||house_row.begin()[3])throw Stop{Status::Deferred};houses.push_back(CrateHouseRules::LockedHouse(db,N(house_row.begin()[0])));}else if(house_row.RowCount())throw Stop{Status::Deferred};
  auto storage=ItemStorage::Locked(db,id);if(storage.revision==UINT64_MAX||(storage.revision&&storage.native_estate_id!=b.parent_native_estate_id)||!r[1]||std::string(r[1]).size()!=56)throw Stop{Status::Deferred};
  ids.push_back(id);plan.push_back({id,storage.revision,storage.stowed,r[1]});
 }
 if(!CrateHouseRules::Allowed(houses))throw Stop{Status::Deferred};
 out.before=Cargo(db,b.crate_id,owner,ids);const auto payload=Cargo(db,b.crate_id,owner,ids,true);out.before_allocation=Hex(CrateBinding::EncodeAllocation(crate));
 for(const auto& c:plan){
  One(db,"INSERT INTO housing_crate_members(custody_id,crate_id,revision,origin_native_estate_id,origin_house_id,origin_pose_blob,origin_stowed) VALUES("+Id(c.custody)+","+Id(b.crate_id)+",1,"+Id(b.parent_native_estate_id)+",NULL,x'"+c.pose+"',"+Id(c.stowed)+")");
  if(c.storage_revision)One(db,"UPDATE housing_live_item_storage SET stowed=1,revision=revision+1 WHERE custody_id="+Id(c.custody)+" AND native_estate_id="+Id(b.parent_native_estate_id)+" AND revision="+Id(c.storage_revision));
  else One(db,"INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+Id(c.custody)+",1,1,"+Id(b.parent_native_estate_id)+")");
 }
 One(db,"UPDATE housing_crate_instances SET revision=revision+"+Id(plan.size())+" WHERE crate_id="+Id(b.crate_id)+" AND revision="+Id(b.revision)+" AND packed=0");
 auto after=crate.inventory_character_id?CrateBinding::ReadCarriedUnderTransaction(db,root.row):CrateBinding::ReadPlacedUnderTransaction(db,root);if(!after)throw Stop{Status::Conflict};auto expected=crate;expected.binding.revision+=plan.size();if(CrateBinding::EncodeAllocation(after.value)!=CrateBinding::EncodeAllocation(expected)||Cargo(db,b.crate_id,owner,{},true)!=payload)throw Stop{Status::Conflict};
 out.count=uint32_t(plan.size());out.after=Cargo(db,b.crate_id,owner);out.after_allocation=Hex(CrateBinding::EncodeAllocation(after.value));return out;
}
inline uint32_t Destination(Database& db,uint32_t owner){
 auto columns=InventoryRepository::ColumnsRaw();columns.erase(std::remove(columns.begin(),columns.end(),' '),columns.end());if(columns!=InventoryRowDB::WriteColumns)throw Stop{Status::DatabaseFailure};
 auto rows=Q(db,"SELECT slot_id FROM inventory WHERE character_id="+Id(owner)+" ORDER BY slot_id FOR UPDATE");std::set<uint32_t> occupied;for(auto r=rows.begin();r!=rows.end();++r)occupied.insert(uint32_t(N(r[0])));
 auto bound=Q(db,"SELECT slot_id FROM housing_crate_locations WHERE location_kind=1 AND character_id="+Id(owner)+" UNION SELECT slot_id FROM housing_house_locations WHERE location_kind=1 AND character_id="+Id(owner)+" FOR UPDATE");for(auto r=bound.begin();r!=bound.end();++r)occupied.insert(uint32_t(N(r[0])));
 for(uint32_t s=23;s<=30;++s){if(occupied.count(s))continue;bool clear=true;for(unsigned i=0;i<10;++i)if(occupied.count(EQ::InventoryProfile::CalcSlotId(int16_t(s),uint8_t(i))))clear=false;if(clear)return s;}throw Stop{Status::Deferred};
}
struct Created {bool made=false;CrateBinding::Allocation initial;LiveItems::InventoryRow row;std::string review;};
// No simulated placed item, custody receipt or Client reservation. The new
// permanent typed crate starts carried; GUID0 uses the normal inventory loader.
inline Created CreateCarried(Database& db,uint32_t estate,uint32_t owner,uint32_t native,const std::string& catalog,uint64_t revision,const char* reviewed_manifest,bool guild=false){
 Created out;auto schema=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_inactivity_crate_creation_policy','housing_inactivity_crate_creations')");if(!schema.RowCount())return out;if(schema.RowCount()!=2)throw Stop{Status::DatabaseFailure};for(auto r=schema.begin();r!=schema.end();++r)if(!r[0]||std::string(r[0])!="InnoDB")throw Stop{Status::DatabaseFailure};
 auto column=Q(db,"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_inactivity_deliveries' AND column_name='source_kind'");if(column.RowCount()!=1||N(column.begin()[0])!=1)throw Stop{Status::DatabaseFailure};
 auto policy=Q(db,"SELECT HEX(crate_manifest_sha256),review_reference FROM housing_inactivity_crate_creation_policy WHERE estate_id="+Id(estate)+" AND native_plot_id="+Id(native)+" AND catalog_revision="+Id(revision)+" AND mode=1 FOR UPDATE");if(!policy.RowCount())return out;if(policy.RowCount()!=1||!policy.begin()[0]||!reviewed_manifest||std::string(policy.begin()[0])!=reviewed_manifest||!policy.begin()[1]||!policy.begin()[1][0])throw Stop{Status::Deferred};out.review=policy.begin()[1];
 // Empty property refunds never mint a gratuitous crate.
 auto loose=Q(db,"SELECT p.custody_id FROM housing_live_placed_items p JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id AND r.state=1 AND r.custody_id=p.custody_id WHERE r.native_estate_id="+Id(native)+" LIMIT 1 FOR UPDATE");if(!loose.RowCount())return out;
 auto packing=Q(db,"SELECT estate_id FROM housing_inactivity_packing_policy WHERE estate_id="+Id(estate)+" AND native_plot_id="+Id(native)+" AND catalog_revision="+Id(revision)+" AND mode=1 FOR UPDATE");if(packing.RowCount()!=1)throw Stop{Status::Deferred};
 auto item=Q(db,"SELECT idfile,itemclass,bagslots,stackable,UNK227,UNK226,maxcharges,evoitem,loregroup FROM items WHERE id=62586 FOR UPDATE");if(item.RowCount()!=1)throw Stop{Status::Deferred};auto m=item.begin();if(!m[0]||std::string(m[0])!="IT20028"||N(m[1])||N(m[2])||N(m[3])||N(m[4])!=174||N(m[5])||std::stoll(m[6])||N(m[7])||std::stoll(m[8]))throw Stop{Status::Deferred};Empty(db,"SELECT id FROM items_evolving_details WHERE item_id=62586");Empty(db,"SELECT id FROM character_evolving_items WHERE character_id="+Id(owner)+" AND item_id=62586 AND deleted_at IS NULL");
 out.row.values=InventoryRepository::NewEntity();auto& v=out.row.values;v.character_id=owner;v.slot_id=Destination(db,owner);v.item_id=62586;v.charges=0;v.guid=0;v.custom_data.clear();out.row.null_fields=0;
 const std::string key=std::string(guild?"offline-guild-inactivity:":"offline-inactivity:")+Id(estate);if(guild)CrateCreationOrigin::OfflineGuild(db,key,estate,owner);else CrateCreationOrigin::Offline(db,key,estate);auto made=Q(db,"INSERT INTO housing_crate_instances(item_template_id,created_by_character_id,creation_request_key,content_manifest_sha256) VALUES(62586,"+Id(owner)+","+Quote(db,key)+",x'"+std::string(reviewed_manifest)+"')");const auto crate=made.LastInsertedID();if(made.RowsAffected()!=1||!crate)throw Stop{Status::Conflict};
 auto parent=Q(db,"SELECT neighborhood_id FROM housing_native_estates WHERE native_id="+Id(native)+" AND ownership_estate_id="+Id(estate)+" AND active=1 FOR UPDATE");if(parent.RowCount()!=1)throw Stop{Status::Conflict};
 auto allocated=Q(db,"INSERT INTO housing_native_estates(catalog_key,node_key,parent_native_id,neighborhood_id,definition_id,static_id,active) VALUES("+Quote(db,catalog)+","+Quote(db,"crate:"+Id(crate))+","+Id(native)+","+Id(N(parent.begin()[0]))+",174,0,0)");const auto child=allocated.LastInsertedID();if(allocated.RowsAffected()!=1||!child||child>=UINT32_MAX)throw Stop{Status::Conflict};
 One(db,"INSERT INTO inventory("+std::string(InventoryRowDB::WriteColumns)+") VALUES("+InventoryRowDB::Values(out.row,v.slot_id)+")");
 One(db,"INSERT INTO housing_crate_locations(crate_id,location_kind,character_id,slot_id,custody_id) VALUES("+Id(crate)+",1,"+Id(owner)+","+Id(v.slot_id)+",NULL)");
 One(db,"INSERT INTO housing_crate_estates(crate_id,native_estate_id,parent_native_estate_id,ownership_estate_id,definition_id,catalog_key,catalog_revision) VALUES("+Id(crate)+","+Id(child)+","+Id(native)+","+Id(estate)+",174,"+Quote(db,catalog)+","+Id(revision)+")");
 auto read=CrateBinding::ReadCarriedUnderTransaction(db,out.row);if(!read||!CrateBinding::ReadDefinitionUnderTransaction(db,read.value))throw Stop{Status::Conflict};out.initial=read.value;out.made=true;return out;
}

}
// Server-owned offline disposition, independent of Client lease/fence journals.
// Supported: empty funded plot, or one existing placed small moving crate whose
// full belongings are already packed, or separately reviewed yard packing.
// A separate review can create one carried crate when absent. Guild mode uses
// separate inactivity/recipient authority and journals. Foreign belongings,
// vaults and parcel conversion remain unsupported.
inline Status RetireImpl(Database& db,uint32_t estate,uint32_t now,bool guild){
 using namespace Detail;bool active=false,committing=false;
 try{
 const uint32_t inactive_seconds=guild?2592000u:InactivityRetirement::InactiveSeconds;
 if(!estate||now<inactive_seconds||!Installed(db))return Status::NotDue;
 auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0]))return Status::Conflict;
 Q(db,"START TRANSACTION");active=true;
 const auto personal_query="SELECT e.plot_id,e.owner_id,c.account_id,c.last_login,n.native_id,n.definition_id,cat.revision,p.review_reference,HEX(cat.definitions_blob),HEX(cat.regions_blob),HEX(d.crate_manifest_sha256),d.review_reference FROM housing_current_estates e JOIN character_data c ON c.id=e.owner_id AND c.deleted_at IS NULL JOIN account a ON a.id=c.account_id JOIN housing_native_estates n ON n.ownership_estate_id=e.id AND n.plot_id=e.plot_id AND n.active=1 JOIN housing_catalogs cat ON cat.catalog_key=n.catalog_key JOIN housing_inactivity_policy p ON p.estate_id=e.id AND p.native_plot_id=n.native_id AND p.catalog_revision=cat.revision AND p.inactive_seconds=7776000 JOIN housing_inactivity_delivery_policy d ON d.estate_id=e.id AND d.native_plot_id=n.native_id AND d.catalog_revision=cat.revision AND d.mode=1 WHERE e.id="+Id(estate)+" AND e.owner_kind=1 AND cat.client_build=20130510 AND cat.origin IN (1,2) FOR UPDATE";
 const auto guild_query="SELECT e.plot_id,p.recipient_character_id,c.account_id,c.last_login,n.native_id,n.definition_id,cat.revision,p.review_reference,HEX(cat.definitions_blob),HEX(cat.regions_blob),HEX(d.crate_manifest_sha256),d.review_reference,e.owner_id FROM housing_current_estates e JOIN housing_guild_inactivity_policy p ON p.estate_id=e.id JOIN guilds g ON g.id=e.owner_id AND g.leader=p.recipient_character_id JOIN guild_members leader ON leader.guild_id=g.id AND leader.char_id=g.leader JOIN character_data c ON c.id=p.recipient_character_id AND c.deleted_at IS NULL JOIN account a ON a.id=c.account_id JOIN housing_native_estates n ON n.ownership_estate_id=e.id AND n.plot_id=e.plot_id AND n.active=1 AND n.native_id=p.native_plot_id JOIN housing_catalogs cat ON cat.catalog_key=n.catalog_key AND cat.revision=p.catalog_revision AND p.definitions_sha256=UNHEX(SHA2(cat.definitions_blob,256)) AND p.regions_sha256=UNHEX(SHA2(cat.regions_blob,256)) JOIN housing_inactivity_delivery_policy d ON d.estate_id=e.id AND d.native_plot_id=n.native_id AND d.catalog_revision=cat.revision AND d.mode=1 WHERE e.id="+Id(estate)+" AND e.owner_kind=3 AND p.inactive_seconds=2592000 AND cat.client_build=20130510 AND cat.origin IN (1,2) FOR UPDATE";
 if(guild){
  auto schema=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_guild_inactivity_policy','housing_guild_inactivity_retirements','housing_guild_inactivity_deliveries','housing_guild_inactivity_packings','housing_guild_inactivity_crate_creations','guilds','guild_members')");if(schema.RowCount()!=1||N(schema.begin()[0])!=7)throw Stop{Status::DatabaseFailure};
  auto identity=Q(db,"SELECT owner_id FROM housing_current_estates WHERE id="+Id(estate)+" AND owner_kind=3");if(identity.RowCount()!=1)throw Stop{Status::NotDue};
  auto locked=Q(db,"SELECT id FROM guilds WHERE id="+Id(N(identity.begin()[0]))+" FOR UPDATE");if(locked.RowCount()!=1)throw Stop{Status::Deferred};
 }
 auto row=Q(db,guild?guild_query:personal_query);
 if(!row.RowCount())throw Stop{Status::NotDue};if(row.RowCount()!=1)throw Stop{Status::Conflict};auto r=row.begin();
 const auto plot=N(r[0]),owner=N(r[1]),account=N(r[2]),native=N(r[4]),revision=N(r[6]);uint64_t last=N(r[3]);const uint64_t owner_id=guild?N(r[12]):owner;
 if(guild){
  auto members=Q(db,"SELECT m.char_id,c.last_login,c.deleted_at FROM guild_members m LEFT JOIN character_data c ON c.id=m.char_id WHERE m.guild_id="+Id(owner_id)+" ORDER BY m.char_id FOR UPDATE");if(!members.RowCount())throw Stop{Status::Deferred};last=0;
  for(auto member=members.begin();member!=members.end();++member){if(!member[1]||member[2]||!N(member[1]))throw Stop{Status::Deferred};last=std::max(last,N(member[1]));}
  Empty(db,"SELECT s.character_id FROM housing_inventory_sessions s JOIN guild_members m ON m.char_id=s.character_id WHERE s.active=1 AND m.guild_id="+Id(owner_id));
 }
 if(!plot||!owner||owner>UINT32_MAX||!account||!native||native>=UINT32_MAX||!revision||!r[7]||!r[7][0]||!r[11]||!r[11][0])throw Stop{Status::Conflict};
 if(!last||last>uint64_t(now)-inactive_seconds)throw Stop{Status::NotDue};
 auto bundle=Catalog::DecodePayloads(UpkeepSelection::Detail::Unhex(r[8]),UpkeepSelection::Detail::Unhex(r[9]));auto definition=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==N(r[5]);});if(definition==bundle.definitions.end()||definition->type!=(guild?8u:4u))throw Stop{Status::Conflict};
 // Account/character row locks serialize real inventory-session acquisition.
 // Pending foreign returns require a global item-application barrier as well.
 Empty(db,"SELECT character_id FROM housing_live_item_leases WHERE pending_key IS NOT NULL");
 Empty(db,"SELECT character_id FROM housing_inventory_sessions WHERE character_id="+Id(owner)+" AND active=1");
 Empty(db,"SELECT character_id FROM housing_live_item_leases WHERE character_id="+Id(owner)+" AND (active=1 OR pending_key IS NOT NULL)");
 if(CoinPending::Inspect(db,uint32_t(owner),true)!=CoinPending::State::Clear)throw Stop{Status::Deferred};
 const auto inventory_pending=HouseInventory::InspectPending(db,uint32_t(owner),true);if(inventory_pending!=HouseInventory::PendingState::Clear)throw Stop{inventory_pending==HouseInventory::PendingState::Pending?Status::Deferred:Status::DatabaseFailure};
 if(Q(db,"SELECT id FROM housing_plot_definitions WHERE id="+Id(plot)+" FOR UPDATE").RowCount()!=1)throw Stop{Status::Conflict};
 Empty(db,"SELECT estate_id FROM "+std::string(guild?"housing_guild_inactivity_retirements":"housing_inactivity_retirements")+" WHERE estate_id="+Id(estate));
 Empty(db,"SELECT instance_key FROM housing_fixture_item_custody WHERE estate_id="+Id(estate));
 Empty(db,"SELECT native_item_id FROM housing_live_placement_reservations WHERE (estate_id="+Id(estate)+" OR native_estate_id="+Id(native)+") AND state=0");
 Empty(db,"SELECT p.request_key FROM housing_live_escrow_pending p JOIN housing_live_escrow_receipts r ON r.request_key=p.request_key WHERE r.estate_id="+Id(estate));
 Empty(db,"SELECT custody_id FROM housing_live_vault_items WHERE realm_estate_id="+Id(estate));
 auto balance=Q(db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+Id(estate)+" FOR UPDATE");if(balance.RowCount()>1)throw Stop{Status::Conflict};const auto refund=balance.RowCount()?N(balance.begin()[0]):0;
 auto before_money=Currency(db,uint32_t(owner));if(refund>uint64_t(INT32_MAX)-before_money[3])throw Stop{Status::Deferred};auto after_money=before_money;after_money[3]+=uint32_t(refund);
 auto roots=Q(db,"SELECT l.custody_id FROM housing_crate_locations l JOIN housing_crate_estates e ON e.crate_id=l.crate_id WHERE e.parent_native_estate_id="+Id(native)+" AND l.location_kind=2 FOR UPDATE");if(roots.RowCount()>1)throw Stop{Status::Deferred};
 std::string cargo="housing-offline-cargo-v1;",before_root,after_root;uint64_t crate=0,root_id=0;uint32_t slot=0;LiveItems::InventoryRow delivered;std::optional<CrateBinding::Allocation> allocation;Packing packing;Created created;
 if(roots.RowCount()){
  root_id=N(roots.begin()[0]);auto inspected=LiveItems::Service::InspectCustodyUnderTransaction(db,root_id);if(!inspected)throw Stop{Status::Deferred};const auto& stored=inspected.value;
  if(stored.estate_id!=estate||stored.row.values.character_id!=owner||!stored.placement||stored.stowed||stored.row.values.item_id!=62586||stored.row.null_fields&LiveItems::NullGuid||!stored.row.values.guid||stored.row.values.guid>INT32_MAX||stored.catalog.item_class||stored.catalog.bag_slots)throw Stop{Status::Deferred};
  auto storage=ItemStorage::Locked(db,root_id);if(storage.revision&&storage.native_estate_id!=native)throw Stop{Status::Deferred};
  const auto& v=stored.row.values;Empty(db,"SELECT slot_id FROM inventory WHERE guid="+Id(v.guid));Empty(db,"SELECT id FROM items_evolving_details WHERE item_id=62586");Empty(db,"SELECT id FROM character_evolving_items WHERE character_id="+Id(owner)+" AND item_id=62586 AND deleted_at IS NULL");if(v.augment_one||v.augment_two||v.augment_three||v.augment_four||v.augment_five||v.augment_six)throw Stop{Status::Deferred};
  auto item=Q(db,"SELECT itemclass,bagslots,evoitem,idfile,loregroup,nodrop FROM items WHERE id=62586 FOR UPDATE");if(item.RowCount()!=1||N(item.begin()[0])||N(item.begin()[1])||N(item.begin()[2])||!item.begin()[3]||std::string(item.begin()[3])!="IT20028"||std::stoll(item.begin()[4])!=stored.catalog.lore_group||std::stoll(item.begin()[5])!=stored.catalog.no_drop)throw Stop{Status::Deferred};
  auto read=CrateBinding::ReadPlacedUnderTransaction(db,stored);if(!read)throw Stop{Status::Deferred};allocation=read.value;const auto& a=*allocation;const auto& b=a.binding;crate=b.crate_id;
  if(a.created_by_character_id!=owner||a.packed||b.parent_native_estate_id!=native||b.estate_id!=estate||b.catalog_revision!=revision||b.definition_id!=174||b.revision==UINT64_MAX||!r[10]||std::string(r[10])!=EstateRetirement::Detail::Hex(a.content_manifest_sha256))throw Stop{Status::Deferred};
  if(!CrateBinding::ReadDefinitionUnderTransaction(db,a))throw Stop{Status::Deferred};
  Empty(db,"SELECT custody_id FROM housing_house_item_scopes WHERE custody_id="+Id(root_id));
  Empty(db,"SELECT custody_id FROM housing_live_npc_frames WHERE custody_id="+Id(root_id));
  packing=PackLoose(db,estate,uint32_t(owner),stored,*allocation,guild);if(packing.count){auto packed=CrateBinding::ReadPlacedUnderTransaction(db,stored);if(!packed)throw Stop{Status::Conflict};allocation=packed.value;}
  const auto allowed="SELECT m.custody_id FROM housing_crate_members m WHERE m.crate_id="+Id(crate);
  Empty(db,"SELECT c.custody_id FROM housing_live_item_custody c WHERE c.estate_id="+Id(estate)+" AND c.custody_id<>"+Id(root_id)+" AND c.custody_id NOT IN ("+allowed+") AND NOT EXISTS (SELECT 1 FROM housing_house_item_scopes s JOIN housing_house_locations h ON h.house_id=s.house_id AND h.location_kind=2 WHERE s.custody_id=c.custody_id AND h.custody_id IN ("+allowed+"))");
  Empty(db,"SELECT n.native_id FROM housing_native_estates n WHERE n.parent_native_id="+Id(native)+" AND n.active=1 AND n.native_id<>"+Id(b.native_estate_id)+" AND NOT EXISTS (SELECT 1 FROM housing_house_interiors i JOIN housing_house_locations h ON h.house_id=i.house_id AND h.location_kind=2 WHERE i.native_estate_id=n.native_id AND h.custody_id IN ("+allowed+"))");
  Empty(db,"SELECT custody_id FROM housing_live_item_storage WHERE native_estate_id="+Id(native)+" AND custody_id<>"+Id(root_id)+" AND custody_id NOT IN ("+allowed+")");
  Empty(db,"SELECT p.custody_id FROM housing_live_placed_items p JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE r.native_estate_id="+Id(native)+" AND p.custody_id<>"+Id(root_id)+" AND p.custody_id NOT IN ("+allowed+")");
  cargo=Cargo(db,crate,uint32_t(owner));slot=Destination(db,uint32_t(owner));delivered=stored.row;delivered.values.character_id=uint32_t(owner);delivered.values.slot_id=slot;
  before_root=Hex(CrateBinding::EncodeAllocation(a))+":"+Hex(CustodyCodec::Encode(stored.row));
  Append(db,before_root,"root-custody","SELECT custody_id,estate_id,owner_character_id,HEX(schema_descriptor),HEX(row_blob),HEX(catalog_blob) FROM housing_live_item_custody WHERE custody_id="+Id(root_id),6);
  Append(db,before_root,"root-pose","SELECT custody_id,native_item_id,pose_revision,HEX(pose_blob) FROM housing_live_placed_items WHERE custody_id="+Id(root_id),4);
  Append(db,before_root,"root-storage","SELECT custody_id,revision,stowed,native_estate_id FROM housing_live_item_storage WHERE custody_id="+Id(root_id),4);
  One(db,"INSERT INTO inventory("+std::string(InventoryRowDB::WriteColumns)+") VALUES("+InventoryRowDB::Values(delivered,slot)+")");
  One(db,"UPDATE housing_crate_locations SET location_kind=1,character_id="+Id(owner)+",slot_id="+Id(slot)+",custody_id=NULL WHERE crate_id="+Id(crate)+" AND location_kind=2 AND custody_id="+Id(root_id));
  One(db,"UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+Id(crate)+" AND revision="+Id(b.revision)+" AND packed=0");
  One(db,"UPDATE housing_native_estates SET active=0 WHERE native_id="+Id(b.native_estate_id)+" AND parent_native_id="+Id(native)+" AND active=1");
  if(stored.storage_revision)One(db,"DELETE FROM housing_live_item_storage WHERE custody_id="+Id(root_id)+" AND revision="+Id(stored.storage_revision));
  One(db,"DELETE FROM housing_live_placed_items WHERE custody_id="+Id(root_id)+" AND native_item_id="+Id(stored.placement->reservation.native_item_id));
  One(db,"UPDATE housing_live_placement_reservations SET state=2,custody_id=NULL WHERE native_item_id="+Id(stored.placement->reservation.native_item_id)+" AND state=1 AND custody_id="+Id(root_id));
  One(db,"DELETE FROM housing_live_item_custody WHERE custody_id="+Id(root_id));
 }else{
  auto cat=Q(db,"SELECT catalog_key FROM housing_native_estates WHERE native_id="+Id(native)+" FOR UPDATE");if(cat.RowCount()!=1||!cat.begin()[0])throw Stop{Status::Conflict};
  created=CreateCarried(db,estate,uint32_t(owner),uint32_t(native),cat.begin()[0],revision,r[10],guild);
  if(created.made){
   delivered=created.row;slot=delivered.values.slot_id;crate=created.initial.binding.crate_id;allocation=created.initial;LiveItems::Stored carrier;carrier.row=delivered;
   packing=PackLoose(db,estate,uint32_t(owner),carrier,*allocation,guild);if(!packing.count)throw Stop{Status::Deferred};
   auto packed=CrateBinding::ReadCarriedUnderTransaction(db,delivered);if(!packed)throw Stop{Status::Conflict};allocation=packed.value;
   const auto allowed="SELECT m.custody_id FROM housing_crate_members m WHERE m.crate_id="+Id(crate);
   Empty(db,"SELECT c.custody_id FROM housing_live_item_custody c WHERE c.estate_id="+Id(estate)+" AND c.custody_id NOT IN ("+allowed+") AND NOT EXISTS (SELECT 1 FROM housing_house_item_scopes s JOIN housing_house_locations h ON h.house_id=s.house_id AND h.location_kind=2 WHERE s.custody_id=c.custody_id AND h.custody_id IN ("+allowed+"))");
   Empty(db,"SELECT custody_id FROM housing_live_item_storage WHERE native_estate_id="+Id(native)+" AND custody_id NOT IN ("+allowed+")");
   Empty(db,"SELECT p.custody_id FROM housing_live_placed_items p JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE r.native_estate_id="+Id(native)+" AND p.custody_id NOT IN ("+allowed+")");
   Empty(db,"SELECT n.native_id FROM housing_native_estates n WHERE n.parent_native_id="+Id(native)+" AND n.active=1 AND NOT EXISTS (SELECT 1 FROM housing_house_interiors i JOIN housing_house_locations h ON h.house_id=i.house_id AND h.location_kind=2 WHERE i.native_estate_id=n.native_id AND h.custody_id IN ("+allowed+"))");
   cargo=Cargo(db,crate,uint32_t(owner));
  }else{
  Empty(db,"SELECT custody_id FROM housing_live_item_custody WHERE estate_id="+Id(estate));
  Empty(db,"SELECT custody_id FROM housing_live_item_storage WHERE native_estate_id="+Id(native));
  Empty(db,"SELECT p.custody_id FROM housing_live_placed_items p JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE r.native_estate_id="+Id(native));
  CheckRetirementChildren(db,uint32_t(native));
  }
 }
 if(refund){One(db,"UPDATE character_currency SET copper="+Id(after_money[3])+" WHERE id="+Id(owner)+" AND copper="+Id(before_money[3]));One(db,"UPDATE housing_escrow_balances SET copper=0 WHERE estate_id="+Id(estate)+" AND copper="+Id(refund));}
 if(Currency(db,uint32_t(owner))!=after_money)throw Stop{Status::Conflict};
 One(db,"UPDATE housing_native_estates SET ownership_estate_id=NULL WHERE native_id="+Id(native)+" AND ownership_estate_id="+Id(estate)+" AND plot_id="+Id(plot)+" AND active=1");
 One(db,"UPDATE housing_estates SET active=0 WHERE id="+Id(estate)+" AND owner_kind="+Id(guild?3:1)+" AND owner_id="+Id(owner_id)+" AND active=1");
 if(allocation){auto actual=CrateBinding::ReadCarriedUnderTransaction(db,delivered);if(!actual)throw Stop{Status::Conflict};auto expected=*allocation;if(!created.made)expected.binding.revision++;expected.binding.custody_id=0;expected.inventory_character_id=uint32_t(owner);expected.inventory_slot_id=slot;
 if(CrateBinding::EncodeAllocation(actual.value)!=CrateBinding::EncodeAllocation(expected)||Cargo(db,crate,uint32_t(owner))!=cargo)throw Stop{Status::Conflict};after_root=Hex(CrateBinding::EncodeAllocation(actual.value))+":"+Hex(CustodyCodec::Encode(delivered));}
 One(db,"INSERT INTO "+std::string(guild?"housing_guild_inactivity_retirements":"housing_inactivity_retirements")+"("+std::string(guild?"guild_id,":"")+"estate_id,native_plot_id,plot_id,character_id,account_id,last_login,retired_epoch,catalog_revision,review_reference) VALUES("+std::string(guild?Id(owner_id)+",":"")+Id(estate)+","+Id(native)+","+Id(plot)+","+Id(owner)+","+Id(account)+","+Id(last)+","+Id(now)+","+Id(revision)+","+Quote(db,r[7])+")");
 const auto cargo_hex=Hex(cargo);auto packet=Q(db,"SELECT @@max_allowed_packet");if(packet.RowCount()!=1||cargo_hex.size()*2+before_root.size()*2+after_root.size()*2+65536>N(packet.begin()[0]))throw Stop{Status::Deferred};
 One(db,"INSERT INTO "+std::string(guild?"housing_guild_inactivity_deliveries":"housing_inactivity_deliveries")+"("+std::string(created.made?"source_kind,":"")+"estate_id,character_id,crate_id,source_custody_id,destination_slot,refund_copper,currency_before,currency_after,root_before,root_after,cargo_blob,cargo_sha256,review_reference) VALUES("+std::string(created.made?"2,":"")+Id(estate)+","+Id(owner)+","+(crate?Id(crate):"NULL")+","+(root_id?Id(root_id):"NULL")+","+(slot?Id(slot):"NULL")+","+Id(refund)+",x'"+Hex(CurrencyBytes(before_money))+"',x'"+Hex(CurrencyBytes(after_money))+"',x'"+Hex(before_root)+"',x'"+Hex(after_root)+"',x'"+cargo_hex+"',UNHEX(SHA2(x'"+cargo_hex+"',256)),"+Quote(db,r[11])+")");
 if(packing.count){
  const auto before=Hex(packing.before),after=Hex(packing.after);auto limit=Q(db,"SELECT @@max_allowed_packet");if(limit.RowCount()!=1||before.size()*2+after.size()*2+65536>N(limit.begin()[0]))throw Stop{Status::Deferred};
  One(db,"INSERT INTO "+std::string(guild?"housing_guild_inactivity_packings":"housing_inactivity_packings")+"(estate_id,crate_id,member_count,before_allocation,after_allocation,before_blob,after_blob,before_sha256,after_sha256,review_reference) VALUES("+Id(estate)+","+Id(crate)+","+Id(packing.count)+",x'"+Hex(packing.before_allocation)+"',x'"+Hex(packing.after_allocation)+"',x'"+before+"',x'"+after+"',UNHEX(SHA2(x'"+before+"',256)),UNHEX(SHA2(x'"+after+"',256)),"+Quote(db,packing.review)+")");
 }
 if(created.made)One(db,"INSERT INTO "+std::string(guild?"housing_guild_inactivity_crate_creations":"housing_inactivity_crate_creations")+"(estate_id,crate_id,native_estate_id,inventory_row,initial_allocation,delivered_allocation,review_reference) VALUES("+Id(estate)+","+Id(crate)+","+Id(created.initial.binding.native_estate_id)+",x'"+Hex(CustodyCodec::Encode(delivered))+"',x'"+Hex(CrateBinding::EncodeAllocation(created.initial))+"',x'"+Hex(CrateBinding::EncodeAllocation(*allocation))+"',"+Quote(db,created.review)+")");
 committing=true;auto commit=db.QueryDatabase(std::string("COMMIT"),false);active=false;return commit.Success()?Status::Retired:Status::CommitUncertain;
 }catch(const Stop& e){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return committing?Status::CommitUncertain:e.status;}
 catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return committing?Status::CommitUncertain:Status::DatabaseFailure;}
}
inline Status Retire(Database& db,uint32_t estate,uint32_t now){return RetireImpl(db,estate,now,false);}
inline Status RetireGuild(Database& db,uint32_t estate,uint32_t now){return RetireImpl(db,estate,now,true);}
inline InactivityRetirement::Batch ProcessGuildDue(Database& db,uint32_t now,uint32_t& cursor,Status (*delivery)(Database&,uint32_t,uint32_t)=nullptr){
 using namespace Detail;InactivityRetirement::Batch out;
 auto schema=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_guild_inactivity_policy'");if(!schema.RowCount())return out;if(schema.RowCount()!=1||!schema.begin()[0]||std::string(schema.begin()[0])!="InnoDB")throw Stop{Status::DatabaseFailure};
 auto read=[&](){return Q(db,"SELECT e.id FROM housing_guild_inactivity_policy p JOIN housing_current_estates e ON e.id=p.estate_id AND e.owner_kind=3 WHERE e.id>"+Id(cursor)+" ORDER BY e.id LIMIT 100");};
 auto rows=read();if(!rows.RowCount()&&cursor){cursor=0;rows=read();}
 for(auto r=rows.begin();r!=rows.end();++r){auto id=N(r[0]);if(!id||id>UINT32_MAX)throw Stop{Status::DatabaseFailure};cursor=uint32_t(id);auto result=delivery?delivery(db,cursor,now):RetireGuild(db,cursor,now);if(result==Status::Retired){++out.retired;out.refresh=true;}else if(result==Status::CommitUncertain){++out.deferred;out.refresh=true;}else if(result!=Status::NotDue)++out.deferred;}
 return out;
}
}
#endif
