# Housing service NPCs and the guild portal

This addition implements purchased, placeable guild-hall assistants as real interactive server NPCs. Housing pets and service assistants remain separate classes. Placement previews do not open a merchant, bank or tribute service; the service is created only from a committed, currently placed contract in a supported neighbourhood guild hall.

**Representative native service acceptance passed in a Grand Guild Hall.** The original RoF2 client placed all six genuine assistant contracts and the teleport device, opened their native service interfaces and completed the transactions below. The character and properties were administratively provisioned; this establishes the recorded service flows, rather than every visitor role, hall variant or original Live appearance.

|Native workflow|Observed result|
|---|---|
|Neighbourhood furniture merchant|Purchased a reviewed chair from Erinn Hume through the normal merchant interface|
|Ordinary furniture with all services present|Picked up the chair and completed a new ordinary chair placement while all seven service actors remained present|
|Service pickup and runtime revocation|Placed one general-supply assistant, verified one actual merchant NPC, then used the native Items Inventory pickup; the contract returned to inventory, the placed row was removed and the NPC/model disappeared|
|Placed general-supply merchant|Bought a 20-item stack and sold all 20 back|
|Placed personal banker|Deposited and withdrew the same item through personal bank storage, then through shared bank storage|
|Placed guild banker|Deposited, promoted, changed permissions to Public and withdrew the item; the bank row was removed and one authoritative item remained on the cursor|
|Placed guild tribute master|A 971-platinum donation credited 9710 guild/member favor; an item donation added 1 favor and removed the donated item|
|Placed personal tribute master|A 1-platinum donation credited 1 favor; the saved value persisted after zoning|
|Placed teleport assistant|Purchased a North Karana crystal normally and handed it back; exactly one crystal was consumed and the current hall retained its one-day alignment|
|Placed teleportation device|No closed the confirmation and retained the character in the hall; re-entering and choosing Yes actually zoned to North Karana|
|Placed Standard hall, built-in geode|With a genuine placed assistant and no placed device, handing in an administratively supplied North Karana crystal aligned the hall; accepting the actual geode confirmation zoned to North Karana. This separate check does not claim a second native crystal purchase|

The representative Grand service flows, ordinary furniture coexistence, direct service pickup/revocation and separate Standard built-in geode path have completed native acceptance. This does not certify every visitor role, simultaneous editor combination, hall variant, destination or historical Live appearance. The generic suspension fix was included in the final pickup acceptance build; arbitrary older untagged NPC snapshots are not a recovered housing identity.

## Contracts and services

| Item ID | Item | Role | Item definition key |
|---:|---|---|---|
|66639|Friendly Guild General Supply Merchant|Normal general-supply merchant, including purchases and sales|`NGH_Merchant_General`|
|66640|Friendly Guild Teleportation Assistant|Portal-crystal merchant and crystal hand-in|`NGH_Teleport_Assistant`|
|66641|Friendly Guild Personal-Banker|Personal and shared banking|`NGH_Banker_Personal`|
|66642|Friendly Guild Guild-Banker|Owning guild's bank, with existing rank/item restrictions|`NGH_Banker_Guild`|
|66643|Friendly Guild Guild-Tribute Master|Owning guild's tribute service|`NGH_Tribute_Guild`|
|66644|Friendly Guild Personal-Tribute Master|Personal tribute service|`NGH_Tribute_Personal`|
|66646|Guild Teleportation Device|Placed portal destination/activation point|`NGH_Teleport_Device`|

The item keys identify server content; they are not renderer model names. Reviewed later collector records provide placement mask2, interactive flag1, fixed scale1 and opaque respawn value750. The six assistants have the signed unlimited per-template sentinel; the device has a per-estate maximum of1. They still consume the applicable total item budget. They are excluded from the ordinary housing-pet quota, matching the original client's interactive-versus-pet classifier. The respawn number is retained as opaque metadata; this addition does not invent its units or an NPC respawn scheduler.

Bindings and evidence classifications are published in [service_item_bindings.json](../../overlay/common/housing/service_item_bindings.json) and [the provenance guide](../provenance/housing-services/SOURCES.md).

