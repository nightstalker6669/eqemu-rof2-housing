#ifndef EQEMU_HOUSING_TROPHY_BENEFITS_DB_H
#define EQEMU_HOUSING_TROPHY_BENEFITS_DB_H
#include "trophy_benefits.h"
#include "trophy_location.h"
#include "scene_service.h"
#include "interior_profile_db.h"
#include "trophy_billing.h"
#include <openssl/sha.h>
#include "live_item_service.h"
#include "custody_codec.h"
#include "static_furniture_registry.h"
#include "../database.h"
#include <cmath>
#include <cstring>
namespace Housing::TrophyBenefits {
enum class Action { Read,Select,Enable,Disable,Renew };
struct Request {Action action=Action::Read;TrophyAnchorWire::Selection selection;uint32_t level=0,capacity=4;std::optional<SelectionLocation> selection_location;};
namespace Detail {
struct Failure{LiveItems::Error error;};
inline MySQLRequestResult Q(Database& db,const std::string& sql){auto r=db.QueryDatabase(sql,false);if(!r.Success())throw Failure{LiveItems::Error::DatabaseFailure};return r;}
inline uint32_t Number(const char* p){if(!p)throw Failure{LiveItems::Error::DatabaseFailure};auto v=std::stoull(p);if(v>UINT32_MAX)throw Failure{LiveItems::Error::DatabaseFailure};return uint32_t(v);}
inline std::vector<uint8_t> Unhex(const char* p){if(!p)throw Failure{LiveItems::Error::DatabaseFailure};std::string s=p;if(s.size()%2)throw Failure{LiveItems::Error::DatabaseFailure};std::vector<uint8_t> out;for(size_t i=0;i<s.size();i+=2){size_t used=0;auto v=std::stoul(s.substr(i,2),&used,16);if(used!=2)throw Failure{LiveItems::Error::DatabaseFailure};out.push_back(uint8_t(v));}return out;}
}
// Must be called inside the inventory barrier and its exclusive DB transaction.
// Personal owner benefits, with room authority derived from the actual CURRENT
// placed host and existing reviewed Scene content/pose checks. Guild is separate.
inline LiveItems::Result<View> ExecuteUnderTransaction(Database& db,const LiveItems::Lease& lease,LiveItems::InventorySessionFence& fence,const Request& request,const FavorAuthority* favor_authority=nullptr,const Scene::HouseScenePolicy* houses=nullptr,const InteriorContent::ProfilePolicy* rooms=nullptr,const CrateBinding::CustodyContext* crates=nullptr){
 using namespace Detail;using Error=LiveItems::Error;
 try{
  if(!request.level||request.capacity>10)return {{},Error::InvalidArgument};
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||Number(tx.begin()[0])!=1)return {{},Error::InvalidArgument};
  auto authenticated=LiveItems::Service::AuthenticateUnderTransaction(db,true,lease,fence);if(!authenticated)return {{},authenticated.error};if(!authenticated.value.pending_key.empty())return {{},Error::PendingApplication};
  auto character=Q(db,"SELECT c.level,c.deleted_at,a.id FROM character_data c JOIN account a ON a.id=c.account_id WHERE c.id="+std::to_string(lease.actor.character_id)+" AND c.account_id="+std::to_string(lease.actor.account_id)+" FOR UPDATE");
  if(character.RowCount()!=1||character.begin()[1]||!character.begin()[2])return {{},Error::Unauthorized};
  // Runtime level may include an unsaved level-up; the authenticated actual
  // Client supplies it. Persisted account/deletion identity is independently held.
  auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_trophy_benefit_sources','housing_trophy_benefit_state','housing_trophy_benefit_selections','tributes','tribute_levels','items','housing_live_item_custody','housing_live_placed_items','housing_live_placement_reservations','housing_estates','housing_native_estates','housing_house_item_scopes','housing_live_item_storage','housing_crate_members','housing_trophy_billing_policy','housing_trophy_favor_charges')");
  if(engines.RowCount()!=1||Number(engines.begin()[0])!=16)return {{},Error::DatabaseFailure};
  Catalog catalog;std::map<uint32_t,uint32_t> declared_tiers;
  auto definitions=Q(db,"SELECT s.benefit_id,s.placed_item_id,s.source_sha256,s.source_description,t.name,t.isguild,l.level,l.cost,l.item_id,i.id,t.unknown FROM housing_trophy_benefit_sources s JOIN tributes t ON t.id=s.benefit_id JOIN tribute_levels l ON l.tribute_id=t.id LEFT JOIN items i ON i.id=l.item_id WHERE t.isguild=0 ORDER BY s.benefit_id,l.level FOR UPDATE");
  if(definitions.RowCount()>10000)return {{},Error::UnsupportedItem};
  for(auto row=definitions.begin();row.operator!=(definitions.end());++row){auto id=Number(row[0]);declared_tiers[id]=Number(row[10]);auto& d=catalog[id];if(d.tiers.empty()){d.id=id;d.placed_item_id=Number(row[1]);d.source_sha256=row[2]?row[2]:"";d.name=row[4]?row[4]:"";if(!row[3]||!row[3][0])return {{},Error::UnsupportedItem};}if(!row[9])return {{},Error::UnsupportedItem};d.tiers.push_back({Number(row[6]),Number(row[7]),Number(row[8])});}
  for(const auto& [id,d]:catalog)if(!Valid(d)||declared_tiers[id]!=d.tiers.size())return {{},Error::UnsupportedItem};
  std::set<uint32_t> eligible;bool selection_at_source=false;
  auto actual=Q(db,"SELECT s.benefit_id,c.owner_character_id,c.schema_descriptor,HEX(c.row_blob),r.character_id,r.native_estate_id,r.estate_id,r.state,p.pose_revision,n.native_id,c.estate_id,s.placed_item_id,i.idfile,r.account_id,HEX(r.source_row),HEX(p.pose_blob),c.custody_id FROM housing_trophy_benefit_sources s JOIN housing_live_item_custody c ON c.owner_character_id="+std::to_string(lease.actor.character_id)+" JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id AND r.custody_id=c.custody_id JOIN housing_estates e ON e.id=c.estate_id AND e.active=1 AND e.owner_kind=1 AND e.owner_id="+std::to_string(lease.actor.character_id)+" JOIN housing_native_estates n ON n.ownership_estate_id=e.id AND n.plot_id=e.plot_id AND n.active=1 AND n.catalog_key=r.catalog_key JOIN items i ON i.id=s.placed_item_id WHERE NOT EXISTS (SELECT 1 FROM housing_house_item_scopes hs WHERE hs.custody_id=c.custody_id) AND NOT EXISTS (SELECT 1 FROM housing_live_item_storage st WHERE st.custody_id=c.custody_id AND st.stowed=1) AND NOT EXISTS (SELECT 1 FROM housing_crate_members cm WHERE cm.custody_id=c.custody_id) ORDER BY c.custody_id,s.benefit_id LIMIT 10001 FOR UPDATE");
  if(actual.RowCount()>10000)return {{},Error::UnsupportedItem};
  for(auto row=actual.begin();row.operator!=(actual.end());++row){
   auto id=Number(row[0]);if(!catalog.count(id))continue;if(!row[2]||std::string(row[2])!=LiveItems::Service::SchemaDescriptor())return {{},Error::UnsupportedItem};auto row_bytes=Unhex(row[3]);auto custody=CustodyCodec::Decode(row_bytes);
   if(custody.values.item_id!=Number(row[11]))continue;
   if(custody.values.character_id!=lease.actor.character_id||Number(row[1])!=lease.actor.character_id||Number(row[4])!=lease.actor.character_id||Number(row[5])!=Number(row[9])||Number(row[6])!=Number(row[10])||Number(row[7])!=1||!Number(row[8]))return {{},Error::Conflict};
   if(Number(row[13])!=lease.actor.account_id||Unhex(row[14])!=row_bytes)return {{},Error::Conflict};
   auto pose=Unhex(row[15]);if(pose.size()!=28)return {{},Error::Conflict};for(size_t i=0;i<7;++i){float v;std::memcpy(&v,pose.data()+4*i,4);if(!std::isfinite(v)||(i==3&&v<=0))return {{},Error::Conflict};}
   // Existing accepted placement ledger is authority, with current exclusion
   // of stow/crates/rooms. A definition never bypasses the static asset registry.
   if(row[12]&&StaticFurniture::Lookup(custody.values.item_id,std::string(row[12]))){
    auto witnessed=Scene::Service::ReadTrophyYardSourceUnderTransaction(db,lease,Number(row[10]),std::stoull(row[16]),fence,houses,crates);
    if(!witnessed&&(witnessed.error==Error::DatabaseFailure||witnessed.error==Error::UnheldFence||witnessed.error==Error::StaleLease||witnessed.error==Error::PendingApplication))return {{},witnessed.error};
    if(!witnessed||witnessed.value.items.size()!=1||witnessed.value.items[0].stored.stowed||witnessed.value.items[0].contained_in_crate||!witnessed.value.items[0].stored.placement)continue;
    eligible.insert(id);
    if(request.action==Action::Select&&request.selection.benefit_id==id&&request.selection_location){
     auto context=Q(db,"SELECT n.definition_id,HEX(c.definitions_blob),HEX(c.regions_blob),c.client_build,h.zone_id,h.zone_version,h.instance_id,i.zone,i.version,i.never_expires FROM housing_native_estates n JOIN housing_catalogs c ON c.catalog_key=n.catalog_key JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN instance_list i ON i.id=h.instance_id WHERE n.native_id="+std::to_string(Number(row[9]))+" AND n.active=1 AND n.ownership_estate_id="+std::to_string(Number(row[10]))+" FOR UPDATE");
     if(context.RowCount()!=1)return {{},Error::Conflict};auto cx=context.begin();const auto& actual=request.selection_location->route;
     if(Number(cx[3])!=20130510||Number(cx[4])!=Number(cx[7])||Number(cx[5])!=Number(cx[8])||Number(cx[9])!=1)return {{},Error::Conflict};
     if(actual.zone_id==Number(cx[4])&&actual.zone_version==Number(cx[5])&&actual.instance_id==Number(cx[6])){
      auto bundle=Housing::Catalog::DecodePayloads(Unhex(cx[1]),Unhex(cx[2]));for(const auto& d:bundle.definitions)if(d.id==Number(cx[0])&&d.type==4)for(const auto& region:bundle.regions)if(region.id==d.area_id&&Within(region,request.selection_location->position))selection_at_source=true;
     }
    }
   }
  }
  // Room sources retain historical custody estate/creator. Their benefits belong
  // to the current personal hosting property owner, never the old custody owner.
  // Missing/withdrawn reviewed host/profile/placement simply removes eligibility.
  if(houses&&rooms){
   auto candidates=Q(db,"SELECT scope.house_id,c.custody_id,e.id,c.schema_descriptor,HEX(c.row_blob),s.benefit_id FROM housing_house_item_scopes scope JOIN housing_live_item_custody c ON c.custody_id=scope.custody_id JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_house_locations loc ON loc.house_id=scope.house_id AND loc.location_kind=2 JOIN housing_live_item_custody hc ON hc.custody_id=loc.custody_id JOIN housing_live_placed_items hp ON hp.custody_id=hc.custody_id JOIN housing_estates e ON e.id=hc.estate_id AND e.active=1 AND e.owner_kind=1 AND e.owner_id="+std::to_string(lease.actor.character_id)+" JOIN housing_trophy_benefit_sources s ORDER BY scope.house_id,c.custody_id,s.benefit_id LIMIT 10001 FOR UPDATE");
   if(candidates.RowCount()>10000)return {{},Error::UnsupportedItem};
   for(auto row=candidates.begin();row!=candidates.end();++row){
    const auto id=Number(row[5]);if(!catalog.count(id)||!row[3]||std::string(row[3])!=LiveItems::Service::SchemaDescriptor())continue;
    const auto decoded=CustodyCodec::Decode(Unhex(row[4]));if(decoded.values.item_id!=catalog.at(id).placed_item_id)continue;
    auto room=Scene::Service::ReadTrophySourceUnderTransaction(db,lease,Number(row[2]),std::stoull(row[0]),std::stoull(row[1]),fence,*houses,*rooms,crates);
    if(!room){if(room.error==Error::DatabaseFailure||room.error==Error::UnheldFence||room.error==Error::StaleLease||room.error==Error::PendingApplication)return {{},room.error};continue;}const auto& current=room.value;
    if(!current.house||!current.room_profile||current.contents.size()!=1||!current.crate_contents.empty())continue;
    const auto& content=current.contents[0];const auto& stored=content.stored;
    if(stored.custody_id!=std::stoull(row[1])||stored.row.values.item_id!=catalog.at(id).placed_item_id||stored.stowed||!stored.placement||!content.room||content.contained_in_crate)continue;
    eligible.insert(id);
    if(request.action==Action::Select&&request.selection.benefit_id==id&&request.selection_location){const auto& route=request.selection_location->route;const auto& source=current.context.source;
     if(route.zone_id==source.zone_id&&route.zone_version==source.zone_version&&route.instance_id==source.instance_id&&Within(current.room_profile->region,current.room_profile->additional_regions,request.selection_location->position))selection_at_source=true;
    }
   }
  }
  const auto owner=std::to_string(lease.actor.character_id);
  Q(db,"INSERT INTO housing_trophy_benefit_state(character_id) VALUES ("+owner+") ON DUPLICATE KEY UPDATE character_id=VALUES(character_id)");
  auto state=Q(db,"SELECT enabled,revision,remaining_ms,paid_until_ms,selection_hash FROM housing_trophy_benefit_state WHERE character_id="+owner+" FOR UPDATE");if(state.RowCount()!=1||Number(state.begin()[0])>1||!std::stoull(state.begin()[1]))return {{},Error::DatabaseFailure};
  View out;out.selected.enabled=Number(state.begin()[0]);PaidState paid{bool(out.selected.enabled),Number(state.begin()[2]),std::stoull(state.begin()[3]),state.begin()[4]?state.begin()[4]:""};const auto prior_paid=paid;
  auto selections=Q(db,"SELECT slot,benefit_id,tier FROM housing_trophy_benefit_selections WHERE character_id="+owner+" ORDER BY slot FOR UPDATE");if(selections.RowCount()>10)return {{},Error::DatabaseFailure};
  for(auto row=selections.begin();row.operator!=(selections.end());++row){auto slot=Number(row[0]);if(slot>=10)return {{},Error::DatabaseFailure};out.selected.ids[slot]=Number(row[1]);out.selected.tiers[slot]=int32_t(Number(row[2]));}
  const auto previous=out.selected;
  Revalidate(out,catalog,eligible,request.level,request.capacity,true);
  if(request.action==Action::Select&&request.selection.benefit_id!=UINT32_MAX&&!selection_at_source)return {{},Error::Unauthorized};
  if(request.action==Action::Select&&!Select(out,request.selection,catalog,eligible,request.level,request.capacity))return {{},Error::Unauthorized};
  const bool selection_changed=previous.ids!=out.selected.ids||previous.tiers!=out.selected.tiers;
  uint64_t cost_sum=0;for(size_t i=0;i<10;++i)if(out.selected.ids[i]!=UINT32_MAX)cost_sum+=catalog.at(out.selected.ids[i]).tiers[out.selected.tiers[i]].cost;
  if(cost_sum>UINT32_MAX)return {{},Error::UnsupportedItem};
  if(selection_changed||request.action==Action::Select){paid={};out.selected.enabled=0;}
  if(!cost_sum){
   paid={};
   if(request.action==Action::Enable&&!Toggle(out,true,catalog,eligible,request.level,request.capacity))return {{},Error::UnsupportedItem};
   if(request.action==Action::Disable)Toggle(out,false,catalog,eligible,request.level,request.capacity);
  }else if(request.action!=Action::Select&&!selection_changed){
   auto policies=Q(db,"SELECT period_ms,policy,source_sha256,source_description FROM housing_trophy_billing_policy WHERE scope='personal' FOR UPDATE");
   if(policies.RowCount()!=1||!policies.begin()[3]||!policies.begin()[3][0]){paid={};out.selected.enabled=0;out.billing_unavailable=true;}else{
   BillingPolicy policy{Number(policies.begin()[0]),policies.begin()[2]?policies.begin()[2]:"",Number(policies.begin()[1])};if(!Valid(policy)){paid={};out.selected.enabled=0;out.billing_unavailable=true;}else{
   std::vector<uint8_t> fingerprint;for(size_t i=0;i<10;++i){TrophyAnchorWire::Detail::Write(fingerprint,out.selected.ids[i]);TrophyAnchorWire::Detail::Write(fingerprint,uint32_t(out.selected.tiers[i]));if(out.selected.ids[i]!=UINT32_MAX){const auto& d=catalog.at(out.selected.ids[i]);const auto& tier=d.tiers[out.selected.tiers[i]];TrophyAnchorWire::Detail::Write(fingerprint,d.placed_item_id);TrophyAnchorWire::Detail::Write(fingerprint,tier.stat_item_id);TrophyAnchorWire::Detail::Write(fingerprint,tier.cost);fingerprint.insert(fingerprint.end(),d.source_sha256.begin(),d.source_sha256.end());}}
   TrophyAnchorWire::Detail::Write(fingerprint,policy.period_ms);TrophyAnchorWire::Detail::Write(fingerprint,policy.policy);fingerprint.insert(fingerprint.end(),policy.source_sha256.begin(),policy.source_sha256.end());
   std::array<unsigned char,32> digest{};if(!SHA256(fingerprint.data(),fingerprint.size(),digest.data()))return {{},Error::DatabaseFailure};const char* hex="0123456789abcdef";std::string selection;for(auto byte:digest){selection+=hex[byte>>4];selection+=hex[byte&15];}
   auto clock=Q(db,"SELECT CAST(UNIX_TIMESTAMP(CURRENT_TIMESTAMP(3))*1000 AS UNSIGNED)");if(clock.RowCount()!=1)return {{},Error::DatabaseFailure};const auto now=std::stoull(clock.begin()[0]);
   auto action=request.action==Action::Enable?BillAction::Enable:request.action==Action::Disable?BillAction::Disable:request.action==Action::Renew?BillAction::Renew:BillAction::Read;
   if(!out.selected.enabled&&paid.enabled)paid={};
   auto balance=favor_authority?favor_authority->CurrentFavorUnderFence(lease):std::nullopt;
   auto plan=Bill(policy,paid,action,now,uint32_t(cost_sum),balance,selection);if(!plan)return {{},Error::UnsupportedItem};
   paid=plan->state;out.remaining_ms=plan->remaining_ms;out.charged=plan->charge;out.favor_after=plan->favor_after;out.insufficient_favor=plan->insufficient;out.selected.enabled=paid.enabled;
   if(out.favor_after){if(!balance||!favor_authority||favor_authority->CurrentFavorUnderFence(lease)!=balance)return {{},Error::UnheldFence};
    auto changed=Q(db,"UPDATE character_data SET tribute_points="+std::to_string(*out.favor_after)+" WHERE id="+owner+" AND account_id="+std::to_string(lease.actor.account_id));if(changed.RowsAffected()!=1)return {{},Error::Conflict};
    Q(db,"INSERT INTO housing_trophy_favor_charges(character_id,state_revision,lease_epoch,session_key,favor_before,favor_after,cost,period_ms,paid_until_ms,selection_hash) VALUES ("+owner+","+std::to_string(std::stoull(state.begin()[1])+1)+","+std::to_string(lease.epoch)+",'"+db.Escape(lease.session_key)+"',"+std::to_string(*balance)+","+std::to_string(*out.favor_after)+","+std::to_string(out.charged)+","+std::to_string(policy.period_ms)+","+std::to_string(paid.until_ms)+",'"+selection+"')");
   }
   }}
  }
  Revalidate(out,catalog,eligible,request.level,request.capacity,true);
  // Refresh pruning is durable, so disconnects cannot resurrect withdrawn items.
  if(previous.enabled==out.selected.enabled&&previous.ids==out.selected.ids&&previous.tiers==out.selected.tiers&&prior_paid.enabled==paid.enabled&&prior_paid.remaining_ms==paid.remaining_ms&&prior_paid.until_ms==paid.until_ms&&prior_paid.selection_hash==paid.selection_hash)return {std::move(out),Error::None};
  Q(db,"DELETE FROM housing_trophy_benefit_selections WHERE character_id="+owner);
  for(size_t i=0;i<10;++i)if(out.selected.ids[i]!=UINT32_MAX)Q(db,"INSERT INTO housing_trophy_benefit_selections(character_id,slot,benefit_id,tier) VALUES ("+owner+","+std::to_string(i)+","+std::to_string(out.selected.ids[i])+","+std::to_string(out.selected.tiers[i])+")");
  Q(db,"UPDATE housing_trophy_benefit_state SET enabled="+std::to_string(out.selected.enabled)+",remaining_ms="+std::to_string(paid.remaining_ms)+",paid_until_ms="+std::to_string(paid.until_ms)+",selection_hash='"+db.Escape(paid.selection_hash)+"',revision=revision+1 WHERE character_id="+owner);
  return {std::move(out),Error::None};
 }catch(const Failure& f){return {{},f.error};}catch(...){return {{},Error::DatabaseFailure};}
}
}
#endif
