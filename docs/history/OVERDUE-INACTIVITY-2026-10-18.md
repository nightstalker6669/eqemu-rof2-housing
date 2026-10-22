# Witnessed overdue upkeep and guild inactivity

This batch adds default-off, evidence-bound upkeep catch-up and reviewed guild inactivity returns. Production integration is included. The real world-timer empty-guild release/refund passed on the original development server, and the original client displayed the refunded carried wallet at login. Private furnished-return SQL and normal inventory reload verification remain distinct evidence levels.

## Overdue hourly upkeep

A reviewed policy permits one historical hour per existing full billing transaction only when a genuine earlier on-time assessment created a prospective checkpoint. That checkpoint binds ownership generation, catalogue/cost/region hashes, escrow and append-only transfer history, placement history and current typed obligations. No baseline is invented for already-overdue state. Changed or unknown history, unsupported currencies and partially affordable cohorts defer. A fixed reviewed horizon cannot extend itself while catching up. Receipts preserve replay safety; checkpoint failure rolls back the hour's debit and clock advancement.

Future paid acquisitions may explicitly opt into a bounded catch-up policy through the existing purchase/enrollment transaction. The option defaults to zero, including older schemas, and creates no past assessment or checkpoint. Existing properties remain unchanged. Unpaid rent alone does not evict the owner: lockout/storage consequences remain separate from reviewed inactivity disposition.

## Guild inactivity and dissolution

The native RoF2 text describes release after thirty days without a guild member login. The implementation checks the current real roster, latest member activity, current leader recipient, ownership generation and exact reviewed content. It does not reconstruct departed-member history or invent original Live guild-wide distribution rules.

With separate explicit reviews, supported returns can preserve one single-creator furnished registered guild hall, its permanent room and ordinary room/yard objects in one reviewed small crate. The recipient must remain the current leader and actual member. Unused escrow is refunded; the original purchase price is retained. Mixed contributors, vault/evolving/unsupported items, active member sessions or interior visitors, pending applications and insufficient capacity defer the whole transaction. No partial return or guild-bank debit is inferred.

The guild deletion interlock shares the locked guild identity with acquisition/disposition. It checks housing/currency/custody dependencies and publishes local cache removal only after a successful checked database commit. Historical packed and delivered occupied cargo still conservatively blocks disbanding until a separate settlement proof exists; the implementation does not discard it to permit deletion.

## Evidence and limitations

Private SQL validation covered transaction rollback and concurrency. A supported registered guild hall with three ordinary room objects and one yard chair was preserved in a generated crate and survived normal inventory reload. That is not native placement/unpack acceptance for every returned item or hall variant. The actual world timer retired an empty reviewed guild property without a direct SQL retirement call. Database ownership was released, escrow fell from 1834 to zero copper and the recipient wallet increased from 17 to 1851 copper. The original client inventory displayed 1851 copper after login; the controlled inactivity age had been restored before login. This establishes the timer release/refund and native wallet presentation, not a fresh native plot purchase-offer check.

The separately verified furnished return used an actual registered Standard Guild Hall with three ordinary room objects and one yard chair. Private SQL generated a typed guild-origin crate; normal InventoryRepository GUID handoff and CrateBinding reload retained the permanent room. Native placement/unpacking of that guild crate was not exercised.

Cleanup completed after the character camped and all active/pending inventory writers cleared. The temporary 1834-copper refund was removed, restoring the exact original carried wallet and all sixteen original currency counters. Only the new activation reviews were removed; inactivity/delivery policy counts are zero, while immutable retirement/history receipts remain. Original estates, custody, poses, inventory, full house/crate graph and guild identity match their recorded pre-batch state. The controlled inactivity age was cleared before login; normal current login history remains. The seed ledger reconciles exactly: 21000 purchase + 166 rent + 177000 previously removed + 1834 removed at cleanup = 200000 copper, with zero held escrow. No new client opcode or packet layout is claimed. No tests, fixtures, logs, private property identities, captures, credentials or game assets are included.

Module details: [witnessed catch-up](../../overlay/common/housing/upkeep_catchup.md), [guild inactivity](../../overlay/common/housing/guild_inactivity_returns.md), and [lifecycle](../../overlay/common/housing/property_lifecycle.md). Optional migrations create empty review tables and do not enroll existing estates.
