#ifndef EQEMU_HOUSING_ORIGINAL_MESH_SIDECAR_H
#define EQEMU_HOUSING_ORIGINAL_MESH_SIDECAR_H
#include "original_height_scene_collector.h"
#include "original_mesh_catalog.h"
#include <openssl/evp.h>
namespace Housing::OriginalHeightScene {
struct LocalMesh {std::vector<Point> vertices;std::vector<Polygon> polygons;};
// Bounded immutable sidecar bytes already read from a trusted server-only path.
// SHA is pinned by reviewed compiled metadata, not supplied by the peer/client.
inline std::optional<LocalMesh> ReadMeshSidecar(std::span<const uint8_t> bytes,const MeshCatalogEntry& expected){
 if(bytes.size()<80||bytes.size()>64*1024*1024||!Sha(expected.mesh_sha256)||!Sha(expected.archive_sha256)||!Sha(expected.member_sha256))return {};
 std::array<unsigned char,32> digest{};unsigned n=0;if(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&n,EVP_sha256(),nullptr)!=1||n!=32)return {};
 auto Hex=[](std::span<const unsigned char> data){constexpr char abc[]="0123456789abcdef";std::string s;for(auto b:data){s+=abc[b>>4];s+=abc[b&15];}return s;};
 if(Hex(digest)!=expected.mesh_sha256||std::string_view(reinterpret_cast<const char*>(bytes.data()),8)!="EQHMSH01"||Hex(bytes.subspan(16,32))!=expected.archive_sha256||Hex(bytes.subspan(48,32))!=expected.member_sha256)return {};
 auto u32=[&](size_t at){return uint32_t(bytes[at])|(uint32_t(bytes[at+1])<<8)|(uint32_t(bytes[at+2])<<16)|(uint32_t(bytes[at+3])<<24);};
 const auto nv=u32(8),np=u32(12);if(!nv||nv>1000000||!np||np>1000000||uint64_t(80)+uint64_t(nv)*12+uint64_t(np)*16!=bytes.size())return {};
 try{LocalMesh out;out.vertices.reserve(nv);out.polygons.reserve(np);size_t at=80;
 for(uint32_t i=0;i<nv;++i){Point p;for(unsigned k=0;k<3;++k)p[k]=std::bit_cast<float>(u32(at+4*k));if(!OriginalSphereSource::Finite(p))return {};out.vertices.push_back(p);at+=12;}
 for(uint32_t i=0;i<np;++i){Polygon p;p.index=i;for(unsigned k=0;k<3;++k){auto index=u32(at+4*k);if(index>=nv)return {};p.native_abc[k]=out.vertices[index];}p.flags=uint16_t(bytes[at+12])|(uint16_t(bytes[at+13])<<8);if(bytes[at+14]||bytes[at+15])return {};out.polygons.push_back(p);at+=16;}
 return out;}catch(...){return {};}
}
// Deliberately no guessed Euler/azone transform or "already world" boolean.
// Native collision source frame/dispatch must be resolved and authenticated by
// a reviewed implementation before LocalMesh can become Source for Collect().
}
#endif
