#ifndef EQEMU_HOUSING_ORIGINAL_SIMPLE_TREE_QUERY_H
#define EQEMU_HOUSING_ORIGINAL_SIMPLE_TREE_QUERY_H
#include "original_actor_local_query.h"
#include "original_mesh_sidecar.h"
namespace Housing::OriginalSimpleTreeQuery {
using Point=OriginalSphereQuery::Point;
using LocalMesh=OriginalHeightScene::LocalMesh;
namespace Detail {
using namespace OriginalActorLocalQuery::Detail;
struct Math:OriginalActorLocalQuery::Detail::Math {
 long double fp(uint32_t address)const{uint32_t v;switch(address){
#include "original_simple_tree_constants.inc"
 default:return OriginalActorLocalQuery::Detail::Math::fp(address);}return static_cast<long double>(std::bit_cast<float>(v));}

 void run(uint32_t entry,uint32_t stop=0){pc=entry;for(unsigned count=0;count<16000;++count){++steps;if(pc==stop&&stop)return;switch(pc){
#include "original_simple_tree_operations.inc"
 default:throw Fault{};}}throw Fault{};}
};
inline float Store(long double x){volatile long double value=x;volatile float out=float(value);if(!std::isfinite(out))throw Fault{};return out;}
}
struct Cache {Point normal{};float plane{},radius{};bool valid{};};
inline std::optional<Cache> ModelCache(const std::array<Point,3>& abc){
 Detail::Environment environment;if(!environment.good)return {};for(const auto&p:abc)if(!OriginalSphereSource::Finite(p))return {};
 try{Detail::Math m;constexpr uint32_t d=0x10000;constexpr std::array<uint32_t,3>v{0x10100,0x10110,0x10120};for(unsigned j=0;j<3;++j){m.put(d+0x10+4*j,v[j]);for(unsigned k=0;k<3;++k)m.fpwrite(v[j]+4*k,abc[j][k]);}
 m.r[2]=d;m.r[5]=v[0];m.r[4]=v[1];m.r[6]=v[2];m.run(0x100c559d,0x100c56b3);Cache c;for(unsigned k=0;k<3;++k)c.normal[k]=float(m.fp(d+4*k));c.plane=float(m.fp(d+12));c.radius=float(m.fp(d+28));c.valid=c.normal!=Point{};
 // Original100c56db..56f3 clears the entire invalid descriptor including radius.
 if(!c.valid)c={};return c;
 }catch(...){return {};}
}
struct Descriptor {Cache cache;};
struct Node {std::array<float,4> sphere{};std::vector<uint32_t> polygon_ids;std::array<uint32_t,9> children{};};
struct Tree {uint32_t root{};std::shared_ptr<const LocalMesh> mesh;std::vector<Descriptor> descriptors;std::vector<Node> nodes;};
inline bool ShapeValid(const Tree&tree){
 if(!tree.mesh||tree.nodes.empty()||tree.nodes.size()>1024||tree.root>=tree.nodes.size()||tree.descriptors.size()!=tree.mesh->polygons.size()||tree.descriptors.empty()||tree.descriptors.size()>8000)return false;
 try{uint64_t refs=0;for(unsigned i=0;i<tree.descriptors.size();++i){const auto&d=tree.descriptors[i].cache;const auto&p=tree.mesh->polygons[i];if(p.index!=i||p.flags||(d.valid==(d.normal==Point{}))||!OriginalSphereSource::Finite(d.normal)||!std::isfinite(d.plane)||!std::isfinite(d.radius)||d.radius<0)return false;for(const auto&v:p.native_abc)if(!OriginalSphereSource::Finite(v))return false;}
 std::set<uint32_t>visited;std::vector<std::pair<uint32_t,unsigned>>todo{{tree.root,0}};while(!todo.empty()){auto[index,depth]=todo.back();todo.pop_back();if(index>=tree.nodes.size()||depth>32||!visited.insert(index).second)return false;const auto&node=tree.nodes[index];for(float v:node.sphere)if(!std::isfinite(v)||std::abs(v)>2e7)return false;if(node.sphere[0]<0||node.polygon_ids.size()>tree.descriptors.size())return false;refs+=node.polygon_ids.size();if(refs>tree.descriptors.size()*8)return false;for(auto id:node.polygon_ids)if(id>=tree.descriptors.size()||!tree.descriptors[id].cache.valid)return false;for(auto child:node.children)if(child!=UINT32_MAX)todo.push_back({child,depth+1});}return visited.size()==tree.nodes.size();}catch(...){return false;}
}
struct Query {Point start{},delta{};float radius{},inverse_length{};};
inline std::optional<Query> MakeQuery(const Point& start,const Point& delta,float radius){
 Detail::Environment environment;if(!environment.good||!OriginalSphereSource::Finite(start)||!OriginalSphereSource::Finite(delta)||!std::isfinite(radius)||radius<=0||radius>1e6f)return {};
 try{Detail::Math m;constexpr uint32_t v=0x10000;for(unsigned k=0;k<3;++k)m.fpwrite(v+4*k,delta[k]);m.r[2]=v;m.put(m.r[7],0xeeeeeeee);m.run(0x1003ad20);if(m.depth!=1||!(m.st[0]>0))return {};const float inverse=Detail::Store(Detail::quotient(1,m.st[0]));if(!std::isfinite(inverse)||inverse<=0)return {};return Query{start,delta,radius,inverse};}catch(...){return {};}
}
inline bool ValidQuery(const Query&q){return OriginalSphereSource::Finite(q.start)&&OriginalSphereSource::Finite(q.delta)&&q.delta!=Point{}&&std::isfinite(q.radius)&&q.radius>0&&q.radius<=1e6f&&std::isfinite(q.inverse_length)&&q.inverse_length>0;}
// Original100d0200->100d7c90 swept-sphere/node-sphere segment test.
inline std::optional<bool> NodeIntersects(const Query&q,const std::array<float,4>& sphere){
 Detail::Environment environment;if(!environment.good||!ValidQuery(q))return {};for(float f:sphere)if(!std::isfinite(f)||std::abs(f)>2e7)return {};if(sphere[0]<0)return {};try{Detail::Math m;constexpr uint32_t a=0x10000,d=0x10100,b=0x10200;m.fpwrite(a,q.radius);for(unsigned k=0;k<3;++k){m.fpwrite(a+4+4*k,q.start[k]);m.fpwrite(d+4*k,q.delta[k]);}for(unsigned k=0;k<4;++k)m.fpwrite(b+4*k,sphere[k]);m.put(m.r[7],0xeeeeeeee);m.put(m.r[7]+4,a);m.put(m.r[7]+8,d);m.put(m.r[7]+12,b);m.run(0x100d0200);if(m.depth)return {};return (m.r[0]&255)!=0;}catch(...){return {};}
}
inline std::optional<bool> DistanceMayReach(const Query&q,const Point& center,float radius,float maximum){
 Detail::Environment environment;if(!environment.good||!ValidQuery(q)||!OriginalSphereSource::Finite(center)||!std::isfinite(radius)||radius<0||!std::isfinite(maximum)||maximum<0||maximum>1)return {};
 try{const auto x=Detail::difference(q.start[0],center[0]),y=Detail::difference(q.start[1],center[1]),z=Detail::difference(q.start[2],center[2]);const auto length=Detail::root(Detail::sum(Detail::sum(Detail::product(x,x),Detail::product(y,y)),Detail::product(z,z)));return Detail::product(Detail::difference(length,radius),q.inverse_length)<=maximum;}catch(...){return {};}
}
inline std::optional<bool> Front(const Query&q,const Point&normal){Detail::Environment environment;if(!environment.good||!ValidQuery(q)||!OriginalSphereSource::Finite(normal))return {};try{return Detail::sum(Detail::sum(Detail::product(q.delta[1],normal[1]),Detail::product(normal[0],q.delta[0])),Detail::product(q.delta[2],normal[2]))<=0;}catch(...){return {};}}
enum class Status {Ready,Invalid,Unknown,BudgetExceeded};
struct Enumeration {Status status=Status::Invalid;std::vector<uint32_t> polygon_ids;uint32_t nodes{},descriptors{},pruned_nodes{},pruned_descriptors{};};
// Pure conservative max1 enumeration for the next strict scene policy. Preserve
// original reverse node-list order and nine-child LIFO traversal. There is no
// hit-dependent node pruning: that native optimization depends on the preceding
// winner; omitting it only broadens candidate coverage and exposes stored ties.
// Per-descriptor original max1 distance and one-sided guards still apply.
inline Enumeration Enumerate(const Tree& tree,const Point& start,const Point& delta,float radius,uint32_t node_limit=1024,uint32_t reference_limit=65536){
 Enumeration out;Detail::Environment environment;if(!environment.good){out.status=Status::Unknown;return out;}auto q=MakeQuery(start,delta,radius);if(!q||!ShapeValid(tree)||!node_limit||node_limit>1024||!reference_limit||reference_limit>65536)return out;
 try{std::vector<uint32_t> stack{tree.root};std::set<uint32_t> visited;std::set<uint32_t> emitted;
 while(!stack.empty()){const auto index=stack.back();stack.pop_back();if(index==UINT32_MAX)continue;if(index>=tree.nodes.size()||!visited.insert(index).second)return out;if(out.nodes==node_limit){out.status=Status::BudgetExceeded;return out;}++out.nodes;const auto&node=tree.nodes[index];auto intersects=NodeIntersects(*q,node.sphere);if(!intersects){out.status=Status::Unknown;return out;}if(!*intersects){++out.pruned_nodes;continue;}
 for(auto i=node.polygon_ids.rbegin();i!=node.polygon_ids.rend();++i){if(*i>=tree.descriptors.size())return out;if(out.descriptors==reference_limit){out.status=Status::BudgetExceeded;return out;}++out.descriptors;const auto&polygon=tree.mesh->polygons[*i];const auto&desc=tree.descriptors[*i];if(polygon.index!=*i||polygon.flags||!desc.cache.valid)return out;auto front=Front(*q,desc.cache.normal);if(!front){out.status=Status::Unknown;return out;}if(!*front){++out.pruned_descriptors;continue;}auto reaches=DistanceMayReach(*q,polygon.native_abc[0],desc.cache.radius,1);if(!reaches){out.status=Status::Unknown;return out;}if(!*reaches){++out.pruned_descriptors;continue;}if(emitted.insert(*i).second)out.polygon_ids.push_back(*i);}
 if(stack.size()+9>1000){out.status=Status::BudgetExceeded;return out;}for(auto child:node.children)stack.push_back(child);
 }
 out.status=Status::Ready;return out;
 }catch(...){return out;}
}
}
#endif
