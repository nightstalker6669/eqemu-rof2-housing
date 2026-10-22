# Original placement frames and collision source integration

This update connects recovered source providers, exact placement frames and the bounded height-preimage solver. These are production integration libraries. The running server still uses the previously deployed placement policy; this update does not enable complete native Floor/Ceiling or nonzero-centre Wall admission. No new opcode, packet layout, database migration or client modification is required.

## Terrain coordinates and collision flags

The original renderer loads EQGT terrain directly into world coordinates. Its external EQGZ-v2 loader deliberately skips the first authored terrain placement when creating actors. Applying that placement's translation/rotation to the terrain would introduce an incorrect transform. Subsequent authored model placements follow a separate actor path.

A read-only capture of the original Standard Guild Hall confirms all 13,803 terrain triangles match the original TER resource's ordered coordinates, A/C/B winding and low 16 collision-flag bits. None are missing or added. Native partitioning duplicates and reorders vertices, and material-table compaction changes some material IDs; neither changes those collision flags. Material IDs and the source's high flag bits must not be substituted for collision flags.

`OriginalHeightScene::ReadTerrainSource` supports ten enrolled original personal/guild interior families. Compiled metadata binds the archive, terrain member, external ZON, private sidecar and world-frame identities. The bounded loader also requires current resource/frame/manifest association, an enabled terrain entry and a nonzero scene epoch/source identity. It returns a terrain collision source, never a complete-scene receipt. Operator-owned geometry stays private and is not distributed here.

## Exact frames and nonzero model centres

`OriginalReviewedCache::Read` authenticates complete private model geometry for two native-backed cache profiles, chair54573 and ceiling item66803. It verifies the ordinary raw-flags-zero/nondegenerate profile and requires all six reconstructed cache DWORDs to match independently observed original cache values. This does not promote the older placement registry's approximate dimensions into exact native cache values.

`OriginalPlacementFrame::Build` passes that reviewed cache and explicit pose through the original scaled/rotated first-box producer and the separate unrotated radius box. Packet heading/pitch/roll follow the original renderer getter order. The game's box producer uses its recovered FSIN/FCOS chain; it must not be replaced with the distinct renderer mesh lookup-table matrix.

`OriginalWallFrame` implements the literal Wall-mode forward map: each committed actor coordinate is the binary32 store of stopped query-centre minus rotated/scaled model centre. Its three-axis bounded preimage intervals preserve all compatible binary32 stopped coordinates. Adding the centre back to an already stored actor position is not an exact inverse. These arithmetic witnesses do not establish a collision, a clear corridor or authorization to place an object.

The reviewed offline52296 example also exposes a one-ULP cache extent difference from the existing registry. Its reconstructed rotated centre retains a small nonzero component even at a cardinal heading. Neither analytic snapping nor silently rewriting an existing durable geometry policy is justified.

## Complete-scene query adapter

`OriginalHeightPreimageSceneAdapter::Bind` retains an immutable scene owner and an independently supplied content/generation receipt. An external provider must authenticate the same complete scene before and after every probe. A housing DB row list, a hash string, or `scene.complete=true` alone cannot provide that authority.

The query-aware actor resolver receives the actual constructed start, end and radius. Every changed Floor/Ceiling input therefore generates a fresh source traversal receipt; it cannot reuse a receipt from an earlier probe. The preimage payload preserves normal/fraction/stopped-position evidence without inventing an unavailable world feature point or contact kind.

Terrain and actor primitives are evaluated independently with maximum fraction1 under this conservative compatibility policy. Ambiguity belongs to the final nearest fraction: a later closer contact resolves a farther tie, while distinct sources tied at the final minimum refuse. The older standalone source sweep retains its original API and semantics. A future live authority profile must bind this stricter combined selection and dispatch behavior to its own reviewed policy identity.

Source, narrowphase and kernel-operation accounting includes terrain and actors, including work consumed before a later source or freshness refusal. Collection, hashing, tree traversal and frame construction have separate bounds; kernel steps are not a CPU-time or whole-provider instruction budget. The original normal predicate remains after complete nearest-source selection.

## Linear and accelerated model dispatch

The original definition builder creates a source64 tree only above 200 polygons. A reviewed enabled ordinary model with at most 200 polygons and actual source64 absence follows a separate linear path. `OriginalSimpleLinearQuery` preserves its descending order, explicit callback/backface/zero-fraction policy, cached descriptor data and bounded independent-max1 enumeration. Raw flags zero and polygon count alone are not proof of that class or callback context.

