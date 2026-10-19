# All-recipient offline property returns

This opt-in extension returns a supported property as one transaction across all original item owners. Install the guild inactivity/origin migration first, then `2026_10_18_housing_offline_direct_returns.sql`, `2026_10_18_housing_return_settlement.sql`, and `2026_10_18_housing_mixed_return_batches.sql`. New policy tables start empty. Older personal/single-recipient guild reviews retain their prior behavior when no batch review is present.

The new batch review pins the current ownership generation, native plot, exact catalogue revision and definitions/regions hashes, the reviewed small-crate content manifest, and bounded recipient/crate/capacity limits. Existing90-day personal or30-day guild inactivity authority is independently required. Guild activity remains the documented current-roster policy; its reviewed leader receives only the unused property escrow, not other players' belongings. No purchase-price refund is introduced.

Before mutation the service captures the complete source graph: current yard custody, exterior crate members, retained house interiors and vault scopes, including foreign/original ownership generations. It locks the real recipients/accounts and requires every recipient to be offline with no unresolved item, house or coin application. Interior visitors or previews, incomplete graphs, unsupported engines and unknown identities defer the entire property.

Furniture and hall bodies are partitioned by original custody owner into generated moving crates. Bin capacity cannot exceed the actual catalogue definition, a bin holds at most one house/hall, and multiple bins are allocated as needed within reviewed bounds. Same-owner room objects remain attached to their permanent house/room. Foreign-owned room objects and all supported non-placeable vault objects are returned directly to their original owners' saved inventory. Whole stack counts, NULL/binary/custom metadata, ornaments and canonical item identity are retained. No partial-stack splitting or transfer to the guild leader is used. All saved-inventory destinations are checked ordinary slots23–30, including orphan bag-child and typed-container conflicts; lack of space rolls back everything. Parcel overflow and bag-content reconstruction are not inferred.

Supported existing exterior crates are structurally validated before their members are regrouped by owner; their emptied typed roots return to their own original creators without replacing container identity. Interior crate roots, evolving or augmented objects and unsupported NPC/model/quantity shapes defer safely. Ordinary whole returned item and empty crate adapters retain immutable per-item source/postimage receipts. Generated origin4 keys bind source generation, recipient and bin ordinal, so several crates can come from one property without changing their creator after guild deletion.

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
