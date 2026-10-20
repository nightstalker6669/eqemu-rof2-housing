#ifndef EQEMU_HOUSING_ORIGINAL_SIMPLE_TREE_CATALOG_H
#define EQEMU_HOUSING_ORIGINAL_SIMPLE_TREE_CATALOG_H
#include <array>
#include <cstdint>
#include <string_view>
namespace Housing::OriginalSimpleTreeQuery {
struct CatalogEntry {uint32_t item_id,descriptor_count,node_count;std::string_view sidecar_sha256,mesh_sha256,canonical_tree_sha256,renderer_sha256;};
inline constexpr std::array<CatalogEntry,1> OriginalSimpleTreeCatalog{{
{66803,212,24,"ed5576f0bcb1a9cdf1c473f8cdda59dd0125885e5facb629353a034679bcfa8c","9d9806c759de280abac59f2aa749fda7fcad6078bb63d0787fde63b2104baa29","4313a33a6ae5f7173653f5aeb439ce0077d96d502d74630c1c3a904578a7d955","ade556b68e98f6f39c5578f70431e11c0b9babcc15b8571eb22cf1e2b654417e"},
}};
}
#endif
