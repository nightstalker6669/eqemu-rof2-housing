# Integration notes

## 1. Source

Use a clean checkout of EQEmu commit `8175ae61874cbf20d7b05102821f6f8a4f4d1c39`. The overlay tool requires the exact commit and refuses local tracked/untracked changes. It applies the housing-only tracked patch and copies the new overlay files. It never connects to a database or starts a server.

```sh
git clone https://github.com/EQEmu/EQEmu.git EQEmu-housing
cd EQEmu-housing
git checkout 8175ae61874cbf20d7b05102821f6f8a4f4d1c39
git submodule update --init --recursive
cd /path/to/eqemu-rof2-housing
python3 tools/apply_overlay.py /path/to/EQEmu-housing --check
python3 tools/apply_overlay.py /path/to/EQEmu-housing --apply
```

## 2. Build coherently

Use upstream's CMake build process and dependencies, with C++20 and OpenSSL headers/libcrypto. Housing invokes SHA256/RAND APIs even when an alternative transport TLS backend is selected; mbedTLS-only linking is not validated here.

Rebuild world, zone and shared_memory together. The full integration changes shared ItemData/ItemInstance and common interfaces relative to upstream. Regenerate the shared item cache with the matching build before activation. Recovery/profile CLI sources are separate entrypoints; they are not automatically added as server executables. Use the provided CMake source lists; operational CLI entrypoints are separate tools.

## 3. Schema review

The export contains schema files, not a personal database dump. Review and apply them to a backed-up disposable database before any deployment:

- `utils/sql/housing/001...033`: numbered core migrations; read file prerequisites in order.
- Housing-named files under `utils/sql/optional/` and `utils/sql/git/optional/` add later features/columns.
- `common/housing/trophy_benefits.sql`, `trophy_billing.sql`, `guild_trophies.sql` add trophy state/billing/member support.

Some ALTER statements are not idempotent. Do not blindly execute every optional file or treat filename order across directories as a universal migration plan. Core custody/catalog/scopes precede storage/layout/lifecycle additions; trophy state precedes billing ALTERs; native purchase policy precedes guild-purchase ALTERs; NPC metadata precedes runtime/frame additions. Placement flag columns referenced by the modified generated item repository must exist even if the housing rule is disabled. A portable schema-version installer is not included.

## 4. Runtime/content setup

The feature is gated by the Housing rule and authenticated content loaders. This snapshot retains development fences referring to `peq_housing_test`, including house startup registration and recovery/profile tools. Renaming a production database is not an activation procedure; review/adapt those gates deliberately and revalidate.

Loaders expect reviewed house/room/crate registry JSON and lawful original assets (interior EQG files and matching collision maps). Personal runtime manifests, inventory adoption rows, account IDs and server configs are intentionally excluded. The included registries/numeric geometry are compiled dependencies; they do not replace the user's game installation.

Historical tariffs, all per-template vault/NPC limits, and all guild-hall capacities remain incomplete. Do not fill them from the one-copper/no-upkeep technical test settings. Missing values are documented gaps.

## 5. Protocol and rollout

Apply housing/guild-trophy mappings from the integration patch to the actual RoF2 opcode file used by the server. Message numbers are client-build-specific. Begin with a local disposable environment; validate coherent startup, inventory recovery, the current catalog and one representative personal/guild workflow before expanding coverage.

This repository is a research/development integration snapshot. A complete turnkey installer and full Live-equivalent acceptance are not supplied.

Legacy configuration schema names are retained in the production crate parser for compatibility. The parser is published as `crate_content_loader.h`; it is a runtime dependency, not a test fixture.


## October 18 optional schema additions

Review and apply only the schemas for the feature you are provisioning, after the documented base housing schema sequence:

- utils/sql/optional/2026_10_17_housing_estate_preferences.sql: per-ownership voting preference.
- utils/sql/optional/2026_10_17_housing_house_storage.sql: stored replacement-house intent.
- utils/sql/optional/2026_10_17_housing_house_swaps.sql: durable house-swap operation state.

Housing:EnableNativeHouseSwap defaults false. Native swap admission requires supported reviewed templates, same plot/creator, current house and item capabilities, valid occupancy/custody/rent checks and no placed moving crate in the old interior. The integration does not provision historical catalogue content or enable every experimental rule automatically.


## Optional lifecycle extensions and creation provenance

After the existing upkeep-cycle/full-property dependencies, review these migrations in order:

1. `utils/sql/optional/2026_10_18_housing_upkeep_rounding.sql`
2. `utils/sql/optional/2026_10_18_housing_offline_property_delivery.sql`
3. `utils/sql/optional/2026_10_18_housing_offline_property_packing.sql`
4. `utils/sql/optional/2026_10_18_housing_offline_crate_creation.sql`
5. `utils/sql/optional/2026_10_18_housing_crate_creation_origins.sql`

Apply the final provenance migration before enabling offline crate creation. It replaces the direct live-receipt foreign key with typed creation origins and backfills existing crates from genuine receipts. Review legacy crate integrity before migrating. Rebuild world and zone together for origin-aware readers/writers. These migrations do not activate automatic retirement globally; exact per-estate reviews and supported custody/content are required.

See [property lifecycle details](../overlay/common/housing/property_lifecycle.md) for currency, ownership, capacity, session and pending-operation restrictions. The optional `common/housing/trophy_supported_catalogue.sql` is a separately reviewed compatibility profile, not a prerequisite or authentic complete item seed; its referenced stat items and identifiers must exist and be independently checked. Evolving trophy placement remains unsupported.
