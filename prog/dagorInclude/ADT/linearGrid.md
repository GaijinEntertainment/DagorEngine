---
name: linearGrid
description: "Deep reference for LinearGrid (RiExtraGrid) -- density-adaptive spatial index: layers, packed leaf trees, build pipeline, query iterators, config and diagnostics"
metadata:
  node_type: memory
  type: reference
  authored: 2026-07-27
  commit: ad01570053e6
---

# LinearGrid -- Complete Architecture Reference

`prog/dagorInclude/ADT/linearGrid.h` is a header-only spatial index for large sets of
static-ish world objects. Its only production instantiation today is `RiGrid`
(`LinearGrid<RiGridObject>`), used as the `riExtraGrid` singleton in
`prog/gameLibs/rendInst/rendInstGenExtra.cpp` to answer all RIExtra collision and gather
queries (box / sphere / capsule / oriented box / ray).

## The problem it solves: extreme density variance

The structure exists because object density in our open-world scenes varies by three orders
of magnitude *within one level*, and no single-resolution structure handles that:

- large empty or near-empty stretches of terrain: a fine grid wastes memory on nothing;
- sparse regions holding only a few big objects (rocks, cliffs, trees): a fine grid is pure
  overhead, and the objects overhang many cells;
- building interiors packed with furniture, dishes, garbage and small decorations, sometimes
  500-1000 pieces inside a single 8x8 m footprint: a coarse grid degenerates into a linear
  scan of a thousand objects per query;
- multi-story buildings and gravitational anomalies, so those thousand objects are also
  stacked vertically, which rules out any purely 2D subdivision at the bottom level.

The answer is three layers that each absorb one of those cases (coarse XZ cell array ->
optional 8x8 subgrid only where small objects pile up -> a packed BVH inside each cell that
handles the vertical stacking), plus a separate list for the handful of objects too large for
any of them. Everything else in this document follows from that, plus these constraints:

- query cost matters far more than update cost;
- memory per object must stay tiny (no per-object node);
- an object lives in exactly one place (no duplication into overlapping cells).

Measured on our heaviest scene (`rigrid stats`):

```
Total objects: 1125105 (81.8% small) [8.6 Mb]
Cells: 75x98=7350 (4.8x6.3 km); Empty: 9 (0.1%) [229.7 Kb]
Leafs: 108588; Branches: 22131 (20.4%) [5344.2 Kb]
SubGrids: 1001; (13.6% of cells) empty subCells: 39.3% [2331.0 Kb]
Main ext: 38.1 37.9 35.5 33.1; Sub ext: 3.0; Max ov.rad: 327.33
Max leaf depth: 8 (avg. 2.83) Split fails: 10 (max 59)
Oversize objects: 34, cover 27 cells (0.37%)
Total memory: 16.30 Mb
```

How to read that, because it is the best summary of the design in practice:

- 1.1M objects indexed in 16.3 Mb total, about 15 bytes per object including all cells,
  trees and bitvectors;
- 81.8% of objects went to subgrids, but only 13.6% of cells needed one: the dense-interior
  case is common in object count and rare in area, exactly the asymmetry the optional
  subgrid layer targets;
- average leaf depth 2.83 and max 8: trees stay shallow because the two grid layers already
  did most of the partitioning, so the BVH only has to resolve the remaining local (largely
  vertical) overlap;
- `Main ext` around 33-38 m against a 40 m `configMaxMainExtension` budget: the extension
  trick is running right at its configured limit, which is what keeps query boxes from
  growing further;
- 34 oversize objects (up to 327 m radius) are excluded from the grid entirely. Without that
  escape hatch those 34 objects would push `Main ext` to 327 and widen *every* query on the
  map by that much;
- 10 split fails out of 22131 branches: the fallback path in `reCreateBalancedLeaf` almost
  never gives up, and when it does the affected leaf holds at most 59 objects.

## Layers

```
LinearGrid                              // layer 0: dense 2D array of main cells (XZ, unbounded Y)
  cells[gridWidth * gridHeight]         //   LinearGridMainCell, 32 B each, cellSize (default 128 m)
    rootLeaf  -> leaf tree              // layer 2: per-cell BVH over "large" objects
    subGridIdx -> LinearSubGrid         // layer 1: optional 8x8 subdivision of one main cell
      cellsData[64]                     //   LinearGridSubCell, 32 B each, cellSize/8
        rootLeaf -> leaf tree           // layer 2 again: per-subcell BVH over "small" objects
  oversizeObjects[]                     // flat list, brute-forced on every query
```

