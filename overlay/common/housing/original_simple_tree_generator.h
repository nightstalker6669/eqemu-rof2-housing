#ifndef EQEMU_HOUSING_ORIGINAL_SIMPLE_TREE_GENERATOR_H
#define EQEMU_HOUSING_ORIGINAL_SIMPLE_TREE_GENERATOR_H
#include "original_simple_tree_query.h"
namespace Housing::OriginalSimpleTreeGenerator {
using Point=OriginalSphereQuery::Point;
using Mesh=OriginalHeightScene::LocalMesh;
using Tree=OriginalSimpleTreeQuery::Tree;
namespace Detail {
using namespace OriginalActorLocalQuery::Detail;
struct Math:OriginalActorLocalQuery::Detail::Math {
 long double fp(uint32_t address)const {uint32_t v;switch(address){
#include "original_simple_tree_generator_constants.inc"
 default:return OriginalActorLocalQuery::Detail::Math::fp(address);}return static_cast<long double>(std::bit_cast<float>(v));}
 void run(uint32_t entry,uint32_t stop=0){pc=entry;for(unsigned n=0;n<16000;++n){++steps;if(stop&&pc==stop)return;switch(pc){
#include "original_simple_tree_generator_operations.inc"
 default:throw Fault{};}}throw Fault{};}
};
inline float Store(long double x){volatile long double a=x;volatile float b=float(a);if(!std::isfinite(b))throw Fault{};return b;}
// Stream actual ordered vertices through the unchanged original bounds loop.
// Reuse scratch storage, preserving all six live x87 extrema and loop counter.
// No all-vertex-to-eligible-triangle or centre formula substitution is made.
inline std::optional<std::array<float,4>> RootSphere(const Mesh& mesh){
 if(mesh.vertices.empty()||mesh.vertices.size()>100000)return {};
 try{Math m;constexpr uint32_t def=0x10000,input=0x11000,dest=0x12000;m.put(def+0x6c,uint32_t(mesh.vertices.size()));m.put(def+0x74,dest);m.put(m.r[7],0xeeeeeeee);m.put(m.r[7]+4,input);m.r[2]=def;m.run(0x100c4d70,0x100c4d9e);
 for(const auto&p:mesh.vertices){if(!OriginalSphereSource::Finite(p))return {};for(unsigned k=0;k<3;++k)m.fpwrite(input+4*k,p[k]);m.r[5]=input+8;m.r[3]=0;m.run(0x100c4d9e,0x100c4e3b);}m.run(0x100c4e3b);if(m.depth)return {};
 m.r.fill(0);m.r[7]=0x700000;m.r[1]=def;m.put(def+0x60,0);m.run(0x100c5469,0x100c54a8);if(m.depth)return {};const float input_radius=float(m.fp(m.r[7]+4));return std::array<float,4>{Store(product(input_radius,1.5L)),0,0,0};
 }catch(...){return {};}
}
inline std::optional<std::array<float,4>> TriangleSphere(const std::array<Point,3>& abc){
 try{Math m;constexpr uint32_t d=0x10000,out=0x11000;constexpr std::array<uint32_t,3> v{0x12000,0x12010,0x12020};for(unsigned j=0;j<3;++j){m.put(d+0x10+4*j,v[j]);for(unsigned k=0;k<3;++k)m.fpwrite(v[j]+4*k,abc[j][k]);}m.put(m.r[7],0xeeeeeeee);m.put(m.r[7]+4,out);m.r[2]=d;m.run(0x100d93e0);if(m.depth)return {};std::array<float,4>s;for(unsigned k=0;k<4;++k)s[k]=float(m.fp(out+4*k));return s;}catch(...){return {};}
}
struct ChildResult {bool valid{},contained{};std::array<float,4> sphere{};};
inline ChildResult Child(const std::array<float,4>& parent,const std::array<float,4>& target,float radius,unsigned index){
 try{Math m;constexpr uint32_t n=0x10000,s=0x11000;for(unsigned k=0;k<4;++k){m.fpwrite(n+0x1c+4*k,parent[k]);m.fpwrite(s+4*k,target[k]);}m.put(m.r[7],0xeeeeeeee);m.put(m.r[7]+4,index);for(unsigned k=0;k<3;++k)m.fpwrite(m.r[7]+8+4*k,index==8?0:((index&(k==0?2u:k==1?1u:4u))?.5L:-.5L));m.fpwrite(m.r[7]+20,radius);m.put(m.r[7]+24,s);m.r[2]=n;m.run(0x100d14a0,0x100d150d);if(m.pc!=0x100d150d)return {true,false,{}};std::array<float,4> c{radius};for(unsigned k=0;k<3;++k)c[k+1]=float(m.fp(m.r[7]+8+4*k));return {true,true,c};}catch(...){return {};}
}
struct Budget{};
struct Node {std::array<float,4> sphere{};std::array<uint32_t,9> children{};std::vector<uint32_t> refs;Node(){children.fill(UINT32_MAX);}};
}
// Scope receipt supplied only by the authenticated model-class adapter. Raw
// mesh/count alone cannot assert renderer enabled/plain/static/source64 policy.
struct Definition {bool reviewed{},enabled{},disabled{};uint32_t kind{},polygon_count{};};
struct Limits {uint32_t nodes=4096,depth=32;uint64_t visits=1000000;};
enum class Status {Ready,Invalid,Unknown,BudgetExceeded};
struct Result {Status status=Status::Invalid;Tree tree;uint32_t allocated_nodes{};uint64_t visits{};};
inline Result Build(std::shared_ptr<const Mesh> mesh,const Definition& d,Limits limit={}){
 Result out;if(!d.reviewed||!d.enabled||d.disabled||d.kind==4||d.polygon_count<=200||d.polygon_count>8000||!mesh||mesh->polygons.size()!=d.polygon_count||!limit.nodes||limit.nodes>65536||!limit.depth||limit.depth>32||!limit.visits||limit.visits>16000000)return out;
 Detail::Environment env;if(!env.good){out.status=Status::Unknown;return out;}
 try{auto root=Detail::RootSphere(*mesh);if(!root||(*root)[0]<=0)return out;std::vector<Detail::Node> nodes(1);nodes[0].sphere=*root;out.tree.mesh=mesh;out.tree.descriptors.resize(d.polygon_count);
 for(uint32_t n=d.polygon_count;n>0;--n){const auto id=n-1;const auto&p=mesh->polygons[id];if(p.index!=id||p.flags)return out;auto cache=OriginalSimpleTreeQuery::ModelCache(p.native_abc);if(!cache)return out;out.tree.descriptors[id].cache=*cache;if(!cache->valid)continue;auto sphere=Detail::TriangleSphere(p.native_abc);if(!sphere)return out;uint32_t node=0;unsigned depth=0;
  for(;;){if(++out.visits>limit.visits){out.status=Status::BudgetExceeded;return out;}const auto next=Detail::product(nodes[node].sphere[0],std::bit_cast<float>(uint32_t(0x3f353f7d)));const auto minimum=std::bit_cast<float>(uint32_t(0x38d1b717));if(next<=(*sphere)[0]||next<minimum)break;const float child_radius=Detail::Store(next);bool descended=false;
   for(unsigned k=0;k<9;++k){if(++out.visits>limit.visits)throw Detail::Budget{};auto child=Detail::Child(nodes[node].sphere,*sphere,child_radius,k);if(!child.valid){out.status=Status::Unknown;return out;}if(!child.contained)continue;if(depth==limit.depth){out.status=Status::BudgetExceeded;return out;}if(nodes[node].children[k]==UINT32_MAX){if(nodes.size()==limit.nodes){out.status=Status::BudgetExceeded;return out;}const uint32_t created=uint32_t(nodes.size());nodes.emplace_back();nodes[created].sphere=child.sphere;nodes[node].children[k]=created;}node=nodes[node].children[k];++depth;descended=true;break;}if(!descended)break;
  }nodes[node].refs.push_back(id);
 }
 //100d1240 children8..0, small leaves concatenate;100d13b0 removes empty
 // single-child chains after recursively compacting each promoted descendant.
 std::function<void(uint32_t,unsigned)> compact;std::function<uint32_t(uint32_t,unsigned)> promote;
 auto leaf=[&](uint32_t n){return std::all_of(nodes[n].children.begin(),nodes[n].children.end(),[](auto c){return c==UINT32_MAX;});};
 promote=[&](uint32_t n,unsigned depth){compact(n,depth);while(nodes[n].refs.empty()){uint32_t one=UINT32_MAX;unsigned count=0;for(auto c:nodes[n].children)if(c!=UINT32_MAX){one=c;++count;}if(count!=1)break;n=one;compact(n,depth+1);}return n;};
 compact=[&](uint32_t n,unsigned depth){if(depth>64)throw Detail::Fault{};if(++out.visits>limit.visits)throw Detail::Budget{};for(unsigned slot=9;slot>0;--slot){const auto k=slot-1;while(nodes[n].children[k]!=UINT32_MAX){auto c=nodes[n].children[k];if(leaf(c)&&nodes[c].refs.size()<10){nodes[n].refs.insert(nodes[n].refs.end(),nodes[c].refs.begin(),nodes[c].refs.end());nodes[n].children[k]=UINT32_MAX;break;}auto p=promote(c,depth+1);nodes[n].children[k]=p;if(p==c)break;}}};compact(0,0);
 out.allocated_nodes=uint32_t(nodes.size());std::function<uint32_t(uint32_t)> emit=[&](uint32_t n){const auto id=uint32_t(out.tree.nodes.size());out.tree.nodes.emplace_back();out.tree.nodes[id].sphere=nodes[n].sphere;out.tree.nodes[id].polygon_ids=nodes[n].refs;out.tree.nodes[id].children.fill(UINT32_MAX);for(unsigned k=0;k<9;++k)if(nodes[n].children[k]!=UINT32_MAX){auto c=emit(nodes[n].children[k]);out.tree.nodes[id].children[k]=c;}return id;};out.tree.root=emit(0);if(!OriginalSimpleTreeQuery::ShapeValid(out.tree)){out.status=Status::Unknown;return out;}out.status=Status::Ready;return out;
 }catch(const Detail::Budget&){out.status=Status::BudgetExceeded;return out;}catch(...){out.status=Status::Unknown;return out;}
}
}
#endif
