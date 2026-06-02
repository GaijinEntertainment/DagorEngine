---
name: ref-tiled-scene
description: "Deep reference for scene::TiledScene -- architecture, data layout, culling pipeline, threading model, maintenance, and KD-tree internals"
metadata: 
  node_type: memory
  type: reference
  authored: 2026-05-28
  commit: 86acfc172403
---

# TiledScene -- Complete Architecture Reference

Source: `prog/dagorInclude/scene/dag_tiledScene.h`, `prog/engine/scene/tiledScene.cpp`
Base: `prog/dagorInclude/scene/dag_scene.h`, `prog/engine/scene/scene.cpp`
KD-tree: `prog/dagorInclude/scene/dag_kdtree.h`, `prog/dagorInclude/scene/dag_kdtreeCull.h`

---

## REVIEWER QUICK REFERENCE (read this first)

### What TiledScene Is (one paragraph)
A 2-level spatial acceleration structure for frustum/box culling of large instance sets (primarily RIExtra render instances). Level 1: uniform 2D XZ tile grid. Level 2: per-tile flat KD-tree (leaves only). Non-writing threads enqueue mutations as deferred commands; a designated writing thread flushes them. Read lock allows concurrent culling; write lock blocks readers during mutations.

### Proven Crash Patterns (from production incidents)

**P1: Orphaned reservations after command drop.**
`allocate()` calls `reserveOne()` (pre-claims a node slot) then enqueues ADD. If `flushDeferredTransformUpdates()` drops commands (e.g., `usedTilesCount == 0` after `term()`), the reservation persists. Next init + allocate cycle creates a tile with `tileCull.nodes == nullptr`. Crash in `internalFrustumCull`.
*Red flag*: Any code path in `flushDeferredTransformUpdates` that clears commands without calling `clearReserves()`.

**P2: Race in term().**
`term()` clears the command vector. Without the mutex, a concurrent `addCmd()` corrupts the vector.
*Red flag*: Any call to `mDeferredCommand.clear()` or vector mutation outside `mDeferredSetTransformMutex`.

**P3: Stale tile flags hide nodes from culling (FIXED but fragile).**
`setFlags()`/`unsetFlags()` changes the node's flags immediately at `SimpleScene` level, but tile-level and KD-leaf-level flag unions remain stale until `doMaintenance()` runs `MFLG_RECALC_FLAGS`.
**The fix**: `internalFrustumCull` checks `isTileFlagsValid = !(maintenanceFlags & MFLG_RECALC_FLAGS)`. When false (flags stale), the tile-level and leaf-level flag early-outs are skipped, so per-node flag checks still work correctly. The node WILL appear in culling results despite stale tile flags.
*Red flag for reviewer*: New culling code paths that add flag-based early-outs WITHOUT checking `isTileFlagsValid`. Also: code that adds a new maintenance flag but doesn't set `MFLG_RECALC_FLAGS` when node flags change.

**P4: `v_perm_xyzd` column mismatch corrupts transform.**
`v_perm_xyzd(src, val)` takes xyz from `src` and w from `val`. The bug: `m.col0 = v_perm_xyzd(m.col3, disappearDistSq)` -- this copies col3.xyz (translation) into col0.xyz (X-axis basis), corrupting the rotation/scale. Correct: `m.col0 = v_perm_xyzd(m.col0, disappearDistSq)`.
*Red flag*: Any `m.colX = v_perm_xyzd(m.colY, ...)` where X != Y. The first arg's xyz must be the column being overwritten, or the transform is silently corrupted.

**P5: node_index vs raw index confusion.**
In debug builds (`KEEP_GENERATIONS`), `node_index` has a generation counter in the low 8 bits: `rawIndex = node >> 8`. In release, `node_index == rawIndex`. Using `node_index` where a raw index is expected works silently in release but crashes in debug (wrong array offset). Bugs hide until someone runs a debug build.
*Red flag*: `nodes[node_index_var]` or `getIndexPool(node_index_var)` without going through `getNodeIndexInternal()` or `getNodeIndex()` first. Also: returning a raw index where `node_index` is expected (missing `getNodeFromIndex()`).

### Threading Contract

| Operation | Writing thread | Other threads | Lock held |
|-----------|---------------|---------------|-----------|
| `allocate`, `destroy`, `reallocate` | Immediate (if queue empty) | Deferred via command queue | Mutex for queue; WriteLock for immediate |
| `setTransform`, `setTransformNoScaleChange` | Immediate (if queue empty) | Deferred | Mutex; WriteLock (optional `do_not_wait`) |
| `setFlags`, `unsetFlags` | Immediate (if queue empty) | Deferred | Mutex; WriteLock |
| `flushDeferredTransformUpdates` | **Required** | **Never** | Mutex + WriteLock |
| `doMaintenance` | **Required** | **Never** | WriteLock |
| `frustumCull`, `boxCull`, `nodesInRange` | Yes (no lock needed) | Yes (auto ReadLock) | ReadLock |
| `getNode`, `getNodeFlags`, etc. | Yes | Yes (if under ReadLock) | -- |

**Rule**: If `writingThread == -1` (default), no locks are taken anywhere. All operations are single-threaded with zero lock overhead. Call `setWritingThread(tid)` to enable multi-threaded mode.

**Rule**: `tileCull[i].nodes` is a raw pointer into `tileData[i].nodes.data()`. If `tileData[i].nodes` reallocates while a reader holds the read lock, the reader crashes. This is safe because all mutations go through deferred commands + write lock, but custom code that mutates `tileData` directly would break this.

