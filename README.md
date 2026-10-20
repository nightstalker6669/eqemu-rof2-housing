# EQEmu RoF2 Housing

An experimental implementation of EverQuest personal and guild housing for EQEmu, targeting the **May 10, 2013 Rain of Fear 2 client**.

This repository publishes the housing implementation, recovered client opcode/packet documentation, database schemas, integration patch, and a candid account of working behavior and remaining work. It is a housing-only source overlay for EQEmu, not a complete server distribution or a finished recreation of Live.

Snapshot updated **October 20, 2026**. The [oriented placement update](docs/PLACEMENT-FIDELITY-2026-10-20.md) fixes rotated-body/footprint false refusals in the reviewed mounted-region extension, preserves saved policy versions and removes the eight-map mounting-cache ceiling. The [Standard model-door exit follow-up](docs/GUILD-ACCESS-ROUTING-2026-10-19.md#standard-model-door-follow-up-october20) adds optional guarded migration038 and owner/revoked-visitor native click acceptance. See the [crate preview handoff update](docs/CRATE-PREVIEW-2026-10-19.md) for the preview fix, and [stored-house layout restoration](docs/STORED-HOUSE-LAYOUT-2026-10-19.md) for the completed furnished Standard Guild Hall recovery check. See [interior permission editing and unguilded visitors](docs/INTERIOR-ACCESS-2026-10-19.md) for its required quest override. See [direct furnished-building Shed storage](docs/DIRECT-HOUSE-SHED-2026-10-19.md) for native storage/layout restoration scope. The [same-plot ownership recovery fix](docs/SAME-PLOT-RECOVERY-2026-10-19.md) adds migration034 and replay safeguards; native recovery and permanent-room entry passed after administrative ownership re-acquisition. See [guild permissions and Lobby routing](docs/GUILD-ACCESS-ROUTING-2026-10-19.md) for the own-hall/basic fallback and native access-role behavior. See [personal catalog crate support and capacity](docs/CRATE-CATALOG-CAPACITY-2026-10-19.md) for the managed-content repair. See [vault-aware Move Out and packing](docs/VAULT-MOVE-OUT-2026-10-19.md) for migration036 and mixed furniture/vault recovery. See [individual vault-item crate controls](docs/INDIVIDUAL-VAULT-CRATE-2026-10-19.md) for partial-stack retrieval and storage. See the [change summary](docs/UPDATES-2026-10-18.md) for the current implementation and protocol additions.

## What works

The original client has completed the following representative workflows against the development server:

- Open Plot Search by clicking an actual neighborhood plot marker, inspect a plot, and purchase it.
- Buy a small house from an NPC merchant, place it on an actual plot, restart the server, and enter the same persistent interior through the house's original context menu.
- Place a representative painting on a guild-hall wall, see it rendered, and pick it back up.
- Store part of an item stack in a yard vault or house closet and retrieve part again without losing or duplicating the stack.
- Display My Plots and My Items and invoke a local Find path.
- Retire an empty owned plot through the native Move Out UI.
- Receive an automatically generated moving crate after opt-in personal-property inactivity retirement, place it on another ownership generation, unpack it and retrieve its returned furniture.
- Swap a reviewed furnished personal house through the native Swap Houses menu and confirmation; retain the old house token/interior and move contents into replacement storage.
- Transfer a reviewed furnished personal plot with recipient consent, preserving content and creator identities; refuse acceptance after contents change.
- Pack a furnished house into a moving crate, transfer its plot, then unpack and enter its retained interior as the recipient.
- Show a closet-stored chair inside its house, place it and return it to inventory. An optional yard-view failure no longer erases the independently authorized room list.
- Restore a furnished Standard Guild Hall from yard storage through native Load Yard on a guild plot, enter its original interior normally, and retain the hall and furniture after relogging.
- Bulk Pack and Unpack a furnished Standard Guild Hall, immediately display it in yard storage, restore its layout and enter its retained interior. See the [bulk Unpack update](docs/BULK-UNPACK-2026-10-19.md) for behavior and scope.

The server implementation also includes instanced neighborhoods/interiors; estate ownership and ACLs; furniture scenes and geometry checks; inventory custody and recovery; house and crate identity; packing/unpacking; layout transactions; escrow/upkeep machinery; guild hall routing; and personal/guild trophy and anchor machinery. **Implemented code and original-client acceptance are distinct evidence levels.** See [status](docs/STATUS.md).

## Protocol work

The published runtime mapping inventory contains **54 mappings: 48 real-estate opcodes and 6 additional guild-trophy opcodes**, with **67 opcode numbers documented**. This is not a claim that every housing message, layout, or behavior is known.

- [Opcode table](protocol/OPCODES.md)
- [Machine-readable mapping inventory](protocol/opcodes.json)
- [All 67 recovered layouts and structs](protocol/STRUCTS.md)
- [Machine-readable recovered layouts](protocol/recovered-layouts.json)
- [Packet layouts and shared schemas](protocol/housing-wire-reference.json)
- Authoritative codecs: [`overlay/common/housing`](overlay/common/housing)
- [Unresolved messages and fields](docs/KNOWN-GAPS.md)
- [Remaining protocol boundaries](protocol/PROTOCOL-BOUNDARIES.md)

Notable recovered messages include plot inspection (`0x796c`), purchase (`0x45be`), Move Out/Sale (`0x792c`), transfer (`0x507f`), placement begin/commit (`0x626e` / `0x711a`), pickup (`0x2036`), storage operations (`0x3424`), pack/unpack (`0x613d`), layout load (`0x5b41`), and the inspect-mode Plot Search opener (`0x2b35`).

## Repository layout

| Path | Contents |
|---|---|
| `overlay/` | New housing source files, required registries, and SQL; copied into a matching EQEmu checkout |
| `patches/eqemu-housing.patch` | Housing integration changes to existing EQEmu files |
| `protocol/` | Current opcode inventory and recovered packet layouts |
| `docs/` | Integration notes, architecture, validation, and known gaps |
| `tools/apply_overlay.py` | Checks the exact upstream base and applies the patch plus overlay |

## Integrating the source

The integration base is upstream [EQEmu/EQEmu](https://github.com/EQEmu/EQEmu), commit **`8175ae61874cbf20d7b05102821f6f8a4f4d1c39`**. Start from a clean checkout of that commit:

```sh
python3 tools/apply_overlay.py /path/to/clean/EQEmu --check
python3 tools/apply_overlay.py /path/to/clean/EQEmu --apply
```

Read [INSTALL.md](docs/INSTALL.md) before building or applying schemas. This snapshot retains development-database guards and requires reviewed catalog/content setup; applying the source alone does not activate a ready-to-play neighborhood. Game assets must be supplied separately by the operator.

## What is not finished

Remaining work includes unsupported custody classes, complete floor/ceiling placement fidelity, additional guild-hall variants/capacities, the wider delegated/interior acceptance matrix, authentic trophy catalogue values and further original-client acceptance. Personal vault-aware Move Out, reviewed multi-owner offline returns and furnished same-plot recovery have the distinct evidence described below; same-plot ownership re-acquisition was administrative. All plots now have approximate prices, while original currency ordering and full Live economics remain unresolved.

The local test purchase policy used one copper and no upkeep. **Those are explicitly experimental settings, not recovered Live prices or rules.** No personal database, local credentials, or unfinished draft patches are published here.

## Publication scope

This repository contains production housing code and protocol documentation. Test suites, test fixtures, build logs, test reports, private runtime data and unfinished external drafts are excluded. The snapshot is experimental; the known gaps above remain open.

## License and attribution

EQEmu upstream's GPLv3 license text is retained in [LICENSE](LICENSE). Existing notices are preserved. This repository is an experimental community project, not affiliated with or endorsed by Daybreak Games. EverQuest and related names belong to their respective owners. No game executables, DLLs, EQG archives, maps, textures, database dumps, or packet captures are included. See [NOTICE](NOTICE).


## Billing and Sunrise follow-up

The latest production source and optional finance-window patch are included. Representative personal bulk controls, paid guild purchase/enrollment/upkeep, and all 75 Plot Search rows have native acceptance. The guild-frame click and automatic refresh of an already-open finance window also passed. See [billing and Sunrise update](docs/BILLING-SUNRISE-2026-10-18.md) for scope and compatibility limits.


## Overdue upkeep and guild inactivity follow-up

Default-off witnessed catch-up, reviewed guild inactivity returns and checked guild dissolution are included. The original server world-timer empty-guild release/refund passed, and client inventory displayed the refunded wallet at login. Furnished guild-crate reload was verified separately through private SQL; native crate placement remains outside that evidence. Cleanup completed with original state/currency restored, activation reviews cleared and immutable history retained. See [scope and evidence](docs/OVERDUE-INACTIVITY-2026-10-18.md).


## Mixed-owner return follow-up

Optional all-recipient atomic returns, direct rightful-owner delivery and sealed settlement are included. Combined SQL/reload and private guarded guild-deletion verification passed. Representative native recipient login, rightful direct returns, loose-crate placement/unpack and chair pickup also passed; the carried furnished-hall crate was not unpacked. Actual native guild deletion also passed after the settlement follow-up fix, preserving returned items and the separate protected guild. Final deployment and original-state conservation passed; new activation reviews remain empty. See [scope and limits](docs/MIXED-RETURNS-2026-10-18.md).

Plot pricing: all personal and guild plots receive positive purchase and daily upkeep defaults, including custom addresses. Known Sunrise street tiers are preserved; see [plot pricing configuration](overlay/common/housing/plot_pricing_defaults.md) for rates, fallbacks and safe economic-only catalogue upgrades. These are approximate emulator defaults.

Personal/guild retirement now supports typed existing crates, reviewed NPC furniture, interior crate extraction and optional native parcel overflow. Exact item metadata and crate identity survive authenticated parcel claim. See [property retirement and returns](docs/RETIREMENT-RETURNS.md) for ownership, refund, recovery, native evidence and remaining historical limits.

The retirement parcel workflow has fresh native acceptance: world-timer retirement, exact escrow refund, parcel display/retrieval of a vault stack and generated nonempty crate, followed by normal typed crate reload. Current scope and remaining limits are recorded in [retirement and returns](docs/RETIREMENT-RETURNS.md).
