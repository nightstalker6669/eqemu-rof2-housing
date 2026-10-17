# Current housing opcode inventory

Target: May10,2013 RoF2. **41 real-estate +6 guild-trophy mappings; not complete protocol closure.** Client opcode values exclude transport/opcode bytes from documented application lengths. Internal server coordination messages are not Live wire discoveries.

| Opcode | Name | Direction | Recovered purpose |
|---|---|---|---|
| `0x2691` | `OP_RealEstateNeighborhood` | C2S/S2C | search/My Plots/My Items/owners/plot search; neighborhood list type5/subtype0 or1; owners type5/subtype2; plot search type5/subtype4 or5; single plot inspection type4 or8 |
| `0x218c` | `OP_RealEstateGo` | C2S | neighborhood Go / placed-house Enter |
| `0x2698` | `OP_RealEstateDefinitions` | S2C | full definition+cost catalog replacement |
| `0x49df` | `OP_RealEstateRegions` | S2C | full region bootstrap |
| `0x63fd` | `OP_RealEstateSnapshot` | S2C | full estate snapshot |
| `0x6fd0` | `OP_RealEstateZoneContext` | S2C | full estate snapshot establishing zone root |
| `0x796c` | `OP_RealEstateInspect` | C2S | selected plot inspection |
| `0x2b35` | `OP_RealEstatePlotSearchOpen` | S2C | open Plot Search in inspect mode |
| `0x2aa7` | `OP_RealEstateOwnedList` | S2C | full estate-ID array replacement |
| `0x485d` | `OP_RealEstateAccess` | S2C | full permission snapshot/replacement |
| `0x3146` | `OP_RealEstateAccessEdit` | C2S | full permission snapshot/replacement |
| `0x45be` | `OP_RealEstatePurchase` | C2S/S2C | buy fixed envelope; management-window show acknowledgement |
| `0x792c` | `OP_RealEstateSale` | C2S | native Move Out / sale fixed envelope |
| `0x507f` | `OP_RealEstateTransfer` | C2S | ownership transfer fixed envelope |
| `0x2aa6` | `OP_RealEstateEscrow` | C2S/S2C | deposit/withdraw/select/day/week/withdraw-all; action response/cursor assignment |
| `0x1cef` | `OP_RealEstateItemList` | S2C | full estate furniture list |
| `0x087f` | `OP_RealEstateItemDelta` | S2C | upsert furniture entries |
| `0x626e` | `OP_RealEstatePlacementBegin` | C2S/S2C | cursor/stored-item Begin; preview actor + setup suffix |
| `0x711a` | `OP_RealEstatePlacementCommit` | C2S | transform commit |
| `0x1045` | `OP_RealEstateWorldItem` | S2C | rendered actor create/update |
| `0x2a9e` | `OP_RealEstateWorldRemove` | S2C | actor retirement reader prefix |
| `0x2523` | `OP_RealEstatePlacementSource` | S2C | retarget active preview source/IDs reader prefix |
| `0x6213` | `OP_RealEstatePlacementEnd` | S2C | cancel/end active preview |
| `0x1228` | `OP_RealEstateOperationResult` | S2C | shared housing result |
| `0x2036` | `OP_RealEstatePickup` | C2S | whole/split pickup to inventory |
| `0x3424` | `OP_RealEstateItemOperation` | C2S | storage operation |
| `0x613d` | `OP_RealEstatePack` | C2S | pack/unpack current context |
| `0x6917` | `OP_RealEstateStoreAll` | C2S | stow all in current context |
| `0x5b41` | `OP_RealEstateLoadLayout` | C2S | mixed-source layout load |
| `0x21ec` | `OP_RealEstateItemMenu` | C2S/S2C | placed-actor right-click menu query; housing-list action menu query; fixed40-record action menu |
| `0x050c` | `OP_RealEstateItemQuery` | C2S | actor fallback/menu action8 query |
| `0x1da9` | `OP_RealEstateExit` | C2S | leave current real estate |
| `0x756a` | `OP_RealEstateTrophyToggle` | C2S | enable/disable personal trophy benefits |
| `0x4b65` | `OP_RealEstateTrophy` | C2S/S2C | absolute benefit slot/tier selection; absolute benefit slot/tier selection |
| `0x7697` | `OP_RealEstateTrophyList` | S2C | active trophy state reader prefix |
| `0x51ae` | `OP_RealEstateTrophyTimer` | S2C | timer |
| `0x08bf` | `OP_RealEstateTrophyClear` | S2C | clear active benefits |
| `0x7bb1` | `OP_RealEstateAnchorConfirm` | C2S/S2C | anchor transport offer; accepted offer object |
| `0x1967` | `OP_RealEstateOwnerItemPurge` | S2C | purge estate cached entries by owner name |
| `0x3e00` | `OP_RealEstateOwnedItemList` | S2C | full estate-ID array replacement |
| `0x66df` | `OP_RealEstateArchivedList` | S2C | full estate-ID array replacement |
| `0x7db5` | `OP_GuildTrophyAvailable` | S2C | u32 guild,u32 count,u32 benefitIDs[count] |
| `0x7dd2` | `OP_GuildTrophyActive` | S2C | Minimal consumer-covered52bytes: guild,favor,timer,u32enabled,u32opaque,IDs[4],tiers[4]; full original server length not asserted |
| `0x039d` | `OP_GuildTrophyToggle` | C2S | 12bytes; DWORD0 bool; tail8 uninitialized/opaque, never identity |
| `0x7c1d` | `OP_GuildTrophySave` | C2S | 68bytes; IDs DWORDs20..35, tiers36..51; remaining bytes opaque |
| `0x7a41` | `OP_GuildTrophyModify` | C2S/S2C | C2S80bytes with initialized boolean byte0; implemented S2C acknowledgement boolean byte |
| `0x02bc` | `OP_GuildTrophyOptIn` | C2S | 80bytes; boolDWORD4,name[64]@8; packetguild/tail not authority |

Packet structs, offsets, variable-length framing and explicitly opaque fields are in [housing-wire-reference.json](housing-wire-reference.json). Guild trophy structs are implemented in [guild_trophy_wire.h](../overlay/common/housing/guild_trophy_wire.h). Integration names are project labels, not names extracted from the original executable.

Mapped framing does not establish complete game behavior. [Known gaps](../docs/KNOWN-GAPS.md) include five related unmapped messages.
