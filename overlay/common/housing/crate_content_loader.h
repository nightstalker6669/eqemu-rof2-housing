#ifndef EQEMU_HOUSING_CRATE_CONTENT_LOADER_H
#define EQEMU_HOUSING_CRATE_CONTENT_LOADER_H
#include "crate_binding.h"
#include "catalog_service.h"
#include "house_geometry_registry.h"
#include "../json/json.h"
#include <sstream>
namespace Housing::CrateTest {
// Explicit experimental test content, never a claim of recovered Live limits.
struct Content {
 AuthenticatedPurchaser actor;EstateId estate_id=0;uint32_t native_plot_id=0,plot_definition_id=0,root_definition_id=0;
 uint16_t neighborhood_instance=0;uint64_t catalog_revision=0;std::string catalog_key,definitions_hex,regions_hex,map_sha256;
 uint32_t placement_mask=0,minimum_scale_bits=0,maximum_scale_bits=0;
 std::array<uint8_t,32> digest{};
};
inline std::vector<uint8_t> Unhex(const std::string& text){
 if(text.empty()||text.size()%2||text.size()>131070||text.find_first_not_of("0123456789abcdef")!=std::string::npos)throw std::invalid_argument("Invalid crate content hex");
 std::vector<uint8_t> out;for(size_t i=0;i<text.size();i+=2){auto digit=[](char c){return c<='9'?c-'0':c-'a'+10;};out.push_back(uint8_t(digit(text[i])*16+digit(text[i+1])));}return out;
}
inline std::optional<Content> Parse(const std::string& raw){
 try{
  if(raw.empty()||raw.size()>262144)return {};
  Json::CharReaderBuilder builder;builder["rejectDupKeys"]=true;builder["failIfExtra"]=true;builder["strictRoot"]=true;Json::Value doc;std::string errors;std::istringstream stream(raw);if(!Json::parseFromStream(builder,stream,&doc,&errors)||!doc.isObject())return {};
  auto number=[&](const char* k){if(!doc[k].isUInt()||!doc[k].asUInt()||doc[k].asUInt()==UINT32_MAX)throw std::invalid_argument("Invalid crate test number");return doc[k].asUInt();};
  auto text=[&](const char* k){if(!doc[k].isString())throw std::invalid_argument("Invalid crate test string");return doc[k].asString();};
  if(number("version")!=1||text("schema")!="housing-crate-test-v1"||!doc["experimental_no_upkeep"].isBool()||!doc["experimental_no_upkeep"].asBool()||number("item_template_id")!=62586||number("definition_id")!=174||text("client_sha256")!=HouseGeometry::ClientSha256||text("renderer_sha256")!=HouseGeometry::RendererSha256||text("crate_archive_sha256")!=CrateGeometry::Original.archive_sha256||text("crate_member_sha256")!=CrateGeometry::Original.member_sha256||text("crate_witness_sha256")!=CrateGeometry::Original.witness_sha256)return {};
  Content c;c.actor={number("character_id"),number("account_id")};c.estate_id=number("estate_id");c.native_plot_id=number("native_plot_id");c.plot_definition_id=number("plot_definition_id");c.root_definition_id=number("root_definition_id");const auto instance=number("neighborhood_instance");if(instance>32000)return {};c.neighborhood_instance=uint16_t(instance);
  if(!doc["catalog_revision"].isUInt64()||!doc["catalog_revision"].asUInt64())return {};c.catalog_revision=doc["catalog_revision"].asUInt64();c.catalog_key=text("catalog_key");if(!ContentBinding::Detail::Key(c.catalog_key)||c.catalog_key.rfind("geometryprobe:crate-",0)!=0)return {};
  c.map_sha256=text("neighborhood_map_sha256");if(!ContentBinding::Detail::Hash(c.map_sha256))return {};
  c.placement_mask=number("placement_mask");c.minimum_scale_bits=number("minimum_scale_bits");c.maximum_scale_bits=number("maximum_scale_bits");float minimum,maximum;std::memcpy(&minimum,&c.minimum_scale_bits,4);std::memcpy(&maximum,&c.maximum_scale_bits,4);if(c.placement_mask!=142||!std::isfinite(minimum)||!std::isfinite(maximum)||!(minimum>0)||maximum<minimum)return {};
  c.definitions_hex=text("definitions_hex");c.regions_hex=text("regions_hex");auto bundle=Catalog::DecodePayloads(Unhex(c.definitions_hex),Unhex(c.regions_hex));if(bundle.definitions.size()!=3||bundle.regions.size()!=1||!bundle.costs.empty())return {};
  const EstateWire::Definition *root=nullptr,*plot=nullptr,*crate=nullptr;for(const auto& d:bundle.definitions){if(d.purchase_scheme||d.upkeep_scheme)return {};if(d.id==c.root_definition_id)root=&d;if(d.id==c.plot_definition_id)plot=&d;if(d.id==174)crate=&d;}
  if(!root||!plot||!crate||root==plot||root==crate||plot==crate||root->type!=5||root->zone_id!=712||!root->max_children||root->children!=std::vector<uint32_t>{plot->id}||plot->type!=4||plot->zone_id!=712||!plot->max_children||plot->children!=std::vector<uint32_t>{174}||plot->area_id!=bundle.regions[0].id||!plot->max_items||plot->max_non_estate_items>plot->max_items||crate->type!=7||crate->zone_id||crate->dynamic_zone_id||crate->area_id||!crate->max_items||crate->max_non_estate_items>crate->max_items||crate->max_children||!crate->children.empty())return {};
  RegionWire::Detail::ValidateMatrix(bundle.regions[0].matrix);if(!SHA256(reinterpret_cast<const unsigned char*>(raw.data()),raw.size(),c.digest.data()))return {};return c;
 }catch(...){return {};}
}
}
#endif
