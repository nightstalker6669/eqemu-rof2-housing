# Housing returns through native RoF2 parcels

These are existing RoF2 parcel mappings reused by housing, not newly discovered housing opcode numbers.

| Message | Existing mapping / discriminator | Housing use |
| --- | --- | --- |
| List item | `OP_ItemPacket`, subtype `ItemPacketParcel=0x73` | Exact serialized visible item plus parcel sender/time/note |
| Retrieve | `OP_ShopRetrieveParcel=0x7013` | Authenticated typed claim |
| Delete visible row | `OP_ShopDeleteParcel=0x47f1` | Remove the successfully claimed row |
| Parcel icon | `OP_ShopParcelIcon=0x46f0` | Refresh mailbox indicator |

The common retrieval body is16 bytes, four little-endian uint32 values:

```cpp
struct ParcelRetrieve_Struct {
    uint32_t merchant_entity_id; // offset0
    uint32_t player_entity_id;   // offset4
    uint32_t parcel_slot_id;     // offset8
    uint32_t parcel_item_id;     // offset12, template ID
};
```

The visible merchant selector combines parcel slot in its low32 bits and template item ID in its high32 bits. Merchant serialization deliberately substitutes the parcel slot for the visible instance ID. It is not the durable crate ID or original inventory GUID. The server resolves that selector through its recipient's cached native parcel entry to the actual durable parcel ID, then revalidates it under lock.

The original native list reader consumes serialized ItemInstance data and parcel metadata. No housing-specific crate manifest is added to that list. The server retains typed crate location, origin and members in its existing custody graph plus a canonical return sidecar, then hydrates the actual inventory instance at claim.

For RoF2 the adapter now sends a zero-initialized16-byte retrieval acknowledgement. Matching-client analysis found a nullable body guard and a non-null branch that reads the optional field at bytes8–15; this is deterministic emulator framing, not a claim that the exact historical success/error semantics have been recovered. Other client versions retain the original acknowledgement path. Absolute historical parcel capacity remains unresolved; delivery uses the emulator's configured native `Parcel:ParcelMaxItems` bound.

See [retirement and returns](../docs/RETIREMENT-RETURNS.md) for transaction, recovery and acceptance boundaries. Raw client artifacts and private analysis are not included.