Each layer absorbs one density regime: the main array is cheap enough to leave mostly empty
over open terrain, the subgrid is allocated only for cells that actually accumulated small
objects, and the leaf tree is the only part that subdivides in Y (multi-story interiors,
anomalies).

Four containers, all indexed by `leaf_id_t` / offsets rather than pointers:

| member | contents |
| --- | --- |
| `cells` | dense main-cell array, row-major, `gridSize` (IBBox2, in cell coords) gives its origin |
| `leafs` | flat pool of 16-byte `LinearGridLeaf` slots shared by *all* trees of all cells |
| `branches` | `eastl::bitvector`, one bit per leaf slot: is this slot a branch node |
| `subGrids` | vector of owned `LinearSubGrid *` (2064 B each), referenced by `int16_t subGridIdx` |

`cellsData` / `leafsData` are cached raw pointers into `cells` / `leafs`, refreshed on resize.

## Object contract

`ObjectType` must be <= 8 bytes (`static_assert`) and is always passed by value. It must
provide `getWBSph()` (vec4f `xyz` = world bsphere center, `w` = radius), `getWBBox()`
(bbox3f), `operator==`/`<`, `static null()`, and `getDebugName()`. `RiGridObject` is a bare
`riex_handle_t`: both accessors recompute the sphere/box from `rendinst::riExtra` pools, so
the grid stores no transform of its own.

The grid is therefore a pure index. It never owns or copies object geometry, and any
external change to an object's transform must be reported through `update()`.

## Cell layout trick: metadata inside the bbox

`LinearGridMainCell` and `LinearGridSubCell` are both exactly `sizeof(bbox3f)` (32 B,
`static_assert`ed) and are laid out as `Point3 bboxMin; <4 bytes>; Point3 bboxMax; <4 bytes>`.
The two 4-byte holes are the `w` lanes of the two vectors and hold the metadata:

- main cell: `rootLeaf` (leaf id) and `subGridIdx` + `changes`;
- sub cell: `rootLeaf` and `empty` + `changes`.

`setBBox()` uses `v_perm_xyzd` so storing a box preserves those lanes, and
`getBBoxRefUnsafe()` hands out the 32 bytes as a `bbox3f &` with the metadata still in `w`.
That is safe because every box operation on it only consumes/produces `xyz`.

The cell box is the union of the boxes of the objects assigned to that cell, not the
geometric cell rectangle. It is the first rejection test of every query and is exact in Y
(cells are unbounded vertically).

## Assignment and the extension trick

An object is assigned to a single cell, chosen by its bounding-sphere *center*
(`calcCellIds`: `floor(pos) >> cellSizeLog2` on X and Z). Its box may stick out of that
cell. Instead of duplicating the object into every overlapped cell, the grid records how far
objects stick out and grows the *query* box instead:

- `calcMaxExtension()` returns the 4-component overhang of an object box past its cell
  rectangle as `[-X, -Z, +X, +Z]`;
- `maxMainExtension` / `maxSubExtension` accumulate the maximum over all inserted objects
  (per direction for the main layer, a single scalar for subgrids);
- query construction (`LinearGridBoxIterator` ctor, `LinearGridRayIterator` ctor) expands the
  query box by the *opposite* extension component before converting it to a cell range:
  `mainBox.bmin -= perm_zzww(maxMainExtension)`, `mainBox.bmax += perm_xxyy(maxMainExtension)`.

Only X and Z of that widened box are ever read (`getClampedOffsets` uses
`v_perm_xzac(bmin, bmax)`), so the incidental Y widening is dead. The unwidened `queryBox` is
what leaf and object tests use, so widening only costs extra cells visited, never false hits.

`getClampedOffsets` clamps the resulting cell range into the grid, so a query fully outside
the grid still scans a degenerate in-grid range; the cell-box tests reject it.

**Oversize objects.** If any extension component exceeds `configMaxMainExtension` the object
is not put in the grid at all: it goes into `oversizeObjects` as a `LinearGridPosObject`
(object + cached `wbsph`) and is brute-forced at the start of every query. This keeps
`maxMainExtension` from being inflated by a handful of huge props, which would otherwise
widen every query on the map. `maxOversizeRad` bounds the 2D pre-filter used on that list.

## Leaf trees

Below the cell level each cell holds a binary BVH built out of 16-byte `LinearGridLeaf`
slots. The type is a union discriminated by the `branches` bitvector, never by a tag field:

- **lnode** (leaf): a `dag::Vector<ObjectType, MidmemAlloc>` of objects. 16 bytes for
  pointer + size + capacity.
