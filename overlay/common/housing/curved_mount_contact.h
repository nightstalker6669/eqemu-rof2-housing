#ifndef EQEMU_HOUSING_CURVED_MOUNT_CONTACT_H
#define EQEMU_HOUSING_CURVED_MOUNT_CONTACT_H
#include <cstdint>
#include <vector>
namespace Housing::CurvedMount {
struct Entry {uint32_t id;uint8_t side_mask,native_mode_mask;};

struct Limits {uint32_t source=65536,component=4096,relevant=128,corridors=8;float search_margin=2;};
enum class Error {None,Invalid,Source,TooLarge,MissingSeed,Coverage,Fold,NotCurved,Collision};
struct Proof {Error error=Error::Invalid;std::vector<uint32_t> support_ids,collision_exempt_ids;explicit operator bool()const{return error==Error::None;}};
}
#endif
