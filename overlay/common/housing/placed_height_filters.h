#ifndef EQEMU_HOUSING_PLACED_HEIGHT_FILTERS_H
#define EQEMU_HOUSING_PLACED_HEIGHT_FILTERS_H
#include <cstdint>
#include <cmath>
#include <optional>
#include <string_view>
// Original ordinary558030 contexttype9 with a verified nonnull
// CPlacedItem/AsPlacedItem(). Predicates only; no collision-source collection.
namespace Housing::PlacedHeightFilters {
enum class Decision {Exclude,Include,Unknown};
struct Switch {uint8_t type{};std::string_view name;};
struct Actor {
 // Explicit unknowns MUST make the adapter fail closed, not silently omit a
 // potential blocker. A known null application pointer is optional{0}.
 std::optional<uint64_t> application_identity;
 std::optional<uint32_t> type,restriction_mask,volume;
 std::optional<float> radius;
 std::optional<Switch> switch_data;
};
inline bool FloorSwitch(uint32_t encoded_zone,uint8_t type,std::string_view name){
 // Exact50ff80..510160; domain: valid positive native zone/extended zone IDs.
 if((type>=153&&type<=155)||(type>=30&&type<=45)||(type>=54&&type<=59)||(type>=50&&type<=51))return true;
 const uint32_t zone=encoded_zone>767?encoded_zone&65535:encoded_zone;
 switch(zone){
 case 154:return type==27;
 case 205:case 212:return type==26;
 case 208:return type==8;
 case 402:return name=="OBJ_SHADE_ICESTAT"||name=="OBJ_SHADE_SECRET";
 case 404:return name=="OBJ_VM_CRYPTDOORA";
 case 419:return name=="OBJ_ZONEIN"||name=="OBJ_LIBTELPORTPAD";
 case 421:return name=="OBJ_SUNDIAL_PAD";
 default:return false;
 }
}
inline Decision FloorPrimary(const Actor&a,uint64_t verified_placed_context,uint32_t zone){
 if(!verified_placed_context||!a.type||!a.application_identity)return Decision::Unknown;
 //476296..2c7: placed self check then context.AsPlacedItem earlyinclude.
 if(*a.type==9)return *a.application_identity==verified_placed_context?Decision::Exclude:Decision::Include;
 if(!a.restriction_mask)return Decision::Unknown;
 if(*a.restriction_mask&1)return Decision::Exclude;
 // No player-context exclusions: real CPlacedItem.AsPlayerBase is null.
 if(*a.application_identity){
  if(*a.type==4)return Decision::Exclude;
  if(*a.type==3){if(!a.switch_data||!zone||zone>0x7fffffff)return Decision::Unknown;return FloorSwitch(zone,a.switch_data->type,a.switch_data->name)?Decision::Include:Decision::Exclude;}
 }
 if(!a.volume)return Decision::Unknown;
 //476382..39f FUCOMPP0,radius; TESTAH44; JP excludes nonzero AND unordered.
 if(*a.volume==0||*a.volume==1)return Decision::Include;
 if(!a.radius)return Decision::Unknown;
 return *a.radius==0?Decision::Include:Decision::Exclude;
}
inline Decision CeilingPrimary(const Actor&a,uint64_t verified_placed_context){
 if(!verified_placed_context||!a.type)return Decision::Unknown;
 //4764df..516 overrides ordinary volume/switch rules for other placed actors.
 if(*a.type==9){if(!a.application_identity)return Decision::Unknown;return *a.application_identity&&*a.application_identity==verified_placed_context?Decision::Exclude:Decision::Include;}
 if(!a.volume)return Decision::Unknown;
 return *a.volume==2||*a.type==3?Decision::Exclude:Decision::Include;
}
//475f40 reduces to this for contexttype9;4760a0 is this for every context.
// Domain: real nonnull graphics actor. D1FD8C's resource identity is unnamed.
inline Decision Secondary(std::optional<uint64_t> actor,std::optional<uint64_t> excluded_graphics_global){
 if(!actor||!*actor||!excluded_graphics_global)return Decision::Unknown;
 return *actor!=*excluded_graphics_global?Decision::Include:Decision::Exclude;
}
// The caller supplies EXACT original dot(delta,normal) with nativePC53/chop
// operation order, not an arbitrary hostfloat dot. Missing sourceflags refuses.
inline Decision TerrainTriangle(std::optional<uint16_t> source_flags,bool query60,bool query61,bool option9c,
 bool nonzero_motion,double native_delta_dot_normal,float normal_z){
 if(!source_flags||!std::isfinite(native_delta_dot_normal)||!std::isfinite(normal_z))return Decision::Unknown;
 if((*source_flags&1)&&!query61)return Decision::Exclude;
 if((*source_flags&2)&&query60)return Decision::Exclude;
 if(nonzero_motion&&native_delta_dot_normal>0)return Decision::Exclude;
 if(option9c&&normal_z==0)return Decision::Exclude;
 return Decision::Include;
}
}
#endif
