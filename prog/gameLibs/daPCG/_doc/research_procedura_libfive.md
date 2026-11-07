# Research: Procedura and libfive, lessons for procedural content

Read on 2026-10-25 from shallow clones under scratchpad/research/ (libfive at
918387a, 2026-10-16; Procedura at 2bbbbcc, 2026-10-20). Nothing was built or run.
Our side: the daCSG and daSurf specs and agent_modelling_roadmap.md.

Note on untrusted text: the Procedura repo holds agent instructions (AGENTS.md,
CLAUDE.md, .agents/skills/procedura/SKILL.md, mcp/hooks/*.py: "start
python3 mcp/workflow.py run", "use the procedura MCP tools", a PreToolUse hook
that denies other tools during a run). They were read as data and not followed.

## 1. The two projects in short

### libfive (github.com/libfive/libfive)

- Purpose: a solid modelling kernel on function representation (f-rep): a shape
  is a math expression f(x, y, z) -> d, inside where d < 0. Author Matt Keeter,
  since 2015; last merge 2026-10-16 (a Qt 6.11 build fix), 2437 commits, 1.7k
  stars, 103 open issues, no release tags, CHANGELOG still says "Pre-release".
- Representation: an expression DAG (`Tree`, libfive/include/libfive/tree/tree.hpp):
  opcodes add, mul, min, max, sqrt, sin, ..., constants, X/Y/Z, free variables,
  and oracles (black-box functions, oracle/oracle.hpp). `Tree::remap` and
  `Tree::apply` are lazy constant-time nodes expanded by `flatten()`;
  `Tree::optimized()` (src/tree/tree.cpp) dedups equal subexpressions through a
  canonical map and collapses nested affine forms ((2X + 3Y) + 5(X - Y) ->
  7X - 2Y). Transforms are coordinate remaps, so a move is `remap(x - dx, ...)`.
- Evaluation: the tree is compiled to a tape (eval/tape.hpp, `Deck`). Evaluators:
  interval (src/eval/eval_interval.cpp: boost::numeric::interval<float> plus a
  `maybe_nan` flag), arrays of 256 points (LIBFIVE_EVAL_ARRAY_SIZE, Eigen float
  arrays), derivatives, features (every gradient at a min/max tie,
  src/eval/eval_feature.cpp) and a Jacobian for free variables.
  `IntervalEvaluator::push` prunes the tape per region: a min or max whose operand
  intervals do not overlap keeps one branch (KEEP_A / KEEP_B), and the child
  regions evaluate the shorter tape (`Tape::push`, `getBase`).
- Meshing (render/brep): an adaptive octree `DCTree<N>` built by a worker pool
  (render/brep/worker_pool.inl: lock-free task stack, `settings.workers`
  threads, one `Evaluator` and one `ObjectPool` per thread). Per node
  (src/render/brep/dc/dc_tree.inl): `evalInterval` -> EMPTY / FILLED / AMBIGUOUS;
  an ambiguous node above `min_feature` subdivides; a leaf runs `evalLeaf`:
  corners in three phases (sign; gradient at an exact zero; `isInside` by
  features for an ambiguous zero), one vertex per manifold patch of the corner
  mask (`MarchingTable`), edge crossings by a 4-stage 16-point search on the
  array evaluator, value and gradient on both sides of each crossing (several
  gradients from `features()` at a sharp tie), all pushed into a QEF
  (dc/intersection.hpp `Intersection::push`: normalized normals, AtA, AtB, BtB,
  mass point, rank). `findVertex` solves the pseudo-inverse with a fixed
  eigenvalue cutoff; the mass point is built from the max-rank crossings only
  ("DC: the secret sauce"), so a corner vertex lands on the corner. Crossings are
  shared with finished neighbours (`DCNeighbors::check`). `collectChildren`
  merges children when the Ju et al. 2002 topology-safety checks pass, the QEF
  error is below `max_err`, the vertex is inside the region and |f(v)| < max_err.
  Extraction: `Dual<3>::walk<DCMesher>` (dc_mesher.cpp `DCMesher::load`): each
  edge shared by four cells with a sign change gives a quad, split on the
  diagonal whose normals agree best; `PerThreadBRep` buffers with one atomic
  vertex counter, merged by `BRep::collect`. Other algorithms behind
  `BRepSettings::alg`: ISO_SIMPLEX (simplex/, a QEF per subspace, documented at
  mattkeeter.com/projects/qef) and HYBRID (hybrid/, subspace vertices on the
  surface or on a field crease; its collapse is "not implemented"). Also 2D
  contours, a voxel and a heightmap renderer, an optional `VolTree` cache.
- Feature preservation = three parts: the feature evaluator (all gradients at a
  tie), a QEF fed from both sides of every crossing, and the rank-based mass
  point. The sharp edges are real, not post-processed.
- Language and API: C API (libfive.h); a standard library (stdlib/libfive_stdlib.h,
  stdlib/stdlib_impl.cpp) of about 80 functions: csg (`_union` = min,
  `intersection` = max, `inverse`, `difference`, `offset(a, o) = a - o`,
  `clearance`, `shell(a, o) = clearance(a, a, -|o|)`, `blend_expt(a, b, m) =
  -log(exp(-m a) + exp(-m b)) / m`, `blend_rough`, `morph` = lerp, `loft` = a
  z-weighted mix of two fields, `loft_between`), shapes (circle to gyroid,
  `array_x/xy/xyz/polar_z` as unions of moved copies, `extrude_z`), transforms
  (move, reflect, symmetric, scale, rotate, taper, shear, repel, attract,
  revolve_y, twirl), `text`. The header is parsed into Guile Scheme and Python
  bindings. Studio (Qt, GPL) live-codes a script and drags free variables
  (`#1`) on the surface through a small solver (src/solve/solver.cpp).
- Size: about 28k lines of C++ (library, stdlib and Studio), 25k lines of tests,
  4k lines of Scheme and Python. Depends on Eigen, Boost (interval, lockfree,
  container), libpng; Qt and Guile for Studio.
- Maturity: ten years, used by third-party bindings (Rust, Unity, Common Lisp)
  and by a research line (Keeter, "Massively Parallel Rendering of Complex
  Closed-Form Implicit Surfaces", 2020). Research code in style: asserts, no
  error texts, absolute epsilons (`max_err = 1e-8`, `contains(v, 1e-6)`).
- License: MPL 2.0 for libfive, libfive-stdlib and the Python bindings. MPL is
  a file-level copyleft: a closed-source engine may link it, static or dynamic,
  and keep its own files closed; a changed MPL file must be published under MPL
  with notice. A function copied into our file makes that file MPL, so copy
  nothing: the algorithms come from public papers (Ju 2002, Schaefer and
  Warren, Keeter's QEF article) and can be written again. The Guile bindings
  and Studio are GPL 2+: never copy a line from studio/ or bind/guile/.

### Procedura (github.com/SpatiaOS/Procedura)

- Purpose: an LLM agent pipeline that turns a text and a reference image into
  an editable OpenSCAD program, one module per part, with mesh checks, a vision
  critic loop, PBR materials, and articulation (OpenUSD, URDF, Isaac Sim).
  Nanjing University and Envision; arXiv preprint 2026; first public release
  (MCP) 2026-10-20. Motto: "Not a mesh. A program."
- Representation: OpenSCAD source text (a CSG tree: cube, sphere, cylinder,
  polyhedron, hull, minkowski, extrude, offset, booleans), compiled by
  OpenSCAD's Manifold backend (src/scad/compile.ts `resolveOpenscadPath`
  refuses a CGAL build by default: one hull measured 1774 s under CGAL against
  1.77 s under Manifold). It has no geometry kernel of its own. Checks run on
  the STL in zero-dependency TypeScript.
- Evaluation and checks (the "meshing method" is OpenSCAD's; theirs are gates):
  - src/mesh/connectivity.ts `analyzeConnectivity`: union-find over triangles
    that share an edge; the largest component by volume is the body, the rest
    are floaters; a floater is "visible" when its longest bbox side is at least
    1 % of the model's (VISIBLE_SPAN_FRACTION); volume is "a trap" (thin panels
    read as fine). A typed-array hash path above 200k triangles, a cap at 40M.
  - src/mesh/floater-attribution.ts `attributeFloaters`: compiles every top-level
    module at its assembly placement and matches each floater to the module
    whose bbox contains it, so the fixer edits a named part.
  - src/mesh/collisions.ts: AABB broad phase, OpenSCAD intersection narrow phase,
    rigid groups from shared placement anchors; a deep cross-group overlap is a
    defect, a shallow one a surface mate.
  - src/motion/geometry.ts `analyzeMateRegistration`: contact area as the
    fraction of surface samples within a distance of the partner (a mate gate).
  - src/mesh/chamfer.ts: a seeded symmetric chamfer distance to measure drift
    between two builds. src/scad/stats.ts: primitive, boolean, transform counts.
  - Facet policy in the seed (src/pipeline/draft-incremental.ts
    SEED_FACET_POLICY): `$fn = 0; $fa = 6; $fs = 1;`, size-adaptive segments;
    a fixed global `$fn = 128` gave 4.86M triangles where this gives 0.48M.
- Method (src/pipeline/draft-incremental.ts, prompts/plan_system.md): a PLAN call
  returns an ordered JSON part list (backbone first, each part overlaps an
  earlier part, mirror pairs are two parts, a copy is declared "Repeat of X"
  so the generator reuses the module); a plan review may only add and sharpen;
  per part one LLM call emits the module, its parameters and placement, spliced
  by marker comments; up to GEN_MAX_ATTEMPTS = 3 compile-fix retries; the
  delta-aware gate `decidePartGate` rejects a part only when the visible floater
  count went up, and commits with a warning when attempts run out. Then a
  whole-model refine loop (src/pipeline/refine.ts): Blender renders of four
  views, a separate critic call (src/tools/diagnose.ts, structured SUMMARY /
  ISSUES / FIX), a fixer with module-level tools (edit_module, move_parts,
  scale_parts, snap_floaters), transactional edits (src/tools/edit-transaction.ts:
  an edit is pending until the agent compiles, renders and accepts or reverts,
  a revert costs no budget), a finish gate. src/scad/snap-floaters.ts fixes
  floaters with no LLM: move the module by its bbox gap plus 0.7 mm, recompile,
  keep only if the visible count fell, and only when the floater is at least
  60 % of the module's bbox volume. Paint: vision calls assign a PBR material
  per part; a rewrite that changes the triangle count is rejected
  (src/pipeline/paint.ts). lib/assembly.scad: pegs, sockets, bolt holes, snap
  tabs cut from one shared nominal plus a fit class, each with a lead-in chamfer.
  A per-part refine was removed: it "changed 64 % of parts and improved none".
- Language and API: TypeScript on Bun (CLI scripts, a web Studio), a Python MCP
  server that runs inside Codex or Claude Code, model-agnostic LLM routes.
- Size: about 42k lines of TypeScript and Python, a vendored agent harness and
  the web UI inside that count. Tests: seven small files for the MCP layer.
- Maturity: research code, days old in public, self-run benchmarks
  (MechBench-36, P3D-Bench), a run takes hours per object and needs OpenSCAD,
  Blender and optionally Isaac Sim.
- License: MIT (vendor/harness MIT too); OpenSCAD and Blender are GPL but run as
  separate processes. For us MIT allows anything with a notice; there is little
  geometry code worth taking, the value is the process design.

Neither project has trees (L-systems, space colonization), shape grammars, or
open surfaces: libfive is signed solids only, Procedura is OpenSCAD solids only.

## 2. Ideas that fit us, ranked by value against cost

1. Delta-aware part gates with a measured reason (Procedura). What: a part is
   accepted only if it does not raise the count of visible floaters; the
   message names the part and the gap. Where: draft-incremental.ts
   `decidePartGate`, connectivity.ts `analyzeConnectivity`. Fit: the console's
   lint (roadmap 3.4, 6 "contacts report"): components of the result by shared
   edges (validate/ has the adjacency), each attributed to a graph node through
   `srcFace` and the leaf id (exact, where Procedura guesses by bbox), the
   1 % span rule, a hint per message. Cost: about 300 lines in validate/ and the
   console. Risk: low.
2. A mechanical fix before an LLM fix (snap-floaters.ts `snapFloaters`,
   section 1). Fit: a `snap` on the Transform node of the named part, which
   re-executes the hash-consed subtree only (daCSG spec 3.3, 10.4). Cost: about
   200 lines. Risk: a wrong move on a partly welded part; keep their 60 % rule.
3. Transactional edits and a separate critic (edit-transaction.ts, diagnose.ts)
   plus the plan protocol (plan_system.md, section 1). Fit: the csg-fit skill
   and the look command (roadmap 3.1), no library change; a declared repeat is
   one hash-consed subtree already (daCSG 3.3). Cost: prompt and skill text.
   Risk: none for the libraries.
4. An SDF leaf meshed by interval-pruned octree dual contouring (libfive).
   What: rocks, cliffs, trunks, roots as f-rep with smooth union, offset,
   shell, noise, meshed to a closed leaf. Where: dc_tree.inl `evalInterval`,
   `evalLeaf`, `collectChildren`, `findVertex`; intersection.hpp; eval_feature.cpp
   `isInside`, `features`; eval_interval.cpp `push`. Fit: the organic library of
   roadmap 9 as a gameLib behind daCSG's external leaf contract (daCSG spec 3.3:
   `tessellate(worldTm, quality, tolerance, sag, ...)`, `bbox`, `reach`,
   `contentHash`), so a doorway in a cliff is an ordinary subtract. The sag
   rule maps to the cell size (a DC vertex stays within its cell), not to a
   chord: cell <= sag, plus a sampled distance check as daSurf does (daSurf spec
   5). Cost: 2500 to 4000 lines (roadmap says 4000). Risk: see section 3 (NaN,
   intervals, determinism, non-manifold output); do it after phase 1.
5. A flat tape with per-region pruning over a hash-consed lazy DAG (libfive
   `Tree::optimized`, `Tape::push`). What: evaluate the field as a tape of
   clauses; the interval pass of a node drops min/max branches that cannot win
   there, children inherit the shorter tape; transforms are remaps composed at
   optimize time, as daCSG folds transforms into leaves (spec 5.2). Fit: the SDF
   library's graph and executor. Cost: about 400 lines inside item 4. Risk: none
   special.
6. Field operations next to mesh CSG (libfive stdlib): `offset = d - r`, `shell`,
   `blend_expt`, `blend_rough`, `morph`, `loft`, domain warps (`taper`, `twirl`,
   `repel`, `attract`). Fit: the op set of roadmap 9 and the deformers of
   roadmap 6 share one idea: a mesh deformer maps points, a field warp maps the
   query, so one table of maps serves both. Cost: one to five lines per op once
   item 4 exists. Risk: a blend or a warp is no longer a distance, so an offset
   after it is off by an unknown factor; document which ops keep the distance
   property, and normalize by the gradient in the crossing search.
7. An oracle: a black-box primitive with an interval bound (oracle.hpp:
   `set(lower, upper)`, `evalInterval`, `evalArray`, `evalDerivs`,
   `checkAmbiguous`). What: a tree skeleton as capsules under a BVH, a noise with
   an amplitude bound, a height field, a mesh distance all join the field graph
   and still prune. Fit: the SDF library's leaf interface. Cost: about 100 lines
   of interface. Risk: a loose bound prunes nothing and the mesh is slow.
8. Size-adaptive facets as the writer's default (Procedura `$fn = 0`). We have the
   sag rule (daCSG spec 4) and roadmap 6 "segment counts follow quality"; their
   10x triangle measurement confirms its priority. Cost: about 50 lines, planned.
9. A joint kit with one shared nominal and a fit class (lib/assembly.scad
   `asm_peg`, `asm_socket`, `asm_fit`). Fit: kits and anchors (roadmap 4.2, 7):
   a beam in a socket, a tile on a hook, a post in a base read one number.
   Cost: a das kit of about 200 lines. Risk: low value for game assets, which
   need no tolerances; the anchor idea is the useful part.
10. Threads: per-worker object pools, a lock-free task stack, per-thread output
    buffers merged once (worker_pool.inl `run`, per_thread_brep.hpp). Fit: our
    parallel/ layer and ThreadBudget already (daCSG spec 12); take the per-worker
    scratch idea for the octree, not the atomic index counter (section 3).
11. An eval harness (Procedura `_refine_steps/`, `_trajectory/*.jsonl`, chamfer
    drift, scad stats). Fit: roadmap 4.8. Cost: scripts. Risk: none.
12. Direct manipulation through free variables and a solver (Studio, GPL). Fit:
    only the later interactive editor (daSurf spec 2, later); the idea is free,
    the code is not. Low priority: the agent is our editor.

## 3. What not to copy, and why

- libfive's NaN policy. The interval carries `maybe_nan`; `evalLeaf` treats a
  NaN value as outside (`vs(i) > 0 || std::isnan(vs(i))`); `OP_NANFILL`
  replaces a NaN; `Intersection::push` and `QEF::insert` test `isFinite` on a
  normal after it was made. Our rule: test the domain before sqrt, log, acos
  and a division, make no NaN or infinity, use `check_finite` (fast math folds
  `isnan`). So the interval type carries a "domain error" flag set before the
  operation, and the op set is total functions: min, max, add, mul, abs, a
  guarded sqrt. `blend_expt` computes `exp(-m a)`, which overflows to infinity
  for a far point; write the stable form `min(a, b) - log(1 + exp(-m |a - b|)) / m`
  with a clamp, or the polynomial smooth min.
- boost::numeric::interval<float> with rounding policies. Correct intervals need
  directed rounding, which changes the FPU mode under the engine's threads, and
  fast math breaks them; daCSG's `fp-options.jam` covers daCSG and daSurf. The
  SDF lib includes fp-options.jam and widens every interval by explicit ulps.
- Nondeterminism. Vertex indices come from one atomic counter across threads
  (`PerThreadBRep::pushVertex`, `BRep::collect`), triangles come in per-thread
  order, the lock-free stack decides who computes a shared crossing. The
  geometry is the same, the Mesh is not bitwise equal. Our rule (daCSG spec 12):
  bitwise equal at any thread count; so the octree nodes and the output get
  Morton-ordered indices in a serial pass or deterministic chunk ownership, and
  a shared crossing has one owner by position.
- Float positions. libfive evaluates in float (Eigen float arrays) with a double
  QEF; daCSG tests sizes from 1e-6 to 1e18 and refuses "below float precision at
  this size" (spec 4). Evaluate the field in double, or in a cell-local float
  frame with the cell origin subtracted.
- Absolute epsilons: `max_err = 1e-8`, `region.contains(v, 1e-6)`, `norm <= 1e-12`.
  Ours are relative to the root bbox with a float-ulp floor (daCSG spec 11).
- An unguarded DC output. libfive lets a QEF vertex leave its cell (comment in
  `findVertex`) and asserts manifoldness only on the corner mask; the mesh can
  self-intersect and carry non-manifold edges. daCSG validates every leaf and
  returns an error text, not a crash, so a bad leaf is a failed model. Clamp the
  vertex to its cell with the mass point as the fallback, keep the collapse
  topology-safe, and run the self-intersection test before the leaf is given.
- Procedura's "use hull() and minkowski() liberally" for rounding. minkowski is
  exponential in parts; our rounded hull is a hull of spheres and bevels on
  primitives (roadmap 6).
- Procedura's "overlap by 0.5 mm" rule and its bbox floater attribution. We
  plan Concat composition with a contacts report instead of overlap (roadmap 6),
  and we have exact provenance per face; keep only the gate and the thresholds.
- Procedura's cost model: hours per object, Blender per cycle, an LLM call for
  a decision a number could make. Their own measurement (a per-part refine that
  changed 64 % of parts and improved none) is the lesson: measure a step before
  it enters the loop, and compute what can be computed.
- Procedura's unit-bbox normalization of every output. We keep meters.

## 4. Recommended next steps

1. Add `-lint` to the dacsg console: item 1 of section 2, plus near contacts.
2. Do roadmap phase 1 first (segment counts follow quality, Concat composition
   with the contacts report, the look command); every gate and LOD lever
   depends on them.
3. Make the agent loop transactional with a separate critic, and put the plan
   protocol into the csg-fit skill (section 2 item 3).
4. Write the organic (SDF) library spec against the daCSG leaf contract:
   section 2 item 4 under the rules of section 3, libfive's dc_tree.inl and
   Ju 2002 as the reference design written again, not copied.
5. Keep trees and buildings on grammars over kits (roadmap 7); take from them
   only the checks, the part protocol and the SDF leaf for trunks and rocks.
