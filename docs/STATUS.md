# Status at publication

Snapshot: October 20, 2026. This is an experimental source publication. Housing remains incomplete; representative original-client acceptance is distinguished from implementation and static protocol recovery.

| Area | Implementation / evidence | Remaining limit |
|---|---|---|
| Neighborhoods / ownership | Catalogs, regions, instancing, ownership and access services; native marker/inspection/purchase success | Complete original neighborhood content/economics and remote navigation matrix |
| Personal house | Native merchant purchase, placement/persistence/interior entry; native furnished same-plot Swap Houses with contents moved to replacement storage | All house variants/capacities are not independently client-accepted |
| Furniture | Scene/custody/storage plus complete original-source Floor/Ceiling admission, trusted per-recipient source census, V8 centre-aware native-contact Wall admission and bounded Free placement; representative native placement/movement/pickup and restart persistence verified; saved V6/V7 predecessors remain readable | Reviewed matching-client/source coverage; unsupported transformed actors, exhausted bounded queries and untransmitted camera-ray reconstruction remain outside the compatibility claim. See [current admission scope](PLACEMENT-ADMISSION-2026-10-20.md) |
| Stack quantities | Native yard-vault partial store/retrieve and house-closet partial store/retrieve conserved totals | Native whole/partial Inventory and Shed/Crate quantity transfers passed for an owned placed personal-yard crate; Standard guild-hall interior whole/partial cycle, Trustee creator-restriction refusals and a delegated visitor-owned crate/revocation cycle also passed; other variants remain open |
| Crates / houses | Stable identities and recovery; native furnished house-in-crate plot transfer, recipient unpack and retained interior entry; Standard guild automatic return, cross-plot restoration and delegated visitor-owned interior crate cycle | Other hall profiles and automatic-vault variants; native repurchase/economics for same-plot recovery |
| Layouts | Native decoder and transactional mixed-source application/recovery; native personal Inventory/Shed/Crate load and furnished retained-crate guild-room load | Other source classes, rotations and cross-room combinations remain unaccepted |
| Guild housing | Guild ownership/ACL/current-membership checks, permanent interiors, source-bound routes and Guild Lobby left-door contract | Additional hall profiles/capacities and simultaneous native editor interactions; current-authority matrix and representative delegated-crate lifecycle are checked |
| Trophies / anchors | Native messages, placement/source validation, benefits/billing/activation and transport machinery | Authentic benefit/cost/item classifications and full native gameplay acceptance |
| Economy / lifecycle | Purchase, escrow/upkeep selection, retirement, durable receipts and recovery | Original paid wallet ordering/cadence/refunds/eviction/inactivity/guild dissolution |
| Inventory safety | Session/writer fencing, raw-row preservation, atomic transactions, durable operation receipts, recovery/reconciliation | This is experimental software; fixture coverage is not a guarantee for arbitrary databases |

The October19 layout/crate follow-up also fixes exterior scene reads after a furnished interior crate is picked up. Carried room members retain room provenance and are hidden only after full carried-container validation.

## Representative native results

1. One-copper test plot purchase succeeded through original `0x45be`, after correcting the original client's `FFFFFFFF` absent-upkeep selection. Exactly one copper was debited. This verifies the technical purchase path, not historical pricing.
2. A Cobbler Style Plaster House, Stucco interior (item 61895), was bought from a normal NPC merchant, placed on an owned plot, retained through a server restart and entered into its persistent `phinterior1a1` instance (zone 715).
3. A House of Thule painting (52295) was placed on a Standard guild-hall wall, rendered visibly and returned to inventory. This proves one representative mounted-wall cycle, not every surface.
4. Bone Chips: initial 17; store 2 -> inventory 15 / vault 2; retrieve 1 -> inventory plus cursor 16 / vault 1; remaining vault item was then returned, restoring inventory 17.
5. Bricks (62765): four GM-provided test items; store 2 to house closet -> inventory 2 / room 2; retrieve 1 -> inventory 3 / room 1. Total remained four.
6. An empty property was retired through the original Move Out confirmation. The later generated single-crate personal-yard retirement workflow is accepted below; full furnished/guild lifecycle coverage remains incomplete.

