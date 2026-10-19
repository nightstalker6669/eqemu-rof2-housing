#ifndef EQEMU_HOUSING_CATALOG_PRICING_UPGRADE_DB_H
#define EQEMU_HOUSING_CATALOG_PRICING_UPGRADE_DB_H
#include "catalog_service.h"
#include "catalog_content_binding.h"
#include "../database.h"
#include <openssl/sha.h>
#include <cctype>
namespace Housing::CatalogPricingUpgrade {
// Offline administrative maintenance, never a client-controlled price change.
// Only economics may change. Geometry fingerprints deliberately exclude prices.
// Existing rent contracts require a separate prospective enrollment review.
inline void ApplyUnderTransaction(Database& db,const Catalog::Bundle& before,
 const Catalog::Bundle& after,const std::string& review){
 auto require=[](bool v,const char* m){if(!v)throw std::invalid_argument(m);};
 auto q=[&](const std::string& s){auto r=db.QueryDatabase(s,false);require(r.Success(),"Pricing upgrade SQL failed");return r;};
 auto hex=[](const std::vector<uint8_t>& b){const char*h="0123456789abcdef";std::string s;for(auto v:b){s+=h[v>>4];s+=h[v&15];}return s;};
 auto hash=[&](const std::vector<uint8_t>& b){std::vector<uint8_t>d(32);require(SHA256(b.data(),b.size(),d.data()),"Pricing hash failed");return hex(d);};
 auto s=[&](const std::string& v){return "'"+db.Escape(v)+"'";};
 require(std::string(q("SELECT @@in_transaction").begin()[0])=="1","Pricing upgrade needs caller transaction");
 require(before.key==after.key&&before.revision&&before.revision<UINT64_MAX&&after.revision==before.revision+1,"Pricing revision mismatch");
 require(before.provenance.origin==Catalog::Origin::Experimental&&after.provenance.origin==Catalog::Origin::Experimental&&review.rfind("EXPERIMENTAL:",0)==0&&review.size()<=255,"Explicit default pricing review required");
 Catalog::Validate(before);Catalog::Validate(after);
 auto old=EstateWire::EncodeBootstrap(before.definitions,before.costs),next=EstateWire::EncodeBootstrap(after.definitions,after.costs);
 auto regions=RegionWire::EncodeBootstrap(before.regions);
 require(regions==RegionWire::EncodeBootstrap(after.regions)&&before.definitions.size()==after.definitions.size(),"Pricing cannot change geometry or definitions");
 auto stripped_before=before.definitions,stripped_after=after.definitions;
 for(size_t i=0;i<stripped_before.size();++i){
  const auto& a=before.definitions[i];const auto& b=after.definitions[i];
  if(a.purchase_scheme!=b.purchase_scheme||a.upkeep_scheme!=b.upkeep_scheme)require((a.type==4||a.type==8)&&b.purchase_scheme&&b.upkeep_scheme,"Only plot tariffs may change");
  stripped_before[i].purchase_scheme=stripped_before[i].upkeep_scheme=0;
  stripped_after[i].purchase_scheme=stripped_after[i].upkeep_scheme=0;
 }
 require(EstateWire::EncodeBootstrap(stripped_before,{})==EstateWire::EncodeBootstrap(stripped_after,{}),"Pricing changed non-economic content");
 // Old scheme values remain immutable for other definitions and old receipts.
 for(const auto& c:before.costs){auto found=std::find_if(after.costs.begin(),after.costs.end(),[&](const auto& x){return x.id==c.id;});require(found!=after.costs.end()&&EstateWire::EncodeBootstrap({},{c})==EstateWire::EncodeBootstrap({},{*found}),"Existing cost scheme changed");}
 require(old!=next,"Empty pricing transition");
 const auto key=s(before.key),from=std::to_string(before.revision),to=std::to_string(after.revision);
 // Full range locks plus an offline server prevent both old writers and entrants.
 for(const auto* table:{"housing_inventory_sessions","housing_live_item_leases","housing_live_escrow_pending","housing_native_plot_purchase_pending","housing_house_inventory_pending"})q(std::string("SELECT character_id FROM ")+table+" ORDER BY character_id FOR UPDATE");
 for(const auto* query:{"SELECT character_id FROM housing_inventory_sessions WHERE active<>0 FOR UPDATE","SELECT character_id FROM housing_live_item_leases WHERE active<>0 OR pending_key IS NOT NULL FOR UPDATE","SELECT character_id FROM housing_live_escrow_pending FOR UPDATE","SELECT character_id FROM housing_native_plot_purchase_pending FOR UPDATE","SELECT character_id FROM housing_house_inventory_pending FOR UPDATE","SELECT house_id FROM housing_house_instances WHERE pending_request_key IS NOT NULL FOR UPDATE"})require(q(query).RowCount()==0,"Pricing requires no active writer or pending application");
 for(const auto* table:{"housing_catalogs","housing_catalog_pricing_upgrades","housing_live_placement_reservations","housing_house_interiors","housing_crate_estates","housing_native_plot_purchase_policies"}){auto engine=q(std::string("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='")+table+"'");require(engine.RowCount()==1&&engine.begin()[0]&&std::string(engine.begin()[0])=="InnoDB","Transactional pricing schema required");}
 auto head=q("SELECT revision,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+key+" FOR UPDATE");require(head.RowCount()==1,"Missing pricing catalog");
 auto equalhex=[&](const char* raw,const std::vector<uint8_t>& bytes){if(!raw)return false;std::string v=raw;std::transform(v.begin(),v.end(),v.begin(),[](unsigned char c){return char(std::tolower(c));});return v==hex(bytes);};
 require(std::string(head.begin()[0])==from&&equalhex(head.begin()[1],old)&&equalhex(head.begin()[2],regions),"Stale pricing plan");
 require(q("SELECT u.estate_id FROM housing_upkeep_contracts u JOIN housing_native_estates n ON n.ownership_estate_id=u.estate_id WHERE n.catalog_key="+key+" FOR UPDATE").RowCount()==0,"Existing rent contract needs separate reviewed migration");
 require(q("SELECT native_item_id FROM housing_live_placement_reservations WHERE catalog_key="+key+" AND state=0 FOR UPDATE").RowCount()==0,"Outstanding placement preview");
 // Current physical links advance; immutable historical receipts stay untouched.
 for(const auto* table:{"housing_live_placement_reservations","housing_house_interiors","housing_crate_estates"}){
  auto stale=q(std::string("SELECT catalog_revision FROM ")+table+" WHERE catalog_key="+key+" AND catalog_revision<>"+from+(std::string(table)=="housing_live_placement_reservations"?" AND state=1":"")+" FOR UPDATE");require(stale.RowCount()==0,"Stale physical catalog link");
 }
 auto policies=q("SELECT definitions_sha256,regions_sha256,catalog_revision FROM housing_native_plot_purchase_policies WHERE catalog_key="+key+" FOR UPDATE");
 for(auto r=policies.begin();r.operator!=(policies.end());++r)require(r[0]&&r[1]&&r[2]&&std::string(r[0])==hash(old)&&std::string(r[1])==hash(regions)&&std::string(r[2])==from,"Stale purchase policy");
 q("SAVEPOINT housing_pricing_upgrade");
 try{
  for(const auto* table:{"housing_live_placement_reservations","housing_house_interiors","housing_crate_estates"})q(std::string("UPDATE ")+table+" SET catalog_revision="+to+" WHERE catalog_key="+key+" AND catalog_revision="+from+(std::string(table)=="housing_live_placement_reservations"?" AND state=1":""));
  q("UPDATE housing_native_plot_purchase_policies SET catalog_revision="+to+",definitions_sha256="+s(hash(next))+",policy_revision=policy_revision+1,review_source="+s(review)+" WHERE catalog_key="+key);
  // Other policy grants remain on the old revision and require explicit review.
  auto changed=q("UPDATE housing_catalogs SET revision="+to+",source="+s(review)+",source_sha256="+s(after.provenance.sha256)+",definitions_blob=UNHEX("+s(hex(next))+") WHERE catalog_key="+key+" AND revision="+from);require(changed.RowsAffected()==1,"Pricing catalog CAS failed");
  q("INSERT INTO housing_catalog_pricing_upgrades(catalog_key,from_revision,to_revision,before_definitions,after_definitions,review_reference) VALUES("+key+","+from+","+to+",UNHEX("+s(hex(old))+"),UNHEX("+s(hex(next))+"),"+s(review)+")");
  q("RELEASE SAVEPOINT housing_pricing_upgrade");
 }catch(...){db.QueryDatabase(std::string("ROLLBACK TO SAVEPOINT housing_pricing_upgrade"),false);db.QueryDatabase(std::string("RELEASE SAVEPOINT housing_pricing_upgrade"),false);throw;}
}
}
#endif
