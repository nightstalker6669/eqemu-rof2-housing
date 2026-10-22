# Housing item support

## Furniture, NPCs and plushies

Ordinary decorations use reviewed original model/geometry bindings. NPC furniture also needs an authenticated association between the item’s server NPC key and the original actor model/appearance. A readable pet name does not provide that association.

Holgresh House Pet and the documented guild service contracts are enabled. Additional NPC/plushie bindings remain unseeded until original appearance evidence is available. The [December 2010 announcement](https://www.everquest.com/news/imported-eq-enus-52105) confirms Black Bear Cub and Friendly Puppy existed, but does not provide their actor records. Later collected item records alone do not establish May 2013 placement behavior. Do not assign an arbitrary model to make an unsupported item appear placeable.

## Legacy scaling items

Legacy charm/scaling experience (`IsScaling()` / `GetExp()`) is preserved separately from `character_evolving_items`. The server writes an exact unsigned 32-bit experience value for the item and each scaling augment into the existing per-instance `custom_data` field. Ordinary saves, housing custody, supported bags and parcel/offline returns carry that same representation.

The reserved key is `eqemu.housing.legacy_scaling.v1`. It binds each entry to its item template and augment socket; quests should not edit this key. Missing or inconsistent housing witnesses are refused. Normal loading restores the values after augments are created and rebuilds scaled stats. Quest-driven scaling calculations continue normally and save their updated state. Zone entry also saves unchanged scaling items so existing inventory can enter housing directly.

Existing inventory without a saved record follows the previous initialization and quest recalculation behavior. This update cannot recover experience already lost before installation. Unrelated quest fields and empty values are retained; the loader accepts the historical writer’s double separators. Oversized custom data is refused before a save deletes the source row or bag contents.

This does not change NoStorage, transfer, lore, placement, item-class or capacity rules. A scaling item that is not a decoration belongs in the vault/closet, rather than being placed as a world object.

## Bags and nested containers

Supported normal bags retain their actual template capacity, up to the existing native 200-slot namespace bound. Filled bags can contain ordinary items, augments, supported evolving/scaling items and **empty** bags.

A **filled bag inside another bag** remains unsupported. The current RoF2 inventory integration has one content level: a bag-content slot cannot itself be the parent of another saved bag-content slot. Merely making the housing capsule recursive would lose those children when the ordinary inventory writer/loader runs. Housing therefore refuses that shape without consuming the original item. This is a target compatibility restriction, not an undiscovered housing opcode.
