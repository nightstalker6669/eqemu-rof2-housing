#ifndef EQEMU_HOUSING_INTERIOR_CAPACITY_POLICY_DB_H
#define EQEMU_HOUSING_INTERIOR_CAPACITY_POLICY_DB_H
#include "interior_content_binding.h"
#include "../database.h"
#include <cctype>
#include "npc_runtime_db.h"
namespace Housing::InteriorCapacity {
namespace Detail {
inline void Require(bool valid){if(!valid)throw std::invalid_argument("Invalid room capacity policy");}
inline auto Query(Database& db,const std::string& sql){auto rows=db.QueryDatabase(sql,false);Require(rows.Success());return rows;}
inline uint64_t Number(const char* value){Require(value);std::string s=value;size_t end=0;auto n=std::stoull(s,&end);Require(end==s.size());return n;}
inline std::string Hash(const std::vector<uint8_t>& bytes){std::array<unsigned char,SHA256_DIGEST_LENGTH> sum{};Require(SHA256(bytes.data(),bytes.size(),sum.data()));const char* digits="0123456789abcdef";std::string out;for(auto b:sum){out+=digits[b>>4];out+=digits[b&15];}return out;}
inline std::string Hex(const std::vector<uint8_t>& bytes){const char* digits="0123456789abcdef";std::string out;for(auto b:bytes){out+=digits[b>>4];out+=digits[b&15];}return out;}
inline std::vector<uint8_t> Unhex(const char* value){Require(value);std::string s=value;Require(s.size()%2==0&&s.size()<=2*std::max(size_t(NeighborhoodWire::MaximumPayload),size_t(RegionWire::MaximumPayload)));auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw std::invalid_argument("Invalid policy hex");};std::vector<uint8_t> out;for(size_t n=0;n<s.size();n+=2)out.push_back(uint8_t(digit(s[n])*16+digit(s[n+1])));return out;}
inline std::string Lower(const char* value){Require(value);std::string s=value;for(auto& c:s)c=char(std::tolower(static_cast<unsigned char>(c)));return s;}
inline void Transaction(Database& db){auto rows=Query(db,"SELECT @@in_transaction");Require(rows.RowCount()==1&&Number(rows.begin()[0])==1);}
inline bool Installed(Database& db){auto rows=Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_house_capacity_policies','housing_house_capacity_heads')");if(!rows.RowCount())return false;Require(rows.RowCount()==2);for(auto row=rows.begin();row!=rows.end();++row)Require(row[0]&&std::string(row[0])=="InnoDB");return true;}
// Monotone only: existing retained contents never become illegal under a new
// count limit. Existing typed child definitions keep their original bytes.
inline void CapacityOnly(const InteriorContent::Profile& before,const InteriorContent::Profile& after){
 Require(InteriorContent::ValidChildDefinitions(before)&&InteriorContent::ValidChildDefinitions(after));
 const auto& a=before.definition;const auto& b=after.definition;
 Require(b.max_items>=a.max_items&&b.max_non_estate_items>=a.max_non_estate_items&&b.max_children>=a.max_children&&b.max_npc_items>=a.max_npc_items&&b.max_items&&b.max_items<=16384&&b.max_children<=b.max_items&&b.max_non_estate_items<=b.max_items&&(b.max_npc_items==UINT32_MAX||b.max_npc_items<=b.max_items));
 auto identity=b;identity.max_items=a.max_items;identity.max_non_estate_items=a.max_non_estate_items;identity.max_npc_items=a.max_npc_items;identity.max_children=a.max_children;identity.children=a.children;
 auto neutral=before;neutral.definition=identity;Require(EstateWire::EncodeBootstrap(InteriorContent::Definitions(before),{})==EstateWire::EncodeBootstrap(InteriorContent::Definitions(neutral),{}));
 for(const auto& old:before.child_definitions){auto found=std::find_if(after.child_definitions.begin(),after.child_definitions.end(),[&](const auto& child){return child.id==old.id;});Require(found!=after.child_definitions.end()&&EstateWire::EncodeBootstrap({old},{})==EstateWire::EncodeBootstrap({*found},{}));}
}
inline InteriorContent::Profile DecodeEffective(const InteriorContent::Profile& base,const std::vector<uint8_t>& payload){
 auto bundle=Catalog::DecodePayloads(payload,InteriorContent::RegionPayload(base));Require(!bundle.definitions.empty()&&bundle.definitions.size()<=1001&&bundle.costs.empty());auto result=base;bool found=false;result.child_definitions.clear();
 for(const auto& definition:bundle.definitions){if(definition.id==base.definition.id){Require(!found);result.definition=definition;found=true;}else result.child_definitions.push_back(definition);}
 Require(found);CapacityOnly(base,result);return result;
}
inline std::string Digest(uint64_t house,uint64_t revision,uint64_t base_revision,const InteriorContent::Profile& base,const std::string& previous,const std::string& source,const std::string& source_sha,const std::vector<uint8_t>& effective){
 Require(source.size()&&source.size()<=1024&&ContentBinding::Detail::Hash(source_sha)&&ContentBinding::Detail::Hash(base.manifest_sha256)&&ContentBinding::Detail::Hash(previous));ContentBinding::Detail::Writer w;w.Text("EQEmuHousingCapacityPolicyV1");w.Word(uint32_t(house));w.Word(uint32_t(house>>32));w.Word(uint32_t(revision));w.Word(uint32_t(revision>>32));w.Word(uint32_t(base_revision));w.Word(uint32_t(base_revision>>32));w.Text(base.manifest_sha256);w.Text(Hash(InteriorContent::DefinitionPayload(base)));w.Text(Hash(InteriorContent::RegionPayload(base)));w.Text(previous);w.Text(source);w.Text(source_sha);w.Word(uint32_t(effective.size()));w.bytes.insert(w.bytes.end(),effective.begin(),effective.end());return Hash(w.bytes);
}
}
// Called only after the owning service locks/recognizes the immutable base
// room. The common profile projection serves native display and every admission,
// storage, NPC, child, layout, transfer and swap capacity consumer alike.
inline bool ApplyUnderTransaction(Database& db,InteriorContent::Profile& room,uint64_t base_revision){
 using namespace Detail;
 try{
  Transaction(db);Require(room.house_id&&base_revision&&!room.capacity_identity&&!room.capacity_policy_revision&&ContentBinding::Detail::Hash(room.manifest_sha256));
  if(!Installed(db))return true;
  const auto id=std::to_string(room.house_id);auto heads=Query(db,"SELECT revision FROM housing_house_capacity_heads WHERE house_id="+id+" FOR UPDATE");Require(heads.RowCount()<=1);
  auto rows=Query(db,"SELECT revision,base_revision,LOWER(HEX(base_manifest_sha256)),LOWER(HEX(base_definitions_sha256)),LOWER(HEX(base_regions_sha256)),LOWER(HEX(previous_sha256)),source_reference,LOWER(HEX(source_sha256)),HEX(definitions_blob),LOWER(HEX(policy_sha256)) FROM housing_house_capacity_policies WHERE house_id="+id+" ORDER BY revision FOR UPDATE");
  if(!heads.RowCount()){Require(!rows.RowCount());return true;}
  Require(rows.RowCount()&&rows.RowCount()<=1024);const auto head=Number(heads.begin()[0]);Require(head==rows.RowCount());
  const auto original=room;auto effective=room;std::string previous(64,'0');uint64_t revision=0;
  for(auto row=rows.begin();row!=rows.end();++row){for(unsigned n=0;n<10;++n)Require(row[n]);++revision;
   Require(Number(row[0])==revision&&Number(row[1])==base_revision&&Lower(row[2])==original.manifest_sha256&&Lower(row[3])==Hash(InteriorContent::DefinitionPayload(original))&&Lower(row[4])==Hash(InteriorContent::RegionPayload(original))&&Lower(row[5])==previous);
   auto bytes=Unhex(row[8]);auto next=DecodeEffective(original,bytes);CapacityOnly(effective,next);auto digest=Digest(room.house_id,revision,base_revision,original,previous,row[6],Lower(row[7]),bytes);Require(digest==Lower(row[9]));previous=digest;effective=std::move(next);
  }
  auto identity=std::make_shared<InteriorContent::CapacityIdentity>();identity->definition=original.definition;identity->child_definitions=original.child_definitions;effective.capacity_identity=std::move(identity);effective.capacity_policy_revision=head;room=std::move(effective);return true;
 }catch(...){return false;}
}
namespace Detail {
inline bool Append(Database& db,const InteriorContent::Profile& base,uint64_t base_revision,const InteriorContent::Profile& after,uint64_t expected_head,const std::string& source_reference,const std::string& source_sha256){
 bool savepoint=false;
 try{
  const auto bytes=EstateWire::EncodeBootstrap(InteriorContent::Definitions(after),{});std::string previous(64,'0');
  if(expected_head){auto old=Query(db,"SELECT LOWER(HEX(policy_sha256)) FROM housing_house_capacity_policies WHERE house_id="+std::to_string(base.house_id)+" AND revision="+std::to_string(expected_head)+" FOR UPDATE");Require(old.RowCount()==1);previous=Lower(old.begin()[0]);}
  const auto revision=expected_head+1;const auto digest=Digest(base.house_id,revision,base_revision,base,previous,source_reference,source_sha256,bytes);
  Query(db,"SAVEPOINT housing_capacity_upgrade");savepoint=true;
  auto inserted=Query(db,"INSERT INTO housing_house_capacity_policies(house_id,revision,base_revision,base_manifest_sha256,base_definitions_sha256,base_regions_sha256,previous_sha256,source_reference,source_sha256,definitions_blob,policy_sha256) VALUES("+std::to_string(base.house_id)+","+std::to_string(revision)+","+std::to_string(base_revision)+",UNHEX('"+base.manifest_sha256+"'),UNHEX('"+Hash(InteriorContent::DefinitionPayload(base))+"'),UNHEX('"+Hash(InteriorContent::RegionPayload(base))+"'),UNHEX('"+previous+"'),'"+db.Escape(source_reference)+"',UNHEX('"+source_sha256+"'),UNHEX('"+Hex(bytes)+"'),UNHEX('"+digest+"'))");Require(inserted.RowsAffected()==1);
  const auto head_sql=expected_head?"UPDATE housing_house_capacity_heads SET revision="+std::to_string(revision)+" WHERE house_id="+std::to_string(base.house_id)+" AND revision="+std::to_string(expected_head):"INSERT INTO housing_house_capacity_heads(house_id,revision) VALUES("+std::to_string(base.house_id)+","+std::to_string(revision)+")";
  Require(Query(db,head_sql).RowsAffected()==1);Query(db,"RELEASE SAVEPOINT housing_capacity_upgrade");savepoint=false;return true;
 }catch(...){if(savepoint){db.QueryDatabase(std::string("ROLLBACK TO SAVEPOINT housing_capacity_upgrade"),false);db.QueryDatabase(std::string("RELEASE SAVEPOINT housing_capacity_upgrade"),false);}return false;}
}
}
// Maintenance API: caller must stop world/all zones, retain backup and own the
// transaction. Append one immutable policy; only the separate head advances.
// No room payload, profile revision, content tag or permanent identity changes.
inline bool UpgradeUnderTransaction(Database& db,const InteriorContent::Profile& base,uint64_t base_revision,const InteriorContent::Profile& after,uint64_t expected_head,const std::string& source_reference,const std::string& source_sha256){
 using namespace Detail;
 try{
  Transaction(db);Require(Installed(db)&&base.house_id&&base_revision&&expected_head<1024&&!base.capacity_identity&&!after.capacity_identity&&after.house_id==base.house_id&&after.native_estate_id==base.native_estate_id);
  auto current=Query(db,"SELECT revision,LOWER(HEX(manifest_sha256)),HEX(definitions_blob),HEX(regions_blob) FROM housing_house_room_profiles WHERE house_id="+std::to_string(base.house_id)+" FOR UPDATE");Require(current.RowCount()==1);auto row=current.begin();Require(Number(row[0])==base_revision&&Lower(row[1])==base.manifest_sha256&&Unhex(row[2])==InteriorContent::DefinitionPayload(base)&&Unhex(row[3])==InteriorContent::RegionPayload(base));
  for(const auto& statement:{"SELECT 1 FROM housing_inventory_sessions WHERE active=1 LIMIT 1 FOR UPDATE","SELECT 1 FROM housing_live_item_leases WHERE active=1 OR pending_key IS NOT NULL LIMIT 1 FOR UPDATE","SELECT 1 FROM housing_house_instances WHERE pending_request_key IS NOT NULL LIMIT 1 FOR UPDATE","SELECT 1 FROM housing_house_inventory_pending LIMIT 1 FOR UPDATE"})Require(Query(db,statement).RowCount()==0);
  // Historical birth records are not death certificates. Capacity maintenance
  // locks them without reconciling inventory or claiming their processes died.
  // Caller must stop all world/zones; active sessions/leases/pending still refuse.
  Query(db,"SELECT process_key FROM housing_inventory_processes WHERE reaped=0 ORDER BY process_key FOR UPDATE");
  auto effective=base;Require(ApplyUnderTransaction(db,effective,base_revision));Require(effective.capacity_policy_revision==expected_head);CapacityOnly(effective,after);
  for(const auto* table:{"housing_house_item_scopes","housing_live_vault_items","housing_crate_members","housing_crate_vault_members","housing_native_estates"}){auto engine=Query(db,std::string("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='")+table+"'");Require(engine.RowCount()==1&&engine.begin()[0]&&std::string(engine.begin()[0])=="InnoDB");}
  const auto house=std::to_string(base.house_id),native=std::to_string(base.native_estate_id);
  auto total=Query(db,"SELECT c.custody_id FROM housing_house_item_scopes c WHERE c.house_id="+house+" AND NOT EXISTS (SELECT 1 FROM housing_crate_members m WHERE m.custody_id=c.custody_id) AND NOT EXISTS (SELECT 1 FROM housing_crate_vault_members m WHERE m.custody_id=c.custody_id) ORDER BY c.custody_id FOR UPDATE");Require(total.RowCount()<=after.definition.max_items);
  auto vault=Query(db,"SELECT v.custody_id FROM housing_live_vault_items v WHERE v.house_id="+house+" AND NOT EXISTS (SELECT 1 FROM housing_crate_vault_members m WHERE m.custody_id=v.custody_id) ORDER BY v.custody_id FOR UPDATE");Require(vault.RowCount()<=after.definition.max_non_estate_items);
  auto children=Query(db,"SELECT native_id FROM housing_native_estates WHERE parent_native_id="+native+" AND active=1 ORDER BY native_id FOR UPDATE");Require(children.RowCount()<=after.definition.max_children);
  Require(after.definition.max_npc_items==UINT32_MAX||NpcRuntime::Count(db,base.native_estate_id,base.house_id)<=after.definition.max_npc_items);
  return Detail::Append(db,base,base_revision,after,expected_head,source_reference,source_sha256);
 }catch(...){return false;}
}
// Install an exact, versioned template grant during stopped-server maintenance.
// This grants capacity policy only, never route, ownership or item admission.
inline bool RegisterTemplateUnderTransaction(Database& db,uint32_t item,const InteriorContent::Profile& base,uint64_t base_revision,const InteriorContent::Profile& after,uint64_t revision,const std::string& source_reference,const std::string& source_sha256){
 using namespace Detail;
 try{
  Transaction(db);Require(Installed(db)&&item&&revision&&revision<=1024&&base_revision&&!base.capacity_identity&&!after.capacity_identity&&ContentBinding::Detail::Hash(base.manifest_sha256));CapacityOnly(base,after);
  auto engine=Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_house_capacity_templates'");Require(engine.RowCount()==1&&engine.begin()[0]&&std::string(engine.begin()[0])=="InnoDB");
  for(const auto& statement:{"SELECT 1 FROM housing_inventory_sessions WHERE active=1 LIMIT 1 FOR UPDATE","SELECT 1 FROM housing_live_item_leases WHERE active=1 OR pending_key IS NOT NULL LIMIT 1 FOR UPDATE"})Require(Query(db,statement).RowCount()==0);
  Query(db,"SELECT process_key FROM housing_inventory_processes WHERE reaped=0 ORDER BY process_key FOR UPDATE");
  auto bytes=EstateWire::EncodeBootstrap(InteriorContent::Definitions(after),{});const auto digest=Digest(item,revision,base_revision,base,std::string(64,'0'),source_reference,source_sha256,bytes);
  const auto where="item_template_id="+std::to_string(item)+" AND base_manifest_sha256=UNHEX('"+base.manifest_sha256+"') AND revision="+std::to_string(revision);
  auto rows=Query(db,"SELECT base_revision,LOWER(HEX(base_definitions_sha256)),LOWER(HEX(base_regions_sha256)),source_reference,LOWER(HEX(source_sha256)),HEX(definitions_blob),LOWER(HEX(policy_sha256)) FROM housing_house_capacity_templates WHERE "+where+" FOR UPDATE");Require(rows.RowCount()<=1);
  if(rows.RowCount()){auto row=rows.begin();return Number(row[0])==base_revision&&Lower(row[1])==Hash(InteriorContent::DefinitionPayload(base))&&Lower(row[2])==Hash(InteriorContent::RegionPayload(base))&&row[3]&&source_reference==row[3]&&Lower(row[4])==source_sha256&&Unhex(row[5])==bytes&&Lower(row[6])==digest;}
  return Query(db,"INSERT INTO housing_house_capacity_templates(item_template_id,revision,base_revision,base_manifest_sha256,base_definitions_sha256,base_regions_sha256,source_reference,source_sha256,definitions_blob,policy_sha256) VALUES("+std::to_string(item)+","+std::to_string(revision)+","+std::to_string(base_revision)+",UNHEX('"+base.manifest_sha256+"'),UNHEX('"+Hash(InteriorContent::DefinitionPayload(base))+"'),UNHEX('"+Hash(InteriorContent::RegionPayload(base))+"'),'"+db.Escape(source_reference)+"',UNHEX('"+source_sha256+"'),UNHEX('"+Hex(bytes)+"'),UNHEX('"+digest+"'))").RowsAffected()==1;
 }catch(...){return false;}
}
// Called exclusively after EnsureProfile inserts a new empty immutable profile
// in its owning house allocation transaction. Existing profiles never use this
// online path. Exact grants were registered during separate offline maintenance.
inline bool InstallForNewProfileUnderTransaction(Database& db,uint32_t item,const InteriorContent::Profile& base,uint64_t base_revision){
 using namespace Detail;
 try{
  Transaction(db);Require(item&&base.house_id&&base_revision&&!base.capacity_identity&&ContentBinding::Detail::Hash(base.manifest_sha256));
  if(!Installed(db))return true;
  auto engine=Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_house_capacity_templates'");Require(engine.RowCount()==1&&engine.begin()[0]&&std::string(engine.begin()[0])=="InnoDB");
  auto grants=Query(db,"SELECT revision,base_revision,LOWER(HEX(base_definitions_sha256)),LOWER(HEX(base_regions_sha256)),source_reference,LOWER(HEX(source_sha256)),HEX(definitions_blob),LOWER(HEX(policy_sha256)) FROM housing_house_capacity_templates WHERE item_template_id="+std::to_string(item)+" AND base_manifest_sha256=UNHEX('"+base.manifest_sha256+"') ORDER BY revision DESC LIMIT 1 FOR UPDATE");
  if(!grants.RowCount())return true;Require(grants.RowCount()==1);auto row=grants.begin();for(unsigned n=0;n<8;++n)Require(row[n]);
  const auto revision=Number(row[0]);Require(revision&&revision<=1024&&Number(row[1])==base_revision&&Lower(row[2])==Hash(InteriorContent::DefinitionPayload(base))&&Lower(row[3])==Hash(InteriorContent::RegionPayload(base)));auto bytes=Unhex(row[6]);Require(Digest(item,revision,base_revision,base,std::string(64,'0'),row[4],Lower(row[5]),bytes)==Lower(row[7]));auto after=DecodeEffective(base,bytes);
  const auto house=std::to_string(base.house_id);auto original=Query(db,"SELECT revision,LOWER(HEX(manifest_sha256)),HEX(definitions_blob),HEX(regions_blob) FROM housing_house_room_profiles WHERE house_id="+house+" FOR UPDATE");Require(original.RowCount()==1);auto stored=original.begin();Require(Number(stored[0])==base_revision&&Lower(stored[1])==base.manifest_sha256&&Unhex(stored[2])==InteriorContent::DefinitionPayload(base)&&Unhex(stored[3])==InteriorContent::RegionPayload(base));
  auto identity=Query(db,"SELECT item_template_id FROM housing_house_instances WHERE house_id="+house+" FOR UPDATE");Require(identity.RowCount()==1&&Number(identity.begin()[0])==item);
  Require(Query(db,"SELECT 1 FROM housing_house_item_scopes WHERE house_id="+house+" LIMIT 1 FOR UPDATE").RowCount()==0&&Query(db,"SELECT 1 FROM housing_interior_placement_reservations WHERE house_id="+house+" LIMIT 1 FOR UPDATE").RowCount()==0&&Query(db,"SELECT 1 FROM housing_live_vault_items WHERE house_id="+house+" LIMIT 1 FOR UPDATE").RowCount()==0&&Query(db,"SELECT 1 FROM housing_native_estates WHERE parent_native_id="+std::to_string(base.native_estate_id)+" AND active=1 LIMIT 1 FOR UPDATE").RowCount()==0);
  auto current=base;Require(ApplyUnderTransaction(db,current,base_revision)&&!current.capacity_policy_revision);
  return Detail::Append(db,base,base_revision,after,0,row[4],Lower(row[5]));
 }catch(...){return false;}
}

}
#endif
