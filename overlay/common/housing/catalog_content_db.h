#include "npc_item_metadata_db.h"
#ifndef EQEMU_HOUSING_CATALOG_CONTENT_DB_H
#define EQEMU_HOUSING_CATALOG_CONTENT_DB_H
#include "catalog_content_binding.h"
#include "../database.h"
#include "mount_flags_db.h"
namespace Housing::ContentBinding {
inline PlacementPolicy::Geometry KnownChairGeometry(){return StaticFurniture::Lookup(54573)->geometry;}
namespace DatabaseDetail {
inline MySQLRequestResult Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::invalid_argument("Housing content query failed");return r;}
inline uint32_t Number(const char* p){if(!p)throw std::invalid_argument("Missing housing content metadata");auto n=std::stoull(p);if(n>UINT32_MAX)throw std::invalid_argument("Housing content metadata range");return uint32_t(n);}
inline float Bits(const char* p){uint32_t bits=Number(p);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f))throw std::invalid_argument("Nonfinite housing content metadata");return f;}
}
// Caller owns transaction and has freshly authenticated/catalog-origin/route
// checked the target. Source fields use the pinned shareddb.cpp1008..1015 and
// RoF2.cpp7017..7032 metadata mapping, not default guessed client metadata.
inline bool ColumnsInstalled(Database& db){auto r=DatabaseDetail::Q(db,"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='housing_live_placement_reservations' AND column_name IN ('content_binding_version','content_binding_digest')");auto n=DatabaseDetail::Number(r.begin()[0]);if(n!=0&&n!=2)throw std::invalid_argument("Partial housing content schema");return n==2;}
inline std::string SelectColumns(Database& db,const std::string& prefix=""){return ColumnsInstalled(db)?prefix+"content_binding_version,HEX("+prefix+"content_binding_digest)":"NULL,NULL";}
inline std::optional<Identity> DecodeIdentity(const char* version,const char* digest){if(!version&&!digest)return {};if(!version||!digest||!SupportedVersion(DatabaseDetail::Number(version))||std::string(digest).size()!=64)throw std::invalid_argument("Invalid housing content identity");Identity out;out.version=DatabaseDetail::Number(version);auto nibble=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;throw std::invalid_argument("Invalid housing content hex");};for(size_t i=0;i<32;++i)out.digest[i]=(nibble(digest[2*i])<<4)|nibble(digest[2*i+1]);return out;}
inline Identity Current(Database& db,const std::string& key,Catalog::Bundle bundle,uint32_t native_id,Route route,uint32_t trusted_item_id,const HouseRoute* house_route=nullptr,const CrateProfile* crate_profile=nullptr){
 using namespace DatabaseDetail;
 if(house_route&&crate_profile)throw std::invalid_argument("Ambiguous housing content kind");
 bundle.key=key;bundle.nodes.clear();
 auto nodes=Q(db,"SELECT n.native_id,n.node_key,p.node_key,n.neighborhood_id,COALESCE(n.plot_id,0),COALESCE(n.ownership_estate_id,0),n.definition_id,n.static_id FROM housing_native_estates n LEFT JOIN housing_native_estates p ON p.native_id=n.parent_native_id AND p.active=1 AND p.catalog_key=n.catalog_key WHERE n.active=1 AND n.catalog_key='"+db.Escape(key)+"' ORDER BY n.native_id FOR UPDATE");
 if(nodes.RowCount()>1000)throw std::invalid_argument("Housing content nodes bound");
 for(auto r=nodes.begin();r.operator!=(nodes.end());++r)bundle.nodes.push_back({r[1],r[2]?r[2]:"",{Number(r[0])},Number(r[3]),Number(r[4]),Number(r[5]),Number(r[6]),Number(r[7])});
 auto meta=Q(db,"SELECT idfile,UNK224,UNK225,UNK226,UNK227,UNK228,UNK229,UNK230,UNK231 FROM items WHERE id="+std::to_string(trusted_item_id)+" FOR UPDATE");
 if(meta.RowCount()!=1||!meta.begin()[0])throw std::invalid_argument("Missing housing item metadata");
 auto r=meta.begin();const std::string model=r[0];
 auto npc_source=NpcItemMetadata::ReadUnderTransaction(db,trusted_item_id);auto npc=npc_source?NpcRuntime::Review(*npc_source):std::nullopt;
 const auto* supported=!npc&&!house_route&&!crate_profile?StaticFurniture::Lookup(trusted_item_id,model):nullptr;
 const auto* house=house_route?HouseGeometry::Lookup(trusted_item_id,model):nullptr;
 const auto* crate=crate_profile?CrateGeometry::Lookup(trusted_item_id,Number(r[4]),model):nullptr;
 if((npc_source&&!NpcItemMetadata::Ordinary(*npc_source)&&!npc)||!supported&&!house&&!crate&&!npc)throw std::invalid_argument("Unsupported housing content model");
 Input input{&bundle};input.npc=npc;input.native_estate_id=native_id;input.neighborhood_route=route;
 input.item={trusted_item_id,npc?npc->model_id:house?house->model_id:crate?crate->model_id:supported->model_id,Number(r[1]),Bits(r[5]),Bits(r[6]),bool(supported),house?(HouseTemplates::Type(trusted_item_id)):crate?7u:0u};
 if(house)input.house_route=*house_route;if(crate)input.crate_profile=*crate_profile;
 auto& native=input.native_metadata;native.placeable=input.item.placement_mask;native.ignore_collisions=Number(r[2])!=0;native.placement_type=Number(r[3]);native.definition_id=Number(r[4]);native.scale_min=input.item.minimum_scale;native.scale_max=input.item.maximum_scale;native.upkeep_id=Number(r[7]);
 auto maximum=std::stoll(r[8]);if(maximum<INT32_MIN||maximum>INT32_MAX)throw std::invalid_argument("Housing max per estate range");native.max_per_estate=int32_t(maximum);
 if(auto npc=NpcItemMetadata::ReadUnderTransaction(db,trusted_item_id))NpcItemMetadata::ToWire(native,*npc);
 if(supported&&StaticFurniture::Mounted(trusted_item_id)){auto flags=MountFlags::ReadUnderTransaction(db,trusted_item_id);input.mount_flags_known=flags.known;native.disable_rotation=flags.disable_rotation;native.disable_free_placement=flags.disable_free;}
 input.geometry=npc?NpcRuntime::Geometry(*npc):house?house->geometry:crate?crate->geometry:supported->geometry;
 input.model_member_sha256=npc?npc->source_sha256:house?house->member_sha256:crate?crate->member_sha256:supported->member_sha256;
 input.geometry_policy_revision=npc?1u:house?house->geometry_policy_revision:crate?1u:supported->geometry_policy_revision;
 input.native_metadata_verified=input.authenticated_server_context=input.permanent_route_verified=true;
 auto bound=Build(input);if(!bound)throw std::invalid_argument("Unsupported housing semantic content");return {bound.value.version,bound.value.digest};
}
}
#endif
