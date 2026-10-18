#ifndef EQEMU_HOUSING_TRANSFER_CONSENT_WIRE_H
#define EQEMU_HOUSING_TRANSFER_CONSENT_WIRE_H
#include <array>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
namespace Housing::TransferConsentWire {
inline constexpr size_t Size=188;
using Body=std::array<uint8_t,Size>;
// Original5ad760 normalizes three fixed text blocks. Generate canonical NUL
// terminated blocks. The five opaque scalar DWORDs carry OUR offer nonce;
// this is an emulator correlation policy, not recovered original field meaning.
inline std::optional<Body> Offer(const Body& request,const std::string& sender,const std::array<uint8_t,20>& nonce){
 if(sender.empty()||sender.size()>=64)return {};
 Body body=request;
 std::fill(body.begin()+12,body.begin()+108,0);
 std::copy(sender.begin(),sender.end(),body.begin()+12);
 const size_t offsets[]={4,8,172,176,180};
 for(size_t i=0;i<5;++i)std::copy_n(nonce.begin()+i*4,4,body.begin()+offsets[i]);
 body[184]=body[185]=body[186]=body[187]=0;
 return body;
}
inline std::optional<bool> Reply(const Body& offer,const uint8_t* bytes,size_t size){
 if(!bytes||size!=Size||bytes[185]>1)return {};
 // Native185 is rewritten only when the estate cache lookup succeeds. A
 // missing estate echoes our zero and safely declines.187 is heap padding.
 for(size_t i=0;i<187;++i)if(i!=185&&bytes[i]!=offer[i])return {};
 return bytes[185]==1;
}
}
#endif