### Known Unresolved Concerns

**C1: `addTile` synchronization in `frustumCullTilesPass`.**
The `addTile` lambda does `tilesCount.load(relaxed)` to get the write index, then `tilesCount.fetch_add(1)` to publish it. There's a source TODO questioning whether the relaxed load before the fetch_add is safe. Single-producer (tiles pass runs on one thread), so likely fine, but the pattern is fragile.

**C2: `unsetFlagsUnordered` bypasses deferred system.**
This batch method requires the writing thread and takes WriteLock directly -- no deferred path, no mutex on the command queue. Safe for the writing thread, but a reviewer should ensure no caller uses this from another thread (the `G_ASSERT_RETURN(isInWritingThread())` is debug-only).

**C3: `setNodeUserDataDeferred` skips mutex.**
`RendinstTiledScene::setNodeUserDataDeferred()` calls `mDeferredCommand.addCmd()` directly WITHOUT taking `mDeferredSetTransformMutex`. If two threads call `setNodeUserDataDeferred` concurrently, `addCmd`'s `DoGrow` (reallocation) is not thread-safe. This is only safe if the caller guarantees single-writer access externally. Compare with `TiledScene::setNodeUserData()` which always takes the mutex.

### Key Contracts for Callers

1. **init before use**: Call `init(tileSize)` before any `allocate`. `tileSize > 0` enables tiling; `tileSize <= 0` is passthrough to SimpleScene.
2. **flush before cull**: Call `flushDeferredTransformUpdates()` before culling to apply pending commands. Culling stale data is safe (no crash) but may show outdated positions. Culling methods (`frustumCull`, `boxCull`, `nodesInRange`) acquire and release the read lock internally -- callers do NOT manage locking for these.
3. **maintenance is optional but important**: `doMaintenance()` is time-budgeted. Skipping it degrades KD-tree quality and leaves tile AABBs/flags stale. Return value `false` means "not done, call again next frame."
4. **pool bbox before allocate**: `setPoolBBox(pool, box)` must be called before `allocate(..., pool, ...)` for correct bsphere/disappear distance. Missing pool box = unit sphere with 1000m disappear distance.
5. **outer tile is special**: Objects with `bsphere_diameter > tileSize` go to the outer tile (index 0). The outer tile has no grid position and no tile-level spatial rejection -- all its nodes are tested individually. Too many objects in the outer tile degrades performance.
6. **flags are OR-accumulating**: `setFlags` does `|=`, not replace. There is no `replaceFlags`. To change a flag, `unsetFlags` then `setFlags`.

---

## 1. Class Hierarchy

```
SimpleScene           -- flat array of mat44f nodes with pool/flags/bsphere packed in W components
  TiledScene          -- 2D spatial grid over SimpleScene, per-tile KD-trees, deferred commands
    RendinstTiledScene -- rendInst-specific: distance array, user data, per-instance render data
      TiledScenesGroup<N> -- up to N RendinstTiledScenes (used for RIExtra LOD layers)
```

## 2. Node Data Layout (mat44f)

Each node is a single `mat44f` (4x vec4f = 64 bytes). The 3x4 transform occupies the XYZ of each column; the W components carry packed metadata:

| Field       | Location    | Content                                         |
|-------------|-------------|-------------------------------------------------|
| `col0.w`    | `m[0][3]`   | `worldSphRad^2 * poolDistScaleSq` -- disappear distance threshold. `worldSphRad = poolLocalSphRad * sqrt(maxScaleSq)`. Culling skips when `euclideanDist^2 * globalDistScale > col0.w` |
| `col1.w`    | `m[1][3]`   | Pool AABB center Y in local space (`poolSphereVerticalCenter[pool]`). Used to offset bsphere center along the object's local Y axis |
| `col2.w`    | `m[2][3]`   | `(flags << 16) | pool` -- packed 16-bit flags + 16-bit pool index |
| `col3.xyz`  | `m[3][0-2]` | World-space translation                         |
| `col3.w`    | `m[3][3]`   | Bounding sphere radius (world-space, accounts for scale) |

Accessors: `get_node_pool(m)`, `get_node_flags(m)`, `get_node_bsphere(m)`, `get_node_bsphere_rad(m)`.

The bounding sphere center is NOT just col3.xyz -- it's `col3.xyz + col1.xyz * col1.w` (translation + localY_axis * verticalOffset). `col1.xyz` is the object's local Y axis in world space, NOT necessarily world-up for rotated objects. See `get_node_bsphere()`:
```cpp
vec4f center = v_madd(col1, v_splat_w(col1), col3);  // center = col1 * col1.w + col3
return v_perm_xyzd(center, col3);                      // return (center.xyz, col3.w=radius)
```

### Node Index Handle

`node_index` is a `uint32_t`. In debug builds (`KEEP_GENERATIONS`), the low 8 bits are a generation counter for use-after-free detection: `index = node >> 8`, `generation = node & 0xFF`. In release, `node_index == array_index`.

### Pool Data

`poolBox[]` -- array of `bbox3f` per pool. The W components carry extra data:
- `poolBox[p].bmin.w` = distance scale squared (for LOD/disappear)
- `poolBox[p].bmax.w` = sphere radius of the pool's local-space bounding sphere

`poolSphereVerticalCenter[]` -- Y-offset of sphere center from pool origin.

## 3. Tiled Grid Structure

TiledScene divides the XZ world plane into a uniform 2D grid of square tiles.

