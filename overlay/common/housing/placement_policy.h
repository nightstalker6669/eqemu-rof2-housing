#ifndef EQEMU_HOUSING_PLACEMENT_POLICY_H
#define EQEMU_HOUSING_PLACEMENT_POLICY_H
#include "access_policy.h"
#include "region_wire.h"
#include <algorithm>
#include <limits>
namespace Housing::PlacementPolicy {
using Point=std::array<double,3>; // Native axis0/1/2; no guessed ServerXYZ adapter.
enum class Operation:uint8_t { Add,Move };
enum class Error:uint8_t { None,InvalidContext,UnsupportedGeometry,InvalidMetadata,Denied,InvalidTransform,OutsideRegion,NumericallyAmbiguous };
struct Geometry {
 uint32_t model_id=0;
 Point center{},extents{};
 // Set only from server-owned original asset evidence corroborated against native
 // collision bounds. An arbitrary render-mesh AABB is not sufficient authority.
 bool native_collision_verified=false;
 double measured_local_error=0; // Per-corner component discrepancy budget at unit scale.
 uint32_t mounting_type=0; // Hash-pinned mounted resource class only; default preserves floor geometry.
 float npc_height=0; // Exact native NPC envelope; appended to preserve existing aggregate fields.
};
struct ItemMetadata {
 uint32_t item_id=0,model_id=0,placement_mask=0;
 float minimum_scale=0,maximum_scale=0;
 bool furniture_only=false; // Verified ordinary static furniture classification.
 uint32_t house_definition_type=0; // Legacy field name: server-validated container type2 hall /3 house /7 crate.
 // Neither template UNK227 nor a packet flag alone establishes this type or
 // native collision geometry. NPC/animated/unknown kinds stay unsupported.
};
struct Context {
 uint32_t estate_id=0,estate_type=0,region_id=0;
 AccessPolicy::EffectiveAccess access;
 bool authoritative=false; // Caller has verified current custody/catalog/ACL context.
 bool mount_contact_verified=false,geometry_only=false; // Read/preflight skips do not authorize mutations.
};
struct Transform { float heading=0,pitch=0,roll=0,scale=1;std::array<float,3> position{}; };
struct Result {Error error=Error::None;std::array<Point,8> world_corners{};explicit operator bool()const{return error==Error::None;}};
// Exact native7f5e30 dispatch for established types1..8. Type4 ->8, not16.
inline uint32_t EstateFlag(uint32_t type){return type>=1&&type<=8?uint32_t{1}<<(type-1):0;}
inline std::array<double,9> Rotation(float heading,float pitch,float roll){
 // Native4b2c89 uses float literals1/256 and pi before7a32b0. Angles512=turn.
 const double pi=double(3.1415927410125732421875f);
 const double h=double(float(double(heading)*pi/256)),p=-double(float(double(pitch)*pi/256)),r=double(float(double(roll)*pi/256));
 const double ch=std::cos(h),sh=std::sin(h),cp=std::cos(p),sp=std::sin(p),cr=std::cos(r),sr=std::sin(r);
 // Row-vector rotation, independently recovered x87 stores7a32df..7a3336.
 return {cp*ch,cp*sh,-sp,sr*sp*ch-cr*sh,cr*ch+sr*sp*sh,sr*cp,cr*sp*ch+sr*sh,sh*cr*sp-ch*sr,cp*cr};
}
inline Point Rotate(Point v,const std::array<double,9>& m){return {v[0]*m[0]+v[1]*m[3]+v[2]*m[6],v[0]*m[1]+v[1]*m[4]+v[2]*m[7],v[0]*m[2]+v[1]*m[5]+v[2]*m[8]};}
inline Result Check(const Context& context,const ItemMetadata& item,const Geometry& geometry,
 const RegionWire::Region& region,const Transform& transform,Operation operation){
 auto bad=[](Error e){Result r;r.error=e;return r;};
 if(!context.authoritative||!context.estate_id||context.estate_id==0xffffffff||!context.region_id||context.region_id!=region.id||
  (context.estate_type!=2&&context.estate_type!=3&&context.estate_type!=4&&context.estate_type!=8))return bad(Error::InvalidContext);
 if(geometry.mounting_type&&((geometry.mounting_type!=1&&geometry.mounting_type!=2)||(!context.geometry_only&&!context.mount_contact_verified)))return bad(Error::UnsupportedGeometry);
 if(operation!=Operation::Add&&operation!=Operation::Move)return bad(Error::InvalidContext);
 const bool npc=geometry.npc_height>0;
 const bool house=item.house_definition_type==2||item.house_definition_type==3;
 const bool crate=item.house_definition_type==7;
 if((item.furniture_only&&item.house_definition_type)||(!item.furniture_only&&!house&&!crate&&!npc)||!item.item_id||item.item_id==0xffffffff||!item.model_id||item.model_id!=geometry.model_id||!geometry.native_collision_verified)return bad(Error::UnsupportedGeometry);
 if(npc&&(item.furniture_only||item.house_definition_type||geometry.center!=Point{0,0,geometry.npc_height}||geometry.extents!=Point{1,1,geometry.npc_height}))return bad(Error::UnsupportedGeometry);
 if(house&&((item.house_definition_type==2&&context.estate_type!=8)||(item.house_definition_type==3&&context.estate_type!=4)))return bad(Error::InvalidContext);
 if(!item.placement_mask||(item.placement_mask&~uint32_t(0xff))||!(item.minimum_scale>0)||!std::isfinite(item.minimum_scale)||
  !std::isfinite(item.maximum_scale)||item.maximum_scale<item.minimum_scale)return bad(Error::InvalidMetadata);
 const auto capability=operation==Operation::Move?AccessPolicy::Capability::MoveItem:(house?AccessPolicy::Capability::AddHouse:crate?AccessPolicy::Capability::AddCrate:AccessPolicy::Capability::AddItem);
 if(!(item.placement_mask&EstateFlag(context.estate_type))||!context.access.Allows(capability))return bad(Error::Denied);
 for(double v:geometry.center)if(!std::isfinite(v))return bad(Error::UnsupportedGeometry);
 for(double v:geometry.extents)if(!(v>0)||!std::isfinite(v))return bad(Error::UnsupportedGeometry);
 // Two-ULP center and extent enclosures sum to at most four epsilon times
 // the maximum component magnitude. Retain their full conservative uncertainty;
 // this cap validates trusted geometry, not submitted packet dimensions.
 // Original large house models have one-ULP cache differences larger than
 // the chair's fixed 1e-6 envelope. Bound evidence uncertainty by float32
 // precision at this model's size, rather than rejecting verified houses.
 // This does not enlarge legal regions: uncertainty is still rejected by the
 // corner boundary guard below, and the value is server-owned evidence.
 double model_magnitude=1;
 for(size_t axis=0;axis<3;++axis)model_magnitude=std::max({model_magnitude,std::abs(geometry.center[axis]),geometry.extents[axis]});
 const double maximum_local_error=std::max(0.000001,4*double(std::numeric_limits<float>::epsilon())*model_magnitude);
 if(!std::isfinite(geometry.measured_local_error)||geometry.measured_local_error<0||geometry.measured_local_error>maximum_local_error)return bad(Error::UnsupportedGeometry);
 for(float v:{transform.heading,transform.pitch,transform.roll,transform.scale,transform.position[0],transform.position[1],transform.position[2]})if(!std::isfinite(v))return bad(Error::InvalidTransform);
 for(float angle:{transform.heading,transform.pitch,transform.roll})if(angle<0||angle>512)return bad(Error::InvalidTransform);
 if(transform.scale<item.minimum_scale||transform.scale>item.maximum_scale||!(transform.scale>0))return bad(Error::InvalidTransform);
 try{RegionWire::Detail::ValidateMatrix(region.matrix);}catch(...){return bad(Error::InvalidContext);}
 const auto& m=region.matrix;double a=m[0],b=m[1],c=m[2],d=m[4],e=m[5],f=m[6],g=m[8],h=m[9],i=m[10];
 const std::array<double,9> adj{e*i-f*h,c*h-b*i,b*f-c*e,f*g-d*i,a*i-c*g,c*d-a*f,d*h-e*g,b*g-a*h,a*e-b*d};
 const double determinant=a*adj[0]+b*adj[3]+c*adj[6];if(!std::isfinite(determinant)||!determinant)return bad(Error::InvalidContext);
 std::array<double,9> inverse{};for(size_t k=0;k<9;++k)inverse[k]=adj[k]/determinant;
 double matrix_norm=0,inverse_norm=0;for(size_t row=0;row<3;++row){double aa=0,bb=0;for(size_t column=0;column<3;++column){aa+=std::abs(m[4*row+column]);bb+=std::abs(inverse[3*row+column]);}matrix_norm=std::max(matrix_norm,aa);inverse_norm=std::max(inverse_norm,bb);}
 const double condition=matrix_norm*inverse_norm;
 // Deliberate supported numeric envelope, not a claim of native bit identity.
 // Diagonal affine boxes have no cross-axis cancellation in their inverse.
 // Permit the measured guild corridor's67:1 aspect ratio while retaining a
 // bounded128 envelope and the existing condition-scaled boundary uncertainty.
 // General rotated/sheared matrices keep the previous64 envelope.
 const bool axis_aligned=b==0&&c==0&&d==0&&f==0&&g==0&&h==0;
 if(!std::isfinite(condition)||condition>(axis_aligned?128:64))return bad(Error::InvalidContext);
 for(size_t axis=0;axis<3;++axis)if(std::abs(double(m[12+axis]))>65536)return bad(Error::InvalidContext);
 // Native NPC711a envelope is axis-aligned regardless of graphics orientation.
 const auto rotation=npc?Rotation(0,0,0):Rotation(transform.heading,transform.pitch,transform.roll);const auto geometry_scale=npc?1.f:transform.scale;Result out;
 // Numeric margin is supplied from measured server evidence, transformed through
 // inverse region coefficients. Never trust the submitted711a vectors.
 Point world_margin{};for(size_t axis=0;axis<3;++axis)world_margin[axis]=geometry.measured_local_error*geometry_scale*(std::abs(rotation[axis])+std::abs(rotation[3+axis])+std::abs(rotation[6+axis]));
 for(size_t corner=0;corner<8;++corner){Point local{};for(size_t axis=0;axis<3;++axis)local[axis]=(geometry.center[axis]+((corner>>axis)&1?geometry.extents[axis]:-geometry.extents[axis]))*geometry_scale;
  auto world=Rotate(local,rotation);for(size_t axis=0;axis<3;++axis){world[axis]+=transform.position[axis];if(!std::isfinite(world[axis])||std::abs(world[axis])>65536)return bad(Error::InvalidTransform);}out.world_corners[corner]=world;
  for(size_t axis=0;axis<3;++axis){double value=0,margin=0,rounding=0,magnitude=0;for(size_t j=0;j<3;++j){const double coefficient=inverse[j*3+axis];value+=(world[j]-m[12+j])*coefficient;margin+=world_margin[j]*std::abs(coefficient);
    const float rounded=float(world[j]);const double ulp=std::max(std::abs(double(std::nextafter(rounded,std::numeric_limits<float>::infinity()))-rounded),std::abs(double(std::nextafter(rounded,-std::numeric_limits<float>::infinity()))-rounded));
    rounding+=16*ulp*std::abs(coefficient);magnitude+=std::abs((world[j]-m[12+j])*coefficient);}
   if(!std::isfinite(value)||std::abs(value)>1+margin)return bad(Error::OutsideRegion);
   // Reject the uncertainty band rather than accepting a client-valid anchor
   // whose float32 corner/inverse result may cross a boundary. Measured model
   // error is additional uncertainty, not permission to place outside. This
   // 16-ULP/256-epsilon guard is a conservative server policy, not a recovered
   // Live epsilon or a formally proved enclosure of every x87 renderer path.
   const double numeric_guard=rounding+256*std::numeric_limits<float>::epsilon()*condition*(1+magnitude);
   if(std::abs(value)+margin+numeric_guard>=1)return bad(Error::NumericallyAmbiguous);
  }
 }
 return out;
}
// Native4b2bf0/8d2b00: one allowed region must contain ALL eight corners.
// Do not accept corners distributed between regions or enable a root fallback.
// The original authenticated context and ACL are retained for every candidate.
inline Result CheckAny(const Context& context,const ItemMetadata& item,const Geometry& geometry,
 const RegionWire::Region& primary,const std::vector<RegionWire::Region>& additional,
 const Transform& transform,Operation operation){
 if(additional.empty())return Check(context,item,geometry,primary,transform,operation);
 auto invalid=[](){Result r;r.error=Error::InvalidContext;return r;};
 if(context.region_id!=primary.id)return invalid();
 std::vector<RegionWire::Region> regions{primary};regions.insert(regions.end(),additional.begin(),additional.end());
 try{RegionWire::EncodeBootstrap(regions);}catch(...){return invalid();}
 Result accepted,last;last.error=Error::OutsideRegion;bool found=false;
 for(const auto& region:regions){
  auto candidate=context;candidate.region_id=region.id;auto checked=Check(candidate,item,geometry,region,transform,operation);
  if(checked){if(!found)accepted=checked;found=true;}
  else if(checked.error==Error::OutsideRegion||checked.error==Error::NumericallyAmbiguous)last=checked;
  else return checked;
 }
 return found?accepted:last;
}

}
#endif
