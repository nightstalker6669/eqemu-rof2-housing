# HLG native NPC preview envelope

This is a reconstruction of the matching RoF2 client's renderer admission behavior. It is not an additional recovered opcode, a captured Live packet, or a change to the original physical room geometry.

The original native NPC preview checks its animated renderer bounding box against room regions before sending the item request. The later item sender uses a smaller, fixed NPC envelope. Consequently, a physically valid NPC can fail the earlier native check when its animated renderer box crosses a room floor. The server retains its physical placement, floor, ownership and permission checks; the compatibility envelope exists only for transient native preview admission.

## Source identity

| Artifact | SHA256 |
| --- | --- |
| eqgame.exe | `2a8702ad9f722704f01355c0750be7d6f164a8b9c9128ba0cf286ea32b405b0e` |
| EQGraphicsDX9.dll | `ade556b68e98f6f39c5578f70431e11c0b9babcc15b8571eb22cf1e2b654417e` |
| Microsoft d3dx9_30.dll | `5edeed79f2359527a55b8189cfa8b9b121cd608d44eead905a0f3436938ad532` |
| hlg.eqg | `7c98d2226b8c7c6d21fb20224114e0f2bc732f64e541df2a5810b16bc19c0496` |
| hlg.mds | `5cdc9a28fb4660d964c1368af3f79364a3397ea98b5638819b75e4f891f9c7cf` |

The authenticated archive contains one 93-node EQGSv1 model and 13 EQGA clips: 1209 tracks and 28923 keyframes. All members and all frame records were parsed with finite-value and complete-consumption checks. Original assets and decoded animation data are not distributed in this repository. Primary format corroboration used [Quail](https://github.com/nickgal/quail) commit `fdff35ee38ae43ab5d29153dc9c212b779214729`, alongside independent decoding of the original bytes.

## Native behavior recovered

The renderer collision getter is EQGraphicsDX9 `10046ae0`; its hierarchy recomputation is `1004a040`, with recursive node union `100497f0`. The union updates its stored center and extents after every non-root node, requiring 92 separately enclosed updates. The native preview uses this renderer box; eqgame `5b1f90` later substitutes the fixed NPC envelope for the outgoing item request.

EQGS/EQGA loader `1003c960` registers source scale, rotation and translation keys. Strict-zero clips receive the original HLG root-height correction of 3.125. D3DX keyframe GetSRT `43bb8d` uses clamped vector interpolation and normalized, sign-corrected quaternion interpolation. Compression `43c4ff` selects original key records without component quantization; compressed sets use the same GetSRT implementation. The original housing controller constructs exactly two tracks and uses its original 0/1 transition targets, shared transition time and duration, sign alignment, and bind fallback.

The matching runtime selected MatrixMultiply `6110b6`, RotationQuaternion `44252f` and Vec4Normalize `6117fd`. The normalizer uses reciprocal square root followed by one Newton refinement. Its ordinary original-controller error is enclosed by an added quaternion norm of `2^-17`. Matrix construction and multiplication, interpolation, iterative bbox union, visual scaling, inverse visual normalization and cached-getter multiplication are enclosed separately. A single animated frame is not promoted to an all-animation bound.

## Finite bound and scope

Let A bound the norm of the native actor world anchor, and H bound the exact admitted NPC entity height. Before the final placement-corner and region-projection arithmetic, the recovered HLG renderer corner radius is bounded by:

```
B(A) = 83.88973520919154 + 0.0002442745683602869 * A
D(A,H) = B(A) + H
```

The integration must additionally enclose its final placement and projection arithmetic and require `A <= 65536`. The constants include a source-derived conservative root rotation operator, every source animation, the 92 iterative bbox updates, a tiny weighted-mean allowance, and all intermediate rounding in the raw-height-six visual-scale interval `[0.9230765104293823, 1.3000006675720215]`.

This bound is scoped to the matching original housing controller and ordinary finite animation path, raw actor height six, and the authenticated HLG model. It does not assert support for arbitrary D3DX API or clock mutations, external model attachments, unrelated renderer scale changes, or another client build. Unsupported cases must keep their existing guarded behavior. Existing item binding identity and source hashes remain unchanged; envelope metadata is separate from item bindings.
