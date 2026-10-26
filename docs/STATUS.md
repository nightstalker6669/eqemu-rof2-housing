# Housing status

Target: original May 10, 2013 RoF2 client and pinned EQEmu source base. Linux/MariaDB is the accepted runtime; a native Windows installation path is implemented with the validation limits below. This status distinguishes implemented mechanisms from representative native acceptance and unknown Live data.

| Area | Implemented scope | Acceptance |
| --- | --- | --- |
| Neighborhoods | Instanced Sunrise Hills, plot markers/search/inspection, priced personal/guild plots, native purchase and owning-guild Lobby routing | Representative native personal/guild purchases and own-hall/basic fallback passed; pedestal-to-selected-plot management and purchase confirmed |
| Houses and guild halls | Persistent physical buildings and original interiors, retained identity across pickup/storage/moves, reviewed variant families and capacity policies | Representative personal, Standard, Modest, Palatial, Grand, Evantil and Hermit flows passed within documented scope |
| Placement | Original models/geometry, floors/walls/ceilings, move/rotate/scale, regional/collision checks, strict source identity | Representative furnishing placement, wall mounting and pickup passed; no per-item exhaustive claim |
| Storage and layouts | Closet/vault, partial stacks, shed, moving crates, pack/unpack, layouts and furnished-house retention | Representative native storage, crate, layout and house-return workflows passed |
| Billing and lifecycle | Reconstructed tariffs, hourly upkeep, deposit/withdrawal, reviewed recovery, voluntary retirement, opt-in inactivity returns and parcel overflow | Representative purchases/rent and generated-crate return flows passed; destructive legacy-property enrollment remains explicit |
| NPC furniture | Authenticated Holgresh actor binding and original model-default sizing, checked indoor preview projection, physical/source persistence | Representative real-model placement, relog and pickup passed; no invented binding for other plushies/pets |
| Guild services | Six purchased assistant contracts plus teleport device; banks, supply trades, tribute and one-crystal paid portal alignment | Seven items accepted in representative Grand flow; Standard built-in geode flow accepted without a placed device |
| Trophy benefits | Three researched reconstructed definitions, selection/activation, paid time, pause/resume and removal invalidation | Representative Giant personal and Hardened Steel guild cycles passed; Landing shares its reviewed placement class without a separate full cycle |
| Extended item storage | Canonical augment/filled-bag state, exact legacy scaling root/augment experience and same-UID evolving custody; ordinary-loader binding, held-writer guard, offline/parcel conservation | Eight matching production binaries deployed; original-client 40-slot filled-bag store, server restart and retrieval passed with augment, empty contained bag and same-UID evolving progress preserved. Legacy scaling root+augment direct vault store, relog and retrieval passed, preserving quest metadata; rollback/parcel checks passed |
| Installation | Ordered 89-migration driver, backup/adoption ledger, explicit database activation, source/build/deployment pipeline, original-resource import, fresh 75-plot provisioning | Actual isolated migration/adoption/retry, full orchestration, 75-plot provisioning and checked full rollback passed; orchestration reused the independently compiled eight production binaries |
| Native Windows upgrade | `.exe`/multi-configuration build/deployment, resolved DLL cohort and checked backup/restore, Windows process guards and portable arithmetic | Focused Windows-Python/Wine and portable arithmetic checks passed; full MSVC build, native server startup, and real NTFS permissions/junctions remain unverified; see [Windows guide](UPGRADE-WINDOWS.md) |

The [October 22 fixes](UPDATES.md) add the verified Palatial front-door exit and existing-catalogue placement correction. Brimming’s border transform is corrected with the legal region and placed hall pose preserved. Empty-escrow Add a Day confirmation remains outstanding; the update adds precise cursor errors rather than claiming that report fully reproduced.

The ten final tradeskill physical trophies have reviewed geometry and real evolving identity. Custody support does not manufacture their missing benefit minimum levels or costs. Unknown benefits remain disabled.

Detailed native records and implementation history are linked from [historical notes](history/README.md). Tests, private runtime IDs, packet captures, logs, fixtures and original client assets are not part of this repository.

## Housing pet controls (October 25, 2026)

The custom interaction opcode and bounded pet runtime are implemented for the admitted upright, unit-scale Holgresh profile. Builds, packet/menu checks and clean-overlay syntax checks passed. The local server was deployed and started. Native in-game acceptance of these new commands was explicitly deferred; do not interpret compilation as that proof. The Live spider remains unseeded. See [pet controls and limits](HOUSING-PETS.md).