## Real NPCs and housing identity

A preview uses the original housing actor path to display the proposed model and pose. After a successful placement commit, the server reconciles the complete current room scene and creates the corresponding actual NPC. This gives merchants, bankers and tribute masters the spawn IDs and classes required by the client's existing service windows and ordinary server handlers. A client-local pet actor alone cannot supply those services.

The normal RoF2 spawn packet already has housing GUID, estate ID and item ID fields. The implementation fills these fields for managed service NPCs and preserves ordinary unbound spawn behavior. It also activates the previously recovered `0x21ea` binding message as `OP_RealEstateBindSpawn`. After each viewer's item list is published, this message associates the real NPC with that viewer's current housing presentation GUID. The server's durable authority continues to use custody and native item/estate identities. A presentation GUID does not grant ownership or interaction permission.

`0x21ea` is a reused recovered number, not a newly discovered opcode. Its implemented canonical consumed profile is:

| Offset | Field |
|---:|---|
|0|Spawn ID, little-endian `u32`|
|4–19|16 canonical GUID characters|
|20|NUL terminator|
|21–23|Zero emitted by this implementation; original meaning unobserved|
|24|Estate ID, little-endian `u32`|
|28|Estate item ID, little-endian `u32`|

The original receiver requires at least32 accessible bytes. Its exclusive original emitted size is unresolved. The larger ordinary spawn packet uses a separate variable tail and does not acquire this message's three-byte gap.

## Authority, revocation and existing services

Managed services are bound to an actual supported guild hall, its current owning guild/plot, a non-stowed and non-packed contract, the current interior reservation, and the committed pose. Reconciliation does not adopt an item from a preview, a crate contents list, a stale native ID, or a database item name alone.

Each service use revalidates the source against current locked custody, item metadata, reviewed room profile, content fingerprint, native NPC frame and item cache. Current paid-access state and the hosting house's placement are checked. The viewer must have a current authenticated inventory-session reservation, actual instance membership, and the applicable entry/interaction permission. Guild bank and guild tribute additionally retain the owning-guild and ordinary rank/item restrictions. A read-only visitor does not remove services for everyone else; that visitor's service use is denied independently.

Pickup, storage, movement, packing, house removal, retirement, changed ownership, invalid source state or revoked access cannot leave a service authorized by its former pose or hall. The shared managed-NPC source is reconciled after committed room changes; individual requests also recheck current authority. Malformed or unsupported sources fail closed. Successful whole-item returns revoke the exact runtime actor immediately, independently of subsequent furniture-list serialization. Partial-stack returns do not revoke a remaining source. Generic zone suspension neither saves managed service actors as ordinary dynamic NPCs nor restores previously tagged service snapshots. Only current committed housing custody reconstructs those actors; ordinary authored spawn points remain separate.

The original basic guild hall's NPCs, merchant stock, scripts and portal remain in place. Its existing version 1 teleport assistant (`345004`), 22-item crystal stock, Porter (`345005`) and 24-hour hand-in/proximity quests were checked directly; this batch did not repeat a native basic-hall acceptance cycle. The new source and access guards apply to tagged housing NPCs. Shared financial safety fixes also cover ordinary banking and tribute operations. Neighbourhood assistants remain placed contracts, rather than unconditional default spawns.

## Appearance policy

Assistant models and sizes are explicit server equivalents of the existing basic guild hall's service NPCs: the general merchant and teleport assistant use its gnome appearances; the banking and tribute services use its human appearances. The new personal tribute template uses the corresponding human appearance with the correct personal-tribute class.

The device uses the supported original `I28` Teleportation Stand model: race706, gender2, with an explicit size6 policy. Its original compiled model registration, resource manifest and race data are identified in the provenance guide.

These are deliberate server appearance choices. The exact historical friendly-assistant faces, equipment and sizes have not been recovered. The item66646-to-I28 association is a reviewed reconstruction, not a captured original Live NPC record. No proprietary client assets are distributed.

## Portal operation

