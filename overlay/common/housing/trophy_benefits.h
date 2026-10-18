#ifndef EQEMU_HOUSING_TROPHY_BENEFITS_H
#define EQEMU_HOUSING_TROPHY_BENEFITS_H
#include "trophy_anchor_wire.h"
#include <algorithm>
#include <map>
#include <set>
namespace Housing::TrophyBenefits {
// Original 4335c0: four base slots plus SPA432, capped at the ten wire slots.
inline uint32_t Slots(int64_t expansion){return uint32_t(std::clamp<int64_t>(4+std::clamp<int64_t>(expansion,-4,6),0,10));}
struct Tier {uint32_t minimum_level=0,cost=0,stat_item_id=0;};
struct Definition {uint32_t id=0,placed_item_id=0;std::string name,source_sha256;std::vector<Tier> tiers;};
using Catalog=std::map<uint32_t,Definition>;
inline bool SourceDigest(const std::string& s){return s.size()==64&&s.find_first_not_of("0123456789abcdef")==std::string::npos&&s.find_first_not_of('0')!=std::string::npos;}
inline bool Valid(const Definition& d){
 if(d.id==UINT32_MAX||!d.placed_item_id||d.name.empty()||d.name.find('\0')!=std::string::npos||!SourceDigest(d.source_sha256)||d.tiers.empty()||d.tiers.size()>10)return false;
 uint32_t prior=0;for(const auto& t:d.tiers){if(!t.stat_item_id||!t.minimum_level||t.minimum_level<prior||t.minimum_level>255)return false;prior=t.minimum_level;}return true;
}
// Eligibility is supplied only by locked, current server custody/placement
// authority. A client benefit ID and an inventory copy never supply it.
struct View {TrophyAnchorWire::Benefits selected;std::vector<uint32_t> stat_items;std::array<uint32_t,10> stat_item_slots{};uint32_t remaining_ms=0,charged=0;std::optional<uint32_t> favor_after;bool insufficient_favor=false,billing_unavailable=false;};
inline void Revalidate(View& out,const Catalog& definitions,const std::set<uint32_t>& eligible,uint32_t level,uint32_t capacity,bool paid_authorized=false){
 std::set<uint32_t> seen;uint32_t count=0;bool paid=false;out.stat_items.clear();out.stat_item_slots.fill(0);
 for(size_t i=0;i<10;++i){const auto id=out.selected.ids[i];const auto tier=out.selected.tiers[i];auto found=definitions.find(id);
  if(id==UINT32_MAX) {out.selected.tiers[i]=-1;continue;}
  if(count>=capacity||found==definitions.end()||!Valid(found->second)||!eligible.count(id)||!seen.insert(id).second||tier<0||size_t(tier)>=found->second.tiers.size()||level<found->second.tiers[tier].minimum_level){out.selected.ids[i]=UINT32_MAX;out.selected.tiers[i]=-1;continue;}
  ++count;paid|=found->second.tiers[tier].cost!=0;
 }
 // Paid favor accounting and original cadence are not recovered. Never grant
 // a paid tier for free or borrow the unrelated ordinary tribute timer.
 if((paid&&!paid_authorized)||!count)out.selected.enabled=0;
 if(out.selected.enabled)for(size_t i=0;i<10;++i)if(out.selected.ids[i]!=UINT32_MAX){out.stat_item_slots[i]=definitions.at(out.selected.ids[i]).tiers[out.selected.tiers[i]].stat_item_id;out.stat_items.push_back(out.stat_item_slots[i]);}
}
inline bool Select(View& view,const TrophyAnchorWire::Selection& selection,const Catalog& defs,const std::set<uint32_t>& eligible,uint32_t level,uint32_t capacity){
 if(selection.slot>=10)return false;auto next=view;
 if(selection.benefit_id==UINT32_MAX){if(selection.tier!=-1)return false;next.selected.ids[selection.slot]=UINT32_MAX;next.selected.tiers[selection.slot]=-1;}
 else{auto d=defs.find(selection.benefit_id);if(d==defs.end()||!Valid(d->second)||!eligible.count(selection.benefit_id)||selection.tier<0||size_t(selection.tier)>=d->second.tiers.size()||level<d->second.tiers[selection.tier].minimum_level)return false;
  for(size_t i=0;i<10;++i)if(i!=selection.slot&&next.selected.ids[i]==selection.benefit_id)return false;
  next.selected.ids[selection.slot]=selection.benefit_id;next.selected.tiers[selection.slot]=selection.tier;
  size_t count=0;for(auto id:next.selected.ids)count+=id!=UINT32_MAX;if(count>capacity)return false;
 }
 // Editing never retains paid activation authority from another selection.
 next.selected.enabled=0;Revalidate(next,defs,eligible,level,capacity);view=std::move(next);return true;
}
inline bool Toggle(View& view,bool enable,const Catalog& defs,const std::set<uint32_t>& eligible,uint32_t level,uint32_t capacity){
 Revalidate(view,defs,eligible,level,capacity);if(!enable){view.selected.enabled=0;view.stat_items.clear();view.stat_item_slots.fill(0);return true;}
 bool any=false;for(size_t i=0;i<10;++i)if(view.selected.ids[i]!=UINT32_MAX){any=true;if(defs.at(view.selected.ids[i]).tiers[view.selected.tiers[i]].cost)return false;}
 if(!any)return false;view.selected.enabled=1;Revalidate(view,defs,eligible,level,capacity);return true;
}
}
#endif
