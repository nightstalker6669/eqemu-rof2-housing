#ifndef EQEMU_HOUSING_TROPHY_BILLING_H
#define EQEMU_HOUSING_TROPHY_BILLING_H
#include "trophy_benefits.h"
#include "live_item_service.h"
namespace Housing::TrophyBenefits {
// Original51ae is milliseconds. Retained May2013 help/tips/ctxtributebenefit.html
// lines 15/23 explicitly specify ten-minute personal/guild trophy cycles and
// charging on activation. Other pause/edit/offline details remain unproven.
// No policy row is seeded; policy1 still requires explicit server review.
inline constexpr uint32_t OriginalTrophyPeriodMs=600000;
struct BillingPolicy {uint32_t period_ms=0;std::string source_sha256;uint32_t policy=0;};
inline bool Valid(const BillingPolicy& p){return p.policy==1&&p.period_ms==OriginalTrophyPeriodMs&&SourceDigest(p.source_sha256);}
// Policy1: charge one reviewed period up front, real-time deadline while enabled,
// pause remaining paid time on explicit Disable, clear prepayment on edit/source
// withdrawal, no catch-up bills or offline renewal. This is explicit server policy.
struct PaidState {bool enabled=false;uint32_t remaining_ms=0;uint64_t until_ms=0;std::string selection_hash;};
enum class BillAction {Read,Enable,Disable,Renew,Invalidate};
struct BillResult {PaidState state;uint32_t remaining_ms=0,charge=0;std::optional<uint32_t> favor_after;bool insufficient=false;};
struct FavorAuthority {
 virtual ~FavorAuthority()=default;
 // Genuine current Client favor under the held inventory/profile save barrier.
 // No packet-supplied amount, stale character_data balance, sends or callbacks.
 virtual std::optional<uint32_t> CurrentFavorUnderFence(const LiveItems::Lease&)const noexcept=0;
};
inline std::optional<BillResult> Bill(const BillingPolicy& policy,PaidState state,BillAction action,uint64_t now,uint32_t cost,std::optional<uint32_t> favor,const std::string& selection){
 if(!Valid(policy)||!now||!SourceDigest(selection)||state.remaining_ms>policy.period_ms)return {};
 BillResult out;out.state=state;
 auto invalidate=[&]{out.state={false,0,0,selection};out.remaining_ms=0;};
 if(state.selection_hash!=selection||action==BillAction::Invalidate){invalidate();if(action!=BillAction::Enable)return out;}
 auto remaining=[&]()->uint32_t{if(out.state.enabled){if(out.state.until_ms<=now)return 0;auto left=out.state.until_ms-now;return left<=policy.period_ms?uint32_t(left):0;}return out.state.remaining_ms;};
 if(action==BillAction::Disable){out.state.remaining_ms=remaining();out.state.enabled=false;out.state.until_ms=0;out.remaining_ms=out.state.remaining_ms;return out;}
 if(action==BillAction::Read){out.remaining_ms=remaining();if(out.state.enabled&&!out.remaining_ms)invalidate();return out;}
 if(action==BillAction::Renew&&!out.state.enabled){out.remaining_ms=out.state.remaining_ms;return out;}
 if(out.state.enabled&&out.state.until_ms>now){out.remaining_ms=remaining();if(!out.remaining_ms)return {};return out;}
 if(!out.state.enabled&&out.state.remaining_ms){if(now>UINT64_MAX-out.state.remaining_ms)return {};out.state.enabled=true;out.state.until_ms=now+out.state.remaining_ms;out.remaining_ms=out.state.remaining_ms;return out;}
 if(!favor||cost>*favor){invalidate();out.insufficient=true;return out;}
 if(now>UINT64_MAX-policy.period_ms)return {};out.state={true,policy.period_ms,now+policy.period_ms,selection};out.remaining_ms=policy.period_ms;out.charge=cost;out.favor_after=*favor-cost;return out;
}
}
#endif
