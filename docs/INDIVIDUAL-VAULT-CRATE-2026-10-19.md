# Individual vault items in placed moving crates

October19,2026. Typed nonplaceable crate members now support individual Inventory retrieval and Crate/Shed/Closet transfers, including strict partial-stack quantities. Bulk Unpack is no longer the only available retrieval path.

## Implementation

The native client already supplies the existing OperationSource44 quantity field. `0x3424` selects destination2 Closet,3 Shed or6 Crate; `0x2036` selects Inventory retrieval. This change adds no opcode, client packet layout or client-file change.

The server resolves a displayed crate estate to its actual current parent yard/room. The internal vault context binds the placed crate identity/revision and, for an existing member, its membership revision. Fresh scene authority checks require a placed, unpacked crate created by the acting character. Existing owner, item identity, source route, catalogue, room and inventory-session checks remain in force. Carried crates do not expose this path.

Inventory Store/Return, typed membership, container revision, vault marker, custody source and operation receipt mutate in one existing transaction. Strict partial returns retain the source member and create the separately identified inventory piece; partial storage transfers retain the source and allocate a real new list-only marker for the destination piece. Total and nonplaceable limits are checked against the selected container rather than its parent. Nonplaceables never acquire dummy poses or physical actors.

Legacy request/receipt bytes remain unchanged when no typed crate context exists. New internal binding extensions and a distinct partition-context discriminator preserve crate witnesses through Resolve, pending-outcome validation and recovery. Native packet structs are unchanged. Apply already-required migration036; no additional migration is introduced. Deploy coherent world, zone and recovery binaries rebuilt against the same common library.

## Verification

The world/zone build passed and a recovery CLI was linked against the matching production common archive. The focused private run passed23 assertions, including stale crate/member refusal, idempotent whole/partial retries, pending inventory exclusion, restart-outcome validation, direct partial inventory storage and a deliberately failed late container revision write that rolled back inventory, marker and membership changes together. The previous mixed-house/vault recovery fixture passed34 assertions in current-property mode against the new archive. These are private service/database checks.

The original RoF2 client completed the cycle on an existing personal plot and placed crate. Whole storage put17 Bone Chips into the crate. Direct Inventory retrieval selected2, leaving15 packed. Reverse Inventory-to-Crate storage selected1 of the returned2, leaving1 in inventory and16 packed. An individual Shed operation selected1 from the remaining15-stack; the resulting yard-vault piece then moved back through Crate. Finally, individual Inventory retrieval returned the remaining three packed stacks. All17 items finished in inventory; the crate was empty, no operation remained pending, and bulk Unpack was never used.

Each observed intermediate inventory/yard/crate total remained17. The pre-existing house, permanent interior, nested scope, furniture source/catalog/pose/storage records and wallet matched their baseline exactly. This is native personal-yard acceptance, not proof of every room/guild/delegated configuration.

## Limits

The supported path requires an active placed crate, current authorized yard/room access and creator-owned vault payloads. It does not establish delegated/foreign-owner retrieval, every guild/interior configuration, carried-crate item editing or original Live policy equivalence. Unsupported payload classes, original catalogue capacities, automatic lifecycle variants and multi-crate overflow remain separate gaps. Same-container transfers are refused without mutation.

No suites, fixtures, helpers, logs, screenshots, private data, binaries or game assets are included in this publication.
