#ifndef EQEMU_HOUSING_ORIGINAL_SIMPLE_TREE_SIDECAR_H
#define EQEMU_HOUSING_ORIGINAL_SIMPLE_TREE_SIDECAR_H
#include "original_simple_tree_query.h"
#include "original_simple_tree_catalog.h"
namespace Housing::OriginalSimpleTreeQuery {
inline bool Same(float a,float b){return std::bit_cast<uint32_t>(a)==std::bit_cast<uint32_t>(b);}
// Both private sidecars are authenticated against compiled reviewed catalogs.
// A caller-provided LocalMesh without its content digest is insufficient.
inline std::optional<Tree> ReadTreeSidecar(std::span<const uint8_t> bytes,const CatalogEntry& expected,std::span<const uint8_t> mesh_bytes){
 if(bytes.size()<116||bytes.size()>2*1024*1024||!OriginalHeightScene::Sha(expected.sidecar_sha256)||!OriginalHeightScene::Sha(expected.mesh_sha256)||!OriginalHeightScene::Sha(expected.canonical_tree_sha256)||!OriginalHeightScene::Sha(expected.renderer_sha256))return {};
 try{std::array<unsigned char,32> digest{};unsigned size=0;if(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&size,EVP_sha256(),nullptr)!=1||size!=32)return {};
 auto Hex=[](std::span<const uint8_t> data){constexpr char alphabet[]="0123456789abcdef";std::string out;for(auto byte:data){out+=alphabet[byte>>4];out+=alphabet[byte&15];}return out;};
 if(Hex(digest)!=expected.sidecar_sha256||std::string_view(reinterpret_cast<const char*>(bytes.data()),8)!="EQHTRE01"||Hex(bytes.subspan(8,32))!=expected.renderer_sha256||Hex(bytes.subspan(40,32))!=expected.mesh_sha256||Hex(bytes.subspan(72,32))!=expected.canonical_tree_sha256)return {};
 const OriginalHeightScene::MeshCatalogEntry* mesh_entry=nullptr;for(const auto& row:OriginalHeightScene::OriginalMeshCatalog)if(row.item_id==expected.item_id&&row.mesh_sha256==expected.mesh_sha256){if(mesh_entry)return {};mesh_entry=&row;}if(!mesh_entry)return {};
 auto mesh=OriginalHeightScene::ReadMeshSidecar(mesh_bytes,*mesh_entry);if(!mesh)return {};Tree tree;tree.mesh=std::make_shared<const LocalMesh>(std::move(*mesh));
 size_t at=104;auto Get=[&](){if(at+4>bytes.size())throw Detail::Fault{};uint32_t value=uint32_t(bytes[at])|(uint32_t(bytes[at+1])<<8)|(uint32_t(bytes[at+2])<<16)|(uint32_t(bytes[at+3])<<24);at+=4;return value;};
 const auto descriptors=Get(),nodes=Get();tree.root=Get();if(descriptors!=expected.descriptor_count||descriptors!=tree.mesh->polygons.size()||!descriptors||descriptors>8000||nodes!=expected.node_count||!nodes||nodes>1024||tree.root>=nodes)return {};
 tree.descriptors.reserve(descriptors);for(uint32_t i=0;i<descriptors;++i){Descriptor d;for(auto&value:d.cache.normal)value=std::bit_cast<float>(Get());d.cache.plane=std::bit_cast<float>(Get());d.cache.radius=std::bit_cast<float>(Get());auto valid=Get();if(valid>1)return {};d.cache.valid=valid;
 const auto& polygon=tree.mesh->polygons[i];if(polygon.index!=i||polygon.flags)return {};const auto actual=ModelCache(polygon.native_abc);if(!actual||actual->valid!=d.cache.valid||!OriginalSphereSource::Bits(actual->normal,d.cache.normal)||!Same(actual->plane,d.cache.plane)||!Same(actual->radius,d.cache.radius))return {};tree.descriptors.push_back(d);}
 uint64_t references=0;tree.nodes.reserve(nodes);for(uint32_t i=0;i<nodes;++i){Node n;for(auto&value:n.sphere)value=std::bit_cast<float>(Get());for(float value:n.sphere)if(!std::isfinite(value)||std::abs(value)>2e7)return {};if(n.sphere[0]<0)return {};const auto count=Get();if(count>descriptors||references+count>uint64_t(descriptors)*8)return {};references+=count;for(auto& child:n.children){child=Get();if(child!=UINT32_MAX&&child>=nodes)return {};}for(uint32_t j=0;j<count;++j){const auto index=Get();if(index>=descriptors||!tree.descriptors[index].cache.valid)return {};n.polygon_ids.push_back(index);}tree.nodes.push_back(std::move(n));}
 if(at!=bytes.size())return {};std::set<uint32_t>visited;std::vector<std::pair<uint32_t,unsigned>>todo{{tree.root,0}};while(!todo.empty()){auto[index,depth]=todo.back();todo.pop_back();if(depth>32||!visited.insert(index).second)return {};for(auto child:tree.nodes[index].children)if(child!=UINT32_MAX)todo.push_back({child,depth+1});}if(visited.size()!=nodes)return {};return tree;
 }catch(...){return {};}
}
}
#endif
