#include "housing_house_surface.h"
#include "housing_house_surface_data.h"
#include "../common/housing/house_geometry_registry.h"
#include <cstring>
#include <vector>
namespace Housing::HouseSurface {
std::optional<Provider::Identity> Provider::Identify(const HouseBinding::Binding& house,const LiveItems::Stored& row){
 if(!house.house_id||!house.revision||!house.pending_key.empty()||!row.custody_id||row.stowed||!row.placement||
  house.location.kind!=HouseBinding::Location::Kind::Custody||house.location.custody_id!=row.custody_id||
  house.item_template_id!=row.row.values.item_id||!row.estate_id||row.estate_id!=row.placement->reservation.estate_id)return {};
 const auto& p=*row.placement;const auto& r=p.reservation;
 if(!p.pose_revision||!r.native_estate_id||r.native_estate_id==UINT32_MAX||!r.native_item_id||r.native_item_id==UINT32_MAX||
  r.catalog_key.empty()||!r.catalog_revision||r.catalog_revision!=row.current_catalog_revision||!r.content_binding||r.content_binding->version!=ContentBinding::HouseVersion)return {};
 for(float v:{p.transform.heading,p.transform.pitch,p.transform.roll})if(!std::isfinite(v)||v<0||v>512)return {};
 if(!PlacementContact::Finite(p.transform.position)||!std::isfinite(p.transform.scale)||p.transform.scale<=0||p.transform.scale>16)return {};
 Identity id;id.house=house.house_id;id.house_revision=house.revision;id.custody=row.custody_id;id.pose_revision=p.pose_revision;id.catalog_revision=r.catalog_revision;id.storage_revision=row.storage_revision;
 id.estate=row.estate_id;id.native_estate=r.native_estate_id;id.native_item=r.native_item_id;id.item=house.item_template_id;id.catalog=r.catalog_key;id.content=*r.content_binding;id.transform=p.transform;return id;
}
std::unique_ptr<Provider> Provider::Build(const HouseBinding::Binding& house,const LiveItems::Stored& row){
 auto identity=Identify(house,row);if(!identity)return {};
 const HouseGeometry::Record* record=nullptr;for(const auto& r:HouseGeometry::Records)if(r.item_id==identity->item)record=&r;
 if(!record)return {};
 const Data::Mesh* source=nullptr;for(const auto& m:Data::Meshes)if(m.model==record->model_id)source=&m;
 if(!source||source->vertices!=record->vertices||source->triangles!=record->triangles||
  std::strcmp(source->member_sha,record->member_sha256)||std::strcmp(source->positions_sha,record->ordered_positions_sha256)||std::strcmp(source->indices_sha,record->ordered_indices_sha256))return {};
 const auto& pose=identity->transform;const auto rotation=PlacementPolicy::Rotation(pose.heading,pose.pitch,pose.roll);std::vector<float> world;world.reserve(source->vertices*3);
 for(uint32_t i=0;i<source->vertices;++i){
  PlacementPolicy::Point local;for(unsigned j=0;j<3;++j)local[j]=double(source->positions[3*i+j])*pose.scale;
  auto native=PlacementPolicy::Rotate(local,rotation);for(unsigned j=0;j<3;++j){native[j]+=pose.position[j];if(!std::isfinite(native[j])||std::abs(native[j])>65536)return {};}
  // Existing authoritative pose is native Y,X,Z; Map::DoCollisionCheck and
  // server query positions are X,Y,Z. Swap once AFTER native rotation.
  world.push_back(float(native[1]));world.push_back(float(native[0]));world.push_back(float(native[2]));
 }
 auto result=std::unique_ptr<Provider>(new Provider);result->identity_=*identity;
 result->mesh_.reset(createRaycastMesh(source->vertices,world.data(),source->triangles,source->indices));if(!result->mesh_)return {};return result;
}
bool Provider::Matches(const HouseBinding::Binding& house,const LiveItems::Stored& row)const{
 auto current=Identify(house,row);if(!current)return false;const auto&a=identity_;const auto&b=*current;
 return a.house==b.house&&a.house_revision==b.house_revision&&a.custody==b.custody&&a.pose_revision==b.pose_revision&&
  a.catalog_revision==b.catalog_revision&&a.storage_revision==b.storage_revision&&a.estate==b.estate&&a.native_estate==b.native_estate&&a.native_item==b.native_item&&a.item==b.item&&a.catalog==b.catalog&&ContentBinding::Same(a.content,b.content)&&
  a.transform.heading==b.transform.heading&&a.transform.pitch==b.transform.pitch&&a.transform.roll==b.transform.roll&&a.transform.scale==b.transform.scale&&a.transform.position==b.transform.position;
}
std::optional<PlacementContact::Contact> Provider::Trace(const HouseBinding::Binding& house,const LiveItems::Stored& row,const PlacementContact::Vector& start,const PlacementContact::Vector& end)const{
 if(!Matches(house,row)||!PlacementContact::Finite(start)||!PlacementContact::Finite(end))return {};
 float hit[3],normal[3],distance;if(!mesh_->raycast(start.data(),end.data(),hit,normal,&distance))return {};
 return PlacementContact::FromMapRay(start,end,{normal[0],normal[1],normal[2]},distance);
}
}
