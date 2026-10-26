# Housing pet controls

Upright, unit-scale pets with an admitted, reviewed model can offer **Stay, Walk Around, Run Around, Follow Me, and Play Dead** in their native item menu. The initial supported server profile is **Holgresh House Pet (77408)**. NPC services and decorative plushies do not inherit these commands.

The commands use the recovered original RoF2 custom-interaction request, `0x51e3`. Its body is three little-endian 32-bit words:

| Offset | Field |
|---:|---|
| 0 | Native estate ID |
| 4 | Native estate-item ID |
| 8 | Server-supplied menu parameter |

The decoder and structure are in [`pet_commands.h`](../overlay/common/housing/pet_commands.h). Parameters 16175 through 16179 select the five commands in the order above. Those parameters were observed in a current Live owned-spider menu; their labels also exist in the original RoF2 client. They are a reviewed command set offered by this emulator, not proof that the historical Holgresh menu was identical. Other custom interactions are not decoded as pet commands.

## Behavior and access

The server creates a noncombat actor bound to the placed item's identity and publishes its binding to each viewer. The item stays in housing custody; walking does not repeatedly rewrite its saved placement. Commands require a current inventory session, fresh item/custody/catalogue and rent checks, Interact Item permission, and a distance of at most 20 units from the pet's current position.

Walk and Run select nearby destinations. Follow Me follows the requesting character within the property and stops when that character leaves, disconnects or loses access. Stay stops movement. Play Dead changes the pet's appearance without killing it or creating loot. Another command stands it up again.

Movement uses the existing collision map, rejects steep or obstructed steps, and stays inside the property's current regions. Behavior is transient: a zone restart restores the saved placement in Stay mode. The current walk/run rates are emulator behavior settings in the runtime, not recovered historical rates. Tilted or scaled placements retain their existing static presentation rather than losing their saved transform.

Storage, pickup or changed placement removes/replaces the runtime actor when the scene reconciles. Active actors also recheck durable authority every five seconds. Commands recheck it immediately. There is no schema migration or new NPC database record.

## Model boundary

Recovering a command does not recover an item's model, size, texture or placement bounds. The Live Itty Bitty Spider used to identify the command set remains **unseeded**: its original TAR appearance/size and native placement admission need further recovery. This update does not bypass those checks or enable every housing pet automatically.
