# Housing opcodes and packet structures

**You can use the protocol documentation without installing the housing server.** It targets the original **May 10, 2013 RoF2** client; numbers from another client build are not interchangeable.

| What you need | Open |
| --- | --- |
| Opcode numbers, names and directions | [Opcode inventory](OPCODES.md) |
| Packet sizes, field offsets, framing and variable data | [Packet structure reference](STRUCTS.md) |
| Machine-readable opcode records | [opcodes.json](opcodes.json) |
| Machine-readable field dictionary | [housing-wire-reference.json](housing-wire-reference.json) |
| C++ packet definitions and codecs | [Housing source directory](../overlay/common/housing/) |
| Known unknowns and original-client evidence limits | [Protocol boundaries](PROTOCOL-BOUNDARIES.md) |

The inventory contains **67 documented opcode numbers** and **55 active mappings**: 49 real-estate messages and six guild-trophy messages. Some documented messages are analysis-only or deliberately excluded from active mappings. A recovered number does not mean its entire gameplay feature is implemented or every field is understood.

## How to read a packet entry

- **C2S** means the client sends the message to the server; **S2C** means the server sends it to the client. Some opcodes use both directions.
- A field offset is its byte position within the packet body. Fixed headers may be followed by variable-length strings, lists or item data; do not assume a fixed C++ structure is the complete message.
- The structure reference distinguishes observed values, interpreted fields and opaque data. Preserve unknown fields and check the matching codec before changing their meaning.

For a starting example, compare `OP_RealEstateItemOperation` in the opcode inventory with its source44-plus-action layout in the structure reference and the matching code in `placement_wire.h`. Nearby definitions include `estate_wire.h`, `item_list_wire.h`, `interaction_wire.h`, `layout_wire.h` and `guild_trophy_wire.h`.

Applying opcode mappings alone does not add housing gameplay. Server operators should use the [complete installation guide](../docs/INSTALL.md).

The [October 22 gameplay fixes](../docs/UPDATES.md) reuse the existing opcodes and structures: pedestal clicks resolve the selected plot through the normal inspection/purchase flow; the Palatial front door uses the checked current-house exit. No new opcode number is claimed.
