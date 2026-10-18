# Full reconstructed Sunrise Hills catalog builder

`sunrise_neighborhood_defaults.h` assembles a complete fresh neighborhood catalog without modifying a database or running server. `Build(settings,content)` returns75 plot nodes and one neighborhood root. It runs the existing `Catalog::Validate` graph and wire validation before returning.

The caller must provision a fresh neighborhood and its75 domain plot IDs through `PersistenceService`, supply a new catalog key and noncolliding definition/region/cost namespaces, and select a premerged original content bundle. `plot_ids` order is71 `SunrisePlotReconstruction::PersonalPlots` records followed by4 `GuildPlots` records. The builder copies selected house type2/3 and crate type7 definitions unchanged, with their original cost and region records. Old root/plot definitions and all old native nodes are excluded. Content selection must contain at least one personal house, guild hall and moving crate, and every referenced child definition.

Personal plots allow supplied type3 houses and type7 crates; guild plots allow type2 halls and type7 crates. New definition children are a content allowance, not instantiated houses. The compatibility child budget defaults20 because `max_children` counts every active native child, including moving crates; a budget1 would allow a house or crate but prevent using both. Separate runtime guards enforce one placed house/hall during admission, stored-house restoration and retained-house relocation. Moving a placed house excludes its own native item identity from sibling counting. This keeps the shared child budget available to moving crates. The child budget is local policy rather than a recovered original Live limit.

Explicit reconstruction choices:

- All75 plots require `allow_inferred_103_betwixt=true`; without this review the full builder refuses activation. The geometry helper can still select70 labeled plots separately.
- Personal item/pet limits and copper tariffs use the published later-Live defaults documented by the geometry/pricing helpers.
- Guild exterior item/pet limits must be supplied explicitly. No authenticated guild exterior inventory cap was recovered. For the current development content,200 items/6 pets is an explicit compatibility policy, not an original-value claim.
- Plot inventories have no non-placeable vault capacity. Houses and halls retain their own original definitions.
- Vertical volumes span measured collision minimum−10 to measured maximum+200; these are explicit local bounds.
- Root and plot definitions, region IDs and all purchase/upkeep scheme/component IDs are allocated from caller namespaces. Domain IDs and native estate IDs are not conflated.

`GuildMarkers()` returns4 proposed original-model frame actors with static IDs199–202 in order Guild Way, Terminus Heights, Vanward Heights, Brimming Way. The caller must refuse any conflicting existing neighborhood/version0 door records before inserting these actors and use `OBP_GUILDSQUARE`/open160. Actor centers are the reconstructed footprint centers; they are not front-edge inspection points. Native-axis heading reconstruction requires batched native visual corroboration. Existing personal markers, unassigned full-size markers30/70 and tiny decorative markers155–157 remain unchanged.

Import this as a **new** catalog attached to a fresh neighborhood. Do not replace a catalog containing existing owned properties. Native IDs are assigned by `Catalog::Service::Import`; no fake owner or current estate is provisioned. Purchase and upkeep enrollment policies are separate administrative reviews, bound to the resulting catalog revision and wire hashes. The builder does not silently enable either policy.
