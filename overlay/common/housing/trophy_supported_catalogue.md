# Evidence-supported trophy catalogue

The deployable profile contains three researched benefit definitions. It uses published level/cost/effect values and retained item-collector source/stat associations. It is a reconstruction, not an authenticated original May 2013 server catalogue.

| Benefit | Physical item | Minimum level | Favor per ten-minute cycle | Hidden stat item | Effect |
| --- | --- | --- | --- | --- | --- |
| Trophy of the Giant (216) | 68730 | 95 | 75 | 68736 | +400 HP, mana and endurance |
| Trophy of the Landing (213) | 68727 | 95 | 250 | 68733 | +45 AC, +20 heroic stamina |
| Guild Trophy of Hardened Steel (111) | Empowered Trophy of Hardened Steel, 66750 | 90 | 225 | 66571 | +50 AC, +250 HP, +20 heroic strength |

The hidden stat items grant the researched benefit rather than the physical trophy's equipment statistics. Existing code requires a valid placed source on an eligible property. A catalogue entry alone neither admits an item for placement nor proves native gameplay acceptance.

## No guessed content values

The 2026-10-18 profile also seeded ten final tradeskill benefits at minimum level 1 as an explicit emulator default. Their historical housing-benefit minimum levels have not been established. That default has been withdrawn: fresh installations no longer insert those ten definitions. Their known 15% skill effects and zero upkeep remain research evidence; they are not sufficient to establish every required deployment field. The JSON records these excluded entries with `level: null`.

For an existing installation of the earlier profile, apply `trophy_verified_catalogue_correction_20261020.sql` with world/zone processes stopped and no other database writers. The correction accepts only the exact prior seed triplets or wholly absent triplets. It refuses changed/partial rows and any personal, guild or ordinary tribute selections referencing the affected benefits. It rolls back on error and does not erase selections to force success. It preserves inventory, evolution, placements, favor, billing state, historical charges and unrelated definitions. Reapplication after a successful correction is harmless. Do not rerun the fresh-install catalogue SQL over existing keys.

Other researched trophies remain excluded when a required tier price, minimum level, effect or stat association is missing or conflicting. This includes lower tradeskill forms; alchemy's missing dedicated local stat item; incomplete Breeding Grounds modifiers; and contradictory Empowered Resplendent Purity effects. No new tiers or benefit values are inferred.

## Timing and evidence boundaries

Original RoF2 client help documents ten-minute trophy billing cycles. Pause, withdrawal, edit invalidation and offline renewal behavior are existing emulator implementation policies, not newly asserted historical content values. The two paid personal source items previously lacked reviewed placement metadata; their catalogue definitions alone must not be advertised as usable placement. Native placement, selection, activation, billing and withdrawal acceptance must be reported separately from catalogue deployment.

The retained research ledger covers 119 physical items and 167 benefit/research rows. It distinguishes physical evolving-item ranks from housing-benefit tiers. This production profile deliberately covers fewer definitions than that ledger.

## Sources

- [Bonzz tribute and trophies](https://www.bonzz.com/tribute.htm): published levels, prices and effects; later secondary evidence.
- [EQ Traders trophy guide](https://www.eqtraders.com/articles/article_page.php?article=g178): physical evolving tradeskill modifier progression, distinct from housing-benefit tiers.
- [Item collector](https://items.sodeq.org/download.php): retained later source-to-benefit and hidden-stat association evidence.
- Historical Rasper observations and original client help/spell witnesses remain in the private research ledger; disagreements are not resolved by inventing values.
