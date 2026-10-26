# Housing opcode inventory

Snapshot: October 25, 2026. Target: May 10, 2013 RoF2. **56 active mappings (50 real-estate and 6 guild-trophy);67 documented opcode numbers.** Mapping does not establish working gameplay or full original protocol coverage.

| Opcode | Mapping | Direction |
|---|---|---|
| 0x51e3 | OP_RealEstateCustomInteraction | C2S |
| 0x2691 | OP_RealEstateNeighborhood | C2S/S2C |
| 0x218c | OP_RealEstateGo | C2S |
| 0x2698 | OP_RealEstateDefinitions | S2C |
| 0x49df | OP_RealEstateRegions | S2C |
| 0x63fd | OP_RealEstateSnapshot | S2C |
| 0x6fd0 | OP_RealEstateZoneContext | S2C |
| 0x796c | OP_RealEstateInspect | C2S |
| 0x2b35 | OP_RealEstatePlotSearchOpen | S2C |
| 0x2aa7 | OP_RealEstateOwnedList | S2C |
| 0x485d | OP_RealEstateAccess | S2C |
| 0x3146 | OP_RealEstateAccessEdit | C2S |
| 0x45be | OP_RealEstatePurchase | C2S/S2C |
| 0x792c | OP_RealEstateSale | C2S |
| 0x507f | OP_RealEstateTransfer | C2S |
| 0x2aa6 | OP_RealEstateEscrow | C2S/S2C |
| 0x1cef | OP_RealEstateItemList | S2C |
| 0x087f | OP_RealEstateItemDelta | S2C |
| 0x626e | OP_RealEstatePlacementBegin | C2S/S2C |
| 0x711a | OP_RealEstatePlacementCommit | C2S |
| 0x1045 | OP_RealEstateWorldItem | S2C |
| 0x2a9e | OP_RealEstateWorldRemove | S2C |
| 0x2523 | OP_RealEstatePlacementSource | S2C |
| 0x6213 | OP_RealEstatePlacementEnd | S2C |
| 0x1228 | OP_RealEstateOperationResult | S2C |
| 0x2036 | OP_RealEstatePickup | C2S |
| 0x3424 | OP_RealEstateItemOperation | C2S |
| 0x613d | OP_RealEstatePack | C2S |
| 0x6917 | OP_RealEstateStoreAll | C2S |
| 0x5b41 | OP_RealEstateLoadLayout | C2S |
| 0x21ec | OP_RealEstateItemMenu | C2S/S2C |
| 0x050c | OP_RealEstateItemQuery | C2S |
| 0x1da9 | OP_RealEstateExit | C2S |
| 0x51a9 | OP_RealEstateTrophyOpen | C2S |
| 0x756a | OP_RealEstateTrophyToggle | C2S |
| 0x4b65 | OP_RealEstateTrophy | C2S/S2C |
| 0x7697 | OP_RealEstateTrophyList | S2C |
| 0x51ae | OP_RealEstateTrophyTimer | S2C |
| 0x08bf | OP_RealEstateTrophyClear | S2C |
| 0x7bb1 | OP_RealEstateAnchorConfirm | C2S/S2C |
| 0x1967 | OP_RealEstateOwnerItemPurge | S2C |
| 0x3e00 | OP_RealEstateOwnedItemList | S2C |
| 0x66df | OP_RealEstateArchivedList | S2C |
| 0x7db5 | OP_GuildTrophyAvailable | S2C |
| 0x7dd2 | OP_GuildTrophyActive | S2C |
| 0x039d | OP_GuildTrophyToggle | C2S |
| 0x7c1d | OP_GuildTrophySave | C2S |
| 0x7a41 | OP_GuildTrophyModify | C2S/S2C |
| 0x02bc | OP_GuildTrophyOptIn | C2S |
| 0x58c7 | OP_RealEstateItemInspect | C2S+S2C |
| 0x6573 | OP_RealEstateVotingOptOut | C2S |
| 0x21ba | OP_RealEstateHouseSwap | C2S |
| 0x66bf | OP_RealEstateSaleResult | S2C |
| 0x68ba | OP_RealEstateTransferResult | S2C |
| 0x5a63 | OP_RealEstateTransferConsent | S2C+C2S |
| 0x21ea | OP_RealEstateBindSpawn | S2C |

All 67 recovered layouts and explicit unknown fields are in [STRUCTS.md](STRUCTS.md) and [recovered-layouts.json](recovered-layouts.json). Detailed shared schemas remain in [housing-wire-reference.json](housing-wire-reference.json). Production serialization/decoding implementations are in [overlay/common/housing](../overlay/common/housing). Native/internal EQEmu roster fields are distinguished from client wire layouts in the integration patch.

Project opcode names are integration labels. Internal ServerOP_Housing messages are emulator coordination, not original-client discoveries.

The service integration activates previously recovered `0x21ea`; it adds no new recovered number. The canonical server profile is 32 bytes with a 16-character GUID plus NUL and a zero-filled unobserved gap. The original exclusive emitted length remains unknown. Representative Grand hall service placement and transactions passed; see [service acceptance](../docs/history/SERVICE-NPCS-2026-10-21.md) for remaining boundaries.
