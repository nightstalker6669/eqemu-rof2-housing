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

- `utils/sql/housing/001...034`: numbered core migrations; read file prerequisites in order.
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


## Guild Hall visitor quest override

The source overlay installer does not copy quest files. If your `guildhall/player.lua` has the legacy unconditional unguilded-player boot, review and merge the separate [production quest override](../quest-overlay/guildhall/player.lua) into your active quest directory. Preserve unrelated local customizations and back up the current script. The override waits for the server-verified occupied-housing bootstrap; it retains the basic Guild Hall guild requirement. Deploy the matching C++ source and reload/restart quests together. See [interior Access editing](INTERIOR-ACCESS-2026-10-19.md).

## Same-plot ownership recovery

After026, apply the one-time034 relocation-ledger ALTER while housing writers are stopped, then deploy the matching world and zone builds. It retains legacy cross-plot history and adds explicit ownership-generation evidence for same-parent recovery. See [deployment and acceptance scope](SAME-PLOT-RECOVERY-2026-10-19.md).

### Optional existing personal-catalog crate transition

Schema035 creates an empty maintenance journal and does not activate content. The one-shot `CatalogCrateUpgrade` API requires a reviewed exact previous catalog/native graph, original content witnesses and stopped writers; it coherently advances current item/interior/purchase-policy bindings. Do not bypass the normal `Catalog::Import` managed-interior refusal or directly edit a bootstrap under existing houses. See [transition instructions and limits](CRATE-CATALOG-CAPACITY-2026-10-19.md).


## Vault-aware moving crates

Apply additive migration036 after the existing vault/crate schema, while housing writers are stopped. It creates an empty typed membership table; it does not seed items, activate retirement policies or rewrite existing physical members. Back up the database first. Deploy coherent world, zone and any inventory-recovery binaries linked to the same rebuilt common library: the internal scene/request structures changed. Do not run an older recovery CLI against the new build. Shared item data and client wire structs are unchanged by this milestone.

Without036 legacy non-vault behavior remains available; vault packing refuses rather than discarding contents. Do not remove the table while it has members. See [vault Move Out scope](VAULT-MOVE-OUT-2026-10-19.md).


### Individual vault-item crate transfers

The [individual-control follow-up](INDIVIDUAL-VAULT-CRATE-2026-10-19.md) uses the same036 table; no further migration or client patch is needed. Deploy matching world, zone and inventory-recovery CLI binaries rebuilt against the same common library. Internal vault contexts and recovery decoding now carry typed container witnesses. Legacy bindings without those witnesses retain their original bytes.

## Native retirement parcel overflow

Apply numbered migration037 after036 and the optional offline-direct-return, return-settlement and mixed-return-batch schemas. Stop housing writers for the change. Native parcel/inventory/custody tables must use InnoDB. The migration adds durable sidecars, a distinct queued crate location, and an overflow flag defaulting to0; it does not enroll existing properties. Enable overflow only in an exact current estate/catalogue batch review. Capacity exhaustion defers the complete property.

Deploy matching world, zone, UCS and inventory-recovery binaries linked to the same rebuilt common library. The tracked integration patch now includes `zone/parcels.cpp`; applying only new overlay headers omits the native retrieval adapter. A queued typed parcel must never use generic template-only retrieval. Keep sidecars and their claim tombstones; they outlive native parcel deletion. Review [retirement/returns](RETIREMENT-RETURNS.md) and [batch contracts](../overlay/common/housing/offline_return_batches.md) before activation.


## Standard guild-hall model door

Optional numbered content migration038 enables the existing `OBJ_GUILD_DOOR` row4 in `guildhall` version0 for RoF2 only. It requires the reviewed model, type and position; it does not create a missing door or expose the duplicate `GUILD_DOOR`. Apply only after reviewing local door content, then reload doors or restart the zone. Other customized masks are left unchanged. Deploy the updated zone hook with it: a housing-bound Standard hall exits through its verified current parent neighborhood, while absent housing bindings retain legacy door handling. The normal basic hall in version1 is outside this hook. No new client asset, opcode, wire struct or schema is required.


Original collision geometry: follow [original geometry installation](INSTALL-ORIGINAL-GEOMETRY.md) after applying the overlay. The runtime reads private authenticated resources from `housing-original-meshes` under the server working directory, or `EQEMU_HOUSING_ORIGINAL_ASSETS`. No client assets are included in this repository.


## Verified trophy profile, October 20

Use the [trophy scope](TROPHIES-2026-10-20.md) to distinguish reconstructed
content, implemented placement admission and native acceptance. This optional
profile enables Giant, Landing and guild Hardened Steel only. Verify the
referenced hidden stat items and keys in your own item database before use.