- **bnode** (branch): two *consecutive* slots (32 bytes total), each holding
  `float packedCorrection[3]` + `leaf_id_t idx`. The first slot carries the min corrections
  and the left child index, the second the max corrections and the right child index. Both
  slots get a bit in `branches` (hence `getBranchesCount()` divides by two).

Child boxes are not stored; they are *derived from the parent box*. Given two sibling boxes
whose union is the parent box, at least 6 of their 12 face planes coincide with the parent's.
`pack_bboxes()` stores only the 6 that differ, as offsets of the inner intersection box from
the parent, and puts the "which sibling owns this offset" flag in the sign bit
(32-byte variant) or the low bit (16-byte variant). Unpacking is branchless
(`unpack_bboxes_32b`): one sibling gets the offset, the other keeps the parent plane.

Consequence: a branch node's boxes are meaningless without its parent box, so every
traversal threads the parent box down the recursion, and any change to a cell box requires
re-packing the whole tree under it (`leaf_repack_bboxes`). The comment in `insertAt` spells
out the trap: growing the parent box grows the packing step, which can silently *shrink*
unpacked child boxes.

`EXTRA_SMALL_BRANCHES_AND_64K_LEAFS_LIMIT` (off) switches to 16-byte branches with
uint16-quantized corrections and `uint16_t` leaf ids, halving branch memory at the cost of
box precision and a 64K leaf cap.

## Build pipeline

Insertion has two modes, selected by the `initial` flag threaded from
`rendinst::addRIGenExtra(..., on_loading)`:

1. **Loading.** `insert(..., initial = true)` on a not-yet-optimized cell just appends to the
   cell's single flat lnode vector. No tree, no split, no box packing.
2. **Optimize.** `optimizeCells()` (called from `rendinst::optimizeRIGenExtra()`, which
   `rendInstGen.cpp` fires when the last RIGen cell-load job completes) turns those flat
   vectors into the real structure. On the first pass it moves `leafs` aside and rebuilds the
   whole pool from scratch so leaf indices come out in cell order (cache locality), then per
   cell calls `refillCell()`. Later passes only touch cells failing `isCellOptimized()`.

`refillCell()` per cell:

- sort objects by handle (`stlsort::sort_branchless`) for deterministic, cache-friendly order;
- cells small enough for a single flat leaf and too small for a subgrid return right there,
  without computing any object box;
- if there are at least `configObjectsToCreateSubGrid` objects with radius
  <= `configMaxSubExtension`, allocate a `LinearSubGrid` and move every object whose radius
  <= `configMaxSubRadius` *and* whose sub-cell extension <= `configMaxSubExtension` into the
  matching sub cell, via a counting sort that leaves each sub cell's objects contiguous;
- build a balanced tree over what is left in the main cell, and one per non-empty sub cell,
  via `reCreateBalancedLeaf()`.

So the subgrid is not a uniform refinement: it is a "many small objects" fast path. Large
objects stay on the main layer precisely because they would overhang many sub cells and blow
up `maxSubExtension`.

### Splitting (`reCreateBalancedLeaf`)

Recursive, stops at `configMaxLeafObjects`. Two strategies:

1. **Center split (cheap, tried first).** For all three axes at once (SIMD), classify each
   object by whether its box leans below or above the parent box center, then score
   `|count_below - N/2| * sideOversizeCoef`, where `sideOversizeCoef` penalizes splitting the
   long axis of a flat cell. Axes are disqualified if one side is empty or if the worst box
   overflow past the center exceeds `configMaxSplitOverflow` (20%) of the box size, i.e. the
   split would produce heavily overlapping siblings.
2. **Best half cut (fallback, `findBestHalfCut`).** Per axis, sort all box mins and all box
   maxs and sweep candidate planes, maximizing `min(objects fully left, objects fully right)`
   so that siblings do not overlap at all. Runs the three axes on a `threadpool::JobPool` when
   N >= 4000.

If both fail to produce a non-empty split the node stays a flat lnode above
`configMaxLeafObjects`, `OptimizationStats::splitFails` is bumped (visible via `rigrid stats`),
and the leaf is recorded in `unsplittableLeafs` so `needCreateBranch` stops proposing the same
doomed split on every later insert. That mark describes one set of objects and is dropped when
the leaf is refilled or freed, so the periodic `leaf_rebuild` is where a leaf that has since
grown enough gets another attempt.

