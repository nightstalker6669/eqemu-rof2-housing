#ifndef EQEMU_HOUSING_ORIGINAL_TERRAIN_SOURCE_H
#define EQEMU_HOUSING_ORIGINAL_TERRAIN_SOURCE_H
#include "original_mesh_sidecar.h"
namespace Housing::OriginalHeightScene {
struct TerrainCatalogEntry {uint32_t zone_id,vertices,polygons;std::string_view zone,zon_sha256,world_frame_sha256;MeshCatalogEntry mesh;};
// Reviewed original external EQGZ-v2 / EQGT-v3 path:10066230 loads original
// EQGT through10065c00/10063f10.100620f0 only enumerates material/vertex arrays;
// CTerrain10020aa0 copies XYZ unchanged and A,C,B with source low16 flags.
// The later externalZON10065430 SKIPS first terrain placement; its authored
// transform is not applied to terrain. Do not substitute azone triangles.
inline constexpr std::array<TerrainCatalogEntry,13> OriginalTerrainCatalog{{
{714,6540,4935,"phinterior3a1","6a50b95f20c88870e132b2e40fc907addddf1b3066ecade9a1afeaf0d9a69112","fbd6a4de7dfcce1a4686443ef30cc802bf39405a6454504e0ba858b832504a2c",{0,"ter_plhinterior1.ter","6571467569e3be5d5828b02665a17c4bc4a5b6e30cf952e3de2e202a1da3759d","7bb719cfb1301d17dfbeab4888b42385bbb6f12ef758c797fe57f2cb24a601f1","c3eeae898a9ee3239f47e845373b6b617f684a768d9a92546c03bba3215db36b"}},
{716,7815,4935,"phinterior3a2","1b646dc15140d446067efff9d5334d41044df0471119350c53670a6e355fba8c","d6547c239886477fb3b123b1dd37d3bf86d7ea03446c613edef0b39cafe98229",{0,"ter_phinterior3a2.ter","e1e3595ea2e2f36885031bb73396f28ce374fe519812444e511d3bd147b15aba","68ce4785a02b54b1e2d153378b32ef22914af148b9caaf0bfdf2c77e4dc8bfbb","c574c773c443cd205fc5db7f4f44b4ea1914971e9831b68d45ea042ef6624ca7"}},
{717,7371,4935,"phinterior3a3","12b899113198ce6c72a8998a461b6e50595cdbc4742ab37eb48477a2567b6f90","20c91d96537279ce91925786e5b4bdc590d5399fec3e8b4cf45fe5106f742596",{0,"ter_phinterior3a3.ter","380eb62c362cdc7c6f8fb609ff7bc7d32a3cd5dab32a9f0ca273c848237cb5ab","45b81431734c0babbf4c22c45bf7e21dbd684f2c7b871bf7ab125f7fefd5e9e2","3697c9b8975bb6c5ce155ef419bbe77e9a3d48806ec47ab5302623591ce635f5"}},
{715,12142,7948,"phinterior1a1","975dab599e0b3539d9727bfcce37edfd3f7f376d999993bd2d77b82dab6a9e6a","ba0c292d576560020d76343e9a02aeebf4cb36805fb7152fb1fbc42e258b00fa",{0,"ter_phinterior1a1.ter","a9f9a6b63f980324c9b07eed90fa6e0b13aae5a1624ed9167795613c263636ad","202d7fb69594fa8dc1b1ca1c5699e99587465a85ae525068bae685b9d67efc21","8ccb7f35b4acff39eaf910c86eb77bf2704775a5427c4577359d50a0273d4f6d"}},
{718,12337,7948,"phinterior1a2","8fc5fae1caa466be6d37374f113814a15f27f1890f19c0d56f3fd2e7b85e43df","04315f44ab7584d67b917608cad2b81238a46493e7732768f44a2fd1c4a20f24",{0,"ter_phinterior1a2.ter","099f9ab23ee3afd07d530e9cf611044da6c15ab0c37b95c02720f34e8afa4b63","a44a450f5e20377a045ab564297eca47a766da51a9f90b17566436631e009beb","762ee1e54c5e569e1ab65a0048cce732c7dc599df2307f1f458873005f6e8ea2"}},
{719,12426,7948,"phinterior1a3","1011ca11aa5ad7d4d4666554e2de317d75cc6b2cbe50c722fa4ffd65f5e0cf05","92c003c71624bd8c84746e888bf06b8c7f179ec7f787b3f96505fbd593d8c269",{0,"ter_phinterior1a3.ter","6c8a2017d06a8405cb4d2bb6400116b4c8d217c6ccdc22d24c23d586058f3cc3","9d44db9ddf3df695af6ca72d97fad2c784df315da6de65d5daa7ce96fab83c01","33545533adb7502c01a9c0a83f1159316f05464e8506d47acdbd33d7edd65565"}},
{345,28584,13803,"guildhall","b789663e437b323aaef44a35568fe5c0683b431f35bc706407da92d963466d9e","eff18eb195ec570f62eed2497529c47bb96431c7e2f94a2f8648d67acba12464",{0,"ter_guildhall.ter","ace813cd1d20836aa3eb0c4d740d2aad06069ab15030cd27cccbe36fce1f6d1a","086b30c6236546054fde24efea0274b0c34e561320935809afa74a9a03182416","2b09c3d90a6a0fcc6a25353a0d0582cf0cf1729c1e4230aed030111ab805ee38"}},
{738,12612,7854,"guildhallsml","c2d721bc975db78b20586366eb69bca160b12fef50541cc9285e8267d6279e3d","baefe3746ef9f32d0426b14f9feca2d50addb38a2ef8e5d012959d90b9d8ca40",{0,"ter_entry_room.ter","2041335411b4e2426b1a90521fc38703bd1e56e0ff39497c7b54a2e3739eebe1","f33bc06d3415b07e8745bea7d7802297b3049ce7aa4d4ccdc3f3d45f9942daef","cc399428a2706b1ed1773e7a823afd5bbb015cba327610aca1e2d414ca9cc38e"}},
{737,100636,50542,"guildhalllrg","6e89264f72245f79b062d206fb3cfa79599c26c523ec0fe568c1101a321e4029","3f7b8c0dbde496a4a802930aceb39694650305bafdeb019cdafc985acfdc90fa",{0,"ter_centerrm.ter","4c18ed312bad5cc547b970494d27ff8235c6d3d09d5ca342fc6246469317abc5","66840bd528fe9a214e8d3029399cd24c80687773f67026967d5508ac5d2ab6da","c394dea220a74d2960b22633c7924c4e4093269689312835ed5d1c8e504e7377"}},
{751,8378,5264,"guildhall3","f8c8703ff3288e2350bb394d1c52b06dbd4392d852aca5f21f27227371651240","592667a7b6ff715dbb6a55532109b191db2efcbd82a6278a53678ebaead47f2d",{0,"ter_guildhall3.ter","8731c7bdf261b4b428d1f077b4e7ccc54a9f5f9f1f4aeef7017cfb4b92fa500f","fe3ecd056c2b199ad26c1ea3d7a06bac48be827a030e2150ebe7cc732826a0b8","c5c1b9a8dd09b0db76e4f49c36fa377b2a852aacbbbe5c41a436232a4e189bf2"}}
#include "original_variant_terrain.inc"
}};
// Original-owned private immutable sidecar only. Compiled metadata authenticates
// source bytes and exact world-frame convention. Caller separately authenticates
// current zone resource/disabled state and complete immutable scene generation.
// This does NOT invent alternate-field absence, dynamic-source metadata or
// scene.complete from housing rows. A terrain-only list cannot claim that.
inline std::shared_ptr<const Source> ReadTerrainSource(uint32_t zone_id,
 std::string_view current_zon_sha256,const Entry& entry,std::span<const uint8_t> bytes){
 if(!entry.terrain||!entry.disabled||*entry.disabled||!entry.identity.epoch||!entry.identity.actor)return {};
 const TerrainCatalogEntry* catalog=nullptr;for(const auto& candidate:OriginalTerrainCatalog)if(candidate.zone_id==zone_id){if(catalog)return {};catalog=&candidate;}
 if(!catalog||current_zon_sha256!=catalog->zon_sha256||entry.identity.resource_sha256!=catalog->mesh.member_sha256||entry.identity.frame_sha256!=catalog->world_frame_sha256||entry.expected_manifest_sha256!=catalog->mesh.mesh_sha256)return {};
 auto mesh=ReadMeshSidecar(bytes,catalog->mesh);if(!mesh||mesh->vertices.size()!=catalog->vertices||mesh->polygons.size()!=catalog->polygons)return {};
 try{auto result=std::make_shared<Source>();result->identity=entry.identity;result->manifest_sha256=entry.expected_manifest_sha256;result->polygons=std::move(mesh->polygons);result->dispatch=Source::Dispatch::Terrain100bda70;return result;}catch(...){return {};}
}
}
#endif
