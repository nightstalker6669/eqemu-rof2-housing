# Property lifecycle integration

The existing RoF2 purchase, upkeep selection and Move Out messages are used. No new opcode is required by this batch.

Apply the existing upkeep cycles/full-property schemas, then the optional2026_10_18_housing_upkeep_rounding.sql migration. It adds empty review-policy tables and immutable inactivity receipts; it does not enroll existing properties or seed prices.

## Supported core

- Bounded fair dispatch prevents old deferred contracts or a backlog above100 from starving every other property.
- Exact daily copper conservation across24 hourly assessments is available through an explicit generation/native/catalogue-bound UTC rounding policy. Daily tariffs below24c and alternate currencies remain unsupported. Integral rates retain existing behavior.
- Paid-upkeep purchase requires the existing native purchase policy **and** an exact catalogue/revision/definitions-hash/regions-hash enrollment review. The existing authenticated acquisition transaction commits the property, currency result, selected upkeep, first due clock, rounding policy and funded-reactivation policy together. Recovery authority is bound to the acquired ownership generation, native plot and catalogue revision; existing properties receive no automatic policy. Mode1 schedules the first assessment one hour later. These timing/rounding and current-cycle recovery details are compatibility policy, not recovered original server arithmetic.
- Opt-in empty personal property inactivity retirement uses90 days since login. It requires current generation and personal-plot definition, genuine inactive-session/pending checks, empty custody/storage/vault/child state, zero escrow and InnoDB. It retains ownership history and creates a separate immutable server receipt. It never simulates a Client lease or detaches a newer owner.

## Opt-in offline inventory delivery

Apply `utils/sql/optional/2026_10_18_housing_offline_property_delivery.sql` after the existing lifecycle migration. Both the original per-estate inactivity review and the new per-estate delivery review must match the current personal ownership generation, native plot and catalogue revision. No policy rows are seeded.

Delivery mode1 is an explicit compatibility choice: return one existing, placed small moving crate (62586 / definition174) to a free ordinary saved-inventory slot23–30, and refund the entire unused escrow to carried copper without denomination conversion. A crate requires the exact reviewed content manifest. An empty funded personal plot can refund without a crate. No original purchase price is refunded. This is server-owned offline work with its own immutable receipts, not a fabricated Client lease or an acknowledged online item application.

The owner must be genuinely offline and inactive for90 days. Current account/character/ownership rows, real sessions, global pending item applications, coin applications and the complete supported cargo graph are checked under the same InnoDB transaction. An already packed furnished house retains its permanent interior, room scopes and stored objects. Exact raw crate state and full currency snapshots are retained in the delivery receipt; a bounded deterministic cargo manifest and SHA256 bind the unchanged contents. Legacy absence of a storage row means unstowed, as in the existing online path.

Saved inventory delivery, crate binding transition, escrow refund, ownership retirement and both receipts commit together.

### Optional automatic yard packing

Apply `utils/sql/optional/2026_10_18_housing_offline_property_packing.sql` after the delivery migration. A separate per-estate packing review must match the same native plot and catalogue revision. Its absence retains packed-crate-only delivery.

Mode1 packs supported loose yard custody into the existing small crate in the retirement transaction. Registered ordinary static items and personal houses are supported, including stowed yard items. A furnished house remains one contained house with its existing permanent room and room objects; this does not flatten room furniture into yard membership. The crate's actual reviewed capacity and the original at-most-one-nonempty-house rule apply to the complete batch. Other crates, guild hall templates, evolving items, unsupported models, foreign belongings, active interior visitors, vaults and incomplete custody coverage defer the whole retirement.

Only crate membership, storage disposition/revision and the container revision change during packing. Exact custody payloads, NULL/binary item metadata, creator, poses, reservations, house identities, room profiles and scopes must match before/after witnesses. Every member retains its origin pose and stowed flag. A separate immutable packing receipt stores the complete before/after graph, hashes and hex-encoded canonical crate allocation snapshots. These receipts commit with delivery, money and ownership; neither a halfway pack nor a late receipt failure can commit a partial return.

### Optional crate creation

Apply `utils/sql/optional/2026_10_18_housing_offline_crate_creation.sql` after the packing migration, followed by `2026_10_18_housing_crate_creation_origins.sql`. An additional exact per-estate creation review must name the same native plot, catalogue revision and crate content manifest as the delivery review. All activation tables start empty. This compatibility policy creates one small crate (62586 / definition174); it does not establish Live's historical container-size selection.

When a supported occupied personal plot lacks a placed crate, the transaction can allocate a new permanent typed crate and inactive native container identity directly in a safe saved-inventory slot. It creates no placed custody, pose, placement reservation, Client lease or online application acknowledgement. Its initial inventory GUID is0, following normal new-item loading; the normal checked inventory loader assigns a real runtime serial at login. The crate is then packed and validated with the same complete-batch checks as an existing crate. No free crate is granted for an empty-property refund.

