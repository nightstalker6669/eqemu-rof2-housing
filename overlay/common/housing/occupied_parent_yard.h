#ifndef EQEMU_HOUSING_OCCUPIED_PARENT_YARD_H
#define EQEMU_HOUSING_OCCUPIED_PARENT_YARD_H
#include "scene_service.h"
#include "interior_profile_db.h"
#include "custody_codec.h"
namespace Housing::OccupiedParentYard {
using Context=LiveItems::OccupiedParentContext;
inline bool Shape(const Context& c){return c.house_id&&c.house_revision&&c.profile_revision&&c.source.zone_id&&c.source.instance_id&&c.source.instance_id<=32000;}
inline CustodyCodec::Bytes Encode(const Context& c){
 if(!Shape(c))throw std::invalid_argument("Invalid occupied parent context");
 CustodyCodec::Bytes b;for(auto v:{c.house_id,c.house_revision,c.profile_revision})CustodyCodec::W(b,v,8);
 CustodyCodec::W(b,c.source.zone_id,4);CustodyCodec::W(b,c.source.zone_version,1);CustodyCodec::W(b,c.source.instance_id,2);return b;
}
inline Context Decode(const CustodyCodec::Bytes& b){
 CustodyCodec::Reader r{b};Context c;c.house_id=r.N(8);c.house_revision=r.N(8);c.profile_revision=r.N(8);
 c.source={uint32_t(r.N(4)),uint8_t(r.N(1)),uint16_t(r.N(2))};
 if(r.at!=b.size()||!Shape(c))throw std::invalid_argument("Invalid occupied parent bytes");return c;
}
// Re-read the genuine occupied room and parent under the caller's transaction.
// Neither these supplied IDs nor parent visibility grant mutation capability;
// callers still check destination ACL, capacity, item identity, pose and CAS.
inline LiveItems::Result<Scene::Snapshot> ReadUnderTransaction(Database& db,const LiveItems::Lease& lease,const Context& c,
 LiveItems::InventorySessionFence& fence,const Scene::HouseScenePolicy* houses,const InteriorContent::ProfilePolicy* profiles,
 const CrateBinding::CustodyContext* crates=nullptr){
 using Error=LiveItems::Error;if(!Shape(c))return {{},Error::InvalidArgument};if(!houses||!profiles)return {{},Error::Unauthorized};
 auto inside=Scene::Service::ReadInteriorSceneUnderTransaction(db,lease,c.source,fence,*houses,*profiles,crates);if(!inside)return {{},inside.error};
 const auto& room=inside.value;
 if(!room.house||!room.house->interior||!room.room_profile||room.exterior_source||room.house->house_id!=c.house_id||
    room.house->revision!=c.house_revision||room.profile_revision!=c.profile_revision||
    room.context.source.zone_id!=c.source.zone_id||room.context.source.zone_version!=c.source.zone_version||room.context.source.instance_id!=c.source.instance_id)return {{},Error::Conflict};
 auto yard=Scene::Service::ReadParentSceneUnderTransaction(db,lease,c.source,fence,*houses,crates);if(!yard)return yard;
 if(yard.value.estate_id!=room.context.estate_id||yard.value.native_plot_id!=room.context.native_plot_id||
    yard.value.route.zone_id!=room.context.neighborhood.zone_id||yard.value.route.zone_version!=room.context.neighborhood.zone_version||
    yard.value.route.instance_id!=room.context.neighborhood.instance_id)return {{},Error::Conflict};
 return yard;
}
}
#endif
