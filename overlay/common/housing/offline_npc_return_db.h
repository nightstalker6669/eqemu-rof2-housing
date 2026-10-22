#ifndef EQEMU_HOUSING_OFFLINE_NPC_RETURN_DB_H
#define EQEMU_HOUSING_OFFLINE_NPC_RETURN_DB_H
#include "runtime_activation_db.h"
#include "live_item_service.h"
#include "custody_codec.h"
#include "catalog_content_db.h"
#include "interior_item_content_db.h"
#include "npc_native_frame_db.h"
#include "item_storage_db.h"
#include <cctype>

namespace Housing::OfflineNpcReturn {
// Trusted server-only validation. No transaction, recipient authority, delivery,
// or inventory/session fencing is acquired here. Caller owns all of those and
// must roll back the ENTIRE disposition transaction on any exception.
struct Validated {
 LiveItems::Stored source;
 NpcRuntime::Profile profile;
 uint64_t house_id=0;
 std::optional<InteriorContent::Profile> room;
};
namespace Detail {
inline void Require(bool value){if(!value)throw std::invalid_argument("Offline NPC return identity");}
inline auto Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::runtime_error("Offline NPC return SQL");return r;}
inline uint64_t N(const char* p){Require(p&&*p);std::string s=p;Require(s.find_first_not_of("0123456789")==std::string::npos);size_t end=0;auto n=std::stoull(s,&end);Require(end==s.size());return n;}
inline std::string I(uint64_t n){return std::to_string(n);}
inline std::vector<uint8_t> Unhex(const char* p){Require(p);std::string s=p;Require(s.size()%2==0&&s.size()<=2*InteriorContent::MaximumManifestBytes);auto nibble=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;throw std::invalid_argument("Offline NPC hex");};std::vector<uint8_t> out;out.reserve(s.size()/2);for(size_t i=0;i<s.size();i+=2)out.push_back(uint8_t(nibble(s[i])*16+nibble(s[i+1])));return out;}
inline std::string Lower(const char* p){Require(p);std::string out=p;for(auto& c:out)c=char(std::tolower(static_cast<unsigned char>(c)));return out;}
inline void Empty(Database& db,const std::string& sql){Require(Q(db,sql+" LIMIT 1 FOR UPDATE").RowCount()==0);}
inline bool SameFrame(const std::optional<NpcNativeFrame::Frame>& a,const std::optional<NpcNativeFrame::Frame>& b){return bool(a)==bool(b)&&(!a||(a->height_bits==b->height_bits&&a->origin_z_bits==b->origin_z_bits));}
inline std::vector<uint8_t> Pose(const PlacementPolicy::Transform& t){std::vector<uint8_t> out;auto word=[&](float f){Require(std::isfinite(f));uint32_t bits;std::memcpy(&bits,&f,4);CustodyCodec::W(out,bits,4);};word(t.heading);word(t.pitch);word(t.roll);word(t.scale);for(auto v:t.position)word(v);return out;}
inline void NoEvolutionOrContainers(Database& db,const LiveItems::Stored& s){
 const auto id=I(s.custody_id),item=I(s.row.values.item_id);
 for(const auto* table:{"housing_live_vault_items","housing_crate_members"})Empty(db,"SELECT custody_id FROM "+std::string(table)+" WHERE custody_id="+id);
 auto optional=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_crate_vault_members'");if(optional.RowCount()){Require(optional.RowCount()==1&&optional.begin()[0]&&std::string(optional.begin()[0])=="InnoDB");Empty(db,"SELECT custody_id FROM housing_crate_vault_members WHERE custody_id="+id);}
 Empty(db,"SELECT house_id FROM housing_house_locations WHERE location_kind=2 AND custody_id="+id);Empty(db,"SELECT crate_id FROM housing_crate_locations WHERE location_kind=2 AND custody_id="+id);
 Empty(db,"SELECT id FROM items_evolving_details WHERE item_id="+item);Empty(db,"SELECT id FROM character_evolving_items WHERE character_id="+I(s.row.values.character_id)+" AND item_id="+item+" AND deleted_at IS NULL");
}
// Geometry validation only. Offline retirement authority is checked by the
// caller; this local mask never grants a player capability or creates a lease.
inline PlacementPolicy::Context GeometryContext(uint32_t native,uint32_t kind,uint32_t region){PlacementPolicy::Context c;c.estate_id=native;c.estate_type=kind;c.region_id=region;c.authoritative=true;c.geometry_only=true;c.access.capabilities=AccessPolicy::KnownCapabilities;return c;}
}

