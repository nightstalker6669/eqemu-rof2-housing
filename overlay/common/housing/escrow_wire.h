#ifndef EQEMU_HOUSING_ESCROW_WIRE_H
#define EQEMU_HOUSING_ESCROW_WIRE_H
#include "neighborhood_wire.h"
#include <limits>
namespace Housing::EscrowWire {
// Exact May10 2013 reader748700/serializer748ce0. See ESCROW-RESPONSE-FUNDING.md.
// Framing only: no funding policy, permission grants or native handler binding.
inline constexpr uint16_t Opcode=0x2aa6;
enum class Action:uint32_t { Deposit=0,Withdraw=1,SelectUpkeep=2,AddDay=3,AddWeek=4,WithdrawAll=7 };
struct Value { int32_t type{},subtype{}; int64_t amount{}; };
struct Message {
 uint32_t action{},estate_id{},component_id{};
 std::string first,second;
 uint8_t response_flag{};
 uint32_t opaque_word{};
 std::vector<Value> values;
};
inline std::vector<uint8_t> Encode(const Message& m){
 using namespace NeighborhoodWire;
 if(m.first.size()>63||m.second.size()>31||m.first.find('\0')!=std::string::npos||m.second.find('\0')!=std::string::npos)
  throw std::invalid_argument("Invalid escrow string");
 const size_t prefix=23+m.first.size()+m.second.size();
 if(m.values.size()>(MaximumPayload-prefix)/16)throw std::invalid_argument("Escrow payload too large");
 std::vector<uint8_t> b;b.reserve(prefix+16*m.values.size());
 for(auto v:{m.action,m.estate_id,m.component_id})Detail::Write32(b,v);
 for(const auto& s:{m.first,m.second}){b.insert(b.end(),s.begin(),s.end());b.push_back(0);}
 b.push_back(m.response_flag);Detail::Write32(b,m.opaque_word);Detail::Write32(b,uint32_t(m.values.size()));
 for(const auto& v:m.values){Detail::Write32(b,uint32_t(v.type));Detail::Write32(b,uint32_t(v.subtype));for(unsigned i=0;i<8;++i)b.push_back(uint8_t(uint64_t(v.amount)>>(8*i)));}
 return b;
}
inline std::optional<Message> Decode(const uint8_t* p,size_t n){
 if(!p||n<23||n>NeighborhoodWire::MaximumPayload)return std::nullopt;
 Message m;size_t at=0;
 auto word=[&](){auto v=NeighborhoodWire::Detail::Read32(p+at);at+=4;return v;};
 m.action=word();m.estate_id=word();m.component_id=word();
 auto text=[&](std::string& out,size_t limit){size_t start=at;while(at<n&&p[at]!=0){if(at-start>=limit)return false;++at;}if(at==n)return false;out.assign(reinterpret_cast<const char*>(p+start),at-start);++at;return true;};
 if(!text(m.first,63)||!text(m.second,31)||n-at<9)return std::nullopt;
 m.response_flag=p[at++];m.opaque_word=word();uint32_t count=word();
 if(count>(n-at)/16||size_t(count)*16!=n-at)return std::nullopt;
 m.values.reserve(count);
 for(uint32_t i=0;i<count;++i){Value v;uint32_t a=word(),b=word();std::memcpy(&v.type,&a,4);std::memcpy(&v.subtype,&b,4);uint64_t bits=0;for(unsigned j=0;j<8;++j)bits|=uint64_t(p[at++])<<(8*j);std::memcpy(&v.amount,&bits,8);m.values.push_back(v);}
 return m;
}
// Checked request subset. These rules deliberately exclude unrecovered actions and
// alternate currency. Online authority and balance checks belong to the handler.
inline bool IsSupportedCoinRequest(const Message& m){
 if(!m.estate_id||m.estate_id==0xffffffff||!m.first.empty()||!m.second.empty()||m.response_flag||m.opaque_word)return false;
 switch(static_cast<Action>(m.action)){
 case Action::Deposit:case Action::Withdraw:
  return m.component_id==0&&m.values.size()==1&&m.values[0].type==0&&m.values[0].subtype>=0&&m.values[0].subtype<=3&&m.values[0].amount>0&&m.values[0].amount<=std::numeric_limits<int32_t>::max();
 case Action::SelectUpkeep:return m.component_id&&m.component_id!=0xffffffff&&m.values.empty();
 case Action::AddDay:case Action::AddWeek:case Action::WithdrawAll:return !m.component_id&&m.values.empty();
 default:return false;
 }
}
// Only the two proved cursor-assignment branches. Quantity is authoritative
// resulting cursor state, not a requested amount or a delta. The caller must
// fence cursor state while committing/resolving a transaction and delivering it.
inline std::vector<uint8_t> EncodeCursorAssignment(Action action,uint32_t estate_id,int32_t denomination,int64_t quantity){
 if((action!=Action::Deposit&&action!=Action::Withdraw)||!estate_id||estate_id==0xffffffff||denomination<0||denomination>3||quantity<0||quantity>std::numeric_limits<int32_t>::max())
  throw std::invalid_argument("Invalid cursor assignment");
 Message m{};m.action=uint32_t(action);m.estate_id=estate_id;m.response_flag=action==Action::Withdraw?1:0;m.values={{0,denomination,quantity}};
 return Encode(m);
}
inline std::optional<uint64_t> CoinCopperValue(const Value& v){
 if(v.type!=0||v.subtype<0||v.subtype>3||v.amount<=0)return std::nullopt;
 constexpr uint64_t factors[]={1,10,100,1000};auto f=factors[v.subtype];
 if(uint64_t(v.amount)>std::numeric_limits<uint64_t>::max()/f)return std::nullopt;
 return uint64_t(v.amount)*f;
}
} // namespace Housing::EscrowWire
#endif