A separate immutable creation receipt binds the exact initial and delivered canonical allocations and the newly created raw inventory row. Delivery `source_kind=2` records a generated crate with no prior custody; legacy pickup/refund rows remain `source_kind=1`. All creation, packing, money, ownership and receipts share one transaction. A pending or malformed house inventory application blocks offline inventory/wallet delivery as well as ordinary live-item and coin barriers. Lack of a safe slot (including orphan bag children or typed bindings), signed coin overflow, mismatched reviews, foreign belongings, vaults, loose/unaccounted objects or occupied house interiors defer the whole operation. An uncertain commit never authorizes a second delivery: the retired generation and immutable receipt are retained, while replacement ownership is independently checked. Current bindings and saved inventory are structurally revalidated before commit.

## Explicit limits

Unsupported loose contents and additional container/capacity handling remain unimplemented. Guild inactivity/returns, foreign-owner delivery routing and vault return also remain deferred. Offline delivery is currently limited to empty funded plots or a supported existing small crate on a personal plot; loose yard packing requires its additional explicit review. Housing returns are not connected to the existing parcel service. More than one missed billing interval remains deferred because current furnishings cannot reconstruct historical obligations. Partial affordability, alternate currencies, sub-copper hourly cohorts and exact original reactivation semantics remain separate work. New lifecycle and offline-delivery native client acceptance is outstanding; passing C++ and database checks is not evidence of that acceptance.

No tariff, historical guild inactivity threshold, purchase-price refund, original wallet selection or global policy seed is supplied. The [official launch-era guide](https://www.everquest.com/news/imported-eq-enus-52047) supplies the hourly/daily and90-day outline; it does not establish all May2013 server arithmetic. Review policies must identify any compatibility choice.


Crate provenance is stored in `housing_crate_creation_origins`: kind1 links the genuine live-item creation receipt; kind2 links the offline retirement generation. The crate instance retains its existing creation key, now referencing this typed provenance table. Existing crates are backfilled through their actual client receipts. Both online creation paths insert kind1 in their existing transaction; offline creation inserts kind2. Structural crate readers validate the immutable creator against its historical origin without granting current property access. No synthetic client receipt satisfies an offline foreign key.


## Reconstructed plot defaults and paid guild acquisition

See `plot_pricing_defaults.md` and `plot_pricing_defaults.h` for the explicit address-based copper profile. This is a later-source reconstruction, not an authenticated2013 tariff seed or a boundary survey. Existing catalogue prices and active ownership contracts are not automatically replaced.

After the native guild purchase schema, `2026_10_18_housing_paid_guild_plot_purchase.sql` adds an unseeded `allow_guild_carried_copper` switch. The paid path requires both that switch and the existing carried-funding authority, exact published-catalogue hashes, the actual current guild leader, action27, expansion authority, reviewed member minimum and plot cap. Only the leader's carried coin is debited through the existing inventory/profile application journal. No guild bank funding is inferred. New paid guild offer3 retains the original byte contracts of personal1 and free guild2; it adds no native client opcode.

Representative actual SQL verifies the paid guild debit, unchanged bank/cursor/crystal balances, receipt replay/application acknowledgement, legacy modes, permission loss, rollback and certified-writer-death recovery. A separate guild plot billing graph verifies24 hourly assessments sum to the configured2000 copper daily rate, insolvency expiry, reviewed funded recovery and late-debit rollback. Native paid-guild acceptance and partial/alternate-currency/overdue-history behavior remain outstanding.

Native paid-personal acceptance now covers a42-platinum purchase with4-platinum2-gold daily upkeep, purchase-time contract enrollment, a5-platinum cursor deposit, an actual world-timer175-copper charge and individual withdrawals of the remaining4825 copper. The test advanced only that property's due clock to avoid waiting an hour; the world performed the debit. Add Day, Add Week and Withdraw All remain unsupported. The management window's daily-cost label truncates the displayed denomination and its open escrow balance remained stale after the world charge until a financial refresh; those presentation issues are not resolved by this acceptance. New-purchase recovery enrollment also passed actual SQL expiry, funding and final-insertion rollback checks. This does not establish native acceptance of every lifecycle branch or complete neighborhood plot content.


## Witnessed overdue assessments and guild disposition

See `upkeep_catchup.md` for the default-off, prior-observation catch-up extension. More than one missed interval is supported only with a genuine earlier on-time checkpoint and unchanged validated funding/content history; unknown arrears still defer. See `guild_inactivity_returns.md` for separate30-day, current-roster, explicit-recipient guild returns and the dissolution interlock. These extensions supersede the corresponding earlier limitations above only within their reviewed supported cases. Unpaid rent alone still locks/stows; it does not trigger a new invented eviction rule.


## Mixed recipients, vaults and multiple moving crates

See `offline_return_batches.md` for generation-bound all-recipient returns and `return_settlement_db.h` for the immutable conservation seal used by guild dissolution. Supported foreign room/vault items now return to their original owners, exterior cargo is partitioned into owner-specific capacity-limited crates, and a verified settled generation can disband without orphaning retained cargo. The new policy is default-off and does not retroactively certify earlier occupied returns. These changes supersede the corresponding earlier single-recipient limits only within the documented supported shapes.
