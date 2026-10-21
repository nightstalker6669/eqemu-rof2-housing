# Manor item aliases

Items **64378–64395** are separate physical Manor items corresponding, in order, to reviewed items **57501–57518**. Their structured native housing metadata agrees: display model, definition ID, placement mask, collision flag, scale limits, upkeep and maximum-per-estate. Local item rows already contain these fields, so this addition requires no item repair migration.

The six original exterior models are IT20020–IT20025. Definitions 32, 33 and 34 select the existing reviewed three-room interiors 714, 716 and 717. Each alias retains its own physical item ID and reuses the corresponding canonical model's exact original geometry, source hashes, bounds and placement policy. Recognition does not grant ownership, inventory or placement authority.

Thirteen later items, **64383–64395**, have inconsistent style/material text in their lore. The display names and structured model/definition fields agree with their canonical counterparts. The implementation follows those structured fields and preserves the historical lore text. It does not infer a different interior from the inconsistent lore.

The retained [collector item download](https://items.sodeq.org/downloads/items.txt.gz), obtained through the [download page](https://items.sodeq.org/download.php), has gzip SHA-256 `ec0699f4f0e7f070b2e99297f521e9a266b13010b41b0b443216da2be2865302`. Its item records are later collector data, with 2015 creation and 2021 update dates; they are **not** an authenticated May 2013 item catalogue. Original May 2013 client geometry and existing reviewed canonical interior associations provide separate evidence. This addition establishes a reviewed reconstruction, rather than claiming a recovered Live server item-routing table.

The [capacity policy](../../CONFIGURING-HOUSE-CAPACITIES.md) remains revision 1: researched three-room total 500, with separately labelled configurable defaults for vault, NPC and child-estate limits. Existing bound room manifests and capacity grants are preserved. Newly generated aliases receive independent manifests and profile identities.

Representative native acceptance used alias64378 on an actual personal plot.
Placement, the original context-menu entry into714, the500-item budget display,
and Placed→Shed storage passed. The permanent interior was retained. This is
representative family coverage, not an individual gameplay cycle for all18 items.