Purchase a supported portal crystal from the placed teleportation assistant, then hand that crystal back. The normal merchant system handles payment. The hand-in consumes exactly one supported crystal and records the destination for that particular current hall and guild. Unsupported crystals and rejected hand-ins return through the ordinary trade path; direct coin hand-ins do not align the portal.

Alignment lasts one day, matching the deployed basic hall quest policy. The placed device must remain current, and a valid placed assistant must be present. Approaching the active device offers the original-style confirmation. Acceptance rechecks the current hall, assistant/device source, destination revision, access, position and expiry. Cancel/No does not teleport; the response discriminator is checked as well as the pending destination identity. The popup alone is never teleport authority.

**Standard Guild Hall exception:** item 66618 uses zone345 and contains the original built-in crystal geode. When no current placed item 66646 exists, a valid placed assistant may operate that geode. Its activation point is derived from the existing basic hall's Porter source. The fallback creates no fabricated pad item, custody ID or automatic assistant. A placed device takes precedence. Other guild-hall styles require an actual placed device. The neighbourhood activation range is a documented server policy using a 10-unit three-dimensional radius; it is not claimed to reproduce the basic quest's two-dimensional proximity box exactly.

The current destination catalogue deliberately follows the deployed basic hall implementation:

| Crystal ID | Destination |
|---:|---|
|10092|Plane of Hate|
|10094|Plane of Sky|
|64191|Dragonscale Hills|
|76000|North Karana|
|76001|East Commonlands|
|76002|Lavastorm Mountains|
|76003|Toxxulia Forest|
|76004|Greater Faydark|
|76005|Dreadlands|
|76006|Iceclad Ocean|
|76007|Cobalt Scar|
|76009|Twilight Sea|
|76010|Stonebrunt Mountains|
|76011|Wall of Slaughter|
|76012|Barindu|
|76070|Undershore|
|88735|Arcstone|
|88736|Goru'kar Mesa|
|88738|Katta Castrum|
|88739|Plane of Time|
|88740|Brell's Rest|

This is 21 supported destinations, not the full modern Live portal catalogue. Guild-banner stone 88737 has an unimplemented placeholder in the basic quests, and 76008 has no supported destination there. Both are refused rather than consuming a crystal for an invented or placeholder route. The implementation does not claim the later expansion destinations absent from that baseline. See [guild_portal_service.h](../../overlay/common/housing/guild_portal_service.h).

## Acquisition and installation