### Grid Parameters
- `tileSize` -- side length of each tile in world units (set via `init(float tileSize)`)
- `tileGridInfo.{x0, z0, x1ofsmax, z1ofsmax}` -- grid origin (x0, z0) in tile coordinates; max offsets (x1ofsmax = width-1, z1ofsmax = height-1). Actual bounds: `[x0, x0+tilesDX)` x `[z0, z0+z1ofsmax+1)`
- `tilesDX` -- grid width (x1 - x0)
- `gridObjMaxExt` -- maximum extent of any object beyond its tile boundary (used to expand frustum query region)

### Tile Coordinate Mapping
```
posToTile(pos) -> tx = floor((pos.x + 0.5*tileSize) / tileSize)
                  tz = floor((pos.z + 0.5*tileSize) / tileSize)
```
The +0.5 offset means tile centers are at multiples of tileSize.

### Grid Indirection
```
tileGrid[gridIndex] -> tileDataIndex (or INVALID_INDEX)
gridIndex = (tz - z0) * tilesDX + (tx - x0)
```

### SoA Tile Storage (3 parallel arrays, same indices)

| Array        | Per-Tile Content                                          | Used During |
|--------------|-----------------------------------------------------------|-------------|
| `tileBox[]`  | `bbox3f` -- world AABB of all nodes in tile. `bmin.w` = packed `(kdTreeNodeCount \| flags<<16)`. `bmax.w` = max `(sphRad*distScale)^2` | Culling (hot path) |
| `tileCull[]` | `TileCullData` -- pointer to node_index array, kdtree left node or node count, maintenance flags | Culling (hot path) |
| `tileData[]` | `TileData` -- owns the `dag::Vector<node_index>` nodes list, KD-tree metadata, tile offsets, generation, rebuild penalty | Maintenance (cold path) |

**Index 0 is the "outer tile"** (`OUTER_TILE_INDEX = 0`). It holds objects that are too large for normal tiles (`bsphere_rad * 2 > tileSize` -- see `selectTileIndex`). The outer tile is always present after `init()`.

### Dead Tile Detection (Hot Path)
`isEmptyTileMemory(bbox)` checks the sign bit of `bmax.w` via `(((int*)&bbox)[7] & 0x80000000)`. When `v_bbox3_init_empty` is called, bmax becomes very negative (sign bit set). This is a branchless scalar check used in the culling loop to skip empty tile slots without loading TileCullData.

`isEmptyTile(bbox, cull)` is the SIMD version using `_mm_movemask_ps(bbox.bmax) & 8` (SSE) or `v_test_vec_x_lt_0(v_splat_w(bbox.bmax))`.

### Tile Lifecycle
1. `allocTile(tx, tz)` -- scans from `mFirstFreeTile` for a dead slot, or appends. Sets `tileOfsX/Z` (relative to grid origin, stored as uint16)
2. `releaseTile(idx)` -- clears tile data, sets `tileGrid[gridIdx] = INVALID_INDEX`, `v_bbox3_init_empty(tileBox[idx])`, decrements `usedTilesCount`
3. During `doMaintenance` phase 0, dead tiles are compacted via two-pointer swap (dead from front, live from back), then arrays resized to `usedTilesCount`. Grid indices are updated for swapped tiles.
4. Grid is auto-expanded via `rearrange(tx, tz)` when a node falls outside current bounds. `rearrange(float tileSize)` re-tiles the entire scene with a new tile size (calls `termTiledStructures()` then re-adds all alive nodes)

### selectTileIndex -- Routing to Tiles
```
if bsphere_rad * 2 > tileSize -> OUTER_TILE_INDEX
else:
  posToTile(node.col3) -> (tx, tz)
  if outside grid && allow_add -> rearrange(tx, tz) to expand grid
  if tileGrid[tx,tz] == INVALID && allow_add -> allocTile
  return tileGrid[tx,tz]
```
`allow_add=true` for allocations/moves, `allow_add=false` for lookups (returns INVALID_INDEX on miss with logerr).

## 4. KD-Tree Per Tile

Each tile with >= `min_kdtree_nodes_count` (default 16) nodes gets a flat KD-tree built from its nodes. The engine uses **KD_LEAVES_ONLY mode** (compile-time `#define KD_LEAVES_ONLY 1`), meaning only leaf nodes are stored -- no internal tree nodes.

### KDNode Layout (32 bytes)
```
bmin_start: vec4f  -- xyz = leaf AABB min, w = packed (flags<<16 | nodeCount) in leaves-only mode
bmax_count: vec4f  -- xyz = leaf AABB max, w = max (sphRad*distScale)^2 across leaf nodes
```

### Global KD-Node Pool
All tiles share a single `kdNodes[]` array. Each tile records `kdTreeLeftNode` (start index) and the count is stored in `tileBox[].bmin.w & 0xFFFF`. When a tile's KD-tree is rebuilt, it tries to reuse its old slot; if the new tree is larger, it appends to the end.

`usedKdNodes` tracks total usage. When load factor drops below 66%, `shrinkKdTree()` compacts the global array.

### Build Process (`buildKdTree`)
1. Sort tile nodes by internal index for cache locality
2. Compute per-node AABB and center
3. Call `kdtree::make_nodes()` to build a full binary KD-tree
4. Flatten to leaves-only via `kdtree::leaves_only()`
5. Rearrange tile's node_index array to match leaf ordering (critical: culling iterates nodes by leaf ranges)
6. Encode per-leaf flags union and max disappear distance into KDNode W components

