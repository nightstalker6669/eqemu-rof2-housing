# Known gaps and remaining work

## Protocol completeness

The reference documents67 opcode numbers;54 have active mapping entries and13 do not. The full list and per-direction extent/meaning limits are in [recovered layouts](../protocol/STRUCTS.md). Mapped values can still lack active producers, retain opaque fields or implement a narrower emulator compatibility profile.

Unresolved operations include membership deltas0x0c13/0x76c6, access invalidation0x3968, text submission0x3c8b, custom interaction0x51e3, menu action19/0x64a3 and action5/0x7ad1. Several other recovered but inactive messages are cache/item update variants with working snapshot alternatives; an inactive opcode is not automatically an independent missing gameplay feature.

The [protocol boundary audit](../protocol/PROTOCOL-BOUNDARIES.md) separates optional deltas from missing original-server operation data. In particular, item-owned membership is a distinct cache; its mapped `0x3e00` replacement currently has no zone producer, while My Items uses direct mode5 search.

0x58c7 is now mapped as OP_RealEstateItemInspect. Project names and implemented effects are not proof that all original operation semantics or response variants are recovered. Original incoming full emitted extents remain unknown for several consumer-covered layouts, including transfer consent and some result messages. The two one-DWORD result senders are explicit emulator profiles.

Internal ServerOP_Housing messages are emulator coordination, not recovered Live client opcodes. This is an identified documented catalogue, not proof that no further message exists.

## Crates and lifecycle

- The older experimental personal catalog now includes reviewed crate174 through an explicit managed-content transition. Stored houses no longer consume the crate allocator's placed-child count; independent house/crate limits remain enforced. See [catalog repair and capacity scope](CRATE-CATALOG-CAPACITY-2026-10-19.md).
- Vault-aware mixed packing and occupied personal Move Out are now included with migration036. Native Move Out, loaded-crate placement, bulk Unpack and ordinary vault retrieval passed for a representative stack. Individual Inventory/Crate/Shed controls and partial quantities are now implemented and representatively client-accepted for owned placed personal-yard and Standard guild-hall interior crates. Native Trustee foreign-creator retrieval/storage refusal is also accepted; successful delegated use of a visitor-owned Standard guild-room crate and same-session revocation are now accepted in the [guild lifecycle update](GUILD-DELEGATED-LIFECYCLE-2026-10-20.md); other guild/interior variants and carried-crate editing remain unaccepted. See [individual vault controls](INDIVIDUAL-VAULT-CRATE-2026-10-19.md). Automatic generated-crate vault returns and the guild/foreign-owner matrix remain separately unaccepted. See [vault scope](VAULT-MOVE-OUT-2026-10-19.md).
- The same-plot ownership-generation relocation fix now has native Move Out, returned-crate placement, Unpack, house restoration and original-room entry evidence. Ownership re-acquisition was administrative; native repurchase/debit/economics remain unverified. See [same-plot recovery](SAME-PLOT-RECOVERY-2026-10-19.md).
- Supported personal/guild mixed-owner offline retirement, exact copper refunds and guarded dissolution are implemented. Existing exterior crates containing typed nonplaceables now participate in the atomic return path; see [retirement and returns](RETIREMENT-RETURNS.md). Interior crates, recognized NPCs and native parcel overflow are now implemented; complete historical rules and unsupported custody remain unresolved.

## Interior versus parent-yard presentation

Indoor parent-yard and populated moving-crate lists now use a separately authenticated read-only parent policy. Successful crate operations refresh estate ancestry, fixing immediate house recognition after unpack. A representative original-client personal-house session verified both. Guild/visitor variations still require distinct acceptance.

## Relocation acceptance

A representative personal loaded-crate/furnished-house round trip across plots, neighborhoods and catalogs completed through the original client. House and crate identities, closet contents and permanent interior were retained; the user placed the house at the destination. The stored-house restore rejection was fixed with retained binding, permission and child-capacity checks. Direct personal house-inventory relocation and interior-loaded variants remain separately unverified. The later [guild lifecycle update](GUILD-DELEGATED-LIFECYCLE-2026-10-20.md) accepts representative furnished Standard guild-hall automatic return and cross-plot inventory restoration, plus delegated visitor-owned interior crate operations. Same-plot/new-generation recovery is separately accepted with the administrative re-acquisition limit described above.

## Layout scope

A representative personal Load Yard consumed three real sources together: an inventory chair, a Shed chair and a physical-crate chair. Native Load House also placed a retained returned crate with its existing chair into a Standard guild-hall interior. These used reviewed local XML loaded through the original client; no fabricated network packet or SQL placement supplied the result. Original-client Save Yard/Save House supplied the starting records. The room rejected an invalid crate position without committing the other layout operation, then accepted a valid position. This confirms selected mixed-source and retained-token paths, not every file, rotation, source class, cross-room combination or historical matching rule.

## Placement fidelity

Original-scene Floor/Ceiling admission and centre-aware V8 Wall admission are now integrated. The [runtime scope](PLACEMENT-ADMISSION-2026-10-20.md) describes exact complete-scene witnesses, publication freshness, source coverage and bounded query scheduling. Earlier library-only notes describe prior stages.

