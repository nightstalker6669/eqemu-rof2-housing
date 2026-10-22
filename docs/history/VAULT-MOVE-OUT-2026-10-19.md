# Vault-aware moving crates and occupied Move Out

October19,2026. This closes the supported personal-property occupied Move Out rejection when direct nonplaceable vault items are present. It does not establish complete Live lifecycle equivalence.

## Implementation

Migration036 adds `housing_crate_vault_members`, separate from physical furniture membership. Each member binds its custody, crate, membership/vault revisions, original realm and optional house, exact serialized inventory source, catalog bytes and schema descriptor. Nonplaceables never receive invented poses, placed rows or world actors. Ordinary vault lists hide packed members; the native moving-crate list displays them through the existing list-only representation.

Bulk Pack and Unpack include both physical and vault candidates under the existing transaction and inventory-session fence. Total and nonplaceable limits are checked together. Unsupported, changed or pending source state refuses the operation. Occupied retirement packs direct yard vaults into the returned crate; vault contents belonging to a packed furnished house retain their original room scope. Foreign room creators with pending item work prevent retirement. Relocation revalidates source witnesses, allocates actual destination native item IDs and updates current membership markers without rewriting source bytes or creator identity.

Bulk Unpack returns eligible physical items to Shed/Closet and vault items to current vault storage. Restore the house with Place or Load Yard afterward. Existing legacy physical-member receipt bindings retain their format; typed vault operations use a separate internal binding discriminator. No client opcode, client packet struct, client asset or wire mapping changed.

## Verification

The current world and zone build passed. A matching recovery CLI was rebuilt against the same common library and deployed with migration036. Private checks linked to the production common library passed32 current-property and35 overdue-property assertions, covering mixed packing, source conservation, real reservation-backed relocation, late-failure rollback, pending-outcome reconciliation and foreign room-creator safeguards. These are private service/database checks, not native overdue-retirement evidence.

One representative original RoF2 client cycle passed on a personal plot: Move Out returned the loaded crate to inventory; placing that retained crate immediately displayed the packed house, yard chair and17 Bone Chips; native bulk Unpack returned the stack to yard vault storage and the house/chair to Shed. Ordinary Inventory retrieval returned all17 Bone Chips. Exact persisted source, schema, catalog and creator bytes for the house, yard chair, nested room chair and vault stack were conserved through relocation/unpacking. The logical crate identity remained stable; returning and replacing it created a new placement custody, as expected.

Native Load Yard restored the house and its normal Enter action reached the original permanent room. The original nested chair remained in that room’s storage.

The old experimental plot has no purchase scheme. A new ownership generation was provisioned administratively using the production estate service, then bound to the retired physical plot. This is not native repurchase/debit/economics evidence. The first Move Out attempt from outside the plot was refused without mutation; the in-plot confirmed request succeeded.

## Remaining limits

- Follow-up: [individual vault-item crate controls](INDIVIDUAL-VAULT-CRATE-2026-10-19.md) now support representative native whole/partial Inventory and Shed/Crate transfers from an owned placed personal-yard crate. Bulk Unpack remains available. Carried-crate editing and the wider permission/interior matrix remain separate.
- Automatic generated-crate vault returns, guild/foreign-owner return combinations and the full interior-container matrix are not newly client-accepted here.
- Overflow across multiple crates, historical eviction/refund rules, unsupported custody classes and original catalogue capacities remain separate gaps.
- The additive membership schema does not activate any retirement policy. Do not remove it while members exist. Rebuild and deploy matching world, zone and recovery binaries together; internal C++ scene/request structures changed.

No test suites, fixtures, helpers, logs, screenshots, accounts, private data, binaries or game assets are included in this publication.
