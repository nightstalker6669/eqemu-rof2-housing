# Evolving item custody

Housing storage carries evolving identity separately from the ordinary inventory
GUID. The canonical versioned payload in `evolving_item_custody.h` preserves the
existing `character_evolving_items.id`, owner, item, activation/equipment flags,
amount, persisted progression bits, runtime progression bits, final item and SQL
NULL markers. Components 1–6 represent actual augment sockets; bag children have
separate inventory-row payloads and custody paths 1–200, never augment identities.
The root uses path zero; all native RoF2 bag slots participate in held-save preflight.
Ordinary rows without extended state retain their original version 1 encoding.

`evolving_item_custody_db.h` requires the caller's existing fenced InnoDB item
transaction. `Capture` and `ValidateInventory` are read-only: they authenticate
an actual runtime UID, matching physical component, exact CEI state and durable
locator. Unbound legacy identities are accepted only when the physical component
and active CEI identity are unique; duplicate templates are not assigned arbitrary
UIDs. `StoreHold` runs only after that validation and the inventory memory fence.
It parks the same CEI at character zero with activation/equipment disabled;
it does not soft-delete the identity or reset its amount. The immutable payload
retains original values and `housing_evolving_item_locations` binds the UID to
its exact custody ID, child path and component. There are deliberately no foreign
keys to the core REPLACE-written character or evolving tables.

Every held read must call `ValidateHeld`. Packing, retirement and parcel delivery
must retain that original custody/path identity until release. `ReturnRelease`
authenticates it and atomically restores the same CEI UID and state to the target
inventory owner, returning an owner-normalized presentation payload. It does not
require the destination inventory row to exist before the caller inserts it.
No new evolving UID is allocated for a housing return. Runtime reconstruction
uses `Restore`; `Matches` checks its exact postimage before a memory fence ends.
Unknown commit outcomes must freeze the existing inventory session and use the
ordinary recovery protocol; they must not replay an unverified second release.

The ordinary loader resolves tracked UID/slot/component before considering a
unique legacy template, and rejects missing tracked identities. Save preflight
checks the entire bag before any container clearing, and ordinary inventory saves
update the locator without replacing CEI identity. The derived
`CharacterEvolvingItemsRepository::UpdateOne` uses an atomic locking join to
reject ordinary XP, equipment, transfer and soft-delete writes against held UIDs.
The toggle handler additionally requires the exact live inventory UID and owner;
foreign or held packet IDs cannot activate an item. Base generated repositories
are unchanged. Deploy the locator migration before enabling extended custody.

This persistence support grants no new item model, trophy rank, activation
benefit, minimum level or cost. Existing reviewed geometry and placement guards
still decide which evolving physical items can be placed. Legacy `IsScaling` state remains separate from CEI. The legacy scaling persistence
extension now preserves root and augment experience in per-instance custom_data;
see legacy_scaling_state.h and docs/ITEM-SUPPORT.md in the housing package.
The reviewed final tradeskill physical trophies use CEI identity. Unknown trophy
benefits remain disabled.