[Brent Harnak's November 2011 entry](https://everquest.allakhazam.com/db/npc.html?id=38576) identifies the Guildhall Accessories merchant in Sunrise Hills and lists all six assistant contracts. The optional [service vendor migration](../../overlay/utils/sql/optional/2026_10_21_housing_service_vendor.sql) restores that six-item stock and the documented location, plus one explicitly labelled server stock addition for the device. It labels the missing original NPC appearance and heading as server equivalents. Existing item prices and normal merchant price calculations are retained.

The original [Collection of Standard Guild Services66653](https://items.sodeq.org/item.php?id=66653) description includes all six assistants, Guild Regeneration Pool66645 and Guild Teleportation Device66646. The complete guild-hall packages include that service collection. The exact original direct device vendor remains unresolved.

For ordinary acquisition without an administrator grant, the Brent migration adds the genuine device as a seventh stock item using its existing 1000-copper base price. **This seventh slot is explicit server stock policy, not a claim about Brent's historical inventory.** The service addition does not implement all package-opening/furnishing or regeneration-pool behavior. No automatic device or service contract is installed when a guild hall is placed.

Install the matching production overlay, integration patch and prerequisites from [INSTALL.md](../INSTALL.md), then:

1. Stop all world, zone and shared-item writers. Apply the existing housing NPC metadata/runtime schemas and capacity prerequisites first.
2. Apply guarded [service contract metadata/templates](../../overlay/utils/sql/optional/2026_10_21_housing_service_contracts.sql), [guild portal alignment storage](../../overlay/utils/sql/optional/2026_10_21_housing_guild_portal_alignment.sql), and the optional Brent stock migration. Conflicting existing rows or reserved IDs cause refusal; the migrations do not overwrite unrelated content.
3. Rebuild and deploy all affected common/world/zone components together. The canonical spawn structure and Mob binding fields change the server ABI; a zone-only rebuild against older shared components is insufficient.
4. Regenerate shared items using the same item-cache prefix loaded by every world and zone. The authentic interactive metadata, source hash and original height-unknown field must agree across SQL, cache and wire serialization.
5. Deploy the source RoF2 mapping containing `OP_RealEstateBindSpawn=0x21ea`, restart the affected processes, and use the matching original client/resources already required by the housing overlay.

The service-contract seed preserves the historical actor-height source as unknown 0. Explicit reviewed appearance sizes are applied by the runtime policy; unknown historical size is not silently presented as a recovered Live value.

## Neighbourhood entrance vendors

The standard PEQ Sunrise Hills content already provides the entrance-area merchants and banker. This batch retains those existing spawns instead of duplicating them. The source audit found one unconditional zone-version 0 spawn per NPC, normal merchant class 41, matching merchant lists, and no expansion or content-flag restriction. Banker Fraener (`712001`) uses normal banker class 40. A representative Erinn Hume furniture purchase passed in the original client. The source audit covers all thirteen merchant lists; each vendor and the entrance banker have not received a separate native transaction cycle.

|NPC ID|Merchant|Existing stock rows|Reviewed housing stock within that list|
|---|---|---:|---|
|712011|Varlad Rahnta — Furniture Crafting Supplies|29|Craft supplies remain ordinary goods; station interaction is not implemented here|
|712012|Erinn Hume — Wood Crafted Furniture|80|80 static furniture profiles|
|712013|Neliths Sylnis — Wood Hewn Furniture|80|80 static furniture profiles|
|712014|Elliere Freegaze — Wood Carved Furniture|80|80 static furniture profiles|
|712015|Darix D`Unnar — Stone Carved Furniture|80|80 static furniture profiles|
|712016|Lorreal Songsword — Stone Chiseled Furniture|80|80 static furniture profiles|
|712017|Darlenna Everhot — Miscellaneous Furniture|55|51 static furniture profiles|
|712018|Ragthar — Three Room Houses|72|72 supported house variants|
|712019|Indy — One Room Houses|48|48 supported house variants|
|712020|Garik Nogflop — The Master Collector|22|Ordinary stock retained; generic decorative NPC items require separate authenticated bindings|
|712021|Ardak McMahon — Barkeep|18|All 11 supported storage crates, items62586–62596|
|712022|Lorekeeper Arbel Neklene — Historian on Ancient Objects|36|33 static furniture profiles|
|712023|Gebsle Nogflop — House Pets|31|Stock retained; unbound pet templates are not silently enabled|

This supplies 484 reviewed static furniture entries, 120 house variants and 11 crate variants through existing merchants. These counts describe reviewed source support, not a separate native test of every item. The legacy `items.placeable` column is not the housing mask: the shared item cache reads the original `UNK224` mask and associated native metadata. Furniture with legacy placeable 0 can therefore have the correct native mask 142 and work normally.

Existing stock also includes four particle-effect boxes (`62719`, `62720`, `62731`, `62732`), three unreviewed tree profiles (`62671`, `62672`, `62674`), fifteen crafting stations and generic pet/decorative NPC items outside the reviewed placement profiles. They are not promoted by a guessed model or database flag. Two existing Gebsle stock rows reference absent item definitions and are skipped by normal merchant item lookup. Original unrelated merchant goods and prices remain intact. The entrance merchants require the corresponding standard PEQ content; the new Brent migration adds only the separately documented guild-service seller.


Existing neighbourhood instances can retain a persisted spawn-state snapshot from before the Brent migration. After installing the new spawn content, use the server's normal administrative repop or reviewed spawn-state refresh procedure for those instances; a process restart alone may restore the older snapshot. Newly created instances read the new seed. Do not delete housing estates, item custody or placement records to refresh ordinary NPC spawns.
