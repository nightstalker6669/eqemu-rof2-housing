#ifndef EQEMU_HOUSING_PLANAR_MOUNT_CONTACT_H
#define EQEMU_HOUSING_PLANAR_MOUNT_CONTACT_H
#include "placement_contact.h"
namespace Housing::PlanarMount {
// Produced only by a trusted source-map query, never decoded from a packet.
struct SurfaceSeed {uint32_t triangle_id=UINT32_MAX;int32_t side=0;};
struct Trace {PlacementContact::Contact contact;SurfaceSeed seed;};
}
#endif
