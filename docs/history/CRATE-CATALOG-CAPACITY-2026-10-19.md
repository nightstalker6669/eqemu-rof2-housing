# Personal catalog crate support and placed-child capacity

The older experimental personal house-choice catalog omitted moving-crate definition174 and its allowed plot-child reference. Its shared child budget was also one. That prevented a crate from being placed alongside a house. Separately, the crate allocator counted permanent interior nodes retained by stored houses as placed children. A property with an active house and an archived house could therefore reject its first crate.

The allocator now excludes only exact managed house nodes whose inventory, stored or packed location proves they are off the yard. Unknown/orphan nodes continue to count. Existing authentication, geometry, custody and content checks remain. The independent one-placed-house and property-wide one-placed-crate checks, including interior crates, remain enforced. Raising a shared plot child budget does not authorize another house or crate.

## Optional administrative catalog transition

`overlay/common/housing/catalog_crate_upgrade_db.h` provides a deliberately narrow, one-shot administrative operation for existing experimental personal catalogs. It adds a supplied reviewed type7 crate174 definition and its plot-child relationship, sets selected one-child personal definitions to a shared two-child budget and advances the visible catalog revision once. It is not a client handler or an automatic content repair.

Apply optional schema035 to install an empty transition journal. Stop housing writers and retain a database backup. On an explicit administrator-owned repeatable-read transaction, supply the exact previous catalog payload and full active native graph, reviewed original crate definition, original house routes and verified room profiles, selected plot definitions and an audited plan fingerprint. Use `ApplyUnderTransaction(..., false)` to review the operation before applying it in the same maintenance conditions. Commit only a successful write result; roll back a refusal.

The operation refuses active sessions, pending work, old previews, existing crates in the target catalog, enrolled upkeep requiring a separate review, changed native identities, wrong content witnesses and stale revisions. It verifies existing exterior and room content before advancing current reservation/interior revisions and rebuilding affected exterior fingerprints. Exact purchase-policy fingerprints advance with the catalog. Raw custody, poses, permanent room/native identities and room profiles remain unchanged. Historical receipts are retained; they are not rewritten as current authorization. Repeating the transition is refused, rather than treating a historical journal entry as proof of present correctness.

The journal stores old/new definition payloads, region hash, plan fingerprint and binding transition witnesses. Schema035 alone does not activate a catalog or publish prices. The helper rejects captured/original catalogs: this is explicit maintenance of experimental reconstructed content, not recovered May2013 content.

## Local acceptance and limits

The affected older personal catalog advanced coherently with two retained interiors and three current item bindings. Exact custody rows, native identities, stored/placed house locations, poses and room profiles were conserved. Private checks refused stale revisions and wrong original content; both a successful rolled-back transition and a late policy failure conserved the previous state. Matching world and zone binaries were built and deployed.

The original client placed a Small Storage Crate through its normal inventory Place preview on the repaired plot while one house was placed and another was stored. Its native item list immediately displayed the crate and its capacity. The crate was then picked up through the native item window and returned to inventory. Normal Enter on the existing house reached the same permanent interior with its original stored room item still listed. No pending inventory application remained.

The full reconstructed75-plot Sunrise catalog already included crate174 and allowed personal/guild child references; that catalog did not need this retroactive repair. Its child budget and later-Live reconstructed tariffs remain separate administrative content policy. Nonplaceable vault packing during Move Out remains unfinished. No new opcode or wire struct was required. Private accounts, fixtures, logs and client assets are excluded from this repository.
