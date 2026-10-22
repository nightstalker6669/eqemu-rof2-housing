# Reviewed house and guild-hall variants, October 21, 2026

The compiled registry now recognizes 163 distinct physical templates: the
existing 120 personal houses, 18 canonical Manor variants, 18 equivalent later
Manor aliases, Hermit's Hideaway54999, Behroe hut67078, Evantil's Abode77407,
and four guild halls.
Recognition still requires matching item metadata, original model geometry,
reviewed room content, current ownership and the normal custody transaction.
A registry entry alone does not activate a room or establish native acceptance.

Manor definitions 32/33/34 reuse the reviewed original three-room
stucco/wood/stone associations. Item 67078 has the same complete placement
metadata and model as reviewed 61913, while retaining its own physical identity.
Evantil uses original registered interior 766 (`phinteriortree`) and the
`COL_IT23651` collision resource explicitly selected by the original EQLOD;
its much larger decorative visual mesh is not the collision box.
See [resource provenance](../provenance/house-variants/IT23651-COL-witness.json).

The runtime now checks each guild template's exact reviewed interior association;
being any registered guild interior is insufficient. The existing explicit
legacy 61823 route exception is retained. Registry/selection size is bounded by
the compiled template count instead of an arbitrary 128. Production-named
configuration schemas coexist with the old formats, without changing existing
manifest bytes or durable bindings.

## Portable content generation

The production [generator](../../tools/generate_house_variant_content.py) uses the
operator's matching original client and prepared collision maps, plus the
[capacity registry](../../overlay/common/housing/house_variant_capacities.json) and
[source-bound geometry blueprints](../../overlay/common/housing/house_variant_geometry.json).
It authenticates the executable, renderer, source resources and maps before
writing a fresh private directory. The output contains no actor, property,
allocated native-estate or instance identity. Region IDs are profile-local
implementation identifiers, allocated deterministically by definition.

```sh
python3 tools/generate_house_variant_content.py \
  --client /path/to/ROF2 \
  --maps /path/to/server/maps/base \
  --output /path/to/new-private-content \
  --copy-resources
```

`--copy-resources` copies authenticated operator resources into that private
output; game assets are never supplied by this repository. Without it, the
output lists the exact private resources the loader requires. Use a new output
directory. The command refuses overwrites and does not connect to a database,
control services, grant ownership or activate its catalogue proposal.

For an existing deployment, preserve every configured template with repeatable
`--preserve-registry`, `--preserve-manifest` or `--preserve-item` options. For
example, pass the active personal content registry and both legacy personal/guild
manifests. Preserved files are not rewritten or copied as replacement profiles;
their existing hashes, room capacities and permanent bindings remain unchanged.
The new catalogue proposal still lists every compiled supported family. Unknown
capacity values are supplied only by the separately labelled operator policy
described below.
Do not install a second configuration for an already configured SKU: startup
rejects duplicates rather than reinterpreting bound houses.

The output uses `housing-personal-content-registry.json` and
`housing-guild-content-registry.json`, with `housing-house-ITEM.json` and
`housing-room-ITEM.json` entries. The runtime accepts these production schemas
and the existing legacy schemas. The initial 22-profile extension retained 121 existing configurations, yielding 143
selections. The separately configured Grand profile completed the initial144 selections.
The subsequent18 Manor aliases and Hermit add19 selections, yielding163, while
preserving those144 configured profiles byte for byte. Merge new personal
registry entries rather than replacing the existing registry. Preserve
existing Palatial/Modest registry entries when adding Grand; a generated registry
containing only the new entry is not a replacement for their active registry.

## Capacity and geometry boundaries

The researched evidence registry remains unchanged, including its historical
unknowns. A separate [configurable capacity policy](../../overlay/common/housing/house_variant_capacity_defaults.json)
records every effective field and its source classification. Personal totals
are300/500, Manor500 and Evantil600. Hermit uses the researched400 total and the reviewed723 route described
in its [provenance guide](../provenance/house-variants/HERMIT-HIDEAWAY.md). Standard1000 and Modest1500 use the stated
later placeable-derived budget; Palatial2000 has period player evidence.
**Grand1500 is an explicit server default**, not a recovered historical total.

