#ifndef EQEMU_HOUSING_EVOLVING_ITEM_CUSTODY_H
#define EQEMU_HOUSING_EVOLVING_ITEM_CUSTODY_H
#include "../item_instance.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <stdexcept>
#include <vector>
namespace Housing::EvolvingItemCustody {
using Bytes=std::vector<uint8_t>;
inline constexpr size_t MaximumPayload=4096;
// component0 is the ordinary instance;1..6 are its existing augment sockets.
// Original SQL NULLs and persisted double bits are distinct from the native
// recalculated progression shown by an actual ItemInstance.
struct State {uint8_t component=0,nulls=0;uint64_t id=0;uint32_t owner=0,item=0;uint8_t activated=0,equipped=0;int64_t amount=0;uint64_t progression_bits=0;uint32_t final_item=0;uint64_t runtime_progression_bits=0;};
inline uint64_t Bits(double d){uint64_t n;std::memcpy(&n,&d,8);return n;}
inline double Float(uint64_t n){double d;std::memcpy(&d,&n,8);return d;}
inline bool Valid(const State&s){return s.component<=6&&!(s.nulls&0x83)&&s.id&&s.owner&&s.item&&s.activated<=1&&s.equipped<=1&&s.amount>=0&&std::isfinite(Float(s.progression_bits))&&Float(s.progression_bits)>=0&&std::isfinite(Float(s.runtime_progression_bits))&&Float(s.runtime_progression_bits)>=0&&(!(s.nulls&4)||!s.activated)&&(!(s.nulls&8)||!s.equipped)&&(!(s.nulls&16)||!s.amount)&&(!(s.nulls&32)||!s.progression_bits)&&(!(s.nulls&64)||!s.final_item);}
inline Bytes Encode(const std::vector<State>& states){if(states.empty())return {};if(states.size()>7)throw std::invalid_argument("Evolving component bound");Bytes b{'H','E','V',1};auto word=[&](uint64_t n,unsigned width){for(unsigned k=0;k<width;++k)b.push_back(uint8_t(n>>(k*8)));};word(states.size(),4);int last=-1;std::set<uint64_t> ids;for(const auto&s:states){if(!Valid(s)||int(s.component)<=last||!ids.insert(s.id).second)throw std::invalid_argument("Invalid evolving custody identity");last=s.component;word(s.component,1);word(s.nulls,1);word(s.id,8);word(s.owner,4);word(s.item,4);word(s.activated,1);word(s.equipped,1);word(uint64_t(s.amount),8);word(s.progression_bits,8);word(s.final_item,4);word(s.runtime_progression_bits,8);}return b;}
inline std::vector<State> Decode(const Bytes& b){if(b.empty())return {};if(b.size()>MaximumPayload||b.size()<8||b[0]!='H'||b[1]!='E'||b[2]!='V'||b[3]!=1)throw std::invalid_argument("Invalid evolving custody payload");size_t at=4;auto word=[&](unsigned width){if(at+width>b.size())throw std::invalid_argument("Truncated evolving custody payload");uint64_t n=0;for(unsigned k=0;k<width;++k)n|=uint64_t(b[at++])<<(k*8);return n;};auto count=word(4);if(!count||count>7||b.size()!=8+48*count)throw std::invalid_argument("Invalid evolving custody framing");std::vector<State> states;for(unsigned k=0;k<count;++k){State s;s.component=word(1);s.nulls=word(1);s.id=word(8);s.owner=word(4);s.item=word(4);s.activated=word(1);s.equipped=word(1);s.amount=int64_t(word(8));s.progression_bits=word(8);s.final_item=word(4);s.runtime_progression_bits=word(8);states.push_back(s);}if(Encode(states)!=b)throw std::invalid_argument("Noncanonical evolving custody payload");return states;}
inline EQ::ItemInstance* Component(EQ::ItemInstance& item,uint8_t c){return c?(item.IsClassCommon()?item.GetAugment(c-1):nullptr):&item;}
inline bool Has(const EQ::ItemInstance& item){if(item.IsEvolving())return true;if(item.IsClassCommon())for(unsigned i=0;i<6;++i)if(auto*a=item.GetAugment(i);a&&a->IsEvolving())return true;return false;}
inline bool Matches(const Bytes& b,EQ::ItemInstance& item,uint32_t owner){try{auto states=Decode(b);unsigned count=0;for(unsigned k=0;k<=6;++k)if(auto*n=Component(item,k);n&&n->IsEvolving())++count;if(count!=states.size())return false;for(const auto&s:states){auto*n=Component(item,s.component);if(!n||!n->IsEvolving()||n->GetID()!=s.item||s.owner!=owner||n->GetEvolveUniqueID()!=s.id||n->GetEvolveCharID()!=owner||n->GetEvolveItemID()!=s.item||n->GetEvolveActivated()!=bool(s.activated)||n->GetEvolveEquipped()!=bool(s.equipped)||n->GetEvolveCurrentAmount()!=uint64_t(s.amount)||Bits(n->GetEvolveProgression())!=s.runtime_progression_bits||n->GetEvolveFinalItemID()!=s.final_item)return false;}return true;}catch(...){return false;}}
inline bool Restore(const Bytes& b,EQ::ItemInstance& item,uint32_t owner){try{auto states=Decode(b);unsigned count=0;for(unsigned k=0;k<=6;++k)if(auto*n=Component(item,k);n&&n->IsEvolving())++count;if(count!=states.size())return false;for(const auto&s:states){auto*n=Component(item,s.component);if(!n||!n->IsEvolving()||n->GetID()!=s.item||s.owner!=owner||(n->GetEvolveUniqueID()&&n->GetEvolveUniqueID()!=s.id))return false;}for(const auto&s:states){auto*n=Component(item,s.component);auto&e=n->GetEvolvingDetails();e.id=s.id;e.character_id=owner;e.item_id=s.item;e.activated=s.activated;e.equipped=s.equipped;e.current_amount=s.amount;e.progression=Float(s.runtime_progression_bits);e.final_item_id=s.final_item;e.deleted_at=0;}return Matches(b,item,owner);}catch(...){return false;}}
}
#endif
