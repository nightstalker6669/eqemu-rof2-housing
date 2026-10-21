#ifndef EQEMU_ZONE_HOUSING_HOUSE_CONTENT_LOADER_H
#define EQEMU_ZONE_HOUSING_HOUSE_CONTENT_LOADER_H
#include "housing_house_runtime.h"
#include "../common/json/json.h"
#include <filesystem>
#include <iterator>
#include <fstream>
#include <sstream>
#include <set>
#include <openssl/sha.h>
namespace Housing::HouseRuntime::ContentLoader {
// File/content verification only. Called once by the registered zone startup
// path; neither parser nor registry supplies actor, inventory or ACL authority.
inline bool ReadManifest(const char* filename,bool guild,std::optional<PersonalTestSelection>& result,const char* room_override=nullptr,bool content_only=false){
 std::ifstream input(filename,std::ios::binary);
 if(!input){std::error_code error;const bool present=std::filesystem::exists(filename,error);return !present&&!error;}
 try{
  std::string raw;char buffer[4096];while(input.read(buffer,sizeof buffer)||input.gcount()){
   raw.append(buffer,size_t(input.gcount()));if(raw.size()>65536)return false;
  }
  if(input.bad())return false;
  Json::CharReaderBuilder builder;builder["rejectDupKeys"]=true;builder["failIfExtra"]=true;builder["strictRoot"]=true;
  Json::Value doc;std::string error;std::istringstream text(raw);
  if(!Json::parseFromStream(builder,text,&doc,&error)||!doc.isObject())return false;
  auto number=[&](const char* key){if(!doc[key].isUInt())throw std::invalid_argument("Invalid test manifest number");return doc[key].asUInt();};
  auto string=[&](const char* key){if(!doc[key].isString())throw std::invalid_argument("Invalid test manifest string");return doc[key].asString();};
  if(number("version")!=1||string("client_sha256")!=HouseGeometry::ClientSha256||string("renderer_sha256")!=HouseGeometry::RendererSha256||
   !doc["experimental_no_upkeep"].isBool()||!doc["experimental_no_upkeep"].asBool())return false;
  auto digest=[](const std::string& bytes){std::array<unsigned char,SHA256_DIGEST_LENGTH> sum{};SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),sum.data());
   std::string result;const char* hex="0123456789abcdef";for(auto b:sum){result+=hex[b>>4];result+=hex[b&15];}return result;};
  auto file_digest=[](const char* path){std::ifstream file(path,std::ios::binary);if(!file)throw std::invalid_argument("Missing reviewed house resource");
   SHA256_CTX ctx;SHA256_Init(&ctx);char block[65536];while(file.read(block,sizeof block)||file.gcount())SHA256_Update(&ctx,block,size_t(file.gcount()));
   if(file.bad())throw std::invalid_argument("Unreadable reviewed house resource");std::array<unsigned char,SHA256_DIGEST_LENGTH> sum{};SHA256_Final(sum.data(),&ctx);
   std::string result;const char* hex="0123456789abcdef";for(auto b:sum){result+=hex[b>>4];result+=hex[b&15];}return result;};
  HouseReviewedContent::Entry entry;entry.item_id=guild||content_only||doc.isMember("item_template_id")?number("item_template_id"):61823;auto& route=entry.route;
  const bool explicit_personal=!guild&&(content_only||entry.item_id!=61823);
  route.definition_id=guild||explicit_personal?number("definition_id"):5;route.zone_id=guild||explicit_personal?number("interior_zone_id"):714;route.zone_short_name=guild||explicit_personal?string("interior_short_name"):"phinterior3a1";route.reviewed_server_route=true;
  if(!OriginalInteriors::Matches(route.zone_id,route.zone_short_name)||route.zone_short_name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")!=std::string::npos)return false;
  route.client_asset_sha256=string("interior_asset_sha256");route.collision_map_sha256=string("collision_map_sha256");route.content_manifest_sha256=digest(raw);
  if(file_digest((std::string("housing-test-assets/")+route.zone_short_name+".eqg").c_str())!=route.client_asset_sha256||file_digest((std::string("maps/base/")+route.zone_short_name+".map").c_str())!=route.collision_map_sha256)return false;
  for(const auto* key:{"spawn","exit"}){
   if(!doc[key].isArray()||doc[key].size()!=4)return false;auto& point=std::string(key)=="spawn"?route.spawn:route.exit;
   for(size_t i=0;i<4;++i){if(!doc[key][Json::ArrayIndex(i)].isNumeric())return false;point[i]=doc[key][Json::ArrayIndex(i)].asFloat();}
  }
  PersonalTestSelection s;
  auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::invalid_argument("Invalid manifest hex");};
  if(content_only){
   if(HouseTemplates::Type(entry.item_id)!=(guild?2u:3u)||!room_override||!HouseTemplates::Matches(entry.item_id,route.definition_id,route.zone_id))return false;
   s.item_template_id=entry.item_id;s.experimental_no_upkeep=true;s.template_content_only=true;
   s.content=std::make_shared<const HouseReviewedContent::Policies>(std::vector<HouseReviewedContent::Entry>{entry});
  }else{
  const auto hex=string("adoption_row_hex");if(hex.empty()||hex.size()>600000||hex.size()%2)return false;
  CustodyCodec::Bytes bytes;
  for(size_t i=0;i<hex.size();i+=2)bytes.push_back(uint8_t((digit(hex[i])<<4)|digit(hex[i+1])));
  auto source=CustodyCodec::Decode(bytes);if(source.values.item_id!=entry.item_id||source.values.slot_id<23||source.values.slot_id>33)return false;
  s.item_template_id=entry.item_id;s.guild_id=guild?number("guild_id"):0;if(guild&&!s.guild_id)return false;s.actor={number("character_id"),number("account_id")};s.estate_id=number("estate_id");s.native_plot_id=number("native_plot_id");
  const auto instance=number("neighborhood_instance");if(!instance||instance>32000)return false;s.neighborhood_instance=uint16_t(instance);
  s.catalog_key=string("catalog_key");s.experimental_no_upkeep=true;
  s.content=std::make_shared<const HouseReviewedContent::Policies>(std::vector<HouseReviewedContent::Entry>{entry},std::vector<HouseReviewedContent::Adoption>{{s.actor,source}});
  }
  // Separate optional file: adding room data MUST NOT change the original
  // outer-house manifest digest already bound to the placed house/token.
  const char* room_path=room_override?room_override:guild?".house-guild-room-test.json":".house-room-test.json";std::ifstream room_file(room_path,std::ios::binary);
  if(room_file){
   std::string room_raw;char chunk[4096];while(room_file.read(chunk,sizeof chunk)||room_file.gcount()){room_raw.append(chunk,size_t(room_file.gcount()));if(room_raw.size()>InteriorContent::MaximumManifestBytes)return false;}if(room_file.bad())return false;
   Json::Value room_doc;std::istringstream stream(room_raw);Json::CharReaderBuilder builder;std::string errors;if(!Json::parseFromStream(builder,stream,&room_doc,&errors)||!room_doc.isObject()||!room_doc["schema"].isString()||!room_doc["profile_revision"].isUInt64()||!room_doc["profile_revision"].asUInt64())return false;
   const auto room_schema=room_doc["schema"].asString();if(room_schema!=(guild?"housing-room-guild-v1":"housing-room-personal-v1")&&room_schema!=(guild?"housing-room-guild-test-v1":"housing-room-personal-test-v1"))return false;
   auto payload=[&](const char* key){if(!room_doc[key].isString())throw std::invalid_argument("Missing room payload");auto text=room_doc[key].asString();if(text.empty()||text.size()>2*(std::string(key)=="regions_hex"?RegionWire::MaximumPayload:NeighborhoodWire::MaximumPayload)||text.size()%2)throw std::invalid_argument("Room payload bound");std::vector<uint8_t> out;for(size_t j=0;j<text.size();j+=2)out.push_back(uint8_t((digit(text[j])<<4)|digit(text[j+1])));return out;};
   auto bundle=Catalog::DecodePayloads(payload("definitions_hex"),payload("regions_hex"));if(bundle.definitions.empty()||bundle.definitions.size()>1001||bundle.regions.empty()||!bundle.costs.empty())return false;
   InteriorContent::Profile profile;bool found_definition=false;for(const auto& d:bundle.definitions){if(d.id==route.definition_id){if(found_definition)return false;profile.definition=d;found_definition=true;}else profile.child_definitions.push_back(d);}if(!found_definition||!InteriorContent::ValidChildDefinitions(profile))return false;profile.region=bundle.regions[0];profile.additional_regions.assign(bundle.regions.begin()+1,bundle.regions.end());profile.route={route.zone_id,0,0};profile.zone_short_name=route.zone_short_name;profile.client_asset_sha256=route.client_asset_sha256;profile.collision_map_sha256=route.collision_map_sha256;profile.manifest_sha256=digest(room_raw);
   if(profile.definition.id!=route.definition_id||profile.definition.type!=HouseTemplates::Type(s.item_template_id)||profile.definition.zone_id!=route.zone_id||!profile.definition.area_id||profile.definition.area_id!=profile.region.id||!profile.definition.max_items||profile.definition.max_non_estate_items>profile.definition.max_items)return false;InteriorContent::RegionPayload(profile);
   s.room_profile_revision=room_doc["profile_revision"].asUInt64();s.room_profile=std::make_shared<const InteriorContent::Profile>(std::move(profile));
  }else{std::error_code error;if(content_only||std::filesystem::exists(room_path,error)||error)return false;}
  result=std::move(s);return true;
 }catch(...){return false;}
}
inline bool FileName(const std::string& value){
 return !value.empty()&&value.size()<=128&&value.find("..") == std::string::npos&&value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-")==std::string::npos;
}
inline bool ReadRegistry(const char* filename,std::vector<PersonalTestSelection>& result,bool guild=false){
 std::ifstream input(filename,std::ios::binary);
 if(!input){std::error_code error;return !std::filesystem::exists(filename,error)&&!error;}
 try{
  std::string raw;char block[4096];while(input.read(block,sizeof block)||input.gcount()){raw.append(block,size_t(input.gcount()));if(raw.size()>65536)return false;}if(input.bad())return false;
  Json::Value doc;Json::CharReaderBuilder builder;builder["rejectDupKeys"]=true;builder["failIfExtra"]=true;builder["strictRoot"]=true;std::istringstream stream(raw);std::string error;
  if(!Json::parseFromStream(builder,stream,&doc,&error)||!doc.isObject()||!doc["schema"].isString()||!doc["entries"].isArray()||doc["entries"].empty()||doc["entries"].size()>HouseTemplates::Count)return false;
  const auto registry_schema=doc["schema"].asString();if(registry_schema!=(guild?"housing-guild-content-registry-v1":"housing-personal-content-registry-v1")&&registry_schema!=(guild?"housing-guild-content-registry-test-v1":"housing-personal-content-registry-test-v1"))return false;
  std::vector<PersonalTestSelection> entries;std::set<uint32_t> items;std::set<std::string> manifests,rooms;
  for(const auto& entry:doc["entries"]){
   if(!entry.isObject()||!entry["manifest"].isString()||!entry["room_manifest"].isString())return false;
   const auto manifest=entry["manifest"].asString(),room=entry["room_manifest"].asString();
   if(!FileName(manifest)||!FileName(room)||manifest==room||!manifests.insert(manifest).second||!rooms.insert(room).second)return false;
   std::optional<PersonalTestSelection> content;if(!ReadManifest(manifest.c_str(),guild,content,room.c_str(),true)||!content||!items.insert(content->item_template_id).second||!content->room_profile)return false;
   entries.push_back(std::move(*content));
  }
  result.insert(result.end(),std::make_move_iterator(entries.begin()),std::make_move_iterator(entries.end()));return true;
 }catch(...){return false;}
}
// Validate the whole immutable set before publishing anything. Different
// templates may target the same genuine personal plot; templates are content,
// and the transaction still resolves actual plot/owner/child capacity afresh.
inline bool ValidSelections(const std::vector<PersonalTestSelection>& entries){
 try{
  if(entries.empty()||entries.size()>HouseTemplates::Count)return false;std::set<uint32_t> items;
  for(const auto& s:entries){
   const auto* known=HouseTemplates::Lookup(s.item_template_id);
   if(!known||!items.insert(s.item_template_id).second||!s.content||s.content->Entries().size()!=1||s.content->Entries()[0].item_id!=s.item_template_id||!s.content->Route(s.item_template_id)||!s.experimental_no_upkeep||!HouseGeometry::Lookup(known->item,known->model))return false;
   if(s.template_content_only){if((known->type!=2&&known->type!=3)||s.guild_id||s.actor.character_id||s.actor.account_id||s.estate_id||s.native_plot_id||s.neighborhood_instance||!s.catalog_key.empty()||!s.room_profile||!s.room_profile_revision)return false;}
   else{
    if(!s.actor.character_id||!s.actor.account_id||!s.estate_id||!s.native_plot_id||s.native_plot_id==UINT32_MAX||!s.neighborhood_instance||s.neighborhood_instance>32000||!ContentBinding::Detail::Key(s.catalog_key))return false;
    if(known->type==3){if(s.guild_id||s.catalog_key.rfind("geometryprobe:personal-",0)!=0)return false;}
    else if(!s.guild_id||s.catalog_key.rfind("geometryprobe:guild-",0)!=0)return false;
   }
   if(s.room_profile){const auto& p=*s.room_profile;const auto* route=s.content->Route(s.item_template_id);
    if(p.origin!=Catalog::Origin::Experimental||p.client_build!=20130510||!s.room_profile_revision||p.house_id||p.native_estate_id||p.route.instance_id||p.route.zone_version!=route->zone_version||p.route.zone_id!=route->zone_id||p.definition.id!=route->definition_id||p.definition.type!=known->type||p.definition.zone_id!=route->zone_id||!p.definition.area_id||p.definition.area_id!=p.region.id||!p.definition.max_items||p.definition.max_non_estate_items>p.definition.max_items||p.zone_short_name!=route->zone_short_name||p.client_asset_sha256!=route->client_asset_sha256||p.collision_map_sha256!=route->collision_map_sha256||!ContentBinding::Detail::Hash(p.manifest_sha256)||!InteriorContent::ValidChildDefinitions(p))return false;
    InteriorContent::DefinitionPayload(p);InteriorContent::RegionPayload(p);
   }else if(s.room_profile_revision)return false;
  }
  for(size_t i=0;i<entries.size();++i)for(size_t j=0;j<i;++j){const auto&a=entries[i];const auto&b=entries[j];if(a.template_content_only||b.template_content_only)continue;
   const bool shared=a.estate_id==b.estate_id||a.native_plot_id==b.native_plot_id;
   if(shared&&(a.guild_id!=b.guild_id||a.estate_id!=b.estate_id||a.native_plot_id!=b.native_plot_id||a.neighborhood_instance!=b.neighborhood_instance||a.catalog_key!=b.catalog_key))return false;
   if(bool(a.guild_id)!=bool(b.guild_id)&&a.neighborhood_instance==b.neighborhood_instance)return false;
  }
  return true;
 }catch(...){return false;}
}
}
#endif
