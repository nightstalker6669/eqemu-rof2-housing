# Reviewed housing NPC item binding, October 21, 2026

The genuine RoF Holgresh House Pet (item77408) now has a reviewed item-to-actor
binding. The item retains its real NPC-definition key `a_holgresh`; the existing
preview and scene response sends the separate original renderer prefix `HLG`,
NPC discriminator1 and an explicit original-model-default size6. This is a
client-local housing actor, not an ordinary server `Spawn2` NPC.

## Evidence and sizing policy

The [period expansion announcement](https://www.everquest.com/news/imported-eq-enus-524410)
identifies the house pet and its appearance relationship to the Holgresh familiar.
Original `spells_us.txt` identifies the familiar illusion as race715; the matching
client's compiled prefix registration maps `HLG` to race715/gender2. Original
`Resources/OnDemandResources.txt` binds `HLG_ACTORDEF` to `hlg.eqg`/`HLG.MDS`.
The [later collector record](https://items.sodeq.org/itemraw.php?id=77408) supplies
the exact item NPC key and physical metadata. These are later collected item
values, not an authenticated May2013 item packet.

Original `racedata.txt` supplies model-default size6 for both Holgresh genders.
The constructor loads this value before the original height/ROffset arithmetic.
The native renderer subsequently uses the raw actor height for its visual scale
setter, so emitting zero is not assumed safe. The explicit model-default policy
emits6 while retaining zero/unknown in the source height field and `null` for the
historical raw height in provenance. **Historical Live item size was not
recovered.** This is an authenticated original model-default sizing policy.

[The reviewed source registry](../../overlay/common/housing/npc_reviewed_item_bindings.json)
pins the original executable, model archive, resource manifest, race defaults,
collector record, opaque respawn field4500 and selected policy. The compiled
binding and SQL seed use that exact JSON hash. The raw respawn field is preserved
on the item wire; no timing unit, server AI, wandering, interaction or respawn
scheduler is inferred from it. Unresolved named rotation/free-placement flags
remain unchanged. Black Bear Cub and Friendly Puppy are confirmed period items,
but their exact item-to-renderer associations remain unseeded.

## Runtime and durable identity

The same reviewed binding is used by fresh SQL reads and cached `ItemData`.
Both item serializers preserve `a_holgresh`, respawn4500 and default scale1.
`FurnitureActor` in the existing native Begin/preview and scene/replay paths
publishes `HLG` and size6. The original client consumes the first three actor
model characters to resolve the local race/gender; it does not need an invented
client lookup from `a_holgresh` to `HLG`.

Only the exact item/key/source hash/opaque-field/unknown-source-height tuple can
select this policy. Wrong source, altered key, zeroed respawn, a claimed source
height6, interactive records passed to this pet policy and unproved item/model pairs remain rejected.
Ordinary and legacy NPC guards remain unchanged. Content fingerprints add the
model-default domain and exact race-data/default-size witness only for this new
binding; older fingerprint bytes and permanent profile identities are preserved.
Native derived-height frames, custody, capacity, sphere/placement validation,
source withdrawal, replay and offline return use the same reviewed profile.

## Installation

Install the matching source and build common/world/zone/UCS/shared-memory tools.
Stop world, every zone and concurrent item writers. Retain a backup and inspect
any pre-existing item77408 custody using the versioned custody codec before
activation; do not rewrite retained contents to fit changed metadata.
Apply the existing NPC metadata/runtime migrations first, then
[the guarded Holgresh seed](../../overlay/utils/sql/optional/2026_10_21_housing_holgresh_npc_binding.sql).
It accepts only the exact empty physical metadata or exact reviewed postimage,
refuses conflicting source data, preserves unrelated fields and retries exactly.

Regenerate the actual shared-item dataset after the seed. Select its intended
prefix after `shared_memory` finishes, then cold-start matching processes. No
`ItemData` ABI expansion or client-file edit is required by this binding, but
stale shared-item metadata will not match the new fresh source. SQL readback
alone is not proof that a running zone uses the new item data.

The source/cache/item-wire/native-actor/frame and actual interior/exterior
canonical binding pipeline passed isolated checks. Native Holgresh outdoor
placement, camp/relog replay and pickup into inventory passed. Native indoor
placement and camp/relog replay in Hermit's Hideaway also passed with a
committed durable pose and the native Items list retaining its placed House
location; an
ordinary chair preview subsequently returned green using its canonical regions.
This document does not certify every house-pet item or captured Live size.
Interactive service contracts use the separate [service implementation](SERVICE-NPCS-2026-10-21.md),
with real server NPCs and independently authenticated metadata; they do not use
the Holgresh client-local pet path. Representative Grand hall placement and service transactions passed; the separate service document records remaining acceptance boundaries. Acceptance used an administratively provisioned character
and properties; it does not establish a fresh purchase or all non-admin roles.

## Indoor animated preview admission

The original renderer tests its animated collision box against native regions,
while the outgoing NPC request uses the original fixed ground/height envelope.
A tight room envelope can therefore reject an otherwise valid floor placement.
The [source-derived animation envelope](../provenance/housing-npc/HLG-PREVIEW-ENVELOPE.md)
covers all13 original clips and their controller/rounding behavior. A separate
[native admission policy](../../overlay/common/housing/npc_native_admission.json)
creates finite temporary presentation geometry for the reviewed Holgresh only.

Begin and Move preserve the complete native region collection and add a checked
positive ID with a preview-only allowed status. Ordinary interior previews
restore the canonical full collection, removing stale NPC projection geometry.
Estate fallback remains denied. Original authoritative room regions, ground and
collision checks, capacity, ownership, frame validation and item bindings remain
unchanged; the temporary region is never persisted or used for placement authority.
Unsupported sources, shapes or numeric ranges retain their guarded behavior.
