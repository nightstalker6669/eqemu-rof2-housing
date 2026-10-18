#ifndef EQEMU_HOUSING_ESCROW_RESPONSE_H
#define EQEMU_HOUSING_ESCROW_RESPONSE_H
#include "../common/housing/coin_wallet.h"
#include "../common/housing/escrow_wire.h"
namespace Housing::InventoryBridge::EscrowResponse {
// A rejected/disabled deposit restores only a recoverable selected coin bucket
// from CURRENT server state. No request quantity, old receipt, or guessed
// denomination supplies money. Never emit a failure/refund after known COMMIT.
inline std::optional<std::vector<uint8_t>> Rejected(
 const EscrowWire::Message& request,const CoinWallet::Snapshot& cursor,bool known_committed=false){
 if(known_committed||!request.estate_id||request.estate_id==UINT32_MAX)return {};
 if(request.action==uint32_t(EscrowWire::Action::Deposit)){
  if(!CoinWallet::Valid(cursor)||request.values.empty()||request.values.front().type!=0)return {};
  const auto denomination=request.values.front().subtype;
  if(denomination<0||denomination>3)return {};
  return EscrowWire::EncodeCursorAssignment(EscrowWire::Action::Deposit,
   request.estate_id,denomination,
   CoinWallet::Detail::Counter(cursor,CoinWallet::Denomination(denomination)));
 }
 if((request.action==uint32_t(EscrowWire::Action::Withdraw)||request.action==3||request.action==4||request.action==7)&&EscrowWire::IsSupportedCoinRequest(request)){
  EscrowWire::Message response;response.action=request.action;response.estate_id=request.estate_id;
  // Native withdraw1/flag0/count0 performs no cursor assignment.
  return EscrowWire::Encode(response);
 }
 return {};
}
}
#endif
