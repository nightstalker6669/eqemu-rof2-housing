# Recovered housing packet layouts and structs

Snapshot: October 18, 2026. All offsets/lengths below describe application bodies, excluding transport and opcode bytes. Multi-byte scalar fields use little endian unless a codec explicitly says otherwise. These are serialization layouts, not permission to cast arbitrary packet bytes to packed C++ objects. Production codecs perform their own length and value checks.

There are67 documented numbers:47 complete and20 partial documented layouts; meanings are30 complete,34 partial and3 unknown. A complete classification covers only the documented variants. Consumer-accessed minimum size is not an authenticated original server-emitted length. Emulator compatibility profiles do not prove original Live field meanings or behavior.

Canonical definitions include estate_wire.h, neighborhood_wire.h, permission_wire.h, placement_wire.h, item_list_wire.h, interaction_wire.h, transfer_consent_wire.h, ownership_transfer_service.h, native_purchase_wire.h, house_swap_service.h, layout_wire.h, guild_trophy_wire.h and trophy_anchor_wire.h in overlay/common/housing. See the detailed shared-schema dictionary in housing-wire-reference.json for field-by-field structures and embedded ItemPtr framing.

## 0x02bc — OP_GuildTrophyOptIn

Direction: C2S. Mapped: yes. Layout: complete; meaning: complete.

C2S exact80 in named-member and selfcheckbox variants: current guild/referenceu32@0, enabledu32@4, NULname[64]@8; remaining8bytes opaque/stale. Self variant joins shared7a41 send block with02bc opcode.

## 0x039d — OP_GuildTrophyToggle

Direction: C2S. Mapped: yes. Layout: complete; meaning: complete.

C2S exact12: u32enabled@0; tail8 uninitialized/stale, not identity. Both toggle value and unused tail extent established by sender.

## 0x050c — OP_RealEstateItemQuery

Direction: C2S. Mapped: yes. Layout: complete; meaning: partial.

C2S exact12: estateID,itemID,parameter; actor fallback usesFFFFFFFF, menu action8 forwards server record parameter.

Remaining limits: Non-FFFFFFFF menu action8 parameters are forwarded without local interpretation; query option taxonomy unknown.

## 0x087f — OP_RealEstateItemDelta

Direction: S2C. Mapped: yes. Layout: complete; meaning: partial.

u32 estateID,LP32 title,u32 count,then count complete entries;12+titleBytes+sum(entryBytes). EntryBytes=49+ownerNameBytes+ownerHandleBytes+ItemPtrBytes. DefinitionD=785+sum13Zstringchars+4*socketCount. ItemPtrBytes=97+GUIDchars+ornamentChars+25E+D+4*estateIDCount+sum(4+childItemPtrBytes). Complete standalone offline decoder now supplies formerly external D grammar.

Remaining limits: Separate owner/creator handle semantics and some nested item-definition product fields remain incompletely interpreted.

## 0x08bf — OP_RealEstateTrophyClear

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

S2C no body consumed; clears active trophy enabled state and ten selections. Empty overlay encoding is a consumer-covered implementation choice.

Remaining limits: No payload access does not prove original08bf body was empty.

## 0x0c13 — Owned-item estate membership delta

Direction: S2C. Mapped: no. Layout: partial; meaning: partial.

estateID:u32@0; addFlag:u8@104

Remaining limits: Only estateID@0 and addFlag@104 consumed;100 intermediate bytes and total length unknown; Ignored intermediate fields have no established semantic names

## 0x1045 — OP_RealEstateWorldItem

Direction: S2C. Mapped: yes. Layout: complete; meaning: partial.

S2C complete serialized ActorRecord only:Z model,u8 NPC flag,f32 NPC height,u32 RecordNum,Z GUID,IDs,scale-first7 floats,3 mode flags,placementType,6 default/scale floats,lightType. Grammar82+modelBytes+GUIDBytes, normally98+modelBytes with16-character GUID. No ItemPtr dependency.

