#ifndef EQEMU_HOUSING_GUILD_PURCHASE_AUTHORITY_H
#define EQEMU_HOUSING_GUILD_PURCHASE_AUTHORITY_H
#include "live_item_service.h"
namespace Housing::GuildPurchaseAuthority {
inline constexpr uint32_t GuildPlotBuyAction=27;
struct Authority {
 AuthenticatedPurchaser actor;
 uint32_t guild_id=0;
 uint8_t native_rank=0,rank_mask=0;
 OwnerKind ownership_kind=OwnerKind::Guild;
};
enum class Error {None,Disabled,InvalidArgument,UnheldFence,TransactionRequired,Unauthorized,StaleLease,PendingInventory,CorruptData,DatabaseFailure};
template<class T>struct Result{T value{};Error error=Error::None;explicit operator bool()const{return error==Error::None;}};
inline bool RankGranted(uint32_t mask,uint32_t native_rank){return mask<=255&&native_rank>=1&&native_rank<=8&&((mask>>(8-native_rank))&1);}
// ONLY the exact ordinary guild action27 gate, not a completed purchase/ticket.
// Caller owns an active transaction and must keep it open through any eventual
// ownership/debit operation. Freshly validates actual guild/rank/mask; no packet
// guild ID, assumed leader grant or estate Owner6 bypass. No mutation/COMMIT.
// Target/VoA/member-limit/neighborhood/funding gates remain separate and absent
// evidence must still prohibit purchase. Explicit enablement defaults false.
Result<Authority> CheckUnderTransaction(Database&,const LiveItems::Lease&,
 LiveItems::InventorySessionFence&,bool enabled=false);
}
#endif
