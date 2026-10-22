#ifndef EQEMU_HOUSING_OFFLINE_ROOM_CONTENT_DB_H
#define EQEMU_HOUSING_OFFLINE_ROOM_CONTENT_DB_H
#include "runtime_activation_db.h"
#include "offline_npc_return_db.h"
// Reuse the exact startup file/asset verifier. No Client, online policy, lease,
// or startup-registration entry point is invoked by this offline resolver.
#include "../../zone/housing_house_content_loader.h"
namespace Housing::OfflineRoomContent {
inline InteriorContent::Profile ReadUnderTransaction(Database& db,uint64_t house,uint32_t retiring_estate){
 using namespace OfflineNpcReturn::Detail;NpcNativeFrame::Transaction(db);Require(house&&retiring_estate);
 auto name=Q(db,"SELECT DATABASE()");Require(name.RowCount()==1&&name.begin()[0]);std::string database=name.begin()[0];Require(Housing::RuntimeActivation::AllowsReconstructedContent(db));
 std::vector<HouseRuntime::PersonalTestSelection> entries;std::optional<HouseRuntime::PersonalTestSelection> personal,guild;
 Require(HouseRuntime::ContentLoader::ReadManifest(".house-personal-test.json",false,personal)&&HouseRuntime::ContentLoader::ReadManifest(".house-guild-test.json",true,guild));if(personal)entries.push_back(std::move(*personal));if(guild)entries.push_back(std::move(*guild));
 Require(HouseRuntime::ContentLoader::ReadRegistry(".house-personal-content-registry-test.json",entries)&&HouseRuntime::ContentLoader::ReadRegistry(".house-guild-content-registry-test.json",entries,true)&&HouseRuntime::ContentLoader::ReadRegistry("housing-personal-content-registry.json",entries)&&HouseRuntime::ContentLoader::ReadRegistry("housing-guild-content-registry.json",entries,true)&&HouseRuntime::ContentLoader::ValidSelections(entries));
 auto current=Q(db,"SELECT h.item_template_id,h.pending_request_key,i.native_estate_id,i.definition_id,i.zone_id,i.zone_version,i.instance_id,i.catalog_key,i.catalog_revision,n.active,n.parent_native_id,instances.zone,instances.version,instances.never_expires FROM housing_house_instances h JOIN housing_house_interiors i USING(house_id) JOIN housing_house_locations l ON l.house_id=h.house_id AND l.location_kind=2 JOIN housing_live_item_custody root ON root.custody_id=l.custody_id JOIN housing_native_estates n ON n.native_id=i.native_estate_id JOIN instance_list instances ON instances.id=i.instance_id WHERE h.house_id="+I(house)+" AND root.estate_id="+I(retiring_estate)+" FOR UPDATE");Require(current.RowCount()==1);auto r=current.begin();Require(!r[1]&&N(r[2])&&N(r[2])<UINT32_MAX&&N(r[6])&&N(r[6])<=32000&&r[7]&&N(r[8])&&N(r[9])==1&&N(r[10])&&N(r[11])==N(r[4])&&N(r[12])==N(r[5])&&N(r[13])==1);
 const HouseRuntime::PersonalTestSelection* selected=nullptr;for(const auto& entry:entries)if(entry.item_template_id==N(r[0])){Require(!selected&&entry.room_profile&&entry.room_profile_revision);selected=&entry;}Require(selected);
 auto profile=*selected->room_profile;Require(profile.definition.id==N(r[3])&&profile.route.zone_id==N(r[4])&&profile.route.zone_version==N(r[5]));profile.house_id=house;profile.native_estate_id=uint32_t(N(r[2]));profile.route.instance_id=uint16_t(N(r[6]));
 auto rows=Q(db,"SELECT revision,origin,client_build,zone_short_name,HEX(client_asset_sha256),HEX(collision_map_sha256),HEX(manifest_sha256),HEX(definitions_blob),HEX(regions_blob) FROM housing_house_room_profiles WHERE house_id="+I(house)+" FOR UPDATE");Require(rows.RowCount()==1);auto p=rows.begin();Require(N(p[0])==selected->room_profile_revision&&N(p[1])==uint32_t(profile.origin)&&N(p[2])==profile.client_build&&p[3]&&std::string(p[3])==profile.zone_short_name&&Lower(p[4])==profile.client_asset_sha256&&Lower(p[5])==profile.collision_map_sha256&&Lower(p[6])==profile.manifest_sha256&&Unhex(p[7])==InteriorContent::DefinitionPayload(profile)&&Unhex(p[8])==InteriorContent::RegionPayload(profile));
 Require(InteriorCapacity::ApplyUnderTransaction(db,profile,selected->room_profile_revision));profile.reviewed_room=true;return profile;
}
}
#endif