// Classifies real current native metadata, rather than using frame presence.
// Missing/ordinary metadata returns null; malformed nonordinary metadata fails.
inline std::optional<NpcRuntime::Profile> CurrentProfileUnderTransaction(Database& db,uint32_t item){
 NpcNativeFrame::Transaction(db);auto source=NpcItemMetadata::ReadUnderTransaction(db,item);if(!source||NpcItemMetadata::Ordinary(*source))return {};auto profile=NpcRuntime::Review(*source);Detail::Require(bool(profile));return profile;
}

// `source` must be the caller's freshly inspected, locked custody. For an
// interior, `reviewed_room` is independently server-reviewed current geometry;
// its complete selected persisted profile is checked again below. No room is
// invented from a packet, a guessed hash, or an offline synthetic online lease.
inline Validated InspectUnderTransaction(Database& db,const LiveItems::Stored& source,uint32_t retiring_estate,uint32_t expected_owner,const InteriorContent::Profile* reviewed_room=nullptr){
 using namespace Detail;NpcNativeFrame::Transaction(db);Require(NpcNativeFrame::Installed(db));const auto& row=source.row;const auto& v=row.values;
 Require(retiring_estate&&expected_owner&&source.custody_id&&v.character_id==expected_owner&&CustodyCodec::Valid(row)&&source.placement&&source.placement->pose_revision);
 Require(!(row.null_fields&LiveItems::NullItemId)&&v.item_id&&!v.augment_one&&!v.augment_two&&!v.augment_three&&!v.augment_four&&!v.augment_five&&!v.augment_six&&!source.catalog.item_class&&!source.catalog.bag_slots);
 auto profile=CurrentProfileUnderTransaction(db,v.item_id);Require(bool(profile));const auto& placed=*source.placement;const auto& r=placed.reservation;const auto id=I(source.custody_id);
 Require(r.native_item_id&&r.native_item_id<UINT32_MAX&&r.native_estate_id&&r.native_estate_id<UINT32_MAX&&r.catalog_revision&&!r.catalog_key.empty());
 auto custody=Q(db,"SELECT estate_id,owner_character_id,schema_descriptor,HEX(row_blob),HEX(catalog_blob) FROM housing_live_item_custody WHERE custody_id="+id+" FOR UPDATE");Require(custody.RowCount()==1);auto c=custody.begin();Require(N(c[0])==source.estate_id&&N(c[1])==expected_owner&&c[2]&&std::string(c[2])==LiveItems::Service::SchemaDescriptor()&&Unhex(c[3])==CustodyCodec::Encode(row));
 std::vector<uint8_t> catalog;for(auto n:{source.catalog.item_class,source.catalog.bag_slots,source.catalog.lore_group,source.catalog.no_drop})CustodyCodec::W(catalog,uint32_t(n),4);Require(Unhex(c[4])==catalog);
 auto meta=Q(db,"SELECT itemclass,bagslots,loregroup,nodrop,evoitem,UNK224,UNK228,UNK229 FROM items WHERE id="+I(v.item_id)+" FOR UPDATE");Require(meta.RowCount()==1);auto m=meta.begin();Require(!N(m[0])&&!N(m[1])&&m[2]&&m[3]&&std::stoll(m[2])==source.catalog.lore_group&&std::stoll(m[3])==source.catalog.no_drop&&!N(m[4]));
 NoEvolutionOrContainers(db,source);
 auto pose=Q(db,"SELECT native_item_id,pose_revision,HEX(pose_blob) FROM housing_live_placed_items WHERE custody_id="+id+" FOR UPDATE");Require(pose.RowCount()==1&&N(pose.begin()[0])==r.native_item_id&&N(pose.begin()[1])==placed.pose_revision&&Unhex(pose.begin()[2])==Pose(placed.transform));
 auto reservation=Q(db,"SELECT native_estate_id,estate_id,state,character_id,HEX(source_row),catalog_key,catalog_revision,content_binding_version,HEX(content_binding_digest) FROM housing_live_placement_reservations WHERE native_item_id="+I(r.native_item_id)+" AND custody_id="+id+" FOR UPDATE");Require(reservation.RowCount()==1);auto p=reservation.begin();Require(N(p[0])==r.native_estate_id&&N(p[1])==source.estate_id&&r.estate_id==source.estate_id&&N(p[2])==1&&N(p[3])==expected_owner&&Unhex(p[4])==CustodyCodec::Encode(row)&&p[5]&&std::string(p[5])==r.catalog_key&&N(p[6])==r.catalog_revision);
 auto binding=ContentBinding::DecodeIdentity(p[7],p[8]);Require(bool(binding)==bool(r.content_binding)&&(!binding||ContentBinding::Same(*binding,*r.content_binding)));
 auto storage=ItemStorage::Locked(db,source.custody_id);Require(storage.revision==source.storage_revision&&storage.stowed==source.stowed&&(!storage.revision||storage.native_estate_id==r.native_estate_id));
 auto frame=NpcNativeFrame::ReadUnderTransaction(db,source.custody_id,r.native_item_id,placed.pose_revision,placed.transform);Require(SameFrame(frame,placed.npc_frame));auto geometry=NpcRuntime::Geometry(*profile,frame,placed.transform);
 auto scope=HouseItemScope::Locked(db,source.custody_id);Validated out{source,*profile,scope?scope->house_id:0,{}};
 auto bits=[](const char* value){const auto n=N(value);Require(n<=UINT32_MAX);uint32_t b=uint32_t(n);float f;std::memcpy(&f,&b,4);Require(std::isfinite(f));return f;};
 PlacementPolicy::ItemMetadata item{v.item_id,profile->model_id,uint32_t(N(m[5])),bits(m[6]),bits(m[7]),false,0};
 if(scope){
  Require(reviewed_room&&reviewed_room->reviewed_room&&reviewed_room->house_id==scope->house_id&&reviewed_room->native_estate_id==r.native_estate_id&&!binding);const auto& room=*reviewed_room;
  auto current=Q(db,"SELECT i.definition_id,i.zone_id,i.zone_version,i.instance_id,i.catalog_key,i.catalog_revision,h.pending_request_key FROM housing_house_interiors i JOIN housing_house_instances h USING(house_id) JOIN housing_house_locations l ON l.house_id=h.house_id AND l.location_kind=2 JOIN housing_live_item_custody root ON root.custody_id=l.custody_id WHERE i.house_id="+I(scope->house_id)+" AND i.native_estate_id="+I(r.native_estate_id)+" AND root.estate_id="+I(retiring_estate)+" FOR UPDATE");Require(current.RowCount()==1);auto h=current.begin();Require(N(h[0])==room.definition.id&&N(h[1])==room.route.zone_id&&N(h[2])==room.route.zone_version&&N(h[3])==room.route.instance_id&&h[4]&&std::string(h[4])==r.catalog_key&&N(h[5])==r.catalog_revision&&!h[6]);
  auto saved=Q(db,"SELECT origin,client_build,zone_short_name,HEX(client_asset_sha256),HEX(collision_map_sha256),HEX(manifest_sha256),HEX(definitions_blob),HEX(regions_blob) FROM housing_house_room_profiles WHERE house_id="+I(scope->house_id)+" FOR UPDATE");Require(saved.RowCount()==1);auto s=saved.begin();Require(N(s[0])==uint32_t(room.origin)&&N(s[1])==room.client_build&&s[2]&&std::string(s[2])==room.zone_short_name&&Lower(s[3])==room.client_asset_sha256&&Lower(s[4])==room.collision_map_sha256&&Lower(s[5])==room.manifest_sha256&&Unhex(s[6])==InteriorContent::DefinitionPayload(room)&&Unhex(s[7])==InteriorContent::RegionPayload(room));
  auto input=InteriorContent::ItemInputUnderTransaction(db,room,v.item_id);Require(bool(input)&&input.value.npc&&NpcRuntime::Same(*input.value.npc,*profile));auto fresh=InteriorContent::Build(input.value);Require(bool(fresh));
  auto tag=Q(db,"SELECT house_id,content_version,HEX(content_digest),source_zone,source_version,source_instance FROM housing_interior_placement_reservations WHERE native_item_id="+I(r.native_item_id)+" FOR UPDATE");Require(tag.RowCount()==1);auto t=tag.begin();Require(N(t[0])==scope->house_id&&N(t[1])==fresh.value.version&&Unhex(t[2])==std::vector<uint8_t>(fresh.value.digest.begin(),fresh.value.digest.end())&&N(t[3])==room.route.zone_id&&N(t[4])==room.route.zone_version&&N(t[5])==room.route.instance_id);
  Require(source.stowed||bool(InteriorContent::CheckPlacement(room,GeometryContext(r.native_estate_id,room.definition.type,room.region.id),item,geometry,placed.transform,PlacementPolicy::Operation::Add)));out.room=room;
 }else{
  Require(!reviewed_room&&source.estate_id==retiring_estate&&binding&&(binding->version==ContentBinding::NpcClassVersion||binding->version==ContentBinding::GenericNpcClassVersion));
  auto target=Q(db,"SELECT n.definition_id,c.revision,c.origin,c.client_build,HEX(c.definitions_blob),HEX(c.regions_blob),h.zone_id,h.zone_version,h.instance_id FROM housing_native_estates n JOIN housing_current_estates e ON e.id=n.ownership_estate_id AND e.plot_id=n.plot_id JOIN housing_catalogs c ON c.catalog_key=n.catalog_key JOIN housing_neighborhoods h ON h.id=n.neighborhood_id WHERE n.native_id="+I(r.native_estate_id)+" AND n.ownership_estate_id="+I(retiring_estate)+" AND n.active=1 AND n.catalog_key='"+db.Escape(r.catalog_key)+"' FOR UPDATE");Require(target.RowCount()==1);auto t=target.begin();Require(N(t[1])==r.catalog_revision&&(N(t[2])==1||N(t[2])==2)&&N(t[3])==20130510);
  if(N(t[2])==2){auto name=Q(db,"SELECT DATABASE()");Require(name.RowCount()==1&&name.begin()[0]);std::string value=name.begin()[0];Require(Housing::RuntimeActivation::AllowsReconstructedContent(db));}
  auto bundle=Catalog::DecodePayloads(Unhex(t[4]),Unhex(t[5]));Require(N(t[6])<=UINT32_MAX&&N(t[7])<=UINT8_MAX&&N(t[8])&&N(t[8])<=32000);Route route{uint32_t(N(t[6])),uint8_t(N(t[7])),uint16_t(N(t[8]))};auto fresh=ContentBinding::Current(db,r.catalog_key,bundle,r.native_estate_id,route,v.item_id);Require(ContentBinding::Same(*binding,fresh));
  auto definition=std::find_if(bundle.definitions.begin(),bundle.definitions.end(),[&](const auto& d){return d.id==N(t[0]);});Require(definition!=bundle.definitions.end()&&(definition->type==4||definition->type==8));auto region=std::find_if(bundle.regions.begin(),bundle.regions.end(),[&](const auto& a){return a.id==definition->area_id;});Require(region!=bundle.regions.end());Require(source.stowed||bool(PlacementPolicy::Check(GeometryContext(r.native_estate_id,definition->type,region->id),item,geometry,*region,placed.transform,PlacementPolicy::Operation::Add)));
 }
 return out;
}

// Call only for direct delivery, after the destination row/witness is secured
// and BEFORE removing source pose/custody. Retained house NPCs keep their frame.
// Revalidation prevents using a token captured before a changed source state.
inline void RetireFrameUnderTransaction(Database& db,const Validated& validated,uint32_t retiring_estate,uint32_t expected_owner){
 auto current=InspectUnderTransaction(db,validated.source,retiring_estate,expected_owner,validated.room?&*validated.room:nullptr);Detail::Require(NpcRuntime::Same(current.profile,validated.profile)&&current.house_id==validated.house_id);const auto& p=*current.source.placement;NpcNativeFrame::RetireUnderTransaction(db,current.source.custody_id,p.reservation.native_item_id,p.pose_revision,p.npc_frame);
}
}
#endif