Remaining limits: RecordNum and complete nonordinary/NPC model-resource association remain incompletely attributed; RecordNum must not be called itemID.

## 0x1097 — Update existing housing item

Direction: S2C. Mapped: no. Layout: complete; meaning: partial.

u32 outerEstateID,one complete entry;4+entryBytes. EntryBytes=49+ownerNameBytes+ownerHandleBytes+ItemPtrBytes. DefinitionD=785+sum13Zstringchars+4*socketCount. ItemPtrBytes=97+GUIDchars+ornamentChars+25E+D+4*estateIDCount+sum(4+childItemPtrBytes). Complete standalone offline decoder now supplies formerly external D grammar.

Remaining limits: Nested item opaque-field roles are not all recovered

## 0x1228 — OP_RealEstateOperationResult

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

S2C shared completion consumer reads a signed32-bit result at0;4 bytes consumed/validated encoder size, not an independently proved original exclusive emitted length. Complete retained result branch inventory includes52 direct eqstr branches,-1 special handling,0 cleanup,45 partial-stack success,83 indirect formatting and generic fallback.

Remaining limits: Four consumed bytes do not establish original full body size or absence of ignored suffix.; Operation-specific request/result ordering and authoritative outcome are not established for every sender. Resolving result83 label does not close transaction correlation.

## 0x1967 — OP_RealEstateOwnerItemPurge

Direction: S2C. Mapped: yes. Layout: complete; meaning: complete.

S2C estateIDu32 followed by NULownerName; total5+ownerNameBytes; purges all matching owner-name cached entries within estate.

## 0x1baf — Remove housing item

Direction: S2C. Mapped: no. Layout: partial; meaning: complete.

estateID:u32@0; itemID:u32@4

Remaining limits: Only8 consumed bytes established; exclusive original full packet length not demonstrated

## 0x1cef — OP_RealEstateItemList

Direction: S2C. Mapped: yes. Layout: complete; meaning: partial.

u32 estateID,u32 count,then count complete entries;8+sum(entryBytes). EntryBytes=49+ownerNameBytes+ownerHandleBytes+ItemPtrBytes. DefinitionD=785+sum13Zstringchars+4*socketCount. ItemPtrBytes=97+GUIDchars+ornamentChars+25E+D+4*estateIDCount+sum(4+childItemPtrBytes). Complete standalone offline decoder now supplies formerly external D grammar.

Remaining limits: Separate owner/creator handle semantics and some nested item-definition product fields remain incompletely interpreted.

## 0x1da9 — OP_RealEstateExit

Direction: C2S. Mapped: yes. Layout: complete; meaning: complete.

C2S exact4 opaque/stale bytes: leaverealestate command overwrites opcode only; body is not an estateID.

## 0x2036 — OP_RealEstatePickup

Direction: C2S. Mapped: yes. Layout: complete; meaning: partial.

C2S exact44: GUID18,padding2,source threeu32@20/24/28,estateID@32,itemID@36,quantity@40; ordinary and stack-dialog producers share envelope.

Remaining limits: Quantity0 is ordinary native constructor value; exact whole/remainder and partial-fit server behavior is not fully established.

## 0x218c — OP_RealEstateGo

Direction: C2S. Mapped: yes. Layout: complete; meaning: complete.

C2S exactly12 bytes:estateID,itemID,zero DWORD. Neighborhood Go uses itemIDFFFFFFFF; placed-house/menu/actor branches use actual itemID. No destination coordinates or zone are serialized.

## 0x21ba — OP_RealEstateHouseSwap

Direction: C2S. Mapped: yes. Layout: complete; meaning: partial.

C2S exact24: FFFFFFFF@0, zero@4, current placed house estateID/itemID@8/12, selected replacement house estateID/itemID@16/20. Last pair reads live ItemWnd+384/+388 at confirmation; callback receiver is ItemWnd+220.

Remaining limits: Roles of initial constants FFFFFFFF and zero remain unnamed; both house identity pairs are recovered.; Replacement source/custody and response ordering unresolved

