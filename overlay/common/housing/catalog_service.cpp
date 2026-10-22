#include "runtime_activation_db.h"
#include "inventory_row_db.h"
#include "catalog_service.h"
#include "custody_codec.h"
#include "crate_binding.h"
#include "catalog_content_binding.h"
#include "../database.h"
#include <map>
#include <set>
#include <limits>
#include <stdexcept>
#include <functional>
namespace Housing::Catalog {
namespace {
struct Failure { Error error; };
void Require(bool ok){if(!ok)throw std::invalid_argument("Invalid housing catalog");}
bool Key(const std::string& s){if(s.empty()||s.size()>96)return false;for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c==':'))return false;return true;}
std::string Quote(Database& d,const std::string& s){return "'"+d.Escape(s)+"'";}
MySQLRequestResult Query(Database& d,const std::string& s){auto r=d.QueryDatabase(s,false);if(!r.Success())throw Failure{Error::DatabaseFailure};return r;}
struct Tx { Database& db;bool active=true;explicit Tx(Database& d):db(d){Query(db,"START TRANSACTION");}~Tx(){if(active)try{db.QueryDatabase(std::string("ROLLBACK"),false);}catch(...){}}void Commit(){try{Query(db,"COMMIT");active=false;}catch(...){throw Failure{Error::CommitUncertain};}} };
std::string Hex(const std::vector<uint8_t>& bytes){std::string s;const char* h="0123456789abcdef";s.reserve(bytes.size()*2);for(uint8_t b:bytes){s+=h[b>>4];s+=h[b&15];}return s;}
std::vector<uint8_t> Unhex(const std::string& s,size_t maximum=NeighborhoodWire::MaximumPayload){Require(!(s.size()%2)&&s.size()<=2*maximum);auto digit=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw std::invalid_argument("Bad hex");};std::vector<uint8_t>b;for(size_t i=0;i<s.size();i+=2)b.push_back(uint8_t((digit(s[i])<<4)|digit(s[i+1])));return b;}
struct Reader {
 const std::vector<uint8_t>& bytes;size_t at=0;
 void Room(size_t n){Require(n<=bytes.size()-at);}
 uint32_t Word(){Room(4);auto v=NeighborhoodWire::Detail::Read32(bytes.data()+at);at+=4;return v;}
 int64_t Wide(){Room(8);uint64_t v=0;for(size_t i=0;i<8;++i)v|=uint64_t(bytes[at++])<<(8*i);int64_t result;std::memcpy(&result,&v,8);return result;}
 float Float(){auto bits=Word();float f;std::memcpy(&f,&bits,4);return f;}
 std::string Text(){auto n=Word();Require(n<=255);Room(n);std::string s(bytes.begin()+at,bytes.begin()+at+n);at+=n;return s;}
 uint32_t Count(size_t minimum){auto n=Word();Require(minimum && n<=(bytes.size()-at)/minimum);return n;}
};
std::map<uint32_t,const EstateWire::Definition*> DefinitionMap(const Bundle& b){std::map<uint32_t,const EstateWire::Definition*> out;for(const auto& d:b.definitions)Require(out.emplace(d.id,&d).second);return out;}
std::map<std::string,const Node*> NodeMap(const Bundle& b){std::map<std::string,const Node*> out;for(const auto& n:b.nodes)Require(out.emplace(n.key,&n).second);return out;}
void Graph(const Bundle& b){
 auto definitions=DefinitionMap(b);auto nodes=NodeMap(b);std::map<std::string,uint32_t> child_counts;std::map<uint32_t,std::string> roots;std::set<uint32_t> plots,ownership,native_ids;
 for(const auto& n:b.nodes){
  Require(Key(n.key));
  Require(n.neighborhood_id && n.neighborhood_id!=0xffffffff && definitions.count(n.definition_id));
  if(n.native_id.value)Require(n.native_id.value!=0xffffffff&&native_ids.insert(n.native_id.value).second);
  const auto& d=*definitions.at(n.definition_id);
  if(n.parent_key.empty()){
   Require(d.type==5&&!n.plot_id&&!n.ownership_estate_id&&roots.emplace(n.neighborhood_id,n.key).second);
  }else{
   Require(Key(n.parent_key)&&nodes.count(n.parent_key)&&n.parent_key!=n.key);
   const auto& parent=*nodes.at(n.parent_key);Require(definitions.count(parent.definition_id));const auto& pd=*definitions.at(parent.definition_id);
   Require(parent.neighborhood_id==n.neighborhood_id);
   Require(std::find(pd.children.begin(),pd.children.end(),d.id)!=pd.children.end());
   if(d.type==4||d.type==8)Require(pd.type==5&&n.plot_id&&n.plot_id!=0xffffffff&&plots.insert(n.plot_id).second);
   else Require((d.type==2||d.type==3||d.type==7)&&((pd.type==4||pd.type==8)||(d.type==7&&(pd.type==2||pd.type==3)))&&!n.plot_id&&!n.ownership_estate_id);
   if(d.type==2)Require(pd.type==8);if(d.type==3)Require(pd.type==4);
   Require(++child_counts[parent.key]<=pd.max_children);
  }
  if(n.ownership_estate_id)Require(n.plot_id&&n.ownership_estate_id!=0xffffffff&&ownership.insert(n.ownership_estate_id).second);
 }
 for(const auto& n:b.nodes){std::set<std::string> visited;auto cursor=&n;size_t edges=0;while(!cursor->parent_key.empty()){Require(visited.insert(cursor->key).second&&edges++<10);cursor=nodes.at(cursor->parent_key);}Require(cursor->neighborhood_id==n.neighborhood_id&&roots.at(n.neighborhood_id)==cursor->key);}
 // Definition cycles are independently forbidden even if no instantiated node uses them.
 std::map<uint32_t,uint32_t> depths;std::set<uint32_t> visiting;std::function<uint32_t(uint32_t)> depth=[&](uint32_t id){if(depths.count(id))return depths.at(id);Require(visiting.insert(id).second);uint32_t result=1;for(uint32_t child:definitions.at(id)->children)result=std::max(result,1+depth(child));Require(result<=11);visiting.erase(id);return depths.emplace(id,result).first->second;};for(const auto& d:b.definitions)depth(d.id);
}
void Domain(Database& db,const Bundle& b){
 auto defs=DefinitionMap(b);std::set<uint32_t> neighborhoods;
 for(const auto& n:b.nodes)neighborhoods.insert(n.neighborhood_id);
 for(uint32_t id:neighborhoods){
  auto q=Query(db,"SELECT zone_id,zone_version,instance_id FROM housing_neighborhoods WHERE id="+std::to_string(id)+" FOR UPDATE");if(q.RowCount()!=1)throw Failure{Error::InvalidRoute};auto row=q.begin();const auto zone=std::stoul(row[0]),version=std::stoul(row[1]),instance=std::stoul(row[2]);
  auto route=Query(db,"SELECT zone,version,never_expires FROM instance_list WHERE id="+std::to_string(instance)+" FOR UPDATE");if(!instance||instance>32000||route.RowCount()!=1||std::stoul(route.begin()[0])!=zone||std::stoul(route.begin()[1])!=version||std::stoul(route.begin()[2])!=1)throw Failure{Error::InvalidRoute};
  for(const auto& n:b.nodes)if(n.neighborhood_id==id&&n.parent_key.empty()&&defs.at(n.definition_id)->zone_id!=zone)throw Failure{Error::InvalidRoute};
  auto other=Query(db,"SELECT native_id FROM housing_native_estates WHERE neighborhood_id="+std::to_string(id)+" AND active=1 AND parent_native_id IS NULL AND catalog_key<>"+Quote(db,b.key)+" FOR UPDATE");if(other.RowCount())throw Failure{Error::Conflict};
 }
 for(const auto& n:b.nodes)if(n.plot_id){
  auto p=Query(db,"SELECT neighborhood_id FROM housing_plot_definitions WHERE id="+std::to_string(n.plot_id)+" FOR UPDATE");if(p.RowCount()!=1||std::stoul(p.begin()[0])!=n.neighborhood_id)throw Failure{Error::InvalidRoute};
  if(n.ownership_estate_id){auto e=Query(db,"SELECT plot_id FROM housing_current_estates WHERE id="+std::to_string(n.ownership_estate_id)+" FOR UPDATE");if(e.RowCount()!=1||std::stoul(e.begin()[0])!=n.plot_id)throw Failure{Error::InvalidRoute};}
 }
}
void ExperimentalDatabase(Database& db){auto name=Query(db,"SELECT DATABASE()");if(name.RowCount()!=1||!name.begin()[0])throw Failure{Error::Unauthorized};const std::string n=name.begin()[0];if(!Housing::RuntimeActivation::AllowsReconstructedContent(db))throw Failure{Error::Unauthorized};}
void Engines(Database& db){auto r=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_catalogs','housing_native_estates','housing_neighborhoods','housing_plot_definitions','housing_estates','instance_list')");if(r.RowCount()!=1||std::stoul(r.begin()[0])!=6)throw Failure{Error::DatabaseFailure};}
// A saved interior is durable identity; active does not mean physically placed.
bool HouseSchema(Database& db){
 auto r=Query(db,"SELECT COUNT(*),SUM(engine='InnoDB') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_house_instances','housing_house_locations','housing_house_interiors','housing_house_receipt_effects')");
 if(r.RowCount()!=1)throw Failure{Error::DatabaseFailure};auto n=std::stoul(r.begin()[0]);if(!n)return false;
 if(n!=4||!r.begin()[1]||std::stoul(r.begin()[1])!=4)throw Failure{Error::DatabaseFailure};return true;
}
uint64_t Number(const char* p){if(!p||!*p)throw Failure{Error::DatabaseFailure};std::string s(p);if(s.find_first_not_of("0123456789")!=std::string::npos)throw Failure{Error::DatabaseFailure};return std::stoull(s);}
// Room children belong to an independently authorized room projection. Their
// persisted catalog/neighborhood can be historical after the hosting house
// moves. Validate their typed ledger first, then omit them from the generic
// neighborhood view; never publish or silently forgive an orphan allocation.
std::set<uint32_t> RoomCrateNodes(Database& db,const std::string& key){
 std::set<uint32_t> out;if(!CrateBinding::Installed(db))return out;
 auto tables=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_house_item_scopes','housing_house_interiors','housing_interior_placement_reservations')");if(tables.RowCount()!=1||Number(tables.begin()[0])!=3)throw Failure{Error::DatabaseFailure};
 auto rows=Query(db,"SELECT n.native_id FROM housing_native_estates n LEFT JOIN housing_crate_estates e ON e.native_estate_id=n.native_id LEFT JOIN housing_crate_locations l ON l.crate_id=e.crate_id AND l.location_kind=2 LEFT JOIN housing_house_item_scopes scope ON scope.custody_id=l.custody_id LEFT JOIN housing_house_interiors room ON room.native_estate_id=n.parent_native_id WHERE n.catalog_key="+Quote(db,key)+" AND n.active=1 AND (scope.house_id IS NOT NULL OR (room.house_id IS NOT NULL AND n.node_key LIKE 'crate:%')) ORDER BY n.native_id LIMIT 1001 FOR UPDATE");
 if(rows.RowCount()>1000)throw Failure{Error::Conflict};
 for(auto row=rows.begin();row.operator!=(rows.end());++row){const auto id=Number(row[0]);if(!id||id>=UINT32_MAX||!out.insert(uint32_t(id)).second)throw Failure{Error::Conflict};auto current=CrateBinding::ReadInteriorNativeUnderTransaction(db,uint32_t(id));if(!current||current.value.binding.catalog_key!=key)throw Failure{Error::Conflict};}
 return out;
}
void LiveHouseView(Database& db,Bundle& b){
 if(!HouseSchema(db))return;
 auto engines=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('inventory','housing_live_item_custody','housing_live_placed_items','housing_live_placement_reservations')");if(engines.RowCount()!=1||Number(engines.begin()[0])!=4)throw Failure{Error::DatabaseFailure};
 auto rows=Query(db,"SELECT i.native_estate_id,i.house_id,i.definition_id,i.zone_id,i.instance_id,i.zone_version,i.catalog_revision,h.house_id,h.item_template_id,h.pending_request_key,l.location_kind,l.character_id,l.slot_id,l.custody_id,c.custody_id,c.estate_id,c.owner_character_id,HEX(c.row_blob),placed.native_item_id,r.native_estate_id,r.estate_id,r.catalog_key,r.state,r.content_binding_version,HEX(r.source_row),instance.zone,instance.version,instance.never_expires,HEX(r.content_binding_digest) FROM housing_house_interiors i LEFT JOIN housing_house_instances h ON h.house_id=i.house_id LEFT JOIN housing_house_locations l ON l.house_id=i.house_id LEFT JOIN housing_live_item_custody c ON c.custody_id=l.custody_id LEFT JOIN housing_live_placed_items placed ON placed.custody_id=c.custody_id LEFT JOIN housing_live_placement_reservations r ON r.native_item_id=placed.native_item_id LEFT JOIN instance_list instance ON instance.id=i.instance_id WHERE i.catalog_key="+Quote(db,b.key)+" FOR UPDATE");
 if(rows.RowCount()>1000)throw Failure{Error::DatabaseFailure};
 auto definitions=DefinitionMap(b);std::set<uint32_t> managed,hidden;
 for(auto r=rows.begin();r.operator!=(rows.end());++r){
  auto id=Number(r[0]);if(!id||id>=UINT32_MAX||!managed.insert(uint32_t(id)).second)throw Failure{Error::Conflict};
  auto node=std::find_if(b.nodes.begin(),b.nodes.end(),[&](const auto& n){return n.native_id.value==id;});
  if(node==b.nodes.end()||node->definition_id!=Number(r[2])||!definitions.count(node->definition_id)||!Number(r[1])||Number(r[7])!=Number(r[1])||!Number(r[8])||r[9]||Number(r[6])!=b.revision)throw Failure{Error::Conflict};
  const auto& definition=*definitions.at(node->definition_id);
  auto parent=std::find_if(b.nodes.begin(),b.nodes.end(),[&](const auto& n){return n.key==node->parent_key;});
  if(parent==b.nodes.end()||!definitions.count(parent->definition_id)||(definition.type!=2&&definition.type!=3)||definition.zone_id!=Number(r[3])||!Number(r[4])||Number(r[4])>32000||Number(r[25])!=Number(r[3])||Number(r[26])!=Number(r[5])||Number(r[27])!=1)throw Failure{Error::InvalidRoute};
  const auto& plot=*definitions.at(parent->definition_id);
  if((definition.type==2&&plot.type!=8)||(definition.type==3&&plot.type!=4)||std::find(plot.children.begin(),plot.children.end(),definition.id)==plot.children.end())throw Failure{Error::InvalidRoute};
  const auto kind=Number(r[10]);
  if(kind==1){
   if(!Number(r[11])||!r[12]||r[13]||r[14]||r[18])throw Failure{Error::Conflict};
   auto inventory=Query(db,"SELECT item_id FROM inventory WHERE character_id="+std::to_string(Number(r[11]))+" AND slot_id="+std::to_string(Number(r[12]))+" FOR UPDATE");
   if(inventory.RowCount()!=1||Number(inventory.begin()[0])!=Number(r[8]))throw Failure{Error::Conflict};
   hidden.insert(uint32_t(id));continue;
  }
  if(kind!=2||r[11]||r[12]||!r[13]||Number(r[14])!=Number(r[13])||!r[17])throw Failure{Error::Conflict};
  const auto source=CustodyCodec::Decode(Unhex(r[17],CustodyCodec::MaximumPayload));
  if(source.values.item_id!=Number(r[8])||source.values.character_id!=Number(r[16]))throw Failure{Error::Conflict};
  // A house in an exact carried crate retains its permanent interior/native
  // identity and original ownership generation. It is not current property
  // content, including after that native plot gets a different owner. Validate
  // the entire typed carried token before hiding this archived member.
  if(CrateBinding::Installed(db)){
  auto carried_rows=Query(db,"SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE (character_id,slot_id) IN (SELECT container.character_id,container.slot_id FROM housing_crate_members member JOIN housing_crate_locations container ON container.crate_id=member.crate_id AND container.location_kind=1 WHERE member.custody_id="+std::to_string(Number(r[13]))+") FOR UPDATE");
  if(carried_rows.RowCount()>1)throw Failure{Error::Conflict};
  if(carried_rows.RowCount()){
   auto carried=CrateBinding::ReadCarriedUnderTransaction(db,InventoryRowDB::Decode(carried_rows.begin()));
   if(!carried||carried.value.created_by_character_id!=source.values.character_id)throw Failure{Error::Conflict};
   hidden.insert(uint32_t(id));continue;
  }
  }
  // Custody inventory/storage is not itself physical placement.
  if(!r[18]){hidden.insert(uint32_t(id));continue;}
  if(!Number(r[18])||Number(r[18])>=UINT32_MAX||Number(r[19])!=parent->native_id.value||Number(r[20])!=Number(r[15])||Number(r[15])!=parent->ownership_estate_id||!r[21]||std::string(r[21])!=b.key||Number(r[22])!=1||Number(r[23])!=ContentBinding::HouseVersion||!r[28]||Unhex(r[28]).size()!=32||!r[24]||Unhex(r[24],CustodyCodec::MaximumPayload)!=CustodyCodec::Encode(source))throw Failure{Error::Conflict};
  // A stored house keeps its permanent interior identity, but is not a
  // physically placed child of the plot. Its item remains in the scene's
  // storage list; omit only the interior from this placed-estate projection.
  auto storage=Query(db,"SELECT stowed FROM housing_live_item_storage WHERE custody_id="+std::to_string(Number(r[13]))+" FOR UPDATE");
  if(storage.RowCount()>1||(storage.RowCount()&&Number(storage.begin()[0])>1))throw Failure{Error::Conflict};
  if(storage.RowCount()&&Number(storage.begin()[0])==1)hidden.insert(uint32_t(id));
 }
 // Allocator keys are reserved for ledger-managed nodes; never publish an
 // orphan allocation as an ordinary static catalog child.
 for(const auto& n:b.nodes)if(n.key.rfind("house:",0)==0&&!managed.count(n.native_id.value))throw Failure{Error::Conflict};
 b.nodes.erase(std::remove_if(b.nodes.begin(),b.nodes.end(),[&](const auto& n){return hidden.count(n.native_id.value);}),b.nodes.end());
}
Export Output(Bundle b){Export out;out.definition_payload=EstateWire::EncodeBootstrap(b.definitions,b.costs);out.region_payload=RegionWire::EncodeBootstrap(b.regions);out.catalog=std::move(b);return out;}
}
Bundle DecodePayloads(const std::vector<uint8_t>& defs,const std::vector<uint8_t>& regions){
 Require(defs.size()<=NeighborhoodWire::MaximumPayload&&regions.size()<=RegionWire::MaximumPayload);Bundle b;Reader r{defs};
 for(uint32_t count=r.Count(76);count;--count){EstateWire::Definition d;d.id=r.Word();d.area_id=r.Word();d.purchase_scheme=r.Word();d.upkeep_scheme=r.Word();d.max_items=r.Word();d.max_non_estate_items=r.Word();d.max_children=r.Word();d.zone_id=r.Word();d.dynamic_zone_id=r.Word();d.type=r.Word();d.group_id=r.Word();d.address=r.Text();d.icon=r.Word();d.max_npc_items=r.Word();for(float& f:d.switch_coordinates)f=r.Float();d.switch_heading=r.Float();for(uint32_t n=r.Count(4);n;--n)d.children.push_back(r.Word());b.definitions.push_back(std::move(d));}
 for(uint32_t count=r.Count(8);count;--count){EstateWire::CostScheme s;s.id=r.Word();for(uint32_t n=r.Count(8);n;--n){EstateWire::CostComponent c;c.id=r.Word();for(uint32_t k=r.Count(16);k;--k){EstateWire::CostValue v;const uint32_t bits=r.Word();std::memcpy(&v.type_id,&bits,4);v.subtype_id=r.Word();v.amount=r.Wide();c.values.push_back(v);}s.components.push_back(std::move(c));}b.costs.push_back(std::move(s));}Require(r.at==defs.size());
 Reader rr{regions};for(uint32_t n=rr.Count(RegionWire::RecordSize);n;--n){RegionWire::Region reg;reg.id=rr.Word();for(float& f:reg.matrix)f=rr.Float();for(float& f:reg.opaque_vector)f=rr.Float();for(uint32_t& id:reg.opaque_ids)id=rr.Word();b.regions.push_back(reg);}Require(rr.at==regions.size());
 Require(EstateWire::EncodeBootstrap(b.definitions,b.costs)==defs&&RegionWire::EncodeBootstrap(b.regions)==regions);return b;
}
void Validate(const Bundle& b){
 Require(Key(b.key)&&!b.definitions.empty()&&!b.nodes.empty()&&b.nodes.size()<=1000);
 Require((b.provenance.origin==Origin::HistoricalVerified||b.provenance.origin==Origin::Experimental)&&b.provenance.client_build==20130510);
 Require(!b.provenance.source.empty()&&b.provenance.source.size()<=255&&b.provenance.source.find('\0')==std::string::npos);
 Require(b.provenance.sha256.size()==64);for(unsigned char c:b.provenance.sha256)Require((c>='0'&&c<='9')||(c>='a'&&c<='f'));
 if(b.provenance.origin==Origin::Experimental)Require(b.provenance.source.rfind("EXPERIMENTAL:",0)==0);
 auto defs=EstateWire::EncodeBootstrap(b.definitions,b.costs);auto regions=RegionWire::EncodeBootstrap(b.regions);auto decoded=DecodePayloads(defs,regions);(void)decoded;
 std::set<uint32_t> areas,components;for(const auto& r:b.regions)areas.insert(r.id);
 for(const auto& d:b.definitions){Require(d.max_non_estate_items<=d.max_items&&(int32_t(d.max_npc_items)<0||d.max_npc_items<=d.max_items)&&d.children.size()<=1000&&d.max_children<=1000);if(d.area_id)Require(areas.count(d.area_id));if(d.type==4||d.type==8)Require(d.area_id);}
 for(const auto& s:b.costs){Require(!s.components.empty());for(const auto& c:s.components){Require(components.insert(c.id).second&&!c.values.empty());for(const auto& v:c.values)Require(v.amount>=0);}}
 Graph(b);
}
Service::Service(std::unique_ptr<Database> d,Settings s):database_(std::move(d)),settings_(s){}
Service::~Service()=default;
Result<Export> Service::Import(const Bundle& input,uint64_t expected){
 std::lock_guard<std::mutex> lock(mutex_);if(!settings_.enabled||!database_)return Result<Export>::Failure(Error::Disabled);
 try{
  Validate(input);if(input.revision!=expected||expected==std::numeric_limits<uint64_t>::max())throw Failure{Error::Conflict};
  auto& db=*database_;Tx tx(db);Engines(db);
  if(input.provenance.origin==Origin::Experimental)ExperimentalDatabase(db);
  Query(db,"INSERT IGNORE INTO housing_catalogs(catalog_key,revision,origin,source,source_sha256,client_build,definitions_blob,regions_blob) VALUES ("+Quote(db,input.key)+",0,"+std::to_string(static_cast<uint8_t>(input.provenance.origin))+","+Quote(db,input.provenance.source)+","+Quote(db,input.provenance.sha256)+",20130510,'','')");
  auto existing=Query(db,"SELECT revision FROM housing_catalogs WHERE catalog_key="+Quote(db,input.key)+" FOR UPDATE");if(existing.RowCount()!=1||std::stoull(existing.begin()[0])!=expected)throw Failure{Error::Conflict};
  if(HouseSchema(db)){auto managed=Query(db,"SELECT house_id FROM housing_house_interiors WHERE catalog_key="+Quote(db,input.key)+" LIMIT 1 FOR UPDATE");if(managed.RowCount())throw Failure{Error::Conflict};}
  Domain(db,input);Bundle b=input;std::map<std::string,uint32_t> ids;
  for(auto& n:b.nodes){
   auto old=Query(db,"SELECT n.native_id,n.neighborhood_id,COALESCE(n.plot_id,0),COALESCE(n.ownership_estate_id,0),n.definition_id,n.static_id,p.node_key FROM housing_native_estates n LEFT JOIN housing_native_estates p ON p.native_id=n.parent_native_id WHERE n.catalog_key="+Quote(db,b.key)+" AND n.node_key="+Quote(db,n.key)+" FOR UPDATE");
   if(old.RowCount()){
    auto row=old.begin();if(old.RowCount()!=1||std::stoul(row[1])!=n.neighborhood_id||std::stoul(row[2])!=n.plot_id||std::stoul(row[4])!=n.definition_id||std::stoul(row[5])!=n.static_id||std::string(row[6]?row[6]:"")!=n.parent_key||(std::stoul(row[3])&&std::stoul(row[3])!=n.ownership_estate_id))throw Failure{Error::Conflict};
    const uint32_t id=std::stoul(row[0]);if(n.native_id.value&&n.native_id.value!=id)throw Failure{Error::Conflict};n.native_id.value=id;
   }else{
    if(n.native_id.value)throw Failure{Error::Conflict};auto inserted=Query(db,"INSERT INTO housing_native_estates(catalog_key,node_key,neighborhood_id,plot_id,ownership_estate_id,definition_id,static_id,active) VALUES ("+Quote(db,b.key)+","+Quote(db,n.key)+","+std::to_string(n.neighborhood_id)+","+(n.plot_id?std::to_string(n.plot_id):"NULL")+","+(n.ownership_estate_id?std::to_string(n.ownership_estate_id):"NULL")+","+std::to_string(n.definition_id)+","+std::to_string(n.static_id)+",0)");
    const auto id=inserted.LastInsertedID();if(!id||id>=0xffffffff)throw Failure{Error::InstanceCapacity};n.native_id.value=id;
   }
   ids.emplace(n.key,n.native_id.value);
  }
  Query(db,"UPDATE housing_native_estates SET active=0 WHERE catalog_key="+Quote(db,b.key));
  for(const auto& n:b.nodes)Query(db,"UPDATE housing_native_estates SET active=1,parent_native_id="+(n.parent_key.empty()?"NULL":std::to_string(ids.at(n.parent_key)))+",ownership_estate_id="+(n.ownership_estate_id?std::to_string(n.ownership_estate_id):"NULL")+" WHERE native_id="+std::to_string(n.native_id.value));
  b.revision=expected+1;auto result=Output(b);
  Query(db,"UPDATE housing_catalogs SET revision="+std::to_string(b.revision)+",origin="+std::to_string(static_cast<uint8_t>(b.provenance.origin))+",source="+Quote(db,b.provenance.source)+",source_sha256="+Quote(db,b.provenance.sha256)+",client_build=20130510,definitions_blob=UNHEX('"+Hex(result.definition_payload)+"'),regions_blob=UNHEX('"+Hex(result.region_payload)+"') WHERE catalog_key="+Quote(db,b.key));
  tx.Commit();Result<Export> r;r.value=std::move(result);return r;
 }catch(const Failure& f){return Result<Export>::Failure(f.error);}catch(const std::invalid_argument&){return Result<Export>::Failure(Error::InvalidArgument);}catch(...){return Result<Export>::Failure(Error::DatabaseFailure);}
}
Result<Export> Service::Load(const std::string& key){
 std::lock_guard<std::mutex> lock(mutex_);if(!settings_.enabled||!database_)return Result<Export>::Failure(Error::Disabled);
 try{
  if(!Key(key))throw std::invalid_argument("Invalid catalog key");auto& db=*database_;Tx tx(db);Engines(db);
  auto stored=Query(db,"SELECT revision,origin,source,source_sha256,client_build,HEX(definitions_blob),HEX(regions_blob) FROM housing_catalogs WHERE catalog_key="+Quote(db,key)+" FOR UPDATE");if(stored.RowCount()!=1||!std::stoull(stored.begin()[0]))throw Failure{Error::NotFound};
  auto row=stored.begin();Bundle b=DecodePayloads(Unhex(row[5]),Unhex(row[6],RegionWire::MaximumPayload));b.key=key;b.revision=std::stoull(row[0]);b.provenance={static_cast<Origin>(std::stoul(row[1])),row[2],row[3],static_cast<uint32_t>(std::stoul(row[4]))};
  const auto room_crates=RoomCrateNodes(db,key);
  auto nodes=Query(db,"SELECT n.native_id,n.node_key,p.node_key,n.neighborhood_id,COALESCE(n.plot_id,0),COALESCE(n.ownership_estate_id,0),n.definition_id,n.static_id,p.catalog_key,p.active FROM housing_native_estates n LEFT JOIN housing_native_estates p ON p.native_id=n.parent_native_id WHERE n.catalog_key="+Quote(db,key)+" AND n.active=1 ORDER BY n.native_id FOR UPDATE");
  for(auto n=nodes.begin();n.operator!=(nodes.end());++n){if(room_crates.count(uint32_t(Number(n[0]))))continue;if(n[2]&&(!n[8]||std::string(n[8])!=key||!n[9]||std::stoul(n[9])!=1))throw Failure{Error::InvalidRoute};b.nodes.push_back({n[1],n[2]?n[2]:"",{static_cast<uint32_t>(std::stoul(n[0]))},static_cast<uint32_t>(std::stoul(n[3])),static_cast<uint32_t>(std::stoul(n[4])),static_cast<uint32_t>(std::stoul(n[5])),static_cast<uint32_t>(std::stoul(n[6])),static_cast<uint32_t>(std::stoul(n[7]))});}
  LiveHouseView(db,b);Validate(b);if(b.provenance.origin==Origin::Experimental)ExperimentalDatabase(db);Domain(db,b);auto out=Output(std::move(b));tx.Commit();Result<Export> r;r.value=std::move(out);return r;
 }catch(const Failure& f){return Result<Export>::Failure(f.error);}catch(const std::invalid_argument&){return Result<Export>::Failure(Error::InvalidArgument);}catch(...){return Result<Export>::Failure(Error::DatabaseFailure);}
}
}
