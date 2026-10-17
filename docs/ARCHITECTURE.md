# Architecture and source map

This is a source overlay for the pinned upstream EQEmu revision, rather than a replacement server repository.

| Component | Main locations | Purpose |
|---|---|---|
| Packet framing | `common/housing/*wire*.h`, `common/patches/rof2.*`, `zone/client_housing*.cpp` | Bounded native packet decode/encode and handler dispatch |
| Catalog / geometry | `catalog_*`, `content_binding*`, `placement_*`, `interior_*`, `zone/map.*`, `zone/raycast_mesh.*` | Definitions, regions, item eligibility, source/destination binding, collision and mount checks |
| Ownership / access | `persistence_service*`, `access_policy*`, `current_estate_access*`, guild authority helpers | Current ownership generation, account/character/guild identity and ACL checks |
| Inventory bridge | `zone/housing_inventory_bridge*`, `zone/inventory_session_owner*`, inventory repositories | Real inventory snapshots, writer barriers, operation application/acknowledgement |
| Custody / storage | `live_item_service*`, storage/transfer/room/vault helpers | Raw item state preservation, durable possession and storage scopes |
| Houses / interiors | `house_*`, `scene_service*`, `zone/housing_house_*` | Stable house identity, permanent interior allocation, scene/routing and preload fences |
| Crates | `crate_*`, `zone/housing_crate_runtime*` | Typed crate identity, members, packing/unpacking, relocation and inventory transitions |
| Layouts | `layout_*` | Decode client load operations and apply mixed inventory/room/crate batches with journals |
| Economy / retirement | `native_plot_purchase*`, `coin_wallet*`, `escrow*`, `upkeep*`, `occupied_moveout*` | Locked policies, balances, idempotent receipts, lifecycle and recovery |
| Trophies / anchors | trophy/anchor helpers, `world/wguild_trophies.cpp`, zone handlers | Source-bound benefits, shared guild favor, activation/timers and transport |

The integration patch also changes generic upstream files because inventory lifetime, zoning, item serialization and world/zone messaging are cross-cutting concerns. Their generic names do not make those changes unrelated to housing.

## Authority and durability

The client describes intent; it does not supply trusted ownership, guild rank, item possession, geometry or paid-state authority. Requests pass through current session/lease/writer fences and fresh database/context checks. Custody, identity, placement, inventory transitions and receipts are committed together or reconciled through the retained application journal.

Wire sentinels are preserved rather than treated as interchangeable authority. A decoded struct is framing evidence, not permission to perform its requested operation. Shared ItemData/ItemInstance changes require coherent binaries and shared-memory cache generation on a new integration.

## Content portability

The source retains fixed development-database admission guards and relies on separately reviewed runtime house/room/crate registries and lawful game assets. It is intentionally not a database dump. A portable content pack and complete historical policy seeds remain future work.
