#ifndef EQEMU_HOUSING_LAYOUT_CRATE_MEMBER_IDENTITY_H
#define EQEMU_HOUSING_LAYOUT_CRATE_MEMBER_IDENTITY_H
#include "layout_crate_identity.h"
#include "crate_member_scope_db.h"
#include "custody_codec.h"
namespace Housing::LayoutCrateMember {
// A contained item has its own custody/native item identity. The native source
// estate is the crate child, while its durable reservation retains the parent
// yard/room. Neither identity may be inferred from a packet GUID.
struct Record {
 uint64_t custody_id=0;uint32_t native_item_id=0;
 CrateMemberScope::Membership membership;
 LayoutCrate::Allocation container;
};
using Proof=std::shared_ptr<const Record>;
inline bool Valid(const Record& r){
 if(!r.custody_id||!r.native_item_id||r.native_item_id==UINT32_MAX||!r.container)return false;
 const auto& m=r.membership;const auto& c=*r.container;const auto& b=c.binding;
 try{CrateBinding::EncodeAllocation(c);}catch(...){return false;}
 if(!m.crate_id||!m.revision||m.crate_id!=b.crate_id||m.origin_native_estate_id!=b.parent_native_estate_id||
    m.origin_pose.size()!=28||c.packed||c.inventory_character_id||c.inventory_slot_id||!b.custody_id||b.custody_id==r.custody_id)return false;
 for(size_t i=0;i<7;++i){uint32_t bits=0;for(size_t j=0;j<4;++j)bits|=uint32_t(m.origin_pose[4*i+j])<<(8*j);float v;std::memcpy(&v,&bits,4);if(!std::isfinite(v)||(i==3&&!(v>0)))return false;}
 return true;
}
inline CustodyCodec::Bytes Encode(const Record& r){
 if(!Valid(r))throw std::invalid_argument("Invalid layout crate member proof");
 CustodyCodec::Bytes b;CustodyCodec::W(b,1,4);CustodyCodec::W(b,r.custody_id,8);CustodyCodec::W(b,r.native_item_id,4);
 const auto& m=r.membership;CustodyCodec::W(b,m.crate_id,8);CustodyCodec::W(b,m.revision,8);CustodyCodec::W(b,m.origin_native_estate_id,4);CustodyCodec::W(b,m.origin_house_id,8);CustodyCodec::W(b,m.origin_stowed,1);b.insert(b.end(),m.origin_pose.begin(),m.origin_pose.end());
 const auto container=CrateBinding::EncodeAllocation(*r.container);CustodyCodec::W(b,container.size(),4);b.insert(b.end(),container.begin(),container.end());return b;
}
inline Proof Decode(const CustodyCodec::Bytes& b){
 CustodyCodec::Reader reader{b};if(reader.N(4)!=1)throw std::invalid_argument("Unknown layout crate member version");auto r=std::make_shared<Record>();
 r->custody_id=reader.N(8);r->native_item_id=reader.N(4);auto& m=r->membership;m.crate_id=reader.N(8);m.revision=reader.N(8);m.origin_native_estate_id=reader.N(4);m.origin_house_id=reader.N(8);auto stowed=reader.N(1);if(stowed>1)throw std::invalid_argument("Invalid origin state");m.origin_stowed=stowed;for(size_t i=0;i<28;++i)m.origin_pose.push_back(reader.N(1));
 auto c=CrateBinding::DecodeAllocation(reader.Blob(512));if(!c)throw std::invalid_argument("Invalid layout container proof");r->container=std::make_shared<const CrateBinding::Allocation>(std::move(c.value));
 if(reader.at!=b.size()||!Valid(*r)||Encode(*r)!=b)throw std::invalid_argument("Invalid layout crate member bytes");return r;
}
inline bool Same(const Proof& a,const Proof& b){return (!a||!b)?!a&&!b:Encode(*a)==Encode(*b);}
// Comparison for the original retained origin only. Callers must separately
// prove every intervening container revision through the ordered journal.
inline bool SameOrigin(const Proof& a,const Proof& b){if(!a||!b)return !a&&!b;auto adjusted=*a;auto container=std::make_shared<CrateBinding::Allocation>(*a->container);container->binding.revision=b->container->binding.revision;adjusted.container=container;return Encode(adjusted)==Encode(*b);}
inline uint32_t SourceEstate(const Record& r){if(!Valid(r))throw std::invalid_argument("Invalid layout crate source");return r.container->binding.native_estate_id;}
inline bool Shape(const Record& r,const LiveItems::Stored& s){
 if(!Valid(r)||!s.placement||!s.placement->pose_revision||!s.stowed||!s.storage_revision||s.custody_id!=r.custody_id||
    s.placement->reservation.native_item_id!=r.native_item_id||s.placement->reservation.native_estate_id!=r.membership.origin_native_estate_id)return false;
 CustodyCodec::Bytes pose;const auto& p=s.placement->transform;for(float v:{p.heading,p.pitch,p.roll,p.scale,p.position[0],p.position[1],p.position[2]}){uint32_t bits;std::memcpy(&bits,&v,4);CustodyCodec::W(pose,bits,4);}return pose==r.membership.origin_pose;
}
// Preserve the membership returned by a fresh authorized Scene without doing
// SQL or mistaking the retained parent reservation for the native crate source.
// This is structural evidence only; the coordinator must recapture under locks.
inline Proof FromSnapshot(const LiveItems::Stored& member,const std::shared_ptr<const CrateMemberScope::Membership>& membership,const LayoutCrate::Allocation& container){
 if(!membership||!member.placement||!container)throw std::invalid_argument("Incomplete layout crate member snapshot");
 auto r=std::make_shared<Record>();r->custody_id=member.custody_id;r->native_item_id=member.placement->reservation.native_item_id;r->membership=*membership;r->container=container;
 if(!Shape(*r,member))throw std::invalid_argument("Invalid layout crate member snapshot");return r;
}
// Structural snapshot only: no lease/ACL authority, mutation, allocation,
// lifecycle or sends. Caller supplies fresh authorized member/container scenes
// and retains their locks for the complete layout transaction.
inline LiveItems::Result<Proof> CaptureUnderTransaction(Database& db,const LiveItems::Stored& member,const LiveItems::Stored& container){
 using Error=LiveItems::Error;try{
  auto tx=db.QueryDatabase(std::string("SELECT @@in_transaction"),false);if(!tx.Success())return {{},Error::DatabaseFailure};if(tx.RowCount()!=1||!tx.begin()[0]||std::string(tx.begin()[0])!="1")return {{},Error::InvalidArgument};
  auto membership=CrateMemberScope::Locked(db,member.custody_id,true);if(!membership)return {{},Error::NotFound};
  if(!member.placement)return {{},Error::Conflict};
  auto actual=db.QueryDatabase("SELECT p.native_item_id,p.pose_revision,HEX(p.pose_blob),s.revision,s.stowed,s.native_estate_id,c.estate_id,HEX(c.row_blob) FROM housing_live_item_custody c JOIN housing_live_placed_items p ON p.custody_id=c.custody_id JOIN housing_live_item_storage s ON s.custody_id=c.custody_id WHERE c.custody_id="+std::to_string(member.custody_id)+" FOR UPDATE",false);
  if(!actual.Success())return {{},Error::DatabaseFailure};if(actual.RowCount()!=1)return {{},Error::Conflict};auto a=actual.begin();for(size_t i=0;i<8;++i)if(!a[i])return {{},Error::Conflict};
  auto hex=[](const CustodyCodec::Bytes& bytes){std::string result;const char* digits="0123456789ABCDEF";for(auto v:bytes){result+=digits[v>>4];result+=digits[v&15];}return result;};
  if(std::stoull(a[0])!=member.placement->reservation.native_item_id||std::stoull(a[1])!=member.placement->pose_revision||std::string(a[2])!=hex(membership->origin_pose)||std::stoull(a[3])!=member.storage_revision||std::stoull(a[4])!=1||std::stoull(a[5])!=membership->origin_native_estate_id||std::stoull(a[6])!=member.estate_id||std::string(a[7])!=hex(CustodyCodec::Encode(member.row)))return {{},Error::Conflict};
  auto allocation=LayoutCrate::ReadUnderTransaction(db,container);if(!allocation)return {{},allocation.error};
  auto scope=HouseItemScope::Locked(db,member.custody_id);if((scope?scope->house_id:0)!=membership->origin_house_id)return {{},Error::Conflict};
  auto r=std::make_shared<Record>();r->custody_id=member.custody_id;r->native_item_id=member.placement?member.placement->reservation.native_item_id:0;r->membership=std::move(*membership);r->container=std::move(allocation.value);
  if(!Shape(*r,member))return {{},Error::Conflict};return {std::move(r),Error::None};
 }catch(const std::invalid_argument&){return {{},Error::Conflict};}catch(...){return {{},Error::DatabaseFailure};}
}
}
#endif
