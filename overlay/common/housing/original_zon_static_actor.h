#ifndef EQEMU_HOUSING_ORIGINAL_ZON_STATIC_ACTOR_H
#define EQEMU_HOUSING_ORIGINAL_ZON_STATIC_ACTOR_H
#include "original_zon_authored_actor_catalog.h"
#include "original_simple_tree_generator.h"
#include "original_height_scene_query.h"
#include "original_renderer_trig_tables.h"
namespace Housing::OriginalZonStaticActor {
namespace S=OriginalHeightSceneQuery;
inline constexpr std::string_view RendererSha="ade556b68e98f6f39c5578f70431e11c0b9babcc15b8571eb22cf1e2b654417e";
struct Pose {Point position,heading_pitch_roll;float scale{};OriginalActorLocalQuery::Matrix matrix;};
// Literal10065754..794: raw XYZ copied in original order; rawrot0/2 multiply
// source float42a2f983, rawrot1 multipliesc2a2f983; every product storesbinary32.
// Original startup/mainloop and precision-only display guards preserve RCchop.
// Binary32 product followed FSTP32 is identical under evidenced PC24/PC53 chop.
// No source radians/Euler convention is guessed from a map builder.
inline std::optional<Pose> PoseFromZon(const Profile& p){
 OriginalSphereQuery::Detail::Rounding env;if(!env.good||!OriginalSphereSource::Finite(p.raw_position)||!OriginalSphereSource::Finite(p.raw_angles)||!std::isfinite(p.scale)||p.scale<=0||p.scale>1e6f)return {};
 try{Point angles;for(unsigned k=0;k<3;++k)angles[k]=OriginalActorQueryFrame::Detail::Store(OriginalSphereQuery::Detail::product(p.raw_angles[k],F(k==1?0xc2a2f983u:0x42a2f983u)));
 auto tables=OriginalActorLocalQuery::OriginalRendererTables();if(!tables)return {};auto matrix=OriginalActorLocalQuery::MatrixFromRendererAngles(p.raw_position,angles,*tables);if(!matrix)return {};return Pose{p.raw_position,angles,p.scale,*matrix};
 }catch(...){return {};}
}
inline std::string Hex(std::span<const unsigned char> bytes){static constexpr char digits[]="0123456789abcdef";std::string out;for(auto c:bytes){out+=digits[c>>4];out+=digits[c&15];}return out;}
inline std::string FrameSha(const Profile&p){
 auto pose=PoseFromZon(p);if(!pose)return {};std::vector<uint8_t>b;const std::string_view tag="OriginalZonV2AuthoredPlainFrameV2/TreeGenerator268V1/LinearV1";b.insert(b.end(),tag.begin(),tag.end());b.insert(b.end(),RendererSha.begin(),RendererSha.end());b.insert(b.end(),p.zon_sha256.begin(),p.zon_sha256.end());
 auto word=[&](uint32_t n){for(unsigned k=0;k<4;++k)b.push_back(uint8_t(n>>(8*k)));};word(p.placement_index);for(const auto&v:{p.raw_position,p.raw_angles,pose->heading_pitch_roll})for(float f:v)word(std::bit_cast<uint32_t>(f));word(std::bit_cast<uint32_t>(p.scale));for(float f:pose->matrix)word(std::bit_cast<uint32_t>(f));
 std::array<unsigned char,32>d{};unsigned n=0;if(EVP_Digest(b.data(),b.size(),d.data(),&n,EVP_sha256(),nullptr)!=1||n!=32)return {};return Hex(d);
}
// Trusted-source metadata factory. It authenticates compiled source/pose/class
// construction, not the current actor census. The caller's independently frozen
// scene must prove these exact default fields still describe the current actor;
// game switch/appdata reassociation or changed pose requires another profile.
// Immutable prepared geometry may be cached by compiled mesh_sha256. Only the
// compiled profile lookup may enroll a class; callers cannot prepare arbitrary
// peer-provided profiles or substitute a different generated tree/mesh owner.
class Resource {
 friend struct Provider;
 std::string mesh_sha_;std::shared_ptr<const OriginalHeightScene::LocalMesh> mesh_;
 std::shared_ptr<const OriginalSimpleTreeQuery::Tree> tree_;
 std::shared_ptr<const OriginalSimpleLinearQuery::Source> linear_;bool all_skipped_{};
 Resource()=default;
};
struct Provider {
 static const Profile* Find(std::string_view zone,uint32_t index){
  const Profile* found=nullptr;for(const auto&p:PersonalDoors)if(p.zone==zone&&p.placement_index==index){if(found)return nullptr;found=&p;}for(const auto&p:GuildAuthoredPlacements)if(p.zone==zone&&p.placement_index==index){if(found)return nullptr;found=&p;}return found;
 }
 // Geometry-only descriptor for explicit Unknown sources too. Returning this
 // mesh never grants a collision dispatch or source-census completeness.
 static std::shared_ptr<const OriginalHeightScene::LocalMesh> Geometry(std::string_view zone,uint32_t index,std::span<const uint8_t> bytes){
  const auto*p=Find(zone,index);if(!p)return {};auto mesh=OriginalHeightScene::ReadMeshSidecar(bytes,p->mesh);if(!mesh||mesh->vertices.size()!=p->vertices||mesh->polygons.size()!=p->polygons)return {};return std::make_shared<const OriginalHeightScene::LocalMesh>(std::move(*mesh));
 }
 static std::shared_ptr<const Resource> ReadResource(std::string_view zone,uint32_t index,std::span<const uint8_t> bytes){
  const auto*p=Find(zone,index);if(!p)return {};auto mesh=Geometry(zone,index,bytes);if(!mesh)return {};auto resource=std::shared_ptr<Resource>(new Resource);resource->mesh_sha_=p->mesh.mesh_sha256;resource->mesh_=mesh;
  if(!p->raw_low16_zero){
   // Exact compiled Standard OBP_water and four original Tree visual steps.
   // Every source triangle has low16 flag1 and is skipped by the original query.
   // Hash/geometry/count checks above
   // bind the original ordinary kind0, +5cfalse, no-override profile. Its name
   // matches none of100c5270's special mutation profiles. Current association
   // with that original definition remains independently required by Bind.
   const bool reviewed_empty=(p->zone=="guildhall"&&p->placement_index==86&&p->vertices==30&&p->polygons==40)||
    (p->zone=="phinteriortree"&&p->placement_index>=21&&p->placement_index<=24);
   if(!reviewed_empty||!std::all_of(mesh->polygons.begin(),mesh->polygons.end(),[](const auto&poly){return poly.flags==1;}))return {};
   resource->all_skipped_=true;return resource;
  }
  if(p->polygons>200){auto generated=OriginalSimpleTreeGenerator::Build(mesh,{true,true,false,0,p->polygons});if(generated.status!=OriginalSimpleTreeGenerator::Status::Ready)return {};resource->tree_=std::make_shared<const OriginalSimpleTreeQuery::Tree>(std::move(generated.tree));}
  else{auto linear=OriginalSimpleLinearQuery::Prepare(mesh,{true,false,0,p->polygons,false});if(!linear||!std::any_of(linear->descriptors.begin(),linear->descriptors.end(),[](const auto&d){return d.valid;}))return {};resource->linear_=std::make_shared<const OriginalSimpleLinearQuery::Source>(std::move(*linear));}return resource;
 }
 static std::shared_ptr<const S::ActorSource> BindAuthored(std::string_view zone,uint32_t index,std::string_view current_zon_sha,std::string_view current_renderer_sha,const OriginalHeightScene::Entry&e,const std::shared_ptr<const Resource>&resource){
  const auto*profile=Find(zone,index);
  if(!profile||!resource||resource->mesh_sha_!=profile->mesh.mesh_sha256||current_zon_sha!=profile->zon_sha256||current_renderer_sha!=RendererSha||e.terrain||!e.disabled||*e.disabled||!e.identity.actor||e.identity.actor>UINT32_MAX||!e.identity.epoch||!e.identity.pose_revision||e.identity.resource_sha256!=profile->mesh.member_sha256||e.expected_manifest_sha256!=profile->mesh.mesh_sha256||e.identity.frame_sha256!=FrameSha(*profile))return {};
  auto pose=PoseFromZon(*profile);if(!pose)return {};auto proof=std::shared_ptr<const S::AuthoredClassProof>(new S::AuthoredClassProof(e.identity,e.expected_manifest_sha256,pose->matrix,pose->scale,resource->mesh_,resource->tree_,resource->all_skipped_));if(!proof->Matches(e,pose->matrix,pose->scale,resource->mesh_,resource->tree_))return {};
  auto out=std::make_shared<S::ActorSource>();out->role=S::ActorSource::Role::AuthoredNullAppPlain;out->authored_class=std::move(proof);out->authored_tree=resource->tree_;out->identity=e.identity;out->manifest_sha256=e.expected_manifest_sha256;out->original_actor_matrix=pose->matrix;out->scale=pose->scale;out->mesh=resource->mesh_;out->dispatch=resource->all_skipped_?S::ActorSource::Dispatch::PlainSimpleLinearAllSkipped:resource->tree_?S::ActorSource::Dispatch::PlainSimpleAccelerated:S::ActorSource::Dispatch::PlainSimpleLinear;out->linear_source=resource->linear_;return out;
 }
 static std::shared_ptr<const S::ActorSource> ReadAuthored(std::string_view zone,uint32_t index,std::string_view zon,std::string_view renderer,const OriginalHeightScene::Entry&e,std::span<const uint8_t> bytes){return BindAuthored(zone,index,zon,renderer,e,ReadResource(zone,index,bytes));}
 static std::shared_ptr<const S::ActorSource> ReadPersonalDoor(std::string_view zone,std::string_view zon,std::string_view renderer,const OriginalHeightScene::Entry&e,std::span<const uint8_t> bytes){if(!Find(zone,1)||!zone.starts_with("phinterior"))return {};return ReadAuthored(zone,1,zon,renderer,e,bytes);}
 // Moving+68=true; original zero-fraction callback selects actor.type==3.
 // Authenticated authored type5 therefore uses +0c=false for the linear branch.
 static std::shared_ptr<const S::ActorSource> ForQuery(const std::shared_ptr<const S::ActorSource>&source,const OriginalHeightScene::Entry&e,const OriginalHeightScene::Query&q){
  if(!source||!source->authored_class||source->role!=S::ActorSource::Role::AuthoredNullAppPlain||!source->authored_class->Matches(e,source->original_actor_matrix,source->scale,source->mesh,source->authored_tree))return {};
  auto out=std::make_shared<S::ActorSource>(*source);
  if(source->authored_class->AllSkipped()){if(source->dispatch!=S::ActorSource::Dispatch::PlainSimpleLinearAllSkipped||source->authored_tree||source->linear_source||source->native_tree_query||source->native_linear_query)return {};return out;}
  if(source->authored_class->Accelerated()){
   if(!source->authored_tree)return {};auto delta=S::Delta(q);if(!delta)return {};auto frame=OriginalActorLocalQuery::Build(source->original_actor_matrix,source->scale,q.start,*delta,q.radius);if(!frame)return {};auto local_delta=OriginalActorLocalQuery::SphereDelta(frame->start,frame->delta);if(!local_delta)return {};auto candidates=OriginalSimpleTreeQuery::Enumerate(*source->authored_tree,frame->start,*local_delta,frame->radius);if(candidates.status!=OriginalSimpleTreeQuery::Status::Ready)return {};out->native_tree_query=S::ActorSource::Enumeration{q.start,*delta,q.radius,std::move(candidates.polygon_ids)};
  }else{
   if(!source->linear_source)return {};auto receipt=S::LinearCandidates(*source->linear_source,{true,true,false},source->original_actor_matrix,source->scale,q);if(!receipt)return {};out->native_linear_query=std::move(receipt);
  }return out;
 }
};
}
#endif
