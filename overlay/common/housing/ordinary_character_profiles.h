#ifndef EQEMU_HOUSING_ORDINARY_CHARACTER_PROFILES_H
#define EQEMU_HOUSING_ORDINARY_CHARACTER_PROFILES_H
#include <cstdint>
#include <string_view>
namespace Housing::OrdinaryCharacterProfiles {
// Original classifier50a440/name50a370, model-height5a3f40 and correction59efd0.
// These exact ordinary player/Eye and TPN marker classes have classifierbit0 clear. Original
// positive ROffset and all model option multipliers preserve positive height;
// correction either returns unchanged, multiplies by positive0.5 or fixesEye3.
// Current ordinary codec/publication/appearance epoch is independently required.
inline constexpr std::string_view ClientSha="2a8702ad9f722704f01355c0750be7d6f164a8b9c9128ba0cf286ea32b405b0e";
inline constexpr std::string_view ModdatSha="aa9c7b337de23630f2ee78d442c619453efe6b500b01616d9a561474c6561ec8";
struct Profile {uint32_t race,gender,flags,roffset_bits,correction;std::string_view model;};
inline constexpr Profile Profiles[]={
 {2,1,32,0x40480000u,0x59f116u,"BAF"},
 {2,0,32,0x40480000u,0x59f116u,"BAM"},
 {6,1,32,0x40480000u,0x59f116u,"DAF"},
 {6,0,32,0x40480000u,0x59f116u,"DAM"},
 {522,1,40,0x40480000u,0x59f116u,"DKF"},
 {522,0,40,0x40480000u,0x59f116u,"DKM"},
 {8,1,32,0x40480000u,0x59f116u,"DWF"},
 {8,0,32,0x40480000u,0x59f116u,"DWM"},
 {4,1,32,0x40480000u,0x59f116u,"ELF"},
 {4,0,32,0x40480000u,0x59f116u,"ELM"},
 {3,1,36,0x40480000u,0x59f116u,"ERF"},
 {3,0,36,0x40480000u,0x59f116u,"ERM"},
 {108,2,0,0x40480000u,0x59f038u,"EYE"},
 {330,1,32,0x40480000u,0x59effdu,"FRF"},
 {330,0,32,0x40480000u,0x59effdu,"FRM"},
 {12,1,32,0x40480000u,0x59f116u,"GNF"},
 {12,0,32,0x40480000u,0x59f116u,"GNM"},
 {7,1,32,0x40480000u,0x59f116u,"HAF"},
 {7,0,32,0x40480000u,0x59f116u,"HAM"},
 {5,1,32,0x40480000u,0x59f116u,"HIF"},
 {5,0,32,0x40480000u,0x59f116u,"HIM"},
 {11,1,32,0x40480000u,0x59f116u,"HOF"},
 {11,0,32,0x40480000u,0x59f116u,"HOM"},
 {1,1,36,0x40480000u,0x59f116u,"HUF"},
 {1,0,36,0x40480000u,0x59f116u,"HUM"},
 {128,1,32,0x40480000u,0x59f116u,"IKF"},
 {128,0,32,0x40480000u,0x59f116u,"IKM"},
 {130,1,32,0x40480000u,0x59f116u,"KEF"},
 {130,0,32,0x40480000u,0x59f116u,"KEM"},
 // Original registration50b382: neutral model TPN, flags0; no moddat ROffset override.
 {240,2,0,0x40480000u,0x59f116u,"TPN"},
 {10,1,32,0x40480000u,0x59f116u,"OGF"},
 {10,0,32,0x40480000u,0x59f116u,"OGM"},
 {9,1,32,0x40480000u,0x59f116u,"TRF"},
 {9,0,32,0x40480000u,0x59f116u,"TRM"},
};
inline const Profile* Find(uint32_t race,uint32_t gender){const Profile*found=nullptr;for(const auto&p:Profiles)if(p.race==race&&p.gender==gender){if(found)return nullptr;found=&p;}return found;}
}
#endif
