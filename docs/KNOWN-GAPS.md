# Known gaps and remaining work

## Protocol completeness

The reference documents67 opcode numbers;54 have active mapping entries and13 do not. The full list and per-direction extent/meaning limits are in [recovered layouts](../protocol/STRUCTS.md). Mapped values can still lack active producers, retain opaque fields or implement a narrower emulator compatibility profile.

Unresolved operations include membership deltas0x0c13/0x76c6, access invalidation0x3968, text submission0x3c8b, custom interaction0x51e3, menu action19/0x64a3 and action5/0x7ad1. Several other recovered but inactive messages are cache/item update variants with working snapshot alternatives; an inactive opcode is not automatically an independent missing gameplay feature.

0x58c7 is now mapped as OP_RealEstateItemInspect. Project names and implemented effects are not proof that all original operation semantics or response variants are recovered. Original incoming full emitted extents remain unknown for several consumer-covered layouts, including transfer consent and some result messages. The two one-DWORD result senders are explicit emulator profiles.

Internal ServerOP_Housing messages are emulator coordination, not recovered Live client opcodes. This is an identified documented catalogue, not proof that no further message exists.

## Crates and lifecycle

- The newest experimental personal catalog lacks crate definition 174 and its allowed-child entry. Its capacity policy also conflates a house with total estate children. Merely increasing that limit could allow multiple houses and is not an acceptable fix.
- The existing occupied-property path rejects nonplaceable vault payloads rather than fully packing them. The draft successor is not in this export.
- Replacing a loaded furnished crate after retiring and rebuying the same plot hits an existing house-relocation constraint when the parent native ID is unchanged across ownership generations.
- Complete original inactive-owner eviction, guild dissolution, refunds and overflow delivery rules remain unfinished.

## Interior versus parent-yard presentation

Indoor parent-yard and populated moving-crate lists now use a separately authenticated read-only parent policy. Successful crate operations refresh estate ancestry, fixing immediate house recognition after unpack. A representative original-client personal-house session verified both. Guild/visitor variations still require distinct acceptance.

## Relocation acceptance

A representative personal loaded-crate/furnished-house round trip across plots, neighborhoods and catalogs completed through the original client. House and crate identities, closet contents and permanent interior were retained; the user placed the house at the destination. The stored-house restore rejection was fixed with retained binding, permission and child-capacity checks. Direct house-inventory relocation, interior-loaded and guild/delegated variants remain separately unverified. This does not close the same-plot/rebought ownership-generation issue above.

## Placement fidelity

- Full constrained floor/ceiling collision-query semantics are not complete.
- Original query behavior includes sphere sweep, finite faces/edges/vertices, model-centre/extents transformations and numeric quirks. A guessed global tolerance is not a recovered solution.
- Representative wall placement passed. Additional nonzero-centre, curved, large and rotated surfaces need work.
- Connected coplanar surface support is integrated and server-tested; the new beyond-pair boundary still needs a native acceptance witness.

## Content and economics

Original numeric data is not inferred from modern Live or illustrative help examples. Remaining unknowns include per-template vault/NPC capacities, some guild-hall capacities, original tariffs/upkeep/currency ordering and trophy benefits/costs. The candidate ledger contains42 source candidates and90 tiers, with35 specific later cost observations (34 undisputed) and55 unpriced tiers. None is an authenticated May2013 catalogue record. See [candidate data](../protocol/trophy-catalogue-candidates.json); it is not deployable original content. Some published manifests/policies deliberately disable unknown categories; those technical limits must not be advertised as historical rules.

The Standard guild hall uses the accepted basic guild-hall interior geometry but a separate persistent housing instance. This does not imply the ordinary Guild Lobby fallback hall supports neighborhood furnishing.

## Completion standard

Completion requires end-to-end original-client acceptance for personal and guild plots/houses, item placement/pickup and movement/storage, permissions, layouts, crates, persistence/recovery, routes and lifecycle—not merely mapped opcode numbers or successful compilation.
