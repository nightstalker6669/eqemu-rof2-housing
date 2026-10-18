# Status at publication

Snapshot: October 18, 2026. This is an experimental source publication. Housing remains incomplete; representative original-client acceptance is distinguished from implementation and static protocol recovery.

| Area | Implementation / evidence | Remaining limit |
|---|---|---|
| Neighborhoods / ownership | Catalogs, regions, instancing, ownership and access services; native marker/inspection/purchase success | Complete original neighborhood content/economics and remote navigation matrix |
| Personal house | Native merchant purchase, placement/persistence/interior entry; native furnished same-plot Swap Houses with contents moved to replacement storage | All house variants/capacities are not independently client-accepted |
| Furniture | Scene/custody/geometry/storage; representative native wall cycle and closet-stored floor chair listing/placement/pickup; valid room survives optional yard-read refusal | Full floor/ceiling, curved/rotated and nonzero-centre behavior |
| Stack quantities | Native yard-vault partial store/retrieve and house-closet partial store/retrieve conserved totals | Full crate quantity acceptance still open |
| Crates / houses | Stable identities and recovery; native furnished house-in-crate plot transfer, recipient unpack and retained interior entry | Fresh catalog crate definition/capacity gap; vault packing; same-plot/new-generation relocation |
| Layouts | Native decoder and transactional mixed-source application/recovery | Complete original-client load-layout acceptance |
| Guild housing | Guild ownership/ACL/current-membership checks, permanent interiors, source-bound routes and Guild Lobby left-door contract | Full additional hall profiles/capacities and final native routing matrix |
| Trophies / anchors | Native messages, placement/source validation, benefits/billing/activation and transport machinery | Authentic benefit/cost/item classifications and full native gameplay acceptance |
| Economy / lifecycle | Purchase, escrow/upkeep selection, retirement, durable receipts and recovery | Original paid wallet ordering/cadence/refunds/eviction/inactivity/guild dissolution |
| Inventory safety | Session/writer fencing, raw-row preservation, atomic transactions, durable operation receipts, recovery/reconciliation | This is experimental software; fixture coverage is not a guarantee for arbitrary databases |

## Representative native results

1. One-copper test plot purchase succeeded through original `0x45be`, after correcting the original client's `FFFFFFFF` absent-upkeep selection. Exactly one copper was debited. This verifies the technical purchase path, not historical pricing.
2. A Cobbler Style Plaster House, Stucco interior (item 61895), was bought from a normal NPC merchant, placed on an owned plot, retained through a server restart and entered into its persistent `phinterior1a1` instance (zone 715).
3. A House of Thule painting (52295) was placed on a Standard guild-hall wall, rendered visibly and returned to inventory. This proves one representative mounted-wall cycle, not every surface.
4. Bone Chips: initial 17; store 2 -> inventory 15 / vault 2; retrieve 1 -> inventory plus cursor 16 / vault 1; remaining vault item was then returned, restoring inventory 17.
5. Bricks (62765): four GM-provided test items; store 2 to house closet -> inventory 2 / room 2; retrieve 1 -> inventory 3 / room 1. Total remained four.
6. An empty property was retired through the original Move Out confirmation. The later generated single-crate personal-yard retirement workflow is accepted below; full furnished/guild lifecycle coverage remains incomplete.

## Unfinished work excluded from the implementation export

The fresh personal crate catalog/capacity migration, vault-aware occupied packing successor, and faithful full floor-query integration remain separate unfinished work. Automatic personal-yard packing/creation is now included with the explicit supported limits below. They are documented as gaps, not published as completed implementation. No suites, fixtures, test reports, logs, accounts, private database contents, or original binary disassembly are included.


## October 18 additions

- Recipient-consented empty and reviewed furnished personal-property transfers use content-bound offers and revalidation at acceptance. Changed content or pending creator work refuses the transfer.
- Packed-house conservation includes complete retained room contents without granting packed-house entry or storage access. New-owner house movement retains creator title and the house/crate creator invariant.
- Native stored replacement-house admission, action17/21ba swap, contents migration and replacement interior entry are implemented and exercised for reviewed personal templates.
- Voting preference, item inspection, sale/transfer result messages and transfer consent are integrated. Guild trophy roster flags and stat-item admission checks are updated.
- Populated personal/guild trophy technical paths were exercised with explicitly synthetic catalogue assignments/prices and removed afterward. Original catalogue values are still incomplete; the candidate research ledger is not an activation seed.
- Current protocol reference:54 mapped values,67 documented numbers.47 complete/20 partial documented layouts;30 complete/34 partial/3 unknown meanings. These categories are independent of gameplay support.

Indoor parent-yard and populated crate lists are now separately authenticated and original-client verified. House recognition refreshes immediately after unpack. A personal loaded crate containing a furnished house completed a cross-plot, cross-neighborhood and cross-catalog round trip, retaining its permanent room and closet contents. The user completed destination placement after the stored-house restore fix. These observations do not certify every guild/delegated variant.


## Personal property lifecycle and generated-crate acceptance

The world timer now dispatches upkeep fairly across deferred/backlogged contracts. Explicit catalogue-bound policies support UTC fractional daily-copper apportionment and atomic paid-upkeep enrollment at purchase. These are labelled compatibility choices; unknown historical prices and arithmetic are not supplied.

Opt-in personal-property inactivity retirement now supports unused-escrow refunds and offline delivery of one reviewed small moving crate to saved inventory. With separate reviews it can pack supported loose yard contents and create a crate when none exists. Ownership, item return, packing, refund and immutable receipts share one transaction. Furnished personal houses retain their permanent rooms. Unsupported contents, visitors, pending operations, unsafe inventory slots and insufficient capacity defer the whole retirement. New policy tables start empty; existing properties are not enrolled automatically. Housing returns do not use the parcel service.

The native generated-crate workflow passed on a disposable property: real world-timer retirement, generated crate delivered at login, native placement onto a new ownership generation, confirmed Unpack, and chair retrieval into inventory. A17-copper escrow refund was conserved. The destination ownership was administratively provisioned; native purchase/billing was not part of this acceptance. Original furnishings were conserved.

This native check exposed a real creation-receipt foreign key mismatch. The included migration introduces typed immutable creation origins: genuine live-item receipts for online creation, historical ownership generations for offline creation. Existing genuine origins are backfilled. Offline creation never fabricates a Client receipt; origin validation retains the historical creator across relocation.

The optional trophy compatibility catalogue contains13 reconstructed benefit definitions: ten final tradeskill, two paid personal and one guild. It is not an authenticated May2013 catalogue. The SQL references stat item definitions in the operator's item database; it does not supply a portable complete item/content seed. Review those definitions and keys before use. Native final tradeskill trophy placement remains blocked by evolving-item custody support; paid personal placement metadata and guild billing acceptance also remain incomplete. These rows do not establish working trophy gameplay.

No new opcode number, wire-layout change or client-file modification was required by this batch. Multi-container capacity, guild/foreign/vault returns, overdue reconciliation, alternate-currency/partial billing and other listed housing gaps remain. No test suites, fixtures, logs, personal runtime data or game assets are included.
