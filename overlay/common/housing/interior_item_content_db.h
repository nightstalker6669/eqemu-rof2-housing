#include "npc_item_metadata_db.h"
#ifndef EQEMU_HOUSING_INTERIOR_ITEM_CONTENT_DB_H
#define EQEMU_HOUSING_INTERIOR_ITEM_CONTENT_DB_H
#include "interior_content_binding.h"
#include "live_item_service.h"
#include "../database.h"
#include "mount_flags_db.h"
#include "interior_wall_slab_registry.h"
#include "centered_wall_policy.h"
namespace Housing::InteriorContent {
// Metadata only. Caller must first resolve a genuine current reviewed room and
// its real lease/source/ACL in this transaction. No placement rights are granted.
inline LiveItems::Result<Input> ItemInputUnderTransaction(Database& db,const Profile& profile,uint32_t trusted_item_id,const std::optional<std::array<uint8_t,SHA256_DIGEST_LENGTH>>& stored_digest={},bool new_add=false){
 using Error=LiveItems::Error;if(!profile.reviewed_room)return {{},Error::Unauthorized};
 try{
  auto query=[&](const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::invalid_argument("Room item content query");return r;};
  auto tx=query("SELECT @@in_transaction");if(tx.RowCount()!=1||std::stoul(tx.begin()[0])!=1)return {{},Error::InvalidArgument};
  auto rows=query("SELECT idfile,UNK224,UNK225,UNK226,UNK227,UNK228,UNK229,UNK230,UNK231,itemclass,bagslots FROM items WHERE id="+std::to_string(trusted_item_id)+" FOR UPDATE");
  if(rows.RowCount()!=1)return {{},Error::UnsupportedItem};auto row=rows.begin();for(size_t k=0;k<11;++k)if(!row[k])return {{},Error::UnsupportedItem};
  if(std::stoll(row[9])!=0||std::stoll(row[10])!=0)return {{},Error::UnsupportedItem};
  auto number=[](const char* value){size_t end=0;std::string s=value;auto n=std::stoull(s,&end);if(end!=s.size()||n>UINT32_MAX)throw std::invalid_argument("Room item metadata range");return uint32_t(n);};
  auto bits=[&](const char* value){auto b=number(value);float f;std::memcpy(&f,&b,4);if(!std::isfinite(f))throw std::invalid_argument("Room item scale");return f;};
  auto npc_source=NpcItemMetadata::ReadUnderTransaction(db,trusted_item_id);auto npc=npc_source?NpcRuntime::Review(*npc_source):std::nullopt;
  const auto* model=npc?nullptr:StaticFurniture::Lookup(trusted_item_id,std::string(row[0]));
  const auto* crate=CrateGeometry::Lookup(trusted_item_id,number(row[4]),std::string(row[0]));
  if((npc_source&&!NpcItemMetadata::Ordinary(*npc_source)&&!npc)||!model&&!crate&&!npc)return {{},Error::UnsupportedItem};
  Input input;input.room=profile;input.npc=npc;input.item={trusted_item_id,npc?npc->model_id:crate?crate->model_id:model->model_id,number(row[1]),bits(row[5]),bits(row[6]),!crate&&!npc,crate?7u:0u};input.geometry=npc?NpcRuntime::Geometry(*npc):crate?crate->geometry:model->geometry;input.model_member_sha256=npc?npc->source_sha256:crate?crate->member_sha256:model->member_sha256;input.geometry_policy_revision=npc?1u:crate?CrateGeometry::GeometryPolicyRevision:model->geometry_policy_revision;input.authenticated_current_house=input.native_metadata_verified=true;
  auto& native=input.native_metadata;native.placeable=input.item.placement_mask;native.ignore_collisions=number(row[2])!=0;native.placement_type=number(row[3]);native.definition_id=number(row[4]);native.scale_min=input.item.minimum_scale;native.scale_max=input.item.maximum_scale;native.upkeep_id=number(row[7]);auto max=std::stoll(row[8]);if(max<INT32_MIN||max>INT32_MAX)return {{},Error::UnsupportedItem};native.max_per_estate=int32_t(max);if(auto npc=NpcItemMetadata::ReadUnderTransaction(db,trusted_item_id))NpcItemMetadata::ToWire(native,*npc);if(model&&StaticFurniture::Mounted(trusted_item_id)){auto flags=MountFlags::ReadUnderTransaction(db,trusted_item_id);input.mount_flags_known=flags.known;native.disable_rotation=flags.disable_rotation;native.disable_free_placement=flags.disable_free;}if(stored_digest||new_add){auto policy=InteriorWallSlab::Eligible(input.geometry)&&input.item.furniture_only&&!input.npc?InteriorWallSlab::Reviewed(profile):std::nullopt;auto selected=CenteredWall::Select(input,policy?&*policy:nullptr,stored_digest,new_add);if(!selected)return {{},Error::Conflict};input=std::move(selected->input);}return {std::move(input),Error::None};
 }catch(...){return {{},Error::DatabaseFailure};}
}
inline LiveItems::Result<Input> ItemInputForStoredUnderTransaction(Database& db,const Profile& profile,const LiveItems::Stored& source){
 using Error=LiveItems::Error;const auto* model=StaticFurniture::Lookup(source.row.values.item_id);if(!model||!InteriorWallSlab::Eligible(model->geometry)||!source.placement)return ItemInputUnderTransaction(db,profile,source.row.values.item_id);
 try{const auto& r=source.placement->reservation;auto q=db.QueryDatabase("SELECT house_id,content_version,HEX(content_digest) FROM housing_interior_placement_reservations WHERE native_item_id="+std::to_string(r.native_item_id)+" FOR UPDATE",false);if(!q.Success()||q.RowCount()!=1||!q.begin()[0]||!q.begin()[1]||!q.begin()[2]||std::stoull(q.begin()[0])!=profile.house_id||std::stoul(q.begin()[1])!=Version)return {{},Error::Conflict};std::string hex=q.begin()[2];if(hex.size()!=64)return {{},Error::Conflict};std::array<uint8_t,SHA256_DIGEST_LENGTH> digest{};auto nibble=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw std::invalid_argument("Room tag digest");};for(size_t i=0;i<digest.size();++i)digest[i]=uint8_t(nibble(hex[2*i])*16+nibble(hex[2*i+1]));return ItemInputUnderTransaction(db,profile,source.row.values.item_id,digest);
 }catch(...){return {{},Error::DatabaseFailure};}
}

