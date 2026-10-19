#ifndef EQEMU_HOUSING_OCCUPIED_MOVE_OUT_DB_H
#define EQEMU_HOUSING_OCCUPIED_MOVE_OUT_DB_H
#include "rent_state_db.h"
#include "occupied_retirement_authority.h"
#include "estate_retirement_db.h"
#include "crate_storage_service.h"
#include "crate_binding.h"
#include "vault_storage_db.h"
#include "crate_vault_members_db.h"
#include "custody_codec.h"
#include "live_escrow_service.h"
#include <type_traits>
namespace Housing::OccupiedMoveOut {
// Server-selected destination, complete fresh scene, real fence. Native792c
// supplies none of these inventory/ledger identities. The native bridge stages
// ExpectedCrate before invoking this caller-owned inventory application path.
struct ExpectedCrate {uint64_t crate_id=0,custody_id=0,revision=0;LiveItems::InventoryRow row;};
struct Request {EstateRetirement::Request retirement;uint32_t destination_slot=0;std::optional<ExpectedCrate> expected_crate;std::optional<LiveEscrow::Request> refund;};
// Existing exact item outcome/recovery remains authoritative; the immutable
// retirement marker binds this outcome to the old generation independently of
// current plot ownership. Never acknowledges or clears pending here.
inline LiveItems::Result<LiveItems::Receipt> Resolve(Database& marker_db,LiveItems::Service& items,
 const Request& request,LiveItems::InventorySessionFence& fence){
 using namespace EstateRetirement::Detail;using Error=LiveItems::Error;const auto& r=request.retirement;
 auto outcome=items.Resolve(r.lease,r.key,fence);if(!outcome)return outcome;
 try{auto prior=Q(marker_db,"SELECT estate_id,character_id,account_id,lease_epoch,session_key,HEX(lease_token),native_plot_id FROM housing_estate_retirements WHERE request_key="+Quote(marker_db,r.key));
 if(prior.RowCount()!=1)return {{},Error::Conflict};auto row=prior.begin();
 if(N(row[0])!=r.estate_id||N(row[1])!=r.lease.actor.character_id||N(row[2])!=r.lease.actor.account_id||N(row[3])!=r.lease.epoch||!row[4]||std::string(row[4])!=r.lease.session_key||!row[5]||std::string(row[5])!=Hex(r.lease.token)||N(row[6])!=r.native_plot_id||outcome.value.action!=LiveItems::Action::Return||outcome.value.stored.estate_id!=r.estate_id||!outcome.value.crate_allocation||outcome.value.inventory_after.values.slot_id!=request.destination_slot)return {{},Error::Conflict};
 if(request.expected_crate){const auto& expected=*request.expected_crate;
  auto packed=Q(marker_db,"SELECT COUNT(*) FROM housing_crate_storage_receipts WHERE LEFT(request_key,"+std::to_string(r.key.size()+5)+")="+Quote(marker_db,r.key+":pack"));
  if(packed.RowCount()!=1)return {{},Error::DatabaseFailure};auto count=N(packed.begin()[0]);
  if(expected.revision>UINT64_MAX-count||!outcome.value.crate_before||outcome.value.crate_before->binding.crate_id!=expected.crate_id||outcome.value.stored.custody_id!=expected.custody_id||outcome.value.crate_before->binding.revision!=expected.revision+count||CustodyCodec::Encode(outcome.value.stored.row)!=CustodyCodec::Encode(expected.row))return {{},Error::Conflict};
 }
 return outcome;}catch(...){return {{},Error::DatabaseFailure};}
}
// Owns one TX. Existing typed item journal/pending is the application barrier.
// All packing, exact container return, and ownership retirement commit together.
// Funded requests require an exact explicit server-review wallet policy; no defaults.
inline LiveItems::Result<LiveItems::Receipt> Execute(Database& db,bool enabled,const Request& request,
 LiveItems::InventorySessionFence& fence,CrateBinding::CustodyContext& crates,
 HouseBinding::CustodyContext* houses=nullptr,const Scene::HouseScenePolicy* scene_policy=nullptr,
 const InteriorContent::ProfilePolicy* rooms=nullptr,LiveEscrow::CoinSessionFence* coin_fence=nullptr,
 std::optional<LiveEscrow::Receipt>* coin_out=nullptr){
 using namespace EstateRetirement::Detail;using Error=LiveItems::Error;const auto& r=request.retirement;
 if(!enabled)return {{},Error::Disabled};
 if(!Key(r.key)||r.key.size()>80||!r.estate_id||!r.native_plot_id||r.native_plot_id==UINT32_MAX)return {{},Error::InvalidArgument};
 if(coin_out)coin_out->reset();std::optional<LiveEscrow::Receipt> staged_refund;
 bool active=false,committing=false;
 try{
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1||N(tx.begin()[0]))return {{},Error::InvalidArgument};
  Q(db,"START TRANSACTION");active=true;
  auto auth=LiveItems::Service::AuthenticateUnderTransaction(db,true,r.lease,fence);if(!auth)throw Failure{auth.error};
  if(!auth.value.pending_key.empty()||CoinPending::Inspect(db,r.lease.actor.character_id,true)!=CoinPending::State::Clear)throw Failure{Error::PendingApplication};
  // Replay is intentionally resolved through existing LiveItems::Resolve on a
  // fresh connection, then the immutable retirement marker is checked. No new
  // current-generation mutation is attempted by this execution method.
  if(Q(db,"SELECT estate_id FROM housing_estate_retirements WHERE request_key="+Quote(db,r.key)+" FOR UPDATE").RowCount())throw Failure{Error::Conflict};
  Empty(db,"SELECT request_key FROM housing_live_item_receipts WHERE request_key="+Quote(db,r.key));
  auto view=Scene::Service::ReadSceneUnderTransaction(db,r.lease,r.estate_id,fence,scene_policy,&crates);if(!view)throw Failure{view.error};
  // All yard vault rows must be freshly visible under their own immutable
  // creator/payload witness. Existing direct packed rows remain covered by the
  // actual container validator; house vaults retain their same house scopes.
  if(VaultStorage::Installed(db)){auto vaults=Q(db,"SELECT custody_id FROM housing_live_vault_items v WHERE v.house_id IS NULL AND v.realm_estate_id="+std::to_string(r.estate_id)+VaultStorage::Uncontained(db)+" FOR UPDATE");if(vaults.RowCount()!=view.value.vault_items.size())throw Failure{Error::Conflict};}
  // Original Move Out help permits the owner to leave at any time. Keep
  // validating rent schema/identity while retaining its unchanged history;
  // the exception belongs only to this fully retiring aggregate.
  (void)RentState::Plot(db,r.estate_id,r.native_plot_id);
  for(const auto& item:view.value.items)(void)RentState::HouseActive(db,item.stored.custody_id);
  if(view.value.native_plot_id!=r.native_plot_id||view.value.definition.type!=4||!view.value.access.implicit_owner)throw Failure{Error::Unauthorized};
  auto generation=Q(db,"SELECT plot_id FROM housing_current_estates WHERE id="+std::to_string(r.estate_id)+" AND owner_kind=1 AND owner_id="+std::to_string(r.lease.actor.character_id)+" FOR UPDATE");if(generation.RowCount()!=1)throw Failure{Error::Unauthorized};auto plot=N(generation.begin()[0]);
  if(Q(db,"SELECT id FROM housing_plot_definitions WHERE id="+std::to_string(plot)+" FOR UPDATE").RowCount()!=1)throw Failure{Error::Conflict};
  Empty(db,"SELECT native_item_id FROM housing_live_placement_reservations WHERE estate_id="+std::to_string(r.estate_id)+" AND state=0");
  Empty(db,"SELECT instance_key FROM housing_fixture_item_custody WHERE estate_id="+std::to_string(r.estate_id));
  Empty(db,"SELECT pending.request_key FROM housing_live_escrow_pending pending JOIN housing_live_escrow_receipts receipt ON receipt.request_key=pending.request_key WHERE receipt.estate_id="+std::to_string(r.estate_id));
  auto balance=Q(db,"SELECT copper FROM housing_escrow_balances WHERE estate_id="+std::to_string(r.estate_id)+" FOR UPDATE");if(balance.RowCount()>1)throw Failure{Error::Conflict};
  const uint64_t copper=balance.RowCount()?N(balance.begin()[0]):0;
  if(copper){
   if(!request.refund||!coin_fence||!coin_out)throw Failure{Error::UnsupportedItem};
   const auto& c=*request.refund;
   if(c.request_key!=r.key||c.estate_id!=r.estate_id||c.lease.actor.character_id!=r.lease.actor.character_id||c.lease.actor.account_id!=r.lease.actor.account_id||c.lease.epoch!=r.lease.epoch||c.lease.session_key!=r.lease.session_key||c.lease.token!=r.lease.token||c.wire.estate_id!=r.native_plot_id||!c.occupied_refund||copper>INT64_MAX||c.wire.values.size()!=1||c.wire.values[0].amount!=int64_t(copper))throw Failure{Error::Conflict};
   auto refunded=LiveEscrow::Service::RefundOccupiedUnderTransaction(db,c,*coin_fence,scene_policy);if(!refunded)throw Failure{refunded.error==LiveEscrow::Error::Overflow?Error::Conflict:Error::UnsupportedItem};
   staged_refund=std::move(refunded.value);
  }else if(request.refund)throw Failure{Error::Conflict};
  const Scene::Item* crate=nullptr;
  for(const auto& item:view.value.items)if(item.crate){if(crate)throw Failure{Error::UnsupportedItem};crate=&item;}
  if(!crate||crate->stored.stowed||!crate->stored.placement||crate->crate->packed||crate->crate->created_by_character_id!=r.lease.actor.character_id)throw Failure{Error::UnsupportedItem};
  if(request.expected_crate){const auto& expected=*request.expected_crate;
   if(crate->crate->binding.crate_id!=expected.crate_id||crate->stored.custody_id!=expected.custody_id||crate->crate->binding.revision!=expected.revision||CustodyCodec::Encode(crate->stored.row)!=CustodyCodec::Encode(expected.row))throw Failure{Error::Conflict};
  }
  // Original6435: a different carried/banked nonempty crate prevents sale.
  // Bank location adapters are not installed; all typed location_kind1 records
  // are conservatively checked, and unbound bank crate templates deny below.
  auto carried=Q(db,"SELECT l.crate_id FROM housing_crate_locations l WHERE l.location_kind=1 AND l.character_id="+std::to_string(r.lease.actor.character_id)+" AND (EXISTS (SELECT 1 FROM housing_crate_members m WHERE m.crate_id=l.crate_id)"+(VaultStorage::CrateMembersInstalled(db)?" OR EXISTS (SELECT 1 FROM housing_crate_vault_members vm WHERE vm.crate_id=l.crate_id)":"")+") FOR UPDATE");if(carried.RowCount())throw Failure{Error::Conflict};
  Empty(db,"SELECT h.house_id FROM housing_house_locations h WHERE h.location_kind=1 AND h.character_id="+std::to_string(r.lease.actor.character_id)+" AND EXISTS (SELECT 1 FROM housing_house_item_scopes s WHERE s.house_id=h.house_id)");
  auto bank=Q(db,"SELECT slot_id FROM inventory WHERE character_id="+std::to_string(r.lease.actor.character_id)+" AND slot_id BETWEEN "+std::to_string(EQ::invslot::BANK_BEGIN)+" AND "+std::to_string(EQ::invbag::BANK_BAGS_END)+" AND item_id IN (52147,61823,61824,61825,61826,61827,61828,61829,61830,61831,61832,61833,61834,61835,61836,61837,61838,61839,61840,61841,61842,61843,61844,61845,61846,61847,61848,61849,61850,61851,61852,61853,61854,61855,61856,61857,61858,61859,61860,61861,61862,61863,61864,61865,61866,61867,61868,61869,61870,61871,61872,61873,61874,61875,61876,61877,61878,61879,61880,61881,61882,61883,61884,61885,61886,61887,61888,61889,61890,61891,61892,61893,61894,61895,61896,61897,61898,61899,61900,61901,61902,61903,61904,61905,61906,61907,61908,61909,61910,61911,61912,61913,61914,61915,61916,61917,61918,61919,61920,61921,61922,61923,61924,61925,61926,61927,61928,61929,61930,61931,61932,61933,61934,61935,61936,61937,61938,61939,61940,61941,61942,66618,66619,66620,66660,62586,62587,62588,62589,62590,62591,62592,62593,62594,62595,62596) FOR UPDATE");if(bank.RowCount())throw Failure{Error::UnsupportedItem};
  // Reject unsupported inputs before creating the transaction-only authority.
  for(const auto& item:view.value.items)if(item.stored.custody_id!=crate->stored.custody_id&&(!item.stored.placement||item.crate))throw Failure{Error::UnsupportedItem};
  if(uint64_t(view.value.items.size()-1+view.value.vault_items.size())>=UINT64_MAX-crate->crate->binding.revision)throw Failure{Error::Conflict};
  OccupiedRetirement::Authority retirement(db,r.lease,r.key,r.estate_id,r.native_plot_id,request.destination_slot,view.value,*crate);
  uint64_t revision=crate->crate->binding.revision;const auto crate_id=crate->crate->binding.crate_id;
  unsigned index=0;
  for(const auto& item:view.value.items){
   if(item.stored.custody_id==crate->stored.custody_id)continue;
   if(!item.stored.placement||item.crate)throw Failure{Error::UnsupportedItem};
   CrateStorage::Request pack;pack.request_key=r.key+":pack"+std::to_string(index++);pack.lease=r.lease;pack.estate_id=r.estate_id;pack.native_plot_id=r.native_plot_id;pack.catalog_revision=view.value.current_catalog_revision;pack.source=view.value.route;pack.crate_id=crate_id;pack.crate_revision=revision;pack.custody_id=item.stored.custody_id;pack.pose_revision=item.stored.placement->pose_revision;pack.storage_revision=item.stored.storage_revision;
   auto moved=CrateStorage::Service::PackForRetirementUnderTransaction(db,true,pack,fence,retirement,crates,scene_policy,rooms);if(!moved)throw Failure{moved.error};revision=moved.value.crate_revision;
  }
  for(const auto& item:view.value.vault_items){CrateStorage::Request pack;pack.request_key=r.key+":pack"+std::to_string(index++);pack.lease=r.lease;pack.estate_id=r.estate_id;pack.native_plot_id=r.native_plot_id;pack.catalog_revision=view.value.current_catalog_revision;pack.source=view.value.route;pack.crate_id=crate_id;pack.crate_revision=revision;pack.custody_id=item.stored.custody_id;pack.storage_revision=item.revision;pack.vault=true;auto moved=CrateStorage::Service::PackForRetirementUnderTransaction(db,true,pack,fence,retirement,crates,scene_policy,rooms);if(!moved)throw Failure{moved.error};revision=moved.value.crate_revision;}
  // Every old-origin custody must be the container, its direct member, or an
  // interior item of a house retained as a direct member. No deletion, owner
  // rewrite or furnished-room flattening can make this proof succeed.
  Empty(db,"SELECT c.custody_id FROM housing_live_item_custody c WHERE c.estate_id="+std::to_string(r.estate_id)+" AND c.custody_id<>"+std::to_string(crate->stored.custody_id)+" AND NOT EXISTS (SELECT 1 FROM housing_crate_members m WHERE m.crate_id="+std::to_string(crate_id)+" AND m.custody_id=c.custody_id)"+(VaultStorage::CrateMembersInstalled(db)?" AND NOT EXISTS (SELECT 1 FROM housing_crate_vault_members vm WHERE vm.crate_id="+std::to_string(crate_id)+" AND vm.custody_id=c.custody_id)":"")+" AND NOT EXISTS (SELECT 1 FROM housing_house_item_scopes s JOIN housing_house_locations h ON h.house_id=s.house_id AND h.location_kind=2 JOIN housing_crate_members m ON m.custody_id=h.custody_id WHERE s.custody_id=c.custody_id AND m.crate_id="+std::to_string(crate_id)+")");
  Empty(db,"SELECT n.native_id FROM housing_native_estates n WHERE n.parent_native_id="+std::to_string(r.native_plot_id)+" AND n.active=1 AND n.native_id<>"+std::to_string(crate->crate->binding.native_estate_id)+" AND NOT EXISTS (SELECT 1 FROM housing_house_interiors i JOIN housing_house_locations h ON h.house_id=i.house_id AND h.location_kind=2 JOIN housing_crate_members m ON m.custody_id=h.custody_id WHERE i.native_estate_id=n.native_id AND m.crate_id="+std::to_string(crate_id)+")");
  Empty(db,"SELECT sessions.character_id FROM housing_crate_members m JOIN housing_house_locations h ON h.location_kind=2 AND h.custody_id=m.custody_id JOIN housing_house_interiors i ON i.house_id=h.house_id JOIN housing_inventory_sessions sessions ON sessions.zone_id=i.zone_id AND sessions.instance_id=i.instance_id AND sessions.active=1 WHERE m.crate_id="+std::to_string(crate_id));
  Empty(db,"SELECT storage.custody_id FROM housing_live_item_storage storage WHERE storage.native_estate_id="+std::to_string(r.native_plot_id)+" AND storage.custody_id<>"+std::to_string(crate->stored.custody_id)+" AND NOT EXISTS (SELECT 1 FROM housing_crate_members member WHERE member.crate_id="+std::to_string(crate_id)+" AND member.custody_id=storage.custody_id)");
  Empty(db,"SELECT pose.custody_id FROM housing_live_placed_items pose JOIN housing_live_placement_reservations reservation ON reservation.native_item_id=pose.native_item_id WHERE reservation.native_estate_id="+std::to_string(r.native_plot_id)+" AND pose.custody_id<>"+std::to_string(crate->stored.custody_id)+" AND NOT EXISTS (SELECT 1 FROM housing_crate_members member WHERE member.crate_id="+std::to_string(crate_id)+" AND member.custody_id=pose.custody_id)");
  LiveItems::Request pickup;pickup.request_key=r.key;pickup.lease=r.lease;pickup.estate_id=r.estate_id;pickup.action=LiveItems::Action::Return;pickup.expected=crate->stored.row;pickup.custody_id=crate->stored.custody_id;pickup.destination_slot=request.destination_slot;pickup.crate=LiveItems::CrateIdentity{crate_id,revision};
  auto returned=LiveItems::Service::ReturnCrateForRetirementUnderTransaction(db,true,pickup,fence,retirement,houses,&crates,scene_policy,rooms);if(!returned)throw Failure{returned.error};
  if(!returned.value.crate_allocation||returned.value.crate_allocation->inventory_character_id!=r.lease.actor.character_id||returned.value.crate_allocation->binding.crate_id!=crate_id)throw Failure{Error::Conflict};
  if(Q(db,"UPDATE housing_native_estates SET ownership_estate_id=NULL WHERE native_id="+std::to_string(r.native_plot_id)+" AND ownership_estate_id="+std::to_string(r.estate_id)+" AND active=1").RowsAffected()!=1)throw Failure{Error::Conflict};
  if(Q(db,"UPDATE housing_estates SET active=0 WHERE id="+std::to_string(r.estate_id)+" AND active=1").RowsAffected()!=1)throw Failure{Error::Conflict};
  Q(db,"INSERT INTO housing_estate_retirements(estate_id,request_key,character_id,account_id,lease_epoch,session_key,lease_token,native_plot_id,plot_id,catalog_revision) VALUES ("+std::to_string(r.estate_id)+","+Quote(db,r.key)+","+std::to_string(r.lease.actor.character_id)+","+std::to_string(r.lease.actor.account_id)+","+std::to_string(r.lease.epoch)+","+Quote(db,r.lease.session_key)+",x'"+Hex(r.lease.token)+"',"+std::to_string(r.native_plot_id)+","+std::to_string(plot)+","+std::to_string(view.value.current_catalog_revision)+")");
  committing=true;auto commit=db.QueryDatabase(std::string("COMMIT"),false);active=false;if(!commit.Success())return {{},Error::CommitUncertain};
  static_assert(std::is_nothrow_move_assignable_v<std::optional<LiveEscrow::Receipt>>);
  if(coin_out)*coin_out=std::move(staged_refund);return returned;
 }catch(const Failure& failure){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},committing?Error::CommitUncertain:failure.error};}
 catch(...){if(active)db.QueryDatabase(std::string("ROLLBACK"),false);return {{},committing?Error::CommitUncertain:Error::DatabaseFailure};}
}
}
#endif
