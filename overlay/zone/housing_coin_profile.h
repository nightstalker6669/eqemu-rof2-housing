#ifndef EQEMU_HOUSING_COIN_PROFILE_H
#define EQEMU_HOUSING_COIN_PROFILE_H
#include "../common/housing/live_escrow_service.h"
#include "../common/eq_packet_structs.h"
#include "../common/inventory_profile.h"
namespace Housing::InventoryBridge::CoinProfile {
// Snapshot every currency column governed by the live escrow service. Shared
// platinum belongs to account storage and is deliberately outside this contract.
inline LiveEscrow::Currency Snapshot(const PlayerProfile_Struct& pp) noexcept {
 return {{pp.copper,pp.silver,pp.gold,pp.platinum},
  {pp.copper_bank,pp.silver_bank,pp.gold_bank,pp.platinum_bank},
  {pp.copper_cursor,pp.silver_cursor,pp.gold_cursor,pp.platinum_cursor},
  pp.currentRadCrystals,pp.careerRadCrystals,pp.currentEbonCrystals,pp.careerEbonCrystals};
}
inline bool CursorItemsEmpty(EQ::InventoryProfile& inventory) noexcept {
 if(!inventory.CursorEmpty()||inventory.GetItem(EQ::invslot::slotCursor))return false;
 for(int16_t slot=EQ::invbag::CURSOR_BAG_BEGIN;slot<=EQ::invbag::CURSOR_BAG_END;++slot)
  if(inventory.GetItem(slot))return false;
 return true;
}
// Only a pending committed receipt can be applied. Historical acknowledged
// outcomes must never write their old after-snapshot over current profile state.
// Legacy receipts retain every non-cursor counter. Explicit reviewed occupied
// refunds change only carried copper; all other counters stay exact.
inline bool ApplyPending(PlayerProfile_Struct& pp,const LiveEscrow::Receipt& receipt) noexcept {
 if(!receipt.requires_application)return false;
 auto unchanged=receipt.before;
 if(receipt.funding==LiveEscrow::Funding::Carried){
  if(receipt.occupied_refund||!CoinWallet::Empty(receipt.before.cursor)||!CoinWallet::Empty(receipt.after.cursor)||
   (receipt.action!=EscrowWire::Action::AddDay&&receipt.action!=EscrowWire::Action::AddWeek&&receipt.action!=EscrowWire::Action::WithdrawAll))return false;
  auto before=CoinWallet::TotalCopper(receipt.before.carried),after=CoinWallet::TotalCopper(receipt.after.carried);
  if(!before||!after||receipt.copper_transferred<=0||(receipt.action==EscrowWire::Action::WithdrawAll?after.value-before.value:before.value-after.value)!=receipt.copper_transferred)return false;
  unchanged.carried=receipt.after.carried;
 }else if(receipt.occupied_refund){
  if(receipt.action!=EscrowWire::Action::Withdraw)return false;
  unchanged.carried.copper=receipt.after.carried.copper;
  if(receipt.escrow_after!=0||receipt.copper_transferred<=0||int64_t(receipt.after.carried.copper)-receipt.before.carried.copper!=receipt.copper_transferred)return false;
 }else unchanged.cursor=receipt.after.cursor;
 if(!(unchanged==receipt.after)||!CoinWallet::Valid(receipt.before.carried)||
  !CoinWallet::Valid(receipt.before.bank)||!CoinWallet::Valid(receipt.before.cursor)||
  !CoinWallet::Valid(receipt.after.carried)||!CoinWallet::Valid(receipt.after.bank)||
  !CoinWallet::Valid(receipt.after.cursor))return false;
 auto current=Snapshot(pp);
 if(current==receipt.after)return true;
 if(!(current==receipt.before))return false;
 if(receipt.funding==LiveEscrow::Funding::Carried){pp.copper=receipt.after.carried.copper;pp.silver=receipt.after.carried.silver;pp.gold=receipt.after.carried.gold;pp.platinum=receipt.after.carried.platinum;}
 else if(receipt.occupied_refund)pp.copper=receipt.after.carried.copper;
 pp.copper_cursor=receipt.after.cursor.copper;
 pp.silver_cursor=receipt.after.cursor.silver;
 pp.gold_cursor=receipt.after.cursor.gold;
 pp.platinum_cursor=receipt.after.cursor.platinum;
 return Snapshot(pp)==receipt.after;
}
}
#endif
