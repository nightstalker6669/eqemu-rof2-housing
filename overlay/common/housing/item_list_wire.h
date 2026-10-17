#ifndef EQEMU_HOUSING_ITEM_LIST_WIRE_H
#define EQEMU_HOUSING_ITEM_LIST_WIRE_H
#include "neighborhood_wire.h"
namespace Housing::ItemListWire {
using Bytes=std::vector<uint8_t>;
struct Packets {Bytes list;std::vector<Bytes> deltas;};
// Existing native full-list and per-item delta forms. Large authorized estates
// must not be truncated to an early prototype's 32 items or a UDP-sized list.
inline Packets Encode(uint32_t estate,const std::vector<Bytes>& entries){
 if(!estate||estate==UINT32_MAX||entries.size()>UINT32_MAX)throw std::invalid_argument("Invalid estate item list");
 size_t total=8;for(const auto& e:entries){if(e.empty()||e.size()>65523)throw std::invalid_argument("Oversized estate item");total+=e.size();}
 Packets out;NeighborhoodWire::Detail::Write32(out.list,estate);NeighborhoodWire::Detail::Write32(out.list,total<=65535?uint32_t(entries.size()):0);
 if(total<=65535){for(const auto& e:entries)out.list.insert(out.list.end(),e.begin(),e.end());}
 else{out.deltas.reserve(entries.size());for(const auto& e:entries){Bytes delta;NeighborhoodWire::Detail::Write32(delta,estate);NeighborhoodWire::Detail::Write32(delta,0);NeighborhoodWire::Detail::Write32(delta,1);delta.insert(delta.end(),e.begin(),e.end());out.deltas.push_back(std::move(delta));}}
 return out;
}
}
#endif
