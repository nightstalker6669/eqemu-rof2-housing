#ifndef EQEMU_HOUSING_REVIEWED_ROOM_RUNTIME_H
#define EQEMU_HOUSING_REVIEWED_ROOM_RUNTIME_H
#include "scene_service.h"
#include "house_binding_service.h"
#include "interior_profile_db.h"
namespace Housing::ReviewedRoomRuntime {
inline std::string ProfileHex(const std::vector<uint8_t>& bytes){const char* h="0123456789abcdef";std::string out;for(auto b:bytes){out+=h[b>>4];out+=h[b&15];}return out;}
// Instantiate a registered template for a permanent house in its allocating
// parent transaction. Existing profiles are verified, never overwritten.
// This supplies content only; allocator/source/ACL authority remains mandatory.
inline bool EnsureProfileUnderTransaction(Database& db,const HouseBinding::Binding& house,const HouseBinding::Interior& interior,const InteriorContent::Profile& expected,uint64_t revision){
 try{
  if(!house.house_id||!revision||!interior.native_estate_id||interior.native_estate_id==UINT32_MAX||!interior.instance_id||interior.instance_id>32000||interior.definition_id!=expected.definition.id||interior.zone_id!=expected.route.zone_id||interior.zone_version!=expected.route.zone_version||expected.origin!=Catalog::Origin::Experimental||expected.client_build!=20130510||!InteriorContent::ValidChildDefinitions(expected))return false;
  for(const auto& hash:{expected.client_asset_sha256,expected.collision_map_sha256,expected.manifest_sha256})if(!ContentBinding::Detail::Hash(hash))return false;
  auto transaction=db.QueryDatabase(std::string("SELECT @@in_transaction"),false);if(!transaction.Success()||transaction.RowCount()!=1||!transaction.begin()[0]||std::string(transaction.begin()[0])!="1")return false;
  auto engine=db.QueryDatabase(std::string("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_house_room_profiles'"),false);if(!engine.Success()||engine.RowCount()!=1||!engine.begin()[0]||std::string(engine.begin()[0])!="InnoDB")return false;
  const auto definitions=ProfileHex(InteriorContent::DefinitionPayload(expected)),regions=ProfileHex(InteriorContent::RegionPayload(expected));
  const auto id=std::to_string(house.house_id);auto rows=db.QueryDatabase("SELECT revision,origin,client_build,zone_short_name,LOWER(HEX(client_asset_sha256)),LOWER(HEX(collision_map_sha256)),LOWER(HEX(manifest_sha256)),LOWER(HEX(definitions_blob)),LOWER(HEX(regions_blob)) FROM housing_house_room_profiles WHERE house_id="+id+" FOR UPDATE",false);if(!rows.Success()||rows.RowCount()>1)return false;
  if(rows.RowCount()){auto r=rows.begin();for(unsigned n=0;n<9;++n)if(!r[n])return false;return std::stoull(r[0])==revision&&std::stoull(r[1])==2&&std::stoull(r[2])==20130510&&r[3]==expected.zone_short_name&&r[4]==expected.client_asset_sha256&&r[5]==expected.collision_map_sha256&&r[6]==expected.manifest_sha256&&r[7]==definitions&&r[8]==regions;}
  auto inserted=db.QueryDatabase("INSERT INTO housing_house_room_profiles(house_id,revision,origin,client_build,zone_short_name,client_asset_sha256,collision_map_sha256,manifest_sha256,definitions_blob,regions_blob) VALUES ("+id+","+std::to_string(revision)+",2,20130510,'"+db.Escape(expected.zone_short_name)+"',UNHEX('"+expected.client_asset_sha256+"'),UNHEX('"+expected.collision_map_sha256+"'),UNHEX('"+expected.manifest_sha256+"'),UNHEX('"+definitions+"'),UNHEX('"+regions+"'))",false);if(!inserted.Success()||inserted.RowsAffected()!=1)return false;
  auto allocated=expected;allocated.house_id=house.house_id;allocated.native_estate_id=interior.native_estate_id;allocated.route.instance_id=interior.instance_id;
  return InteriorCapacity::InstallForNewProfileUnderTransaction(db,house.item_template_id,allocated,revision);
 }catch(...){return false;}
}
// Content/route recognition only. The caller separately proves its authenticated
// client, held inventory fence and current ACL; creator equality is not access.
// The profile loader calls this before setting reviewed_room, so that output
// flag must never be an input prerequisite for recognizing registered content.
inline bool Matches(const Scene::InteriorSnapshot& scene,const InteriorContent::Profile& p,uint64_t revision,
 uint32_t item,const InteriorContent::Profile& expected,uint64_t expected_revision,Route physical){
 try{
  if(!scene.house||!scene.house->interior||revision!=expected_revision)return false;
  const auto& h=*scene.house;const auto& i=*h.interior;const auto source=scene.exterior_source.value_or(p.route);
  if(h.item_template_id!=item||h.location.kind!=HouseBinding::Location::Kind::Custody||!scene.context.creator_character_id||!scene.context.estate_id||!scene.context.native_plot_id||p.house_id!=h.house_id||p.native_estate_id!=i.native_estate_id||source.zone_id!=physical.zone_id||source.zone_version!=physical.zone_version||source.instance_id!=physical.instance_id||p.route.zone_id!=i.zone_id||p.route.zone_version!=i.zone_version||p.route.instance_id!=i.instance_id||p.route.zone_id!=expected.route.zone_id||p.route.zone_version!=expected.route.zone_version||p.origin!=Catalog::Origin::Experimental||p.client_build!=20130510)return false;
  return p.zone_short_name==expected.zone_short_name&&p.client_asset_sha256==expected.client_asset_sha256&&p.collision_map_sha256==expected.collision_map_sha256&&p.manifest_sha256==expected.manifest_sha256&&InteriorContent::DefinitionPayload(p)==InteriorContent::DefinitionPayload(expected)&&InteriorContent::RegionPayload(p)==InteriorContent::RegionPayload(expected);
 }catch(...){return false;}
}
}
#endif
