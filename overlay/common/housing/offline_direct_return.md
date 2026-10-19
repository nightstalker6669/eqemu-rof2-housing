# Trusted offline direct item returns

`offline_direct_return_db.h` supplies two server-only primitives. Neither starts,
commits or rolls back a transaction. Install the optional
`2026_10_18_housing_offline_direct_returns.sql` migration before use. The empty
receipt table enables no world job, permission, retirement policy or item grant.

`Housing::OfflineDirectReturns::Return(db, estate, custody, expected_owner)`
returns a supported non-placeable shed/closet item or ordinary static furniture
to its ORIGINAL custody owner. A foreign room object can therefore leave a hall
without being reassigned to the hall owner. Houses, crate tokens, crate members,
NPC frames, augmented and evolving items are refused by this primitive.

`ReturnCrate(db, estate, custody, expected_owner)` returns an EXISTING EMPTY
EXTERIOR moving crate to its actual creator. It does not extract members. The
caller must first validate and detach/repack any prior members in the same
transaction. Host-room crates are refused. Typed `CrateBinding` readers verify
the actual placed allocation, zero members, definition and current parent
catalog/generation. Return retains the crate/native identities, increments its
revision once, moves its location to carried, makes the native child inactive,
closes the old root placement and verifies the actual carried allocation.

The caller must authenticate its exact generation/catalog-bound disposition
policy, lock all affected accounts/characters in a consistent order, and keep
ALL recipient work and the final retirement/disband journal in ONE transaction.
On ANY exception it MUST roll back that entire transaction. A later per-item
failure can occur after earlier items were written. This is deliberately not a
partial-success API. Do not catch a failure and commit earlier returns. Receipt
existence also refuses replay; the owning job must use its generation journal
to resolve a known committed aggregate outcome.

Each helper asserts an existing transaction, locks the recipient's actual
undeleted character/account, checks offline inventory sessions and live custody
leases, and checks item/coin/house pending barriers. The caller's aggregate
barrier remains necessary across all recipients and house interiors. These
primitives convey no client fence or packet authority.

The ordinary item path validates the exact canonical custody/schema/catalog
and the current vault or room/yard reservation/scope graph. Retained room
custody can retain its original estate after a house moves: the reservation
must match that original source estate, while the actual room host/root belongs
to the CURRENT disposition estate. Shed/closet realm and current physical
plot/host checks remain mandatory. The immutable source estate is recorded in
the custody source witness; receipt estate_id identifies the current batch.
No ownership/source row is rewritten to manufacture current-estate equality. Vault metadata uses
the existing native bNoStorage, class and non-placeable rules. Static furniture
must be in the reviewed registry. Stack counts are not split, merged or
normalized: supported nonempty stacks must fit their actual metadata capacity.
Raw ornaments, custom bytes (including NUL), GUID and NULL flags are conserved;
only destination slot changes. Empty bag/book support remains the existing
single-row vault payload supported by the Store path; this does not add nested
bag or augmentation serialization. Evolution and unsupported payloads defer the
whole disposition.

Destination selection considers saved inventory and typed house/crate
locations, uses only clear top-level slots23 through30, and refuses slots with
bag children. There is no cursor, bank, parcel, overflow or item deletion
fallback. NoDrop items stay with their original owner. Lore checks conservatively
include that owner's inventory, account shared bank and other owned housing
custody. Nonzero GUID collisions are checked across inventory/shared bank and a
bounded locked custody scan. Oversized/unknown graphs defer.

`Outcome` includes `custody_id`, `owner`, `account`, `destination_slot`, `vault`,
canonical `before`/`after` inventory rows and framed `source_state`. Crate returns
add nonzero `crate_id` and canonical `allocation_before`/`allocation_after` bytes.
Ordinary returns leave these crate fields empty. `Failure::error` distinguishes
Deferred, Conflict, Unsupported and DatabaseFailure; callers should also roll
back unexpected standard/database exceptions.

`housing_offline_return_items` is immutable application audit keyed by
`(estate_id,custody_id)` with original recipient/account, source kind, native
item, optional room, complete rows, framed source/metadata graph, SHA256
witnesses and optional typed crate allocations. It intentionally has no FK to
the custody that was deleted. Closed placement reservations and interior
preview history remain as historical identity; they are not live custody.
No simulated live request, client ACK or lease is manufactured.
