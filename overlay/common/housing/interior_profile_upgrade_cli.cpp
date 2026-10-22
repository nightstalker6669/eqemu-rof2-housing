#include "offline_process_guard.h"
#include "runtime_activation_db.h"
// Offline maintenance helper. No gameplay endpoint, process control or schema
// changes. Deployment must back up state and coordinate manifest/binary updates.
#include "interior_profile_upgrade.h"
#include "../eqemu_config.h"
#include "../platform.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
const EQEmuConfig* Config=nullptr;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
uint64_t Number(const char* value){Require(value&&*value,"Missing numeric field");std::string text=value;for(char c:text)Require(c>='0'&&c<='9',"Invalid numeric field");return std::stoull(text);}
std::string Hash(const std::string& bytes){std::array<unsigned char,SHA256_DIGEST_LENGTH> sum{};SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),sum.data());return Housing::ReviewedRoomRuntime::ProfileHex({sum.begin(),sum.end()});}
std::vector<uint8_t> Unhex(const std::string& value,size_t maximum){Require(!value.empty()&&value.size()%2==0&&value.size()<=2*maximum,"Invalid manifest payload");std::vector<uint8_t> bytes;auto digit=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("Invalid manifest hex");};for(size_t n=0;n<value.size();n+=2)bytes.push_back((digit(value[n])<<4)|digit(value[n+1]));return bytes;}
struct Manifest {Housing::InteriorContent::Profile profile;uint64_t revision=0;};
Manifest Read(const char* path,const Housing::InteriorContent::Profile& identity){
 std::ifstream file(path,std::ios::binary);Require(bool(file),"Manifest missing");std::string raw;char chunk[4096];while(file.read(chunk,sizeof chunk)||file.gcount()){raw.append(chunk,size_t(file.gcount()));Require(raw.size()<=Housing::InteriorContent::MaximumManifestBytes,"Manifest too large");}Require(!file.bad(),"Manifest read failed");
 Json::Value doc;Json::CharReaderBuilder builder;builder["rejectDupKeys"]=true;builder["failIfExtra"]=true;builder["strictRoot"]=true;std::string errors;std::istringstream text(raw);Require(Json::parseFromStream(builder,text,&doc,&errors)&&doc.isObject(),"Manifest parse failed");
 Require(doc["schema"].isString()&&doc["schema"].asString()==(identity.definition.type==2?"housing-room-guild-test-v1":"housing-room-personal-test-v1")&&doc["profile_revision"].isUInt64()&&doc["profile_revision"].asUInt64(),"Manifest schema/revision mismatch");Require(doc["definitions_hex"].isString()&&doc["regions_hex"].isString(),"Manifest payload missing");
 auto bundle=Housing::Catalog::DecodePayloads(Unhex(doc["definitions_hex"].asString(),Housing::NeighborhoodWire::MaximumPayload),Unhex(doc["regions_hex"].asString(),Housing::RegionWire::MaximumPayload));Require(!bundle.regions.empty()&&bundle.costs.empty(),"Invalid room payload");Manifest result{identity,doc["profile_revision"].asUInt64()};auto& p=result.profile;bool found=false;
 for(const auto& definition:bundle.definitions){if(definition.id==identity.definition.id){Require(!found,"Duplicate room definition");p.definition=definition;found=true;}else p.child_definitions.push_back(definition);}Require(found,"Missing room definition");p.region=bundle.regions.front();p.additional_regions.assign(bundle.regions.begin()+1,bundle.regions.end());p.manifest_sha256=Hash(raw);Housing::InteriorContent::DefinitionPayload(p);Housing::InteriorContent::RegionPayload(p);return result;
}
void RequireStopped(){Housing::OfflineProcessGuard::RequireWorldAndZoneStopped();}
}
int main(int argc,char** argv){
 std::unique_ptr<Database> db;bool transaction=false;
 try{
  Require(argc==5,"Usage: house-id before-manifest after-manifest --inspect|--dry-run|--apply");std::vector<uint64_t> houses;std::set<uint64_t> unique;std::istringstream ids(argv[1]);std::string id;while(std::getline(ids,id,',')){auto house=Number(id.c_str());Require(house&&unique.insert(house).second,"Invalid or duplicate house ID");houses.push_back(house);}Require(!houses.empty()&&houses.size()<=100,"Invalid upgrade batch size");const std::string mode=argv[4];Require((mode=="--inspect"||mode=="--dry-run"||mode=="--apply"),"Invalid upgrade arguments");if(mode!="--inspect")RequireStopped();
  RegisterExecutablePlatform(ExePlatformClientImport);EQEmuLogSys::Instance()->LoadLogSettingsDefaults();Require(EQEmuConfig::LoadConfig("."),"Server config unavailable");const auto* cfg=EQEmuConfig::get();Require(cfg,"Server database configuration missing");db=std::make_unique<Database>();Require(db->Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"housing-profile-upgrade"),"Database connection failed");Require(Housing::RuntimeActivation::AllowsReconstructedContent(*db),"Housing runtime is not activated for this database");
  auto query=[&](const std::string& sql){auto result=db->QueryDatabase(sql,false);Require(result.Success(),"Upgrade query failed");return result;};query("START TRANSACTION");transaction=true;std::ostringstream results;
  for(auto house:houses){
  auto rows=query("SELECT i.native_estate_id,i.zone_id,i.zone_version,i.instance_id,i.definition_id,p.zone_short_name,LOWER(HEX(p.client_asset_sha256)),LOWER(HEX(p.collision_map_sha256)),h.item_template_id FROM housing_house_interiors i JOIN housing_house_room_profiles p ON p.house_id=i.house_id JOIN housing_house_instances h ON h.house_id=i.house_id WHERE i.house_id="+std::to_string(house)+" FOR UPDATE");Require(rows.RowCount()==1,"Permanent room missing");auto r=rows.begin();for(unsigned n=0;n<9;++n)Require(r[n],"Incomplete permanent room");
  Housing::InteriorContent::Profile identity;identity.house_id=house;identity.native_estate_id=uint32_t(Number(r[0]));identity.route={uint32_t(Number(r[1])),uint8_t(Number(r[2])),uint16_t(Number(r[3]))};identity.definition.id=uint32_t(Number(r[4]));identity.definition.type=Housing::HouseTemplates::Type(uint32_t(Number(r[8])));identity.zone_short_name=r[5];identity.client_asset_sha256=r[6];identity.collision_map_sha256=r[7];auto before=Read(argv[2],identity),after=Read(argv[3],identity);
  if(mode=="--inspect"){
   Housing::HouseBinding::Binding binding;binding.house_id=house;Housing::HouseBinding::Interior interior;interior.native_estate_id=identity.native_estate_id;interior.definition_id=identity.definition.id;interior.zone_id=identity.route.zone_id;interior.zone_version=identity.route.zone_version;interior.instance_id=identity.route.instance_id;
   Require(Housing::ReviewedRoomRuntime::EnsureProfileUnderTransaction(*db,binding,interior,before.profile,before.revision),"Current profile does not match before manifest");Require(after.revision==before.revision+1&&Housing::InteriorContent::DefinitionPayload(before.profile)==Housing::InteriorContent::DefinitionPayload(after.profile),"Prepared profile revision/definitions mismatch");results<<"ROOM_PROFILE_INSPECT PASS house="<<house<<" revision="<<before.revision<<"->"<<after.revision<<" regions="<<Housing::InteriorContent::Regions(after.profile).size()<<" after_sha256="<<after.profile.manifest_sha256<<" no changes\n";continue;
  }
  auto result=Housing::InteriorContent::UpgradeRegionsUnderTransaction(*db,before.profile,after.profile,before.revision,after.revision);Require(bool(result),result.error.c_str());results<<"ROOM_PROFILE_UPGRADE PASS house="<<house<<" active_items="<<result.active_items<<" replayed="<<result.replayed<<" committed="<<(mode=="--apply")<<"\n";
  }
  query(mode=="--apply"?"COMMIT":"ROLLBACK");transaction=false;std::cout<<results.str();return 0;
 }catch(const std::exception& error){if(transaction&&db)db->QueryDatabase(std::string("ROLLBACK"),false);std::cerr<<"ROOM_PROFILE_UPGRADE REFUSED "<<error.what()<<"\n";return 1;}
}