## Unfinished work excluded from the implementation export

The personal crate catalog/capacity transition and vault-aware occupied packing successor are now included in the October19 follow-ups below. Original floor/ceiling scene queries are integrated in the later [placement admission update](PLACEMENT-ADMISSION-2026-10-20.md). Automatic personal-yard packing/creation is now included with the explicit supported limits below. They are documented as gaps, not published as completed implementation. No suites, fixtures, test reports, logs, accounts, private database contents, or original binary disassembly are included.


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

Opt-in personal-property inactivity retirement now supports unused-escrow refunds and offline delivery of one reviewed small moving crate to saved inventory. With separate reviews it can pack supported loose yard contents and create a crate when none exists. Ownership, item return, packing, refund and immutable receipts share one transaction. Furnished personal houses retain their permanent rooms. Unsupported contents, visitors, pending operations, unsafe inventory slots and insufficient capacity defer the whole retirement. New policy tables start empty; existing properties are not enrolled automatically. The later exact batch extension can use native parcels with optional migration037 and an explicit overflow review; see [current retirement and returns](RETIREMENT-RETURNS.md).

The native generated-crate workflow passed on a disposable property: real world-timer retirement, generated crate delivered at login, native placement onto a new ownership generation, confirmed Unpack, and chair retrieval into inventory. A17-copper escrow refund was conserved. The destination ownership was administratively provisioned; native purchase/billing was not part of this acceptance. Original furnishings were conserved.

This native check exposed a real creation-receipt foreign key mismatch. The included migration introduces typed immutable creation origins: genuine live-item receipts for online creation, historical ownership generations for offline creation. Existing genuine origins are backfilled. Offline creation never fabricates a Client receipt; origin validation retains the historical creator across relocation.

The optional trophy compatibility catalogue contains13 reconstructed benefit definitions: ten final tradeskill, two paid personal and one guild. It is not an authenticated May2013 catalogue. The SQL references stat item definitions in the operator's item database; it does not supply a portable complete item/content seed. Review those definitions and keys before use. Native final tradeskill trophy placement remains blocked by evolving-item custody support; paid personal placement metadata and guild billing acceptance also remain incomplete. These rows do not establish working trophy gameplay.

No new opcode number, wire-layout change or client-file modification was required by this batch. Multi-container capacity, guild/foreign/vault returns, overdue reconciliation, alternate-currency/partial billing and other listed housing gaps remain. No test suites, fixtures, logs, personal runtime data or game assets are included.


## Billing and Sunrise follow-up

The latest production source and optional finance-window patch are included. Representative personal bulk controls, paid guild purchase/enrollment/upkeep, and all 75 Plot Search rows have native acceptance. The guild-frame click and automatic refresh of an already-open finance window also passed. See [billing and Sunrise update](BILLING-SUNRISE-2026-10-18.md) for scope and compatibility limits.


## Overdue upkeep and guild inactivity follow-up

Default-off witnessed catch-up, reviewed guild inactivity returns and checked guild dissolution are included. The original server world-timer empty-guild release/refund passed, and client inventory displayed the refunded wallet at login. Furnished guild-crate reload was verified separately through private SQL; native crate placement remains outside that evidence. Cleanup completed with original state/currency restored, activation reviews cleared and immutable history retained. See [scope and evidence](OVERDUE-INACTIVITY-2026-10-18.md).


## Mixed-owner return follow-up

