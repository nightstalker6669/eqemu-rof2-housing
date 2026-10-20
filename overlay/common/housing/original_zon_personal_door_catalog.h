#ifndef EQEMU_HOUSING_ORIGINAL_ZON_PERSONAL_DOOR_CATALOG_H
#define EQEMU_HOUSING_ORIGINAL_ZON_PERSONAL_DOOR_CATALOG_H
#include "original_mesh_sidecar.h"
namespace Housing::OriginalZonStaticActor {
using Point=OriginalSphereQuery::Point;
struct Profile {std::string_view zone,zon_sha256;OriginalHeightScene::MeshCatalogEntry mesh;uint32_t placement_index;Point raw_position,raw_angles;float scale;uint32_t vertices=204,polygons=112;bool raw_low16_zero=true;};
constexpr float F(uint32_t b){return std::bit_cast<float>(b);}
inline constexpr std::array<Profile,6> PersonalDoors{{
{"phinterior3a1","6a50b95f20c88870e132b2e40fc907addddf1b3066ecade9a1afeaf0d9a69112",{0,"OBJ_DOOR_PLHA","7e4d87ef79ae782d14795fed85ba8d4a75502e11d7bd38ecea9c29277354c585","7bb719cfb1301d17dfbeab4888b42385bbb6f12ef758c797fe57f2cb24a601f1","d6a5e2ae93f5932bca08fd7cbfe2047a3164df111799d27cd0faefc69558c289"},1,{F(0x4095d4fcu),F(0x4036fe88u),F(0xc168111cu)},{F(0xbfc90fdbu),F(0x00000000u),F(0x00000000u)},F(0x3f800000u)},
{"phinterior3a2","1b646dc15140d446067efff9d5334d41044df0471119350c53670a6e355fba8c",{0,"OBJ_DOOR_PLHA","c910100ab7e0453fc193b10fe646fbba9f454e1434277b36470e1cf9b4ca8384","68ce4785a02b54b1e2d153378b32ef22914af148b9caaf0bfdf2c77e4dc8bfbb","dce22e6af708b04b469e06d89aae18ba0b0106f711b20782522def4512bc4ce5"},1,{F(0x4095d4fcu),F(0x4036fe88u),F(0xc168111cu)},{F(0xbfc90fdbu),F(0x00000000u),F(0x00000000u)},F(0x3f800000u)},
{"phinterior3a3","12b899113198ce6c72a8998a461b6e50595cdbc4742ab37eb48477a2567b6f90",{0,"OBJ_DOOR_PLHA","58ade22b17da27449760f901654e1b43a1dfb1dbecc7dd2018ef9ae749bfaeb7","45b81431734c0babbf4c22c45bf7e21dbd684f2c7b871bf7ab125f7fefd5e9e2","37fa4928d3aa364e4ecd0f50dd07553173115e0d1fa6da7efc593be24c064ec5"},1,{F(0x4095d4fcu),F(0x4036fe88u),F(0xc168111cu)},{F(0xbfc90fdbu),F(0x00000000u),F(0x00000000u)},F(0x3f800000u)},
{"phinterior1a1","975dab599e0b3539d9727bfcce37edfd3f7f376d999993bd2d77b82dab6a9e6a",{0,"OBJ_DOOR_PLHA","4190c9966304a9ebb824569260628bb13f51235b4a7bacdc5a63cad06c5c5657","202d7fb69594fa8dc1b1ca1c5699e99587465a85ae525068bae685b9d67efc21","0546046e94ed1180e611020f82929dec37423e5e59ccbc88722c81f87f09ce41"},1,{F(0xc04d0c80u),F(0x40974bb0u),F(0xc168111cu)},{F(0xbfc90fdbu),F(0x00000000u),F(0x00000000u)},F(0x3f800000u)},
{"phinterior1a2","8fc5fae1caa466be6d37374f113814a15f27f1890f19c0d56f3fd2e7b85e43df",{0,"OBJ_DOOR_PLHA","cb02813a8c6c6a0dcc129d5b23fe6a36a4be239fa27ddc0b6dc4de91821d13fb","a44a450f5e20377a045ab564297eca47a766da51a9f90b17566436631e009beb","e1935be7efbd53d29696665af5999015456e2acb7e93944117d20a4aeb8cffad"},1,{F(0xc04d0c80u),F(0x40974bb0u),F(0xc168111cu)},{F(0xbfc90fdbu),F(0x00000000u),F(0x00000000u)},F(0x3f800000u)},
{"phinterior1a3","1011ca11aa5ad7d4d4666554e2de317d75cc6b2cbe50c722fa4ffd65f5e0cf05",{0,"OBJ_DOOR_PLHA","ba1dfd803c53728883cb24e80162614e8c2722dc94debe1d9703fcf03189b0b4","9d44db9ddf3df695af6ca72d97fad2c784df315da6de65d5daa7ce96fab83c01","1215fd0654d699b2d563b4251aac38ea63cb04697c0ba92cf50ea639a87a1ebe"},1,{F(0xc04d0c80u),F(0x40974bb0u),F(0xc168111cu)},{F(0xbfc90fdbu),F(0x00000000u),F(0x00000000u)},F(0x3f800000u)},
}};
}
#endif
