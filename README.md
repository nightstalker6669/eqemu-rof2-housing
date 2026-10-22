# EQEmu RoF2 Housing

Personal and guild housing for the **May 10, 2013 Rain of Fear 2 client**, supplied as a production source overlay for EQEmu commit `8175ae61874cbf20d7b05102821f6f8a4f4d1c39`.

The implementation includes neighborhood plots, paid purchases and upkeep, persistent houses and guild halls, furniture placement, permissions, vaults, moving crates, layouts, retirement returns and parcels. Reviewed service contracts provide banking, supplies, tribute and guild-hall teleportation. The repository also contains recovered housing opcodes and packet structures.

## Install

Use the [complete installer](docs/INSTALL.md) on an existing Linux/MariaDB EQEmu server. It takes private backups, installs the ordered schemas and reviewed content, builds the server, imports geometry from your own matching client, preserves existing housing identities, and provisions a fresh neighborhood with **71 personal plots and four guild plots**. Every plot has a purchase price and rent policy.

```sh
python3 tools/install_housing.py \
  --source /srv/EQEmu --server /srv/eqemu/server \
  --client /srv/clients/RoF2 --build /srv/eqemu/housing-build \
  --backup /srv/eqemu/backups/housing-install-01
```

This first command produces a read-only plan. Follow the [installation instructions](docs/INSTALL.md) to stop writers and run it with `--apply`. Original client archives, private runtime content, credentials and character data are not distributed here.

## Scope

Representative original-client workflows have passed for personal and guild housing, furniture, crates, billing, property returns, guild-hall variants and the seven service items. See [current status](docs/STATUS.md) for the acceptance scope and [remaining limits](docs/KNOWN-GAPS.md) for unsupported behavior. Extended evolving/augmented/filled-bag storage also passed a representative original-client store/restart/retrieve cycle.

This is a reconstruction with documented server defaults, rather than an authenticated copy of the complete 2013 Live service. Plot boundaries/prices, some capacities and service appearances have explicit provenance. Unknown trophy benefits are disabled; the supported benefit profile contains only the three researched definitions.

- [Installation and upgrades](docs/INSTALL.md)
- [Configurable house and guild-hall capacities](docs/CONFIGURING-HOUSE-CAPACITIES.md)
- [Housing opcodes](protocol/OPCODES.md) and [packet structures](protocol/STRUCTS.md)
- [Implementation architecture](docs/ARCHITECTURE.md)
- [Historical implementation notes](docs/history/README.md)
