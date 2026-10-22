#ifndef EQEMU_HOUSING_INTERIOR_PLACEMENT_REVIEW_H
#define EQEMU_HOUSING_INTERIOR_PLACEMENT_REVIEW_H
#include "interior_profile_db.h"
#include "interior_item_content_db.h"
#include "inventory_row_db.h"
#include "house_item_scope_db.h"
#include "crate_property_limit.h"
#include "crate_member_scope_db.h"
#include "npc_runtime_db.h"
namespace Housing::InteriorContent {
struct PlacementReview {Snapshot profile;Fingerprint identity;PlacementPolicy::ItemMetadata metadata;PlacementPolicy::Geometry geometry;std::optional<PlacementPolicy::Transform> pose;};
// Destination-only review for a source whose inventory/custody authority was
// already proved by its owning transaction. This supplies no source authority.
namespace Detail {
inline LiveItems::Result<PlacementReview> ReviewDestinationItem(Database& db,Snapshot profile,
 const LiveItems::InventoryRow& expected,const std::optional<PlacementPolicy::Transform>& pose,bool room_capacity,LiveItems::InventorySessionFence& mount_fence,const LiveItems::Lease& mount_lease,bool npc_cache=false,const std::optional<NpcNativeFrame::Frame>& npc_frame={},const std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>>& selected_digest={}){
 using Error=LiveItems::Error;
 if(!profile.context.access.effective.Allows(AccessPolicy::Capability::AddItem))return {{},Error::Unauthorized};
 try{
  auto query=[&](const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())throw std::invalid_argument("Room destination review query");return result;};
  const auto& v=expected.values;if(v.augment_one||v.augment_two||v.augment_three||v.augment_four||v.augment_five||v.augment_six)return {{},Error::UnsupportedItem};
  auto loaded=ItemInputUnderTransaction(db,profile.room,v.item_id,selected_digest,!selected_digest);if(!loaded)return {{},loaded.error};auto input=std::move(loaded.value);const auto& native=input.native_metadata;
  if(input.npc&&!npc_cache)return {{},Error::Conflict};
  auto identity=Build(input);if(!identity)return {{},Error::UnsupportedItem};
  if(input.npc&&pose)input.geometry=NpcRuntime::Geometry(*input.npc,npc_frame,*pose);else if(!input.npc&&npc_frame)return {{},Error::InvalidArgument};
  if(CrateGeometry::TemplateDefinition(v.item_id)&&!CratePropertyLimit::AvailableUnderTransaction(db,profile.context.context.estate_id))return {{},Error::Conflict};
  if(pose){PlacementPolicy::Context context{input.room.native_estate_id,input.room.definition.type,input.room.region.id,profile.context.access.effective,true};context.mount_contact_verified=!input.geometry.mounting_type||MountForSelectedInputUnderTransaction(db,mount_fence,mount_lease,expected,*pose,input,LiveItems::MountSubject::NewSource());if(!CheckPlacement(input.room,context,input.item,input.geometry,*pose,PlacementPolicy::Operation::Add))return {{},Error::UnsupportedItem};}
  if(!HouseItemScope::Installed(db))return {{},Error::DatabaseFailure};
  auto used=query("SELECT COUNT(*) FROM housing_house_item_scopes c WHERE house_id="+std::to_string(input.room.house_id)+CrateMemberScope::ExteriorPredicate(db)+CrateMemberScope::VaultPredicate(db)+" FOR UPDATE");if(used.RowCount()!=1)return {{},Error::DatabaseFailure};auto count=std::stoull(used.begin()[0]);
  // Placeables consume total capacity. Vault capacity is independent.
  if(room_capacity&&count>=input.room.definition.max_items)return {{},Error::Conflict};
  if(input.npc&&!input.npc->service&&room_capacity&&!NpcRuntime::Capacity(input.room.definition.max_npc_items,NpcRuntime::Count(db,input.room.native_estate_id,input.room.house_id)))return {{},Error::NpcFull};
  if(room_capacity&&native.max_per_estate>0){auto items=query("SELECT HEX(c.row_blob) FROM housing_house_item_scopes scope LEFT JOIN housing_live_item_custody c ON c.custody_id=scope.custody_id WHERE scope.house_id="+std::to_string(input.room.house_id)+CrateMemberScope::ExteriorPredicate(db)+CrateMemberScope::VaultPredicate(db)+" FOR UPDATE");uint64_t same=0;
   for(auto r=items.begin();r!=items.end();++r){if(!r[0])return {{},Error::DatabaseFailure};std::string hex=r[0];if(hex.size()>2*CustodyCodec::MaximumPayload||hex.size()%2)return {{},Error::UnsupportedItem};std::vector<uint8_t> bytes;auto digit=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw std::invalid_argument("Room custody hex");};for(size_t i=0;i<hex.size();i+=2)bytes.push_back((digit(hex[i])<<4)|digit(hex[i+1]));if(CustodyCodec::Decode(bytes).values.item_id==v.item_id)++same;}
   if(same>=uint64_t(native.max_per_estate))return {{},Error::Conflict};
  }
  return {PlacementReview{std::move(profile),std::move(identity.value),input.item,input.geometry,pose},Error::None};
 }catch(...){return {{},Error::DatabaseFailure};}
}
}
// Inventory->room Add review only, inside the CURRENT owning transaction.
// Real memory source and canonical locked DB row must both match. No mutation,
// reservation, receipt/application or durable version3 activation occurs here.
inline LiveItems::Result<PlacementReview> ReviewInventoryAddUnderTransaction(Database& db,const LiveItems::Lease& lease,Route source,LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy& house_policy,const ProfilePolicy& profile_policy,const LiveItems::InventoryRow& expected,int32_t runtime_serial,const std::optional<PlacementPolicy::Transform>& pose={},const std::optional<AttachedSelection>& attached={},bool room_capacity=true,const std::optional<NpcNativeFrame::Frame>& npc_frame={},const std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>>& selected_digest={}){
 using Error=LiveItems::Error;
 auto profile=ReadProfileUnderTransaction(db,lease,source,fence,house_policy,profile_policy,attached);if(!profile)return {{},profile.error};
 if(!profile.value.context.access.effective.Allows(AccessPolicy::Capability::AddItem))return {{},Error::Unauthorized};
 if(expected.values.character_id!=lease.actor.character_id||!expected.values.item_id||(expected.null_fields&LiveItems::NullItemId)||runtime_serial<=0||!CustodyCodec::Valid(expected))return {{},Error::InvalidArgument};
 if(!fence.SourceMatches(expected,runtime_serial))return {{},Error::UnheldFence};
 try{
  auto query=[&](const std::string& sql){auto result=db.QueryDatabase(sql,false);if(!result.Success())throw std::invalid_argument("Room placement review query");return result;};
  auto inventory=query("SELECT "+std::string(InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(lease.actor.character_id)+" AND slot_id="+std::to_string(expected.values.slot_id)+" FOR UPDATE");
  if(inventory.RowCount()!=1||CustodyCodec::Encode(InventoryRowDB::Decode(inventory.begin()))!=CustodyCodec::Encode(expected))return {{},Error::Conflict};
  auto npc=NpcRuntime::Current(db,expected.values.item_id);const bool npc_cache=npc&&fence.NpcProfileMatchesUnderTransaction(db,lease,*npc);
  return Detail::ReviewDestinationItem(db,std::move(profile.value),expected,pose,room_capacity,fence,lease,npc_cache,npc_frame,selected_digest);
 }catch(...){return {{},Error::DatabaseFailure};}
}
}
#endif
