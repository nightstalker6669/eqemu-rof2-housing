# Known gaps and remaining work

## Protocol completeness

At least these related messages remain insufficiently understood and are not mapped by this snapshot:

| Value | Direction | Recovered evidence / unresolved part |
|---|---|---|
| `0x76c6` | Server -> client | Estate ID at 0, add flag at 104, minimum 105 bytes; intervening/full payload remains unknown |
| `0x7ad1` | Client -> server | 148-byte action-5 sender, estate/item prefix; complete operation meaning unresolved |
| `0x58c7` | Client -> server | Eight-byte estate/item fallback-use request; successful behavior unresolved |
| `0x51e3` | Client -> server | Twelve-byte estate/item/menu parameter; semantic action unresolved |
| `0x64a3` | Client -> server | Twelve-byte estate/item/menu parameter; semantic action unresolved |

This is an identified open list, not proof that no additional unknown branch exists. Some mapped messages retain opaque padding/fields or unsupported variants. Internal `ServerOP_Housing*` messages are project-defined server coordination, not recovered Live client opcodes.

## Crates and lifecycle

- The newest experimental personal catalog lacks crate definition 174 and its allowed-child entry. Its capacity policy also conflates a house with total estate children. Merely increasing that limit could allow multiple houses and is not an acceptable fix.
- The existing occupied-property path rejects nonplaceable vault payloads rather than fully packing them. The draft successor is not in this export.
- Replacing a loaded furnished crate after retiring and rebuying the same plot hits an existing house-relocation constraint when the parent native ID is unchanged across ownership generations.
- Complete original inactive-owner eviction, guild dissolution, refunds and overflow delivery rules remain unfinished.

## Placement fidelity

- Full constrained floor/ceiling collision-query semantics are not complete.
- Original query behavior includes sphere sweep, finite faces/edges/vertices, model-centre/extents transformations and numeric quirks. A guessed global tolerance is not a recovered solution.
- Representative wall placement passed. Additional nonzero-centre, curved, large and rotated surfaces need work.
- Connected coplanar surface support is integrated and server-tested; the new beyond-pair boundary still needs a native acceptance witness.

## Content and economics

Original numeric data is not inferred from modern Live or illustrative help examples. Remaining unknowns include per-template vault/NPC capacities, some guild-hall capacities, original tariffs/upkeep/currency ordering and trophy benefits/costs. Some published manifests/policies deliberately disable unknown categories; those technical limits must not be advertised as historical rules.

The Standard guild hall uses the accepted basic guild-hall interior geometry but a separate persistent housing instance. This does not imply the ordinary Guild Lobby fallback hall supports neighborhood furnishing.

## Completion standard

Completion requires end-to-end original-client acceptance for personal and guild plots/houses, item placement/pickup and movement/storage, permissions, layouts, crates, persistence/recovery, routes and lifecycle—not merely mapped opcode numbers or successful compilation.