### KD-Tree Parameters (tunable)
- `min_kdtree_nodes_count` = 16 -- minimum nodes to build a KD-tree
- `kdtree_rebuild_threshold` = 1.0 -- accumulated penalty before forcing rebuild
- `min_to_split_geom` = 8, `max_to_split_count` = 32 -- split heuristics
- `min_box_to_split_geom` = 32.0, `max_box_to_split_count` = 8.0 -- minimum box size to split

## 5. Culling Pipeline

### Single-Pass Frustum Cull: `frustumCull<use_flags, use_pools, use_occlusion>()`

Template parameters control which checks are compiled in:
- `use_flags` -- test node flags against `(test_flags & nodeFlags) == equal_flags`
- `use_pools` -- use pool AABB for precise narrow-phase visibility
- `use_occlusion` -- test against `Occlusion` (HiZ buffer)

The `pos_distscale` parameter is `vec4f(posX, posY, posZ, distScale)`. If `distScale > 0`, distance culling is enabled (`use_dist = true` internally). Distance checks use Euclidean distance (not Z-distance) to avoid view-dependent artifacts. Flags are shifted left by 16 at the entry point to match the packed format in `col2.w`.

**Flow:**
1. Early-out if `getNodesAliveCount() == 0`
2. Acquire read lock (`ReadLockRAII`)
3. If <= 1 tile, fall back to `SimpleScene::frustumCull()` (linear scan over all alive nodes)
4. Extract 8 frustum planes from globtm, normalize, transpose to SoA layout (planes stored as transposed 4x4 for SIMD-friendly dot products)
5. Compute frustum AABB via `v_frustum_box_unsafe`, map to tile grid region via `getBoxRegion()` (inflated by `gridObjMaxExt`)
6. Branch on `pos_distscale.w > 0` to select `use_dist=true/false` variant
7. Choose iteration strategy:
   - If `tileCull.size() <= tilesInGridRegion`: iterate ALL tiles linearly (cheaper than grid lookup when most tiles are in the region)
   - Otherwise: iterate only the grid region cells via `getTileIndexOffseted` + always check outer tile separately
8. For each tile: `internalFrustumCull()` per tile

### Per-Tile Frustum Cull (`internalFrustumCull` -- the hot inner loop)

1. **Tile-level early-out** (in order):
   - Flag check: if tile flags union doesn't match `equal_flags` AND flags are not stale (`!MFLG_RECALC_FLAGS`), skip entire tile
   - Frustum box-extent test: `v_box_frustum_intersect_extent2` returns OUTSIDE/INTERSECT/INSIDE
   - If `use_external_filter`: call `external_filter(bmin, bmax)` instead of frustum test
   - Distance check (if `use_dist`): `minDist^2_to_tile * globalDistScale > tileBox.bmax.w` -> skip. `tileBox.bmax.w` stores the maximum `(worldSphRad*distScale)^2` across all nodes in the tile -- if even the farthest-visible node in the tile is too far, skip the whole tile
   - Occlusion check: `occlusion->isVisibleBox(tileBox.bmin, tileBox.bmax)`
2. **KD-leaf iteration** (if KD-tree exists):
   - For each leaf: flag check, distance check, frustum intersect, occlusion check
   - Collect visible leaves into `fixed_vector<VisibleLeaf, 256>` (2KB on stack, framemem overflow)
   - Adjacent leaves with same inside/intersect status are merged (optimization)
3. **If no KD-tree**: single VisibleLeaf covering all tile nodes
4. **Per-node loop** over each VisibleLeaf's range:
   - Flag filter
   - Bounding sphere vs frustum (broad phase). Return values follow `Frustum::` enum: 0=OUTSIDE, 1=INSIDE, 2=INTERSECT. Two variants:
     - `v_is_visible_sphere`: returns 0 or 1 only (used when `use_occlusion` or `!use_pools` -- no need to distinguish INSIDE vs INTERSECT)
     - `v_sphere_intersect`: returns 0/1/2 (used with pools + no occlusion, since sphereVis==2 means sphere straddles frustum boundary -> needs narrow-phase pool AABB check)
   - Distance check (if `use_dist`): `euclideanDist^2 * globalDistScale > col0.w` -> skip
   - If `use_pools` and (`use_occlusion` OR `sphereVis == 2` meaning sphere straddles frustum): transform pool AABB by node matrix (`v_mat44_mul43`), test against 8 planes or occlusion buffer (narrow phase). When `sphereVis == 1` (fully inside), narrow phase is skipped unless occlusion is enabled.
   - If `use_occlusion` without `use_pools`: test bounding sphere against occlusion
   - Call `visible_nodes(node_index, mat44f_cref, vec4f distSqScaled)` callback (distSqScaled is zero if !use_dist)

### Two-Pass Frustum Cull (for multithreaded use)

**Pass 1: `frustumCullTilesPass()`** -- runs on one thread, collects visible tile indices into `TiledSceneCullContext`. Can wake up worker threads early via callback when N tiles are ready.

**Pass 2: `frustumCullOneTile()`** -- called per-tile from worker threads, processes one tile through the full per-node culling pipeline. Uses atomic `nextIdxToProcess` for lock-free work stealing.

### Box Cull: `boxCull<use_flags, use_pools>(box, ...)`
Same tile iteration strategy but tests AABB intersection instead of frustum planes. KD-leaf boxes are tested against the cull box. Per-node:
1. Sphere AABB vs cull box (`v_bbox3_test_box_intersect`) -- broad reject
2. If not fully inside: `v_bbox3_test_sph_intersect(cullBox, sphere, radiusSq)` -- tighter sphere-vs-box test
3. If `use_pools`: transform pool AABB by node matrix, test vs cull box (`v_bbox3_test_box_intersect`)
Callback is `(node_index, mat44f_cref)` -- no distance parameter. No occlusion support.

