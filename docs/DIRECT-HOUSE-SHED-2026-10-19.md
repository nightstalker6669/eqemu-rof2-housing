# Direct furnished-building Shed storage

An unoccupied reviewed personal house or guild hall can now move directly from Placed to Shed storage on its current plot. Its HouseID, permanent interior, original contents, closet/vault scopes, custody and furnishing poses are retained. The operation changes the building's storage state; it does not unpack its contents into the yard or allocate a replacement room.

Leave the interior, open the plot's Items window, select the building and click Shed. To restore it, use Place or an applicable saved yard layout. The existing restore path retains the original room and enforces current placement geometry and the one-placed-house rule.

## Authorization and safety

This implemented profile requires the building's immutable creator operating from the actual exterior, with current Move Item, Add House and Remove House capabilities. Parent/house/catalogue/route and custody identities are reread under the genuine inventory fence. Delegated or foreign-creator building storage is not newly enabled.

Before the storage flag changes, the service reads the reviewed attached room and checks its stable house revision. Independently enumerated persisted scopes must match the complete room scene, including closet, crate and vault categories. Occupants, pending house/room work, missing or duplicate custody and incomplete schemas refuse the operation. The checks and existing storage receipt/CAS share the owning transaction. No partial content migration occurs.

Successful Shed storage clears only a retained placement preview for that exact custody item; failed writes retain their retry state. Container/estate relationships and item scenes refresh after building storage. Stored-building entry remains refused by the existing admission checks. No opcode, wire struct or database migration was added.

## Acceptance scope

The deployed build completed one native Standard Guild Hall round trip. Shed was refused from inside the hall without changing its storage state. From the exterior, Shed immediately changed the furnished hall to Yard/Stored. The original client then restored it through Load Yard using a reviewed one-building layout and reported successful loading; the immediate Items list showed the hall placed and its original interior chair retained. Persisted comparison confirmed the same HouseID, permanent room, complete retained custody scope, exact item bytes and exact furniture pose. The client closed before a fresh entry observation could be completed, so post-restoration re-entry is not newly claimed by this check. Earlier normal-entry acceptance remains documented separately.

The production build passed. Private transaction checks covered active occupancy, missing/duplicate custody and pending work, with temporary changes rolled back. Those checks do not establish native second-client occupancy behavior. This is the implemented compatibility profile; it does not establish all historical Live storage rules or acceptance of every building model, contributor or pending-operation variant.