`reCreateBalancedLeaf` takes objects and boxes as two index-parallel spans and partitions both
in place, so a box lookup is `boxes[i]`. The partition is a Hoare-style swap and therefore not
stable; leafs sort their objects when they are filled. Every split decision is
order-independent (per-element masks, box unions, and `findBestHalfCut` sorts its own
coordinates), so the resulting tree does not depend on that ordering.

## Runtime mutation

`insert` / `erase` / `update` (`update` = `eraseAt` + `insertAt`, both with
`change_weight = 0`) take the world bsphere and box, derive main and sub cell ids, and:

- `insertAt`: reject to `oversizeObjects` if too big, `grow()` the cell array if the cell is
  outside the current `gridSize`, extend the cell box, then either hand the object to the
  subgrid (if one exists and the sub extension is small enough) or to
  `leaf_insert_object()` on the main tree;
- `leaf_insert_object` descends into whichever child box already fully contains the new box;
  if neither does, `leaf_insert_object_to_better_branch` picks the child whose growth adds
  the least sibling-overlap volume (tie-break: least surface growth), repacks this node
  against the grown parent box, and recurses;
- `leaf_erase_object` walks the tree linearly and removes from the lnode vector.

**Incremental splitting only exists in dev builds.** `needCreateBranch()` requires
`DAGOR_DBGLEVEL && optimized && count > configMaxLeafObjects` and that the leaf is not marked
unsplittable, so in release an lnode grows without bound during runtime inserts. Quality is instead restored by the rebuild counter:
each cell/sub cell accumulates `changes`, and at `configChangesBeforeRebuild` (64, or
immediately when a leaf empties) `leaf_rebuild()` frees the whole subtree, re-sorts the
objects and rebuilds it balanced. Freed slots go to `freeLeafs` and are reused by
`createLeaf()`, which in 32-byte-branch mode must match branch-ness because a branch needs a
consecutive pair.

`clear()` is full teardown (level change) and releases capacity; it also has to
`destroy_at()` the `objects` vector of every non-branch slot by hand, since the union has no
destructor.

## Queries

Entry points are `getBoxIterator(bbox, with_extension)` and
`getRayIterator(from, dir, len, radius, with_extension)`; both return an iterator whose
`foreach(objects_iterator)` does the work. `prog/gameLibs/publicInclude/grid/gridImpl.h`
supplies the `objects_iterator` implementations (one per query shape: box by pos, box by
bounding, sphere, capsule, oriented box, ray) and `riGrid.cpp` instantiates them as the
`rigrid_find_*` functions, which are `DAGOR_NOINLINE` so the templates are compiled once.

The `objects_iterator` concept as consumed by LinearGrid:

| member | role |
| --- | --- |
| `filterFunc(obj, wbsph)` | cheap pre-filter (pool id, min/max radius) |
| `checkBoxBounding(...)` | query shape vs cell or branch box |
| `checkObjectBounding(...)` | query shape vs object bsphere |
| `predFunc(obj)` | user callback; returning `true` stops the whole query |
| `isCapsule()` | ray queries only: extend branch boxes by the ray radius |

`checkBoxBoundingInside` and `checkFourObjectsBounding` exist in `gridImpl.h` but are unused
by LinearGrid (they serve the spatial-hash grid).

All queries are "find first": iteration stops as soon as `predFunc` returns true and the
object is returned; collectors push into an output array and return `false` to keep going.
`ObjectType::null()` is the not-found sentinel.

Order of work in `foreach`:

1. brute-force `oversizeObjects` with a 2D point-in-rect pre-filter (`fast_pos_check`, query
   box widened by `maxOversizeRad`);
2. iterate main cells in the clamped range; per non-rejected cell, traverse `rootLeaf`, then
   the subgrid if present;
3. subgrid ranges are intersected against the main cell before descending, because the main
   cell range was widened by `maxMainExtension` and the sub range only by `maxSubExtension`.

`leaf_iterate_intersected` is the shared tree walk, templated on `bbox3f` vs
`const LinearGridRay *` so box and ray traversal share one function.

### Two cell walkers for rays

- `LinearGridDirectedRayIteratorImpl`: walks the full axis-aligned cell rectangle of the ray,
  in ray direction (`limits` are swapped per axis by sign so the near cells come first,
  giving early exit on hit). Cheap per cell, but scans the whole rectangle.
- `LinearGridWooRayIteratorImpl`: same row-major walk, but each cell first gets a 2D
  segment-vs-rect test against the cell rectangle extended by the ray radius
  (`checkWooIntersection`), and once a row has been entered and then left, the rest of the row
  is skipped (`rowIntersectionFound` -> jump `x` to the row end). This trims the corridor down
  to roughly the cells the ray actually crosses.

