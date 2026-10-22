# Complete housing installation and upgrades

`tools/install_housing.py` installs the production housing package as one ordered operation. Manual SQL filename sorting and separate feature activation are not required.

Existing installations receive the [October 22 fixes](UPDATES.md) through this same driver, including both door/border content migrations and the recognized unused Palatial catalogue repair. Use a new backup directory for each upgrade. Custom door conflicts or bound conflicting Palatial interiors are refused rather than overwritten.

## Choose your server

| Your server | Start here |
| --- | --- |
| Linux, installed directly | Continue with this guide |
| AkkStack / Docker Compose | [AkkStack upgrade steps](UPGRADE-AKK-STACK.md) |
| Native Windows `.exe` server | [Windows upgrade steps and validation limits](UPGRADE-WINDOWS.md) |

All paths use the complete housing package. Choose the guide for where the **server runs**, not where you play EverQuest.

## Source compatibility before you upgrade

The installer builds a separate clean source checkout at upstream commit `8175ae61874cbf20d7b05102821f6f8a4f4d1c39` plus this housing overlay. It preserves your original source directory, but **does not merge your custom C++ code or other upstream changes into the deployed binaries**. Compare your server revision and review custom features first. Merge work needs a deliberate source integration; this is not a binary drop-in for every EQEmu revision.

The plan reports your source HEAD and local changes. With `--apply`, a different or modified original checkout requires `--accept-pinned-server-build` before database work. That flag acknowledges the pinned rebuild; it is not a compatibility certification. Custom quest conflicts are separately refused. Take a rehearsal copy of a customized server/database before upgrading it.

## Supported prerequisites

Use an existing Linux EQEmu server and MariaDB, with its normal PEQ database, Lua quests, matching server dependencies, and a complete collision-map installation. The installer requires the configured item/content database and housing database to be the same physical database. Split database layouts and arbitrary MySQL variants are refused rather than partially migrated.

Provide your own **May 10, 2013 RoF2 client**. Its executable and renderer must match the hashes in the production geometry catalogs. Original archives and collision maps are authenticated locally; they are not downloaded or redistributed by this repository. Another RoF release or a current Live client is not interchangeable.

The machine running the installer needs Python 3.9+, Git, CMake, a C++20 compiler, EQEmu's ordinary build dependencies, and `mariadb`/`mysql` plus `mariadb-dump`/`mysqldump`. No Python database package is needed. The server directory must contain `eqemu_config.json`. The driver detects deployed binaries through the existing `world` link or `bin/world`, and reads the mapping, quests, maps and shared-cache directories from configuration. This includes AkkStack layouts with `bin/` binaries and `assets/patches/patch_RoF2.conf`. Use `--bin-dir`, `--opcode-dir`, `--quests-dir` or `--maps-dir` when those locations differ. The maps directory must contain `base/`; an existing runtime `maps` link must agree with it. MariaDB must allow packets of at least 64MiB (`max_allowed_packet=64M` or larger). Normal spell/damage-shield tables and the original moving-crate item 62586 metadata must already exist. Missing or conflicting native prerequisites are reported before installation writes.

The `--source` path is an EQEmu Git repository containing the pinned upstream commit. Its working tree is preserved: installation creates a separate managed source checkout and build directory under `--build`. Later releases get separate source/build directories. A modified managed source tree is refused until its changes have been reviewed.

## Plan and apply

Run this first without `--apply`:

```sh
python3 tools/install_housing.py \
  --source /srv/EQEmu --server /srv/eqemu/server \
  --client /srv/clients/RoF2 --build /srv/eqemu/housing-build \
  --backup /srv/eqemu/backups/housing-install-01
```

Use a new private backup directory for each application. The installer reads database credentials directly from the existing server configuration and uses a temporary mode 0600 client option file. Do not put credentials on the command line.

Stop **world and every zone** before repeating that command with `--apply`. The installer rejects active inventory sessions, leases and pending application state. It does not time out sessions, manufacture process-death certificates or reconcile unknown custody outcomes. Resolve a genuinely unfinished inventory operation through its existing supervised recovery workflow first.

An existing service manager can be invoked with explicit argument arrays:

```sh
python3 tools/install_housing.py \
  --source /srv/EQEmu --server /srv/eqemu/server \
  --client /srv/clients/RoF2 --build /srv/eqemu/housing-build \
  --backup /srv/eqemu/backups/housing-install-01 --apply \
  --stop-command '["systemctl","stop","eqemu"]' \
  --start-command '["systemctl","start","eqemu"]'
```

For Docker, stop the server container and run the installer in a one-off container with the same server/source/client mounts and network, overriding its entrypoint with Python. Keep the database container running. Start the normal server container only after installation succeeds. Do not run the installer by starting a second world/zone process.