## 0x21ea — OP_RealEstateBindSpawn

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

S2C minimum32 accessible bytes:spawnID:u32@0;accepted canonicalGUID is16 chars+NUL@4..20;bytes21..23 unobserved on valid profile;estateID:u32@24,itemID:u32@28. Prior18-byte GUID storage wording overstated wire consumption. Malformed unbounded strlen reads are not extra valid fields.

Remaining limits: 32 accessed bytes established, including unobserved gap21..23; original full emitted length unknown


The implemented per-viewer publisher emits canonical 32 bytes with gap21..23 zero-filled after the housing item list. This integration reuses the recovered number. Representative Grand hall service placement and transactions passed; see [service acceptance](../docs/history/SERVICE-NPCS-2026-10-21.md) for remaining boundaries.

## 0x21ec — OP_RealEstateItemMenu

Direction: C2S/S2C. Mapped: yes. Layout: complete; meaning: partial.

C2S placed-actor and housing-list queries, S2C response: shared fixed768-byte Menu768; GUID18/pad2; source12;estate/item@32/36;40x16 records@108;hidebyte748;popup coordinates752/756;opaque760/764. Constructor542a90 plus full40-record parser7375f0 corroborate shared shape.

Remaining limits: Server supplied record parameters/action labels include unnamed action19/64a3; custom action9/51e3 parameter taxonomy and opaque header values are not fully interpreted.; Documented768-byte shared grammar is recovered; original server emission and action eligibility were not captured.

## 0x2523 — OP_RealEstatePlacementSource

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

S2C minimum40 accessible bytes. Matching-preview helper comparesGUID bytes0..15,does not use16..19,then copies sourceDWORDs20..31 andestateID32/itemID36. SharedGUID18+padding2 identity structure remains a compatible producer profile,not a recovered full S2C length.

Remaining limits: Original exclusive body size and unconsumed suffix are unresolved.

## 0x2691 — OP_RealEstateNeighborhood

Direction: C2S/S2C. Mapped: yes. Layout: complete; meaning: partial.

C2S exactly224 bytes, modes0/1/2/4/5, fixed80/64/64-byte query strings and flag/padding suffix. S2C documented type5 subtypes0/1:12 bytes if count0, otherwise16+sum(28+nameBytes); subtype2:13+sum(4+playerNameBytes)+4*guildCount; subtypes4/5:15+titleBytes+sum(20+stringABytes+stringBBytes). Plot types4/8 consume one inspection record:64+fourStringBytes. LP32 strings and every count/conditional branch of these documented variants are framed.

Remaining limits: Request flags220/221, ignored response header/context domains, plot-search row labels and some inspection words/strings remain unnamed. Subtype3 is ignored, not a newly recovered data grammar.

## 0x2698 — OP_RealEstateDefinitions

Direction: S2C. Mapped: yes. Layout: complete; meaning: partial.

S2C u32 definitionCount + count-driven complete DefinitionRecord grammars + u32 costSchemeCount + nested schemes/components/16-byte cost values. Minimum8 bytes for both zero counts; variable strings/counts are explicitly framed.

Remaining limits: Several definition attributes/currency type-subtype domains and their product policies are not completely interpreted; native framing does not recover historical prices or upkeep charging cadence.

## 0x2a9e — OP_RealEstateWorldRemove

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

S2C minimum40 accessible bytes. EstateID32/itemID36 retire world actor;conditional preview matcher comparesGUID bytes0..15. Audited consumers do not interpretbody16..31 as a source tuple. Original full extent remains unknown.

Remaining limits: Any original unconsumed suffix and exclusive full emitted size remain unknown.

## 0x2aa6 — OP_RealEstateEscrow

Direction: C2S/S2C. Mapped: yes. Layout: complete; meaning: partial.

