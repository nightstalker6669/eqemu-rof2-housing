# Housing updates

## Housing pet commands — October 25, 2026

Added the recovered `0x51e3` request, native pet menus, permission checks, and bounded noncombat pet movement/animation. The initial admitted model is Holgresh; unverified pet models remain disabled. See [pet controls and limits](HOUSING-PETS.md). No database migration is required.

## Housing fixes — October 22, 2026

These fixes are included in the complete installer for Linux, AkkStack and native Windows. Update your checkout, stop the server, and follow the same [installation/upgrade guide](INSTALL.md) with a **new private backup directory**. Do not copy only opcode mappings or import a replacement database. The installer retains ownership and existing items.

| Fix | Result |
| --- | --- |
| Plot pedestals | Clicking a pedestal opens that specific plot’s management/purchase panel. Plot Search remains available separately. |
| Existing Palatial catalogue placeholder | The installer corrects the recognized, unused definition40 placeholder from Sunrise Hills to the reviewed Palatial interior. It refreshes matching purchase/upkeep policy hashes in one transaction. A previously bound conflicting interior or mismatched policy is refused for explicit review. |
| Escrow controls | Add a Day/Week and Withdraw All give a clear message when cursor items or coins block the transfer. These controls use carried funds; escrow does not need an existing balance. The original reported zero-escrow failure was not independently reproduced after the cursor warning change. |
| Palatial exit door | The original FrontEntrance leaf is aligned with the collision-map opening and uses the checked current-house return route. Native clicking was confirmed to return the player to the owning neighborhood. |
| Guild-hall labels | Known experimental server labels now identify Palatial and Grand correctly. The original client’s built-in zone737 label can still say “Grand Guild Hall”; the physical Palatial interior is guildhalllrg. |
| 200 Brimming Way | The guild-plot border opening faces the valley ramp. A compensating translation preserves the border’s world XY bounds; legal plot regions and placed buildings are unchanged. |

No opcode numbers or packet layouts changed in this update. The [opcode list](../protocol/OPCODES.md) and [packet structures](../protocol/STRUCTS.md) remain included.

The exit-door height and border orientation are reviewed emulator content corrections, supported by original assets and the reported terrain/doorway positions. They are not claimed as recovered Live door records. The migrations preserve custom zone labels and refuse a conflicting Palatial door1. The Brimming correction only changes the known old marker transform.

Account credentials, GM characters, currencies, guilds, LAN addresses, private evidence and original client assets are excluded from the package. This update does not redistribute the testing account or change another operator’s network configuration.
