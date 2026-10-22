#include "runtime_activation_db.h"
#ifndef EQEMU_HOUSING_INTERIOR_PROFILE_DB_H
#define EQEMU_HOUSING_INTERIOR_PROFILE_DB_H
#include "interior_content_binding.h"
#include "scene_service.h"
#include "house_binding_service.h"
#include "interior_capacity_policy_db.h"
#include <cctype>
namespace Housing::InteriorContent {
struct Snapshot {Scene::InteriorSnapshot context;Profile room;uint64_t profile_revision=0;};
struct AnchorSelection {EstateId estate_id=0;uint64_t house_id=0;};
struct AttachedSelection {EstateId estate_id=0;uint64_t house_id=0;Route physical_source;const CrateBinding::CustodyContext* crates=nullptr;Scene::AttachedRead read=Scene::AttachedRead::Storage;};
class ProfilePolicy {
public:
 virtual ~ProfilePolicy()=default;
 virtual bool ReviewedAnchorUnderTransaction(Database&,LiveItems::InventorySessionFence&,const LiveItems::Lease&,const Snapshot&)const{return false;}

 // Read-only server review of the exact original assets, room/geometry/capacity
 // and selected immutable manifest. Never infer approval from DB existence,
 // filenames, the packet or an unvalidated hash string. Default denies.
 virtual bool ReviewedUnderTransaction(Database&,LiveItems::InventorySessionFence&,const LiveItems::Lease&,const Snapshot&)const{return false;}
};
// Caller owns feature gating and the actual item transaction. Current source,
// stable house, outer identity, held lease and inherited Enter are re-resolved
// on this SAME connection. Add/Move/Remove are separately required for writes.
inline LiveItems::Result<Snapshot> ReadProfileUnderTransaction(Database& db,const LiveItems::Lease& lease,Route source,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy& house_policy,const ProfilePolicy& profile_policy,const std::optional<AttachedSelection>& attached={},const std::optional<AnchorSelection>& anchor={}){
 using Error=LiveItems::Error;
 if(attached&&anchor)return {{},Error::InvalidArgument};
 auto context=anchor?Scene::Service::ReadAnchorAttachedInteriorUnderTransaction(db,lease,anchor->estate_id,anchor->house_id,fence,house_policy):attached?Scene::Service::ReadAttachedInteriorUnderTransaction(db,lease,attached->estate_id,attached->house_id,fence,house_policy,nullptr,false,attached->crates,attached->read):Scene::Service::ReadInteriorUnderTransaction(db,lease,source,fence,house_policy);if(!context)return {{},context.error};
 if(context.value.context.source.zone_id!=source.zone_id||context.value.context.source.zone_version!=source.zone_version||context.value.context.source.instance_id!=source.instance_id)return {{},Error::Conflict};
 if(attached&&(!context.value.exterior_source||context.value.exterior_source->zone_id!=attached->physical_source.zone_id||context.value.exterior_source->zone_version!=attached->physical_source.zone_version||context.value.exterior_source->instance_id!=attached->physical_source.instance_id))return {{},Error::Conflict};
 try{
  auto query=[&](const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())throw std::invalid_argument("Room profile query failed");return result;};
  auto tables=query("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_house_room_profiles'");
  if(!tables.RowCount())return {{},Error::NotFound};if(tables.RowCount()!=1||!tables.begin()[0]||std::string(tables.begin()[0])!="InnoDB")return {{},Error::DatabaseFailure};
  if(!context.value.house||!context.value.house->interior)return {{},Error::Conflict};const auto& house=*context.value.house;const auto& interior=*house.interior;
  auto rows=query("SELECT revision,origin,client_build,zone_short_name,HEX(client_asset_sha256),HEX(collision_map_sha256),HEX(manifest_sha256),HEX(definitions_blob),HEX(regions_blob) FROM housing_house_room_profiles WHERE house_id="+std::to_string(house.house_id)+" FOR UPDATE");if(rows.RowCount()!=1)return {{},Error::NotFound};auto row=rows.begin();for(size_t i=0;i<9;++i)if(!row[i])return {{},Error::DatabaseFailure};
  auto number=[](const char* v){size_t end=0;std::string s=v;auto result=std::stoull(s,&end);if(end!=s.size())throw std::invalid_argument("Room profile numeric field");return result;};
  auto hex=[](const char* value,size_t maximum){std::string s=value;if(s.size()>2*maximum||s.size()%2)throw std::invalid_argument("Room profile payload bound");std::vector<uint8_t> bytes;auto nibble=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;throw std::invalid_argument("Room profile hex field");};for(size_t i=0;i<s.size();i+=2)bytes.push_back((nibble(s[i])<<4)|nibble(s[i+1]));return bytes;};
  Snapshot out;out.profile_revision=number(row[0]);auto origin=number(row[1]);auto build=number(row[2]);if(!out.profile_revision||build!=20130510||(origin!=1&&origin!=2))return {{},Error::UnsupportedItem};
  if(origin==2){auto name=query("SELECT DATABASE()");if(name.RowCount()!=1||!name.begin()[0])return {{},Error::DatabaseFailure};std::string current=name.begin()[0];if(!Housing::RuntimeActivation::AllowsReconstructedContent(db))return {{},Error::Unauthorized};}
  auto bundle=Catalog::DecodePayloads(hex(row[7],NeighborhoodWire::MaximumPayload),hex(row[8],RegionWire::MaximumPayload));if(bundle.definitions.empty()||bundle.definitions.size()>1001||bundle.regions.empty()||!bundle.costs.empty())return {{},Error::UnsupportedItem};
  auto& p=out.room;p.house_id=house.house_id;p.native_estate_id=interior.native_estate_id;p.route=source;p.origin=static_cast<Catalog::Origin>(origin);p.client_build=uint32_t(build);p.zone_short_name=row[3];bool found_definition=false;for(const auto& d:bundle.definitions){if(d.id==interior.definition_id){if(found_definition)return {{},Error::Conflict};p.definition=d;found_definition=true;}else p.child_definitions.push_back(d);}if(!found_definition||!ValidChildDefinitions(p))return {{},Error::Conflict};p.region=bundle.regions[0];p.additional_regions.assign(bundle.regions.begin()+1,bundle.regions.end());
  auto digest=[](const char* value){std::string s=value;for(auto& c:s)c=char(std::tolower(static_cast<unsigned char>(c)));if(!ContentBinding::Detail::Hash(s))throw std::invalid_argument("Room profile digest");return s;};p.client_asset_sha256=digest(row[4]);p.collision_map_sha256=digest(row[5]);p.manifest_sha256=digest(row[6]);
  const auto& outer=context.value.context.house_definition;
  if(p.definition.id!=interior.definition_id||p.definition.type!=outer.type||p.definition.zone_id!=source.zone_id||p.region.id!=p.definition.area_id||!p.definition.area_id||!p.definition.max_items||p.definition.max_non_estate_items>p.definition.max_items||!OriginalInteriors::Matches(source.zone_id,p.zone_short_name))return {{},Error::Conflict};
  RegionPayload(p);out.context=std::move(context.value);
  if(!(anchor?profile_policy.ReviewedAnchorUnderTransaction(db,fence,lease,out):profile_policy.ReviewedUnderTransaction(db,fence,lease,out)))return {{},Error::Unauthorized};if(!InteriorCapacity::ApplyUnderTransaction(db,p,out.profile_revision))return {{},Error::Conflict};p.reviewed_room=true;
  return {std::move(out),Error::None};
 }catch(...){return {{},Error::DatabaseFailure};}
}
}
#endif