Both directions use u32 action,estateID,overloadedID;2 Z strings;u8 flag;opaque u32;valueCount;16-byte values. Complete grammar23+string1Bytes+string2Bytes+16*valueCount. Documented C2S actions0/1/2/3/4/5/6/7 all fit it. New5/6 item-row day/week carry RealEstateUpkeepID at8;3/4 property-level day/week use constructor0. Known response0/1/2 cursor/component cases add no wire fields.

Remaining limits: Opaque word/string roles, action5/6 server grouping/response effect, funding and charge rules remain unresolved; ID@8 is action-dependent, not universally componentID.

## 0x2aa7 — OP_RealEstateOwnedList

Direction: S2C. Mapped: yes. Layout: complete; meaning: complete.

S2C u32 count plus count estateIDs:4+4*count bytes. Replaces general OwnedRealEstates array, distinct from owned-item/archive replacements.

## 0x2b35 — OP_RealEstatePlotSearchOpen

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

S2C consumer reads byte0 only:applies inspection-mode flag to PlotSearch after toggling/showing it. At least1 accessible body byte is proved; original exclusive full body length is not recovered.

Remaining limits: One consumed byte is not an original full emitted length.

## 0x2cf7 — Upsert housing item

Direction: S2C. Mapped: no. Layout: complete; meaning: partial.

u32 outerEstateID,one complete entry;4+entryBytes. EntryBytes=49+ownerNameBytes+ownerHandleBytes+ItemPtrBytes. DefinitionD=785+sum13Zstringchars+4*socketCount. ItemPtrBytes=97+GUIDchars+ornamentChars+25E+D+4*estateIDCount+sum(4+childItemPtrBytes). Complete standalone offline decoder now supplies formerly external D grammar.

Remaining limits: Nested item opaque-field roles are not all recovered

## 0x3146 — OP_RealEstateAccessEdit

Direction: C2S. Mapped: yes. Layout: complete; meaning: complete.

C2S queued complete snapshot replacement:estateID,7 unconditional groupID/capabilityMask pairs, memberCount, repeated LP32 name + groupID. Complete grammar64+sum(8+nameBytes). No trailing3968 hash-key word, requestID or ACL revision.

## 0x3424 — OP_RealEstateItemOperation

Direction: C2S. Mapped: yes. Layout: complete; meaning: partial.

C2S exact48: OperationSource44 plus destination actionu32@44; concrete buttons2closet,3shed,6crate, context-derived supertype values retain same framing.

Implementation follow-up: typed nonplaceable members in a placed crate now use the existing source quantity for individual Shed/Crate transfers and `0x2036` Inventory retrieval. A representative native personal-yard whole/partial cycle passed. Container/member revisions are internal server witnesses, not added packet fields. See [scope](../docs/history/INDIVIDUAL-VAULT-CRATE-2026-10-19.md).

Remaining limits: Zero quantity and context-derived actions0/1/4/5/7/8/9 do not establish complete custody operations or destination policy.

## 0x3968 — Access response and cache invalidation variant

Direction: S2C. Mapped: no. Layout: complete; meaning: partial.

estateID:u32; variable access snapshot; opaque:u32

Remaining limits: TrailingDWORD has a conditional hash-erase use; exact identifier/domain unknown; Unusual null-global/ECX14 branch is static, not proved a valid runtime state

## 0x3c8b — Housing-item text request; exact operation unresolved

Direction: C2S. Mapped: no. Layout: complete; meaning: unknown.

Exactly352 bytes: eight u32 header fields {2,0,estateID,itemID,0,0,0,2}; char text[128]@32; zero[128]@160; zero[64]@288

Remaining limits: Text request initiating prompt and effect unknown; constructor/header roles only partly assigned; No response/effect pairing established

## 0x3e00 — OP_RealEstateOwnedItemList

Direction: S2C. Mapped: yes. Layout: complete; meaning: complete.

S2C u32count then countu32estateIDs, exact documented4+4*count; clears/replaces PcClient OwnedItemRealEstates+29bc, not general ownership.

## 0x45be — OP_RealEstatePurchase

