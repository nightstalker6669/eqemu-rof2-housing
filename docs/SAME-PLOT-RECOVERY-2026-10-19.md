# Furnished crate recovery after repurchasing the same plot

Moving out and repurchasing a plot creates a new ownership estate while retaining the physical plot's native ID. A carried moving crate can therefore require a real ownership relocation without a change of native parent. The previous house-relocation ledger required different parent IDs, and the room-parent update required one changed row even when its target values were unchanged. Either assumption could refuse the furnished recovery.

The crate relocation writer now records source and destination ownership estate IDs alongside each house effect. Migration034 permits the same native parent only when those explicit ownership IDs differ; a same-parent row with missing or equal ownership IDs remains invalid. Existing cross-plot history retains NULL ownership columns and remains readable. The update verifies the final locked native room identity instead of rejecting a valid unchanged-row result.

Replay no longer skips house-effect checks merely because the physical plot is unchanged. When ownership or catalog identity differs, it checks the recorded house revision, crate, permanent interior, parent and exact available ownership IDs. Same-parent recovery always requires the new ownership evidence. Old cross-plot effects retain backward compatibility.

Current ownership, creator, custody, catalog, geometry and inventory-fence checks remain in the existing placement path. The change grants no authority from historical ownership. It does not allocate a replacement interior or rewrite room furniture. No new opcode or wire struct was added.

## Deployment

Stop housing writers and back up the existing relocation ledger. Apply `overlay/utils/sql/housing/034_crate_house_ownership_recovery.sql` once after026, then deploy matching rebuilt world and zone binaries together. The ALTER is not idempotent. New writers require the new columns; old history is retained. This migration contains no property seeds, pricing changes or lifecycle activation.

## Evidence and limits

World and zone built successfully and were deployed with migration034. Existing relocation history was preserved. Eight private connection-local checks against the production replay reader and real migrated SQL constraint passed, including stale source/destination ownership rejection, missing effects, invalid same-generation rows, changed house revision and legacy cross-plot compatibility. The synthetic scenes did not change active property records.

The native move-out, repurchase, loaded-crate placement, Unpack and normal-entry round trip is pending local client sign-in. This is an implemented and narrowly checked fix, not completed original-client acceptance. The earlier cross-plot native recovery and permanent interior conservation remain separate evidence. No private fixtures, checks, accounts, client files or logs are published.