// Same production resolver used by the zone adapter and focused mutation gate.
// Current reviewed profile/source/ACL are prerequisites of the owning caller.
inline LiveItems::Result<InteriorWallSlab::Selection> MountSelectionUnderTransaction(Database& db,const Profile& profile,uint32_t item_id,const LiveItems::InteriorMountContext& context){
 using Error=LiveItems::Error;if(!profile.reviewed_room||!context.house_id||context.house_id!=profile.house_id)return {{},Error::Unauthorized};
 auto input=ItemInputUnderTransaction(db,profile,item_id);if(!input)return {{},input.error};
 auto policy=InteriorWallSlab::Eligible(input.value.geometry)?InteriorWallSlab::Reviewed(profile):std::nullopt;
 auto selected=CenteredWall::Select(input.value,policy?&*policy:nullptr,context.content_digest,false);
 if(!selected)return {{},Error::Conflict};return {std::move(*selected),Error::None};
}
// Input is already selected by the owning mutation from a locked reservation,
// durable stored digest, or genuine fresh Add. The adapter re-resolves this
// exact identity against its independently fresh room/profile; no latest fallback.
inline bool MountForSelectedInputUnderTransaction(Database& db,LiveItems::InventorySessionFence& fence,const LiveItems::Lease& lease,const LiveItems::InventoryRow& row,const PlacementPolicy::Transform& pose,const Input& input,const LiveItems::MountSubject& subject={}){
 if(!input.room.reviewed_room||!input.room.house_id||input.item.item_id!=row.values.item_id)return false;
 auto identity=Build(input);return identity&&fence.MountInteriorSurfaceUnderTransaction(db,lease,row,pose,input.room.native_estate_id,{input.room.house_id,identity.value.digest,subject});
}

}
#endif