Optional all-recipient atomic returns, direct rightful-owner delivery and sealed settlement are included. Combined SQL/reload and private guarded guild-deletion verification passed. Representative native recipient login, rightful direct returns, loose-crate placement/unpack and chair pickup also passed; the carried furnished-hall crate was not unpacked. Actual native guild deletion also passed after the settlement follow-up fix, preserving returned items and the separate protected guild. Final deployment and original-state conservation passed; new activation reviews remain empty. See [scope and limits](MIXED-RETURNS-2026-10-18.md).

October 19 follow-up: [crate preview handoff](CRATE-PREVIEW-2026-10-19.md) fixes canceled-preview state after a successful crate/yard transfer. The [stored-house layout restoration follow-up](STORED-HOUSE-LAYOUT-2026-10-19.md) completed the representative furnished Standard Guild Hall recovery workflow: native Load Yard placement, normal entry and retained furniture after relog. Bulk Unpack and manual dragging are not newly accepted by that result. No new opcode or schema was added.

Bulk Unpack follow-up: the [storage handoff](BULK-UNPACK-2026-10-19.md) now unpacks eligible items into Shed/Closet without requiring a historical placement pose to fit. A native furnished Standard Guild Hall Pack → Unpack → Load Yard → Enter cycle passed with immediate lists and original furniture preserved. This establishes the implemented compatibility profile; exact historical server policy and manual dragging remain separate.


## Guild access and Lobby routing follow-up

The original left-door click entered the guild-owned permanent furnished hall; with that hall packed, it entered the separate basic fallback. Native `/leaverealestate` returned to the current parent plot. Current-authority role/grant/revocation checks passed and the original management window saved a Trustee grant that survived camp/login. See [guild access and routing](GUILD-ACCESS-ROUTING-2026-10-19.md) for capability masks, scope and the historical direct-Shed/interior-door/delegated-session limits; the direct-Shed profile is extended below. No new opcode, struct, schema or production code change was required.


## Interior Access and unguilded visitors

Interior Access edits now target the current parent plot under a separate verified interior source policy. A production guildhall quest override admits authenticated neighborhood visitors while retaining the basic hall guild gate. Representative native second-character Trustee placement/movement/pickup and same-session Add denial after administrative revocation passed. No new opcode, wire struct or migration was required. See [implementation, quest deployment and acceptance scope](INTERIOR-ACCESS-2026-10-19.md).

## Direct furnished-building Shed storage

Unoccupied creator-owned reviewed houses/halls can now move directly to Shed while retaining their permanent room and complete furnishing graph. Native inside refusal, exterior Shed, immediate storage listing and one-building Load Yard restoration passed on a furnished Standard Guild Hall. Exact persisted house/room/item/pose conservation passed. The client closed before a fresh re-entry observation; it is not newly claimed. Occupancy and pending-work safeguards passed private transaction checks. No opcode, struct or migration was added. See [implementation and acceptance limits](DIRECT-HOUSE-SHED-2026-10-19.md).

## Same-plot ownership-generation recovery

The furnished-crate relocation writer/reader and migration034 now distinguish a new ownership estate from an unchanged physical plot. World/zone build and eight private production replay/SQL constraint checks passed; matching binaries and migration are deployed. Native Move Out, loaded-crate placement under a new ownership generation on the same plot, Unpack, Load Yard house restoration and original-room entry passed. Re-acquisition used the production administrative estate service because this old experimental catalog has no purchase scheme; native repurchase/economics remain unverified. A layout refresh fix also passed a focused original-client check: a retained loaded crate placed from inventory immediately exposes its packed house and enables Unpack without zoning. See [implementation and limits](SAME-PLOT-RECOVERY-2026-10-19.md).

## Personal catalog crate capacity

The older personal house-choice catalog received an explicit reviewed crate174/child-budget transition while preserving both permanent interiors and all current item bindings. The allocator excludes retained stored-house rooms from placed-child capacity while keeping the independent one-house and property-wide one-crate limits. Native inventory Place, immediate crate listing/capacity, pickup back to inventory and normal entry to the unchanged existing interior passed on a plot with one house placed and another stored. See [administrative transition, evidence and limits](CRATE-CATALOG-CAPACITY-2026-10-19.md).


