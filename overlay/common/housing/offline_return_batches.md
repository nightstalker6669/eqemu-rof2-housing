# All-recipient offline property returns

This opt-in extension returns a supported property as one transaction across all original item owners. Install the guild inactivity/origin migration first, then `2026_10_18_housing_offline_direct_returns.sql`, `2026_10_18_housing_return_settlement.sql`, and `2026_10_18_housing_mixed_return_batches.sql`. New policy tables start empty. Older personal/single-recipient guild reviews retain their prior behavior when no batch review is present.

The new batch review pins the current ownership generation, native plot, exact catalogue revision and definitions/regions hashes, the reviewed small-crate content manifest, and bounded recipient/crate/capacity limits. Existing90-day personal or30-day guild inactivity authority is independently required. Guild activity remains the documented current-roster policy; its reviewed leader receives only the unused property escrow, not other players' belongings. No purchase-price refund is introduced.

Before mutation the service captures the complete source graph: current yard custody, exterior crate members, retained house interiors and vault scopes, including foreign/original ownership generations. It locks the real recipients/accounts and requires every recipient to be offline with no unresolved item, house or coin application. Interior visitors or previews, incomplete graphs, unsupported engines and unknown identities defer the entire property.

Furniture and hall bodies are partitioned by original custody owner into generated moving crates. Bin capacity cannot exceed the actual catalogue definition, a bin holds at most one house/hall, and multiple bins are allocated as needed within reviewed bounds. Same-owner room objects remain attached to their permanent house/room. Foreign-owned room objects and all supported non-placeable vault objects are returned directly to their original owners' saved inventory. Whole stack counts, NULL/binary/custom metadata, ornaments and canonical item identity are retained. No partial-stack splitting or transfer to the guild leader is used. All saved-inventory destinations are checked ordinary slots23–30, including orphan bag-child and typed-container conflicts; without parcel overflow enabled, lack of space rolls back everything. Optional migration037 and an exact batch review with `allow_parcel_overflow=1` enable native parcel overflow; nested bag-content reconstruction is not inferred.

Supported existing exterior and interior crates are structurally validated before their members are regrouped by owner; their emptied typed roots return to their own original creators without replacing container identity. Interior crate roots return separately before their enclosing house is packed. Recognized NPC furniture uses exact current metadata, geometry and frame validation; legacy fixed-eye objects may have no frame. Same-owner room NPCs remain in their house; other NPCs return directly. Evolving, augmented and unsupported model/quantity shapes remain refused at custody ingress. Ordinary whole returned item and empty crate adapters retain immutable per-item source/postimage receipts. Generated origin4 keys bind source generation, recipient and bin ordinal, so several crates can come from one property without changing their creator after guild deletion.

An immutable conservation seal is written last in the same transaction, after all inventory delivery, packing, escrow refund and ownership release. It binds every captured source to exactly one rightful direct/crate/retained-house disposition, every generated container, complete financial conservation and the original source manifest. A late receipt failure rolls back all recipients and property state. A retired generation and its immutable receipts prevent duplicate delivery on replay; a new owner has a different generation.

Guild dissolution now accepts such settled historical cargo only when the full seal, recipient provenance, current safe custody/container path or genuine completed whole-item Return journal validates. It does not freeze later inventory slots, guild leadership or lawful whole-item movement. Corrupt/partial proofs, extra unaccounted source custody and unresolved applications still block disbanding. Unsupported later partial-stack lineage, certified-death Return reconciliation and destroyed typed containers remain conservative refusal cases. Older occupied returns without a complete source seal are not retroactively certified.

No client opcode or packet layout changes are required. The combined private actual-SQL case used a genuine Standard Guild Hall with its permanent room, two contributors, a foreign-origin room object, a17-count vault stack, yard furniture and a capacity-one review forcing three generated crates. It exercised full-inventory refusal, a second-recipient receipt fault, a final conservation-seal fault, normal GUID assignment/typed crate reload, exact refund, and actual guarded guild deletion. These are SQL/reload checks, not a new native multi-recipient client-playthrough claim. Test fixtures and logs are not publication content.

## Existing crates containing nonplaceable items

Retirement now traverses typed nonplaceable membership as well as physical
crate membership when capturing the complete property. A supported exterior
crate is validated against both member classes before any membership is
removed. Every packed vault source must occur in the captured source graph
with its unchanged row and original owner. Detachment, rightful direct item
return, empty original crate return, generated furniture bins, escrow refund,
ownership release and settlement remain one transaction.

The original crate keeps its identity and creator. Nonplaceable items return
their full quantities and canonical metadata to their original owners; they
are not converted to placed furniture or copied into the guild leader's
inventory. Direct return primitives refuse a source that is still a typed
crate member, so callers cannot leave dangling membership by deleting custody.
The existing optional migration036 supplies the membership table; its absence
preserves older physical-only deployments. A malformed/nontransactional table
refuses the operation.

`ReturnSettlement::VerifyUnderTransaction` can verify a sealed personal or
guild return against an explicit owner kind and ID. The existing guild-only
entry point remains restricted to guild ownership for dissolution. This does
not generate missing historical proofs or grant new retirement authority.

## Native parcel overflow

Apply `037_return_parcels.sql` only after the mixed-return policies/settlement tables, with housing writers stopped. It adds no activation seeds; old policies retain inventory-only behavior. Native parcel, inventory and custody tables must be InnoDB. Native slots1 through `Parcel:ParcelMaxItems` are honored; a full parcel box defers the entire retirement without discarding items or releasing ownership. Both direct returns and generated/existing typed moving crates can overflow.

A native parcel stores the visible template and quantity. Its durable housing sidecar retains the canonical source row, NULL flags, custom/ornament data, original recipient and immutable source witness. A queued crate has a distinct location3 and version3 allocation, without a fabricated inventory slot or custody root. Existing placed/carried allocation encodings remain unchanged. Source manifests and typed contents remain authoritative in server custody.

The RoF2 list displays the exact reconstructed item through the existing parcel subtype. Retrieval authenticates the actual parcel ID, cached native slot/template, recipient, inventory lease and original sealed retirement. The bridge stages the item and packet while holding its normal main-thread inventory barrier. One transaction inserts the exact saved row, moves a crate from parcel to carried identity, deletes the native parcel and records an immutable claim postimage. No generic template-only retrieval, stack merge, duplicate-item deletion or second SaveInventory is used. The claimed sidecar remains a tombstone.

A fresh connection resolves uncertain COMMIT while locking the stable sidecar row. The barrier remains held until that outcome is known and the staged item is applied to memory; a postcommit invariant failure uses the existing fail-stop path. Existing certified-process-death recovery and normal typed inventory admission load the committed state. The claim record certifies database completion, not a separately persisted memory acknowledgement. Canonical generated GUID0 is retained until normal admission's exact GUID-only handoff.

The settlement uses route4 for direct parcel returns; that route's destination field holds the durable native parcel ID. Its immutable delivery witness is unchanged by later claim. Generated-bin identity continues to use its existing crate route, with queued or carried safe-path validation. This does not certify missing legacy proofs, new historical Live rules, partial-stack lineage or destroyed typed containers.
