# Configurable house and guild hall capacities

`overlay/common/housing/house_variant_capacities.json` preserves the research evidence and its unknown values. The separate `overlay/common/housing/house_variant_capacity_defaults.json` selects explicit emulator limits. Defaults are configurable server policy; they are not newly recovered Live values.

Revision 1 uses these limits:

| Family | Shared total | Non-estate vault subset | NPC subset | Child estates |
| --- | --- | --- | --- | --- |
| One-room personal | 300 | 300 | 5 | 1 |
| Three-room personal, including supported Manors | 500 | 500 | 5 | 1 |
| Hermit's Hideaway, when its route is reviewed | 400 | 400 | 5 | 1 |
| Evantil's Abode | 600 | 600 | 5 | 1 |
| Standard Guild Hall | 1,000 | 1,000 | 20 | 1 |
| Modest Guild Hall | 1,500 | 1,500 | 20 | 1 |
| Grand Guild Hall | 1,500 | 1,500 | 20 | 1 |
| Palatial Guild Hall | 2,000 | 2,000 | 20 | 1 |

The personal and Palatial totals have the qualified evidence described in [capacity provenance](provenance/house-variants/CAPACITIES.md). Standard and Modest use later published placeable counts as conservative shared budgets. Grand's 1,500 is an explicitly chosen server default: its historical total remains unknown. Vault, NPC and child limits are labelled defaults. The personal NPC default matches the existing conservative Sunrise yard limit.

Ordinary placed furniture, NPC items and non-estate vault entries consume the same overall item budget. A 500-total house with 300 vault entries has room for at most 200 other direct items; its vault limit does not grant another 500 slots. NPC quotas enable capacity only: an NPC item still needs its separate reviewed metadata and binding. Quotas alone do not make an unsupported NPC item placeable.

The only approved child is moving-crate definition 174: an existing emulator record with 100 total and 100 vault slots, no NPCs and no nested children. Its physical crate consumes a parent item slot; its contents use the separate child budget. Existing property-wide moving-crate restrictions still apply. Recognition of other crate item IDs does not supply their missing capacities.

## Changing limits

Edit a definition's `effective_fields` in the defaults file, keep `field_sources` accurate, and increment `policy_revision`. Each subset must be between zero and `max_items`. A zero disables that category. Only reviewed child definition IDs are accepted, and definition 174's existing bytes must remain unchanged.

Generate into a new private directory with `tools/generate_house_variant_content.py`. Its `--capacity-policy` option selects an explicitly reviewed alternative policy file. Use `--preserve-registry`, `--preserve-manifest` or `--preserve-item` for existing deployments; the generator refuses to overwrite its output directory. It emits `housing-capacity-policy.json`, revised fresh profiles and a catalogue proposal, with the exact policy revision and source hash recorded in their manifests.

Never overwrite a bound room's immutable base profile or manifest merely to change capacity. Existing rooms require the separate reviewed capacity-policy ledger, with an exact base-profile proof and an append-only revision chain. Capacity increases must preserve existing child definitions and their bytes. Future allocations from preserved legacy templates need the runtime's reviewed per-template policy installation; newly generated full-capacity profiles already contain their selected limits. This generator does not apply a policy to a database or running server.
