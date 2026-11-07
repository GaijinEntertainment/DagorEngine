# daPCG: deferred work

Collected 2026-10-25: the features that wait for a later phase. Open bugs, perf concerns and code
to remove stay in the side-finding files that AGENTS.md names.

## 1. Deferred features

From the owner's decisions, the [open surfaces proposal](open_surfaces_proposal.md), the
[research](research_procedura_libfive.md) and daSurf spec section 2.

- The daCSG clip of an open surface by a solid, which also gives back dagCSG SLICE_IN and
  SLICE_OUT of an open A (proposal section 6).
- A bulk `scatter(x, maps)` call past about 50k instances (proposal section 8, question 5).
- A uv affine on sheets and a card bent along v (proposal section 5).
- An organic (SDF) leaf meshed by interval-pruned octree dual contouring, behind daCSG's external
  leaf contract, for rocks, cliffs, trunks and roots (research section 2 item 4; roadmap
  section 9); libfive is its reference (README decision 5).
- A `-lint` gate on floaters (research section 2 item 1, section 4 step 1).
- daSurf trim, only when a model needs a hole drawn in UV (daSurf spec section 2): trim by
  (u, v) curves with cut cells and the (u, v) form of paint, `uv_arclen`, per-sheet quality, the
  fuzz test. Done when a trimmed panel stitched to a loft is closed through daCSG on both kernels,
  and the crease test (a loft through squares with rounded and sharp corners) shows hard edges
  only at the sharp ones.
- daSurf spec section 2, later: offset of a shell, subdivision surfaces, IGES or STEP import, an
  interactive editor with iso-lines, control nets, free edges and a zebra view.
