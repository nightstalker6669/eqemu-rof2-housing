# Evolving trophy custody

Evolving tradeskill trophies remain unsupported by housing custody. Their reviewed
models do not establish safe item persistence. Do not remove the evolving-item
guards to enable their placement or advertise their housing benefits as usable.
Ordinary inventory use and evolution remain available through EQEmu's existing
item system.

The reviewed final physical items are `95014`, `95021`, `95028`, `95035`, `95042`,
`95049`, `95056`, `95063`, `95070`, and `95077`. Their model entries are in
[`static_furniture_rigid_extension.inc`](static_furniture_rigid_extension.inc).
[`StaticFurniture::Lookup`](static_furniture_registry.h) accepts only pinned
resource classes, geometry revisions and models. This allowlist grants geometry
authority, not progression custody or a trophy benefit. Lower ranks and other
evolving items must not gain support through a blanket exception.

## Persistence gap

[`LiveItems::InventoryRow`](live_item_service.h) and the version 1
[`CustodyCodec`](custody_codec.h) preserve the 17 inventory columns and nullable
field markers. They contain no evolving-item identity or progression snapshot.
[`InventoryRowDB`](inventory_row_db.h) reads and writes those same columns.

The separate `character_evolving_items` record contains its own `id`, owner,
item, activation/equipment state, current amount, progression, final item ID and
deletion timestamp. A runtime item's `GetEvolveUniqueID()` refers to that record;
it is not its inventory GUID or the housing custody ID. See
[`ItemInstance`](../item_instance.h) and
[`CharacterEvolvingItemsRepository`](../repositories/character_evolving_items_repository.h).

[`SharedDatabase::GetInventory`](../shareddb.cpp) loads active evolving records
and associates them with inventory items by item ID. Leaving a placed item's
record active can let another inventory copy receive that identity. Merely
soft-deleting it is insufficient as a durable housing ownership protocol:
`SetEquipped`, `SetCurrentAmountAndProgression`, and `UpdateCharID` in the extended
repository clear `deleted_at`. A housing hold would need enforcement at every
relevant writer and loader. Maximum rank does not remove these identity and
state-preservation requirements.

## Enforcement points

| Path | Current enforcement or missing state |
| --- | --- |
| Native placement | `FurnitureModel` in [`client_housing.cpp`](../../zone/client_housing.cpp) rejects `EvolvingItem`; `FurnitureNpc` independently rejects it. The house/crate model guards serve different item classes and must stay strict. |
| Runtime projection and replay | [`InventoryProjection::Matches`](inventory_projection.h) rejects `IsEvolving()`. [`InventoryBridge::Reconstruct`](../../zone/housing_item_reconstruction.cpp) reconstructs inventory fields only. `Bridge::SourceMatches`, `Apply`, `Applied`, and `ReturnAllowed` depend on this projection. |
| Single-item custody | `Simple` in [`live_item_service.cpp`](live_item_service.cpp) rejects evolving definitions and active evolving records. `Operate` journals inventory rows, and `ValidateRestartOutcomeCurrent` verifies those rows after an uncertain commit or process restart. Neither authenticates evolving state. |
| Layout inventory sources | `Metadata` and `ReviewSourcesUnderTransaction` in [`layout_inventory_units.cpp`](layout_inventory_units.cpp) independently reject evolving definitions/records. `StoreLayoutInventoryUnderTransaction` uses `Simple`. Layout source snapshots, consumption and aggregate recovery would need the same identity binding. |
| Offline direct returns | [`OfflineDirectReturns::Return`](offline_direct_return_db.h) independently rejects `evoitem`, evolving definitions and active evolving records before creating inventory or a parcel. |
| Furnished retirement and retained cargo | [`offline_property_delivery_db.h`](offline_property_delivery_db.h) and [`offline_return_batch_db.h`](offline_return_batch_db.h) independently reject evolving metadata before packing or retaining ordinary furniture. Retained house contents and crate members must carry the same evolving ownership proof as directly returned items. |
| NPC and typed containers | [`offline_npc_return_db.h`](offline_npc_return_db.h) rejects evolving NPC sources. House binding, house inventory transitions and preload also reject them. These are not tradeskill trophy admission paths and must not be broadly relaxed. |
| Parcel claim | [`ReturnParcels::QueueUnderTransaction` and `ClaimUnderTransaction`](return_parcel_db.h) conserve inventory row bytes; no evolving identity or state is currently delivered. |
| Retirement conservation | [`ReturnSettlement::Capture`, `Seal`, and `VerifyUnderTransaction`](return_settlement_db.h) cover custody rows, typed locations and delivery witnesses. Those witnesses cannot prove conservation of a separate evolving record today. |

`InspectCustodyUnderTransaction` currently reads canonical custody and storage
state without calling `Simple`. A future evolving binding must be validated by
all custody readers, including scene, crate, room, retirement and recovery
readers; checking only the initial Store is insufficient.

## Minimum safe extension

These requirements describe the missing implementation; they are not an enabled
feature or a migration to run.

1. Define a versioned, canonical evolving snapshot alongside the unchanged raw
   inventory fields. Preserve the exact evolving ID and all state, with explicit
   range/null/finite-number rules and lossless numeric encoding. Continue reading
   old non-evolving version 1 receipts and immutable retirement seals unchanged.
2. Add a durable, unique ownership binding for the evolving ID with explicit
   inventory, custody and queued-delivery locations. Capture the exact runtime
   ID under the real inventory fence, lock its SQL record, and verify owner,
   item, final rank and state. Item-ID lookup alone, packet GUIDs, synthetic
   replicas and altered `evoitem` metadata cannot establish this binding.
3. Move inventory and evolving ownership together in the existing transaction.
   A held record must not appear as an available active inventory record, evolve,
   transfer, or be reactivated by ordinary repository writers. Preserve its
   original state separately from any storage/suspension marker. Runtime
   presentations must not become independent owners of the evolving identity.
4. Restore the same ID and state atomically with pickup, offline direct delivery
   or parcel claim. Authenticate the recipient and reject ambiguous or duplicate
   active ownership. Do not silently create a replacement evolving record on
   relog. Foreign pickup needs an explicit reviewed ownership transition; an
   ordinary furniture transfer permission is insufficient.
5. Carry this proof through layouts, shed storage, packed crates, furnished
   houses, detached rooms and generated retirement bins. Bind it into request
   journals, postimage validation, commit resolution, reconstruction and immutable
   delivery witnesses. Every path must fail before mutation if the proof is
   missing or inconsistent.

Admission should remain limited to explicitly reviewed final physical trophies
until this full path is implemented. Acceptance requires representative native
placement/pickup and benefit use plus relog, uncertain-commit/restart, containment
and retirement/parcel conservation checks of both physical and evolving
identities. Preserving only an item's visible rank or rebuilding a zero-progress
copy does not meet that requirement.