Direction: C2S/S2C. Mapped: yes. Layout: partial; meaning: partial.

C2S exactly188-byte transaction envelope:estateID;sentinels;fixed64/32/64-byte spans;price/upkeep componentIDs at172/176;clientCachedGuildRosterCount DWORD180;flags/padding through187. S2C branch consumes only byte0 (nonzero shows management), with minimum1 accessible byte; native original full response length is unknown.

Remaining limits: The documented S2C show-response has one consumed byte, not a proved exclusive emitted length.; Other transaction opaque spans and flags retain unknown roles; DWORD180 is the client cached guild-roster entry count, not evidence of authoritative guild size or funding policy.

## 0x485d — OP_RealEstateAccess

Direction: S2C. Mapped: yes. Layout: complete; meaning: complete.

S2C replacement snapshot:estateID,7 unconditional groupID/capabilityMask pairs, memberCount, repeated LP32 name + groupID. Complete grammar64+sum(8+nameBytes). No trailing3968 hash-key word, requestID or ACL revision.

## 0x49df — OP_RealEstateRegions

Direction: S2C. Mapped: yes. Layout: complete; meaning: partial.

S2C u32 count followed by exactly88 bytes per region:ID,16 float transform words,3 float opaque vector words,2 opaque DWORDs. Complete documented bootstrap grammar4+88*count; empty bootstrap4 bytes.

Remaining limits: Trailing vector at record68 and words80/84 have no recovered product meanings.

## 0x4b65 — OP_RealEstateTrophy

Direction: C2S/S2C. Mapped: yes. Layout: partial; meaning: complete.

C2S exact12: slotu32,benefitIDu32,tieri32 resulting absolute selection; removal can useFFFFFFFF/-1. S2C same12 interpreted bytes, but native evidence labels12 minimum accessed, not proven original full extent.

Remaining limits: S2C exact total body extent is not established beyond consumed12 bytes; C2S exact12 is established.

## 0x4f09 — Remove estate using full snapshot

Direction: S2C. Mapped: no. Layout: complete; meaning: partial.

Full RealEstate snapshot, same serialization family as 0x63fd

Remaining limits: Removal behavior is established, but shared snapshot nonzero currency-key and upkeep-expiry domains retain the same limits as63fd

## 0x507f — OP_RealEstateTransfer

Direction: C2S. Mapped: yes. Layout: complete; meaning: partial.

C2S0x507f exactly188 bytes:current estateID at0,target PlayerClient.Name copied into fixed64-byte field108;remaining shared-constructor defaults, including unspecified byte187. It is ownership transfer, not the stale507f sale label. Incoming offer5a63 is a separate documented opcode.

Remaining limits: Other188-byte transaction fields remain opaque for this producer; recipient name108 must not be assumed to occupy the same role in all transfer-offer fields.; 507f initial request→server offer5a63 temporal correlation and later68ba ordering are not established by the client alone.

## 0x51a9 — OP_RealEstateTrophyOpen

Direction: C2S. Mapped: yes. Layout: complete; meaning: partial.

Exactly0 application bytes;2-byte frame contains only opcode

Remaining limits: Trophy-window association known; personal/guild domain and exact state requested unresolved; No catalog/response pairing established; audited same-opcode receive paths default

## 0x51ae — OP_RealEstateTrophyTimer

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

S2C signed millisecond countdown DWORD@0; minimum4 consumed bytes, exact original extent unknown. Active local timer subtracts elapsed milliseconds and clamps to zero; display rounds via (timer+500)/1000 to minutes:seconds.

Remaining limits: Only firstDWORD consumed; original full S2C extent not established.

## 0x51e3 — Custom placed-object interaction request

Direction: C2S. Mapped: **OP_RealEstateCustomInteraction**. Layout: complete; meaning: partial.

`estateID:u32, itemID:u32, parameter:u32`, exactly 12 little-endian bytes. Decoder and struct: [pet_commands.h](../overlay/common/housing/pet_commands.h).