`shouldUseWooRay()` picks the second only when both the X and Z spans are >= 4 cells, i.e.
when the rectangle is large enough that per-cell rejection pays for itself. The choice is made
independently for the main layer and for each subgrid.

Cell rectangles for the Woo test are reconstructed arithmetically, not stored:
`lowestCellBox` is the XZ box of the grid's minimum cell, and cell `n` is
`lowestCellBox + (n << cellSizeLog2)`.

## Configuration

Public fields on `LinearGrid`, defaults in the header, overridden from the `riExtraGrid`
block of gameparams (`init_ri_extra_grid` in `rendInstGenExtra.cpp`):

| field | default | blk key | meaning |
| --- | --- | --- | --- |
| cell size | 128 | `cellSize` | main cell edge, must be pow2, set only while empty (`setCellSizeWithoutObjectsReposition`) |
| `configMaxMainExtension` | cellSize*0.625 | `maxMainExtension` | overhang above which an object becomes oversize |
| `configMaxSubExtension` | 3 | `maxSubExtension` | overhang above which an object stays on the main layer |
| `configMaxSubRadius` | 4 | `maxSubRadius` | radius above which an object stays on the main layer |
| `configObjectsToCreateSubGrid` | 200 | `objectsToCreateSubGrid` | small-object count needed to allocate a subgrid |
| `configMaxLeafObjects` | 40 | `maxLeafObjects` | lnode capacity before splitting |
| `configReserveObjectsOnGrow` | 4 | `reserveObjectsOnGrow` | lnode vector growth step |
| `configMaxSplitOverflow` | 0.2 | - | max sibling overlap accepted by the center split |
| `configChangesBeforeRebuild` | 64 | - | mutations per cell before `leaf_rebuild` |

Enlisted and active_matter both ship `cellSize 64` / `maxMainExtension 40` (the value the
header comment calls the tuned pair).

## Limits and invariants

- `sizeof(ObjectType) <= 8`, and it must be relocatable; `riGrid.h` declares
  `DAG_DECLARE_RELOCATABLE` for the object and for the three container types.
- `leaf_id_t` is `uint32_t`; `createLeaf` logs an error and returns `EMPTY_LEAF` when the pool
  is exhausted, so callers must handle a failed leaf allocation.
- `subGridIdx` is `int16_t`: at most 32767 subgrids, `-1` means none.
- `leafs.size() == branches.size()` is asserted after every insert.
- Objects must be erased with the *same* position they were inserted with, otherwise
  `eraseAt` asserts ("Erased twice or wrong old position") and the object leaks into the tree.
- A branch's packed boxes are only valid against the exact parent box that was used to pack
  them.
- The grid is not thread-safe. RIExtra serializes access with the `ccExtra` read/write lock
  (`ScopedRIExtraReadLock`, `ScopedLockWrite`). The only internal parallelism is the 3-axis
  `JobPool` inside `findBestHalfCut`.

## Debug and diagnostics

`VERIFY_ALL` (off by default) turns on the `LG_VERIFY` family: box containment,
no-recursion checks, object-present-after-insert checks, and full `leaf_verify` walks. Expect
it to be very slow; it is the tool for "an object vanished from the grid" bugs.

Console commands (`rigrid_console_handler`): `rigrid stats` (memory breakdown, leaf depth,
split fails), `rigrid oversize`, `rigrid biggest_rad`, `rigrid biggest_ext`, `rigrid bad_cuts`
(draws branch pairs whose object counts differ by more than 8x).

`riGridDebug.cpp` (`rigrid_debug_pos`) draws, for the cell under a world position: all leaf
boxes with object counts, an object-size histogram, and an 8x8 ASCII map of subgrid cell
occupancy oriented to the camera.

## File map

| file | role |
| --- | --- |
| `prog/dagorInclude/ADT/linearGrid.h` | the whole data structure and traversal |
| `prog/gameLibs/publicInclude/grid/gridImpl.h` | query-shape iterators shared with the spatial-hash grid |
| `prog/gameLibs/rendInst/riGen/riGrid.h` | `RiGridObject`, `RiGrid` typedef, `rigrid_find_*` declarations |
| `prog/gameLibs/rendInst/riGrid.cpp` | non-inline instantiation of every query shape |
| `prog/gameLibs/rendInst/riGridDebug.cpp` | in-world visualization |
| `prog/gameLibs/rendInst/rendInstGenExtra.cpp` | the `riExtraGrid` instance, config, mutations, console |
