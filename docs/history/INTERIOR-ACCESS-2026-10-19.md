# Interior Access editing

The native Access editor can now edit the current parent plot from inside a reviewed personal house or neighborhood guild hall. Canonical plot ACLs remain the single authority; no separate room ACL or wire format is introduced.

The handler accepts only the actual occupied room or its current parent ID and resolves the canonical plot. The service separately verifies the live client/session fence and current room-to-parent binding in the same transaction as the ACL write. Current guild/owner identity, Manage permission, allowed grant changes and expected ACL revision remain mandatory. A missing or stale interior proof refuses the request. Historical receipt resolution does not reapply a mutation.

The existing physical yard placement fence is unchanged. A separate optional interior policy is supplied only for ACL Read/Replace; it cannot authorize yard placement or voting changes. The service defaults to refusing an interior source when that policy is absent. Current inherited snapshots are refreshed after success or rejection.

## Legacy Guild Hall quest integration

The ordinary Guild Hall player quest previously ejected all unguilded characters, including named visitors admitted by the neighborhood hall service. The production override in `quest-overlay/guildhall/player.lua` retains the basic-hall guild requirement and delays the version0 instanced visitor check until the loaded housing bootstrap runs. The bootstrap publishes a server-only entity variable only after authenticating the current occupied housing room; missing proof sends the unguilded character back to the Lobby. This is a quest compatibility handshake, not admission or furnishing authority. Every subsequent action still checks current ACL/custody/route state.

The override is separate from the EQEmu source overlay. Review local quest customizations, back up the installed `guildhall/player.lua`, and merge/copy this file into the active quest directory before restarting/reloading quests. Version0 plus an instance number alone does not establish housing authority.

## Acceptance scope

The original client saved Visitor and Trustee parent-plot grants from inside the retained Standard Guild Hall, and removed the temporary Trustee grant through the same editor. An unguilded second character with a reviewed Trustee grant entered through the original house menu, remained in that permanent hall, and placed, moved and picked up one existing chair through the native controls. The original furnishing stayed unchanged and item totals were conserved. The ordinary basic hall continued to eject the unguilded character.

For revocation, the temporary named grant was removed administratively under an exact ACL revision check while the same client remained online. A subsequent native Place request immediately returned the missing Add Item access message; it created no custody or room item. This is a stale-client action check after administrative revocation, not a simultaneous native owner editing session. The temporary Trustee role for the action pass was provisioned administratively.

The generic parent-binding path supports reviewed personal and guild interiors. This native witness used the Standard Guild Hall; it does not certify every personal template, permission operation or cross-zone UI refresh. Server authorization rereads current state; the change does not promise immediate eviction of occupants or refresh of every window in another zone. Direct furnished-building Shed storage is covered by the later [Shed follow-up](DIRECT-HOUSE-SHED-2026-10-19.md). All hall/content variants and original historical Live policy gaps remain outside this interior Access result. No private checks, client layouts, accounts, runtime configuration or game assets are included.

The existing full catalogue refresh can clear an already-open Access list after a save. Close and reopen Manage to reload its current contents; the committed grant and subsequent server authorization do not depend on that display refresh. This follow-up does not claim seamless live refresh of every open management window.
