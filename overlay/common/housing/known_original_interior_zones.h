#ifndef EQEMU_HOUSING_KNOWN_ORIGINAL_INTERIOR_ZONES_H
#define EQEMU_HOUSING_KNOWN_ORIGINAL_INTERIOR_ZONES_H
#include <cstdint>
#include <string_view>
namespace Housing::OriginalInteriors {
// Verified subset of original eqgame.exe registrations, not a complete room
// inventory, SKU mapping, spawn/exit review or room-content approval. Additional
// registrations require original-client evidence before extending this table.
// Evidence: housing/content/original-interior-zone-ids.json, 7dc430/7dc290.
inline constexpr std::string_view ClientSha256="2a8702ad9f722704f01355c0750be7d6f164a8b9c9128ba0cf286ea32b405b0e";
struct Registration {uint32_t zone_id;const char* short_name;};
inline constexpr Registration Known[]={
 {345,"guildhall"},
 {714,"phinterior3a1"},
 {715,"phinterior1a1"},
 {716,"phinterior3a2"},
 {717,"phinterior3a3"},
 {718,"phinterior1a2"},
 {719,"phinterior1a3"},
 {720,"phinterior1b1"},
 {723,"phinterior1d1"},
 {737,"guildhalllrg"},
 {738,"guildhallsml"},
 {739,"plhogrinteriors1a1"},
 {740,"plhogrinteriors1a2"},
 {741,"plhogrinteriors3a1"},
 {742,"plhogrinteriors3a2"},
 {743,"plhogrinteriors3b1"},
 {744,"plhogrinteriors3b2"},
 {745,"plhdkeinteriors1a1"},
 {746,"plhdkeinteriors1a2"},
 {747,"plhdkeinteriors1a3"},
 {748,"plhdkeinteriors3a1"},
 {749,"plhdkeinteriors3a2"},
 {750,"plhdkeinteriors3a3"},
 {751,"guildhall3"},
 {766,"phinteriortree"},
};
inline bool Matches(uint32_t zone_id,std::string_view short_name){
 for(const auto& r:Known)if(r.zone_id==zone_id)return short_name==r.short_name;
 return false;
}
}
#endif
