# EQEmu RoF2 Housing

An experimental implementation of EverQuest personal and guild housing for EQEmu, targeting the **May 10, 2013 Rain of Fear 2 client**.

This repository publishes the housing implementation, recovered client opcode/packet documentation, database schemas, integration patch, and a candid account of working behavior and remaining work. It is a housing-only source overlay for EQEmu, not a complete server distribution or a finished recreation of Live.

## What works

The original client has completed the following representative workflows against the development server:

- Open Plot Search by clicking an actual neighborhood plot marker, inspect a plot, and purchase it.
- Buy a small house from an NPC merchant, place it on an actual plot, restart the server, and enter the same persistent interior through the house's original context menu.
- Place a representative painting on a guild-hall wall, see it rendered, and pick it back up.
- Store part of an item stack in a yard vault or house closet and retrieve part again without losing or duplicating the stack.
- Display My Plots and My Items and invoke a local Find path.
- Retire an empty owned plot through the native Move Out UI.

The server implementation also includes instanced neighborhoods/interiors; estate ownership and ACLs; furniture scenes and geometry checks; inventory custody and recovery; house and crate identity; packing/unpacking; layout transactions; escrow/upkeep machinery; guild hall routing; and personal/guild trophy and anchor machinery. **Implemented code and original-client acceptance are distinct evidence levels.** See [status](docs/STATUS.md).

## Protocol work

The published runtime mapping inventory contains **41 real-estate opcodes and 6 additional guild-trophy opcodes**. This is not a claim that every housing message, layout, or behavior is known.

- [Opcode table](protocol/OPCODES.md)
- [Machine-readable mapping inventory](protocol/opcodes.json)
- [Packet layouts and shared schemas](protocol/housing-wire-reference.json)
- Authoritative codecs: [`overlay/common/housing`](overlay/common/housing)
- [Unresolved messages and fields](docs/KNOWN-GAPS.md)

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

The largest remaining gaps are fresh personal-catalog crate support, packing nonplaceable vault belongings during move-out, same-plot/rebought-generation furnished-house relocation, complete floor/ceiling placement fidelity, additional guild-hall variants/capacities, original paid economics/lifecycle behavior, authentic missing content values, and final protocol/native acceptance coverage.

The local test purchase policy used one copper and no upkeep. **Those are explicitly experimental settings, not recovered Live prices or rules.** No personal database, local credentials, or unfinished draft patches are published here.

## Publication scope

This repository contains production housing code and protocol documentation. Test suites, test fixtures, build logs, test reports, private runtime data and unfinished external drafts are excluded. The snapshot is experimental; the known gaps above remain open.

## License and attribution

EQEmu upstream's GPLv3 license text is retained in [LICENSE](LICENSE). Existing notices are preserved. This repository is an experimental community project, not affiliated with or endorsed by Daybreak Games. EverQuest and related names belong to their respective owners. No game executables, DLLs, EQG archives, maps, textures, database dumps, or packet captures are included. See [NOTICE](NOTICE).
