#ifndef EQEMU_HOUSING_CRATE_CONTENT_MANIFEST_H
#define EQEMU_HOUSING_CRATE_CONTENT_MANIFEST_H
#include "crate_test_content.h"
namespace Housing::CrateContentManifest {
// Actor-free installation content. Actual owners, ACLs, source items and routes
// are always authenticated by the existing custody services, never this file.
inline std::optional<CrateTest::Content> Parse(const std::string&raw){try{
 if(raw.empty()||raw.size()>262144)return {};
 Json::Value d;Json::CharReaderBuilder b;b["rejectDupKeys"]=true;b["failIfExtra"]=true;std::string error;std::istringstream in(raw);if(!Json::parseFromStream(b,in,&d,&error)||!d.isObject())return {};
 auto n=[&](const char*k){if(!d[k].isUInt()||!d[k].asUInt())throw std::invalid_argument("Invalid crate content number");return d[k].asUInt();};auto t=[&](const char*k){if(!d[k].isString())throw std::invalid_argument("Invalid crate content text");return d[k].asString();};
 if(t("schema")!="housing-crate-content-v1"||n("version")!=1||n("item_template_id")!=62586||n("definition_id")!=174||t("client_sha256")!=HouseGeometry::ClientSha256||t("renderer_sha256")!=HouseGeometry::RendererSha256||t("crate_archive_sha256")!=CrateGeometry::Original.archive_sha256||t("crate_member_sha256")!=CrateGeometry::Original.member_sha256||t("crate_witness_sha256")!=CrateGeometry::Original.witness_sha256)return {};
 CrateTest::Content c;c.catalog_key=t("catalog_key");if(!ContentBinding::Detail::Key(c.catalog_key)||!d["catalog_revision"].isUInt64()||!d["catalog_revision"].asUInt64())return {};c.catalog_revision=d["catalog_revision"].asUInt64();c.map_sha256=t("neighborhood_map_sha256");if(!ContentBinding::Detail::Hash(c.map_sha256))return {};c.placement_mask=n("placement_mask");c.minimum_scale_bits=n("minimum_scale_bits");c.maximum_scale_bits=n("maximum_scale_bits");float lo,hi;std::memcpy(&lo,&c.minimum_scale_bits,4);std::memcpy(&hi,&c.maximum_scale_bits,4);if(c.placement_mask!=142||!std::isfinite(lo)||!std::isfinite(hi)||lo<=0||hi<lo)return {};
 c.definitions_hex=t("definitions_hex");c.regions_hex=t("regions_hex");auto decoded=Catalog::DecodePayloads(CrateTest::Unhex(c.definitions_hex),CrateTest::Unhex(c.regions_hex));const EstateWire::Definition*crate=nullptr;for(const auto&x:decoded.definitions)if(x.id==174){if(crate)return {};crate=&x;}if(!crate||crate->type!=7||crate->zone_id||crate->dynamic_zone_id||crate->area_id||!crate->max_items||crate->max_non_estate_items>crate->max_items||crate->max_children||!crate->children.empty()||crate->purchase_scheme||crate->upkeep_scheme)return {};if(!SHA256(reinterpret_cast<const unsigned char*>(raw.data()),raw.size(),c.digest.data()))return {};return c;
 }catch(...){return {};}}
}
#endif