The reviewed profile is specific to the authenticated May2013 client/assets. Unsupported actor classes, changed or absent source geometry and exhausted query budgets still refuse constrained admission. Free remains independently bounded by original room regions and current permissions. Universal historical curved-surface equivalence and untransmitted camera-ray reconstruction are not claimed.

## Content and economics

Every personal and guild plot now has a positive approximate purchase and daily upkeep tariff, including an explicit custom-address fallback. Those usable emulator defaults are distinct from authenticated original numeric data. Remaining unknowns include per-template vault/NPC capacities, some guild-hall capacities, original tariffs/upkeep/currency ordering and trophy benefits/costs. The candidate ledger contains42 source candidates and90 tiers, with35 specific later cost observations (34 undisputed) and55 unpriced tiers. None is an authenticated May2013 catalogue record. See [candidate data](../protocol/trophy-catalogue-candidates.json); it is not deployable original content. Some published manifests/policies deliberately disable unknown categories; those technical limits must not be advertised as historical rules.

The Standard guild hall uses the accepted basic guild-hall interior geometry but a separate persistent housing instance. This does not imply the ordinary Guild Lobby fallback hall supports neighborhood furnishing.

## Completion standard

Completion requires end-to-end original-client acceptance for personal and guild plots/houses, item placement/pickup and movement/storage, permissions, layouts, crates, persistence/recovery, routes and lifecycle—not merely mapped opcode numbers or successful compilation.


## Updated lifecycle boundary

Single-small-crate personal-yard automatic generation, offline return, refund and native unpack/retrieval have representative acceptance. Mixed-owner multi-bin returns and direct vault delivery have the separately documented service/native evidence in [mixed returns](MIXED-RETURNS-2026-10-18.md). The current retirement follow-up adds native parcel overflow and isolated interior-crate/NPC evidence. Complete retirement of arbitrary furnishings, missing historical billing policy, alternate currencies and partial affordability remain open. Final tradeskill trophies remain rejected as evolving items; reconstructed catalogue rows do not remedy that custody limitation.


## Guild routing and access follow-up

The [guild access and routing follow-up](GUILD-ACCESS-ROUTING-2026-10-19.md) confirms representative native left-door owned-hall admission, packed-hall basic fallback and source-bound slash-command exit. Native named Trustee editing and current server authority checks cover role/grant revocation semantics; a representative native Trustee placement/movement/pickup and same-session Add denial now passed in the later interior Access follow-up; the actual production authority matrix is now checked in the later guild lifecycle update; not every combination was clicked through the client. Direct furnished-building Shed storage now supports the creator-only, unoccupied profile described in the [Shed follow-up](DIRECT-HOUSE-SHED-2026-10-19.md); delegated storage and full historical policy equivalence remain unaccepted. Interior ACL editing and a representative delegated-character action pass are covered by the [interior Access follow-up](INTERIOR-ACCESS-2026-10-19.md). The later Standard model-door follow-up passed native owner and revoked-Enter visitor exits to the current neighborhood.

The [original scene integration libraries](ORIGINAL-SCENE-INTEGRATION-2026-10-20.md) now resolve original terrain coordinates/flags and connect query-aware actor receipts to bounded preimage search. The later placement admission update wires the complete reviewed scene into runtime admission; Standard Guild Hall includes 86 authored actors beyond terrain. Exact centre-aware arithmetic alone is not collision or placement authority.


## Guild lifecycle completion scope

The [guild/delegated lifecycle update](GUILD-DELEGATED-LIFECYCLE-2026-10-20.md) supersedes earlier unaccepted labels for its representative Standard hall, visitor-owned crate and return-history paths. Fresh unbound house minting through Load Yard remains unsupported; use native cursor placement first. The native Access edit packet has no recovered ACL revision echo, so simultaneous still-authorized editor lost updates are not certified. Neither this update nor the transaction checks establish every original hall model, capacity, historical retirement rule or UI concurrency behavior.


## Trophy catalogue and custody, October20

Only three researched reconstructed definitions are enabled: Giant, Landing and
guild Hardened Steel. They do not authenticate a complete May2013 catalogue.
Unknown or conflicting ranks, minimum levels, costs and stat associations remain
excluded; the earlier ten tradeskill minimum-level defaults are withdrawn through
a guarded correction. Evolving physical trophies still lack durable progression
custody and remain rejected.

The representative native guild workflow now includes paid activation, bonus
removal, cold-restart persistence and source-pickup invalidation. Personal
placement metadata, shared original IT11223 geometry and yard per-item limits
are implemented. Native Giant placement and selection survived relog, and
activation charged75 favor with +400HP/endurance. Disable restored baseline
stats, paused paid time survived logout/relog, resume added no duplicate debit,
and native pickup returned the item and cleared selection/activation/paid time.
The zero-favor Activate button was disabled in the client UI; a server
insufficient-favor packet check remains outside this evidence. Landing shares
reviewed model/class coverage without an individual native cycle. Wider recipient/roster
combinations and historical pause/offline billing semantics remain outside this
evidence. See [current trophy scope](TROPHIES-2026-10-20.md).

Item cache deployment requires a cold restart and regeneration of the actual
active shared-item prefix from the correct database. The upstream hotfix/content
database path can leave normal zone startup using stale item metadata; SQL
readback or a successful hotfix message alone does not establish runtime use.