## Vault-aware occupied packing

Migration036 and the typed list-only vault-member path now support mixed physical/vault packing and occupied personal Move Out. Native Move Out, retained loaded-crate placement, immediate member listing, bulk Unpack and ordinary retrieval of the complete17-item stack passed. Exact content custody bytes and the original furnished-house room were conserved; native Load Yard and normal Enter reached that original room. Ownership re-acquisition was administrative; individual packed-member controls are extended below; automatic/guild/foreign variants remain separate limits. See [implementation, deployment and evidence scope](VAULT-MOVE-OUT-2026-10-19.md).


## Individual vault-item crate controls

Creator-owned placed moving crates now support individual Inventory retrieval and Crate/Shed/Closet transfers through the existing native messages, with partial-stack custody and durable recovery witnesses. The original client completed a personal-yard stack cycle using whole storage, partial direct retrieval, reverse partial inventory storage and a partial Shed transfer followed by Crate return. All17 items were conserved. The build,23 focused private assertions and34 mixed-house/vault regression assertions passed. No new opcode, native packet struct or migration was introduced. Interior/guild/delegated variants and carried-crate editing remain separately unaccepted. See [implementation and scope](INDIVIDUAL-VAULT-CRATE-2026-10-19.md).

Current retirement update: native parcel overflow, reviewed NPC returns and interior crate extraction are implemented. The fresh original-client timer/parcel/claim/reload workflow passed; see [retirement and returns](RETIREMENT-RETURNS.md) for the exact evidence and limits.


## Standard model-door exit and protocol boundaries, October20

The existing Standard hall model door is now visible to RoF2 through optional guarded content migration038. Its normal door click uses the authenticated current-parent housing exit route. Original-client owner and revoked-Enter unguilded visitor clicks both reached the hall's actual neighborhood. The temporary visitor grant/public-access change was restored. Basic version1 hall handling and the Lobby left-door own-hall/fallback contract are unchanged; no repeat of the previous Pack/fallback workflow is claimed. See [routing implementation](GUILD-ACCESS-ROUTING-2026-10-19.md).

The retained matching-build protocol evidence was reviewed without adding guessed mappings, custom actions or wire structs. Optional membership/cache variants and messages missing original server operation data remain explicit in [protocol boundaries](../protocol/PROTOCOL-BOUNDARIES.md). This follow-up does not close every housing opcode or claim full Live equivalence.


## Shared oriented placement follow-up, October20

New mounted Add operations use a distinct V4 content policy with exact convex footprint support, oriented body collision and full inward-sweep collision. Existing V3/V2/V1/base fingerprints retain their prior semantics. The arbitrary eight-map cache refusal is replaced with bounded registered-family ownership and safe revision replacement. Matching world/zone/recovery binaries are deployed. Focused actual production-object geometry checks passed for personal, Standard and large-hall sources; a native Standard-hall painting placement/rendering/Inventory-pickup cycle passed. No native session in the other two families, additional pitch/roll change or full original Floor/Ceiling parity is claimed. See [implementation and scope](PLACEMENT-FIDELITY-2026-10-20.md).


## Guild and delegated lifecycle completion

The [guild/delegated update](GUILD-DELEGATED-LIFECYCLE-2026-10-20.md) adds complete structural room preflight to furnished house return/relocation and preserves valid completed-return history after later pickup and destination retirement. The actual authority matrix passed, including rank/membership changes, management hierarchy, creator restrictions, revocation and rollback. The original client completed a visitor-owned interior crate/partial-stack cycle with same-session revocation denial, and a world-timer-generated furnished guild-hall return followed by cross-plot restoration, permanent-room entry, Shed restoration and re-entry. These later results extend the narrower historical observations above; they do not certify every hall profile or universal Live policy. No opcode, packet layout or migration changed.