The server implements parameters 16175 Stay, 16176 Walk Around, 16177 Run Around, 16178 Follow Me, and 16179 Play Dead for its admitted pet profile. This bounded set is based on an owned Live pet menu and matching original category28 labels. It does not establish other parameter meanings or exact historical item eligibility. See [behavior and model limits](../docs/HOUSING-PETS.md).

## 0x58c7 — OP_RealEstateItemInspect

Direction: C2S+S2C. Mapped: yes. Layout: complete; meaning: partial.

C2S exactly8 bytes (estateID,itemID);S2C exactly one complete variable recursive ItemPtr with no packet-type/estate/result prefix. EntryBytes=49+ownerNameBytes+ownerHandleBytes+ItemPtrBytes. DefinitionD=785+sum13Zstringchars+4*socketCount. ItemPtrBytes=97+GUIDchars+ornamentChars+25E+D+4*estateIDCount+sum(4+childItemPtrBytes). Complete standalone offline decoder now supplies formerly external D grammar.

Remaining limits: Display purpose established; embedded item-field roles are not all recovered

## 0x5a63 — OP_RealEstateTransferConsent

Direction: S2C+C2S. Mapped: yes. Layout: partial; meaning: partial.

estateID:u32@0; senderText:char[64]@12; acceptance:u8@185; other fields preserved or opaque

Remaining limits: C2S is188 bytes; incoming copy accesses187 bytes without proving full S2C emitted extent; Most transaction fields merely copied; acceptance byte185 depends on local estate cache; Initial transfer request and subsequent result/ownership sequence not linked to this offer

## 0x5b41 — OP_RealEstateLoadLayout

Direction: C2S. Mapped: yes. Layout: complete; meaning: partial.

C2S u32yard,u32house,u16count; count tagged records: tag0=17bytes(tag+fouru32 sourceEstate,item,quantity,targetEstate); tag1=65bytes for canonicalGUID16+NUL, compact7-byte source,estate/item,sevenf32 HPRScaleXYZ,targetEstate. Total10+17*n0+65*n1.

Remaining limits: Tag0 zero-quantity execution and stack split/merge output identity/state ordering remain partly inferred; repeated tag1 source GUID is valid source provenance, not unique placed identity.

## 0x613d — OP_RealEstatePack

Direction: C2S. Mapped: yes. Layout: complete; meaning: complete.

C2S exact1: u8 pack flag1=pack,0=unpack; current estate context implicit.

## 0x6213 — OP_RealEstatePlacementEnd

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

S2C end-preview consumer reads no body. Zero consumed bytes permits an empty body but does not establish the original emitted frame was empty. Housing preview cleanup interface+0xc resolves to RET4; no C2S cancel counterpart is proved on that path.

Remaining limits: Original emitted body length remains unknown despite the zero-byte consumer.

## 0x626e — OP_RealEstatePlacementBegin

Direction: C2S/S2C. Mapped: yes. Layout: complete; meaning: partial.

C2S exactly60 bytes for cursor/stored/menu Begin profiles:GUID18+padding,source12,estate/item IDs,constant/opaque40,action44,flag48,padding,screen coordinates52/56. S2C is serialized ActorRecord (82+modelBytes+GUIDBytes) plus compressed source7,3 flags,3 DWORDs,count+KEY/RESULT pairs and required final KEY/RESULT pair:ActorRecord+34+8*N. This response is PlacedItem/ActorRecord, NOT full ItemPtr/ItemDefinition D.

Remaining limits: Begin action taxonomy beyond observed profiles, meaning of constant/opaque40 and third setupDWORD remain unresolved. Wire-reference flag3 unnamed wording is superseded:it controls screen-ray initialization in retained native input recovery.

## 0x63fd — OP_RealEstateSnapshot

Direction: S2C. Mapped: yes. Layout: complete; meaning: partial.

