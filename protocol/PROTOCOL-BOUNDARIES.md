# Remaining housing protocol boundaries

The current implementation uses 55 mapped values from a catalogue of 67 documented housing-related opcode numbers. These counts describe the reviewed reference; they do not prove that every original message exists in it. The service implementation activates the already recovered0x21ea binding, without adding a newly recovered opcode number. Its exclusive original emitted length remains unresolved.

| Message | Recovered evidence | Current path and remaining boundary |
|---|---|---|
| `0x76c6` | General owned-estate add/remove; estate ID at offset0 and add flag at104. | Full `0x2aa7` replacements supply this list. Original total envelope and unconsumed bytes remain unknown;105 is a minimum accessed length. |
| `0x0c13` | Separate item-owned-estate add/remove array; same accessed offsets. | My Items uses authoritative mode5 search. The separate `0x3e00` full-array replacement is mapped but currently has no zone producer. General-owned IDs must not be copied into this distinct array. Original delta envelope remains unknown. |
| `0x3968` | Access snapshot plus trailing DWORD; invalidates an access-cache entry. | Full `0x485d` snapshots and current-authority refresh serve ACL editing. The trailing word's exact domain remains unknown. |
| `0x3c8b` | Complete352-byte text sender through TextEntryWnd and ItemWnd. | Its initiating housing prompt and server operation are unknown. A text field does not establish rename, mail or another effect. |
| `0x51e3` | Complete12-byte custom-interaction request for menu action9. | Five observed pet controls are implemented for the admitted server profile; other parameters and original item eligibility remain unresolved. |
| `0x64a3` | Complete12-byte action19 request. | Operation and authentic menu binding remain unknown. |
| `0x7ad1` | Complete148-byte action5 request. | Operation and authentic menu binding remain unknown. |

The original client consumes server-supplied `0x21ec` menu parameters and text IDs; outgoing request fields depend on the selected action. The executable does not contain the original server's choice of those menu records or their effects. Further static repetition cannot recover data absent from that executable. Matching-build original menu records or attributed captures are needed before these requests can be given safe gameplay handlers. Current Live observations may inform behavior, but do not independently establish May2013 wire layouts.

The Standard guild-hall model door uses the existing normal door-click message and the verified housing exit service. It does not require inventing a custom interaction, new opcode, or new wire struct. See [guild routing](../docs/history/GUILD-ACCESS-ROUTING-2026-10-19.md).

### Housing pet custom interaction

`0x51e3` has a recovered 12-byte estate/item/parameter body. Parameters16175..16179 are observed Live pet commands with matching original labels. The server implements this bounded set for its admitted pet profile; it does not infer all custom-interaction parameters. Requests `0x7ad1` (action5) and `0x64a3` (action19) remain semantically unresolved and are not enabled by this change. See [pet controls](../docs/HOUSING-PETS.md).