The approved policy uses vault capacity equal to the shared total, personal
NPC quota5/guild20, and one child estate. Vault and NPC entries consume the
parent total; they do not add extra total slots. Only exact existing technical
child174 is supplied, with its separate100-item/100-vault budget. Existing
property-wide moving-crate restrictions still apply. Other recognized crate
definitions have no approved complete record here. These limits and the child
record are labelled emulator policy wherever original data is missing.

Edit effective fields and accurate provenance in the separate policy file and
increment its policy revision. Regenerate fresh profiles. Existing bound rooms
require the checked capacity ledger upgrade; changing a file alone does not
reconfigure them. The 18 later Manor aliases reuse exact structured model/definition metadata;
contradictory lore remains untouched. Hermit's723 association is an explicitly
reviewed reconstruction, not a captured original server definition. See the
[Manor evidence](../provenance/house-variants/MANOR-ALIASES.md) and
[Hermit evidence](../provenance/house-variants/HERMIT-HIDEAWAY.md).

### Existing rooms and future preserved templates

[Migration039](../../overlay/utils/sql/housing/039_house_capacity_policies.sql) adds
append-only house policy history, a current head and exact template/base grants.
The checked [maintenance API](../../overlay/common/housing/interior_capacity_policy_db.h)
requires the caller's transaction and stopped, quiescent runtime for existing
rooms. It pins original profile revision, manifest, definition and region hashes;
checks current usage; permits only monotone capacity changes and retained or
reviewed additional child records; and advances the separate policy head.

The immutable room payloads, manifests, house IDs, custody, layouts, saved poses,
profile revisions and historical furniture fingerprints remain unchanged.
Original technical10 limits in legacy61823/Standard base records remain identity
bytes; an installed policy projects the approved500/1000 limits for gameplay.
Normal display, admission, vault/NPC/child counts, layouts, swaps and offline
room resolution use that same locked effective projection. Native bootstrap
serializes effective definitions; content identity serializes the retained base.
Physical and list-only child-crate members are excluded from parent censuses.

Exact template/base grants cover future preserved legacy configurations. Only a
newly inserted, empty room profile may consume such a grant in its existing
allocation transaction. Already bound rooms use maintenance, never an online
upgrade bypass. A later region migration must explicitly rebase its capacity
policy rather than stranding history; the older region upgrader refuses an
active capacity head. Capacity reductions are deliberately rejected by this
upgrade API. The reviewed deployment installed policies for all ten existing bound rooms and
exact grants for all 144 configured templates. Original profile, custody and
saved-pose bytes were conserved. This deployment result is separate from native
capacity acceptance; source alone never installs the policy.

NPC quotas enable capacity, not item recognition. Actual NPC placement still
requires an authentic reviewed source record, supported model/height/placement
metadata and normal custody/geometry checks. No source binding was invented or
seeded merely to fill the new quota.

The six existing personal-room families reuse their reviewed technical region
matrices unchanged, with fresh profile-local IDs. Other supported guild rooms
and Evantil use their exact compiled full original-source connected envelope,
with actual source-bound collision and placement checks still required. These
are technical admission regions, not captured Live permissions. Original Map
clearance and representative native entry remain distinct from a recovered
historical spawn. No entry-floor-only guild profile is advertised as whole-room
coverage.

## Deployment and native scope

Use the generated proposal only for a reviewed fresh catalogue/neighborhood,
with the production persistence and catalogue allocators. Copy exact source plot
regions and reviewed pricing; allocate new domain/native IDs and retain all
original ownership. Existing bound catalogues must not be rewritten to add these
profiles. Administrative ownership used for development is distinct from native
purchase acceptance.

Stop world and every zone before metadata repair, configuration deployment and
shared-item regeneration. The optional [Evantil item repair](../../overlay/utils/sql/optional/2026_10_21_housing_evantil_metadata.sql) changes only reviewed
item77407 fields and refuses conflicting content or pre-existing bindings.
The [Evantil zone registration](../../overlay/utils/sql/optional/2026_10_21_housing_evantil_zone.sql) supplies original zone766 identity with explicitly technical
entry/noncombat policies. The [supported guild-zone repair](../../overlay/utils/sql/optional/2026_10_21_housing_supported_guild_zones.sql) removes existing admin255 gating only from
matched737/738/751 rows, or creates missing matched identities with stated technical
defaults. It preserves their existing safe points and other columns and covers the approved Grand room as well. These access settings are operator housing policy,
not recovered Live restrictions. Generic player zoning restrictions remain active.