S2C full EstateSnapshot:54+ownerNameBytes+ownerHandleBytes+dynamicNameBytes+4*childCount+12*escrowCount; LP32 strings, u32 child count/IDs, u8 escrow count and currencyKey/f64 balance pairs, final upkeep/expiry/voting fields. 63fd updates the estate cache without establishing zone-root context.

Remaining limits: Nonzero escrow-key currency domains and complete upkeep-expiry encoding/charge interpretation remain unresolved; expiry must not be relabeled an invented paid-until timestamp.

## 0x64a3 — Unresolved menu action19 request

Direction: C2S. Mapped: no. Layout: complete; meaning: unknown.

estateID:u32,itemID:u32,parameter:u32 exactly12 bytes

Remaining limits: Action19 exact operation and parameter domain unknown; No response relationship established

## 0x6573 — OP_RealEstateVotingOptOut

Direction: C2S. Mapped: yes. Layout: complete; meaning: complete.

Exactly8 bytes: estateID:u32@0; votingOptedOut:u8@4; opaque/stale[3]@5

Remaining limits: Native local cache/checkbox update does not establish server persistence or acknowledgement

## 0x66bf — OP_RealEstateSaleResult

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

result:u32@0

Remaining limits: Only resultDWORD prefix established; full original body size unknown; No request/result ordering proof

## 0x66df — OP_RealEstateArchivedList

Direction: S2C. Mapped: yes. Layout: complete; meaning: complete.

S2C u32count then countu32estateIDs, exact documented4+4*count; clears/replaces ArchivedRealEstates+29cc.

## 0x68ba — OP_RealEstateTransferResult

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

result:u32@0

Remaining limits: Only resultDWORD prefix established; full original body size unknown; No transfer request/result ordering or successful state transition established

## 0x6917 — OP_RealEstateStoreAll

Direction: C2S. Mapped: yes. Layout: complete; meaning: complete.

C2S exact0 application bytes; stow placed items in implicit current housing context.

## 0x6fd0 — OP_RealEstateZoneContext

Direction: S2C. Mapped: yes. Layout: complete; meaning: partial.

S2C full EstateSnapshot:54+ownerNameBytes+ownerHandleBytes+dynamicNameBytes+4*childCount+12*escrowCount; LP32 strings, u32 child count/IDs, u8 escrow count and currencyKey/f64 balance pairs, final upkeep/expiry/voting fields. 6fd0 additionally establishes zone-root context.

Remaining limits: Nonzero escrow-key currency domains and complete upkeep-expiry encoding/charge interpretation remain unresolved; expiry must not be relabeled an invented paid-until timestamp.

## 0x711a — OP_RealEstatePlacementCommit

Direction: C2S. Mapped: yes. Layout: complete; meaning: partial.

C2S exactly96 bytes:GUID/source/IDs40-byte prefix,opaque DWORD40,heading/pitch/roll/scale44..56,position Y/X/Z60..68,already scale-adjusted actor-local extents72..80 and center84..92. Stored Commit source-kind000000FF differs from stored BeginFFFFFFFF. Vtable+8 submit→5b1f90 and click path are bound; no extra revision/operation ID is serialized.

Remaining limits: DWORD40 is emitted as0 but its original protocol meaning remains unassigned.

## 0x756a — OP_RealEstateTrophyToggle

Direction: C2S. Mapped: yes. Layout: complete; meaning: complete.

C2S exact4: u32enabled inverse of local personal trophy enabled byte; native sender784cb5.

## 0x7697 — OP_RealEstateTrophyList

Direction: S2C. Mapped: yes. Layout: partial; meaning: complete.

S2C consumed84-byte prefix: u32enabled;10u32 benefitIDs@4;10i32 tiers@44. Two separate arrays, not interleaved pairs.

Remaining limits: Original full7697 body length or any suffix beyond byte83 remains explicitly unknown.

## 0x76c6 — General owned-estate membership delta

Direction: S2C. Mapped: no. Layout: partial; meaning: partial.

estateID:u32@0; addFlag:u8@104

