# Placement admission integration — October 20, 2026

The housing runtime now connects original-client collision geometry and the complete published interior scene to constrained Floor/Ceiling admission. Versioned Wall policies handle reviewed mounted furniture with zero or nonzero model centres. Existing saved policy versions remain readable.

## Runtime behavior

Admission reads the estate, room, item metadata, custody subject and published contents on the operation's database transaction. New items have an explicit Add subject; existing items carry their exact custody/native identity and pose revision. A Move excludes only that exact source from collision queries. Pending publication prevents a stale room snapshot from authorizing placement.

The scene combines authenticated original terrain, authored static actors, published furniture and the recipient's actual outgoing actor ledger. The ledger observes packets at stream handoff and invalidates changed geometry. Supported original player/Eye, invisible-marker and furnishing-NPC classes have explicit collision filtering; unsupported published sources remain unavailable rather than disappearing from the query.

The generic furniture provider covers 948 reviewed static item records. Original asset bytes are supplied privately by the operator, authenticated against compiled hashes and never distributed here. [Install original geometry](../INSTALL-ORIGINAL-GEOMETRY.md) before using the original-scene path.

New Wall additions use V8 contact admission: exact centre-subtraction preimages, authenticated finite source contact, original thin-axis radius/normal rules and full-body containment in the advertised room regions. The native Wall path does not require a complete supported picture footprint, an artificial inward corridor or an oriented-box collision test. Those extra server restrictions falsely refused a green native preview near a lower-wall bevel; V8 removes them. Existing stored V7 and older policies preserve their original identities and behavior.

The final placement commit does not transmit the camera ray or selected mode. The server therefore verifies a compatible contact witness over the permitted mode union; it does not claim to reconstruct that untransmitted ray. Complete historical equivalence for every transient ray is not established.

Free placement is bounded by the original room regions and current permissions. Expanded Wall regions never enlarge Free placement. Optional unknown database flags are not rewritten or promoted into historical Live metadata; the runtime binds any effective outgoing capability to current item cache and locked database metadata.

The per-recipient projector follows actual outgoing player size zero rather than internal server size. The authenticated invisible-marker class (race240/gender2) uses the same original volume/radius predicates as the reviewed ordinary classes. These sources are represented and classified rather than silently omitted.

Floor/Ceiling scheduling first probes near the exact implied anchor. A bounded terrain-primitive search may generate candidate start heights, but every accepted candidate requires a fresh complete-scene query, correct nearest source, original normal rules and exact forward equality. Candidate work is charged against the same aggregate budget; no tolerance replaces exact pose equality.

The reviewed closed Standard Guild Hall door remains a real type-3 collision source. Its authenticated original mesh supplies an outward conservative world bound, allowing remote queries to prove nonintersection. Nearby, open or changed doors without a supported precise source remain unavailable for constrained admission. The source census binds door pose and state as well as identity. Door presence comes from the recipient's actual outgoing door packets, so a cache-only door that was never sent is not a collision source. Published doors with changed or missing cache bindings remain unavailable. Additive door publications preserve earlier identities until an explicit remove-all publication clears them.

Interaction reach for ordinary reviewed furniture now measures to its oriented model bounds, using the existing house proximity policy. This allows a hanging furnishing to be moved or picked up from the floor when its ceiling attachment origin is above the interaction radius. It is an emulator proximity policy, not a newly recovered Live distance rule.

## Compatibility boundaries

This implements a reviewed May 10, 2013 RoF2 compatibility profile. Unsupported actor/model states, changed assets, stale publication, unavailable source geometry and exhausted bounded collision queries are refused for constrained admission. It does not establish equivalence for every historical room, curved surface, transformed actor or client build. No opcode numbers or packet layouts changed in this update.

The source includes strict floating-point compile options for the two placement integration translation units. There is no schema migration or client executable modification.

## Native acceptance

Representative original-client checks completed Floor collision placement and movement, Wall placement and movement, Ceiling placement and movement, floating Free placement, and pickup. Saved Wall placement survived server restart and reconnect. Floor admission used a complete-scene original collision witness; Free admission used the original room bounds. This validates the shared supported paths rather than claiming every item was individually exercised.

Temporary acceptance items were removed afterward. Original placed contents, item quantities and attributes, currency, ownership, catalogs, crate state and door records were preserved. Inventory process serials naturally changed on restart.
