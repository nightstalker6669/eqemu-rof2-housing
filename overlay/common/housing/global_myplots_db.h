#ifndef EQEMU_HOUSING_GLOBAL_MYPLOTS_DB_H
#define EQEMU_HOUSING_GLOBAL_MYPLOTS_DB_H
#include "neighborhood_directory_db.h"
#include "current_estate_access.h"
#include "../guilds.h"
namespace Housing::GlobalMyPlots {
// Display only. The actor is the actual authenticated Client, never the request
// name/context. No ACL, instance membership, route or purchase quote is installed.
struct Entry {EstateWire::PlotSearchRow row;RegionWire::Region region;Route route;};
inline std::vector<Entry> ReadEntries(Database&db,AuthenticatedPurchaser actor){
 using NeighborhoodDirectory::Query;using NeighborhoodDirectory::Number;using NeighborhoodDirectory::Unhex;
 if(!actor.character_id||!actor.account_id)throw std::invalid_argument("Invalid authenticated MyPlots actor");
 Query(db,"SET TRANSACTION ISOLATION LEVEL REPEATABLE READ");Query(db,"START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY");
 try{
  auto identity=Query(db,"SELECT c.account_id,c.deleted_at,a.status FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(actor.character_id));
  if(identity.RowCount()!=1||Number(identity.begin()[0])!=actor.account_id||identity.begin()[1]||!identity.begin()[2]||std::stoll(identity.begin()[2])<0)throw std::invalid_argument("Invalid current MyPlots identity");
  // Same primary-ownership scope as the existing owned-array producer: personal
  // owner, or actual current guild leader with fresh matching membership/rank.
  // A delegated Manage ACL is not primary ownership and is not inferred here.
  auto rows=Query(db,"SELECT n.native_id,n.definition_id,n.catalog_key,n.neighborhood_id,n.plot_id,e.id,e.owner_kind,e.owner_id,c.name,g.name,p.definition_id,p.catalog_key,p.parent_native_id,p.active,d.neighborhood_id,h.zone_id,h.zone_version,i.zone,i.version,i.never_expires,h.instance_id,p.neighborhood_id,p.plot_id,p.ownership_estate_id FROM housing_native_estates n JOIN housing_current_estates e ON e.id=n.ownership_estate_id AND e.plot_id=n.plot_id JOIN housing_plot_definitions d ON d.id=n.plot_id JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN instance_list i ON i.id=h.instance_id JOIN housing_native_estates p ON p.native_id=n.parent_native_id LEFT JOIN character_data c ON e.owner_kind=1 AND c.id=e.owner_id LEFT JOIN guilds g ON e.owner_kind=3 AND g.id=e.owner_id WHERE n.active=1 AND ((e.owner_kind=1 AND e.owner_id="+std::to_string(actor.character_id)+") OR (e.owner_kind=3 AND g.leader="+std::to_string(actor.character_id)+" AND EXISTS(SELECT 1 FROM guild_members gm WHERE gm.char_id="+std::to_string(actor.character_id)+" AND gm.guild_id=e.owner_id AND gm.`rank`="+std::to_string(GUILD_LEADER)+"))) ORDER BY n.native_id");
  std::map<std::string,Catalog::Bundle> catalogs;std::vector<Entry> out;
  for(auto r:rows){
   if(!r[2]||!r[11]||std::string(r[2])!=r[11]||r[12]||Number(r[13])!=1||Number(r[3])!=Number(r[14])||Number(r[15])!=712||Number(r[16])||Number(r[17])!=712||Number(r[18])||Number(r[19])!=1||!Number(r[20])||Number(r[20])>32000||Number(r[21])!=Number(r[3])||r[22]||r[23])throw std::invalid_argument("Invalid current MyPlots parent/route");
   std::string key=r[2];auto found=catalogs.find(key);
   if(found==catalogs.end()){
    std::string hex;const char* digits="0123456789abcdef";for(unsigned char ch:key){hex+=digits[ch>>4];hex+=digits[ch&15];}
    auto cat=Query(db,"SELECT revision,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key=x'"+hex+"'");
    if(cat.RowCount()!=1||!Number(cat.begin()[0]))throw std::invalid_argument("Missing current MyPlots catalog");
    auto decoded=Catalog::DecodePayloads(Unhex(cat.begin()[1]),Unhex(cat.begin()[2]));found=catalogs.emplace(key,std::move(decoded)).first;
   }
   const auto& defs=found->second.definitions;auto def=std::find_if(defs.begin(),defs.end(),[&](const auto&x){return x.id==Number(r[1]);});auto parent=std::find_if(defs.begin(),defs.end(),[&](const auto&x){return x.id==Number(r[10]);});const auto kind=Number(r[6]);
   if(def==defs.end()||parent==defs.end()||parent->type!=5||parent->zone_id!=712||def->zone_id!=712||std::find(parent->children.begin(),parent->children.end(),def->id)==parent->children.end()||!((kind==1&&def->type==4&&r[8])||(kind==3&&def->type==8&&r[9])))throw std::invalid_argument("Invalid current MyPlots definition/owner type");
   auto region=std::find_if(found->second.regions.begin(),found->second.regions.end(),[&](const auto&x){return x.id==def->area_id;});if(region==found->second.regions.end())throw std::invalid_argument("Missing current MyPlots region");
   const auto* owner=kind==1?r[8]:r[9];out.push_back({{0,def->type,uint32_t(Number(r[0])),{owner,def->address}},*region,{712,0,uint16_t(Number(r[20]))}});if(out.size()>65535)throw std::invalid_argument("Too many MyPlots rows");
  }
  Query(db,"COMMIT");return out;
 }catch(...){db.QueryDatabase(std::string("ROLLBACK"),false);throw;}
}
inline std::vector<EstateWire::PlotSearchRow> Read(Database&db,AuthenticatedPurchaser actor){std::vector<EstateWire::PlotSearchRow> rows;for(auto&entry:ReadEntries(db,actor))rows.push_back(std::move(entry.row));return rows;}
}
#endif
