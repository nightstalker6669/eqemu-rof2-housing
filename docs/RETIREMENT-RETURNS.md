# Property retirement and returns

Supported personal and guild-property retirement disposes of the entire reviewed property in one transaction. Belongings return to their original owners, unused copper escrow returns to the reviewed property recipient, and the purchase price remains spent. Ownership is released only after every source and the financial result have been sealed.

Furniture and houses can span several generated moving crates within reviewed limits. Existing exterior and interior crates are validated against physical and typed nonplaceable membership, emptied, and returned with their original identity. An interior crate returns separately before its enclosing house is packed, respecting the existing prohibition against packing a house containing a placed moving crate. Same-owner room contents keep their permanent interior and scope; foreign-owned room objects return to their own creators.

Recognized NPC furniture now participates using current model metadata, content/geometry binding and exact native frame validation. Legacy fixed-eye NPCs may legitimately have no frame. Yard NPCs and foreign room NPCs return directly; same-owner room NPCs retain their frame inside the returned house. Independent original room files, assets and persisted profile equality are required. Unknown models or stale frames are refused.

## Parcel overflow

Optional migration037 adds native parcel overflow to exact batch reviews through `allow_parcel_overflow=1`. Existing policies remain inventory-only by default. Direct items and existing or generated moving crates can overflow to parcels. Native capacity honors `Parcel:ParcelMaxItems`; a full parcel box defers the whole retirement. Nothing is discarded or released early.

A durable sidecar preserves the exact canonical item row, NULL flags, custom/ornament data, source witness, original recipient and typed crate identity. Queued crates use location3 and allocation version3, without fabricated inventory slots or custody roots. Existing placed/carried allocation encodings and earlier settlement routes remain unchanged.

The native parcel list displays the exact reconstructed item. Claim authenticates recipient, durable parcel ID, native slot/template, current inventory lease and the original retirement seal. The normal inventory barrier stays held while an exact item and packet are staged, SQL inserts the canonical saved row, crate identity becomes carried, the native parcel is deleted, and an immutable claim postimage is written. Claimed sidecars remain tombstones; retries cannot fall through to template-only retrieval or duplicate-item deletion. No second inventory save is performed after commit.

Uncertain commit is resolved on a fresh connection under a stable sidecar lock before rollback can be assumed. A postcommit memory invariant failure uses the existing fail-stop mechanism. Certified process-death recovery and normal inventory admission load the committed result; the claim journal does not falsely certify a separate live-memory acknowledgement. Generated GUID0 is preserved until the normal exact admission handoff.

## Evidence and boundaries

The fresh original-client workflow passed on a disposable personal property: the real world timer retired it with supported inventory slots filled, refunded17 copper and queued a17-count vault stack plus a generated nonempty crate. Both appeared in the native parcel window and were retrieved. Normal logout/login then preserved the typed crate, its original chair member and valid settlement proof, including the normal GUID0 admission handoff. Temporary fillers, simulated age, activation reviews, bonus stack and test refund were cleaned up; original housing and wallet contents were conserved. The generated crate remains available for review. No new placement/unpack playthrough for this follow-up or native NPC/item-variant matrix is claimed.

Private actual-database checks covered personal/guild mixed owners, canonical quantities and metadata, original typed crates, multiple nonempty generated bins, empty/populated interior crates, NPC/frame retention, exact escrow refund and plot release. Full inventory/parcel capacity, stale sources/frames, wrong recipient/lease, occupied claim slots and late settlement/claim failures preserved the transaction. Replay/tombstones and fresh-connection outcome resolution passed. Existing recovery was exercised with synthetic supervisor certificates; no real process-death acceptance is claimed.

The original server received matching world, zone, UCS and inventory-recovery binaries plus the additive schema. Loaded process hashes were checked. Existing properties, wallets and belongings were conserved before the separately identified disposable native workflow. No global retirement activation is included.

Augments, evolving items and filled bags remain refused at supported custody ingress. Missing historical Live inactivity/refund rules, alternate currencies, later partial-stack lineage and destroyed typed-container proofs are separate unresolved work. These results establish the configured supported retirement path, not every historical Live rule or every item variant.

See [batch configuration](../overlay/common/housing/offline_return_batches.md), [earlier mixed returns](history/MIXED-RETURNS-2026-10-18.md), and [vault-aware Move Out](history/VAULT-MOVE-OUT-2026-10-19.md). No fixtures, tests, private logs, credentials or game assets are included.

## Empty property with a carried retained room, October 21

A picked-up house keeps its permanent interior and historical native parent.
The shared personal/guild empty-property return path now recognizes only an
exact carried identity with an empty, quiescent room; it preserves that node and
completed placement history for later reparenting. Placed, nonempty, pending or
uncertain children still refuse. The checked guild service completed this case
without item delivery or refund and with exact conservation. The administrative
review advanced only its callsite maintenance time, without changing player login
timestamps or the global clock. See [variant implementation and scope](history/HOUSE-VARIANTS-2026-10-21.md#retirement-of-an-empty-plot-after-house-pickup).
