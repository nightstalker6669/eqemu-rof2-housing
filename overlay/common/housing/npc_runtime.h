#ifndef EQEMU_HOUSING_NPC_RUNTIME_H
#define EQEMU_HOUSING_NPC_RUNTIME_H
#include "npc_item_metadata.h"
#include "placement_policy.h"
#include "placement_wire.h"
#include "npc_native_frame.h"
#include "npc_native_model_profiles.h"
namespace Housing::NpcRuntime {
// Original executable2a8702ad...:50a370 prefix lookup,59efd0 race correction,
// 5b2061..5b2095 NPC placement envelope. No source item-to-model mapping seeded.
struct Profile {uint32_t item_id=0,race=0,gender=0,model_id=0;std::string model,filename,source_sha256;float actor_height=0,entity_height=0;NpcNativeHeight::Variants allowed_heights{};uint32_t roffset_bits=0;bool race_flag=false;bool legacy_fixed_eye=false;};
inline std::optional<Profile> Review(const NpcItemMetadata::Source& s){
 if(!NpcItemMetadata::Valid(s)||s.filename.size()<3||s.interactive||s.respawn_interval||!(s.actor_height>0)||!std::isfinite(s.actor_height))return {};
 const auto prefix=s.filename.substr(0,3);Profile p;p.item_id=s.item_id;p.model=prefix;p.filename=s.filename;p.source_sha256=s.source_sha256;p.actor_height=s.actor_height;
 const auto* model=NpcNativeModels::Lookup(prefix);if(!model)return {};
 p.race=model->race;p.gender=model->gender;p.roffset_bits=model->roffset_bits;p.race_flag=model->race_flag;
 if(model->fixed_height_bits)p.allowed_heights.height_bits={model->fixed_height_bits,model->fixed_height_bits,model->fixed_height_bits,model->fixed_height_bits};
 else{auto heights=NpcNativeHeight::OriginalControlStates(NpcNativeHeight::Bits(s.actor_height),model->roffset_bits,model->race_flag);if(!heights)return {};p.allowed_heights=*heights;}
 p.entity_height=NpcNativeHeight::Float(p.allowed_heights.height_bits[0]);p.legacy_fixed_eye=prefix=="EYE"&&p.race==108&&p.gender==2;
 // Server semantic model namespace; NOT an original wire model/race number.
 p.model_id=0xf0000000u|(p.race<<2)|p.gender;return p;
}
inline std::optional<Profile> Cached(const EQ::ItemData& d){
 if(!d.HousingNpcMetadataKnown)return {};
 const auto end=std::find(std::begin(d.HousingHousepetFilename),std::end(d.HousingHousepetFilename),char(0));
 const auto sha_end=std::find(std::begin(d.HousingNpcSourceSha256),std::end(d.HousingNpcSourceSha256),char(0));
 if(end==std::end(d.HousingHousepetFilename)||sha_end==std::end(d.HousingNpcSourceSha256)||d.HousingInteractiveObject>1)return {};
 return Review({d.ID,std::string(d.HousingHousepetFilename,end),d.HousingInteractiveObject!=0,d.HousingNpcRespawnInterval,std::string(d.HousingNpcSourceSha256,sha_end),d.HousingNpcActorHeight});
}
inline bool Same(const Profile& a,const Profile& b){return a.item_id==b.item_id&&a.race==b.race&&a.gender==b.gender&&a.model_id==b.model_id&&a.model==b.model&&a.filename==b.filename&&a.actor_height==b.actor_height&&a.entity_height==b.entity_height&&a.source_sha256==b.source_sha256&&a.allowed_heights.height_bits==b.allowed_heights.height_bits&&a.roffset_bits==b.roffset_bits&&a.race_flag==b.race_flag&&a.legacy_fixed_eye==b.legacy_fixed_eye;}
inline bool Valid(const Profile& p){auto reviewed=Review({p.item_id,p.filename,false,0,p.source_sha256,p.actor_height});return reviewed&&Same(*reviewed,p)&&p.model_id==reviewed->model_id;}
inline PlacementPolicy::Geometry Geometry(const Profile& p){
 if(!Valid(p))throw std::invalid_argument("Incoherent NPC profile");
 PlacementPolicy::Geometry g;g.model_id=p.model_id;g.center={0,0,p.entity_height};g.extents={1,1,p.entity_height};g.native_collision_verified=true;g.npc_height=p.entity_height;return g;
}
inline bool NativeItem(const ItemWire::Metadata& n){return !n.interactive&&!n.respawn_interval&&std::isfinite(n.scale_min)&&std::isfinite(n.scale_max)&&n.scale_min>0&&n.scale_max>=n.scale_min&&n.placement_type==0&&!n.definition_id;}
inline bool Pose(const PlacementPolicy::Transform& t){return t.scale>0&&std::isfinite(t.scale)&&std::isfinite(t.pitch)&&std::isfinite(t.roll);}
// Native711a's submitted Z is the actor origin minus actual entity height.
// Stored NPC poses use this submitted ground frame;1045/626e restore actor Z.
inline PlacementPolicy::Transform ActorPose(const Profile& p,PlacementPolicy::Transform t){if(!Valid(p)||!Pose(t))throw std::invalid_argument("Unsupported NPC transform");t.position[2]=float(t.position[2]+p.entity_height);if(!std::isfinite(t.position[2]))throw std::invalid_argument("NPC actor origin overflow");return t;}
inline bool SubmittedBounds(const Profile& p,const PlacementWire::Commit& c){return Valid(p)&&c.transform[3]>0&&std::isfinite(c.transform[3])&&std::isfinite(c.transform[1])&&std::isfinite(c.transform[2])&&NpcNativeHeight::Contains(p.allowed_heights,c.vector_a[2])&&c.vector_a==std::array<float,3>{1,1,c.vector_a[2]}&&c.vector_b==std::array<float,3>{0,0,c.vector_a[2]};}
inline std::optional<NpcNativeFrame::Frame> SubmittedFrame(const Profile& p,const PlacementWire::Commit& c){if(!SubmittedBounds(p,c))return {};return NpcNativeFrame::FromSubmitted(p.allowed_heights,c.transform[6],c.vector_a[2]);}
inline PlacementPolicy::Geometry Geometry(const Profile& p,const std::optional<NpcNativeFrame::Frame>& frame,const PlacementPolicy::Transform& pose){
 if(!Pose(pose))throw std::invalid_argument("Unsupported NPC transform");
 if(!frame){if(!p.legacy_fixed_eye)throw std::invalid_argument("NPC frame missing");return Geometry(p);}
 if(!Valid(p)||!Pose(pose)||!NpcNativeFrame::Valid(p.allowed_heights,pose.position[2],*frame))throw std::invalid_argument("NPC frame invalid");
 auto g=Geometry(p);const auto h=NpcNativeHeight::Float(frame->height_bits);g.center={0,0,h};g.extents={1,1,h};g.npc_height=h;return g;
}
inline PlacementPolicy::Transform ActorPose(const Profile& p,PlacementPolicy::Transform pose,const std::optional<NpcNativeFrame::Frame>& frame){
 if(!frame){if(!p.legacy_fixed_eye)throw std::invalid_argument("NPC frame missing");return ActorPose(p,pose);}
 (void)Geometry(p,frame,pose);pose.position[2]=NpcNativeHeight::Float(frame->origin_z_bits);return pose;
}
}
#endif