House and own-guild Lobby entry now check exact loaded zone/version metadata and
matching current content rows before transactional instance membership admission.
Absent/inconsistent destination data returns the existing checked entry error
without changing routes, bindings or custody; this guards the observed missing766
metadata failure. It does not guarantee remote process readiness or prevent
administrative deletion after the check.
Regenerate the intended shared-item prefix, **then select that generated prefix
after `shared_memory` has finished**, and cold-start matching processes. This
upstream generator can clear `variables.hotfix_name` when its value is
`hotfix_`; setting it only before generation can leave startup selecting a
stale/default dataset. Verify the resulting variable, dataset and actual running
processes. A successful `#hotfix` alone is insufficient.

Native Evantil exterior placement passed on an actual personal plot using the
original collision box. The house survived a cold restart and entered its original
tree interior after metadata repair; a native chair placement visibly rendered on
the interior floor. Modest original-room entry, representative chair placement and pickup also passed.
Palatial placement on an actual guild plot and entry into its original737
interior passed. The native client displayed the approved2000 total/shared vault
limit. A chair completed placement/storage/retrieval; all17 items in a stack
completed Inventory→Closet→Inventory; and an original empty small moving crate
completed interior placement and pickup, showing its separate100-item budget.
All three returned item custodies were removed without item loss. The new
Palatial room received its template capacity grant automatically at allocation.
The Tree chair was subsequently picked up and restored to inventory. Grand
placement, entry into its original738 interior and representative furniture
placement/pickup passed; the native UI displayed its explicitly labelled1500
shared budget, and its new room automatically received the template grant.
Palatial exterior Placed→Shed storage also passed while preserving its permanent
room. Original item quantities and pre-existing housing state were conserved.
The native partial-split check also passed: Ctrl-click produced one remaining
inventory chair and one preview item; placement was accepted, pickup merged
the item back into the original two-chair stack, the cursor was empty and no
floating duplicate remained. This source/content update does not claim an individual gameplay
cycle for every SKU, complete historical capacities, new unsupported doors or
full Live parity.

The ordinary partial-split integration now refreshes only newly allocated RoF2
placeable destinations with ItemPacketTrade after persistence. This supplies
the server's actual item GUID without relaxing housing identity checks. Whole
moves and other client/item classes are unchanged. The representative native
Ctrl-click/placement/pickup/merge cycle conserved both chairs and left no cursor
item or floating duplicate.

### Retirement of an empty plot after house pickup

A carried house keeps its permanent interior and historical native parent for
later placement. The shared personal/guild empty-property return path now checks
the complete carried house/location/interior/native identity, actual inventory
item, original model/definition and permanent instance. It requires an empty
room and no pending work, active owner/room session or lease. Completed native
placement history remains readable; live or dangling associations still refuse.
The native room node remains active and unchanged for the existing allocator's
later reparenting. A placed, nonempty or pending child continues to block this
empty-property path.

The checked guild retirement service completed this case on a fresh empty
administrative property with a carried Modest hall. It returned no items or
refund and preserved the original house profiles, custody, inventory, wallet
and unrelated guild state. The explicit local review used an advanced callsite
maintenance time; it did not change player login timestamps or the global clock.
This establishes the service path, not a new native timer or purchase cycle.

### Character-save compatibility

An opt-in guild recipient policy exposed an incompatible foreign key: upstream
character saves use `REPLACE`, which transiently deletes the character row.
The [guarded compatibility migration](../../overlay/utils/sql/optional/2026_10_21_housing_character_save_compatibility.sql) removes only that exact constraint,
retains all policies and receipts, and leaves current-recipient validation with
the checked retirement service. Fresh policy creation omits the constraint.
The migration was applied with policy rows conserved. Ordinary native zoning
then saved and reloaded the correct destination instance, confirming the
character-save correction. No upstream save implementation or unrelated foreign
key changes.
