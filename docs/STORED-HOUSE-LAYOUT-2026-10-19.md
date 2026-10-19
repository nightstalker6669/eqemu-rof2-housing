# Stored-house layout restoration, October 19, 2026

Load Yard can now restore a reviewed house or guild hall from yard storage on its current plot. The layout uses the existing placement opcode and structure. No database migration, new opcode or new interior is required.

A stored house remains a bound object: custody, native item, creator, HouseID, catalog and permanent interior must match. Only a same-yard, single-house placement record is admitted. Restoration runs through the existing storage service inside the aggregate layout transaction, retaining permission, rent/upkeep, geometry and one-placed-house checks. The journal requires exactly the expected storage and pose revision changes while preserving all other identity and payload fields. A successful restore refreshes the estate and its furniture scenes.

The zone build and focused private journal checks passed, including valid Restore and existing Move round trips plus nine invalid transitions. The deployed original RoF2 client loaded a reviewed local layout and restored a furnished Standard Guild Hall on the guild plot at 200 Guild Way. Native Items immediately listed the hall as Yard/Placed and its original chair as House/Placed. Normal Enter reached the same permanent guild-hall interior, with original furniture retained. After camping and logging in again, the original client reloaded the same interior and its retained chair. Hall placement, house identity, interior route, furniture payload and scope were unchanged across the relog. Protected existing properties and items were also unchanged.

The native layout was prepared from original client XML item records with a reviewed destination pose. This confirms the client Load Yard path and server restoration; it does not establish that bulk Unpack or manual dragging succeeded. The preceding crate extraction used Shed. Cross-plot layout relocation, crate-contained house layout sources and arbitrary house stow operations are outside this change. Existing Live-equivalence and catalog gaps remain.

Only production source and documentation are published. Private checks, captures, client XML, accounts and runtime configuration remain excluded.

Subsequent [bulk Unpack acceptance](BULK-UNPACK-2026-10-19.md) completed Pack → Unpack into storage → Load Yard → Enter for the same representative furnished Standard Guild Hall.
