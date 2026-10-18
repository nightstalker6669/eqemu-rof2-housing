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
6. An empty property was retired through the original Move Out confirmation. Native furnished expired-property retirement remains unaccepted despite targeted server tests passing.

## Unfinished work excluded from the implementation export

The fresh personal crate catalog/capacity migration, vault-aware occupied packing successor, and faithful full floor-query integration remain separate unfinished work. They are documented as gaps, not published as completed implementation. No suites, fixtures, test reports, logs, accounts, private database contents, or original binary disassembly are included.


## October 18 additions

- Recipient-consented empty and reviewed furnished personal-property transfers use content-bound offers and revalidation at acceptance. Changed content or pending creator work refuses the transfer.
- Packed-house conservation includes complete retained room contents without granting packed-house entry or storage access. New-owner house movement retains creator title and the house/crate creator invariant.
- Native stored replacement-house admission, action17/21ba swap, contents migration and replacement interior entry are implemented and exercised for reviewed personal templates.
- Voting preference, item inspection, sale/transfer result messages and transfer consent are integrated. Guild trophy roster flags and stat-item admission checks are updated.
- Populated personal/guild trophy technical paths were exercised with explicitly synthetic catalogue assignments/prices and removed afterward. Original catalogue values are still incomplete; the candidate research ledger is not an activation seed.
- Current protocol reference:54 mapped values,67 documented numbers.47 complete/20 partial documented layouts;30 complete/34 partial/3 unknown meanings. These categories are independent of gameplay support.

Indoor parent-yard and populated crate lists are now separately authenticated and original-client verified. House recognition refreshes immediately after unpack. A personal loaded crate containing a furnished house completed a cross-plot, cross-neighborhood and cross-catalog round trip, retaining its permanent room and closet contents. The user completed destination placement after the stored-house restore fix. These observations do not certify every guild/delegated variant.
