# Guild access and Lobby routing

The existing production implementation keeps neighborhood guild halls and the ordinary basic Guild Hall separate. This follow-up documents their operator-facing behavior and representative original-client acceptance. No new opcode, struct, schema or production code change was needed for the checked paths.

## Guild Lobby left door

Safe startup registers the observed left door40 in Guild Lobby344/version0/instance0. The hook requires the actual source, door and server-measured range. Other doors retain their existing handling.

- A current guild member whose guild owns a current guild plot with a placed, eligible guild hall enters that hall's retained permanent interior. Admission checks the current guild, ownership generation, canonical plot/hall graph, custody, content binding, Enter permission, upkeep and actual destination instance before granting membership.
- A current guild with no eligible placed hall uses the existing basic Guild Hall route. Inventory, stored and crate-contained halls do not count as placed halls.
- An authorization, ambiguous target, invalid binding or database failure refuses entry; it does not silently grant basic-hall access after a failed owned-hall authorization. Characters without a guild retain the existing ineligibility.

The native left-door click reached the permanent furnished Standard Guild Hall. After native Pack removed the placed hall, the same door reached the separate finite basic-hall instance, while the original permanent room remained retained. The basic fallback does not become a furnishable neighborhood estate. The Standard Hall's accepted basic interior geometry does not change this instance/ownership distinction.

## Access roles

Open `/realestate manage` on the plot and use Access to configure named characters or exact guild ranks. The current leader receives implicit Owner access only when the stored rank1 and actual guild leader agree. Other ranks require their configured grants.

The default capability masks are recovered native values, not inferred rank privileges:

| Group | Native role | Default mask | Main item permissions |
|---|---|---|---|
|0|Everyone|`0x0003`|Enter and receive buffs|
|1|Visitor|`0x0003`|Enter and receive buffs|
|2|Acquaintance|`0x0803`|Visitor permissions plus interact|
|3|Friend|`0x0e03`|Add, move and interact with furniture|
|4|Trustee|`0x1f87`|Furniture add/move/remove, mail, management and upkeep withdrawal; house/crate add/remove bits remain unset|
|5|Co-Owner|`0x1fff`|All defined capability bits|
|6|Owner|implicit|All defined capability bits; cannot be assigned as a named grant|

These are defaults. Saved per-group masks govern the actual operation. A nonzero named-character assignment takes precedence over a guild-rank grant. A rank grant matches that exact current rank; it is not a threshold. Revocation, rank changes, guild departure and leadership transfer are reread from current database authority. Previously sent windows do not authorize later inventory or furnishing mutations. Rent, creator title, custody, item eligibility and route checks still apply even when a capability bit is granted.

Current server authority checks confirmed the native masks for every named group, default visitor restrictions, exact-rank delegation, named precedence, immediate revocation/rank-change behavior and refusal of stale leader or invalid-rank authority. The original Access window saved a named Trustee grant; it remained visible after a genuine camp and login, with the restored hall still placed and its permanent room retained. These server checks do not claim a separate native furnishing/pickup session for every role.

## Leaving and remaining limits

The original client's `/leaverealestate` command returned the Lobby-entered hall occupant to the hall's actual current neighborhood plot. It resolves current ownership, placement and safe ground instead of relying on the last local entrance. Clicking/walking the Standard interior's model door was not established as successful egress in this follow-up.

Direct Shed storage of a furnished house remains unsupported by the current adapter/service and is refused without changing custody. The supported Pack → Unpack to Shed → Load Yard path remains available; this follow-up did not bypass the direct-house storage restriction. The later [interior Access follow-up](INTERIOR-ACCESS-2026-10-19.md) adds inherited ACL editing and a representative native Trustee placement/movement/pickup and revoked Add check, with explicit scope. Additional hall profiles and other native permission-operation sessions remain separate acceptance work. This is representative emulator behavior, not a complete May2013 Live equivalence claim.
