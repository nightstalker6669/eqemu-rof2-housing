#ifndef EQEMU_HOUSING_STATIC_FURNITURE_REGISTRY_H
#define EQEMU_HOUSING_STATIC_FURNITURE_REGISTRY_H
#include "placement_policy.h"
#include <string>
namespace Housing::StaticFurniture {
// Immutable server-owned evidence table. Parsed candidates do not confer authority.
// Candidates await shared resource-class/scale and numeric-policy evidence.
// Membership is decided by the pinned parser/profile, not repeated native tests
// for each item. No packet or database flag can promote a candidate.
enum class Evidence { LegacyChairAccepted, OfflineCandidate, ReviewedStaticClass, ReviewedMountedStaticClass };
struct Record {
 uint32_t item_id,model_id;const char* model;
 PlacementPolicy::Geometry geometry;
 const char* archive;const char* archive_sha256;const char* member_sha256;
 Evidence evidence;uint32_t geometry_policy_revision;uint32_t mounting_type=0;
};
inline constexpr const char* ClientSha256="2a8702ad9f722704f01355c0750be7d6f164a8b9c9128ba0cf286ea32b405b0e";
inline constexpr const char* RendererSha256="ade556b68e98f6f39c5578f70431e11c0b9babcc15b8571eb22cf1e2b654417e";
inline constexpr const char* ManifestSha256="213cf4a17de6676490f9da97afe330cfb4a317ca156089f398c9521c346c7619";
inline constexpr const char* ActorTagSha256="985ae82cbc2937a6d723e7a662e7b04eb4e9b77a524d5fbd326b02888e9b5f54";
inline constexpr const char* MountedInputSha256="54fb20a1808e69a14d877a061b8bd53365a7e2c71c4aec108e4b597c4054ea49";
inline constexpr const char* ClassInputSha256="64b615f30774940af2d0c1fa10541564a61c728b5cb85b45bc10127652df751d";
inline constexpr const char* RigidExtensionInputSha256="cae2168ffd4f872bcd3953768f9f558e00f943a9243b120d2a73096456f4d677";
inline constexpr Record Records[]={
 {54573,23240,"IT23240",{23240,{0.0,0.00593218207359314,1.999120831489563},{0.8187458515167236,0.7710634171962738,1.999120831489563},true,3.973643e-7},"pfpremium01.eqg","bb7227361b55a0f12b3630c17b88d6d9d38e68b27947518ffdc4962d8a25366d","7102e1e5b903acc0b00ec2675a2aabf2cf9cbf216521b90481425af59c119da3",Evidence::LegacyChairAccepted,1},
#include "static_furniture_class.inc"
#include "static_furniture_rigid_extension.inc"
#include "static_furniture_personal_trophies.inc"
#include "anchor_geometry.inc"
#include "static_furniture_mounts.inc"
};
// Preserve the original chair v1 geometry bytes: candidate float32 rounding is
// deliberately not substituted into an existing durable content identity.
inline const Record* Candidate(uint32_t id){for(const auto& r:Records)if(r.item_id==id)return &r;return nullptr;}
// Narrow pinned original anchor profile. This permits geometry/placement only;
// it grants neither teleport authorization nor a trophy benefit.
inline bool AnchorItem(uint32_t id){return id==50874||id==52584||id==52585;}
inline bool NativeProfile(uint32_t id,uint32_t mask,uint32_t ignore,uint32_t definition,
 float minimum,float maximum,uint32_t upkeep,int32_t max_per_estate){
 if(id==68727||id==68730)return mask==12&&!ignore&&!definition&&
  minimum==0.10000000149011612f&&maximum==2.5f&&upkeep==4&&max_per_estate==1;
 if(!AnchorItem(id))return !definition&&!ignore;
 return mask==(id==50874?130u:142u)&&ignore==1&&!definition&&
  minimum==0.10000000149011612f&&maximum==2.0f&&!upkeep&&max_per_estate==-1;
}
inline bool PlacementTypeMatches(uint32_t id,uint32_t type){const auto* r=Candidate(id);return r&&r->mounting_type==type;}
inline bool Mounted(uint32_t id){const auto* r=Candidate(id);return r&&r->evidence==Evidence::ReviewedMountedStaticClass&&r->geometry_policy_revision==3;}
inline bool Enabled(const Record& r){return r.geometry.native_collision_verified&&((r.evidence==Evidence::LegacyChairAccepted&&r.geometry_policy_revision==1)||(r.evidence==Evidence::ReviewedStaticClass&&r.geometry_policy_revision==2)||(r.evidence==Evidence::ReviewedMountedStaticClass&&r.geometry_policy_revision==3));}
inline const Record* Lookup(uint32_t id){const auto* r=Candidate(id);return r&&Enabled(*r)?r:nullptr;}
inline const Record* Lookup(uint32_t id,const std::string& model){const auto* r=Lookup(id);return r&&model==r->model?r:nullptr;}
} // namespace Housing::StaticFurniture
#endif