## What is installed

The explicit `installer-migrations.json` orders every required housing migration, including the cross-directory prerequisites for vaults, retirement, guild returns, parcels, trophy billing, capacity policies and NPC services. The driver takes a complete database dump and a private server-file backup first. The file receipt records resolved binaries, mappings, quest/configuration files, existing generated resources and shared-cache contents, while retaining their logical deployment links. Newly generated files are journaled for checked rollback. It then checks base-table prerequisites and converts only the documented required MyISAM tables to InnoDB.

Existing structures are adopted without dropping their rows. The migration ledger records each exact source SHA. Duplicate columns and indexes are avoided; superseded check constraints are not temporarily reintroduced over newer history. This matters for existing generated-return origins. A different previously installed migration SHA or a conflicting schema/seed is refused. The supported trophy seed adopts only exact existing rows, with no blanket replacement of custom definitions.

The installer builds and deploys `world`, `zone`, `ucs`, `shared_memory`, `housing_install`, `housing_capacity`, `housing_recovery` and `housing_profile_upgrade`. Only documented housing opcode symbols are merged into `patch_RoF2.conf`; other mappings are preserved. The guild-hall quest override is installed only over its known original version or the already installed housing version. A custom conflicting quest must be merged first.

Original geometry is imported from the operator's client into private runtime storage. The house generator writes reviewed profiles and the complete selected house/hall definition set. Existing bound manifests and their bytes are preserved. Capacity changes use the versioned capacity ledger, retaining original profiles, placement/custody identities and historical fingerprints. Fresh profiles receive the configured limits automatically.

A fresh neighborhood is provisioned through the production neighborhood, plot and catalog services. It contains 71 personal plots and four guild plots, without fabricated player owners. Purchase and upkeep policies are bound to the resulting catalog revision and hashes. Existing owned neighborhoods are not replaced. The default installation key is `sunrise-housing-v1`; its completed receipt is idempotent. To deliberately create another neighborhood, use a new `--key` and `--name`.

The actor-free moving-crate configuration uses the original item 62586/definition 174 and authenticated crate resources. Existing legacy crate manifests retain priority so their placed-crate identity digests do not change. Banking, tribute and vendor/teleport service bindings use the reviewed production seeds. No test character, synthetic NPC stand-in or free property owner is created.

The selected shared-memory cache is generated only after item metadata is installed. The installer restores `hotfix_name` **after** `shared_memory` finishes, because upstream startup can clear the default `hotfix_` value. It then enables the four native housing rules in the configured active ruleset. All existing zones must start fresh with the matching binaries and cache; a hotfix command alone is insufficient.

## Defaults and preservation

The fresh neighborhood uses reconstructed map/collision boundaries, the documented later-Live tariff profile, and the explicitly inferred 103 Betwixt address. Personal yard limits follow the researched later table. Guild yards use the labelled 200-item/six-NPC server policy. The guild purchase member minimum is one as an emulator policy. These are not newly recovered 2013 Live values.

House/hall capacities are configured in `house_variant_capacity_defaults.json`; research evidence remains separate in `house_variant_capacities.json`. Vault and NPC quotas share the total budget. Allowing an NPC quota does not authorize an unreviewed NPC/model binding. Unknown trophy costs/minimum levels do not gain invented values through installation.

Installation supplies lifecycle mechanisms and native voluntary Move Out. It does not retroactively enroll arbitrary existing properties into destructive inactivity retirement or choose recipients for old guilds. Those actions retain their existing exact-generation policy and recipient review requirements.

## Completion, failure and rollback

A successful installation means the package, schemas, reviewed content, cache and rules were installed coherently. After startup, confirm one real personal/guild workflow with the matching client. This is distinct from claiming every item or every Live behavior has been accepted.

On failure, the installer does **not** automatically restart services. Keep writers stopped, read the private step log, and correct the stated conflict before retrying with a new backup directory. Successfully recorded migrations replay through their SHA ledger; partial DDL is inspected and adopted rather than blindly repeated. Existing housing inventory/history is never discarded to make a migration pass.

For full rollback, keep services stopped and inspect the checked restore plan:

```sh
python3 tools/restore_housing.py \
  --server /srv/eqemu/server \
  --backup /srv/eqemu/backups/housing-install-01
```

Repeat with `--apply` to restore the database and matching resolved binaries, mappings, configuration and cache together. Changed deployment links or subsequently modified newly installed files are refused before restoration. The restore tool does not start services. Review the restored state, then start the previous matching server. Do not restore only a database or only an older binary over a newer custody format. The original source checkout and original client remain untouched.
