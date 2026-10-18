# Reconstructed Sunrise Hills plot geometry

`sunrise_plot_reconstruction.h` supplies physical local content for71 personal plots and4 guild sites. It does not automatically import a catalog, allocate estate identities, change a door, or alter existing owned property. These are reconstructed local regions, **not authenticated original Live server boundary records**.

The installed client map `neighborhood.txt` contains71 consecutive four-edge personal footprints at lines14920–15203. All71 footprint centers match unique full-sized database lot-marker centers within1.183 units.70 address labels match a footprint corner within11.1 units. The remaining marker29 is labeled `103 Betwixt Avenue` by elimination; `FindPersonal(29)` deliberately rejects this inference unless the importer explicitly opts in. Markers30 and70 do not correspond to these71 footprints, and tiny decorative markers155–157 are excluded.

Each personal region uses an orthogonal square oriented from opposite physical edges and inset at least2 units from every averaged outline edge. The input map's largest paired-endpoint disagreement is3.042 units. All reconstructed corners lie inside their corresponding averaged map outlines; all75 personal/guild XY boxes are pairwise disjoint. `ground_min_z` and `ground_max_z` preserve actual map-collision elevations at each personal center and four region corners. The five samples per plot are a floor witness, not an exhaustive continuous terrain proof.

Guild Brimming Way derives from the map's explicit four-edge approximately700x350 outline (lines15545–15548), with a2-unit inset. The other three guild sites derive from large flat collision plateaus around their explicit guild address labels. Maximum supported grid rectangles were conservatively capped toward the original guild marker's700x350 size class, then inset2 units. All4 interiors passed37,213 target-floor collision samples at spacing no greater than5 units; no missing floor or elevation disagreement at least0.5 units was observed. Terrain-derived rectangles authenticate physical support, not the original legal estate boundary.

| Guild address | Center server X,Y | Half extents along local X,Y | Rotation radians | Floor Z |
| --- | --- | --- | --- | --- |
|200 Guild Way|2743.542,2612.9705|318,173|0|110|
|200 Terminus Heights|-1104.2726,2418.262|348,158|0|110|
|200 Vanward Heights|2993.658,-765.9966|318,173|0|0|
|200 Brimming Way|571.65,-2227.774825|343.423358,171.711679|1.608336012|0|

No existing `OBP_GUILDSQUARE` marker was found in the inspected door inventory. An importer must allocate and verify guild marker objects separately. The guild marker model is an entire700x350 frame, not a point pedestal; its actor origin belongs at the site center. Placing that whole actor at a front-edge switch coordinate would move the entire border out of its plot. Server switch/inspection interaction points and actor origins must remain distinct. Native heading conversion for the model's long axis must be checked in one batched guild native placement check before declaring its visual border aligned. Do not relabel personal marker18 as a guild marker.

The `Region` overloads require explicit absolute `bottom_z` and `top_z`; these limits are deliberately not inferred from a flat map or a pedestal height. A local importer may choose a documented envelope such as measured floor minimum−10 to measured floor maximum+200. This is a local policy. The helper serializes Y/X/Z native region axes and validates affine matrices through the existing native region encoder.

Default tariffs belong to `plot_pricing_defaults.h`. Item capacities are independent content. The [official housing guide](https://www.everquest.com/news/imported-eq-enus-52047) confirms a fixed per-plot cap, one house per plot, and independent plot/interior counts; it does not authenticate per-address exterior capacities. Do not substitute house/hall inventory limits (300/500/2000) for exterior plot limits. Missing capacity values need explicit reconstructed policy or a matched catalog, not a claim of original recovery.

Source identity pins:

- `neighborhood.txt`:SHA256 `39a250f7e3cd0027df5ee8a8c69fa900563dee0a63dce57a040f8b40faa46286`.
- `neighborhood_1.txt`:SHA256 `0ce7ced61b9f1e4f00e77676472b5d0eeeb1d9c8d5f27ecd6684ffb880a4ad45`.
- Original `neighborhood.eqg`:SHA256 `da6ae5f8668c38d7ccd773e276a4d25407c54ff70429e2a396d5e8c7a351c963`.
- Actual server collision `neighborhood.map`:SHA256 `d16b27d7c3a07797a45b2900017652d58875207122e7df9e0a506b270d5b6f14`.
- `stonesquare.eqg` guild model `obp_guildsquare.mod`:SHA256 `9f6dc316b69a7f25177f9c173ccc09eefd7f77e5477dab82b9c4a2f6970efcb9`.

These pins establish the inputs, not the original author/date of the community map. Model and terrain asset bytes are not redistributed by this helper.

The later2022/2023 [wasami007 housing table](https://forum.gamer.com.tw/C.php?bsn=5179&snA=2008), already used for default tariffs, also supplies personal exterior capacity and pet limits. `PersonalCapacity` exposes those exact numerical defaults: MarketStreet90/4, ordinary Betwixt/Guild/Vanward and BrimmingWay103–113105/5, ParkCircle105/6, remaining premium streets and BrimmingWay101–102120/6. These later-Live values are a documented reconstruction, not a recovered May2013 catalog. Guild exterior capacity is not given by that table; it remains an explicit importer policy.
