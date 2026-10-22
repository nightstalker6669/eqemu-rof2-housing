#ifndef EQEMU_HOUSING_VAULT_STORAGE_DB_H
#define EQEMU_HOUSING_VAULT_STORAGE_DB_H
#include "vault_storage.h"
#include "custody_codec.h"
#include "item_payload_db.h"
#include "scene_service.h"
#include "interior_profile_db.h"
#include "crate_member_scope_db.h"
#include "crate_binding.h"
#include <stdexcept>
#include <set>
namespace Housing::VaultStorage {
inline auto Query(Database& db,const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())throw std::invalid_argument("Vault query failed");return result;}
inline bool Installed(Database& db){auto r=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_live_vault_items','housing_live_vault_transfers')");return r.RowCount()==1&&std::stoul(r.begin()[0])==2;}
inline std::string Hex(const std::vector<uint8_t>& b){std::string s;constexpr char h[]="0123456789abcdef";for(auto v:b){s+=h[v>>4];s+=h[v&15];}return s;}
inline std::vector<uint8_t> Unhex(const char* raw){if(!raw)throw std::invalid_argument("Missing vault bytes");std::string s=raw;if(s.size()%2||s.size()>2*CustodyCodec::MaximumPayload)throw std::invalid_argument("Invalid vault bytes");std::vector<uint8_t>b;auto nib=[](char c){if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;throw std::invalid_argument("Invalid vault hex");};for(size_t i=0;i<s.size();i+=2)b.push_back(uint8_t(nib(s[i])*16+nib(s[i+1])));return b;}
inline bool SameRoute(Route a,Route b){return a.zone_id==b.zone_id&&a.zone_version==b.zone_version&&a.instance_id==b.instance_id;}
// All supported payload fields are preserved by the existing canonical row.
// Bags/books are list-only; complete child, augment and evolving capsules retain
// their identity through the owning fenced Store/Return transaction.
inline LiveItems::Catalog Profile(Database& db,const LiveItems::InventoryRow& row){
 const auto& v=row.values;if(!CustodyCodec::Valid(row)||!v.item_id||row.null_fields&LiveItems::NullItemId)throw std::invalid_argument("Unsupported vault payload");
 ItemPayload::Metadata(db,row);
 auto m=Query(db,"SELECT itemclass,bagslots,loregroup,nodrop,notransfer,UNK224 FROM items WHERE id="+std::to_string(v.item_id)+" FOR UPDATE");if(m.RowCount()!=1)throw std::invalid_argument("Missing vault item");auto r=m.begin();for(unsigned i=0;i<6;++i)if(!r[i])throw std::invalid_argument("Missing vault metadata");
 const auto kind=std::stol(r[0]),bags=std::stol(r[1]);
 // notransfer is the existing serializer's exact native ItemDefinition+0x584
 // bNoStorage byte. It is not inferred from NoDrop, lore or quest status.
 if(kind<0||kind>2||bags<0||std::stoll(r[4])!=0||std::stoll(r[5])!=0||(kind!=1&&bags))throw std::invalid_argument("Not a supported non-placeable vault item");
 return {int32_t(kind),int32_t(bags),int32_t(std::stol(r[2])),int32_t(std::stol(r[3]))};
}
// Typed packed vaults never become placed actors. Optional schema absence keeps
// old realms unchanged; a present malformed schema fails closed.
inline bool CrateMembersInstalled(Database& db){auto r=Query(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_crate_vault_members'");if(!r.RowCount())return false;if(r.RowCount()!=1||!r.begin()[0]||std::string(r.begin()[0])!="InnoDB")throw std::invalid_argument("Invalid crate vault schema");return true;}
inline std::string Uncontained(Database& db,const std::string& alias="v"){return CrateMembersInstalled(db)?" AND NOT EXISTS (SELECT 1 FROM housing_crate_vault_members cv WHERE cv.custody_id="+alias+".custody_id)":"";}
struct ReadOptions {bool include_contained=false,include_pending=false,detached_room_scope=false;};
inline std::vector<Item> Items(Database& db,EstateId estate,uint32_t native,uint64_t house=0,ReadOptions options={}){
 if(!Installed(db))return {};
 auto rows=Query(db,"SELECT v.custody_id,v.native_item_id,v.revision,c.owner_character_id,c.schema_descriptor,HEX(c.row_blob),HEX(c.catalog_blob),r.native_estate_id,r.estate_id,r.state,r.custody_id,HEX(r.source_row),r.content_binding_version,r.content_binding_digest,v.house_id,ch.name,l.pending_key,c.estate_id,ch.deleted_at,a.id,scope.house_id,scope.revision,v.realm_estate_id FROM housing_live_vault_items v JOIN housing_live_item_custody c ON c.custody_id=v.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=v.native_item_id LEFT JOIN character_data ch ON ch.id=c.owner_character_id LEFT JOIN account a ON a.id=ch.account_id LEFT JOIN housing_house_item_scopes scope ON scope.custody_id=c.custody_id LEFT JOIN housing_live_item_leases l ON l.character_id=c.owner_character_id WHERE "+(house?"v.house_id="+std::to_string(house):"v.realm_estate_id="+std::to_string(estate))+" AND r.native_estate_id="+std::to_string(native)+" AND "+(house?"v.house_id="+std::to_string(house):"v.house_id IS NULL")+" ORDER BY v.native_item_id FOR UPDATE");std::vector<Item> result;
 for(auto r=rows.begin();r!=rows.end();++r){for(unsigned i=0;i<16;++i)if(i!=12&&i!=13&&i!=14&&!r[i])throw std::invalid_argument("Incomplete vault identity");if(r[18]||!r[19])throw std::invalid_argument("Invalid vault creator account");std::optional<MySQLRequestResult> packed;
 if(CrateMembersInstalled(db)){auto m=Query(db,"SELECT m.crate_id,m.revision,m.origin_native_estate_id,m.origin_house_id,m.origin_realm_estate_id,m.vault_revision,HEX(m.source_row),HEX(m.catalog_blob),m.schema_descriptor,c.created_by_character_id,l.location_kind,l.custody_id,l.character_id,l.slot_id,e.parent_native_estate_id FROM housing_crate_vault_members m LEFT JOIN housing_crate_instances c ON c.crate_id=m.crate_id LEFT JOIN housing_crate_locations l ON l.crate_id=m.crate_id LEFT JOIN housing_crate_estates e ON e.crate_id=m.crate_id WHERE m.custody_id="+std::string(r[0])+" FOR UPDATE");if(m.RowCount()>1)throw std::invalid_argument("Ambiguous vault containment");if(m.RowCount())packed.emplace(std::move(m));}
 bool carried=false;if(packed){auto m=packed->begin();for(unsigned k=0;k<15;++k)if(k!=3&&k!=11&&k!=12&&k!=13&&!m[k])throw std::invalid_argument("Missing vault container");carried=std::stoul(m[10])==1;if(carried?(!m[12]||!m[13]||m[11]):(std::stoul(m[10])!=2||!m[11]||m[12]||m[13]))throw std::invalid_argument("Invalid vault container location");if(std::stoul(m[14])!=native)throw std::invalid_argument("Vault container origin mismatch");}
 if(options.detached_room_scope&&!carried)throw std::invalid_argument("Detached vault scope requires actual carried container");
 if(house?((options.detached_room_scope||carried)?r[20]!=nullptr:(!r[20]||!r[21]||std::stoull(r[20])!=house||!std::stoull(r[21]))):r[20]!=nullptr)throw std::invalid_argument("Invalid vault house scope");if(r[16]&&!options.include_pending)continue;
  Item item;item.stored.custody_id=std::stoull(r[0]);item.native_item_id=std::stoul(r[1]);item.revision=std::stoull(r[2]);item.native_estate_id=native;item.house_id=r[14]?std::stoull(r[14]):0;item.realm_estate_id=r[22]?std::stoul(r[22]):0;if(!item.realm_estate_id)throw std::invalid_argument("Missing vault realm identity");item.stored.estate_id=r[17]?std::stoul(r[17]):0;
  if(!item.revision||!item.native_item_id||item.native_item_id==UINT32_MAX||std::string(r[4])!=LiveItems::Service::SchemaDescriptor()||std::stoul(r[7])!=native||std::stoul(r[8])!=item.stored.estate_id||std::stoul(r[9])!=1||std::stoull(r[10])!=item.stored.custody_id||r[12]||r[13])throw std::invalid_argument("Changed vault identity");
  item.stored.row=CustodyCodec::Decode(Unhex(r[5]));ItemPayload::ValidateHeld(db,item.stored.row,item.stored.custody_id);if(item.stored.row.values.character_id!=std::stoul(r[3])||Unhex(r[11])!=CustodyCodec::Encode(item.stored.row))throw std::invalid_argument("Changed vault source");
  auto cat=Unhex(r[6]);if(cat.size()!=16)throw std::invalid_argument("Changed vault catalog");CustodyCodec::Reader c{cat};item.stored.catalog={int32_t(c.N(4)),int32_t(c.N(4)),int32_t(c.N(4)),int32_t(c.N(4))};auto current=Profile(db,item.stored.row);
  if(current.item_class!=item.stored.catalog.item_class||current.bag_slots!=item.stored.catalog.bag_slots||current.lore_group!=item.stored.catalog.lore_group||current.no_drop!=item.stored.catalog.no_drop)throw std::invalid_argument("Changed vault item metadata");
  if(packed){auto m=packed->begin();if(!std::stoull(m[0])||!std::stoull(m[1])||std::stoul(m[2])!=native||(m[3]?std::stoull(m[3]):0)!=house||std::stoul(m[4])!=item.realm_estate_id||std::stoull(m[5])!=item.revision||Unhex(m[6])!=CustodyCodec::Encode(item.stored.row)||Unhex(m[7])!=cat||std::string(m[8])!=LiveItems::Service::SchemaDescriptor()||std::stoul(m[9])!=item.stored.row.values.character_id||(carried&&std::stoul(m[12])!=item.stored.row.values.character_id))throw std::invalid_argument("Changed hidden vault containment");item.contained_in_crate=std::stoull(m[0]);item.membership_revision=std::stoull(m[1]);if(!options.include_contained)continue;}
  item.creator_name=r[15];if(item.creator_name.empty()||item.creator_name.size()>63)throw std::invalid_argument("Invalid vault creator");result.push_back(std::move(item));
 }return result;
}
inline void ContextShape(const LiveItems::VaultContext& c){
 if(c.crate_id?(!c.crate_revision||c.crate_revision==UINT64_MAX||c.member_revision==UINT64_MAX):bool(c.crate_revision||c.member_revision))throw std::invalid_argument("Invalid vault crate context");
}
inline std::vector<Item> ContextItems(Database& db,const LiveItems::VaultContext& c){
 ContextShape(c);auto rows=Items(db,c.estate_id,c.native_estate_id,c.house_id,{bool(c.crate_id),false,false});
 rows.erase(std::remove_if(rows.begin(),rows.end(),[&](const auto& i){return i.contained_in_crate!=c.crate_id;}),rows.end());return rows;
}
struct Target {uint64_t crate_id=0,vault_count=0;EstateWire::Definition definition;uint64_t catalog_revision=0;std::string catalog_key;uint64_t house_id=0;uint32_t native=0;size_t placed=0;AccessPolicy::EffectiveAccess access;};
inline Target CurrentTarget(Database& db,const LiveItems::Lease& lease,EstateId estate,const LiveItems::VaultContext& context,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* houses,const InteriorContent::ProfilePolicy* profile){
 if(!context.estate_id||!context.native_estate_id||!context.catalog_revision||!context.source.zone_id||!context.source.instance_id)throw std::invalid_argument("Missing current vault realm");estate=context.estate_id;ContextShape(context);Target out;out.native=context.native_estate_id;
 std::shared_ptr<const CrateBinding::Allocation> container;
 auto select=[&](const auto& entries){if(!context.crate_id)return;for(const auto& item:entries)if(item.crate&&item.crate->binding.crate_id==context.crate_id){if(container||item.stored.stowed||item.crate->packed)throw std::invalid_argument("Unavailable vault crate");container=item.crate;}};
 if(context.house_id){
  if(!houses||!profile||!context.house_revision||!context.profile_revision)throw std::invalid_argument("Missing vault closet authority");auto read=context.source.zone_id==712?Scene::Service::ReadCurrentAttachedInteriorUnderTransaction(db,lease,context.house_id,fence,*houses,profile,houses->Crates()):Scene::Service::ReadInteriorSceneUnderTransaction(db,lease,context.source,fence,*houses,*profile,houses->Crates());
  if(!read||!read.value.house||!read.value.house->interior||!read.value.room_profile)throw std::invalid_argument("Closet read denied");const auto& room=read.value;const auto physical=room.exterior_source.value_or(room.context.source);
  if(room.context.estate_id!=estate||room.house->house_id!=context.house_id||room.house->revision!=context.house_revision||room.profile_revision!=context.profile_revision||room.house->interior->native_estate_id!=context.native_estate_id||!SameRoute(physical,context.source)||room.context.catalog_revision!=context.catalog_revision)throw std::invalid_argument("Changed closet context");
  out.definition=room.room_profile->definition;out.catalog_key=room.house->interior->catalog_key;out.catalog_revision=room.context.catalog_revision;out.house_id=context.house_id;out.placed=room.contents.size();out.access=room.access.effective;select(room.contents);
 }else{
  if(context.house_revision||context.profile_revision)throw std::invalid_argument("Invalid shed context");auto read=context.source.zone_id==712?Scene::Service::ReadSceneUnderTransaction(db,lease,estate,fence,houses,houses?houses->Crates():nullptr):houses?Scene::Service::ReadParentSceneUnderTransaction(db,lease,context.source,fence,*houses,houses->Crates()):LiveItems::Result<Scene::Snapshot>{{},LiveItems::Error::Unauthorized};if(!read)throw std::invalid_argument("Shed read denied");const auto& yard=read.value;
  if(yard.native_plot_id!=context.native_estate_id||yard.current_catalog_revision!=context.catalog_revision||(context.source.zone_id==712&&!SameRoute(yard.route,context.source)))throw std::invalid_argument("Changed shed context");out.definition=yard.definition;out.catalog_revision=yard.current_catalog_revision;out.placed=yard.items.size();out.access=yard.access;auto key=Query(db,"SELECT catalog_key FROM housing_native_estates WHERE native_id="+std::to_string(out.native)+" AND active=1 FOR UPDATE");if(key.RowCount()!=1||!key.begin()[0])throw std::invalid_argument("Missing shed catalog");out.catalog_key=key.begin()[0];select(yard.items);
 }
 auto total=out.house_id?Query(db,"SELECT COUNT(*) FROM housing_house_item_scopes c WHERE c.house_id="+std::to_string(out.house_id)+" AND NOT EXISTS (SELECT 1 FROM housing_live_vault_items v WHERE v.custody_id=c.custody_id)"+CrateMemberScope::ExteriorPredicate(db)+" FOR UPDATE"):Query(db,"SELECT COUNT(*) FROM housing_live_placed_items p JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id WHERE r.native_estate_id="+std::to_string(out.native)+" AND r.state=1"+(CrateMemberScope::Installed(db)?" AND NOT EXISTS (SELECT 1 FROM housing_crate_members packed WHERE packed.custody_id=p.custody_id)":"")+" FOR UPDATE");if(total.RowCount()!=1)throw std::invalid_argument("Missing total vault realm count");out.placed=std::stoull(total.begin()[0]);
 if(context.crate_id){
  if(!CrateMembersInstalled(db)||!container||container->created_by_character_id!=lease.actor.character_id||container->binding.revision!=context.crate_revision||container->binding.parent_native_estate_id!=context.native_estate_id||container->binding.catalog_revision!=context.catalog_revision||container->binding.estate_id!=estate)throw std::invalid_argument("Changed vault crate authority");
  auto definition=CrateBinding::ReadDefinitionUnderTransaction(db,*container);auto members=CrateBinding::ValidateMembersUnderTransaction(db,*container);if(!definition||!members)throw std::invalid_argument("Invalid vault crate contents");
  auto count=Query(db,"SELECT COUNT(*) FROM housing_crate_vault_members WHERE crate_id="+std::to_string(context.crate_id)+" FOR UPDATE");if(count.RowCount()!=1)throw std::invalid_argument("Missing vault crate count");out.vault_count=std::stoull(count.begin()[0]);if(out.vault_count>members.value)throw std::invalid_argument("Invalid vault crate count");out.crate_id=context.crate_id;out.placed=members.value-out.vault_count;out.definition=definition.value;
 }
 return out;
}
inline uint64_t Count(Database& db,uint32_t native){uint64_t total=0;std::set<uint64_t> seen;auto groups=Query(db,"SELECT DISTINCT v.realm_estate_id,v.house_id FROM housing_live_vault_items v JOIN housing_live_placement_reservations r ON r.native_item_id=v.native_item_id WHERE r.native_estate_id="+std::to_string(native)+" AND r.state=1 FOR UPDATE");for(auto g=groups.begin();g!=groups.end();++g){if(!g[0])throw std::invalid_argument("Missing vault capacity realm");for(const auto& item:Items(db,std::stoul(g[0]),native,g[1]?std::stoull(g[1]):0,{true,true,false}))if(!item.contained_in_crate&&seen.insert(item.stored.custody_id).second)++total;}return total;}

inline uint64_t Count(Database& db,const Target& target){return target.crate_id?target.vault_count:Count(db,target.native);}
// Caller owns the enclosing operation transaction and current context locks.
// Every member mutation increments both its witness and its container revision.
inline void CrateWrite(Database& db,const LiveItems::VaultContext& c,const LiveItems::Stored& stored,uint64_t vault_revision,int mode){
 ContextShape(c);if(!c.crate_id)return;const auto id=std::to_string(stored.custody_id),crate=std::to_string(c.crate_id);auto one=[&](const std::string& s){if(Query(db,s).RowsAffected()!=1)throw std::invalid_argument("Vault crate mutation conflict");};
 if(mode==1){
  if(c.member_revision)throw std::invalid_argument("New vault member revision");
  one("INSERT INTO housing_crate_vault_members(custody_id,crate_id,revision,origin_native_estate_id,origin_house_id,origin_realm_estate_id,vault_revision,source_row,catalog_blob,schema_descriptor) SELECT v.custody_id,"+crate+",1,r.native_estate_id,v.house_id,v.realm_estate_id,v.revision,c.row_blob,c.catalog_blob,c.schema_descriptor FROM housing_live_vault_items v JOIN housing_live_item_custody c USING(custody_id) JOIN housing_live_placement_reservations r ON r.native_item_id=v.native_item_id WHERE v.custody_id="+id+" AND v.revision="+std::to_string(vault_revision)+" AND r.state=1 AND r.custody_id=v.custody_id AND r.source_row=c.row_blob");
 }else{
  if(!c.member_revision)throw std::invalid_argument("Missing source vault member revision");
  const auto predicate=" WHERE custody_id="+id+" AND crate_id="+crate+" AND revision="+std::to_string(c.member_revision)+" AND vault_revision="+std::to_string(c.expected_revision);
  if(mode==2)one("UPDATE housing_crate_vault_members SET revision=revision+1,vault_revision="+std::to_string(vault_revision)+",source_row=UNHEX('"+Hex(CustodyCodec::Encode(stored.row))+"')"+predicate);
  else if(mode==0)one("DELETE FROM housing_crate_vault_members"+predicate);else throw std::invalid_argument("Invalid vault member operation");
 }
 one("UPDATE housing_crate_instances SET revision=revision+1 WHERE crate_id="+crate+" AND revision="+std::to_string(c.crate_revision)+" AND packed=0");
}
inline bool CratePostimage(Database& db,const LiveItems::VaultContext& c,const LiveItems::Stored& stored,uint64_t vault_revision,int mode){
 ContextShape(c);if(!CrateMembersInstalled(db))return !c.crate_id;
 auto member=Query(db,"SELECT crate_id,revision,vault_revision,HEX(source_row),HEX(catalog_blob),schema_descriptor,origin_native_estate_id,origin_house_id,origin_realm_estate_id FROM housing_crate_vault_members WHERE custody_id="+std::to_string(stored.custody_id)+" FOR UPDATE");
 if(!c.crate_id)return !member.RowCount();
 auto crate=Query(db,"SELECT revision FROM housing_crate_instances WHERE crate_id="+std::to_string(c.crate_id)+" FOR UPDATE");if(crate.RowCount()!=1||!crate.begin()[0]||std::stoull(crate.begin()[0])!=c.crate_revision+1)return false;
 if(!mode)return !member.RowCount();if(member.RowCount()!=1)return false;auto m=member.begin();for(unsigned i=0;i<9;++i)if(i!=7&&!m[i])return false;
 std::vector<uint8_t> cat;for(auto n:{stored.catalog.item_class,stored.catalog.bag_slots,stored.catalog.lore_group,stored.catalog.no_drop})CustodyCodec::W(cat,uint32_t(n),4);
 return std::stoull(m[0])==c.crate_id&&std::stoull(m[1])==(mode==1?1:c.member_revision+1)&&std::stoull(m[2])==vault_revision&&Unhex(m[3])==CustodyCodec::Encode(stored.row)&&Unhex(m[4])==cat&&std::string(m[5])==LiveItems::Service::SchemaDescriptor()&&std::stoul(m[6])==c.native_estate_id&&(m[7]?std::stoull(m[7]):0)==c.house_id&&std::stoul(m[8])==c.estate_id;
}
inline LiveItems::Error CapacityError(const Target& target,uint64_t vault){if(uint64_t(target.placed)+vault>=target.definition.max_items)return LiveItems::Error::RealEstateFull;if(vault>=target.definition.max_non_estate_items)return LiveItems::Error::VaultFull;return LiveItems::Error::None;}
inline bool Capacity(const Target& target,uint64_t vault){return CapacityError(target,vault)==LiveItems::Error::None;}
// Immutable post-commit recovery validation: no fresh permission is required to
// apply an already-known own inventory outcome. It grants no new mutation.
inline bool Outcome(Database& db,const LiveItems::VaultContext& context,const LiveItems::Stored& stored,bool store){
 if(!Installed(db)||!CratePostimage(db,context,stored,store?1:0,store?1:0))return false;auto rows=Query(db,"SELECT v.native_item_id,v.revision,v.house_id,v.realm_estate_id,r.native_estate_id,r.state,r.custody_id,HEX(r.source_row),r.estate_id FROM housing_live_vault_items v JOIN housing_live_placement_reservations r ON r.native_item_id=v.native_item_id WHERE v.custody_id="+std::to_string(stored.custody_id)+" FOR UPDATE");
 auto scope=Query(db,"SELECT house_id FROM housing_house_item_scopes WHERE custody_id="+std::to_string(stored.custody_id)+" FOR UPDATE");
 if(!store){if(rows.RowCount()||scope.RowCount())return false;auto r=Query(db,"SELECT state,custody_id FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(context.expected_native_item_id)+" FOR UPDATE");return r.RowCount()==1&&r.begin()[0]&&std::stoul(r.begin()[0])==2&&!r.begin()[1];}
 if(rows.RowCount()!=1)return false;auto r=rows.begin();for(unsigned i=0;i<9;++i)if(i!=2&&!r[i])return false;return std::stoull(r[1])==1&&(r[2]?std::stoull(r[2]):0)==context.house_id&&std::stoul(r[3])==context.estate_id&&std::stoul(r[4])==context.native_estate_id&&std::stoul(r[5])==1&&std::stoull(r[6])==stored.custody_id&&Unhex(r[7])==CustodyCodec::Encode(stored.row)&&std::stoul(r[8])==stored.estate_id&&(context.house_id?(scope.RowCount()==1&&scope.begin()[0]&&std::stoull(scope.begin()[0])==context.house_id):scope.RowCount()==0);
}

}
#endif
