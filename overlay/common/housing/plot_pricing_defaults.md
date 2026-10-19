# Plot prices

Every Sunrise Hills personal and guild plot receives a positive copper purchase
price and daily plot upkeep. Known streets retain the reconstructed tiers:

| Plot tier | Purchase, platinum | Daily upkeep, platinum |
|---|---:|---:|
| Small personal | 21 | 2.1 |
| Market Street 101–102 | 42 | 4.2 |
| Ordinary personal | 84 | 8.4 |
| Large personal | 126 | 12.6 |
| Premium personal | 168 | 16.8 |
| Guild | 21 | 2 |

The address table in `plot_pricing_defaults.h` selects the exact tier.
Unlisted or custom personal addresses use 84/8.4 platinum; unlisted guild
addresses use 21/2 platinum. Other estate types do not receive a plot tariff.
`Lookup` remains a strict lookup; `LookupWithFallback` and `Apply` provide the
complete personal/guild coverage. `Apply` refuses already-priced definitions
and colliding identities, so an administrator must explicitly review repricing.

These are approximate emulator defaults, using later community price tables,
not an authenticated May 2013 catalogue. Provenance is recorded in the header.
House and placed-item upkeep remains separate and contributes through the
existing placed-item billing graph; stowed items do not accrue that upkeep.

## Existing catalogues

`CatalogPricingUpgrade::ApplyUnderTransaction` is an offline administrative
operation for experimental catalogues, with a caller-owned transaction and
explicit review. Install the optional pricing journal schema first. Supply an
exact before catalogue and a revision increment containing only plot scheme
changes and additional cost schemes. Old cost records remain immutable.
The operation compares stored bytes, refuses live sessions, pending applications,
previews and existing rent contracts, and verifies transactional table engines.
It advances current placement, permanent interior and crate links without
changing spatial fingerprints, source custody, native identities or geometry.
Purchase policies receive the new catalogue hash. Other revision-bound grants
remain stale until separately reviewed; the migration does not broaden them.
Historical outcomes retain their original revision.

The journal stores before/after definition payloads. A failed operation rolls
back to its savepoint, and the caller must roll back the containing transaction.
Do not reverse an applied price revision after new transactions use it; publish
another reviewed forward revision instead.

Prices alone do not authorize a purchase or start a rent clock. Review the
purchase and upkeep enrollment policies against the new catalogue hashes.
This transition never debits a wallet, refunds a purchase, or manufactures old
arrears. Existing contracts need a separate prospective-rate migration and are
refused by this operation.
