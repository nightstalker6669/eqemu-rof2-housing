#include "runtime_activation_db.h"
#ifndef EQEMU_HOUSING_INTERIOR_PROFILE_UPGRADE_H
#define EQEMU_HOUSING_INTERIOR_PROFILE_UPGRADE_H
#include "interior_item_content_db.h"
#include "reviewed_room_runtime.h"
#include "item_storage_db.h"
#include "custody_codec.h"

namespace Housing::InteriorContent {
struct UpgradeResult {
 bool accepted=false,replayed=false;
 uint64_t active_items=0;
 std::string error;
 explicit operator bool()const{return accepted;}
};
// Maintenance only: caller must stop the actual server processes, retain a
// backup, and own the surrounding SQL transaction. The database checks below
// establish quiescent ledgers, not proof that an OS process has stopped. This
// function is deliberately not wired to a client opcode or gameplay handler.
// Only region/manifest registration changes; custody, poses, scopes, permanent
// identities and historical receipts/reservations remain byte-for-byte intact.
inline UpgradeResult UpgradeRegionsUnderTransaction(Database& db,const Profile& before,
 const Profile& after,uint64_t before_revision,uint64_t after_revision){
 bool savepoint=false;
 try{
  auto require=[](bool ok,const char* error){if(!ok)throw std::invalid_argument(error);};
  auto query=[&](const std::string& sql){auto r=db.QueryDatabase(sql,false);require(r.Success(),"Profile upgrade query failed");return r;};
  auto number=[](const char* value){if(!value)throw std::invalid_argument("Missing upgrade numeric field");size_t end=0;std::string s=value;auto n=std::stoull(s,&end);if(end!=s.size())throw std::invalid_argument("Invalid upgrade numeric field");return n;};
  auto unhex=[](const char* value){if(!value)throw std::invalid_argument("Missing upgrade bytes");std::string s=value;if(s.size()%2||s.size()>std::max(size_t(262600),2*RegionWire::MaximumPayload))throw std::invalid_argument("Invalid upgrade bytes");std::vector<uint8_t> out;auto digit=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;throw std::invalid_argument("Invalid upgrade hex");};for(size_t i=0;i<s.size();i+=2)out.push_back((digit(s[i])<<4)|digit(s[i+1]));return out;};
  auto tx=query("SELECT @@in_transaction,DATABASE()");require(tx.RowCount()==1&&number(tx.begin()[0])==1&&tx.begin()[1],"Profile upgrade needs owning transaction");
  std::string schema=tx.begin()[1];require(Housing::RuntimeActivation::AllowsReconstructedContent(db),"Experimental profile schema required");
  require(before_revision&&before_revision!=UINT64_MAX&&after_revision==before_revision+1,"Profile revision must advance once");
  require(before.house_id&&before.house_id==after.house_id&&before.native_estate_id==after.native_estate_id&&before.route.zone_id==after.route.zone_id&&before.route.zone_version==after.route.zone_version&&before.route.instance_id==after.route.instance_id&&before.zone_short_name==after.zone_short_name&&before.client_asset_sha256==after.client_asset_sha256&&before.collision_map_sha256==after.collision_map_sha256&&before.origin==Catalog::Origin::Experimental&&after.origin==before.origin&&before.client_build==20130510&&after.client_build==before.client_build&&before.region.id==after.region.id,"Profile identity/assets changed");
  require(OriginalInteriors::Matches(before.route.zone_id,before.zone_short_name)&&ContentBinding::Detail::RouteShape(before.route)&&before.native_estate_id&&before.native_estate_id!=UINT32_MAX,"Invalid profile route");
  require((before.definition.type==2||before.definition.type==3)&&before.definition.zone_id==before.route.zone_id&&before.definition.max_items&&before.definition.max_non_estate_items<=before.definition.max_items,"Invalid room definition");
  for(const auto& hash:{before.client_asset_sha256,before.collision_map_sha256,before.manifest_sha256,after.manifest_sha256})require(ContentBinding::Detail::Hash(hash),"Invalid profile hash");
  require(before.manifest_sha256!=after.manifest_sha256&&DefinitionPayload(before)==DefinitionPayload(after)&&before.definition.area_id==before.region.id,"Profile definitions changed or manifest not advanced");
  const auto old_regions=RegionPayload(before),new_regions=RegionPayload(after);
  require(old_regions!=new_regions,"No region change");
  for(const auto* table:{"housing_house_room_profiles","housing_interior_placement_reservations","housing_live_placement_reservations","housing_live_placed_items","housing_live_item_custody","housing_live_item_storage","housing_inventory_sessions","housing_inventory_processes","housing_live_item_leases","housing_house_instances","housing_house_interiors","housing_house_inventory_pending"}){
   auto engine=query(std::string("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='")+table+"'");require(engine.RowCount()==1&&engine.begin()[0]&&std::string(engine.begin()[0])=="InnoDB","Upgrade needs transactional schema");
  }
  for(const auto& sql:{"SELECT 1 FROM housing_inventory_sessions WHERE active=1 LIMIT 1 FOR UPDATE","SELECT 1 FROM housing_inventory_processes WHERE reaped=0 LIMIT 1 FOR UPDATE","SELECT 1 FROM housing_live_item_leases WHERE active=1 OR pending_key IS NOT NULL LIMIT 1 FOR UPDATE","SELECT 1 FROM housing_house_instances WHERE pending_request_key IS NOT NULL LIMIT 1 FOR UPDATE","SELECT 1 FROM housing_house_inventory_pending LIMIT 1 FOR UPDATE"})require(query(sql).RowCount()==0,"Profile upgrade requires quiescent runtime");
  const auto id=std::to_string(before.house_id);
  // Capacity policies pin the immutable region bytes. A region migration must
  // not strand that separate history; require an explicit reviewed rebase.
  if(InteriorCapacity::Detail::Installed(db))require(query("SELECT revision FROM housing_house_capacity_heads WHERE house_id="+id+" FOR UPDATE").RowCount()==0,"Capacity policy requires explicit region rebase");
  auto interior=query("SELECT native_estate_id,zone_id,zone_version,instance_id,definition_id FROM housing_house_interiors WHERE house_id="+id+" FOR UPDATE");require(interior.RowCount()==1,"Missing permanent interior");auto i=interior.begin();require(number(i[0])==before.native_estate_id&&number(i[1])==before.route.zone_id&&number(i[2])==before.route.zone_version&&number(i[3])==before.route.instance_id&&number(i[4])==before.definition.id,"Permanent interior mismatch");
  auto rows=query("SELECT revision,origin,client_build,zone_short_name,LOWER(HEX(client_asset_sha256)),LOWER(HEX(collision_map_sha256)),LOWER(HEX(manifest_sha256)),HEX(definitions_blob),HEX(regions_blob) FROM housing_house_room_profiles WHERE house_id="+id+" FOR UPDATE");require(rows.RowCount()==1,"Missing current profile");auto row=rows.begin();for(unsigned n=0;n<9;++n)require(row[n],"Incomplete current profile");
  auto matches=[&](const Profile& p,uint64_t revision){return number(row[0])==revision&&number(row[1])==2&&number(row[2])==p.client_build&&row[3]==p.zone_short_name&&row[4]==p.client_asset_sha256&&row[5]==p.collision_map_sha256&&row[6]==p.manifest_sha256&&unhex(row[7])==DefinitionPayload(p)&&unhex(row[8])==RegionPayload(p);};
  const bool replay=matches(after,after_revision);require(replay||matches(before,before_revision),"Stale current profile");
  require(query("SELECT 1 FROM housing_interior_placement_reservations t JOIN housing_live_placement_reservations r ON r.native_item_id=t.native_item_id WHERE t.house_id="+id+" AND r.state=0 LIMIT 1 FOR UPDATE").RowCount()==0,"Uncommitted room preview remains");
  auto active=query("SELECT t.native_item_id,t.profile_revision,t.content_version,HEX(t.content_digest),t.source_zone,t.source_version,t.source_instance,t.house_revision,r.custody_id,r.native_estate_id,r.character_id,HEX(r.source_row),c.owner_character_id,c.schema_descriptor,HEX(c.row_blob),p.native_item_id,HEX(p.pose_blob),p.pose_revision FROM housing_interior_placement_reservations t JOIN housing_live_placement_reservations r ON r.native_item_id=t.native_item_id LEFT JOIN housing_live_item_custody c ON c.custody_id=r.custody_id LEFT JOIN housing_live_placed_items p ON p.custody_id=r.custody_id WHERE t.house_id="+id+" AND r.state=1 ORDER BY t.native_item_id FOR UPDATE");
  struct Change {uint64_t native=0;std::string old_digest,new_digest;};std::vector<Change> changes;
  auto old_profile=replay?after:before;old_profile.reviewed_room=true;auto next_profile=after;next_profile.reviewed_room=true;
  for(auto r=active.begin();r.operator!=(active.end());++r){
   for(unsigned n=0;n<18;++n)require(r[n],"Incomplete active room item");
   require(number(r[1])==(replay?after_revision:before_revision)&&number(r[2])==Version&&number(r[4])==before.route.zone_id&&number(r[5])==before.route.zone_version&&number(r[6])==before.route.instance_id&&number(r[7])&&number(r[8])&&number(r[9])==before.native_estate_id&&number(r[0])==number(r[15])&&number(r[17]),"Active room witness mismatch");
   auto raw=unhex(r[14]);auto item=CustodyCodec::Decode(raw);require(CustodyCodec::Valid(item)&&raw==CustodyCodec::Encode(item)&&raw==unhex(r[11])&&number(r[10])==item.values.character_id&&number(r[12])==item.values.character_id&&std::string(r[13])==LiveItems::Service::SchemaDescriptor(),"Active room custody mismatch");
   auto input=ItemInputUnderTransaction(db,old_profile,item.values.item_id);require(bool(input),"Unsupported active room content");auto old=Build(input.value); /* Typed wall tags deliberately fail the exact legacy digest below; Return before profile-wide migration. */require(bool(old)&&unhex(r[3])==std::vector<uint8_t>(old.value.digest.begin(),old.value.digest.end()),"Active room digest mismatch");
   auto pose_bytes=unhex(r[16]);require(pose_bytes.size()==28,"Invalid active room pose");std::array<float,7> values{};for(size_t k=0;k<7;++k){uint32_t bits=0;for(unsigned n=0;n<4;++n)bits|=uint32_t(pose_bytes[4*k+n])<<(8*n);std::memcpy(&values[k],&bits,4);require(std::isfinite(values[k]),"Nonfinite active room pose");}PlacementPolicy::Transform pose{values[0],values[1],values[2],values[3],{values[4],values[5],values[6]}};
   auto stored=ItemStorage::Locked(db,number(r[8]));require(!stored.revision||stored.native_estate_id==before.native_estate_id,"Active room storage mismatch");
   if(!stored.stowed){PlacementPolicy::Context context{before.native_estate_id,before.definition.type,before.region.id,{6,AccessPolicy::KnownCapabilities,true},true};require(bool(CheckPlacement(old_profile,context,input.value.item,input.value.geometry,pose,PlacementPolicy::Operation::Add))&&bool(CheckPlacement(next_profile,context,input.value.item,input.value.geometry,pose,PlacementPolicy::Operation::Add)),"New regions exclude placed content");}
   input.value.room=next_profile;auto next=Build(input.value);require(bool(next),"Invalid new room content binding");changes.push_back({number(r[0]),ReviewedRoomRuntime::ProfileHex(std::vector<uint8_t>(old.value.digest.begin(),old.value.digest.end())),ReviewedRoomRuntime::ProfileHex(std::vector<uint8_t>(next.value.digest.begin(),next.value.digest.end()))});
  }
  if(replay)return {true,true,uint64_t(changes.size()),{}};
  query("SAVEPOINT housing_room_region_upgrade");savepoint=true;
  auto updated=query("UPDATE housing_house_room_profiles SET revision="+std::to_string(after_revision)+",manifest_sha256=UNHEX('"+after.manifest_sha256+"'),regions_blob=UNHEX('"+ReviewedRoomRuntime::ProfileHex(new_regions)+"') WHERE house_id="+id+" AND revision="+std::to_string(before_revision));require(updated.RowsAffected()==1,"Profile upgrade CAS failed");
  for(const auto& change:changes){auto result=query("UPDATE housing_interior_placement_reservations SET profile_revision="+std::to_string(after_revision)+",content_digest=UNHEX('"+change.new_digest+"') WHERE native_item_id="+std::to_string(change.native)+" AND house_id="+id+" AND profile_revision="+std::to_string(before_revision)+" AND content_version=3 AND content_digest=UNHEX('"+change.old_digest+"')");require(result.RowsAffected()==1,"Room binding upgrade CAS failed");}
  query("RELEASE SAVEPOINT housing_room_region_upgrade");savepoint=false;return {true,false,uint64_t(changes.size()),{}};
 }catch(const std::exception& e){if(savepoint){db.QueryDatabase(std::string("ROLLBACK TO SAVEPOINT housing_room_region_upgrade"),false);db.QueryDatabase(std::string("RELEASE SAVEPOINT housing_room_region_upgrade"),false);}return {false,false,0,e.what()};}
}
}
#endif
