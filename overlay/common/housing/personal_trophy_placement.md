# Landing and Giant physical trophy placement

Physical items 68727 (Trophy of the Landing) and 68730 (Trophy of the Giant) use
IT11223. Their missing placement metadata is repaired by the optional migration
`utils/sql/optional/2026_10_20_housing_verified_personal_trophy_placement.sql`.
The migration must run with world/zone stopped, followed by rebuilding/reloading
shared item data. It checks both exact source identities and accepts only empty
or already-correct placement metadata; conflicting content fails as a batch.
The items table must use InnoDB; nontransactional engines are refused before any
row update. On a SQL error the transaction rolls back. A command-line SQL client
that stops at that error may leave the installation procedure; retrying the
migration removes it first, or remove it with
`DROP PROCEDURE IF EXISTS housing_install_verified_personal_trophy_placement;`.

The evidence combines two distinct sources. A retained October 16, 2026 export
from the collector maintainer supplies the physical item-to-model association and
placement metadata. Both rows have the same complete placement block as existing
supported item 57681: mask12, collision-ignore0, floor type0, definition0,
scales0.1 through2.5, upkeep category4 and maximum1 per estate. The IEEE754 scale
bits are 1036831949 and1075838976. The source hashes, row hashes and line numbers
are recorded in `personal_trophy_placement.json`. These are later collected
metadata; they are not authenticated May2013 item packets.

The original pinned RoF2 installation supplies IT11223 in sofequip.eqg. Fresh
parsing reproduces the existing supported IT11223 EQGMv1 static model, with299
referenced vertices,374 triangles, no bones, no degenerate triangles, and no
ActorTag geometric override. Existing renderer class evidence covers this same
model and geometry policy2. The two new registry records use the exact existing
IT11223 float32 center, extents, resource hashes and outward arithmetic envelope.
No new box, scale convention or collision algorithm is invented. Runtime admission
also requires the exact reviewed placement mask, scales, upkeep and item cap.

The migration leaves `housing_disable_rotation` and
`housing_disable_free_placement` unchanged because those restriction flags lack a
separate authenticated source for these two items. Ordinary floor placement does
not require the additional mounted-item flag profile. This repair grants no
benefit, tier, activation permission or favor credit; those remain independently
checked by the trophy services and their reviewed catalogue.

Static evidence establishes metadata and shared-model admission. Native placement,
selection, charging and recovery are separate runtime acceptance checks.
