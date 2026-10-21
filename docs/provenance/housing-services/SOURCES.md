# Guild-hall service content and protocol provenance

The implementation separates recovered client behavior, item metadata, existing server content and explicit server reconstruction.

| Claim | Source and scope |
|---|---|
|Six assistant contract identities and original Sunrise Hills merchant|[Brent Harnak, ZAM NPC38576](https://everquest.allakhazam.com/db/npc.html?id=38576). November2011 screenshot entry and purchase comments identify the stock. Public vendor details have later edits; individual original NPC appearance fields are not recovered.|
|Teleport assistant requires a device, with the Standard hall geode exception|The actual item description reproduced on [Friendly Guild Teleportation Assistant, ZAM item98649](https://everquest.allakhazam.com/db/item.html?item=98649). This is an item-description witness, not a matching-build captured packet.|
|Device requires an assistant|Item description reproduced on [Guild Teleportation Device, ZAM item98692](https://everquest.allakhazam.com/db/item.html?item=98692), corroborated by [Fanra's guild-hall guide](https://everquest.fanra.info/wiki/Guild_halls).|
|Authentic device package association|[Collection of Standard Guild Services66653](https://items.sodeq.org/item.php?id=66653) explicitly includes the device and six assistants. The complete hall packages include that collection. The optional direct Brent device stock is labelled server policy; an original direct vendor is not identified.|
|Exact item keys, interactive flag, opaque interval, mask and scales|Per-item later collector records linked and hashed in [the binding JSON](../../../overlay/common/housing/service_item_bindings.json). These records are explicitly later than the supported client.|
|Interactive services are not ordinary housing pets|Original May10,2013 executable function7b1640 tests a nonempty item NPC filename and requires InteractiveObject false.|
|Real NPC service windows use actual spawn identity/class|Original RightClickedOnPlayer536d80 and its merchant/bank/tribute dispatch paths. This does not assign meanings to unrelated custom housing menu parameters.|
|Spawn-to-housing binding|Original21ea receiver5b26fa resolves a spawn and installs housing GUID/estate/item identity. The documented32-byte canonical consumed profile does not establish an exclusive original emitted size.|
|Teleport stand model exists in the original client|Compiled constructor50ec52..50ec64 maps race706/gender2 toI28; original resource manifest namesI28_ACTORDEF in i28.eqg; original racedata gives size6.|
|Assistant appearance choices|Existing basic guild hall service NPC templates, reused as labelled server equivalents. Exact historical friendly-NPC face/equipment/size is unknown.|
|Device item-to-I28 association|Reviewed server reconstruction using the original Teleportation Stand actor. It is not a recovered original Live NPC row.|
|Twenty-one destinations and one-day alignment|Existing deployed basic guild hall quests, reflected in the published destination table. The guild-banner placeholder and unsupported76008 route remain refused.|
|Standard built-in activation point|Existing basic hall Porter source76577/NPC345005. The neighbourhood fallback uses an explicit10-unit3D server activation policy, not a claim of identical basic quest proximity geometry.|

Original executable SHA256:
`2a8702ad9f722704f01355c0750be7d6f164a8b9c9128ba0cf286ea32b405b0e`.

Original i28.eqg SHA256:
`0eb0ee1086d9cf2da35ba61857371ae022126f6a281d5ddde16f497758a40a99`.

Raw client resources, extracted models, captures and investigation tooling are not distributed. The production binding JSON contains source URLs and digests needed to distinguish authentic item metadata from the deliberately labelled runtime appearance choices.
