#ifndef EQEMU_HOUSING_UPKEEP_CYCLE_DB_H
#define EQEMU_HOUSING_UPKEEP_CYCLE_DB_H
#include "upkeep_selection_db.h"
#include "rent_state_db.h"
#include "house_geometry_registry.h"
#include "custody_codec.h"
#include "catalog_content_db.h"
#include "item_storage_db.h"
#include "crate_member_scope_db.h"
#include <limits>
#include "upkeep_clock.h"
#include "upkeep_catchup_db.h"
namespace Housing::UpkeepCycle {
enum class Status { Disabled, NotDue, Applied, Replayed, Unsupported, Pending, Conflict, DatabaseFailure, CommitUncertain };
struct Outcome {Status status=Status::Disabled;uint64_t charged=0;uint32_t stowed=0;};
namespace Detail {
using UpkeepSelection::Detail::Q;using UpkeepSelection::Detail::N;using UpkeepSelection::Detail::Quote;using UpkeepSelection::Detail::Unhex;
struct Stop {Status status;};
inline bool Installed(Database& db){
 auto r=Q(db,"SELECT table_name,engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_upkeep_contracts','housing_upkeep_cycles','housing_upkeep_cycle_items','housing_upkeep_item_state')");
 if(!r.RowCount())return false;if(r.RowCount()!=4)throw Stop{Status::DatabaseFailure};
 for(auto p=r.begin();p!=r.end();++p)if(!p[1]||std::string(p[1])!="InnoDB")throw Stop{Status::DatabaseFailure};
 auto core=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_estates','housing_native_estates','housing_catalogs','housing_neighborhoods','housing_live_item_custody','housing_live_placed_items','housing_live_placement_reservations','housing_escrow_balances','housing_live_item_leases','items','instance_list','housing_house_item_scopes','housing_house_locations','housing_house_instances','housing_house_interiors','housing_house_room_profiles','housing_interior_placement_reservations','housing_live_item_storage','housing_crate_members')");if(core.RowCount()!=1||N(core.begin()[0])!=19)throw Stop{Status::DatabaseFailure};
 auto full=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_upkeep_property_state','housing_upkeep_property_cycles','housing_upkeep_cycle_scopes','housing_upkeep_reactivation_policy')");if(full.RowCount()!=1||N(full.begin()[0])!=4)throw Stop{Status::DatabaseFailure};return true;
}
inline std::string Hex(const std::vector<uint8_t>& b){const char* h="0123456789ABCDEF";std::string s;for(auto c:b){s+=h[c>>4];s+=h[c&15];}return s;}
inline uint64_t Hourly(const EstateWire::CostComponent& component,uint32_t due=0,bool rounding_reviewed=false){
 uint64_t daily=0;for(const auto& v:component.values){if(v.type_id!=0||v.subtype_id!=0||v.amount<0||uint64_t(v.amount)>uint64_t(INT64_MAX)-daily)throw Stop{Status::Unsupported};daily+=uint64_t(v.amount);}
 auto rate=UpkeepClock::Hourly(daily,due,rounding_reviewed);if(!rate)throw Stop{Status::Unsupported};return *rate;
}
inline uint64_t ItemRate(const Catalog::Bundle& bundle,uint32_t id,uint32_t due=0,bool rounding_reviewed=false){
 if(!id)return 0;const EstateWire::CostComponent* found=nullptr;
 for(const auto& s:bundle.costs)for(const auto& c:s.components)if(c.id==id){if(found)throw Stop{Status::Unsupported};found=&c;}
 if(!found)throw Stop{Status::Unsupported};return Hourly(*found,due,rounding_reviewed);
}
inline bool RoundingReviewed(Database& db,uint32_t estate,uint32_t native,uint64_t revision){
 auto table=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_upkeep_rounding_policy'");
 if(!table.RowCount())return false;if(table.RowCount()!=1||!table.begin()[0]||std::string(table.begin()[0])!="InnoDB")throw Stop{Status::DatabaseFailure};
 auto rows=Q(db,"SELECT native_plot_id,catalog_revision,mode,review_reference FROM housing_upkeep_rounding_policy WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");
 if(!rows.RowCount())return false;if(rows.RowCount()!=1||N(rows.begin()[0])!=native||N(rows.begin()[1])!=revision||N(rows.begin()[2])!=1||!rows.begin()[3]||!rows.begin()[3][0])throw Stop{Status::Conflict};return true;
}
struct Graph {uint64_t revision=0,plot_rate=0;uint32_t native=0,plot_component=0,billing_epoch=0;bool rounding_reviewed=false;Catalog::Bundle bundle;Route route;};
inline Graph Current(Database& db,uint32_t estate,uint32_t native,uint64_t expected_revision,uint32_t billing_epoch=0){
 auto r=Q(db,"SELECT n.definition_id,n.catalog_key,c.revision,HEX(c.definitions_blob),HEX(c.regions_blob),e.owner_kind,e.owner_id,h.zone_id,h.zone_version,h.instance_id FROM housing_current_estates e JOIN housing_native_estates n ON n.ownership_estate_id=e.id AND n.plot_id=e.plot_id AND n.active=1 JOIN housing_catalogs c ON c.catalog_key=n.catalog_key JOIN housing_neighborhoods h ON h.id=n.neighborhood_id JOIN instance_list route ON route.id=h.instance_id AND route.zone=h.zone_id AND route.version=h.zone_version AND route.never_expires=1 WHERE h.instance_id>0 AND h.instance_id<=32000 AND c.client_build=20130510 AND c.origin IN(1,2) AND e.id="+std::to_string(estate)+" AND n.native_id="+std::to_string(native)+" FOR UPDATE");
 if(r.RowCount()!=1)throw Stop{Status::Conflict};auto p=r.begin();Graph g;g.native=native;g.revision=N(p[2]);if(g.revision!=expected_revision)throw Stop{Status::Conflict};g.billing_epoch=billing_epoch;g.rounding_reviewed=RoundingReviewed(db,estate,native,g.revision);
 g.bundle=Catalog::DecodePayloads(Unhex(p[3]),Unhex(p[4]));g.bundle.key=p[1];g.route={uint32_t(N(p[7])),uint8_t(N(p[8])),uint16_t(N(p[9]))};
 auto definition=std::find_if(g.bundle.definitions.begin(),g.bundle.definitions.end(),[&](const auto& d){return d.id==N(p[0]);});if(definition==g.bundle.definitions.end()||!((N(p[5])==1&&definition->type==4)||(N(p[5])==3&&definition->type==8)))throw Stop{Status::Conflict};
 if(definition->upkeep_scheme){
  auto selected=UpkeepSelection::PublishedComponent(db,estate,native,uint32_t(N(p[5])),uint32_t(N(p[6])),*definition,g.bundle.costs);
  const EstateWire::CostScheme* scheme=nullptr;for(const auto& candidate:g.bundle.costs)if(candidate.id==definition->upkeep_scheme)scheme=&candidate;if(!scheme)throw Stop{Status::Unsupported};
  if(selected){auto component=std::find_if(scheme->components.begin(),scheme->components.end(),[&](const auto& c){return c.id==selected;});if(component==scheme->components.end())throw Stop{Status::Conflict};g.plot_component=selected;g.plot_rate=Hourly(*component,g.billing_epoch,g.rounding_reviewed);}
  else for(const auto& component:scheme->components)if(Hourly(component,g.billing_epoch,g.rounding_reviewed))throw Stop{Status::Unsupported};
 }

 return g;
}
enum class Kind:uint8_t {Ordinary=1,House=2,Room=3};
struct Scope {Kind kind=Kind::Ordinary;uint32_t native=0;uint64_t house=0,profile=0;std::vector<uint8_t> proof;};
struct Item {uint64_t custody=0,storage=0,rate=0;uint32_t native=0,creator=0,component=0;std::vector<uint8_t> raw;Scope scope;uint32_t expired_before=0;};
inline void PendingCreator(Database& db,uint32_t creator){auto lease=Q(db,"SELECT pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(creator)+" FOR UPDATE");if(lease.RowCount()>1||(lease.RowCount()&&lease.begin()[0])||CoinPending::Inspect(db,creator,true)!=CoinPending::State::Clear)throw Stop{Status::Pending};}
// Do not change property eligibility or its escrow while any involved actual
// inventory/coin application is unresolved, including free/stored furnishings.
inline void PendingEstate(Database& db,uint32_t estate,uint32_t owner_kind,uint32_t owner){
 if(owner_kind==1)PendingCreator(db,owner);
 auto pending=Q(db,"SELECT l.character_id FROM housing_live_item_leases l JOIN housing_live_item_custody c ON c.owner_character_id=l.character_id WHERE l.pending_key IS NOT NULL AND (c.estate_id="+std::to_string(estate)+" OR EXISTS(SELECT 1 FROM housing_house_item_scopes hs JOIN housing_house_locations hl ON hl.house_id=hs.house_id AND hl.location_kind=2 JOIN housing_live_item_custody host ON host.custody_id=hl.custody_id WHERE hs.custody_id=c.custody_id AND host.estate_id="+std::to_string(estate)+")) LIMIT 1 FOR UPDATE");if(pending.RowCount())throw Stop{Status::Pending};
 auto tables=Q(db,"SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_live_escrow_pending','housing_live_escrow_receipts')");if(!tables.RowCount())return;if(tables.RowCount()!=2)throw Stop{Status::DatabaseFailure};for(auto p=tables.begin();p!=tables.end();++p)if(!p[0]||std::string(p[0])!="InnoDB")throw Stop{Status::DatabaseFailure};
 auto coin=Q(db,"SELECT p.character_id FROM housing_live_escrow_pending p JOIN housing_live_escrow_receipts r ON r.request_key=p.request_key WHERE r.estate_id="+std::to_string(estate)+" LIMIT 1 FOR UPDATE");if(coin.RowCount())throw Stop{Status::Pending};
}
// This consumes an already committed typed placement/house journal solely as a
// billing witness. It never creates a room review, ACL, placement or entry grant.
struct Host {uint64_t house=0,revision=0,custody=0;uint32_t native=0,zone=0,version=0,instance=0;std::vector<uint8_t> proof;};
inline Host HouseWitness(Database& db,uint32_t estate,const Graph& g,uint64_t house){
 auto rows=Q(db,"SELECT h.house_id,h.revision,h.item_template_id,h.created_by_character_id,h.pending_request_key,l.custody_id,i.native_estate_id,i.definition_id,i.zone_id,i.zone_version,i.instance_id,i.catalog_key,i.catalog_revision,n.parent_native_id,n.active,n.definition_id,n.catalog_key,inst.zone,inst.version,inst.never_expires,r.native_item_id,HEX(r.source_row),r.character_id,r.content_binding_version,HEX(r.content_binding_digest),c.owner_character_id,HEX(c.row_blob),m.idfile FROM housing_house_instances h JOIN housing_house_locations l ON l.house_id=h.house_id AND l.location_kind=2 JOIN housing_house_interiors i ON i.house_id=h.house_id JOIN housing_native_estates n ON n.native_id=i.native_estate_id JOIN instance_list inst ON inst.id=i.instance_id JOIN housing_live_item_custody c ON c.custody_id=l.custody_id JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id AND r.custody_id=c.custody_id AND r.state=1 JOIN items m ON m.id=h.item_template_id WHERE h.house_id="+std::to_string(house)+" AND c.estate_id="+std::to_string(estate)+" AND r.estate_id="+std::to_string(estate)+" AND r.catalog_key="+Quote(db,g.bundle.key)+" AND r.native_estate_id="+std::to_string(g.native)+" FOR UPDATE");
 if(rows.RowCount()!=1)throw Stop{Status::Conflict};auto p=rows.begin();for(size_t i=0;i<28;++i)if(i!=4&&!p[i])throw Stop{Status::Conflict};
 if(p[4])throw Stop{Status::Pending};auto raw=Unhex(p[26]);auto source=CustodyCodec::Decode(raw);
 if(!N(p[0])||!N(p[1])||!N(p[5])||N(p[3])!=N(p[22])||N(p[3])!=N(p[25])||source.values.character_id!=N(p[3])||source.values.item_id!=N(p[2])||raw!=Unhex(p[21])||!HouseGeometry::Lookup(source.values.item_id,p[27])||N(p[13])!=g.native||N(p[14])!=1||N(p[7])!=N(p[15])||std::string(p[11])!=g.bundle.key||std::string(p[16])!=g.bundle.key||N(p[12])!=g.revision||N(p[8])!=N(p[17])||N(p[9])!=N(p[18])||N(p[19])!=1||N(p[10])==0||N(p[10])>32000||N(p[23])!=ContentBinding::HouseVersion||Unhex(p[24]).size()!=32)throw Stop{Status::Conflict};
 auto definition=std::find_if(g.bundle.definitions.begin(),g.bundle.definitions.end(),[&](const auto& d){return d.id==N(p[7]);});if(definition==g.bundle.definitions.end()||(definition->type!=2&&definition->type!=3))throw Stop{Status::Unsupported};
 if(ItemStorage::Locked(db,N(p[5])).stowed||CrateMemberScope::Locked(db,N(p[5]),true))throw Stop{Status::Conflict};PendingCreator(db,uint32_t(N(p[3])));
 Host host{N(p[0]),N(p[1]),N(p[5]),uint32_t(N(p[6])),uint32_t(N(p[8])),uint32_t(N(p[9])),uint32_t(N(p[10]))};
 CustodyCodec::W(host.proof,host.house,8);CustodyCodec::W(host.proof,host.revision,8);CustodyCodec::W(host.proof,host.custody,8);auto digest=Unhex(p[24]);host.proof.insert(host.proof.end(),digest.begin(),digest.end());host.proof.insert(host.proof.end(),raw.begin(),raw.end());return host;
}
inline Scope RoomWitness(Database& db,uint32_t estate,const Graph& g,uint64_t house,uint32_t native_item,uint32_t native){
 auto host=HouseWitness(db,estate,g,house);if(host.native!=native)throw Stop{Status::Conflict};
 auto rows=Q(db,"SELECT t.house_revision,t.profile_revision,t.source_zone,t.source_version,t.source_instance,t.content_version,HEX(t.content_digest),p.revision,p.origin,p.client_build,p.zone_short_name,HEX(p.client_asset_sha256),HEX(p.collision_map_sha256),HEX(p.manifest_sha256),HEX(p.definitions_blob),HEX(p.regions_blob) FROM housing_interior_placement_reservations t JOIN housing_house_room_profiles p ON p.house_id=t.house_id WHERE t.house_id="+std::to_string(house)+" AND t.native_item_id="+std::to_string(native_item)+" FOR UPDATE");
 if(rows.RowCount()!=1)throw Stop{Status::Conflict};auto p=rows.begin();for(size_t i=0;i<16;++i)if(!p[i])throw Stop{Status::Conflict};
 if(!N(p[0])||N(p[0])>host.revision||!N(p[1])||N(p[1])!=N(p[7])||N(p[2])!=host.zone||N(p[3])!=host.version||N(p[4])!=host.instance||N(p[5])!=3||Unhex(p[6]).size()!=32||(N(p[8])!=1&&N(p[8])!=2)||N(p[9])!=20130510||Unhex(p[11]).size()!=32||Unhex(p[12]).size()!=32||Unhex(p[13]).size()!=32)throw Stop{Status::Conflict};
 auto definitions=Unhex(p[14]),regions=Unhex(p[15]);auto profile=Catalog::DecodePayloads(definitions,regions);if(profile.definitions.empty()||profile.regions.empty()||!profile.costs.empty())throw Stop{Status::Conflict};
 Scope out{Kind::Room,native,house,N(p[7]),std::move(host.proof)};for(size_t i:{size_t(6),size_t(11),size_t(12),size_t(13)}){auto value=Unhex(p[i]);out.proof.insert(out.proof.end(),value.begin(),value.end());}out.proof.insert(out.proof.end(),definitions.begin(),definitions.end());out.proof.insert(out.proof.end(),regions.begin(),regions.end());return out;
}
inline std::vector<Item> Obligations(Database& db,uint32_t estate,const Graph& g,std::optional<uint64_t> restoring={}){
 auto rows=Q(db,"SELECT c.custody_id,c.owner_character_id,HEX(c.row_blob),p.native_item_id,r.character_id,r.native_estate_id,r.catalog_revision,HEX(r.source_row),r.content_binding_version,HEX(r.content_binding_digest) FROM housing_live_item_custody c JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_placement_reservations r ON r.native_item_id=p.native_item_id AND r.custody_id=c.custody_id AND r.state=1 WHERE (c.estate_id="+std::to_string(estate)+" AND NOT EXISTS(SELECT 1 FROM housing_house_item_scopes hs WHERE hs.custody_id=c.custody_id)) OR EXISTS(SELECT 1 FROM housing_house_item_scopes hs JOIN housing_house_locations hl ON hl.house_id=hs.house_id AND hl.location_kind=2 JOIN housing_live_item_custody host ON host.custody_id=hl.custody_id JOIN housing_live_placed_items hp ON hp.custody_id=host.custody_id WHERE hs.custody_id=c.custody_id AND host.estate_id="+std::to_string(estate)+" AND NOT EXISTS(SELECT 1 FROM housing_live_item_storage st WHERE st.custody_id=host.custody_id AND st.stowed=1) AND NOT EXISTS(SELECT 1 FROM housing_crate_members cm WHERE cm.custody_id=host.custody_id)) ORDER BY c.custody_id LIMIT 1001 FOR UPDATE");
 if(rows.RowCount()>1000)throw Stop{Status::Unsupported};std::vector<Item> out;
 for(auto p=rows.begin();p!=rows.end();++p){auto custody=N(p[0]);auto storage=ItemStorage::Locked(db,custody);
  if(storage.stowed&&(!restoring||*restoring!=custody))continue;if(CrateMemberScope::Locked(db,custody,true))continue;
  auto raw=Unhex(p[2]);auto row=CustodyCodec::Decode(raw);if(row.values.character_id!=N(p[1])||N(p[4])!=N(p[1])||raw!=Unhex(p[7]))throw Stop{Status::Conflict};
  auto metadata=Q(db,"SELECT UNK230,idfile FROM items WHERE id="+std::to_string(row.values.item_id)+" FOR UPDATE");if(metadata.RowCount()!=1||!metadata.begin()[1])throw Stop{Status::Unsupported};auto component=N(metadata.begin()[0]);if(component>UINT32_MAX)throw Stop{Status::Unsupported};auto rate=ItemRate(g.bundle,uint32_t(component),g.billing_epoch,g.rounding_reviewed);if(!rate)continue;
  auto room=HouseItemScope::Locked(db,custody);Scope scope;scope.native=uint32_t(N(p[5]));
  if(room){if(!StaticFurniture::Lookup(row.values.item_id,metadata.begin()[1])||p[8]||p[9])throw Stop{Status::Unsupported};scope=RoomWitness(db,estate,g,room->house_id,uint32_t(N(p[3])),scope.native);}
  else if(HouseGeometry::Lookup(row.values.item_id,metadata.begin()[1])){
   auto location=Q(db,"SELECT house_id FROM housing_house_locations WHERE location_kind=2 AND custody_id="+std::to_string(custody)+" FOR UPDATE");if(location.RowCount()!=1)throw Stop{Status::Conflict};auto host=HouseWitness(db,estate,g,N(location.begin()[0]));if(host.custody!=custody||scope.native!=g.native||N(p[8])!=ContentBinding::HouseVersion)throw Stop{Status::Conflict};scope={Kind::House,g.native,host.house,0,std::move(host.proof)};
  }else{
   if(scope.native!=g.native||N(p[6])!=g.revision||!StaticFurniture::Lookup(row.values.item_id))throw Stop{Status::Unsupported};auto identity=ContentBinding::DecodeIdentity(p[8],p[9]);if(!identity)throw Stop{Status::Unsupported};auto current=ContentBinding::Current(db,g.bundle.key,g.bundle,g.native,g.route,row.values.item_id);if(current.version!=identity->version||current.digest!=identity->digest)throw Stop{Status::Conflict};scope.proof.assign(identity->digest.begin(),identity->digest.end());
  }
  PendingCreator(db,row.values.character_id);if(storage.revision==UINT64_MAX)throw Stop{Status::Conflict};out.push_back({custody,storage.revision,rate,uint32_t(N(p[3])),uint32_t(N(p[1])),uint32_t(component),std::move(raw),std::move(scope),RentState::ItemExpired(db,custody)});
 }
 return out;
}
inline uint32_t PlotExpired(Database& db,uint32_t estate,uint32_t native){auto r=Q(db,"SELECT native_plot_id,expired_since FROM housing_upkeep_property_state WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(!r.RowCount())return 0;if(r.RowCount()!=1||N(r.begin()[0])!=native||N(r.begin()[1])>UINT32_MAX)throw Stop{Status::Conflict};return uint32_t(N(r.begin()[1]));}
inline bool RecoveryReviewed(Database& db,uint32_t estate,const Graph& g){auto r=Q(db,"SELECT native_plot_id,catalog_revision,mode,review_reference FROM housing_upkeep_reactivation_policy WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(!r.RowCount())return false;if(r.RowCount()!=1||N(r.begin()[0])!=g.native||N(r.begin()[1])!=g.revision||N(r.begin()[2])!=1||!r.begin()[3]||std::string(r.begin()[3]).empty())throw Stop{Status::Conflict};return true;}
inline uint64_t Total(const std::vector<Item>& items){uint64_t total=0;for(const auto& i:items){if(i.rate>uint64_t(INT64_MAX)-total)throw Stop{Status::Unsupported};total+=i.rate;}return total;}
// Financial obligation identity is independent of UTC rounding phase and pose
// changes that leave billing membership unchanged. Append-only placement and
// escrow history guards also catch a change-and-return-to-the-same-state gap.
inline UpkeepCatchup::Digest CatchupWitness(Database& db,uint32_t estate,const Graph& g,const std::vector<Item>& items){
 std::vector<uint8_t> proof;using UpkeepCatchup::Word;using UpkeepCatchup::Blob;
 for(uint64_t n:{uint64_t(estate),uint64_t(g.native),g.revision,uint64_t(g.plot_component),uint64_t(PlotExpired(db,estate,g.native))})Word(proof,n);
 auto selected=Q(db,"SELECT component_id,revision FROM housing_upkeep_selection WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(selected.RowCount()>1||(g.plot_component&&(selected.RowCount()!=1||N(selected.begin()[0])!=g.plot_component||!N(selected.begin()[1]))))throw Stop{Status::Conflict};Word(proof,selected.RowCount()?N(selected.begin()[1]):0);
 auto coins=Q(db,"SELECT COUNT(*) FROM housing_live_escrow_receipts WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(coins.RowCount()!=1)throw Stop{Status::DatabaseFailure};Word(proof,N(coins.begin()[0]));
 auto history=Q(db,"SELECT COUNT(*),COALESCE(MAX(native_item_id),0),COALESCE(SUM(state),0) FROM housing_live_placement_reservations WHERE estate_id="+std::to_string(estate)+" OR native_estate_id IN (SELECT i.native_estate_id FROM housing_house_interiors i JOIN housing_house_locations l ON l.house_id=i.house_id AND l.location_kind=2 JOIN housing_live_item_custody host ON host.custody_id=l.custody_id WHERE host.estate_id="+std::to_string(estate)+") FOR UPDATE");if(history.RowCount()!=1)throw Stop{Status::DatabaseFailure};for(unsigned i=0;i<3;++i)Word(proof,N(history.begin()[i]));
 Word(proof,items.size());for(const auto& i:items){for(uint64_t n:{i.custody,i.storage,uint64_t(i.native),uint64_t(i.creator),uint64_t(i.component),uint64_t(i.expired_before),uint64_t(i.scope.kind),uint64_t(i.scope.native),i.scope.house,i.scope.profile})Word(proof,n);Blob(proof,i.raw);Blob(proof,i.scope.proof);}
 return UpkeepCatchup::Hash(proof);
}
}
// Native expiry is a delinquent-since marker; it is never a paid-until date.
inline uint32_t Expired(Database& db,uint64_t custody,uint32_t native_item){using namespace Detail;auto r=db.QueryDatabase("SELECT expired_since FROM housing_upkeep_item_state WHERE custody_id="+std::to_string(custody)+" AND native_item_id="+std::to_string(native_item),false);if(!r.Success()){if(r.ErrorNumber()==ER_NO_SUCH_TABLE)return 0;throw Stop{Status::DatabaseFailure};}if(r.RowCount()>1||(r.RowCount()&&N(r.begin()[0])>UINT32_MAX))throw Stop{Status::DatabaseFailure};return r.RowCount()?uint32_t(N(r.begin()[0])):0;}
// Caller owns the real inventory transaction. No charge or placement here; the
// existing storage transition commits this marker clear with the actual pose.
// Require enough for ALL currently placed obligations plus this item, avoiding
// an invented partial-funding priority. Missing/delinquent contract fails closed.
inline bool RestoreAllowedUnderTransaction(Database& db,uint32_t estate,uint64_t custody){
 using namespace Detail;if(!Installed(db))return true;estate=RentState::BillingEstate(db,custody,estate);if(!RentState::PlotActive(db,estate))return false;auto mark=Q(db,"SELECT expired_since FROM housing_upkeep_item_state WHERE custody_id="+std::to_string(custody)+" FOR UPDATE");if(!mark.RowCount()||!N(mark.begin()[0]))return true;
 try{auto c=Q(db,"SELECT native_plot_id,catalog_revision,next_due_epoch FROM housing_upkeep_contracts WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(c.RowCount()!=1)return false;auto now=Q(db,"SELECT UNIX_TIMESTAMP()");if(N(c.begin()[2])<=N(now.begin()[0]))return false;auto g=Current(db,estate,N(c.begin()[0]),N(c.begin()[1]),uint32_t(N(c.begin()[2])));auto items=Obligations(db,estate,g,custody);if(std::none_of(items.begin(),items.end(),[&](const auto& i){return i.custody==custody;}))return false;auto balance=Q(db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");auto total=Total(items);if(g.plot_rate>uint64_t(INT64_MAX)-total||balance.RowCount()!=1||N(balance.begin()[0])<total+g.plot_rate)return false;Q(db,"UPDATE housing_upkeep_item_state SET expired_since=0 WHERE custody_id="+std::to_string(custody));return true;}catch(const Stop&){return false;}
}
// One complete SQL transaction per due cycle. Rows and source provenance are
// preserved; no player inventory/currency write, delete, refund or wallet choice.
// Receipt identity is (stable ownership generation, exact due epoch).
inline Outcome Tick(Database& db,uint32_t estate,uint32_t due,uint32_t now){
 using namespace Detail;bool active=false,committing=false;
 try{if(!Installed(db))return {};auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0]))return {Status::Conflict};Q(db,"START TRANSACTION");active=true;
  auto previous=Q(db,"SELECT charged_copper,stowed_count FROM housing_upkeep_cycles WHERE estate_id="+std::to_string(estate)+" AND due_epoch="+std::to_string(due)+" FOR UPDATE");
  if(previous.RowCount()){Outcome o{Status::Replayed,N(previous.begin()[0]),uint32_t(N(previous.begin()[1]))};Q(db,"ROLLBACK");active=false;return o;}
  auto c=Q(db,"SELECT native_plot_id,owner_kind,owner_id,catalog_revision,next_due_epoch,revision FROM housing_upkeep_contracts WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(c.RowCount()!=1)throw Stop{Status::Conflict};auto p=c.begin();if(N(p[4])!=due)throw Stop{Status::Conflict};if(now<due)throw Stop{Status::NotDue};if(!due||due>UINT32_MAX-3600)throw Stop{Status::Unsupported};const bool overdue=uint64_t(now)>=uint64_t(due)+3600;
  auto owner=Q(db,"SELECT owner_kind,owner_id FROM housing_current_estates WHERE id="+std::to_string(estate)+" FOR UPDATE");if(owner.RowCount()!=1||N(owner.begin()[0])!=N(p[1])||N(owner.begin()[1])!=N(p[2]))throw Stop{Status::Conflict};
  PendingEstate(db,estate,uint32_t(N(p[1])),uint32_t(N(p[2])));
  auto g=Current(db,estate,N(p[0]),N(p[3]),due);auto items=Obligations(db,estate,g);auto total=Total(items);if(g.plot_rate>uint64_t(INT64_MAX)-total)throw Stop{Status::Unsupported};total+=g.plot_rate;auto plot_expired=PlotExpired(db,estate,g.native);
  Q(db,"INSERT IGNORE INTO housing_escrow_balances(estate_id,copper) VALUES("+std::to_string(estate)+",0)");auto b=Q(db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+std::to_string(estate)+" FOR UPDATE");if(b.RowCount()!=1)throw Stop{Status::DatabaseFailure};const auto before=N(b.begin()[0]);
  auto catchup=UpkeepCatchup::Reviewed(db,estate,g.native,uint32_t(N(p[1])),uint32_t(N(p[2])),g.revision,EstateWire::EncodeBootstrap(g.bundle.definitions,g.bundle.costs),RegionWire::EncodeBootstrap(g.bundle.regions));
  std::optional<UpkeepCatchup::Checkpoint> checkpoint;
  if(overdue){if(!catchup)throw Stop{Status::Unsupported};checkpoint=UpkeepCatchup::MatchPast(db,estate,due,now,N(p[5]),before,CatchupWitness(db,estate,g,items),*catchup);if(!checkpoint)throw Stop{Status::Unsupported};}
  bool pay=before>=total;
  // Below the cheapest positive obligation none can be paid, regardless of
  // ordering. All other partially affordable cohorts remain unsupported.
  if(!pay&&((g.plot_rate&&before>=g.plot_rate)||std::any_of(items.begin(),items.end(),[&](const auto& i){return before>=i.rate;})))throw Stop{Status::Unsupported};
  if(pay&&(plot_expired||std::any_of(items.begin(),items.end(),[](const auto& i){return i.scope.kind==Kind::House&&i.expired_before;}))&&!RecoveryReviewed(db,estate,g))throw Stop{Status::Unsupported};
  Outcome out{Status::Applied,pay?total:0,0};if(!pay)for(const auto& i:items)if(i.scope.kind!=Kind::House)++out.stowed;auto after=before-out.charged;
  Q(db,"INSERT INTO housing_upkeep_cycles VALUES("+std::to_string(estate)+","+std::to_string(due)+","+std::to_string(g.revision)+","+std::to_string(N(p[5]))+","+std::to_string(before)+","+std::to_string(after)+","+std::to_string(out.charged)+","+std::to_string(out.stowed)+")");
  for(const auto& i:items){auto storage_after=i.storage;
   if(!pay&&i.scope.kind!=Kind::House){storage_after++;if(!i.storage)Q(db,"INSERT INTO housing_live_item_storage(custody_id,revision,stowed,native_estate_id) VALUES("+std::to_string(i.custody)+",1,1,"+std::to_string(i.scope.native)+")");else{auto u=Q(db,"UPDATE housing_live_item_storage SET stowed=1,revision=revision+1 WHERE custody_id="+std::to_string(i.custody)+" AND revision="+std::to_string(i.storage)+" AND stowed=0");if(u.RowsAffected()!=1)throw Stop{Status::Conflict};}}
   Q(db,"INSERT INTO housing_upkeep_item_state VALUES("+std::to_string(i.custody)+","+std::to_string(estate)+","+std::to_string(i.native)+","+std::to_string(pay?0:(i.expired_before?i.expired_before:due))+","+std::to_string(due)+") ON DUPLICATE KEY UPDATE estate_id=VALUES(estate_id),native_item_id=VALUES(native_item_id),expired_since=VALUES(expired_since),last_due_epoch=VALUES(last_due_epoch)");
   Q(db,"INSERT INTO housing_upkeep_cycle_items VALUES("+std::to_string(estate)+","+std::to_string(due)+","+std::to_string(i.custody)+","+std::to_string(i.native)+","+std::to_string(i.creator)+","+std::to_string(i.component)+","+std::to_string(i.rate)+","+std::to_string(pay?i.rate:0)+","+std::to_string(i.storage)+","+std::to_string(storage_after)+",x'"+Hex(i.raw)+"')");
   Q(db,"INSERT INTO housing_upkeep_cycle_scopes VALUES("+std::to_string(estate)+","+std::to_string(due)+","+std::to_string(i.custody)+","+std::to_string(uint32_t(i.scope.kind))+","+std::to_string(i.scope.native)+","+std::to_string(i.scope.house)+","+std::to_string(i.scope.profile)+",x'"+Hex(i.scope.proof)+"')");
  }
  const auto expired_after=pay?0u:(g.plot_rate?(plot_expired?plot_expired:due):0u);
  Q(db,"INSERT INTO housing_upkeep_property_cycles VALUES("+std::to_string(estate)+","+std::to_string(due)+","+std::to_string(g.plot_component)+","+std::to_string(g.plot_rate)+","+std::to_string(pay?g.plot_rate:0)+","+std::to_string(plot_expired)+","+std::to_string(expired_after)+")");
  Q(db,"INSERT INTO housing_upkeep_property_state VALUES("+std::to_string(estate)+","+std::to_string(g.native)+","+std::to_string(expired_after)+","+std::to_string(due)+") ON DUPLICATE KEY UPDATE expired_since=VALUES(expired_since),last_due_epoch=VALUES(last_due_epoch)");
  auto debit=Q(db,"UPDATE housing_escrow_balances SET copper="+std::to_string(after)+" WHERE estate_id="+std::to_string(estate)+" AND copper="+std::to_string(before));if(out.charged&&debit.RowsAffected()!=1)throw Stop{Status::Conflict};
  auto advance=Q(db,"UPDATE housing_upkeep_contracts SET next_due_epoch="+std::to_string(due+3600)+",revision=revision+1 WHERE estate_id="+std::to_string(estate)+" AND revision="+std::to_string(N(p[5])));if(advance.RowsAffected()!=1)throw Stop{Status::Conflict};
  if(catchup){auto next_graph=Current(db,estate,g.native,g.revision,due+3600);auto next_items=Obligations(db,estate,next_graph);UpkeepCatchup::Advance(db,estate,due,now,N(p[5])+1,after,CatchupWitness(db,estate,next_graph,next_items),*catchup,checkpoint);}
  committing=true;auto commit=db.QueryDatabase(std::string("COMMIT"),false);active=false;if(!commit.Success())return {Status::CommitUncertain};return out;
 }catch(const Stop& s){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {committing?Status::CommitUncertain:s.status};}catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {committing?Status::CommitUncertain:Status::DatabaseFailure};}
}
struct Batch {uint32_t applied=0,stowed=0,unsupported=0;bool refresh=false;};
// Fair bounded keyset scan. One unsupported old contract must not starve later
// properties; more than 100 due contracts must not disable the entire scheduler.
// Cursor only controls dispatch order, never entitlement or financial replay.
inline Batch ProcessDue(Database& db,uint32_t now,UpkeepClock::Cursor& cursor){
 using namespace Detail;Batch out;if(!Installed(db))return out;
 auto read=[&](){return Q(db,"SELECT c.estate_id,c.next_due_epoch FROM housing_upkeep_contracts c JOIN housing_current_estates e ON e.id=c.estate_id WHERE c.next_due_epoch<="+std::to_string(now)+" AND (c.next_due_epoch>"+std::to_string(cursor.due)+" OR (c.next_due_epoch="+std::to_string(cursor.due)+" AND c.estate_id>"+std::to_string(cursor.estate)+")) ORDER BY c.next_due_epoch,c.estate_id LIMIT 100");};
 auto due=read();if(!due.RowCount()&&(cursor.due||cursor.estate)){cursor={};due=read();}
 for(auto p=due.begin();p!=due.end();++p){const auto estate=N(p[0]),epoch=N(p[1]);if(!estate||estate>UINT32_MAX||!epoch||epoch>UINT32_MAX)throw Stop{Status::DatabaseFailure};cursor={uint32_t(epoch),uint32_t(estate)};auto r=Tick(db,uint32_t(estate),uint32_t(epoch),now);if(r.status==Status::Applied){++out.applied;out.stowed+=r.stowed;out.refresh=true;}else if(r.status!=Status::NotDue&&r.status!=Status::Replayed){++out.unsupported;if(r.status==Status::CommitUncertain)out.refresh=true;}}return out;
}
}
#endif