Remaining limits: Only estateID@0 and addFlag@104 consumed;100 intermediate bytes and total length unknown; Intermediate field roles unresolved

## 0x792c — OP_RealEstateSale

Direction: C2S. Mapped: yes. Layout: complete; meaning: partial.

C2S0x792c exactly188 bytes. Native Move Out notification100 affirmative mask5:shared constructor defaults, current estateID at0. Constructor leaves byte187 unspecified; all188 bytes are copied. This supersedes the stale wire-reference entry that named507f as Sale.

Remaining limits: Fixed-envelope sentinels/opaque spans/flags retain unassigned semantics even though the sale request and default bytes are known.

## 0x796c — OP_RealEstateInspect

Direction: C2S. Mapped: yes. Layout: complete; meaning: complete.

C2S exactly4 bytes containing selected estateID; emitted by PlotSearch selected-row path in inspection mode.

## 0x7a41 — OP_GuildTrophyModify

Direction: C2S/S2C. Mapped: yes. Layout: partial; meaning: complete.

C2S exact80 in close/notification paths; only booleanbyte0 deliberately initialized,79 remainingbytes opaque/stale. S2C original consumer4860f1 reads byte0 into modifyack globalddf04a; overlay emits1-byte ack.

Remaining limits: S2C one consumed booleanbyte does not establish exact original acknowledgment frame length; overlay1-byte encoding is not original emitter evidence.

## 0x7ad1 — Unresolved item action5 request

Direction: C2S. Mapped: no. Layout: complete; meaning: unknown.

estateID:u32,itemID:u32,zero[140] exactly148 bytes

Remaining limits: Action5 operation unknown;140 zero bytes have no proven subdivisions; No receive counterpart or resulting effect established

## 0x7bb1 — OP_RealEstateAnchorConfirm

Direction: C2S/S2C. Mapped: yes. Layout: complete; meaning: partial.

S2C NULcaster + fiveu32 + LP32 destination text, total25+casterBytes+destinationBytes. C2S acceptance exact84: caster[64] plus same fiveDWORDs. Decline local only in documented callback.

Remaining limits: Three offerDWORD meanings remain opaque; only word1 estate lookup and word4 personalFFFFFFFF/guild discriminator are anchored; initialcast/success route unknown.

## 0x7c1d — OP_GuildTrophySave

Direction: C2S. Mapped: yes. Layout: complete; meaning: partial.

C2S exact68: fourbenefitIDs@20..35, fouri32tiers@36..51; remaining36bytes opaque/stale. Close and notification save variants use same frame.

Remaining limits: Fields outside selectedIDs/tiers remain opaque; no semantics assigned to leading20 or trailing16 bytes.

## 0x7d28 — Invalidate estate item-list cache

Direction: S2C. Mapped: no. Layout: partial; meaning: complete.

estateID:u32@0

Remaining limits: Only estateID DWORD consumed; full original emitted extent unknown

## 0x7db5 — OP_GuildTrophyAvailable

Direction: S2C. Mapped: yes. Layout: complete; meaning: partial.

S2C documented count-delimited grammar8+4*count: DWORD0 skipped by original consumer, countDWORD4, benefitIDu32[count]@8. Overlay supplies guildID atDWORD0.

Remaining limits: DWORD0 guild/reference role comes from overlay emission; located original consumer skips it rather than interpreting its meaning.

## 0x7dd2 — OP_GuildTrophyActive

Direction: S2C. Mapped: yes. Layout: partial; meaning: partial.

S2C minimum52 bytes: unknown leading DWORD0; shared guild favor pool DWORD@4; millisecond remaining timer DWORD@8; enabled BYTE12; unobserved13..19; four IDs@20 and four tiers@36. Full original extent unknown.

Remaining limits: Consumer-covered52 bytes do not establish full original7dd2 envelope/suffix.; Leading DWORD0 and unobserved bytes13..19 retain unknown domains. Native enabled access is byte12 only. Timer units and local countdown are recovered.
