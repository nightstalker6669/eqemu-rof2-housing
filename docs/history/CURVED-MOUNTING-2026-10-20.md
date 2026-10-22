# Finite curved-wall support and original height-query integration

This update adds a conservative V6 server placement policy for reviewed original interior surfaces. It is not a recovered reproduction of every Live curved-surface rule. No opcode number, packet layout, SQL migration or client UI modification is required.

## Supported placement

New mounted-region bindings select V6 for the ten enrolled interior geometry families, using the same exact asset/map/source ledger as V5 plus explicit original Wall/Ceiling normal classifications. Only Wall placement gains new curved support. The runtime extension is restricted to the reviewed zero-centre model anchor profile; nonzero-centre models retain their prior path and gain no curved exemption. An unsuccessful V5 planar proof may continue to a connected noncoplanar surface whose exact projection covers the entire oriented item footprint without gaps, overlapping layers or folds.

Collision exemptions remain limited to the contacted connected coplanar component. Every noncoplanar neighboring face remains a blocker for the complete item body and its complete inward corridor into an existing authenticated interior region. Neither a bounding box nor a convex hull replaces finite source coverage. Current transaction, lease, item metadata, room, ownership and access checks remain mandatory.

The new policy bounds work explicitly: at most 65,536 authorized source IDs, 4,096 local/component triangles, 128 footprint-relevant faces and eight full-map corridor attempts. Source shortlisting uses the complete body bounds expanded by a two-unit search margin. Exact fitting corridors are ranked by translation distance. These are server compatibility limits, not historical Live limits; limits and every source classification enter the new policy identity. Complexity exhaustion refuses the new extension.

## Saved-content compatibility

`PlanarReviewed` retains the exact V5 constructor and digest. Default `Reviewed` can select the V6 successor only for a matching reviewed original family. Existing V5, V4 and earlier saved bindings resolve through their immutable predecessors. No stored row or policy digest is silently rewritten. A new Add may adopt V6; moving or replaying an older object retains its selected policy.

The bridge appends a dedicated curved-source callback and verifies the actual loaded map source count against the selected hash-bound source policy. Existing Map/Raycast APIs retain their meanings. Shared headers contain only lightweight contact types; exact geometry stays in implementation/policy code.

## Original Floor/Ceiling mathematics

The original final cached-box producer uses rounding toward zero. A nearest-even conversion can differ by one float step in model centers or extents. `OriginalCachedBox` retains the original sum/difference, half multiplication and final binary32 store for independently reviewed eligible-source bounds; it does not infer source eligibility or ActorTag overrides.

`OriginalHeightPreimage` finds complete finite XY preimage intervals through the actual forward coordinate mapping. Its bounded vertical search seeks a compatible original height fallback result and compares all final position bits. It never treats inverse final-position arithmetic as the original transient input. Search exhaustion is inconclusive. The corrected `MatchesHeightPose` compares the forward result directly.

The scene-query libraries now separate terrain from plain furniture actor dispatch. They preserve the actor scale/inverse matrix, local query start/delta and stored-end reconstruction, renderer lookup-table matrix production, and one final world-space travel pullback. The bounded mesh loader authenticates operator-supplied private geometry against reviewed resource hashes. The merger refuses uncertain sources and equal-time ambiguity. A bounded source-tree reader and traversal now authenticate one reviewed model family against its private mesh/tree hashes. Generated original renderer trig tables match 1,024 captured DWORDs; the matrix factory matches all 32 DWORDs across two actual poses. Native descriptor cache reconstruction matches 1,060 captured DWORDs. Complete current-scene snapshots and additional model families remain provider requirements; these APIs do not manufacture those proofs from item bounds. The conservative candidate traversal omits hit-dependent pruning and the merger refuses stored-fraction ties; it is not an exact ordered native-winner claim.

These recovered APIs require strict floating-point compilation, complete authenticated source coverage, a reviewed native frame producer and a separately versioned authority policy before use. They are not enabled as a replacement for live Floor/Ceiling admission in this update. Raw client assets, native resource trees and process snapshots are not distributed.

## Remaining boundary

A further source audit corrects an older wall-anchor assumption: the client stores query centre `Q = P + rotated/scaled C`, and subtracts the same C after collision resolution. Wall mode retains stopped Q without the Floor/Ceiling extent shift. The current legacy wall witness uses P alone; representative painting geometry has zero centre, but centre-aware admission for nonzero-centre wall models remains unfinished. An inverse P+C calculation does not by itself recover the original binary32 preimage.

The finite curved-wall extension is built and deployed. The unmodified RoF2 client successfully placed a small painting across a curved Standard Guild Hall edge. The exact native pose passes V6 while the base and full V5 policies refuse it; the stored placement matches that pose. A subsequent native pickup returned the item. This is representative acceptance of the bounded extension, not every interior or furniture model. Unsupported folds, complex geometry and any placement lacking a full body/corridor proof refuse. Full Floor/Ceiling scene integration, original transient-input capture and universal curved-surface equivalence remain unfinished. The recovered math must not be advertised as completed in-game Floor/Ceiling behavior.