### Range Query: `nodesInRange<use_flags, use_pools>(box, ..., start_index, end_index)`
Box cull with tile-index range limiting. `start_index`/`end_index` select a subset of tiles to process, enabling multi-frame spreading of expensive queries (e.g., LOD-by-distance). Falls back to `SimpleScene::boxCull` if <= 1 tile.

## 6. Deferred Command System

Non-writing threads cannot modify the scene directly. Instead, they enqueue commands into `mDeferredCommand` (a `DeferredCommandVector`, mutex-protected). The writing thread flushes these in `flushDeferredTransformUpdates()`.

### Command Types
```
DESTROY, ADD, MOVE_FAST, MOVE, SET_FLAGS, UNSET_FLAGS, REALLOC,
SET_POOL_BBOX, SET_POOL_DIST, SET_USER_DATA+N (user-defined)
```

### Fast Path
If the caller IS the writing thread AND the command queue is empty, the operation executes immediately (no deferred path). This avoids queue overhead for the common single-threaded case.

### Flush Process (`flushDeferredTransformUpdates<T, F>`)
Template parameter `T` is the user data type, `F` is a user command processor functor.

1. **Lockless early-out**: `mDeferredCommand.empty()` via atomic read. If empty, return immediately (no mutex).
2. Take mutex + write lock
3. **Post-term guard**: if `usedTilesCount == 0`, commands were queued after `term()` was called. Clear commands + `clearReserves()` and return.
4. `applyReserves()` -- materialize pre-reserved node slots (from `reserveOne()`)
5. Iterate commands, dispatch to `*Imm()` methods. For `SET_USER_DATA+N`: call `user_cmd_processor(N, scene, node, (const T*)data)`
6. If capacity > 256, shrink command buffer (swap with 256-reserved vector, saves ~80MB on mobile)
7. Clear command buffer
8. Assert outer tile consistency (dead state matches between tileData, tileCull, tileBox)

### `DeferredCommandVector` -- Lock-Free Counter
`addCmd()` writes data then does `interlocked_increment` on `mCount`, ensuring readers see consistent data. `empty()` uses `interlocked_acquire_load` for safe lockless check.

### Node Reservation
`allocate()` from non-writing threads calls `reserveOne()` which pre-claims a slot (from firstAlive region, free list, or past-end) WITHOUT modifying the arrays. The actual arrays are grown later in `applyReserves()` during flush. This allows returning a valid `node_index` immediately.

## 7. Threading Model

- **Writing thread**: set via `setWritingThread(tid)`. All mutations (allocate, destroy, transform, flags) must happen on this thread or be deferred. Default `writingThread = -1` means "no threading, no locks needed."
- **Read lock**: `lockForRead()` returns true (and locks) only when caller is NOT the writing thread. `ReadLockRAII` wraps this. Multiple readers can run concurrently.
- **Write lock**: `WriteLockRAII` behavior depends on `writingThread`:
  - If `writingThread == -1`: no lock taken, `locked = true`, `needUnlock = false` (single-threaded mode, zero overhead)
  - If `writingThread != -1` and `do_not_wait = false`: blocks until write lock acquired
  - If `writingThread != -1` and `do_not_wait = true`: `tryLockWrite()` -- if fails, `locked = false` and the caller falls back to deferred path
- **Mutex**: `mDeferredSetTransformMutex` protects command queue access from concurrent `setTransform`/`allocate`/`destroy` calls across threads.

### The `do_not_wait` Path
`setTransform(node, transform, do_not_wait=true)` is for latency-sensitive callers. If the write lock is contested (readers are culling), it doesn't block -- instead defers the transform. This is used to avoid stalls when moving objects during the render frame.

## 8. Maintenance System (`doMaintenance`)

Called periodically from the writing thread with a time budget. Performs incremental upkeep:

### Phase 0: Tile Compaction
Dead tiles (empty, released) at the end of arrays are swapped with live tiles and arrays are resized. `mFirstFreeTile` is updated.

### Phase 1: KD-Tree Rebuild (`MFLG_REBUILD_KDTREE`)
Tiles whose KD-trees are invalid or degraded are rebuilt via `buildKdTree()`. Also recalculates tile AABB as a side effect.

### Phase 2: AABB Recalculation (`MFLG_RECALC_BBOX`)
Tiles whose bounding boxes may have shrunk (node removed from border, node moved inward) get a full AABB recompute from all contained nodes.

### Phase 3: Flag Recalculation (`MFLG_RECALC_FLAGS`)
Tiles whose flag union may be stale get a full recompute. This updates both tile-level and KD-leaf-level flag unions.

Each pass is time-budgeted: if `get_time_usec(reft) > max_time_to_spend_usec`, the function returns false (incomplete). The caller should retry next frame.

### Maintenance Flags (per tile)
- `MFLG_RECALC_BBOX` -- tile AABB needs full recompute
- `MFLG_RECALC_FLAGS` -- tile/leaf flag union needs recompute
- `MFLG_REBUILD_KDTREE` -- KD-tree needs full rebuild
- `MFLG_IVALIDATE_KDTREE` -- KD-tree is immediately invalidated (count set to 0 in tileBox)

### Incremental KD-Tree Updates (in `updateTile`)
When a node moves, the code tries to avoid a full KD-tree rebuild. The logic finds which KD-leaf contains the node by scanning leaf ranges:

