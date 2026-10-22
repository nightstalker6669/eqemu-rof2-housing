# Crate preview handoff, October 19, 2026

After starting Place on a crate member, canceling its preview, and moving the same item to the yard with Shed, the next Place could report that the item was out of sync. The server still held the uncommitted preview with its old crate source.

The crate-transfer handler now clears a move preview for the transferred custody only after the transfer succeeds. Failed transfers retain their retry state, and unrelated item previews are unchanged. No opcode, struct, schema or placement authorization changed.

The zone target built and was deployed to the development server. In the original RoF2 client, a furnished Standard Guild Hall moved from yard storage into the crate, opened a crate preview, returned through Shed, and immediately opened a yard preview without zoning. The hall's permanent interior and retained furniture were conserved. Protected existing properties and items remained unchanged.

At the end of this initial preview-fix batch, the complete furnished guild-hall recovery milestone was pending. Bulk Unpack reported no transferable items; extraction used Shed. Direct placement was not accepted because the client reported player overlap. A subsequent native Load Yard request contained the expected hall placement transform, but the backend safely refused it: the layout house identity check then excluded stored houses. No layout changes committed. Layout restoration of a stored house needs implementation and targeted confirmation before claiming this workflow complete. Normal entry into the recovered hall and post-placement relog were not reached.

No additional housing opcode was discovered in this batch. This change corrects state handling on existing mapped messages. This repository contains the production fix and summary only; private runtime evidence and client layout files are excluded.

Follow-up completed: [stored-house layout restoration](STORED-HOUSE-LAYOUT-2026-10-19.md) restored the same furnished Standard Guild Hall, confirmed normal entry and retained furniture, and passed a post-placement relog. The later [bulk Unpack follow-up](BULK-UNPACK-2026-10-19.md) accepted Unpack into storage followed by Load Layout and normal entry. Manual dragging is not newly accepted.
