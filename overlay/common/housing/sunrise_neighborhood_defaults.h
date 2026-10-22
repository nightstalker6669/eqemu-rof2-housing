#ifndef EQEMU_HOUSING_SUNRISE_NEIGHBORHOOD_DEFAULTS_H
#define EQEMU_HOUSING_SUNRISE_NEIGHBORHOOD_DEFAULTS_H
#include "catalog_service.h"
#include "plot_pricing_defaults.h"
#include "sunrise_plot_reconstruction.h"
#include <algorithm>
#include <limits>
#include <map>
#include <set>
namespace Housing::SunriseNeighborhoodDefaults {
inline constexpr size_t PlotCount=75;
// All identities are supplied by the administrator or PersistenceService.
// The array order is PersonalPlots[0..70], then GuildPlots[0..3].
struct Settings {
 std::string catalog_key,name,source_sha256;
 uint32_t neighborhood_id=0,zone_id=712;
 std::array<uint32_t,PlotCount> plot_ids{};
 uint32_t root_definition_id=0,first_plot_definition_id=0,first_region_id=0,first_price_id=0;
 // Later-Live personal capacities are in the reconstruction helper. No original
 // guild exterior cap was recovered: require explicit administrative policy.
 uint32_t guild_max_items=0,guild_max_npc_items=0;
 uint32_t max_plot_children=20;
 bool allow_inferred_103_betwixt=false;
};
struct Marker {
 uint32_t static_id;
 const char* model;
 float x,y,z,heading;
};
// Entire guild marker actors are centered on their supported footprints.
// Heading is a native-axis reconstruction, requiring one visual corroboration.
inline std::array<Marker,4> GuildMarkers(){
 std::array<Marker,4> out{};size_t i=0;
 for(const auto& p:SunrisePlotReconstruction::GuildPlots){
  float heading=std::fmod(128.f-p.heading_radians*256.f/3.14159265358979323846f,512.f);if(heading<0)heading+=512;
  float marker_x=p.x,marker_y=p.y;
  // Brimming's opening faces its valley ramp. Compensate the original mesh's
  // off-center pivot using all698vertices so its world XY bounds stay fixed;
  // this visual transform never rotates or translates the legal plot region.
  if(p.address=="200 Brimming Way"){
   marker_x-=2.96590555516104f;marker_y+=9.83795808445734f;
   heading=std::fmod(heading+256.f,512.f);
  }
  out[i]={uint32_t(199+i),"OBP_GUILDSQUARE",marker_x,marker_y,p.reference_floor_z,heading};++i;
 }
 return out;
}
namespace Detail {
inline void Require(bool value,const char* reason){if(!value)throw std::invalid_argument(reason);}
inline uint32_t Offset(uint32_t base,size_t offset){Require(base&&base<UINT32_MAX&&offset<UINT32_MAX-base,"Sunrise ID namespace overflow");return base+uint32_t(offset);}
}
// Pure content assembly. No provisioning, DB writes, marker changes or runtime
// control. Pass one premerged content bundle containing original house/crate
// definitions and their referenced cost/region records. Roots/old plots/nodes
// are ignored. Original selected content definition fields remain unchanged.
inline Catalog::Bundle Build(const Settings& settings,const Catalog::Bundle& content){
 using namespace Detail;
 Require(settings.allow_inferred_103_betwixt,"Explicit inferred103 Betwixt review required for all75 plots");
 Require(settings.neighborhood_id&&settings.neighborhood_id!=UINT32_MAX&&settings.zone_id==712,"Explicit Sunrise neighborhood identity required");
 Require(!settings.catalog_key.empty()&&!settings.name.empty()&&settings.source_sha256.size()==64,"Explicit catalog provenance required");
 Require(settings.guild_max_items&&settings.guild_max_npc_items<=settings.guild_max_items,"Explicit guild exterior capacity policy required");
 Require(settings.max_plot_children>=2&&settings.max_plot_children<=1000,"House and moving crates need a shared child budget");
 Catalog::Bundle out;out.key=settings.catalog_key;out.revision=0;
 out.provenance={Catalog::Origin::Experimental,"EXPERIMENTAL: reconstructed75 Sunrise plots; map/collision XY, local Z and guild capacity; later-Live tariffs/capacities;103Betwixt inferred",settings.source_sha256,20130510};
 std::set<uint32_t> definition_ids,region_ids,price_ids,plot_ids;std::vector<uint32_t> personal_children,guild_children;
 for(const auto& d:content.definitions){
  if(d.type==5||d.type==4||d.type==8)continue;
  Require(d.type==2||d.type==3||d.type==7,"Only selected original house/crate content definitions accepted");
  Require(definition_ids.insert(d.id).second,"Duplicate content definition");out.definitions.push_back(d);
  if(d.type==3||d.type==7)personal_children.push_back(d.id);
  if(d.type==2||d.type==7)guild_children.push_back(d.id);
 }
 Require(!personal_children.empty()&&!guild_children.empty(),"Personal/guild content definitions required");
 auto has_type=[&](uint32_t type){return std::any_of(out.definitions.begin(),out.definitions.end(),[&](const auto& d){return d.type==type;});};
 Require(has_type(2)&&has_type(3)&&has_type(7),"A guild hall, personal house and moving crate definition are required");
 for(const auto& d:out.definitions)for(auto child:d.children)Require(definition_ids.count(child),"Content definition child missing from supplied selection");
 // Keep original costs and regions verbatim; unused source records are harmless
 // and never instantiate nodes or current ownership.
 out.costs=content.costs;out.regions=content.regions;
 for(const auto& r:out.regions)Require(region_ids.insert(r.id).second,"Duplicate supplied region");
 for(const auto& s:out.costs){price_ids.insert(s.id);for(const auto& c:s.components)price_ids.insert(c.id);}
 Require(definition_ids.insert(settings.root_definition_id).second,"Root definition collision");EstateWire::Detail::Identity(settings.root_definition_id);
 EstateWire::Definition root;root.id=settings.root_definition_id;root.type=5;root.zone_id=settings.zone_id;root.address=settings.name;root.max_children=PlotCount;
 const std::string root_key="root";out.nodes.push_back({root_key,"",{},settings.neighborhood_id,0,0,root.id,0});
 auto append=[&](size_t i,uint32_t marker,std::string_view address,uint32_t type,float x,float y,float ground_min,float ground_max,const auto& geometry){
  const auto plot_id=settings.plot_ids[i];Require(plot_id&&plot_id!=UINT32_MAX&&plot_ids.insert(plot_id).second,"Missing/duplicate supplied plot domain identity");
  EstateWire::Definition d;d.id=Offset(settings.first_plot_definition_id,i);Require(definition_ids.insert(d.id).second,"New plot definition collision");d.area_id=Offset(settings.first_region_id,i);Require(region_ids.insert(d.area_id).second,"New plot region collision");
  d.type=type;d.zone_id=settings.zone_id;d.address=std::string(address);d.max_children=settings.max_plot_children;d.children=type==4?personal_children:guild_children;
  if(type==4){auto cap=SunrisePlotReconstruction::PersonalCapacity(address);Require(bool(cap),"Missing published personal exterior capacity");d.max_items=cap->max_items;d.max_npc_items=cap->max_npc_items;}else{d.max_items=settings.guild_max_items;d.max_npc_items=settings.guild_max_npc_items;}
  d.max_non_estate_items=0;d.switch_coordinates={y,x,ground_max+2};
  // Z is an explicit local envelope around actual collision witnesses.
  out.regions.push_back(SunrisePlotReconstruction::Region(geometry,d.area_id,ground_min-10,ground_max+200));
  std::array<uint32_t,4> prices{};for(size_t n=0;n<4;++n){prices[n]=Offset(settings.first_price_id,i*4+n);Require(price_ids.insert(prices[n]).second,"New tariff identity collision");}
  PlotPricingDefaults::Apply(d,out.costs,prices[0],prices[1],prices[2],prices[3]);root.children.push_back(d.id);out.nodes.push_back({"plot:"+std::to_string(plot_id),root_key,{},settings.neighborhood_id,plot_id,0,d.id,marker});out.definitions.push_back(std::move(d));
 };
 size_t i=0;for(const auto& p:SunrisePlotReconstruction::PersonalPlots){Require(p.address_label_verified||settings.allow_inferred_103_betwixt,"Unreviewed inferred address");append(i++,p.marker_id,p.address,4,p.x,p.y,p.ground_min_z,p.ground_max_z,p);}
 for(const auto& p:SunrisePlotReconstruction::GuildPlots){append(i,199+uint32_t(i-71),p.address,8,p.x,p.y,p.ground_min_z,p.ground_max_z,p);++i;}
 Require(i==PlotCount,"Full Sunrise plot count");out.definitions.push_back(std::move(root));Catalog::Validate(out);return out;
}
}
#endif