1. **Node stays inside its KD-leaf AND same disappear distance**: No flags set, nothing changes. Fastest path.
2. **Node stays inside its KD-leaf BUT disappear distance changed**: Recalculate leaf's max disappear distance. No geometric rebuild.
3. **Node moved inside leaf (was on border, moved inward)**: Recalculate leaf AABB from all leaf nodes via `fastGrowNodeBox`. Clear REBUILD/INVALIDATE flags.
4. **Node moved outside leaf (crossed border)**: Expand leaf AABB to include new position. Accumulate `rebuildPenalty += relative_expansion`. If `rebuildPenalty < kdtree_rebuild_threshold`: tolerate degraded tree. If exceeded: schedule REBUILD.
5. **Node removal from leaf**: If leaf count drops to 0, remove the KDNode (memmove). If leaf count > 0 and node was on leaf border, recompute leaf AABB. If tile node count drops below `min_kdtree_nodes_count`, invalidate the KD-tree entirely.

The `rebuildPenalty` is the relative amount the KD-leaf has grown beyond its original tight fit. This prevents constant rebuilds for small oscillations while ensuring eventually-consistent tree quality.

### Node-to-Tile Movement
Two variants with different tile-reassignment logic:

- **`setTransformImm` (MOVE)**: ALWAYS rechecks tile assignment via `selectTileIndex`, even for zero movement. Reason: scale may have changed, altering bsphere radius, which could cross the `bsphere_rad * 2 > tileSize` threshold (moving the node to/from the outer tile).
- **`setTransformNoScaleChangeImm` (MOVE_FAST)**: Only rechecks tile if movement vector is non-zero AND node is not in the outer tile. Outer tile nodes stay in the outer tile regardless of position (they're there because of size, not position).

In both cases: old tile's `selectTileIndex` is called first (with `allow_add=false`) to find the current tile, then after applying the transform, new tile is computed (with `allow_add=true`). If they differ, `removeNode` + `insertNode`. Finally `updateTile` updates KD-tree/AABB incrementally.

### `fastGrowNodeBox` Optimization
When expanding a tile/leaf AABB by a node, this function first checks if the node's bounding sphere is already inside the current box. If yes, it only updates `bmax.w` (max disappear distance). Only if the sphere extends beyond the box does it compute the full pool-based AABB via `v_bbox3_init(wabb, node, poolBox[pool])`. This avoids expensive per-pool AABB transform on the hot maintenance path.

## 9. RendinstTiledScene Extensions

`RendinstTiledScene` adds:
- **16 rendInst-specific flags** (REFLECTION, HAS_OCCLUDER, LARGE_OCCLUDER, HAVE_SHADOWS, VISIBLE_0..2, IS_WALLS, IS_RENDINST_CLIPMAP, VISIBLE_IN_SHADOWS, etc.)
- **distance[]** -- per-node uint8 shadow/light distance (LIGHTDIST_TOOBIG=255, DYNAMIC=254, INVALID=253)
- **nodeUserData[]** -- per-node arbitrary int32 array (variable word count, rearrangeable)
- **perInstanceRenderAdditionalData** -- sparse map from node_index to uint32
- **User commands** via deferred system: `ADDED=0, MOVED=1, POOL_ADDED=2, BOX_OCCLUDER=3, QUAD_OCCLUDER=4, SET_UDATA=5, COMPARE_AND_SWAP_UDATA=6, INVALIDATE_SHADOW_DIST=7, SET_FLAGS_IN_BOX=8, SET_PER_INSTANCE_ADDITIONAL_DATA=9`
- **`setNodeUserDataDeferred()`**: Enqueues SET_USER_DATA WITHOUT taking `mDeferredSetTransformMutex` (always deferred, no immediate path, no mutex). Packs word count into `transformCommand.col3[3]`. **Thread-safety concern**: see C3 in reviewer section.
- **`setNodeUserDataEx()`**: Like `setNodeUserData()` but takes dword count + raw int32 array instead of a POD template. Has the same mutex + immediate-execute fast path.

`TiledScenesGroup<N>` groups up to N `RendinstTiledScene` instances with shared pool info (`TiledScenePoolInfo` with per-LOD distance thresholds), box/quad occluders, and a `newlyCreatedNodesBox` for tracking freshly added instances. `reinit(sz)` terminates all scenes and reinitializes with `sz` active scenes.

## 10. Key Invariants

1. `tileData.size() == tileCull.size() == tileBox.size()` (enforced by `checkSoA()`)
2. `tileData[0]` / `tileCull[0]` / `tileBox[0]` is always the outer tile
3. A tile with 0 nodes is dead (`generation == 0`, `nodes == nullptr` in cull data)
4. A tile with 1 node has its AABB set exactly to that node's AABB (via `initTile()`)
5. KD-tree leaves reference contiguous ranges of the tile's `nodes[]` array (leaf ordering matches)
6. `tileCull` mirrors `tileData` for culling -- updated via `copyTileDataToCull()`:
   - `tcull.nodes = tdata.nodes.data()` (raw pointer into tileData's vector)
   - If KD-tree present: `tcull.kdTreeLeftNode = tdata.kdTreeLeftNode`
   - If no KD-tree: `tcull.nodesCount = tdata.nodes.size()`
   - `tcull.maintenanceFlags = tdata.maintenanceFlags`
   - This means tileData's node vector must NOT reallocate while readers hold the read lock
7. Objects with `bsphere_diameter > tileSize` go to the outer tile
8. The grid auto-expands when nodes fall outside, but tile offsets (`tileOfsX/Z`) are uint16, limiting relative grid dimensions to 65535 tiles per axis
9. `lockForRead()` returns false (no lock) when called from the writing thread -- the writing thread already has exclusive access
10. A tile's `generation` counter (in TileData) is bumped on every `updateTile()`. Generation 0 means dead. Not used for culling, only liveness tracking.
11. `setFlags` at `SimpleScene` level is `|=` (OR), not replace. `unsetFlags` is `&= ~`. So flags accumulate; there's no `replaceFlags`.

## 11. Deferred Command System -- Full Detail

### DeferredCommand Structure (80 bytes with alignment padding)
```cpp
struct DeferredCommand {
  mat44f transformCommand;  // 64 bytes, 16-byte aligned -- carries transform or payload data
  node_index node;          // 4 bytes -- target node
  Cmd command;              // 4 bytes -- command enum
  // 8 bytes padding to align next element's mat44f to 16 bytes
};
// sizeof = 80 due to mat44f alignment requirement
```

### Command Encoding

Each command packs its data into the `transformCommand` mat44f differently:

| Command | `transformCommand` usage | `node` |
|---------|-------------------------|--------|
| `ADD` | Full transform. `col2.w` = `(flags<<16)\|pool` via `encodeCommandPoolFlags()` | Reserved node from `reserveOne()` |
| `DESTROY` | Unused | Target node |
| `MOVE_FAST` | Full transform (no scale change) | Target node |
| `MOVE` | Full transform (recalculates bsphere from pool) | Target node |
| `SET_FLAGS` | `col2.w` has flags encoded via `set_node_flags()` (rest zeroed) | Target node |
| `UNSET_FLAGS` | Same as SET_FLAGS | Target node |
| `REALLOC` | Full transform. `col2.w` = `(flags<<16)\|pool` | Target node |
| `SET_POOL_BBOX` | `col0` = box.bmin, `col1` = box.bmax, `col2.w` = pool | `INVALID_NODE` |
| `SET_POOL_DIST` | `*(float*)&col0` = dist_scale_sq, `col2.w` = pool | `INVALID_NODE` |
| `SET_USER_DATA+N` | First N*4 bytes = user payload (memcpy'd) | Target node |

### DeferredCommandVector -- Lockless Empty Check

`DeferredCommandVector` inherits from `dag::Vector<DeferredCommand>` but deletes `push_back()` to force usage of `addCmd()`.

**`addCmd(callback)`**: Grows buffer if needed, then calls `callback(data()[mCount])` to fill the slot in-place, then atomically increments `mCount` via `interlocked_increment`. This ensures readers see the counter change ONLY after data is fully written.

**`empty()`**: Uses `interlocked_acquire_load` on `mCount`. This is the key to the lockless fast path: `flushDeferredTransformUpdates()` checks `mDeferredCommand.empty()` WITHOUT taking the mutex. If empty, it returns immediately. Only if non-empty does it take the mutex. This avoids a mutex lock on every frame when there are no pending commands (the common case).

### Move Command Deduplication

`setTransformNoScaleChange()` and `setTransform()` search the existing command queue for a prior MOVE/MOVE_FAST command for the same node. If found, they update the existing command's transform instead of adding a new one. This prevents the queue from growing unboundedly when a node moves every frame.

### Allocation and Reservation

When `allocate()` is called from a non-writing thread:
1. `reserveOne()` pre-claims a slot index without modifying the node array. It uses three fallback pools:
   - `pendingNewNodesCount1`: slots before `firstAlive` (dead prefix)
   - `pendingNewNodesCount2`: slots from `freeIndices` (holes in array)
   - `pendingNewNodesCount3`: virtual slots past `nodes.size()` (not yet allocated)
2. A `node_index` is returned immediately (with generation counter in debug)
3. During `flushDeferredTransformUpdates()`, `applyReserves()` materializes these reservations: decrements `firstAlive`, pops `freeIndices`, and `nodes.resize()` as needed

**Critical bug fix**: If `flushDeferredTransformUpdates()` dropped commands (e.g., `usedTilesCount == 0`), it forgot to call `clearReserves()`. This left phantom reservations that caused dead tiles to appear in culling (nodes were "reserved" but never initialized), leading to crashes in `internalFrustumCull` where `tileCull.nodes == nullptr`.

### term() Race Fix

`term()` clears the command vector. But without the mutex, a concurrent `setTransform()` call could be mid-`addCmd()` when `clear()` fires, corrupting the vector. Fix: take `mDeferredSetTransformMutex` in `term()` before clearing.

## 12. SimpleScene Internals

### Node Array Layout
`nodes[]` is a flat `dag::Vector<mat44f>`. Allocation uses a 3-tier strategy:
1. **Prefix reuse**: If `firstAlive > 0`, decrement `firstAlive` and use the dead prefix slot
2. **Free list**: `freeIndices[]` tracks holes from destroyed mid-array nodes. Pop from back.
3. **Append**: Push to end. After 128K nodes, grows conservatively (+32K instead of 2x)

### Destruction
`destroy(node)` invalidates the slot by writing `V_CI_MASK0001` into `col0` (makes `col0.w = 0xFFFFFFFF`). Then:
- If it's the last element: `nodes.pop_back()`
- If it equals `firstAlive`: `firstAlive++`
- Otherwise: added to `freeIndices`
- If `firstAlive == nodes.size()`: `clearNodes()` (complete reset)

### `setTransform` vs `setTransformNoScaleChange`
- `setTransformNoScaleChange`: Only overwrites XYZ of each column (via `v_perm_xyzd`), preserving all W metadata. Caller guarantees scale hasn't changed.
- `setTransform`: Recalculates bsphere radius from pool bbox * max_scale, and disappear distance. Slower but handles scale changes.

### Scale Computation (`v_get_max_scale_sq`)
```cpp
res = max(|col0|^2, |col1|^2, |col2|^2)
    + |dot(col0,col1)| + |dot(col0,col2)| + |dot(col1,col2)|
```
This is an upper bound on the squared maximum singular value of the 3x3 rotation-scale matrix. Not exact for non-uniform scale but safe (overestimates).

### Pool Sphere Computation (`setPoolBBox`)
The bounding sphere center is placed at `(0, centerY, 0)` in local space -- only the Y component of the AABB center, X and Z are zeroed:
```cpp
vec3f sphVerticalCenter = v_and(v_bbox3_center(box), V_CI_MASK0100);  // (0, cy, 0)
```
By centering on the Y axis only, the sphere has XZ symmetry. This means only 4 of 8 AABB corners need distance checks (corners 0, 1, 4, 7 -- one per Y/XZ quadrant). The sphere radius is the max distance from this center to those 4 corners. This value is stored in `poolBox[p].bmax.w`. The Y offset itself goes into `poolSphereVerticalCenter[pool]` and then into each node's `col1.w`.

When pool has no valid bbox (pool >= poolBox.size()), `sphereRadius = sqrt(maxScaleSq)` (unit sphere scaled) and `disappearDistSq` uses `defaultDisappearSq` (1000000.0 -- disappear at 1000m for 1m sphere).

## 13. Known Bugs and Historical Fixes

### Crash: Dead Tile in Culling
**Root cause**: `allocate()` called `reserveOne()` then enqueued an ADD command. If `flushDeferredTransformUpdates()` dropped commands (because `usedTilesCount == 0` after `term()`), the reservation was never cleared. Next `init()` + `allocate()` cycle would produce a tile that appeared valid in `tileBox` but had `tileCull.nodes == nullptr`.
**Fix**: `clearReserves()` in the early-exit path of `flushDeferredTransformUpdates()`.

### Crash: Race in term()
**Root cause**: `term()` cleared `mDeferredCommand` without holding the mutex. Concurrent `addCmd()` could corrupt the vector.
**Fix**: Take `mDeferredSetTransformMutex` in `term()`.

### Bug: Flag Culling Returns Stale Results
**Root cause**: When `setFlags()`/`unsetFlags()` changes a node's flags, the tile-level and KD-leaf-level flag unions aren't updated until `doMaintenance()` runs `MFLG_RECALC_FLAGS`. Before the fix, culling used tile flag unions for early-out unconditionally, so the entire tile would be skipped even though the per-node flags were correct.
**Fix**: Added `isTileFlagsValid = !(maintenanceFlags & MFLG_RECALC_FLAGS)` check. When flags are stale, tile-level and leaf-level flag early-outs are bypassed, falling through to per-node checks which have correct (immediately-updated) flags.

### Bug: setPoolBBox Corrupts Transform
**Root cause**: `v_perm_xyzd(m.col3, disappearDistSq)` was written into `m.col0`, copying col3.xyz (translation) into col0.xyz (X-axis basis vector).
**Fix**: Changed to `v_perm_xyzd(m.col0, disappearDistSq)`.

### Bug: setPoolBBox Uses Wrong Index Type
**Root cause**: `getIndexPool(node)` was called with a `node_index` (which includes generation in debug) instead of the raw array index.
**Fix**: Use `getIndexPool(index)` with the already-resolved raw index.

## 14. External Filter System

Added `use_external_filter` template parameter to `frustumCullOneTile`. Instead of the standard frustum planes, callers can provide a custom `ExternalFilterFn(bmin, bmax) -> int` that returns OUTSIDE/INTERSECT/INSIDE. Used by CSM shadow rendering to test tiles against the cascade shadow region (a non-frustum shape).

## 15. Tile Culling for CSM Shadows

Added `gridObjMaxHeight` tracking (maximum Y extent of any tile box). Shadow passes use this to compute a world-space AABB of potential shadow casters, passed as `tileCullBbox` to `frustumCullTilesPass()`. The `disable_region_prefilter` flag skips grid-region optimization for shadow passes where the shadow volume may not align well with the XZ tile grid.

## 16. Performance Characteristics

- Tile AABB test is the first reject -- very cheap (single SIMD box-frustum test)
- KD-leaves provide 8-32x rejection before per-node tests
- Per-node broad phase uses bounding sphere (single SIMD sphere-frustum)
- Per-node narrow phase transforms pool AABB by node matrix (mat44 multiply + 8-plane test)
- Two-pass cull enables tile-level parallelism across worker threads
- `fixed_vector<VisibleLeaf, 256>` keeps leaf lists on stack (2KB), spills to framemem
- Adjacent visible leaves are merged to reduce iteration overhead
- `gridObjMaxExt` expands the frustum-to-grid query to catch objects whose AABB extends beyond their tile
- Move command deduplication prevents queue from growing when nodes move frequently
- Lockless empty check on the command queue saves a mutex lock per frame in the common (no-commands) case
- After 128K nodes, allocation growth is capped at +32K to avoid memory waste on mobile (command buffer shrink at capacity > 256 saves ~80MB)
- LOD-by-distance optimization uses tile culling for coarse distance rejection (3.5x speedup measured: 45.9us -> 13.3us average)