The combined scene adapter distinguishes `PlainSimpleLinear` from `PlainSimpleAccelerated` and refuses mixed receipts. Linear uses the directly transformed local delta and returns the descriptor's cached normal, including edge contacts. The tree branch uses its stored-end/reconstructed-delta stage and its recovered contact-normal handling. Applying the tree's delta cancellation or feature normal to linear models would change native behavior. Both branches retain original local/world transforms and fresh query-bound source receipts.

## Complete scene coverage remains necessary

The original resources themselves explain why terrain-only acceptance would be unsafe:

| Interior family | Additional authored ZON actors after terrain |
| --- | ---: |
| Six enrolled personal interiors | One door model each |
| Standard Guild Hall | 86 props/doors |
| Grand Guild Hall | 26 models |
| Modest Guild Hall | 0 |
| Palatial Guild Hall | 0 |

These are authored-resource counts, not counts of the current dynamic actor trees. Other placed items, doors, NPC/player filters, disabled state and current resource/pose association still need complete coverage or a proved exclusion/nonintersection.

The alternate-height source has an explicit original active flag. Construction/unload clear it; the recovered enable path goes through the EQTZP resource loader. None of the 519 archive entries across the ten enrolled interior resources is EQTZP, and the loaded Standard Hall's original flag was observed zero. This supports the ordinary resource lifecycle; it does not eliminate the requirement to bind the actual current lifecycle/guard state or account for an active alternate source.

## Generic original model preparation

`OriginalSimpleTreeGenerator` reconstructs ordinary source64 trees from authenticated original mesh data. It ports the original initial root, triangle descriptors/spheres, descending insertion, nine-way subdivision, child ordering and compaction, with bounded allocation, depth and visits. The original >200-polygon threshold remains explicit. Its generated item66803 result matches all 24 captured native nodes, 212 ordered references, node-sphere DWORDs and cached descriptor values. This is one native comparison supporting a source-derived generic algorithm; it is not a native observation of every generated model.

`OriginalPreparedFurniture::Prepared::Read` seals preparation behind four compiled resource/class profiles: chair54573, painting52295, ceiling item66803 and crate62586. It authenticates complete private mesh bytes and original archive/member association. Chair, ceiling item and crate use generated trees; the 28-polygon painting uses the genuine linear branch. Only chair54573 and ceiling item66803 have enrolled exact selected-frame caches. Fresh query receipts bind the actual matrix, scale, start, delta and radius. Preparing collision geometry does not authenticate a current actor, its disabled state, inventory custody or scene completeness.

## Authored props and source transforms

`OriginalZonStaticActor` reconstructs authored positions, original chopped binary32 angle conversion and exact scale from compiled ZON profiles. The renderer lookup-table matrix remains unscaled; query transformation applies the separate source scale. All 86 captured Standard Hall authored matrices, angles and scales match their original resource placements, including small nonunit prop scales.

Six personal interior door families and all 112 Standard/Grand authored placements have compiled source profiles. The guild set contains 85 accelerated and 26 linear placements across 54 ordinary collision resources, plus one explicitly accounted-for water resource. The original source path gives these ordinary authored actors public type5, null application data, restriction0 and volume0. A current game-switch reassociation, changed pose or unknown definition invalidates that profile. This does not broadly authorize every type5 actor or crafting object.

The Standard Hall water resource has 40 polygons with preserved low flags1. The original ordinary linear branch skips every one before descriptor, callback or kernel work. A separate sealed `PlainSimpleLinearAllSkipped` profile authenticates this exact compiled resource, flags and ordinary no-mutation class. It keeps the actor present and enabled, counts its mesh/polygons and contributes no height primitives. A water name, raw asset flag or disabled-state assertion alone cannot grant this exclusion; special constructor profiles can change those flags.

## Live bridge work still required

The passive Standard Hall census found 90 current graphics actors: 86 authored props, two character actors, one placed chair and one dynamic guild door. That observation supports the resource audit; it is not a server-generated complete-scene authorization receipt.

The existing display-scene service deliberately omits placed items whose creator has an unacknowledged inventory operation. A collision snapshot must instead account for those sources or refuse, including pending returns whose current scope may already have changed. The moving item's self identity must come from locked custody/native-item state, not be guessed from its item ID or owner. Current generation, source class, appearance transitions and resource association need independent binding for every included or excluded actor.

Ordinary character volume/filter guards have been traced through the spawn decoder and server encoder, but a race/size label alone does not establish the required current model and original height producer. Unproved dynamic actors remain unknown.

Runtime integration must retain estate access, region/body/corridor checks, inventory leases, transaction fences and unchanged scene/pose generations. Full in-game Floor/Ceiling and centre-aware Wall equivalence remain open until that bridge is complete.