1. Back up content and state, stop world and **all zone processes**, and exclude
   other database writers. Deploy matching rebuilt world/zone/common binaries.
2. For a fresh profile, review `common/housing/trophy_supported_catalogue.sql`
   after the trophy, billing and guild schemas. Do not rerun this fresh insert
   script over existing keys. For an installation containing the earlier ten
   level1 tradeskill seeds, instead apply
   `common/housing/trophy_verified_catalogue_correction_20261020.sql`. The
   correction requires seven reference tables using InnoDB, accepts only exact
   old triplets or wholly absent triplets, and refuses affected selections or
   customized/partial rows. Resolve such conflicts explicitly; do not clear user
   state to force the correction.
3. Optionally apply
   `utils/sql/optional/2026_10_20_housing_verified_personal_trophy_placement.sql`
   for Landing68727/Giant68730. It requires InnoDB items, exact identities/models
   and empty or already-reviewed placement fields; a conflict rolls back both
   repairs. Unknown rotation/free-placement flags are not supplied. Review the
   [placement evidence](../overlay/common/housing/personal_trophy_placement.md).
4. Regenerate shared item data with the matching build for the **actual active
   prefix** selected by `variables.hotfix_name`, including a nonempty prefix if
   one is active. Confirm which item database supplies that generation and
   verify the reviewed item metadata is present there. Keep all zones stopped
   while replacing the dataset. Select the generated prefix **after** the
   `shared_memory` process finishes: this upstream tool can clear
   `variables.hotfix_name` when it contains `hotfix_`. Recheck the final variable
   before restarting world and zones, then verify the running processes attach
   the intended regenerated dataset.

Do not substitute `#hotfix` alone for step4. In this upstream code, the shared
item generation/hotfix path can load through `content_db`, while normal zone
startup attaches items through `database`; a hotfix success message and SQL
readback do not prove the cold-start inventory cache uses the repaired metadata.

No guessed tradeskill minimum levels are retained, and evolving physical trophy
placement remains disabled until its separate identity/progression can survive
custody, relog, recovery, containment and retirement. Unknown catalogue entries
remain excluded. The repository supplies no client assets or complete item seed.

## Additional reviewed house variants

Use [the portable variant generator](HOUSE-VARIANTS-2026-10-21.md) with the
operator's matching original client and prepared maps. Preserve every existing
configured template and its permanent bindings. Activate the generated proposal
through reviewed production catalogue/persistence setup in a fresh neighborhood;
generation itself makes no database or runtime changes.

Review the three optional October21 migrations: Evantil77407 item metadata,
missing original zone766 registration, and supported guild737/738/751 zone identity/
normal-player access. Their technical policies and guarded preconditions are
explicit in the SQL and variant documentation. Stop metadata writers, apply
against the correct configured database, regenerate shared items, select the
generated prefix after generation finishes, and cold-start world and every zone
to refresh both item and zone caches. A hotfix alone is insufficient.

For full room quotas, also install unseeded migration039 and review the separate
source-labelled capacity policy. The checked maintenance API appends policies
for existing rooms and exact template/base grants for future legacy allocations;
it does not rewrite original profile bytes or historic item tags. Native bootstrap
uses effective values. Keep all world/zones stopped for existing-room upgrades,
review the complete pinned plan and retain backups. Adding Grand requires merging
its registry entry with existing Palatial/Modest entries and a reviewed fresh
plot catalogue that permits definition39; do not rewrite a bound catalogue.
NPC quotas do not seed missing NPC item metadata. See [capacity and identity
boundaries](HOUSE-VARIANTS-2026-10-21.md#existing-rooms-and-future-preserved-templates).

Capacity maintenance requires the operator to stop every world and zone process. The API locks historical inventory process records without treating them as death certificates; it refuses active inventory sessions, leases or pending work. Installing capacities does not reconcile inventory or mark a process reaped.

### Guild-recipient policy and character saves

Apply [character-save compatibility](../overlay/utils/sql/optional/2026_10_21_housing_character_save_compatibility.sql) after installing guild inactivity policies.
The upstream character save uses `REPLACE`, whose transient delete conflicts
with the old recipient foreign key. The guarded migration removes only the exact
`housing_guild_inactive_recipient` constraint, preserving policy and receipt
rows; conflicting definitions are refused. Fresh guild-policy creation omits this
constraint. The service still locks and revalidates the current guild, leader,
recipient, account and nondeleted character before retirement. Stop concurrent
schema writers; no process restart or item-cache regeneration is required for
this DDL-only correction. The applied migration conserved policy rows, and
ordinary native zoning/save persisted the correct destination instance.
