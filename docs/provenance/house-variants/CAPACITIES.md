# Capacity evidence for RoF house variants

Capacity is server content. Original client geometry identifies a room and its surfaces; it does not supply the server's item, vault, NPC or child-estate limits. No complete authenticated May 2013 definition batch has been recovered.

| Family | Supported count | Evidence and qualification |
| --- | --- | --- |
| One-room personal houses | 300 total | Universal change in the 17 April 2013 housing update. Includes the reviewed Behroe hut alias 67078. |
| Three-room personal houses | 500 total | Same universal change. The 36 Manor items 57501–57518 and 64378–64395 all have three-room lore. Some later Manor lore has incorrect style/material strings; those are not route authority. |
| Hermit's Hideaway, 54999 | 400 total | Explicitly named in that update. Capacity does not resolve the original 720/723 Dragon House interior ambiguity. |
| Evantil's Abode, 77407 | 600 total | Explicitly named in that update, independently of its two-room Tree House lore. Original client registration identifies 766/phinteriortree. |
| Standard Guild Hall, 66618 | 1,000 placeable | Later firsthand player guide, December 2023; not an authenticated 2013 total. Existing bound base profiles remain immutable; the separate capacity ledger can raise their effective limits. |
| Modest Guild Hall, 66660 | 1,500 placeable | Same later guide, with a tentative independent January 2021 report. Any use is explicitly a later-evidence reconstruction. |
| Palatial Guild Hall, 66620 | 2,000 total | Contemporaneous firsthand player report on 17 April 2013; weaker than a server definition packet. |
| Grand Guild Hall, 66619 | Unknown | No exact credible numeric capacity recovered. A price or package name is not a capacity witness. The separate configurable server policy now supplies a labelled 1,500-item default; historical evidence remains unknown. |

The personal totals are supported by the [original producer housing announcement](https://forums.everquest.com/index.php?threads/housing-and-marketplace-improvements.199446/) and the [original RoF2 developer's dated patch transcription](https://www.eqemulator.org/forums/archive/index.php/t-39092.html). The [period Palatial report](https://forums.everquest.com/index.php?threads/housing-and-marketplace-improvements.199446/post-2945712) names a 2,000-item limit. The later Standard and Modest counts come from the [December 2023 firsthand guild hall guide](https://www.redguides.com/community/threads/so-you-want-to-buy-a-guild-hall.89350/), with [January 2021 Modest corroboration](https://www.redguides.com/community/threads/teleport-anchors-and-pocket-guilds.76065/). These later counts must not be presented as recovered 2013 values. The guide's separate approximate statement about guild halls collectively does not establish Grand's limit.

The reviewed capacity registry leaves unknown vault, NPC and child-estate limits and child-definition vectors as `null`. The separately approved configurable server policy can now enable these categories with labelled limits. A configured zero still means an implementation restriction, not a claim that Live's corresponding limit was zero. See [configuring house capacities](../../CONFIGURING-HOUSE-CAPACITIES.md). A placeable count is not automatically proof of a total including every storage class.

Existing bound room profiles and their catalogue/content digests form part of custody and recovery history. New content uses fresh reviewed profiles; this evidence does not authorize rewriting historical bindings or copying Standard's technical fixture capacities to another hall.

The Evantil metadata migration restores only researched item fields from a retained later collector record: personal-estate placement mask 8, definition 42, scale 0.11–1.0, upkeep reference 25 and unlimited-per-estate sentinel -1. The collector record dates from 2015/2021, so the migration is labelled reconstruction. Original client geometry and zone registration independently support the model/room association. Unknown rotation and free-placement flags remain unchanged.
