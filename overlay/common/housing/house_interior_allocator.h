#ifndef EQEMU_HOUSING_HOUSE_INTERIOR_ALLOCATOR_H
#define EQEMU_HOUSING_HOUSE_INTERIOR_ALLOCATOR_H
#include "house_binding_service.h"
namespace Housing::HouseInteriors {
class AllocationPolicy {
public:
 virtual ~AllocationPolicy()=default;
 // Required server review, under the same genuine fence/transaction: reviewed
 // original template/room content, fresh operation authority, exact before
 // binding/properties/adoption, and immutable retained interior manifest.
 // This interface supplies no permissive policy or production route dataset.
 virtual bool ReviewedUnderTransaction(Database&,LiveItems::InventorySessionFence&,
   const HouseBinding::Binding&,const LiveItems::Request&,const ContentBinding::HouseRoute&)const=0;
 // A carried, previously bound house may change its parent only after an
 // explicit destination review. Retained room identity/assets remain intact.
 virtual bool ReparentReviewedUnderTransaction(Database&,LiveItems::InventorySessionFence&,
   const HouseBinding::Binding&,const LiveItems::Request&,const ContentBinding::HouseRoute&,
   uint32_t /*old_parent*/)const{return false;}
 // Separate approval for binding the retained room to a different catalog or
 // revision. It does not authorize changing any permanent room identity.
 virtual bool CatalogRelocationReviewedUnderTransaction(Database&,LiveItems::InventorySessionFence&,
   const HouseBinding::Binding&,const LiveItems::Request&,const ContentBinding::HouseRoute&)const{return false;}
};
// Called from CustodyContext::AllocateInterior AFTER real StorePose and BEFORE
// FinishStore. Never starts/commits/rolls back or creates a second connection.
// On failure caller MUST roll back its entire parent operation. First allocation
// creates a permanent instance and native child; retained same-plot placement
// returns the identical room tuple. Explicitly reviewed same-catalog reparenting
// changes the native graph in this Store transaction, preserving room contents.
// Catalog relocation additionally requires explicit provenance review and an
// identical retained house definition in the two reviewed catalog payloads.
Result<HouseBinding::Interior> AllocateUnderTransaction(Database&,
 const HouseBinding::Binding&,const LiveItems::Request&,LiveItems::InventorySessionFence&,
 const ContentBinding::HouseRoute&,const AllocationPolicy&,const Options&);
}
#endif
