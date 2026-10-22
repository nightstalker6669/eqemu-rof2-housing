#ifndef EQEMU_HOUSING_LAYOUT_INVENTORY_OUTCOME_DB_H
#define EQEMU_HOUSING_LAYOUT_INVENTORY_OUTCOME_DB_H
#include "layout_inventory_outcome.h"
#include "catalog_content_db.h"
#include "item_storage_db.h"
#include "item_payload_db.h"
#include "crate_member_scope_db.h"
namespace Housing::LayoutInventory {
// Exact INVENTORY COMPONENT postimage check only. Caller must authenticate the
// immutable aggregate receipt and the held session or certified writer death.
// No fresh ACL evaluation, implicit lifecycle, ACK or recovery release here.
inline LiveItems::Result<bool> ValidateOutcomeUnderTransaction(Database& db,const LiveItems::Lease& lease,const Outcome& outcome){
 using Error=LiveItems::Error;
 try{OutcomeDetail::Shape(outcome);}catch(...){return {false,Error::InvalidArgument};}
 auto sources=ValidateSourcePostimagesUnderTransaction(db,lease,outcome.inventory);if(!sources)return sources;
 try{
  auto query=[&](const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::invalid_argument("Layout inventory postimage query failed");return r;};
  auto hex=[](const CustodyCodec::Bytes& b){const char* digits="0123456789ABCDEF";std::string text;text.reserve(b.size()*2);for(auto c:b){text+=digits[c>>4];text+=digits[c&15];}return text;};
  auto equal=[](const char* p,const std::string& v){return p&&std::string(p)==v;};
  // The actor's reservation is already held by the parent validator. Stable
  // native identities are sorted before locking all per-piece postimages.
  std::map<uint32_t,const OutcomePiece*> ordered;for(const auto& p:outcome.pieces)ordered.emplace(p.placement.stored.placement->reservation.native_item_id,&p);
  for(const auto& [id,pointer]:ordered){const auto& piece=*pointer;const auto& s=piece.placement.stored;const auto& placed=*s.placement;const auto& reservation=placed.reservation;const auto custody=std::to_string(s.custody_id);ItemPayload::ValidateHeld(db,s.row,s.custody_id);
   auto metadata=query("SELECT itemclass,bagslots,loregroup,nodrop,UNK226,UNK227 FROM items WHERE id="+std::to_string(s.row.values.item_id)+" FOR UPDATE");if(metadata.RowCount()!=1)return {false,Error::Conflict};auto m=metadata.begin();
   if(!equal(m[0],std::to_string(s.catalog.item_class))||!equal(m[1],std::to_string(s.catalog.bag_slots))||!equal(m[2],std::to_string(s.catalog.lore_group))||!equal(m[3],std::to_string(s.catalog.no_drop))||!equal(m[4],"0")||!equal(m[5],"0"))return {false,Error::Conflict};
   auto current_catalog=query("SELECT revision FROM housing_catalogs WHERE BINARY catalog_key=x'"+hex(CustodyCodec::Bytes(reservation.catalog_key.begin(),reservation.catalog_key.end()))+"' FOR UPDATE");if(current_catalog.RowCount()!=1||!equal(current_catalog.begin()[0],std::to_string(s.current_catalog_revision)))return {false,Error::Conflict};
   auto c=query("SELECT estate_id,owner_character_id,schema_descriptor,HEX(row_blob),HEX(catalog_blob) FROM housing_live_item_custody WHERE custody_id="+custody+" FOR UPDATE");if(c.RowCount()!=1)return {false,Error::Conflict};auto cr=c.begin();CustodyCodec::Bytes catalog;for(int32_t v:{s.catalog.item_class,s.catalog.bag_slots,s.catalog.lore_group,s.catalog.no_drop})CustodyCodec::W(catalog,uint32_t(v),4);
   if(!equal(cr[0],std::to_string(s.estate_id))||!equal(cr[1],std::to_string(lease.actor.character_id))||!equal(cr[2],LiveItems::Service::SchemaDescriptor())||!equal(cr[3],hex(CustodyCodec::Encode(s.row)))||!equal(cr[4],hex(catalog)))return {false,Error::Conflict};
   auto r=query("SELECT native_estate_id,estate_id,catalog_key,catalog_revision,character_id,account_id,lease_epoch,session_key,HEX(source_row),runtime_serial,state,custody_id,"+ContentBinding::SelectColumns(db)+" FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(id)+" FOR UPDATE");if(r.RowCount()!=1)return {false,Error::Conflict};auto rr=r.begin();
   if(!equal(rr[0],std::to_string(reservation.native_estate_id))||!equal(rr[1],std::to_string(s.estate_id))||!equal(rr[2],reservation.catalog_key)||!equal(rr[3],std::to_string(reservation.catalog_revision))||!equal(rr[4],std::to_string(lease.actor.character_id))||!equal(rr[5],std::to_string(lease.actor.account_id))||!equal(rr[6],std::to_string(lease.epoch))||!equal(rr[7],lease.session_key)||!equal(rr[8],hex(CustodyCodec::Encode(s.row)))||!equal(rr[9],std::to_string(piece.runtime_serial))||!equal(rr[10],"1")||!equal(rr[11],custody))return {false,Error::Conflict};
   auto binding=ContentBinding::DecodeIdentity(rr[12],rr[13]);if(bool(binding)!=bool(reservation.content_binding)||(binding&&!ContentBinding::Same(*binding,*reservation.content_binding)))return {false,Error::Conflict};
   auto p=query("SELECT native_item_id,pose_revision,HEX(pose_blob) FROM housing_live_placed_items WHERE custody_id="+custody+" FOR UPDATE");if(p.RowCount()!=1)return {false,Error::Conflict};auto pr=p.begin();CustodyCodec::Bytes pose;OutcomeDetail::Pose(pose,placed.transform);
   if(!equal(pr[0],std::to_string(id))||!equal(pr[1],std::to_string(placed.pose_revision))||!equal(pr[2],hex(pose)))return {false,Error::Conflict};
   auto scope=HouseItemScope::Locked(db,s.custody_id);if(bool(scope)!=bool(piece.placement.room))return {false,Error::Conflict};
   if(scope&&(scope->house_id!=piece.placement.room->house_id||scope->revision!=piece.room_scope_revision))return {false,Error::Conflict};
   auto storage=ItemStorage::Locked(db,s.custody_id);if(storage.revision!=s.storage_revision||storage.stowed!=s.stowed||(storage.revision&&storage.native_estate_id!=reservation.native_estate_id))return {false,Error::Conflict};
   // This codec version describes ordinary room/yard placements. Crate member
   // and unique house/crate-token outcomes need their separate typed records.
   if(CrateMemberScope::Locked(db,s.custody_id,true))return {false,Error::Conflict};
   if(piece.placement.room){const auto& room=*piece.placement.room;
    auto parent=query("SELECT h.revision,p.revision FROM housing_house_instances h JOIN housing_house_room_profiles p ON p.house_id=h.house_id WHERE h.house_id="+std::to_string(room.house_id)+" FOR UPDATE");if(parent.RowCount()!=1||!equal(parent.begin()[0],std::to_string(room.house_revision))||!equal(parent.begin()[1],std::to_string(room.profile_revision)))return {false,Error::Conflict};
    auto tags=query("SELECT house_id,house_revision,profile_revision,source_zone,source_version,source_instance,content_version,HEX(content_digest) FROM housing_interior_placement_reservations WHERE native_item_id="+std::to_string(id)+" FOR UPDATE");if(tags.RowCount()!=1)return {false,Error::Conflict};auto t=tags.begin();
    if(!equal(t[0],std::to_string(room.house_id))||!equal(t[1],std::to_string(room.house_revision))||!equal(t[2],std::to_string(room.profile_revision))||!equal(t[3],std::to_string(room.source.zone_id))||!equal(t[4],std::to_string(room.source.zone_version))||!equal(t[5],std::to_string(room.source.instance_id))||!equal(t[6],"3")||!equal(t[7],hex(CustodyCodec::Bytes(room.content_digest.begin(),room.content_digest.end()))))return {false,Error::Conflict};
   }else{
    auto table=query("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_interior_placement_reservations'");if(table.RowCount()!=1||!table.begin()[0])throw std::invalid_argument("Layout room tag schema");
    if(std::stoul(table.begin()[0])){auto tags=query("SELECT native_item_id FROM housing_interior_placement_reservations WHERE native_item_id="+std::to_string(id)+" FOR UPDATE");if(tags.RowCount())return {false,Error::Conflict};}
   }
  }return {true,Error::None};
 }catch(...){return {false,Error::DatabaseFailure};}
}
}
#endif
