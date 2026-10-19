# Property retirement and returns

Supported mixed-owner retirement returns personal or guild-property belongings
to their original owners and refunds unused copper escrow to the reviewed
property recipient. The original purchase price is retained. Furniture can
span several generated moving crates within reviewed limits. Supported vault
items return directly to saved inventory. Existing empty moving crates return
with their original identity. Plot ownership is released only after the complete
item and financial disposition has been sealed in the same transaction.

An existing exterior crate may now contain typed nonplaceable items. The source
capture traverses both physical and nonplaceable memberships; both are validated
before detachment. The complete return preserves stack quantities and canonical
metadata, without inventing placement records for nonplaceables. A direct
return cannot consume custody while typed membership remains attached.

The settlement verifier now accepts an explicit personal or guild owner kind
and identity. Guild dissolution continues to use its restricted guild entry
point. Existing receipt and manifest formats are retained. No opcode, packet
structure, mapping or additional schema was introduced by this follow-up;
typed nonplaceable members require the existing migration036.

The follow-up has isolated service/database coverage for personal and guild
retirement with an existing typed crate, nonplaceable stack, two contributors,
multiple generated furniture bins, exact refund and plot release. Full-recipient
inventory, a changed membership witness and a late settlement-write failure
preserved the whole property. Guild settlement verification and guarded
dissolution preserved returned items and an unrelated guild. These checks do
not establish a fresh native retirement playthrough or complete Live equivalence.

Retirement reviews remain opt-in and bound to the current ownership generation
and catalogue. This publication contains no activation seeds. Interior crate
roots, unsupported augmented/evolving/NPC custody and parcel overflow remain
outside this supported path. Insufficient recipient inventory defers the entire
property; no item is discarded and ownership is not released early.

See [batch configuration and requirements](../overlay/common/housing/offline_return_batches.md),
[earlier mixed-return native evidence](MIXED-RETURNS-2026-10-18.md), and
[vault-aware Move Out](VAULT-MOVE-OUT-2026-10-19.md).

Matching world, zone, UCS and inventory-recovery binaries were deployed together. Loaded process hashes and original ownership, wallet, item, house, crate and catalogue conservation were checked. Existing retirement reviews were not activated or changed.
