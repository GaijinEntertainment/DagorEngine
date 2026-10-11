# raytracedToroidalStaticShadow: paged ray traced sun shadow clipmap - spec

This spec uses Simplified Technical English. Each term in section 1 has
one meaning, and the spec always uses that term for that meaning.

## TL;DR

The rtss is a paged sun shadow clipmap around the camera. Each level is
a window of 8 x 8 pages of 122 x 122 texels, with twice the texel of the
level below: 0.08 m at level 0 and 5 levels to +- 468 m by default.
Build rays (SWRT, HWRT or a project tracer) or a raster callback fill a
page, and each page keeps its own light transform. A moving sun rebuilds
a level page by page in a sweep; a moving camera pre-traces the pages
that come into the window. Two page budgets bound this work: a
validity burst for each frame for the coarsest level, which has no
coarser level to fall back to, and an update budget for all the rest,
which a frame may spend for a batch of frames; the missing pages of the
coarsest level take it too, past the burst. A shader samples the rtss
at any world position inside the levels: screen pixels, GI ray hits,
voxels.

Effect: no empty or popping static shadow when the sun or the camera
moves, a shadow at most two tolerances behind the sun while the budget
follows it, no skewed matrix and no height window, and a range that
doubles with each level of 2 MiB. Cost: in testGI the trace producers
take more GPU time than the classic raster to follow the same moves,
and the screen lookup costs 0.22 ms more at 4K (see "Why").

## Why: against the classic static shadow

The classic static shadow is `ToroidalStaticShadows` (3.1). WT and DNG
use it so:
- **WT** (`skyquake/prog/render/staticShadows.cpp`,
  `_shadowPresets.blk`): 2 cascades at most, DEPTH16. For the animated
  sun the far cascade has a transition texture of the cascade size (a
  double buffer), so WT allocates the memory of 3 cascades. Presets:
  medium 2 x 1024 to 375 m, high 2 x 2048 to 450 m, ultrahigh 2 x 4096
  to 525 m; no static shadow in the aircraft modes. The sun moves in
  real time (15 deg/h), and the shadow sun is at least 11.5 deg high
  (`weather.cpp` clamps the light direction to y <= -0.2). The default
  thresholds give a whole-cascade re-render at each 0.81 deg, a forced
  one at 8.1 deg.
- **DNG** (`shadowsManager.cpp`): 2 cascades of 4096 to 700 m, a static
  sun by default. With a dynamic time each sun step of 2.56 deg is a
  forced re-render (thresholds 0.999, 0.999), and the far cascade gets a
  transition texture, so again the memory of 3 cascades. Enlisted
  updates 0.025 of the texture in a frame, in uniform parts.

What the rtss changes (from the code of both libraries):
- **Moving sun.** The classic re-renders a whole cascade at each sun
  step. Without a transition texture the old shadow stays until each
  region is drawn again (a lazy step), or the cascade reads as empty
  until it is drawn again (a forced step: DNG, and WT above 8.1 deg).
  The rtss sweeps each level in key order (8.4), so a lookup reads no
  gap and no wrong transform (8.5), and a page falls at most twice the
  tolerance of its level behind the sun. The tolerance is 0.1 deg at
  level 0, doubled for each level up to 1.6 deg.
- **Moving camera.** The classic draws the new strip of a cascade (the
  full texture width) in one frame when it is at most twice the budget
  of the frame; a larger strip is cleared and reads as lit until it is
  drawn. The rtss pre-traces the pages before the camera reaches them
  (8.6), inside the page budget, and a teleport reads no shadow of the
  old place (8.2). A move along the sun axis needs no work (8.6).
- **No skewed matrix, no height window.** The classic uses a skewed
  matrix above 5.7 deg of sun elevation and an ortho matrix below, and
  the switch is a forced re-render. The skewed matrix holds a height
  window (`maxHtRange`, the world box height up to 1 km in DNG): when the
  world box is taller, a height change of 10 % of the window scrolls the
  depth of the whole texture and re-renders the cascade. The rtss has one orthonormal light space for each sun angle,
  with anisotropic texels at a low sun (6.6), and takes the depth range
  from the world box (6.4). A camera above the world box keeps the
  window at the box top, so a climb there needs no work (8.1).
- **Range for the memory.** A level adds twice the range for 2 MiB. The
  classic adds range with the texture side: twice the range at the same
  texel is four times the memory. The cost is the texel at range: past
  level 0 an rtss texel is up to 1/183 of its distance from the camera
  (5.1 pixels of a 1080p view with a 60 deg vertical FOV), where WT high
  has 1/341 (2.7 pixels) and DNG 1/681 (1.4 pixels) at the near edge of
  the far cascade. Near the camera the rtss texel is the smaller one.
- **Producers.** A traced page draws nothing, so its CPU cost does not
  grow with the casters in it, and a far level takes the same rays as a
  near one. The classic draws all casters of a region, and its far
  cascade has 9 times the area for each texel. SWRT, HWRT and raster are
  chosen for each level (7.5).
- **Destruction.** Both rebuild only the texels under a box: the classic
  a sub-rectangle of each cascade, the rtss sub-slot jobs on its traced
  levels (8.7).

Memory of the static shadow textures:

| Config | Memory | Range and texel of each cascade or level |
|---|---|---|
| WT medium | 6 MiB, of it 2 MiB transition | +- 125 m at 0.24 m, +- 375 m at 0.73 m |
| WT high | 24 MiB, of it 8 MiB transition | +- 150 m at 0.15 m, +- 450 m at 0.44 m |
| WT ultrahigh | 96 MiB, of it 32 MiB transition | +- 175 m at 0.09 m, +- 525 m at 0.26 m |
| DNG, static sun | 64 MiB | +- 233 m at 0.11 m, +- 700 m at 0.34 m |
| DNG, dynamic time | 96 MiB, of it 32 MiB transition | as with a static sun |
| rtss defaults | 10 MiB, and 0.25 MiB of raster scratch | +- 29 m at 0.08 m, twice for each level, +- 468 m at 1.28 m |
| rtss, 6 levels | 12 MiB | up to +- 937 m at 2.56 m |

Measured in testGI: battle_of_berlin, DX12, RTX 4090 Laptop GPU, 3840 x
2160, about 85 frames per second, one run each. The classic is the
static shadow of testGI before the rtss, with the settings of WT high
(2 x 2048, 450 m, the cascade flags of WT) and of DNG (2 x 4096, 700 m,
budget 0.025, uniform update, thresholds 0.999). The rtss uses the
W x S layout 16 x 64 (update budget 12, validity burst 128) with the
`swrt`, `hwrt` and `raster` presets (12.4) at `batchFrames` 1, before
the carry-over, the raster at a page cost of 3 with a 4-slot scratch
(the defaults of that time); the default 8 x 128 takes about half the
CPU and less GPU (5.1.1). A cell is the CPU / GPU time in ms of the
static shadow update and build, summed over the scenario, and then the
CPU / GPU time of its worst frame; GPU timestamps enclose it. The rtss
update includes its scheduler, about 0.02 ms CPU in each frame.

| Scenario | classic WT high | classic DNG | rtss swrt | rtss hwrt | rtss raster |
|---|---|---|---|---|---|
| walk 6 m/s, 10 s | 2.5 / 1.1, 0.33 / 0.19 | 1.9 / 0.6, 0.33 / 0.20 | 23 / 5.4, 0.14 / 0.18 | 24 / 6.6, 0.09 / 0.21 | 40 / 9.1, 0.25 / 0.08 |
| drive 20 m/s, 10 s | 7.6 / 3.5, 0.38 / 0.21 | 9.1 / 4.3, 1.3 / 0.63 | 24 / 10, 0.09 / 0.20 | 36 / 18, 0.15 / 0.51 | 66 / 19, 0.23 / 0.07 |
| fly 100 m/s, 6 s | 22 / 9.9, 1.0 / 0.29 | 27 / 14, 1.3 / 0.67 | 18 / 32, 0.18 / 1.6 | 35 / 35, 0.40 / 1.6 | 94 / 21, 3.7 / 0.70 |
| climb 10 m/s, 10 s | 1.7 / 0.6, 0.23 / 0.13 | 1.7 / 0.4, 0.27 / 0.13 | 20 / 3.9, 0.10 / 0.13 | 15 / 3.1, 0.07 / 0.14 | 38 / 9.0, 0.22 / 0.06 |
| sun 5 deg in 20 s | 13 / 5.9, 0.88 / 0.24 | 11 / 4.1, 1.2 / 0.26 | 98 / 143, 0.22 / 0.22 | 88 / 137, 1.5 / 0.23 | 691 / 74, 14 / 0.09 |
| invalidate a 50 m box | 0.4 / 0.2, 0.16 / 0.11 | 0.6 / 0.2, 0.26 / 0.11 | 2.9 / 1.6, 0.07 / 0.09 | 2.6 / 1.0, 0.06 / 0.10 | 42 / 5.3, 0.46 / 0.09 |
| teleport 300 m | 2.0 / 1.2, 0.41 / 0.25 | 4.5 / 2.6, 0.47 / 0.28 | 7.3 / 5.9, 0.36 / 0.55 | 7.6 / 3.5, 0.18 / 0.55 | 111 / 16, 5.0 / 0.58 |
| teleport 1.7 km | 4.2 / 2.7, 0.80 / 0.87 | 12 / 9.5, 1.4 / 1.8 | 8.5 / 29, 0.23 / 2.3 | 11 / 18, 0.22 / 2.3 | 170 / 35, 8.5 / 2.5 |

Frames until the update ends, and frames that read a wrong shadow:

| Event | classic WT high | classic DNG | rtss swrt |
|---|---|---|---|
| teleport 300 m | 12 frames; 2 read empty (lit) regions | 20; 13 empty | 79; none empty, 64 read a coarser level |
| teleport 1.7 km | 11; 6 empty | 34; 26 empty | 92; 1 empty (the burst of 128 takes the coarsest level in 2 frames), 74 coarser |
| sun 5 deg | 30 empty, 35 with old content | 27 empty, 26 old | none |
| fly 100 m/s | none | 10 empty | none |

Why the rtss costs more in total, from the cost of a page against the
pages in a frame (swrt, a whole level rebuilt):
- A page costs 3.3, 4.9, 6.8, 9.3 and 15 us GPU on levels 0 to 4 with the
  trace, and 11.8, 13.9, 15.7, 16.1 and 17.8 us with the raster (plus 36
  to 43 us CPU). The trace grows with the texel: neighbor rays of a
  coarse page are far apart, so their walks of the SWRT tree diverge.
  The raster grows less, as its cost is mostly the draws of each page.
  So the trace is the cheaper producer on every level here, most on the
  fine ones.
- A frame that traces costs at least about 30 us GPU, whatever its
  pages: a small dispatch lasts as long as its slowest rays. One page in
  each frame costs 31 us a page, 24 pages in a frame 2.6 us, a whole
  level 1.8 us. The batching of 8.2 at `batchFrames` 1 builds the
  pre-trace pages of a move in 53 of the 857 frames of the walk (375
  without it), where the classic draws a large region 6 times. A sun
  faster than the budget traces in one frame of `batchFrames` (4 by
  default), a batch of pages in one dispatch; a real-time sun builds
  about 0.17 pages a frame (8.4).
- The rtss also builds more: five levels, and a level 0 tolerance of
  0.1 deg, where WT re-renders at 0.81 deg.
- The terrain march was the largest cost before its start moved to the
  top of the height encoding (7.2): then a traced page took 3.7 times
  as long.
- The sun row is 5 deg at 0.25 deg/s (a 24-minute day); at the real
  time of WT (15 deg/h) the rtss swrt takes 0.1 ms GPU in each second.
- The testGI scene is cheap to raster (0.1 to 0.2 ms GPU for a classic
  region); the raster cost of a game scene with many rendinsts is not
  measured here.

Lookup cost, same view, nothing to build: `combineShadows` takes 0.85 to
0.86 ms with the classic and 1.08 ms with the rtss (DITHERED with
HARDEN, 9.7). The GI passes that read the static shadow at ray hits (the
screen probe trace, the radiance grid, the volumetric GI) differ by at
most 4 %.

Code size: the classic has 1906 lines of C++ and 630 of shaders; the
rtss 2117 of C++ and 723 of shaders (142 of them the debug views), and
3441 lines of unit tests.

The L16 atlas (5.2) reads almost as an R32F one did. In 12 testGI views
with shadows (3 cameras, sun elevations of 40, 20, 10 and 4 deg) at
most 0.09 % of the pixels of the rtss visibility differ by more than 8
of 255 (7688 of 8.3 M over trees and roofs, against 67 between two
R32F runs), 99.8 % of them lighter: specks of self shadow on thin
details, which the depth step of the slot range moves past the bias.
The view 5 difference to the reference changes by at most 0.007
percent points, and the build and lookup times are the same. The L16
compare sampling on the consoles is not checked; Vulkan and Metal
take the manual form (9.4).

### Possible extensions

- **Moving objects.** Keep a copy of the static pages, and in each frame
  draw the moving casters over the copy into the pages they touch; or,
  with HWRT, retrace the small regions of light space under each moving
  object.

## 1. Terms

- **level**: one toroidal clipmap level. Level k has the texel size
  `texel0 * 2^k`.
- **page**: P x P texels of one level (`P = S - 2B`, default 122), in
  the light space of one transform. An integer page coordinate (int2)
  addresses it.
- **slot**: the storage of one page in the atlas: S x S texels
  (S = `slotTexels`, default 128), the page plus a border of B texels
  (B = `borderTexels`) on each side.
- **window**: the W x W slots of one level (W = `windowPages`, default
  8). A page goes into the slot `page mod W`.
- **atlas**: one L16 Texture2DArray, one layer for each level, with
  `W*S` texels on each side.
- **slot table**: the GPU buffer with one entry for each slot: page
  coordinate, transform bit, valid flag, depth limit. The CPU owns it.
- **light transform**: `T(X) = M*X + c` for a world point X, with light
  space y scaled by `sy` (6.6). The rows of M are the unit vectors `e_x`,
  `e_y` and `d`, and `sy` is kept apart; only the GPU rows (5.5) hold
  `sy * e_y`. `d` is the direction in which the light travels
  (`d = -dir_to_sun`). Light space z is the depth along d.
- **transform bit**: which of the two live transforms of a level was used
  to build a slot.
- **current bit**: the transform bit of all slots of a level when the
  level does not sweep.
- **target bit**: the transform bit that a sweep writes.
- **sweep**: the rebuild of all slots of one level with a new transform,
  in descending key order.
- **shift direction u**: the light space xy direction in which a rotation
  of the sun moves the receivers of one level. It is the same for all
  receivers of the level.
- **key**: the place of a page in the sweep order of its level:
  `sign(u.y) * page.y * 65536 + sign(u.x) * page.x` (`rtss_sweep_key`),
  its row first, then its place in the row. A sweep builds pages from
  the largest key to the smallest: row by row from the +u.y side, each
  row from its +u.x end (8.4).
- **sweep front**: the key of the last slot that the sweep built from its
  ordered list of current-bit slots (8.4). Pages that 8.2 adds past the
  +u edge of the window do not move it.
- **pivot**: the point where the new and the old transform of a level
  give the same light space xy.
- **producer**: the code that fills a slot: the tracer callback (7.2), the
  raster callback (7.3), or both on an `Augment` level (7.5).
- **recipe**: how the pages of one level are built: `raster` (`None`,
  `All`, `Augment`) and `pageCost` (7.5).
- **build ray**: one ray for each slot texel, parallel to d. It starts on
  the start plane.
- **start plane**: the light space plane `z = zStart` of one job. It is
  sunward of all casters of that job.
- **job**: one rectangle of one slot that a producer fills in one frame.
- **receiver**: a point where a consumer asks for sun visibility.
- **caster**: geometry that a build ray can hit.
- **lookup**: the shader function that gives the sun visibility at a
  receiver.
- **selection extent E_k**: the half size of the light space cube around
  the camera that level k serves.
- **tolerance**: the sun angle that the newest transform of a level can
  fall behind; a page falls up to twice as far behind (8.4). It sets the
  duration of a sweep of that level.
- **fallback**: a lookup that cannot use its selected level and goes to a
  coarser level.
- **page budget**: the cost units that the scheduler can spend: the
  validity burst in one frame for the REQUIRED pages of the coarsest
  level, and the update budget for all other pages and for the missing
  pages of the coarsest level that the burst leaves, a balance of up to
  `batchFrames` frames that one frame may spend at once. A page costs
  the `pageCost` of its level (8.2).
- **classic static shadow**: the current raster `ToroidalStaticShadows`
  library.

## 2. Goal

Make one sun shadow structure that a shader can sample at any world
position: ray hits of GI and reflections, voxels, screen pixels.

The three requirements:
1. **Paged.** The unit of storage and of update is a page (122x122 texels
   in a 128x128 slot by default; the slot size and the window size are
   settings). Each page keeps one transform bit.
2. **Animated sun.** The sun moves all the time. The rtss follows it with
   sweeps at a bounded ray cost for each frame. There is no double buffer,
   and inside the window limit a lookup finds no gaps; a fast sweep above
   it can fall back for its frames (8.8).
3. **Producers.** The tracer callback builds pages with build rays (SWRT,
   HWRT or another tracer, 7.2). A raster callback rasterizes the scene
   page by page (7.3). A level can use both: it rasterizes a part of the
   casters, then traces from the raster hit (7.5).

The feature is a new library, `raytracedToroidalStaticShadow`. The
classic static shadow stays as it is. The first user is testGI (section
12).

### 2.1 Non-goals

- Moving objects. "Static" means the same as in the classic static
  shadow: static geometry. A change of static geometry goes through
  `invalidateBoxes`. "Possible extensions" at the top shows how moving
  objects could be added.
- Alpha-tested casters in SWRT, which has no alpha data; a raster recipe
  draws them (7.5).
- Translucent casters and canopy transmittance.
- Soft shadows wider than a 2x2 PCF at ray hits. A denoiser does the
  rest; the screen consumer has the filters of 9.7.
- A game integration (DNG, WT). testGI only.
- Console targets. The code must compile on all platforms that build
  testGI, but it is tested on PC DX12 and DX11 (HWRT on DX12 only).

## 3. Current state

### 3.1 Classic static shadow

- `prog/gameLibs/publicInclude/render/toroidalStaticShadows.h`,
  `prog/gameLibs/render/toroidalStaticShadows/`.
- Up to 4 cascades in a DEPTH16 texture array (the shaders take 2:
  `static_shadows_cascades`). A high sun uses a skewed
  matrix (texel xy follows world xz at a reference height). A low sun
  (`ORTHOGONAL_THRESHOLD`, `dir_to_sun.y < 0.1`) uses an ortho matrix.
- `setSunDir(dir, cos_force = 0.99, cos_lazy = 0.9999)`: a sun change
  above the lazy threshold (about 0.8 deg) invalidates the whole cascade.
  Above the force threshold (about 8 deg) the cascade also reads as empty
  until it is rendered again. An optional transition texture is a double
  buffer.
- Result for an animated sun: a full re-render of each cascade at each
  lazy step. This is the problem that this spec solves.
- Raster interface: `IStaticShadowsCB`
  (`prog/gameLibs/publicInclude/render/staticShadowsCB.h`):
  `startRenderStaticShadow(view, zn_zf, minHt, maxHt)`,
  `readyToRenderRegionWithFinalQuality(transform, region)`,
  `RasterizedResult renderStaticShadowDepth(culling_view_proj, transform, region)`,
  `endRenderStaticShadow()`.
- DNG uses a two-phase flow: `updateOrigin`, then cull jobs from
  `getRegionToRenderCount`/`getRegionToRenderCullTm`, then `render(cb)`
  (`prog/daNetGame/render/world/shadowsManager.cpp:779-839`). The raster
  producer keeps this flow (section 7.3).

### 3.2 SWRT (daBVH + daSWRT)

- C++: `RenderSWRT` in `prog/gameLibs/publicInclude/daSWRT/swBVH.h`.
  BLAS from collision meshes or boxes, TLAS of instances, upload to the
  global buffers `bvh_top_structures` and `bvh_bottom_structures`. Only
  one SWRT scene can be bound at a time.
- TLAS node boxes are FP16 in world space. No alpha test. Triangles are
  two-sided (`BACKFACE_CULLING 0`).
- Shaders: `prog/engine/daBVH/shaders/swBVH.dshl` (`INIT_SW_BVH_RAYTRACING`,
  `USE_SW_BVH_RAYTRACING`) and `swTLAS.dshl`:
  - `rayTLAS(o, d, dInv, maxT, out t, inout triNorm, inout bCoord, out complex)`:
    closest hit, stackless walk in node order. Only the current hit
    distance prunes it.
  - `rayTLASOrdered(...)`: the same result, nearest-first order. Faster
    for long rays that hit.
  - `rayShadowTLAS`, `rayShadowTLASEst`: any hit, no distance.
- No terrain is in SWRT.
- The only SW sun shadow today is the per-pixel screen pass
  `shadows_swrt_cs` in `prog/gameLibs/daSWRT/shaders/swRT.dshl`.

### 3.3 HWRT (bvh library)

- `prog/gameLibs/publicInclude/bvh/bvh.h`, shaders in
  `prog/gameLibs/bvh/shaders/bvh.dshl`.
- `bvh::bind_resources(ctx, w)` binds `bvh_main`, `bvh_terrain`,
  `bvh_lru_collision` and the other TLASes.
- HW rays are camera-relative: origin = world - `bvh_origin`.
- `trace_ray(bvh, origin, dir, min, max, ...)` (`bvh.dshl:1433`) gives the
  closest hit with alpha test. `bvhGroupHardShadow` has no terrain.
- The LRU collision TLAS (`connect_lru_collision`) streams collision
  meshes in a window around the camera, default radius 256 m.
  `LruCollisionStats::revision` changes when the traced content changes;
  `settled` is true when the window is complete (`bvh.h`).

### 3.4 Terrain trace

- `prog/gameLibs/heightmapTrace`: `HeightmapMaxGrid` (toroidal max height
  grid around the viewer) and
  `heightmap_max_grid_trace(pos, dir, max_t, below_hits_at_once, out hit_t, out back_face)`
  (`shaders/heightmap_max_grid.dshl`).
- The ray origin must be inside the grid window. There is no terrain
  outside the window.
- The grid shader variables are global, so only one grid can be active.
- `INIT_AND_USE_HEIGHTMAP_MAX_GRID(code)` gives the march the heightfield
  that fills the grid, `tex_hmap_low` of the base heightmap.

### 3.5 testGI with the rtss (section 12)

- One file does most of the work: `samples/testGI/prog/test_app.cpp`,
  class `DemoGameScene`. No daFG: a plain render loop.
- Sun: `setDirToSun()` from the `sun_panel` sliders or from the
  astronomical sliders. Then `dir_to_sun = daSkies.getPrimarySunDir()`,
  which can be the moon. The sun moves by itself only with
  `sun_panel.animate` (12.1).
- Static shadow: the rtss only (12), no classic static shadow.
- SWRT: the `swrt` member is built from `ri_collisions.bin` in the
  constructor. It covers the whole map. The console var
  `render.swrt_rebuild_around` makes the TLAS again around a point
  (`remakeSWRTInstances`).
- HWRT: one bvh context "GI" with the LRU collision TLAS (`initBvh`,
  `buildBvh`). testGI does not add terrain to bvh.
- Terrain for traces: `giHmapMaxGrid.init` of
  `render.hmap_max_grid_cells`, rounded up to a power of two, and
  `render.hmap_max_grid_cell_size`: 256 cells of 8 m by default, a 2 km
  window (12.4). The GI trace marches it with
  `INIT_AND_USE_HEIGHTMAP_MAX_GRID(code)` (3.4) in
  `shaders/source/project_trace.dshl`.
- Consumers of the static shadow (all use the rtss lookup, 12.5):
  - GI at ray hits: `SUN_LIGHT_POINT_TRACE_HIT` and
    `PROJECT_LIGHT_POINT_TRACE_HIT` in
    `shaders/source/project_light_point.dshl` (`dagi_get_sun_color`).
    daGI2 calls them from the irradiance grid, screen probes, radiance
    grid, volumetric GI and alternate reflections.
  - GI voxelization: `PROJECT_LIGHT_POINT_VOXELIZE` (CSM x static).
  - Screen: `combine_shadows` in `shaders/source/deferred_shadow.dshl`,
    and `deferred_simple` in the same file.
  - Skies: `shaders/source/project_skies_shadows.dshl`.
- GI update gate: `updateSSGIPos` waits until the static shadow is valid
  around the camera.
- Frame order (`drawScene`): `bvh::bind_resources`,
  `giHmapMaxGrid.update`, the rtss (12.3), `daGI2->beforeRender`,
  voxelization callbacks, `render()`. Inside `render()`: gbuffer, CSM
  and `combineShadows`, SWRT screen shadows, GI lighting
  (`daGI2->beforeFrameLit`), deferred resolve.

## 4. Design overview

```
 setSun(dir) --------------------+
 update(camera pos, velocity) ---+--> per level scheduler (CPU):
 invalidateBoxes(boxes) ---------+    windows, sweeps, invalidation,
                                      page budget
                                        |
                                        v  jobs: level, slot, rect,
                                        |        bit, zStart, zEnd
             +--------------------------+---------------------------+
             | trace                    | raster                    |
             | IRtssTraceCB, a compute  | IStaticShadowsCB, page by |
             | shader for each tracer,  | page into a scratch depth,|
             | a thread for each texel  | then rtss_resolve_cs      |
             +--------------------------+---------------------------+
                                        v
             atlas (L16 array) + slot table + level table
                                        v
             lookup: rtss_sun_visibility(X, N, dither)
```

Each frame the scheduler makes a job list. The producers fill slots. The
CPU then writes the slot table and the level table. All three
change at one point of the GPU timeline (section 9.1).

## 5. Data layout

### 5.1 Configuration

The defaults are those of `RaytracedToroidalStaticShadowSettings`. The
core structs of `core/rtssSettings.h` keep the defaults that the unit
tests were tuned on: 4 levels, a window of 16 slots of 64 texels with a
border of 1 texel, `texel0` 0.05 m, no anisotropy, an update budget of
24 with no batch (`batchFrames` 1) and a validity burst of 256.

| Parameter | Default | Meaning |
|---|---|---|
| `levels` | 5 | number of levels, 1..`RTSS_MAX_LEVELS` (12) |
| `windowPages` W | 8 | slots on each side of a level; a power of two, 4..32 |
| `slotTexels` S | 128 | texels on each side of a slot; a power of two, 32..128; the page is `P = S - 2B` |
| `borderTexels` B | 3 | texels of the slot border on each side, 1..4; a screen filter kernel stays within it (9.7) |
| `texel0` | 0.08 m | texel size of level 0, 0.001..1000 m |
| `maxAnisotropy` | 2 | largest y scale of light space for a low sun, 1..8; 1 turns it off (6.6) |
| `toleranceDeg0` | 0.1 | tolerance of level 0; it doubles for each level |
| `toleranceDegMax` | 1.6 | largest tolerance |
| `fastSweepDeg` | 5 | a sun step above this, or above 80% of the window limit, sweeps in the REQUIRED class (8.8) |
| `minSweepSeconds` | 0.1 | shortest sweep |
| `maxSweepSeconds` | 8 | longest sweep |
| `staticSweepSeconds` | 0.25 | sweep duration for a sun that does not move but changed |
| `maxCameraSpeed` | 343 m/s | the speed of sound; a faster camera velocity is a teleport, with no velocity (8.1) |
| `updatePagesPerFrame` | 3 | page budget in cost units for each frame for all pages but the REQUIRED pages of the coarsest level, and for its missing ones that the burst leaves (8.2); it accrues into a balance of up to `batchFrames` frames that one frame may spend at once, so the peak build cost of a frame outside the validity burst is `batchFrames` times it |
| `validityBurstPagesPerFrame` | 32 | page budget in cost units for the REQUIRED pages of the coarsest level; at 32 a coarsest level of 49 cube pages is complete in the second frame after a teleport out of its window |
| `batchFrames` | 4 | the update budget is a balance of up to this many frames: a frame with a positive balance may spend `batchFrames * updatePagesPerFrame` units outside the validity burst, and the frames after it build nothing from the update budget until the balance is positive again (8.2) |
| `seamBand` | 0.1 | outer fraction of a level cube where the lookup dithers to the next level, or where the coarsest level fades to lit (9.6) |
| `recipes[k]` | `None`, 1 | the build recipe of level k: `raster` and `pageCost`, 0.1..100 (7.5) |
| `rasterBatchPages` | 16 | raster levels: the scratch target is `ceil(sqrt(rasterBatchPages))` slots wide and as high, half as high but at least a slot with `rasterTranspose` (512 x 256 by default), 1..256; it caps the sides of a raster box (7.3) |
| `rasterBoxPagesPerJob` | 2 | a raster box holds at most this many pages for each of its jobs, 1..64 (7.3) |
| `rasterCallCost` | 0 | cost units of a raster call, which the job that opens a raster box pays (8.2) |
| `rasterTranspose` | true | the scratch is half as high, and a box that fits it only turned by 90 deg draws turned (7.3) |
| `maxCasterReach` | 2000 m | the least half-range of a slot's depth range at its whole-slot build, before the world box clips it, so the distance sunward of the camera to which casters count then; a level reaches them to twice its extent where twice the extent is more; a camera move toward the sun shrinks the reach of a kept slot, which a sub-slot job keeps (6.4) |
| `minSunY` | 0 | `dir_to_sun.y` below this: sun below horizon |
| `biasTexels` | 1.0 | depth bias in texels of the level |
| `normalBiasTexels` | 2.0 | normal offset in texels at the receiver distance, times `1 - NoL` (9.5) |
| `sunJumpDeg` | 1 | a larger sun step is a jump, as a time skip, with no sun rate (8.1) |

The defaults are those of the library API (section 10). The testGI
panel uses the same values. The scheduler clamps a value out of its
range, rounds W and S up to a power of two, and logs an error.

General values: page `P = S - 2B`, rays for each slot `S^2`, level side
`W * P` texels, selection extent `E_k / texel_k = (W/2 - 1) * P` texels
(section 6.5).

Derived values for the defaults (W = 8, S = 128, B = 3, P = 122, 5 levels):
- Level k: texel `0.08 * 2^k` m (8, 16, 32, 64, 128 cm), level side
  `976 * texel` (78, 156, 312, 625, 1249 m), selection extent
  `E_k` of 366 texels: 29, 59, 117, 234, 468 m.
- Rays for each slot: 128^2 = 16384. For each level: 64 slots, 1.05M
  rays. Full rebuild of 5 levels: 320 slots, 5.24M rays.
- Memory: 1024 x 1024 x 5 x L16 = 10.5 MB, against 16.8 MB for the
  classic static shadow that it replaces in testGI (2 cascades of 2048^2
  DEPTH16).

The examples in the rest of this spec use the defaults.

### 5.1.1 Choosing W and S

A receiver at light space distance r from the camera, past level 0,
gets a texel between `r / E` and `2r / E`, with E the selection extent
in texels; inside level 0 it gets `texel0`. So E is the resolution for
each unit of distance, and `W * P` alone does not set it: the
`(W/2 - 1)` margin (6.5) takes a part of each level.

Sweep cost for 5 levels, a 24-minute day, the default tolerances with
the clamp of 8.8, B = 1:

| W x S (P) | Level side | E (texels) | Memory for each level | Pages for each level | Sweep pages/frame | Sweep rays/frame |
|---|---|---|---|---|---|---|
| 8 x 64 (62) | 496 | 186 | 0.52 MB | 64 | 5.2 | 21.2k |
| 16 x 32 (30) | 480 | 210 | 0.52 MB | 256 | 20.8 | 21.3k |
| 16 x 64 (62) | 992 | 434 | 2.10 MB | 256 | 20.8 | 85.2k |
| 32 x 32 (30) | 960 | 450 | 2.10 MB | 1024 | 88.5 | 90.6k |
| 8 x 128 (126) | 1008 | 378 | 2.10 MB | 64 | 5.2 | 84.7k |

- At the same memory and rays and B = 1, smaller pages give a larger E:
  the margin is one page on each side of the camera, so a large page
  loses more. A border takes `2B` texels of each page, so half the S at
  twice the W changes E by `S / 2 - W * B` texels: at B = 3, 32 x 32
  gives E = 390 and 16 x 64 406. They trace more border rays (S^2/P^2 - 1 at
  B = 1: 14% at S = 32, 7% at S = 64, 3% at S = 128), and they have a
  larger slot table, a smaller unit of update, and more callback calls
  for the raster producer.
- The large-step limit of 8.8 gets smaller as W grows (7.88 deg at
  W = 8, 3.31 deg at W = 16, 1.54 deg at W = 32).
- The default B = 3 for the screen filters (9.7) makes P = 122 at
  S = 128: E is 366 texels in place of 378, with the same rays and
  memory.
- testGI measured 16 x 64 (budgets 12 and 128) against 8 x 128 (3 and
  32), the same rays for each frame, on battle_of_berlin and
  dalniy_island: 8 x 128 takes half the CPU, 1.3 to 2 times less GPU
  for the raster and 5 to 55 % less for SWRT, with the same validity,
  for an E of 366 texels in place of 406. So the defaults are 8 x 128.

### 5.2 Atlas

- `rtss_atlas`: Texture2DArray, L16, `TEXCF_UNORDERED`, size
  `W*S x W*S x levels` (1024 x 1024 x 5, 10.5 MB, by default).
- Slot s = (x, y) of level k is the texel rectangle
  `[s*S, s*S + S)` of layer k. Slot texel (B, B) is page texel (0, 0).
- A texel keeps the light space z of the first hit in the transform of
  its slot, normalized over the depth range of the slot:
  `(z - minDepth) / (maxDepth - minDepth)` (5.3, 6.4). Only a whole-slot
  job sets that range, and each slot decodes with its own, so a camera
  move along the sun axis needs no depth scroll (8.6). A miss keeps
  `RTSS_MISS = 1`, the far end of the range.
- The depth step is the range over 65535. 6.4 keeps the range of a job
  at rest at most `2 * max(maxCasterReach, 2 * cube depth)`: 4 km for
  the finer levels at the defaults, a step of at most 6.1 cm, under the
  depth bias of one level 0 texel (8 cm, 9.5), and an error of +- 3 cm,
  under half a texel (4 cm). A new whole-slot job outside a sweep of a
  level past `maxCasterReach` has a range of at most four times its
  extent before the world box clips it (a sweep widens the range, and
  a slot keeps the range it was built with, 6.4), and the extent grows
  with the texel, so the step is a fraction of the level texel that the
  layout fixes: about a 45th at a window of 8 pages of 122 texels, about
  a ninth at 32 pages of 126 texels. In testGI the step is at most 2.2 cm.

### 5.3 Slot table

```cpp
struct RtssSlot         // 16 bytes, levels * W * W entries
{
  uint page;            // page coordinate in the transform of `bit`, uint(x) & 0xffff | uint(y) << 16
  uint flags;           // RTSS_SLOT_VALID | RTSS_SLOT_BIT (the transform bit, which rtss_slot_bit reads)
  float minDepth;       // light space z of the job's zStart
  float maxDepth;       // light space z of the job's zEnd
};
```

- The slot keeps the low 16 bits of each page axis, and the lookup
  compares them with the low 16 bits of its page. A slot holds a page
  near its window, and two pages with the same low 16 bits are the same
  page or 65536 pages apart, so the compare is exact at any distance
  from the origin.

- The explicit page coordinate replaces the window bookkeeping of the
  lookup, which a window change during a sweep would break: the slot test
  does not depend on the window. The lookup reads the window origin only
  as the page origin of the level table, to make the page index.
- CPU slot states: EMPTY, VALID, DIRTY. DIRTY data is readable; the GPU
  sees DIRTY as valid. The CPU copy of a slot also keeps the `zStart` and
  `zEnd` of its last whole-slot job.
- **A page change is atomic.** A job that changes the page coordinate or
  the bit of a slot always covers the whole slot, in one frame. Only an
  invalidation (same page, same bit) can use a sub-slot rectangle.
- **One job for each slot and frame.** A whole-slot job removes the
  pending DIRTY rectangle of its slot. A sub-slot job uses the `zStart`
  and `zEnd` of its slot, so it has the same depth range as the rest of
  the slot and `minDepth`/`maxDepth` stay true.
- A sweep job on a slot replaces the DIRTY job of that slot in the same
  frame: it rebuilds the whole slot, so it also removes the rectangle.
- The CPU writes the table after it records the jobs of the frame, as one
  buffer update.

### 5.4 Level table

For each level, 112 bytes in one GPU buffer (`rtss_levels`), updated
each frame:
- `T[2]`: two transforms, 3 float4 each: the rows of M in xyz and the
  relative translation k (5.5) in w. M has orthogonal rows, with the y scale
  of 6.6 in its second row, so the inverse is `M^T` after the second row
  of M is divided by its squared length, and `d` is the third row of M. There is
  no separate inverse matrix.
- One more row: `pageOrigin`, the window origin o of the level (int2, in
  pages), the flags `newestBit` (target bit when sweeping, else current
  bit), `currentBit` and `sweeping`, and `1/(P*texel)`. A tap reads all
  of them with one load.

The texel of level k is `texel_0 * 2^k` (1), so only `rtss_globals`
keeps it. The slot and the level tables are ByteAddressBuffers, which
the shaders read with one `Load4` for a slot or a row: the strides are
multiples of 16 bytes.

Global values, one array `float4 rtss_globals[5]` set in each frame;
`rtss_common.hlsli` names its parts:
- `rtss_params_i` (`int4` of element 0): level count, `windowPages` W
  for the slot mask `W - 1`, the slot side S for the atlas stride
  `S * W`, the page side P. Small integers, exact in float.
- `rtss_params`: `1/E_0`, the "sun below horizon" flag,
  the current bit of level 0, `1/seamBand` (1e30 for a `seamBand` of
  0).
- `rtss_origin` (xyz): the camera position of the frame; w is
  `rtss_select_y`, the smallest `sy` of the live transforms of all
  levels over the `sy` of level 0, at most 1 (6.6).
- `rtss_bias` (xy): `biasTexels`, `normalBiasTexels`; z is
  `rtss_center_y`, the height of the window center over `rtss_origin`
  (8.1); w is `rtss_texel0`, the texel of level 0.
- `rtss_atlas_texel` (x of element 4): `1/(S * W)`, so the lookup goes
  from atlas texels to uv in one madd.

### 5.5 Precision and large coordinates

- Positions can be large, so the CPU keeps them in double: the camera,
  the pivots, the translation c of each transform, the world box.
  Directions and bases (e_x, e_y, d, u) are unit vectors and stay float.
- The GPU gets only small values. For transform b of level k, the CPU
  computes in double each frame
  `k_b = M_b * origin + c_b - (o_k * P * texel_k, z0_b)` with
  `origin = rtss_origin`, and stores it as float. The shader computes
  `ls = M_b * (X - rtss_origin) + k_b`: light space relative to the
  window origin in xy and to `z0_b` in z. The page coordinate is
  `o_k + floor(ls.xy / (P * texel_k))`, in integers.
- `z0_b` is the camera depth in transform b when b is made. It does not
  change while b lives. The atlas texels and the slot depths are relative
  to it, so they stay small, and a camera move along d still changes
  nothing (8.6).
- So the library loses no precision at large coordinates. The limits are
  the positions that come in as float: a consumer's world position and
  the world box (`BBox3`) far from the world origin have their own
  error. A consumer that has a camera-relative position (`-pointToEye`
  in `combine_shadows`) uses the `_rel` functions of 9.2 and loses
  nothing.
- SWRT keeps its own limit: its TLAS boxes are FP16 in world space and
  its leaf origins are float (3.2). The build rays of both tracers start
  at `rtss_origin + M^-1 * (ls - k)` in world space, so far from the
  world origin a ray origin rounds to the float spacing there (8 mm at
  100 km); the HWRT tracer takes that world origin too.

## 6. Transforms

### 6.1 First transform

When a level has no transform (first build, reset): `d = -dir_to_sun`,
`e_x = normalize(cross(Y, d))`, with X in place of Y when d is near vertical,
`e_y = cross(d, e_x)`, `c = 0`. With `c = 0` the page grid is fixed to
the world origin in light space, so it does not depend on the camera.
Light space y is `sy * dot(e_y, X)`, with the y scale of 6.6; M keeps
e_y unit length, and only the GPU rows hold `sy * e_y`.

### 6.2 Next transform: no roll, anchored at the pivot

M is float, and c and the pivot are positions in double (5.5).
`next_transform` makes the new rows orthonormal before it computes c, so
a live transform never changes after that. u comes from the bases, not
from the difference `T_new(X) - T_old(X)` of a world point: that
difference of two large values would lose the small shift.

```cpp
// rows of M: e_x, e_y, d (float, orthonormal); light space y is sy * dot(e_y, X); c (DPoint3)
LightXform next_transform(const LightXform &old, const Point3 &dir_to_sun, const DPoint3 &pivot, float sy)
{
  Point3 d_new = -dir_to_sun;
  Quat r = quat_rotation_arc(old.M.d, d_new);  // minimal rotation, no roll; dag_Quat.h
  LightXform n = {};                           // c = 0 until the pivot sets it
  n.M.d = normalize(d_new);
  n.M.e_y = sy == old.sy ? r * old.M.e_y : (abs(n.M.d.y) > 0.999 ? X : Y); // a new sy: e_y from Y again (6.6)
  orthonormalize(n.M);                         // e_x from e_y and d; before c, or the pivot moves
  n.sy = sy;
  n.c = old.toLs(pivot) - n.toLs(pivot);       // double: T_new(pivot) == T_old(pivot)
  return n;
}
// d is d_new, not r * old.M.d: above 179.19 deg quat_rotation_arc turns exactly 180 deg
// u = normalize(Point2(dot(n.M.e_x, old.M.d), n.sy * dot(n.M.e_y, old.M.d)))
```

- The rotation axis is in the old light plane. The light space component
  along the axis does not change, so a receiver moves exactly along u,
  for any angle:
  `T_new(X).xy = T_old(X).xy + (z * sin(theta) - s * (1 - cos(theta))) * u`,
  with z the depth below the pivot and s the position along u from the
  pivot (both in the old transform). Both terms are for `sy = 1`; with
  the y scale of 6.6 the z term is `sy` times longer along y, and the
  second term is not `s` scaled along u either. The second term is of
  the second order in the angle; it is along u too, so it does not
  move a receiver sideways. Section 8.8 uses this formula for the step
  limits.
- The minimal rotation has no defined axis near 180 deg.
  `quat_rotation_arc` (`dag_Quat.h:194-230`) already turns 180 deg about
  a perpendicular axis there.
- When `theta` is so small that u is not defined, no sweep starts.
- With the y scale of 6.6, `sy` multiplies the e_y term of u. A new
  scale makes e_y from Y again (6.6); c comes from the pivot in both
  cases.

### 6.3 Pivot

- At the start of a sweep of level k: `O = camera - H_k * d_old`.
- `H_k = E_k * (1 + max(0.5, 1.25 * |v_cam| * sweepDuration_k / E_k))`.
  With a slow camera `H_k = 1.5 * E_k`. A fast camera gets a larger H, so
  that it stays below the pivot for the sweep; above `maxCameraSpeed` it
  is a teleport with no velocity (8.1), so 1.5. `sweepDuration_k` is
  always finite (8.4).
- Level k serves receivers with `|z - z_cam| <= E_k`, so these receivers
  have `z >= H_k - E_k >= 0.5 * E_k` below the pivot.

### 6.4 Start plane and depth range of a job

In the transform of the job, in double (the job and the slot table store
them relative to `z0` of that transform, 5.5):
- `h = max(maxCasterReach, 2 * r)`, where `r = E_k` is the depth of the
  receivers that the level cube needs (6.3, 8.6).
- `zStart = max(zMin(worldBox), z_cam - h)`. `zMin` is the smallest z
  of the 8 corners of the caster world box. Without a world box (before
  the first `setWorldBox`), `zStart` is `z_cam - h`.
- `zEnd = max(zStart, min(zMax(worldBox), z_cam + h))`. Without a world
  box, `zEnd = z_cam + h`. A camera past the far face of the box by more
  than `h` gets an empty range: no caster, `maxT = 0`.
- `h` is at least twice `r`, so the range keeps a margin of at least `r`
  over the depths that the cube needs on each side, and of at least
  half `maxCasterReach` on a level within it: a camera move along the
  sun of less than the margin makes no slot old (8.6). The clamp of the
  deep side keeps the L16 depth step small (5.2) on a deep world box.
- During a sweep a job of the target transform uses as `r` the larger
  of `r` and the z half-range of the image of the current cube (8.2):
  `E_k * (|d_t . e_x| + |d_t . e_y| / sy + |d_t . d|)`. A receiver at
  the depth edge then keeps its target slot.
- With a world box the range of a job depends on the camera depth only
  where the clamps by `h` bind (kilometers). So a camera move along the
  sun makes a slot old only where a clamp binds, after a move of `h - r`
  (8.6). Until then the slot keeps its start plane, so the reach of the
  receivers near the top of the cube shrinks by a move toward the sun:
  casters that the move brings into reach are not in the slot yet.
- Only a whole-slot job computes a new `zStart` and `zEnd`. A sub-slot
  job uses the values of its slot (5.3).
- The build ray starts at `(xy, zStart)` and has `maxT = zEnd - zStart`.
- The slot keeps `minDepth = zStart` and `maxDepth = zEnd`. A lookup
  rejects a receiver outside `[minDepth, maxDepth]`: the build ray saw no
  casters of it there.
- A long `maxT` costs little: a closest-hit walk skips the nodes past
  its nearest hit so far, and most build rays hit the ground early.

### 6.5 Selection extent

`E_k = (W/2 - 1) * P * texel_k`. The window scroll (section 8.6) keeps
the camera in the central 2x2 pages, so a cube of half size E_k around
the camera is always inside the window. An extent of half the window
would make the window edge a frequent fallback.

### 6.6 Anisotropic texels

At a low sun a square light space texel is long on the ground:
`1 / sin(elevation)` texels along the sun azimuth, one texel across it.
With `maxAnisotropy` above 1, light space y is `sy * dot(e_y, X)`, so
that a ground texel is near square while e_y is near the up direction:
- `sy` is `1 / max(sin(elevation), 1 / maxAnisotropy)`, rounded to a
  power of `sqrt(2)`, at most `maxAnisotropy` (`rtss_aniso_scale`). So at
  or below the horizon it is `maxAnisotropy`, as at sunrise, and the
  transform of a night start needs no sweep to a new `sy` when the sun
  rises. The GPU rows hold `sy * e_y` as the second row, so the lookup
  reads no new value.
- A sweep that keeps `sy` is the no-roll sweep of 6.2. A sweep to a new
  `sy` makes e_y from Y again, as 6.1 does, and c from
  `T_new(pivot) == T_old(pivot)`. The page grid changes, not only a
  shift along u, so that sweep is always fast (8.8), and the key order
  does not hold for it: during it a receiver anywhere in the cube can
  fall back, and the coarsest level reads lit there until the sweep
  ends. With steps of `sqrt(2)` the sun crosses a step a few times a
  day, below 57 deg.
- Between the steps e_y turns with the no-roll rotation, so a ground
  texel is square only near a step.
- The levels take a new `sy` one at a time, each at its own sweep, so
  for a while a coarser level can hold a smaller `sy` than level 0. The
  level selection (9.3) scales light space y of level 0 by
  `rtss_select_y`, so it never selects past a level whose own cube
  holds the receiver; a finer level that it selects and that does not
  hold the receiver misses, and the lookup reads the next one.
- The shift of 6.2 is up to `sy` times longer in texels, so the window
  limit of 8.8 and the pre-trace crossing time along y use `sy`.

## 7. Page builds

### 7.1 Jobs

```cpp
struct RtssJob          // 32 bytes
{
  uint levelSlot;       // level << 16 | slot y << 8 | slot x
  uint flags;           // bit 0: transform bit
  int2 page;
  uint rect;            // x0, y0, x1, y1 in slot texels (0..S), 8 bits each
  float zStart, zEnd;
  uint rasterTile;      // x | y << 16 | RTSS_TILE_TRANSPOSED: the scratch texel of the slot window of a raster job, ~0u for others
};
```

- The CPU cuts each job into groups of 64 texels of its rectangle. One
  group entry is `jobIndex << 16 | firstTexel / 64`. S is a power of
  two, so a whole slot is exactly `S^2 / 64` groups (256 by default) and
  all threads do work. When both sides of the rectangle are multiples of
  8, as a whole slot is, a group is an 8 x 8 tile (`rtss_group_texel`),
  else 64 texels in row-major order. The parallel rays of a tile walk
  the SWRT tree together: in testGI a tile builds the costly levels 13
  to 41 % faster than a row of 64.
- One trailing trace batch for each frame builds the trace jobs; the
  `Augment` jobs of a raster batch trace in a batch after it (7.5).

### 7.2 Trace producer: the tracer callback

The trace is project code as much as the raster is: SWRT, HWRT, a media
or an SDF tracer, and a project can have more than one. So the tracer
is a C++ callback, as the raster is (7.3): each tracer is a compute
shader that the project dispatches over the texels of the jobs.

```cpp
struct RtssTraceBatch
{
  uint32_t levelGroups[RTSS_MAX_LEVELS + 1]; // the groups of level k are [levelGroups[k], levelGroups[k + 1])
  BaseTexture *atlas;
  // dispatches cs over the groups of the levels of level_mask; false for an invalid cs or a failed dispatch
  bool dispatch(const ComputeShader &cs, uint32_t level_mask) const;
};

class IRtssTraceCB
{
public:
  virtual bool traceJobs(const RtssTraceBatch &batch) = 0; // true only when the groups of all levels were dispatched
};
```

- **Batch.** `render()` calls `traceJobs` once for each trace batch
  with groups (7.1, 7.5). Before the call it uploads the groups of the batch to
  `rtss_groups`, sorted by the level of their job, and sets
  `rtss_job_count`, `rtss_build_augment` and the scratch; the atlas, the
  jobs, the level table and the globals are global shader vars too. So
  the callback only binds its scene and dispatches. `dispatch` sets
  `rtss_group_offset`, the index in `rtss_groups` of the first group it
  dispatches, which `rtss_trace_texel` adds to the group id (a callback
  that dispatches by hand sets it too), and dispatches a run
  of consecutive levels of the mask at once, in parts of at most
  65535 groups.
- **Levels.** The callback decides which tracer builds which level:
  HW for the fine levels and SWRT for the coarse ones is two shaders
  over two level masks. The library gives no tracer ids, so a project
  that changes the tracer of a level invalidates the level (7.5).
- **Tracer shader.** `RTSS_TRACE_TEXEL(code)` of
  `rtss_trace_common.dshl` gives the texel and the store:

```hlsl
[numthreads(RTSS_BUILD_GROUP, 1, 1)] // one thread for each slot texel of a job
void my_tracer(uint3 gid : SV_GroupID, uint tid : SV_GroupThreadID)
{
  RtssTraceTexel tx;
  if (!rtss_trace_texel(gid.x, tid, tx))  // false past the rectangle of the job
    return;
  float t = tx.t;                         // the ray end, or the raster hit of an Augment job (7.5)
  my_trace(tx.origin, tx.dir, t);         // makes t the nearest hit
  rtss_trace_store(tx, t);                // t / (zEnd - zStart) when t < tx.maxT, else RTSS_MISS
}
```

  The build ray of `rtss_trace_texel`:

```hlsl
RtssJob job = rtss_jobs[entry >> 16];
uint idx = (entry & 0xffff) * 64 + tid;
int2 t = rect_texel(job, idx);                         // 0..S-1
float2 xy = ((job.page - pageOrigin(level)) * P + (t - B) + 0.5) * texel(level); // relative, 5.5
float3 origin = rtss_origin + rtss_from_ls(level, bit, float3(xy, job.zStart));  // M^-1 * (ls - k)
float3 dir = rtss_light_dir(level, bit);
float maxT = job.zEnd - job.zStart;
```

  Each tracer shader has its own compile target.
- Border texels are traced exactly like the interior texels, with the
  transform of the slot.
- Build rays are two-sided (no back face cull): a single-sheet caster
  must occlude from both sides.
- **Library tracers**, optional. `RtssComputeTracer(cs_name, level_mask)`
  is the `IRtssTraceCB` of one compute shader for the levels of its
  `levelMask`. The shader is optional: with no shader in the dump the
  tracer can have no levels, else `dispatch` logs an error once and
  fails. A failed build skips the `commit`, logs once and resets the
  scheduler, as its dispatches that ran wrote texels under the slots of
  the old table: `render()` uploads the slot table of the reset, with
  all slots EMPTY, so a lookup finds no data (or a level count of 0 when
  that upload fails, 9.1). The library has two shaders for it. Neither
  is in `_shaders.blk`, as they need the daSWRT, bvh and heightmapTrace
  shaders: a project lists the ones it uses.
  - `rtss_trace_swrt_cs` (`rtss_trace_swrt.dshl`, `target_cs`):
    `rayTLAS` on `bvh_top_structures` and `bvh_bottom_structures` of
    daSWRT (closest hit, t in world units), with
    `BVH_TLAS_DECODE_REFLECTION` 1, because a TLAS of raw instance
    transforms holds mirrored instances, and `SWRT_LEAF_BOX_HALF_EXTENT`
    set for each texel to `texel / (2 * sy)` of its level: an instance
    whose largest half extent is at most half the texel's extent along
    `e_y` hits as its box (a footprint under one texel along the light
    axes, up to `sqrt(3)` on a diagonal; 12 views differ in 0.002 % of
    their pixels). It is faster than `rayTLASOrdered` in testGI.
  - `rtss_trace_hwrt_cs` (`rtss_trace_hwrt.dshl`, `cs_6_5`; with no
    `use_hw_raytracing` or with `compatibility_mode` on it is not made,
    so a project that lists it declares both, as for `bvh_debug`):
    `trace_ray(bvhMain, origin - bvh_origin, ...,
    bvhGroupHardShadow, ...)`, closest hit, two-sided, with the alpha
    test of the main TLAS. With no valid TLAS a HW ray misses and its
    page reads lit until rebuilt, so a project traces HW only while its
    TLAS is valid, as testGI does (12.4).
  - Both then march the terrain with `rtss_trace_hmap(origin, dir,
    inout t)` of `RTSS_TRACE_HMAP(code)` (`rtss_trace_hmap.dshl`): the
    heightmap max grid of 3.4. The ray is first clipped to the grid
    window, as the origin must be inside it, and a ray down starts where
    it reaches the top of the height encoding (`decode_height` of 0 and
    1): a build ray starts at the top of the world box, and without this
    its march visits the empty cells above the terrain one by one, each a
    dependent read. In testGI the jump makes a traced page 3.7 times
    faster, as fast as with no terrain at all. Then its own test compares
    the start with `heightmap_max_grid_get_height`: a start on or inside
    the terrain hits at the start, so a start plane inside a hill counts
    the hill, and only a start above the terrain runs the march, with
    `below_hits_at_once` false. With no grid or no `tex_hmap_low` it
    finds nothing. The project updates its `HeightmapMaxGrid`, and a
    change of the grid invalidates the rtss: the pages hold the terrain
    of the old grid. A project tracer can use `RTSS_TRACE_HMAP` too, as
    testGI does (12.4).

### 7.3 Raster producer

The raster producer rasterizes the scene page by page through
`IStaticShadowsCB`, for the levels whose recipe has a raster (`All` or
`Augment`, 7.5). `renderStaticShadowDepth` returns whether it drew
anything (7.5).

- The unit is a whole slot: an invalidation rectangle becomes a
  whole-slot job on a raster level.
- **Scratch target.** A D16 depth target `ceil(sqrt(rasterBatchPages))
  * S` texels wide, and half as high but at least S with
  `rasterTranspose` (512 x 256 by default); D32S8 on a driver with the
  render pass clear race, as the classic static shadow. A cull group
  has the depth range of its slots and the atlas keeps 16 bits of that
  range (5.2), so D16 loses nothing. The library owns it. Clear value 1.
  Each cull group (below) draws once into its own region: a page box of
  w x h pages takes `w * P + 2B` x `h * P + 2B` texels, as neighbor
  pages share their border texels. A box fits when its long side fits
  the width and its short side the height: a region that is too high is
  turned by 90 deg, so light x runs down its rows. The turn is a
  rotation of the projection, not a mirror, so the triangles keep their
  winding. The plan fills the scratch with regions in shelves. When a
  frame has more regions than one scratch holds, the producer draws and
  resolves one batch after the other.
- **Budget.** A raster page costs the `pageCost` of its level in the
  budgets of 8.2. testGI sets a raster page at 1 unit: at 128 texels
  it costs 31 us GPU against 18 traced on battle_of_berlin and 41
  against 99 on dalniy_island; an `Augment` page at 1.5. The job that
  opens a cull group also pays `rasterCallCost` (8.2).
- **Groups.** Jobs are grouped by (level, bit). Each part of a group in
  one batch is one
  `startRenderStaticShadow` ... `endRenderStaticShadow` pair.
  `startRenderStaticShadow` gets the orthonormal light view of the
  transform (the rows `e_x`, `e_y`, `d` of M without the y scale of 6.6,
  stored as `light_dir_ortho_tm` stores its rows: the slots `m[0][i]`,
  `m[1][i]`, `m[2][i]` hold row i; and the translation
  `(c.x - o.x * page_world, (c.y - o.y * page_world) / sy, c.z - z0)`,
  with `o` the window origin and `page_world = P * texel_k`, so that its
  z is the z relative to `z0` of the slots, 5.5). A callback can invert
  it as a light view (`orthonormalized_inverse`) and gets the true light
  direction. It also gets `zn_zf = (zStart, zEnd)` of the group, and the
  world box heights.
- **Cull groups.** A callback call is a scene cull and draw, about 24 us
  CPU in testGI for one page of 64 texels and about 50 us for a box of
  four, and a page prism is long and thin at a low sun, so a call for
  each page visits most of the tiles along it again. The plan cuts each
  group (level, bit) into cull groups: in plan order, a job joins the
  first cull group whose page box, with the page of the job, has at most
  `rasterBoxPagesPerJob` pages for each job and fits the scratch
  (`rtss_box_takes`), else it starts a new one. The scheduler prices the
  calls with the same rule (8.2), so each cull group is a call that the
  budget paid; only a new page that a later sweep head of the frame
  moves to the target bit can open one more. Pages spread over the
  window become small cull groups, as a draw of their union would cover
  the whole window. The sweep pages of a row (8.4) fill one cull group
  after another, each as far as the box takes them.
- **One cull group.** One callback call draws the page box with its
  border (`rtss_raster_group_rect`: the smallest light space box that
  holds the rectangle and the depth range of each job). Viewport = the
  region of the group. Projection = ortho over light space x, y in
  `[((box0 - o) * P - B) * texel, ((box1 + 1 - o) * P + B) * texel)`,
  with `o` the window origin and `B` the border, and z in
  `[zStart, zEnd]`, to depth `[0, 1]`. The y range of the projection is
  divided by `sy`, as the view has no y scale.
  `renderStaticShadowDepth(cull, transform, region)` gets
  `cull = globtm` (the start plane is already sunward of all casters, so
  the far plane needs no extension) and `region` = the index of the
  cull group. `transform` holds the region as its viewport (`x`, `y`,
  `w`, `h`), the same matrix in `globtm`, the depth range `minz` 0 and
  `maxz` 1, and `cascade = level`. The slot window of a job starts at
  the scratch texel `rasterTile` (x | y << 16): the region origin plus
  `((page.x - box0.x) * P, (box1.y - page.y) * P)`, as the viewport row
  grows down; in a turned region `RTSS_TILE_TRANSPOSED` is set and the
  offset is `((page.y - box0.y) * P, (page.x - box0.x) * P)`.
  `rtss_raster_texel` gives the scratch texel of slot texel t:
  `(t.x, S - 1 - t.y)` from that start, or `t.yx` when turned.
- **Readiness.** When `readyToRenderRegionWithFinalQuality` is false for
  a cull group, for each of its jobs:
  skip the job if its slot holds the page with the bit of the job
  (`RTSS_JOB_SAME_PAGE`); else build it as provisional, and `commit`
  marks the slot DIRTY for a whole rebuild. A receiver of a new page
  or bit finds no data otherwise. No sweep job keeps the page and the
  bit, so no sweep job is skipped (8.2).
- **Resolve.** After the draws of each batch, `rtss_resolve_cs` runs an
  S x S grid for each `All` job (z: the job), reads the scratch depth in
  the slot window of the job and writes the slot in the atlas (the
  `Augment` jobs trace, 7.5):
  the scratch depth is linear (ortho) over `[zStart, zEnd]`, the range
  of the slot, so it is the atlas value as it is (5.2). Depth 1 is no
  hit, `RTSS_MISS`: with D16 also a caster in the last 1/65535 of the
  range, a few centimeters at the far end. Then a barrier makes the
  scratch a render target again for the next batch. A failed dispatch
  resets the scheduler, as for the trace producer.
- **Casters.** On an `All` page the callback draws all the casters, the
  terrain too, as the classic static shadow does: the resolve adds
  nothing. A terrain draw costs a heightmap cull for each cull group.
- **State.** The producer sets an override state with Z clamp, a
  LESS_EQUAL depth compare (the scratch clears to 1, the far end) and no
  depth bias around its draws, so the callback sets none of its own. A
  box spans only the depth range of its pages, so a caster nearer than
  it must clamp, not clip. The lookup applies the bias (section 9.5).

### 7.4 Barriers and order

- The tracer dispatches and the resolve write `rtss_atlas` as a UAV,
  each to the texels of its own jobs, so they need no barrier between
  them: `RtssTraceBatch::dispatch` gives the atlas `RB_NONE` before each
  run, so DX12 does not add its UAV barrier. In testGI that takes about
  20 us GPU from a frame with two tracers (the `hwrt` preset). Then one
  barrier makes it a SRV for the vertex, pixel and compute stages.
- The level table and the globals are written before the dispatches,
  because the build reads the transforms of this frame. The slot table
  is written after the barrier. All three are in the same `render()`
  call.

### 7.5 Build recipes and project tracers

Each level has a build recipe. The library knows how a page is built;
which tracer traces it (SWRT, HWRT, an SDF tracer) is the project's
choice (7.2).

**Recipe of a level.** The project gives each level k:
- `raster`: `None`, `All` or `Augment`.
  - `None`: the tracer callback builds the page (7.2).
  - `All`: the raster producer builds the page (7.3), with no trace.
  - `Augment`: the raster callback draws a subset of the casters (for
    example trees, which a collision scene does not have) into the
    scratch, and then the tracer callback builds the page from the
    raster hit. The raster callback gets `transform.cascade = level`
    (7.3), so it knows the recipe of the job.
- `pageCost`: the budget weight of one page of the level (8.2). A page
  costs at most the smaller of `updatePagesPerFrame` and
  `validityBurstPagesPerFrame`, so that it fits a budget of one frame:
  a level at a higher cost builds one page for each frame on average,
  up to `batchFrames` at once. Both budgets count cost units: with `c`
  that capped cost, a whole level costs `W * W * c`, so a fast sweep of
  the coarsest level takes `W * W * c / validityBurstPagesPerFrame`
  frames and of a finer one `W * W * c / updatePagesPerFrame` frames;
  the missing pages of the coarsest level take the update budget too
  (8.2).

A `raster` change of a level invalidates that level only (8.7) and
drops the plan of `update()`. A `pageCost` change applies from the next
`update()`, as the budgets do.

**Tracer of a level.** The tracer callback selects the tracer of each
level (7.2). The library does not see that choice, so a project that
changes the tracer of a level invalidates the level: `invalidate()`, or
`invalidateBoxes` with the world box and the level mask of that level. A job
of a traced level (`None` or `Augment`) and no tracer callback is as a
raster job and no raster callback: `render()` logs an error once and
builds nothing. A project with `All` levels only needs no tracer.

**Augment order.** For each raster batch:
1. The callback draws the batch into the scratch. For an `Augment` cull
   group that drew nothing, `renderStaticShadowDepth` returns
   `RasterizedResult::NothingRendered`. That report moves its pages to
   the trailing trace batch: a trace for each batch waits for its
   slowest rays, and an `Augment` level with no casters to draw costs
   2.0 to 2.7 times the SWRT time without the report, 1.0 to 1.5 with
   it in testGI.
2. The resolve writes the `All` jobs only: `rtss_raster_tiles` lists no
   `Augment` job.
3. A trace batch traces the other `Augment` jobs of the batch with the
   scratch bound (`rtss_build_augment` 1). `rtss_trace_texel` reads the
   pixel `rtss_raster_texel` of its slot texel, as the resolve does. A
   depth below 1 gives
   `tx.t = depth * (zEnd - zStart)`: the raster hit is the far limit of
   the trace, and the ray starts on the start plane as always.

After the batches, one trace batch with no scratch read builds the
`None` jobs and the empty `Augment` jobs. A skipped job (the
readiness rule of 7.3) is in no dispatch. `RtssJob::rasterTile` holds
the tile of a raster job, `~0u` for the others. A DIRTY
rectangle becomes whole-slot on an `All` and an `Augment` level: the
raster draws the whole tile, so the job carries a whole `pageCost`, and
the raster jobs of one level and bit keep one depth range (7.3).

## 8. Scheduler

### 8.1 Inputs for each frame

- `setSun(dir_to_sun)`: the library gives the scheduler the sun and its
  angular velocity, an axis-angle rate in rad/s whose length sets the
  sweep duration (8.4). A changed sun moves at its step over the update
  time since the last change, as a game sun can change in steps of many
  frames: War Thunder steps its shadow sun by 0.044 deg, and the rate of
  one step in one frame would plan a sweep in a small part of its time.
  A step above `sunJumpDeg` is a jump, as a time skip, and a sun that
  does not change has no rate. Before the first call the sun is straight
  up, `(0, 1, 0)`, and does not move.
- `update(camera_pos, camera_velocity, dt)`: plans the jobs of the frame.
  A velocity above `maxCameraSpeed` is a teleport, whose velocity the
  plan takes as zero: else the pivot of a sweep that starts in that frame
  goes far out (6.3) and makes the sweep fast. The window still moves to
  the camera, and its new pages are REQUIRED (8.6). The raster
  boxes of the plan (7.3) are then known: from `update()` to `render()`,
  `getRasterRegion(r)` gives the transform that `render()` passes to the
  raster callback with region `r`, so an integration can cull the boxes
  ahead, on worker threads, as the classic static shadow does with its
  regions. A call that drops the plan, such as `invalidate()` or
  `invalidateBoxes()`, also drops the regions, and `render()` then draws
  none.
- The window center is the camera with its height clamped to the y range
  of the world box. No caster or receiver lies above or below the box, so
  a camera that climbs or falls outside it moves no window and plans no
  work. The lookup measures the selection cube from that center
  (`rtss_center_y`, 5.4).

### 8.2 Page budget and job order

- The unit of the budget is a cost unit. A whole-slot job costs the
  `pageCost` of its level (7.5), capped at `updatePagesPerFrame` and
  `validityBurstPagesPerFrame` so that it fits each budget of one frame;
  a sub-slot job costs its area divided by `S^2` times that. At cost 1,
  one page of a trace level is `S^2` build rays.
- A raster call costs its scene cull and draw, whatever its pages
  (7.3): about 50 us CPU of rendinst draws in War Thunder. With
  `rasterCallCost` above 0, the plan puts each raster job in the first
  box of its level and transform bit that `rtss_box_takes` lets it
  join, else it opens a box, and the job that opens a box pays the call
  too. The call cost is capped so that one page with its call fits each
  budget of one frame. When a deadline queue item opens a box, the
  queue items of its level that join a box follow it at page cost,
  ahead of their turn: the PRETRACE pages that are candidates, the FILL
  pages, and the DIRTY rectangles of a level that does not sweep. So a
  pre-trace row of pages is one call, not one for each page.
- A frame spends the `validityBurstPagesPerFrame` units of the burst on
  the REQUIRED pages of the coarsest level only, and at most
  `batchFrames * updatePagesPerFrame` units of the update budget when
  its update balance is positive, nothing when it is in debt (Batching
  below): first on the missing pages of the coarsest level that the
  burst leaves, then on the REQUIRED pages of the finer levels, then on
  the deadline queue and FILL. When there is less work, it builds less.
- The burst is only for the coarsest level, because a fallback ends
  there: with that level complete, every lookup inside it finds data. A
  finer level that is not complete only sends its receivers to a coarser
  level for some frames (a blurrier shadow, not a wrong one), so the
  peak cost of a frame after a teleport is `batchFrames` times the
  update budget, plus the burst when the teleport leaves the window of
  the coarsest level.

The order:
1. **REQUIRED**: pages that the cube of `E_k`
   around the camera touches and that their slot does not hold (VALID or
   DIRTY), or holds with a depth range that no longer covers the cube
   (8.6). Causes: first build, teleport, a scroll without pre-trace, a
   device reset, a large sun step (8.8). The coarsest level goes first,
   whole, from the validity burst: its inner pages, its edge pages and
   its fast sweep too. It has no fallback, so its pages go nearest the
   camera first, inner or not, and past the burst its missing pages and
   edge pages take the update budget too: when they take more than one
   frame, the first one covers the camera area. A lit gap of the
   coarsest level is a wrong shadow, which GI gathers and keeps after
   the page is built; a finer level that waits a frame reads a coarser
   one. Its fast sweep keeps to the burst, as a page that the sweep
   builds again still holds its old one, and the coarse pages are the
   costly ones on the GPU. Then, from the update budget that is left,
   fine levels first;
   in a level, the non-inner pages near the camera first. Then the fast
   sweeps of the other levels, and last their inner pages (see below).
   `RtssSchedulerLevelStats::requiredPending` counts the REQUIRED pages
   of a level that wait for budget.
2. **Deadline queue** (what is left of the update budget): DIRTY
   rectangles of the slots that hold their window page or a page a cube
   needs (deadline: now), SWEEP pages (a deadline for each page, 8.4) and
   PRETRACE pages (deadline: the predicted frame of the window move,
   8.6). An item is a candidate when it is due (its deadline is at or
   before the plan time). The earliest deadline goes first. A late page
   has a deadline in the past, so it goes first with no special rule.
   **Batching**: a frame that builds anyway (a REQUIRED page, a DIRTY
   rectangle, or a due PRETRACE or SWEEP page) also takes the PRETRACE
   pages that are not due yet, in deadline order, and the SWEEP pages
   due within the next `batchFrames - 1` frames, up to the update
   budget, which is a balance: each frame adds `updatePagesPerFrame`
   up to `batchFrames` times it, a frame with a positive balance may
   spend that cap, and the frames after it build nothing until the
   balance is positive again. The balance stays within plus or minus
   that cap, so a budget cut forgives the debt beyond the new batch,
   and a reset clears it. So a sun faster than the budget also
   builds in one frame of `batchFrames`, in a box of pages that one
   raster call draws. A dispatch lasts as long as its slowest rays, so
   a frame that traces costs at least about 30 us GPU whatever its
   pages (Why), and a raster call about 25 us CPU for one page and 50
   for a box of four; a walk builds one or two pages in a frame
   otherwise, and a slow sun one or two sweep pages. In testGI at
   `batchFrames` 1 the pre-trace part takes the frames with jobs of a
   6 m/s walk from 375 to 53 and its GPU time from 16 to 5.4 ms; at 4
   the frames with jobs drop 3 to 4 times more.
   A sweep keeps its pace: the frames between its batches build
   nothing, and a batch never goes further than `batchFrames - 1`
   frames of its deadlines, so a sweep longer than that never
   completes in the frame it starts
   (a whole sweep in one frame would build the pages of the +u edge
   out of order, and level 0 would take the whole budget at a sweep
   start). A page pre-traced early takes its slot from the outgoing
   page, so a camera that turns back to that page needs it again as
   REQUIRED (under 2 % of the pre-trace pages in a circle walk).
3. **FILL**: the other slots of a window that hold no page a cube needs,
   with the budget that is left.

**Idle.** A plan with no job, no queue item, no sweep and no REQUIRED
page that waits makes the scheduler idle. The next plan with the same
camera position and velocity and the same sun direction and angular
velocity returns that empty plan and does no work. An invalidation, a
new world box, a raster recipe change and a reset end the idle state.

**Key order in the queue.** The SWEEP pages of one level leave the queue
only in key order. When the sweep page with the largest key of a level
cannot be built in a frame (budget), no
other sweep page of that level is built in that frame. When its slot
already has a job in this frame (a REQUIRED or PRETRACE page that
replaces it), the head is stale: the sweep goes on with the next page. A
sweep job replaces the DIRTY job of its slot (5.3), so a DIRTY rectangle
never blocks a sweep page.

**Pages inside a finer level.** In xy, the `E_k` cube of level k covers
the middle of level k+1. Take the pages of level k+1 that are inside
that cube by more than one page of level k and the seam band (at the
defaults 1 to 4 pages of the 64, as the cube of level k is 3 pages of
level k+1 wide). They are read only for:
- Receivers more than `E_k` from the camera along the sun: with a high
  sun a roof, a treetop or a wall above the camera, a valley below it.
  The level cube is 3D (9.3), so level k does not serve them, but they
  are near the camera in xy. The level k+1 texel is the correct one for
  them: they are more than `E_k` from the camera.
- Fallbacks of level k (9.3).

So these pages are needed, but less often:
- In REQUIRED they come after all other pages of the finer levels (the
  coarsest level goes first, whole); in the deadline queue after all
  its other pages of all levels, but before FILL, whose pages no cube
  reads; in FILL after the other FILL pages.
- A sweep keeps them in key order. If a sweep delayed one, the order of
  8.4 would break, and a receiver that needs it would fall back to level
  k+2. There its xy is inside the cube of level k+1, so its page is an
  inner page too, and so on up to no data.
- They cannot give memory back. The toroidal slots move with the camera,
  and a slot is an inner page only for a time. A memory saving needs a
  page indirection (a pool of physical slots), for at most the inner
  area (up to 6% at the defaults).

During a sweep the scheduler tests the `E_k` cube in both live
transforms. In the target transform it also takes the image of the
current cube (its 8 corners, transformed): a receiver on the +u face and
below the camera sits up to `(z - z_cam) * sin(theta)` outside the
target cube, and its target page must exist too. This is the extension
of 8.8 condition 2, so inside the window limit it takes free slots. The
z range of that image widens the depth range of the target jobs (6.4).
- A page that the cube touches only in the target transform (past the +u
  edge of the window) is REQUIRED and takes the target bit, as a page
  above the head of the sweep (8.4): a sweep job first builds these
  pages above its own key, and a page built for a sweep job that the
  budget then blocks stays above that job. They do not move the front.
  Without them, a receiver near the +u face of the cube finds its
  new-transform page outside the window, while its old-transform page
  is already swept. Pages with a key at or below the key of the next
  sweep job are not built: with a diagonal
  u such a page comes inside the window after a window move, and then a
  receiver whose old page it is finds the wrong bit while its new page is
  not swept yet. Keys above the front are enough, because the shift never
  makes a key smaller. The pages past the +u.y edge are a row with keys
  above every key of the window, so the first sweep job of a level
  builds them all first, in as many frames as their calls need: the
  sweep job waits until each edge page above its key is built. A page
  past the +u.x edge comes when the front reaches its row, takes the
  target bit above the head (8.4) and shares a box with that row when
  the box takes both (7.3).
- A slot can be released (8.6) only when its page is outside the cube in
  both live transforms. Any page that a cube touches, in either
  transform, keeps its slot.
- During a sweep, no pre-trace, FILL or replacement job takes a slot that
  a past-edge page of the target cube can claim.
- When two pages that the cube touches need the same slot (a step above
  the window limit of 8.8: page o in the old transform and page o + W in
  the target transform), the slot keeps its page. The other page is not
  built, and its receivers fall back. So the slot never changes between
  the two pages from frame to frame. Its sweep job still builds page o
  with the target bit, so the sweep finishes (8.4).

### 8.3 Level state

```
IDLE:     all valid slots have the current bit; one transform is live.
SWEEPING: T[target bit] is live too; slots move from the current bit to
          the target bit in descending key order.
```

### 8.4 Sweeps

- **Duration.** `tolerance_k = min(toleranceDeg0 * 2^k, toleranceDegMax)`
  in degrees, and `tol_k` is the same angle in radians. With a moving sun,
  `sweepDuration_k = clamp(tol_k / |angular_velocity|, minSweepSeconds, maxSweepSeconds)`.
  With a zero angular velocity (a sun that changed but does not move,
  for example a slider in testGI), `sweepDuration_k = staticSweepSeconds`.
  The duration is always finite, so `H_k` (6.3) and `framesLeft` are
  finite too.
- **Start.** In IDLE, when the angle between `-d[current]` and
  `dir_to_sun` is at least `tolerance_k / 2`: set the pivot (6.3), build
  `T[!current] = next_transform(...)` toward `dir_to_sun`, compute u, and
  go to SWEEPING. The sun is slow, so the sweep has no prediction of the
  sun at its end. With a moving sun the sweeps come one after the other:
  a sweep that is not fast and ends early, or late by at most 10% of its
  duration, gives the next sweep `min(now, planned end)` as t0, so no
  frame is lost between two sweeps.
- A 180 deg step leaves u undefined. Then u = (1, 0) and the sweep is
  fast (8.8): no order can hold at 180 deg.
- **Error.** The target is the sun at t0. When the duration is not
  clamped, the sun is `|angular_velocity| * sweepDuration_k = tol_k` past
  the target at the end of the sweep, and the next sweep rebuilds a page
  at most `2 * tol_k` after its target.
- **Progress.** In each frame of a sweep, the level lists its slots of the
  current bit in descending key order. With s slots swept and n in the
  list, the i-th of them gets the deadline
  `t0 + (s + i + 1) * sweepDuration_k / (s + n)`. The deadline queue
  (8.2) builds each of them as a whole-slot job with the target bit. The
  deadlines of one level grow with the key order, so the queue keeps the
  key order of each level. A level that needs less than one page for each
  frame builds one page every few frames at `batchFrames` 1; with the
  batching of 8.2 a frame that builds takes its pages due within the
  next `batchFrames - 1` frames, so it builds a few pages at once and
  none in the frames between.
- **Row by row.** The key (section 1) orders the pages row by row, not
  by their distance along u. A distance order with a diagonal u would
  sweep along anti-diagonals, whose next pages spread over more rows and
  columns than a raster box (7.3) takes, so the pages of a frame would
  split into more calls. With the row order the next sweep pages of a
  level are the rest of a row, then the next row, so a call draws a run
  of a row and the pages under it in the next rows: as many as the
  scratch box takes (4 x 2 at the defaults, 8 x 4 with
  `rasterBatchPages` 64) or the budget pays. A run that no box takes
  opens another call. The row order keeps the coverage of 8.5, which
  needs only that no page is swept before a page that a receiver can
  move to.
- **Entering slots.** A slot that gets a new page during a sweep takes the
  target bit when its key is above the key of the next page of the
  ordered list (the head) after all sweep jobs of the frame, else the
  current bit; with no page left in the list, every new page takes the
  target bit. So in the window all target-bit slots have keys at least
  as large as all current-bit slots. The pages past the +u edge of 8.2
  are built above the head, so they take the target bit, also when the
  budget then blocks the head. The jobs that a frame plans before its
  sweep jobs take the bit of this rule after them, so a raster box
  (7.3) can hold a page that the raster plan then draws with the other
  transform; the raster plan groups the final bits.
- An entering slot with the current bit is in the list of the next frame
  at its key, and the deadlines of the list spread again to the same end
  `t0 + sweepDuration_k`. So the sweep builds it in its place, and the
  sweep can finish.
- A head slot that no cube needs and that does not hold its window page
  (left by a window move or an abandoned pre-trace) gets its window page
  as a FILL job in place of a sweep rebuild. An outgoing slot of a
  pending pre-trace at the head gets its incoming page in place of a
  sweep job, and the sweep goes on past it.
- A target-bit page with a key below the front stays: it is valid data
  for its bit, and no lookup reads it with another transform.
- **Finish.** When no valid slot has the current bit:
  `current = target`, IDLE.
- **Skip.** Sun changes during a sweep do not change the sweep. The next
  sweep goes straight to the newest sun. Sweeps never overlap, so
  two transforms are enough.
- **Budget.** With a 24-minute day (0.25 deg/s) and the defaults: level 0
  sweeps in 0.4 s (24 frames at 60 fps), level 1 in 0.8 s, level 2 in
  1.6 s, level 3 in 3.2 s, level 4 in 6.4 s (its 1.6 deg tolerance is
  below the clamp of 8.8 at W = 8). That is
  `64/24 + 64/48 + 64/96 + 64/192 + 64/384 = 2.7 + 1.3 + 0.7 + 0.3 + 0.2 = 5.2`
  pages for each frame on average (85k rays; 20.8 slots of 64 texels at
  16 x 64). At cost 1 on all levels the library default of 3 is below
  the 5.2, so a day this fast goes late with it (8.4, late sweeps),
  while a real-time sun (15 deg/h) needs about 0.17 pages a frame, as
  its sweeps clamp to `maxSweepSeconds`. A
  level at `pageCost` c needs c times its pages in units: `hybrid_sw` at
  the testGI costs (12.4: 1 on levels 0 and 1, 1.5 on the others) needs
  5.8 units for each frame at 60 fps, so its sweeps go late too.
- **Late sweeps.** With a smaller budget the deadlines pass and the
  sweeps take longer, so the next step of that level is larger than its
  tolerance. When a step passes the fast limit of 8.8, the level sweeps
  in the REQUIRED class, ahead of the deadline queue. So the coarsest
  level, whose fast sweep takes the validity burst, does not drift away:
  its sun error stays below about the fast limit. A finer level's fast
  sweep takes the update budget, finest level first, so with a budget too
  small for the sun speed the coarser of the finer levels can fall far
  behind. Its receivers read its own pages, which hold the older sun,
  so that level lags; only a receiver whose page it does not hold falls
  back to a coarser level. The stats show the lag (section 13).
- A late sweep keeps its pivot while the camera moves on. When the camera
  moves sunward by more than `H_k - E_k` during the sweep, some receivers
  are above the pivot and fall back (no wrong read). So the duration in
  the `H_k` formula (6.3) is the measured duration of the last sweep of
  that level when that was longer than planned.
- A running sweep becomes fast (8.8) when a condition of 8.8 fails for the
  current camera (a teleport during a long sweep can put the camera far
  above the pivot), or when its target is farther than the fast limit
  from the sun now. The test is against the sun now, not against the
  next target: else a sweep late by one frame would already be fast.

### 8.5 Why a lookup finds no gaps

The receivers of a level are below
its pivot (6.3), so a new-transform page is at least as far along u as the
old-transform page. The receiver moves along +u, so on each axis the page
index moves only toward the sign of that component of u. Each such move
makes the key (section 1) larger: a move along y changes the row, which
weighs more than any x term, as the pages of a level lie within 2W of
each other (8.6), far below the 65536 of the row term; a move along x
in the same row grows the x term. So the new-transform page of a
receiver is never swept later than its old-transform page, whatever the
direction of u.
In the window, the slots that the sweep built from its ordered list are
the slots with the largest keys (8.4), and the pages past the +u edge
with a key above the front have the target bit (8.2). So when
the old-transform page of a receiver has the target bit, the
new-transform page has it too. The bit check in the lookup (9.3) makes
sure that no lookup reads data with the wrong transform. The
pivot rule only makes sure that the second read finds data. This holds
while the step is inside the limits of 8.8. A normal sweep at the
defaults is inside them; a sweep that is so late that its step passes
them becomes a fast sweep (8.8).

### 8.6 Scroll

- The window of level k is a CPU notion: an origin o in pages. The slot
  table is the truth for the lookup.
- The camera page c is `floor(T[current](camera).xy / (P * texel_k))`.
  The scheduler keeps c in the central 2x2 pages of the window:
  `o + W/2 - 1 <= c < o + W/2 + 1` on each axis. When c leaves that
  range, the window moves by as many pages as it needs on that axis, in
  one frame (a teleport or a fast camera can need more than one).
- **Pre-trace.** From the camera velocity, the scheduler predicts the
  time of the next move. The W incoming pages get spread deadlines,
  nearest the camera first: page i is due at
  `tStart + (i + 1) * (tMove - frameDt - tStart) / W`. Each one is a
  candidate only when due (8.2). The release is part of the incoming
  page's job: the outgoing slot keeps its page until that job commits, so
  a camera that turns back loses nothing.
- The camera also moves in the page grid when a sweep ends with a static
  camera: the new transform shifts the camera's light space xy by about
  `H_k * theta` (0.02 pages/s at level 0 with the defaults). So the
  pre-trace also predicts the camera page in the target transform at the
  end of the sweep; else a static camera near a page edge gets a W-page
  REQUIRED burst at a sweep end.
- A pre-trace starts at the earliest when the camera passes the window
  center on that axis. Then the outgoing column is outside the `E_k`
  cube, the points in it select a coarser level, and no lookup needs the
  released slots. From the center to the move is one page: at 6 m/s
  level 0 (pages of 9.76 m) has 1.63 s (98 frames) for its 8 slots.
- When the camera turns back before the move, the cube can touch the
  released column again. Its pages are then REQUIRED (8.2) and take their
  slots back.
- During a sweep, when u points against the camera motion, the outgoing
  column is on the +u side, and the cube in the target transform can
  still touch it. The release rule of 8.2 then blocks the pre-trace for
  that slot. This is correct: the two cubes together span at most
  `(W - 1) * P` texels on each axis, so W slots are enough. Inside the
  window limit of 8.8 the block ends before the move in the unit tests
  (they count the blocked frames, and no REQUIRED page at a move); in
  testGI a blocked pre-trace can still end in a REQUIRED page (S3).
- **No pre-trace in time** (fast camera, teleport): the new slots are
  REQUIRED.
- A move of W pages or more rebuilds the level whole (REQUIRED).
- A move empties each slot whose page lies more than W pages from its
  window page: an edge page lies W pages out at most, and an older page
  could alias a window page in the 16-bit page key while its jobs are
  skipped.
- **Along the sun axis: no scroll.** A texel keeps light space z
  normalized over the depth range of its slot (5.2), which only a
  whole-slot job sets, and a job traces a range with a margin of at
  least `r` on each side of the cube (6.4). So a camera move toward or
  away from the sun changes nothing in the atlas: the lookup decodes
  with the range of the slot, and the level selection (9.3) uses the new
  camera depth. The classic static shadow needs `ShadowDepthScroller`
  because its DEPTH16 range moves with the camera. With ray tracing,
  what a camera move can make old is the traced range, not the
  encoding, and a rescale cannot add a range that no build ray traced.
  So a slot whose `[minDepth, maxDepth]` no longer contains the depth
  range of the `E_k` cube, clipped to the world box
  (`[max(z_cam - r, zMin), min(z_cam + r, zMax)]`: no receiver is outside
  the box), is REQUIRED again. (Without the clip, a slot near a face of
  the box would be REQUIRED in every frame.) This happens only where a
  clamp by `h` of 6.4 binds, after a camera move of `h - r` along the
  sun: 1530 m or more at the defaults.

### 8.7 Invalidation

- `invalidateBoxes(boxes, cnt, level_mask)`: for each level in the mask
  and each valid slot, project the box into the transform of the slot,
  make the texel rectangle, clip it to the slot `[-B, P + B)`, and add it
  to the DIRTY rectangle of that slot. A slot keeps one DIRTY rectangle,
  the bounds of all that it got, so two far boxes in one slot trace the
  texels between them too. A slot keeps its page and bit.
- A clip to `[-B, P + B)` of each slot also updates the borders of the
  neighbor slots, each in its own transform.
- A box smaller than 2 texels of a level on both light space axes does
  not invalidate that level (the classic static shadow has the same
  rule).
- `invalidate()`: all valid slots become DIRTY. `setWorldBox` with a
  different box calls it.

### 8.8 Large sun steps

A large step is a sweep like any other. It needs no special state. The
old pages stay valid with the old transform until the sweep rebuilds
them, and the bit check makes sure that no lookup reads data with the
wrong transform. Examples: a time skip, a fast slider drag, the switch
from the sun to the moon, the sunrise (8.9).

The only question is coverage during the sweep: a receiver must find its
old page not yet rebuilt, or its new page already rebuilt. With the shift
of 6.2 two conditions give that, for each level:
1. **Order.** The shift is not negative for any receiver of the level:
   `z >= s * tan(theta / 2)`. With `z >= H_k - E_k = 0.5 * E_k`, this
   holds up to 53 deg when u is along an axis (`s <= E_k`), and up to
   39 deg when u is diagonal (`s <= sqrt(2) * E_k`).
2. **Window.** The new-transform pages of the cube fit in the window. In
   the new transform the cube goes past its old +u face by at most
   `E_k * (cos(theta) - 1) + (H_k + E_k) * sin(theta) * sy`, with the y
   scale `sy` of 6.6. The cube is `(W - 2) * P` texels long, so it
   touches at most W - 1 of the W pages on each axis: a page past the +u
   edge takes the slot of a free one (`page mod W`), and 8.2 builds it
   with the target bit. An axis-aligned u is the worst case. With
   `H_k = 1.5 * E_k` and `sy = 1` this holds up to 26.17 deg for W = 4,
   7.88 deg for W = 8, 3.31 deg for W = 16 and 1.54 deg for W = 32.
   A larger `H_k` (a fast camera up to `maxCameraSpeed`, 6.3) or `sy`
   makes the limit smaller: at 50 m/s, `H_0 = 1.85 * E_0` and the limit
   of W = 16 is 2.9 deg; with `sy = 2` and `H_k = 1.5 * E_k` it is
   1.64 deg.

The **window limit** of a level is the angle of condition 2 with the
`H_k` of the sweep. The **fast limit** of a level is
`min(fastSweepDeg, 0.8 * window limit)`.

A step above these limits works too. During its sweep, a receiver near
the +u face of a level cube can find no page. It then falls back to the
next level, where it is well inside the cube. Only the coarsest level has
no fallback: there `rtss_sample_rel` returns false and `rtss_sun_visibility`
gives 1 (lit) near the +u face until the sweep of that level ends, and in
its seam band, which fades to lit (9.6), for good.

Rules:
- **Budget.** A step above the fast limit (a fast sweep) sweeps in the
  REQUIRED class, because until the sweep ends the error is the whole
  step, and above the window limit its +u faces fall back. With the
  defaults the fast limit is 5 deg (`fastSweepDeg`, below 0.8 of the
  7.88 deg window limit of W = 8), and 3.07 deg below a sun of 36.5 deg
  (`sy = 2`, below). The coarsest level goes first, from the validity
  burst (two frames for its 64 pages at the default burst of 32),
  because it has no fallback; the finer levels follow from the update
  budget (256 pages, about 85 frames at the default 3 units), and
  meanwhile their +u faces fall back to a coarser level. A level that
  sweeps when the large step comes first completes that sweep in the
  REQUIRED class (sweeps never overlap).
- **Tolerance.** The normal sweeps of a moving sun must stay inside the
  window limit, so the scheduler clamps each `tolerance_k` to half the
  fast limit. The margin keeps a late normal sweep from becoming a fast
  one: a sweep late by a few frames has a step a little above its
  tolerance. At W = 8 half of the 5 deg fast limit is 2.5 deg, so every
  level keeps its tolerance (up to 1.6 deg). Below 36.5 deg the default
  `maxAnisotropy` gives `sy = 2`, a window limit of 3.84 deg and a clamp
  to 1.54 deg for level 4. At W = 16 the clamp is 1.32 deg, and 0.66
  deg with `sy = 2`.

### 8.9 Sun below the horizon

- `dir_to_sun.y < minSunY`: the flag "sun below horizon" is set, and the
  lookup gives 0 (in shadow) everywhere.
- While the sun is below the horizon, no sweep starts. A sweep that runs
  at the sunset finishes.
- The first frame with `y >= minSunY` starts a sweep (8.4). After a night
  its step is above the fast limit, so it sweeps in the REQUIRED class
  (8.8): the coarsest level in the first two frames at the default
  burst, the finer levels at the update budget after it.

### 8.10 Device reset

All slots become EMPTY. The atlas is created again. The next frame
rebuilds the slots near the camera (REQUIRED).

## 9. Lookup

### 9.1 Frame order contract

`render()` changes the atlas, the slot table and the level table at
one point of the GPU timeline. A consumer before that point sees the
state of the previous frame, and a consumer after it sees the new
state. Both states are consistent. No consumer may run between the
tracer dispatches and the table write (they are in the same `render()`
call, so this is automatic). New resources (a layout change, a device
reset) and a failed slot table upload publish a level count of 0 until
a `render()` writes the tables, so a lookup then finds no data.

### 9.2 Shader interface

`rtss_lookup.dshl`:
- `INIT_RTSS(code)`: binds `rtss_atlas`, `rtss_slots`, `rtss_levels`,
  `rtss_globals` (5.4), and `rtss_gather_sampler` on Vulkan and Metal,
  else as `rtss_cmp_sampler` the compare sampler
  `static_shadow_tex_samplerstate` of the classic static shadow, whose
  state `static_shadow_stcode.dshl` declares; a node based dump
  declares it with no state, so it links to the primary dump by name.
- `USE_RTSS(code)`: the lookup functions, and `rtss_select_level` and
  `rtss_find_tap` of 9.3, which the debug pass also uses:

```hlsl
struct RtssTap { float3 ls; int2 pageRel; uint2 slot; uint bit, level; float zBase, invRange; }; // what 9.3 found, with the slot range (9.4)
// the visibility and the tap (9.3); level RTSS_NO_LEVEL below the horizon and with no data,
// and then tap is untouched below the horizon and holds no hit with no data
half rtss_lookup_rel(float3 pos_rel, float3 world_normal, float dither, out uint level, out RtssTap tap);
// the screen form with the filters of 9.7: 1 (lit) when no level has data (also past the coarsest level, 9.6), 0 below the horizon
half rtss_sun_visibility_screen_rel(float3 pos_rel, float3 world_normal, float dither, float radius_scale, float2 harden);
// false when no level has data (also past the coarsest level, 9.6) and the sun is above the horizon
bool rtss_sample_rel(float3 pos_rel, float3 world_normal, float dither, out half visibility, out uint level);
// 1 (lit) when no level has data (also past the coarsest level, 9.6), 0 when the sun is below the horizon
half rtss_sun_visibility_rel(float3 pos_rel, float3 world_normal, float dither);
// the same with a world position: pos_rel = world_pos - rtss_origin
half rtss_sun_visibility(float3 world_pos, float3 world_normal, float dither);
```

- `pos_rel` is the position relative to `rtss_origin`, the camera
  position that the project gave to `update()` (5.5). When the project
  gives the same position as its `world_view_pos`, a camera-relative
  position of a shader (`-pointToEye`) is `pos_rel` with no loss.

- `dither` in [0, 1) selects the seam level (9.6). Pass a per-pixel or a
  per-ray noise. A zero normal turns the normal offset off.

### 9.3 Algorithm

```hlsl
// RTSS_TAP_HIT, else the miss of the read that passed the most checks: RELEASED < BIT < DEPTH
uint rtss_find_tap(uint level, float3 pos, out RtssTap tap)  // pos: pos_rel with the normal offset of 9.5
{
  uint4 window = rtss_level_window(level);  // pageOrigin, flags, 1/(P*texel) of 5.4
  tap.level = level;
  uint bit = window.z & RTSS_LEVEL_NEWEST_BIT, miss = RTSS_TAP_RELEASED;
  for (uint i = 0; i < ((window.z & RTSS_LEVEL_SWEEPING) ? 2u : 1u); ++i, bit ^= 1)
  {
    tap.ls = rtss_to_ls(level, bit, float4(pos, 1));  // M * X + k; a w of 0 gives M * X, for a direction
    tap.pageRel = (int2)floor(tap.ls.xy * asfloat(window.w));
    tap.bit = bit;
    int2 page = int2(window.xy) + tap.pageRel;
    tap.slot = uint2(page) & (W - 1);
    RtssSlot s = rtss_slot(level, tap.slot);
    bool samePage = (s.flags & RTSS_SLOT_VALID) && s.page == rtss_pack_page(page);  // the low 16 bits of each axis (5.3)
    bool sameBit = rtss_slot_bit(s.flags) == bit;
    if (samePage && sameBit && tap.ls.z >= s.minDepth && tap.ls.z <= s.maxDepth)
      return RTSS_TAP_HIT;
    miss = max(miss, !samePage ? RTSS_TAP_RELEASED : !sameBit ? RTSS_TAP_BIT : RTSS_TAP_DEPTH);
  }
  return miss;
}

// lod: log2 of m; fade: the weight of lit in the seam band of the coarsest level (9.6)
uint rtss_select_level(float3 pos_rel, float dither, out float lod, out float fade)
{
  float3 fromCenter = pos_rel - float3(0, rtss_center_y, 0);  // the window center (8.1)
  float3 dc = rtss_to_ls(0, rtss_level0_bit, float4(fromCenter, 0));
  dc.y *= rtss_select_y;  // 6.6
  float m = max(max3(abs(dc)) * rtss_inv_e0, 1e-6);
  lod = log2(m);
  int first = max(0, (int)ceil(lod)), last = max(rtss_level_count - 1, 0);
  float band = saturate((m * exp2(-first) - 1) * rtss_inv_seam_band + 1);  // 9.6
  float ml = max3(abs(rtss_to_ls(last, rtss_level_window(last).z & RTSS_LEVEL_NEWEST_BIT, float4(fromCenter, 0)))) * rtss_inv_e0 * exp2(-last);
  fade = first >= last ? saturate((ml - 1) * rtss_inv_seam_band + 1) : 0;  // the coarsest cube in its own sy
  return first < last && dither < band ? first + 1 : min(first, last);  // the coarsest cube can hold a point past E_last of level 0
}

uint rtss_find_level(float3 pos_rel, float3 world_normal, float dither, out float lod, out RtssTap tap, out float fade)
{
  tap = (RtssTap)0;  // the outputs start at zero: fxc rejects a read it hoists above the hit test, and the loop can run zero times
  lod = fade = 0;
  uint k = rtss_select_level(pos_rel, dither, lod, fade);
  float3 pos = rtss_normal_offset_pos(pos_rel, world_normal, lod);  // 9.5
  for (; k < rtss_level_count; ++k)
    if (rtss_find_tap(k, pos, tap) == RTSS_TAP_HIT)
      return k;
  return RTSS_NO_LEVEL;
}

// the visibility that every consumer gets; level RTSS_NO_LEVEL (0xffffffff) below the horizon and with no data
half rtss_lookup_rel(float3 pos_rel, float3 world_normal, float dither, out uint level, out RtssTap tap)
{
  level = RTSS_NO_LEVEL;
  if (rtss_sun_below_horizon) return 0;
  float lod, fade;
  level = rtss_find_level(pos_rel, world_normal, dither, lod, tap, fade);
  if (level == RTSS_NO_LEVEL) return 1;  // an early return: HLSL 2018 runs both sides of ?:
  return lerp(rtss_pcf2x2(tap, 0), 1, fade);  // 9.4, 9.6
}

bool rtss_sample_rel(float3 pos_rel, float3 world_normal, float dither, out half visibility, out uint level)
{
  RtssTap tap;
  visibility = rtss_lookup_rel(pos_rel, world_normal, dither, level, tap);
  return level != RTSS_NO_LEVEL || rtss_sun_below_horizon;
}
```

- `rtss_inv_e0`, `rtss_sun_below_horizon`, `rtss_level0_bit` and
  `rtss_inv_seam_band` are `rtss_params`, `rtss_level_count` is
  `rtss_params_i.x` and `W` is `rtss_params_i.y` (5.4).

- The level selection uses level 0 for all levels. The transforms of the
  levels are different by at most their tolerances, and the `0.5 * E_k`
  margin of the pivot covers that.
- A lookup reads one slot entry for each level that it visits, two
  during a sweep.

### 9.4 PCF 2x2

- Hardware compare: `SampleCmpLevelZero` with `rtss_cmp_sampler`
  (bilinear; every driver compares LESS_EQUAL) on the L16 atlas, with
  the reference `ls.z - biasTexels * texel_k` normalized over the range
  of the slot as the atlas stores it (5.2): the tap keeps `zBase`, the
  `minDepth` of the slot it found, and `invRange`, its
  `1 / (maxDepth - minDepth)`. One sample in place of a
  gather and four compares. A compare is 1 (lit) where the reference is
  less than or equal to the stored depth, so a texel of `RTSS_MISS`
  reads lit. The manual form compares the same way.
- Metal and Vulkan use the manual form (`RTSS_HW_COMPARE 0`, set by
  `USE_RTSS`): one `GatherRed` of the 2x2 texels, four compares and the
  bilinear weights. The gather reads at the shared corner of the block
  whose base the weights take, so the texels match the weights whatever
  the subtexel rounding. It reads through `rtss_gather_sampler`, a
  sampler with no state that the library sets, as a gather reads mip 0
  and no filter. Metal compares only depth formats, and the atlas must
  be a UAV for the build, so it cannot be a depth format. Vulkan
  compares L16 only with the optional format feature
  `VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_DEPTH_COMPARISON_BIT`, which the
  NVIDIA Linux driver 595.91.07 (RTX 5070 Ti) reports for no color
  format; that driver flaw is the only reason for the manual form on
  Vulkan. DX11 and DX12 always compare L16 in hardware. Each platform
  has one lookup variant.
- The sample position in slot texels is
  `ls.xy / texel - (page - pageOrigin) * P + B` (all values small, 5.5).
  The 2x2 footprint of any point of the page interior is inside the S x S
  slot, so PCF never reads a neighbor slot.

### 9.5 Bias

- Depth bias: `biasTexels * texel_k`.
- Normal offset: the receiver moves
  `normalBiasTexels * texel_0 * max(m, 1) * (1 - NoL)` along its normal
  before the transform, with m of 9.3 and `NoL = saturate(dot(N, -d))`
  in the transform of level 0. This is the form of
  `STATIC_SHADOW_NORMAL_OFFSET` (`static_shadow.hlsl`): small on surfaces
  that face the sun, so no peter-panning there, and full at grazing
  angles, where acne comes from. The default `normalBiasTexels` is 2, as
  `STATIC_SHADOW_NORMAL_OFFSET_TEXELS`.
- `texel_0 * max(m, 1)` is the texel size at the receiver distance: it
  is `texel_k` at the outer edge of level k, and it grows with no step.
  An offset from the texel of the selected level would double at each
  seam. On flat ground an offset h makes a shadow shorter by
  `h / tan(elevation)` at its far end, because the ray of the receiver
  passes above the top of the caster, so each seam was a step in the
  shadow length. With this form a seam does not change the length.
- The depth bias scales with the level texel size.

### 9.6 Seams

In the outer `seamBand` (default 0.1) of a selected level cube, the
lookup uses level k+1 with a probability that grows to 1 at the edge.
One fetch for each lookup; TAA or a denoiser removes the noise. Past the coarsest level there is no data, so in its band
the lookup does not dither: the visibility of the coarsest level fades
to lit with the same weight, with no noise and no hard end at the
range. That band is measured in the newest transform of the coarsest
level, as its sy can differ from the sy that selects the level (6.6).
A `seamBand` of 0 is a hard switch.
The scheduler uses the same band for the inner pages of 8.2.

### 9.7 Screen filters

GI and the other consumers keep the 2x2 PCF of 9.4: it is fast, and
their denoisers do the rest. On screen the 2x2 PCF shows texel steps,
so `rtss_sun_visibility_screen_rel(pos_rel, normal, dither, radius_scale, harden)`
gives a wider filter, selected with defines:

| Define | Classic form | Samples | Use |
|---|---|---|---|
| none (DITHERED, the default) | `static_shadow_sample_8_tap_normal_adj` | 8 | with TAA or a denoiser; DNG and WT use the dithered form |
| `RTSS_SCREEN_HARDEN`, with DITHERED | `STATIC_SHADOW_DITHER_HARDEN` | + 1 texel read | contact hardening, WT |
| `RTSS_SCREEN_TENT` | `static_shadow_sample_fxaa` (no DITHERED), which the tent replaces | 9 | no dither in the kernel |

- **The kernel stays in the slot of the center tap.** The center tap
  runs the checks of 9.3. The other taps use its slot, bit and
  transform, with no slot read. A tap within `B - 0.5` texels of the
  center keeps its 2x2 footprint inside the slot, so B sets the largest
  radius: 2.5 texels with the default B = 3. A wider border costs rays:
  B = 3 at S = 128 gives P = 122 in place of 126, 7% more rays for the
  same area (5.1.1).
- **DITHERED.** 8 Poisson taps (`noise/poisson_disc_8.hlsl`), rotated
  by `dither`, with a radius of 1.5 texels times `radius_scale`, in the
  tangent plane of the receiver (the NORMAL_ADJ form): the taps follow
  the surface depth, so half of them are not inside the surface, and
  there is less acne than with taps in the light plane. The disc is
  clamped to the border in light space.
- **HARDEN.** The center tap reads the caster depth. Here that depth is
  metric light space z, so the caster-to-receiver distance is in meters
  (the classic form needs `max_ht_range` for it). The radius goes from
  0.5 texel at contact to the full radius at `1 / harden.x` meters (WT:
  `harden.x = 0.5`, so 2 m), with the relax term `harden.y` of WT
  (`skyquake/prog/shaders/source/combined_shadows.dshl:417`). So a
  shadow is sharp near its caster: thin casters stay readable and a
  shadow does not look detached from its caster.
- **TENT.** A 3x3 tent (weights 1, 2, 1) of 2x2 PCF samples at a step
  of `min(radius_scale, 1)` texel, on the receiver plane: the z of each
  tap follows the slope of the normal in light space (clamped to 2), so
  a wide kernel adds no acne. A step above one texel would show each
  edge three times.
- **Seams.** The screen form selects the level with `frac(16 * dither)`,
  so the level noise is apart from the kernel rotation, and TAA resolves
  both. In level k the kernel is `max(m, 1) / 2^k` of its size (`m` of
  9.3, at least 1/2), from half at the inner edge of the level to whole
  at the outer edge, so at a seam both levels filter over the same world
  size.
- **Cost.** DITHERED is 8 PCF samples (one more texel read with
  HARDEN) and TENT 9, against 1 for the GI lookup. Each reads one slot
  entry for each level that it visits (9.3).

## 10. C++ API

`prog/gameLibs/render/raytracedToroidalStaticShadow/raytracedToroidalStaticShadow_api.h`,
at the end of the work items.

```cpp
class IStaticShadowsCB;

// the groups of one trace batch, sorted by level (7.2)
struct RtssTraceBatch
{
  uint32_t levelGroups[RTSS_MAX_LEVELS + 1]; // the groups of level k are [levelGroups[k], levelGroups[k + 1])
  BaseTexture *atlas;
  // dispatches cs over the groups of the levels of level_mask; false for an invalid cs or a failed dispatch
  bool dispatch(const ComputeShader &cs, uint32_t level_mask) const;
};

class IRtssTraceCB
{
public:
  virtual bool traceJobs(const RtssTraceBatch &batch) = 0; // true only when the groups of all levels were dispatched
};

// one compute shader for the levels of levelMask; with a partial mask it is a part of a tracer whose parts cover all levels,
// and its traceJobs answers for its levels only. The library has rtss_trace_swrt_cs and rtss_trace_hwrt_cs (7.2)
class RtssComputeTracer : public IRtssTraceCB
{
public:
  explicit RtssComputeTracer(const char *cs_name, uint32_t level_mask = ~0u);
  bool traceJobs(const RtssTraceBatch &batch) override;
  uint32_t levelMask;
};

// core/rtssSettings.h: the settings of the scheduler, in the table of 5.1
// RtssSchedulerLayout: levels, windowPages, slotTexels, borderTexels, texel0, maxCasterReach, minSunY, maxAnisotropy
// RtssSchedulerBudget: the tolerances, the sweep timing, maxCameraSpeed, the page budgets in cost units, seamBand,
//   and the raster box: rasterBatchPages, rasterBoxPagesPerJob, rasterCallCost, rasterTranspose
// RtssSchedulerRecipes: recipes[k], the raster and the pageCost of level k (7.5)
// RtssSchedulerSettings derives from the three
struct RaytracedToroidalStaticShadowSettings : RtssSchedulerSettings
{
  float biasTexels = 1.f;                     // depth bias, texels of the level
  float normalBiasTexels = 2.f;               // normal offset, texels at the receiver distance, times 1 - NoL
  float sunJumpDeg = 1.f;                     // a larger sun step is a jump, as a time skip, with no sun rate (8.1)

  // the library defaults
  constexpr RaytracedToroidalStaticShadowSettings() :
    RtssSchedulerSettings{
      {.levels = 5, .windowPages = 8, .slotTexels = 128, .borderTexels = 3, .texel0 = 0.08f, .maxAnisotropy = 2.f},
      {.updatePagesPerFrame = 3, .validityBurstPagesPerFrame = 32, .batchFrames = 4}}
  {}
};

class RaytracedToroidalStaticShadow
{
public:
  explicit RaytracedToroidalStaticShadow(const RaytracedToroidalStaticShadowSettings &settings);
  ~RaytracedToroidalStaticShadow(); // a lookup after it finds no level and reads lit

  // budgets, costs and raster sizes apply from the next update(), biases from the next render(); a layout change rebuilds,
  // a raster change of a level invalidates that level; a project must invalidate a level whose tracer it changes (7.5)
  void setSettings(const RaytracedToroidalStaticShadowSettings &settings);
  const RaytracedToroidalStaticShadowSettings &getSettings() const; // as given; the scheduler clamps its copy (5.1)

  void setWorldBox(const BBox3 &casters_box); // a changed box invalidates all slots
  // a changed sun moves at its step over the update time since the last change (8.1)
  void setSun(const Point3 &dir_to_sun);

  void invalidate();
  void invalidateBoxes(const BBox3 *boxes, uint32_t cnt, uint32_t level_mask = ~0u);

  // plans the jobs of this frame
  // camera_pos is also rtss_origin of the lookup (5.5); give the same position as world_view_pos;
  // camera_velocity in m/s, above maxCameraSpeed a teleport (8.1); dt in s
  void update(const DPoint3 &camera_pos, const Point3 &camera_velocity, float dt);
  float getDistance(uint32_t level = ~0u) const; // the reach of the level from the camera, E_k of 6.5; the coarsest by default and past the last
  // from update() to render(): the boxes that render() gives raster_cb, region r as its region argument, to cull them ahead
  int getRasterRegionCount() const;
  const ViewTransformData &getRasterRegion(int region) const;
  // debug: the slot table (rtss_common.hlsli) of the GPU after the next render(), entry (level * W + y) * W + x; a new layout frees it
  dag::ConstSpan<RtssSlot> getSlotTable() const;
  // builds the jobs that update() planned, writes the tables, sets the shader vars; a raster level needs raster_cb, which
  // draws a box of its pages in each call (7.3), and a traced level trace_cb; the pages of an Augment box that raster_cb reports
  // NothingRendered are traced alone; raster_cb draws with the depth clamp override state of the library (7.3)
  void render(IStaticShadowsCB *raster_cb, IRtssTraceCB *trace_cb);

  // the level, clamped to the coarsest, has data over box if it covers it; DIRTY data counts, a page in a depth rebuild does not;
  // false while it sweeps fast or is not live. A lookup with the coarsest level ready reads no lit gap over box
  bool isReady(const BBox3 &box, uint32_t level = ~0u) const;
  void afterDeviceReset();

  // pixels of a debug pass that a later pass reads back, up to 3 frames later, all with the sun above the horizon only (13);
  // reference: the lit pixels that mode 5 compared, and those that differ
  struct GpuCounters
  {
    uint32_t lookups, noData, fallbackReleased, fallbackBit, fallbackDepth, referencePixels, referenceDiff;
  };
  // the scheduler stats: the pages and the cost of this frame by class, rays, the levels (sweep, sun error, tolerance, slots)
  struct Stats : RtssSchedulerStats
  {
    int rasterJobs, rasterSkipped, rasterProvisional, rasterEmpty; // the jobs of the last update(), then of the last render()
    float sunRateDegPerSec;                                        // of the last setSun()
    GpuCounters gpu;
  };
  void getStats(Stats &out) const;
  // from the gbuffer depth; 1: visibility, 2: level, 3: slot bit, 4: fallback, 5: the difference to the project reference
  void debugRenderScreen(int mode);
  void debugDrawAtlas(int level); // slot states and depth, for a debug overlay
};
```

- `setSettings` with a new `raster` of a level drops the plan of
  `update()`, which was made for the old recipes, and makes the slots of
  that level DIRTY, so its old pages stay readable until rebuilt. A new
  layout makes a new scheduler with empty slots.
- A raster level with jobs and no `raster_cb`, or a traced level with
  jobs and no `trace_cb`: `render()` logs an error once and builds
  nothing; the next `update()` plans the same jobs again.
- The library does not return the world box: the caller keeps its own
  copy of the box that it gave `setWorldBox`.
- The library does not call bvh, daSWRT or heightmapTrace. The project
  binds what its tracers read, the SWRT buffers or a TLAS (for example
  `bvh::bind_resources`), and updates its `HeightmapMaxGrid` before
  `render()`.

## 11. Library layout and build

`prog/gameLibs/render/raytracedToroidalStaticShadow/`, with the public
header in the library directory (`prog/gameLibs/CLAUDE.md`) and a plain
jamfile like `render/rtdl`. `gameLibs/build_module.jam` is not used: it
adds an ECS pull file and Quirrel, which this library does not need.

```
jamfile                                   // gameLibs/render/raytracedToroidalStaticShadow.lib
_lib.jam                                  // for installable consumers
_shaders.blk                              // rtss_build, rtss_debug
raytracedToroidalStaticShadow_api.h       // public header
raytracedToroidalStaticShadow.cpp         // class, GPU resources, render(), the raster producer (7.3), the trace batches (7.2)
core/rtssTransforms.h                     // 6.1, 6.2, 6.3
core/rtssSettings.h                       // the settings structs, shared with the public header
core/rtssScheduler.h, rtssScheduler.cpp   // windows, sweeps, budget, jobs; the stats structs, shared with the public header
core/rtssRasterPlan.h                     // order, cull groups, matrices and skips of the raster producer (7.3)
core/rtssCpuLookup.h                      // CPU copy of the 9.3 lookup, for the tests
tests/jamfile, tests/*.cpp                // Catch2 test of core
tests/rtssTestSim.h                       // the simulation harness of the tests
shaders/rtss_common.hlsli                 // structs shared by C++ and HLSL
shaders/rtss_lookup.dshl, rtss_lookup.hlsl
shaders/rtss_static_shadow.dshl           // static_shadow.dshl API over the lookup; static_shadow_stcode.dshl includes it
shaders/rtss_trace_common.dshl            // the build vars, RTSS_TRACE_TEXEL of the tracer shaders (7.2)
shaders/rtss_build.dshl                   // rtss_resolve_cs
shaders/rtss_debug.dshl                   // the debug views; include_optional project_rtss_reference.dshl (13)
shaders/rtss_trace_hmap.dshl              // RTSS_TRACE_HMAP: the terrain of the heightmap max grid (7.2)
shaders/rtss_trace_swrt.dshl              // rtss_trace_swrt_cs, the SWRT tracer (7.2)
shaders/rtss_trace_hwrt.dshl              // rtss_trace_hwrt_cs, the HW tracer of the bvh main TLAS (7.2)
```

- The scheduler and the transforms (`core/`) have no GPU code, so the
  unit test compiles them alone and drives them with a synthetic sun and
  camera, with no driver. It checks the invariants: two live transforms
  at most, the keys of the target-bit slots of a window >= its
  current-bit keys, a whole-slot job for each page change, one
  job for each slot and frame, a finite sweep duration for any angular
  velocity, no budget overrun, the shift formula of 6.2 against a direct
  transform.
- No code or string of the library may contain the word "daNetGame":
  codecheck `check_code_dependencies` fails a gameLibs file (but a .md or
  .txt file) that names it outside a comment line (`//` in code and
  shaders, `#` in jamfiles and Python). Write "DNG" everywhere.
- testGI includes its `_lib.jam` in its jamfile and adds the shaders to
  its lists: `_shaders.blk` and the tracer shaders it uses. testGI has
  no `setup_libs.py`.

## 12. testGI integration

### 12.1 Sun animation

New `sun_panel` fields and GUI lines:
- `animate` (bool, off): the sun moves.
- `day_minutes` (float 1..240, default 24): the length of one day.
- `axis_tilt` (float 0..80 deg, default 40): the angle between the world
  up and the rotation axis of the sun (like a latitude).
- `time_skip` (button): moves the sun by 7.5 deg (30 minutes of a 24-hour
  day), a large step (8.8) above the fast limit of W = 8 and the window
  limit of W = 16.

In the non-astronomical mode: the axis A is the world up turned by
`axis_tilt` about world X. The zenith and azimuth sliders hold the
animated sun: each frame of the animation turns the direction of the
sliders about A by `2*pi*dt / (day_minutes*60)`, a time skip by its
angle, and writes it back, so a drag moves the sun. In the astronomical
mode, `animate` moves the time slider. In both modes the library makes
the rate from the step of the sun direction of each frame (8.1). It
moves the sun along its path to first order; over a sweep the path
curvature adds a second order error, far below the tolerance.

The value that goes to the library is the final `dir_to_sun` after
`daSkies.getPrimarySunDir()`. A switch to the moon is a large step
(8.8). testGI sets `sunJumpDeg` to 1.5 times the day travel of the
frame, so a slider drag or the moon switch is a jump with no rate. The
travel is 0 with `animate` off and in a frame with a time skip, and a
travel that can pass 60 deg gives a `sunJumpDeg` of 0 too, as atan2
sees a wrapped travel as a short step.

### 12.2 Instance and controls

- Member `eastl::unique_ptr<RaytracedToroidalStaticShadow> rtss`, made in
  the constructor before the SWRT build, which then invalidates it
  through `remakeSWRTInstances`. `onSceneLoaded()` calls
  `setWorldBox(levelBox)`, with the y range extended to the heightmap
  range (`htMinMax`), so that the start plane is sunward of the terrain.
- New GUI panel `rtss_panel`: levels, windowPages, slotTexels,
  borderTexels, texel0, maxAnisotropy, toleranceDeg0, updatePagesPerFrame,
  batchFrames, recipe (the preset of 12.4), hwLevels, rasterMaxTexel, rasterCost,
  augmentCost, rasterBatchPages, screen filter, screen radius
  (the `radius_scale` of 9.7), seamBand, debug view, debug level (the
  atlas level of view 6), show_stats (the stats line).
- Console vars (`CONSOLE_*_VAL("rtss", ...)`): `rtss.freeze` (no
  update, to look at one state), `rtss.terrain` (off: no heightmap in
  the raster and no max grid, so no terrain in the rtss and in the GI
  trace), `rtss.reference` (the SWRT shadows as the reference of view
  5, 12.6).
- Console commands: `rtss.debug <mode>`,
  `rtss.recipe <0..4>` (the preset of 12.4), `rtss.hw_levels <n>`,
  `rtss.raster_max_texel <m>`,
  `rtss.raster_cost <all> [augment]`, `rtss.update_pages_per_frame <n>`,
  `rtss.batch_frames <n>`,
  `rtss.stats` (writes the stats to the log, with the
  preset, the recipe of each level and the LRU revision),
  `rtss.invalidate`, `rtss.show_stats [0|1]` (the
  stats line), one command for each filter setting:
  `rtss.screen_filter`, `rtss.screen_radius`, `rtss.max_anisotropy`,
  `rtss.border_texels`, `rtss.normal_bias_texels`, `rtss.seam_band`.
- Scenario tools: `rtss.settle` (the frames since the last frame with a
  job), `rtss.invalidate_around <radius> [level_mask]` (a static change
  test), `rtss.layout <W> <S>` (the window and the slot of the tuning),
  `camera.move [vx] [vy] [vz]` (a constant camera velocity, none stops
  it), `sun.dir <zenith> <azimuth>` and `sun.animate <0|1>
  [day_minutes] [axis_tilt]`.

### 12.3 Place in the frame

- `rtss->update(...)` and `rtss->render(...)` go in `drawScene` after
  `giHmapMaxGrid.update` and before `daGI2->beforeRender`. Then GI,
  voxelization and the screen passes of this frame all see the new
  state.
- `bvh::bind_resources` runs when `hwRtGi` or a level traces HW, so the
  HW tracer has its TLAS without HW GI. `buildBvh()` (in
  `beforeDrawScene`) runs before it. The SWRT buffers are global and set
  at load.

### 12.4 Tracers and recipes in testGI

- **Presets** (`rtss.recipe`, `rtssSettings()`): 0 `swrt` (`None` on all
  levels), 1 `hwrt` (`None`, HW on the HW levels), 2 `raster` (`All`),
  3 `hybrid_sw` (`All` on the levels with a texel up to
  `rtss.raster_max_texel`, 0.25 m: levels 0 and 1; `Augment` with SWRT
  on the others), 4 `hybrid_hw` (`All` on levels 0 and 1, `Augment` on
  the others, with HW on the HW levels). `pageCost` is `rasterCost`
  (1) on `All`, `augmentCost` (1.5) on `Augment` and 1 on `None`
  levels. Without SWRT data the traced levels become `All`; a HW level
  whose scene is absent (DX11, or a scene not resident) leaves the HW
  mask, so the SWRT tracer traces it.
- **SWRT**: the existing `swrt` scene (ri collisions of the whole map).
  `remakeSWRTInstances` (console `render.swrt_rebuild_around`)
  makes the TLAS again with a subset of the instances, so it must call
  `rtss->invalidate()`.
- **Tracers**: the tracer callback of testGI has a `RtssComputeTracer`
  of `rtss_trace_swrt_cs` for the SW levels and one of
  `testgi_rtss_trace_collision_cs` (`shaders/source/testgi_rtss_trace.dshl`)
  for the HW levels: the RayQuery on
  `bvh_lru_collision` of `testgi_trace_collision` (`project_trace.dshl`),
  closest hit, `RAY_FLAG_FORCE_OPAQUE`, no cull, then `RTSS_TRACE_HMAP`.
  The DX11 dump has no HW shader, and there the HW mask is 0.
  The HW levels are the traced
  levels under `rtss.hw_levels` (default 3: levels 0-2, so level 2 in
  `hybrid_hw`), and only while the HW scene is there (DX12 and resident
  collision instances), as the HW GI does; else the mask is 0.
  The LRU window radius is `effectiveBvhRadius()` (the world SDF far
  distance, or `render.bvh_radius`), so HW levels miss casters farther
  than that: the level 2 window is 312 m (+- 156 m), and a HW level 4
  lost the casters of 17 % of its texels. A change of the mask changes
  the tracer of a level, so testGI calls `rtss->invalidate()`. Each frame testGI reads
  `bvh::get_lru_collision_stats(bvhCtx)`. When `settled` is true and
  `revision` differs from the revision of its last invalidation, it
  calls `invalidateBoxes(&box, 1, mask)`, where `box` is the LRU gather
  window: the camera position +- (`bvhLruRadius` + its gather border).
- **Raster**: a new `RtssRasterCallback : IStaticShadowsCB` calls
  `renderOpaque` with the view of its region, whose land mesh and
  collision culls `precullRtssRegions` takes ahead with the same
  `culling_gtm`, the heightmap too (7.3), in the override state of the
  producer. On an
  `Augment` level it draws only the casters that the trace does not
  hold: the trees whose bounding sphere the cull matrix touches (a new
  `renderTreesCulled` of `test_app.cpp`), and the landmesh when the part
  of the page prism in the height range of the land reaches past a
  clamped heightmap or past the window of the max grid, the terrain the
  trace holds (a new `renderLandmeshPastHmap`), and returns
  `NothingRendered` when there is none.
- **Terrain**: the raster draws the heightmap, and the tracers march
  the max grid (`RTSS_TRACE_HMAP`, 7.2), which has no terrain past a
  clamped heightmap: a `None` level has no landmesh shadow there, and an
  `Augment` level draws the landmesh. The grid
  of testGI, 256 cells of 8 m, +- 1 km (`render.hmap_max_grid_cells`,
  `render.hmap_max_grid_cell_size`), covers the windows of all levels
  of the defaults (+- 625 m at level 4) and 400 m more sunward, within
  3 % of the GI cost of 128 x 8. Cells of 16 m
  (+- 2 km) reach farther, but the march takes half the samples for each
  meter and changes the terrain shadow outlines. A change of the grid
  invalidates the rtss: the pages hold the terrain of the old grid.

### 12.5 Consumers

The rtss is the only static shadow of testGI. These shaders use
`rtss_sun_visibility` where the classic one used `getStaticShadow`:
- `dagi_get_sun_color` in `SUN_LIGHT_POINT_TRACE_HIT` and the functions
  of `PROJECT_LIGHT_POINT_TRACE_HIT` and `PROJECT_LIGHT_POINT_VOXELIZE`
  (`project_light_point.dshl`). CSM stays where it is today. The normal
  goes to the lookup. `dither` is a hash of the position, with no frame
  term, so it is stable in time.
- The static term of `combine_shadows` (`deferred_shadow.dshl`), in
  the screen form of 9.7 with the gbuffer normal: DITHERED with HARDEN
  (the panel default) and the panel screen radius (1.5), or TENT from
  the panel, which caps that radius at 1. The panel sets the interval
  `rtss_screen_filter_mode`, whose variants set the defines of 9.7.
  `dither` is the existing `interleavedGradientNoiseFramed` value. The
  heightmap shadow and the CSM stay; the CSM `maxDist` is a fixed 50 m,
  as it read the distance of the classic static shadow.
- `get_project_skies_shadows` (`project_skies_shadows.dshl`), with the
  same position hash as dither.
- `deferred_simple` (`deferred_shadow.dshl`), the simple deferred
  path, with the same position hash as dither.

`updateSSGIPos` waits for `rtss->isReady(box)`, the coarsest level,
which every lookup falls back to, and clamps the position to the rtss
world box; the probe bake waits for level 0. With no SWRT data the
traced levels become `All` (12.4). The SWRT screen shadow has no step
offset (the default of daSWRT): `project_swrt_shadows_step.dshl`
loses its override, which read the transform of the classic cascade.

### 12.6 Reference

The debug view "difference" needs a reference sun visibility for each
pixel. testGI has it: with `rtss.reference 1`, `swRTShadows()` passes
`sun_size = 0` and no checkerboard frame (the reference overrides
`render.swrt_shadow_use_checkerboard`), so it traces one hard sun ray
for each half-res pixel. The reference ray starts at the pixel (the
daSWRT default step), and `combined_shadows` is a white texture for the
pass, so the early-out of the SWRT shader cannot copy the rtss term
that the view checks. The reference has the SWRT casters only, no
terrain, so the diff test runs with `rtss.terrain 0` and the `swrt`
preset.
The reference is `PROJECT_RTSS_REFERENCE` of
`shaders/source/project_rtss_reference.dshl`.

## 13. Debug views and stats

`rtss.debug` modes, drawn by one screen pass from the gbuffer world
positions (`rtss_debug.dshl`):
1. **visibility**: `rtss_sun_visibility` only.
2. **level**: a color for each used level; black for no data. Every
   second page is dimmed (a checker), so the page borders show.
3. **slot**: a color for the transform bit and one for a second read
   (during a sweep); the sweep front shows as the line between them.
4. **fallback**: where the lookup used a coarser level than the selected
   one, a color for the miss of the selected level: red for a released
   slot (empty, or another page), orange for a bit mismatch, magenta for
   the depth range. Blue for no data (also past the coarsest level),
   and the visibility elsewhere.
5. **difference**: `rtss - reference` (12.6), red where the rtss is
   darker, blue where it is lighter, with the percent of pixels that are
   different by more than 0.5. The reference is the project hook
   `PROJECT_RTSS_REFERENCE` of the optional project file
   `project_rtss_reference.dshl` (the default in `rtss_debug.dshl` has
   none, and then the view is the dimmed visibility); a backlit pixel or
   one with no rtss data is not compared.
6. **atlas**: `debugDrawAtlas(level)` as an overlay: EMPTY slots in
   magenta, VALID slots tinted by the transform bit (the GPU sees DIRTY
   as VALID), the depth in grey and dark slot borders.

GPU counters (debug pass only, so the other consumers pay no atomic
cost; the buffers are made at the first debug pass; all count with the
sun above the horizon only, as below it view 5 has no rtss data to
compare): lookups, no data (not the lookups past the coarsest level,
9.6), and fallbacks by the miss of the selected level
(`RTSS_TAP_RELEASED`, `RTSS_TAP_BIT`, `RTSS_TAP_DEPTH` of
`rtss_find_tap`, so the reasons come from the checks of the lookup
itself), and for view 5 the compared and the different pixels. A ring
of 3 readback buffers gives them to the CPU without a stall, at a later
debug pass, in `Stats::gpu` and `rtss.stats`.

CPU stats (`getStats` and `rtss.stats`; the stats line shows a part):
pages and cost units for each class and frame, the edge pages
(`pagesEdge`), rays for each frame, the sun rate and the below horizon
flag, the raster jobs of the last update() with
their skips, provisional builds and empty reports in the last render(),
and for each level: sweeping, fast sweep, a blocked pre-trace,
progress, sun error (`sunErrorDeg`), tolerance, sweep duration, last
sweep duration, sweep count, slot counts.

## 14. Test plan

All scenarios run in testGI on DX12 with the `swrt` preset (12.4).
Scenarios S7 and S10 also use the other presets. Each result goes into
the log with `rtss.stats`.

| Id | Scenario | Pass criteria |
|---|---|---|
| S1 | Static camera, static sun | After the first frames: no jobs, `sunErrorDeg` 0 |
| S2 | Static camera, `animate`, 24-minute day | With `rtss.update_pages_per_frame 6` (above the 5.2 pages of 8.4): about 5.2 pages/frame on average at 60 fps (310/s), never more than `batchFrames * updatePagesPerFrame` outside REQUIRED; `sunErrorDeg` at most `tolerance_k` for each level, 5% more in the frame a sweep starts and up to 1.24 times in the frames where the tolerance clamp falls (15); fallback view: no fallback color inside a level cube, except seam bands and the frames of a sweep to a new `sy` (6.6); no visible pops |
| S3 | Walk at 6 m/s, animated sun | About 0.16 PRETRACE pages/frame at the defaults (0.23 on a diagonal at the same speed: sqrt(2) times); REQUIRED after the first frames only when a sweep blocks a pre-trace (8.6) or past the +u edge of a sweep (8.2, `pagesEdge`); not decided (15) |
| S4 | Fly at 50 m/s, animated sun | About 1.3 PRETRACE pages/frame at the defaults (1.9 on a diagonal at the same speed). With the 5.2 sweep pages of S2 this is more than the update budget of 3, so the sweeps go late and the stats show it; H grows (6.3); the fallback rate is logged with `rtss.debug 4`, whose pass counts the fallbacks |
| S5 | Teleport (`camera.restore`) | The first frame after has REQUIRED jobs; the coarsest level is complete in the frame that `validityBurstPagesPerFrame` and the update budget allow (the second at the defaults), the finer levels at up to `batchFrames * updatePagesPerFrame`, level 0 first; the inner pages of the other levels (8.2) come last; never a shadow from the old place, and no lookup with no data inside the coarsest level |
| S6 | `time_skip` (7.5 deg, above the 5 deg fast limit of W = 8; with `sy = 2` also above its 3.84 deg window limit) | All levels sweep fast in the REQUIRED class, the coarsest first, in two frames at the default burst of 32; no read with the wrong transform; fallbacks only near the +u faces and only in the frames of the sweeps, none after them except seams |
| S7 | Low sun (elevation 3 to 5 deg) | Long shadows of buildings are correct in the difference view (terrain off, 12.6), terrain shadows in the visibility view; build cost is logged |
| S8 | Sun goes below the horizon and up again | Visibility 0 below; no sweep starts below the horizon; the first sweep of each level after the sunrise is fast |
| S9 | `rtss.invalidate_around 20` | DIRTY jobs only for the slots in the box; no pop outside it |
| S10 | Each preset of 12.4 | After the rebuild, no no-data pixel inside the level cubes (view 4); `swrt` difference view at a sun of 40 deg or higher: less than 1% of pixels, all on shadow edges (a lower sun: the normal offset issue of 15) |
| S11 | Device reset (`app.reset_device`) | The slots come back in the next frames without errors |
| S12 | GI with the rtss | No bright or dark leaks at ray hits in the GI views; GI gate opens |
| S13 | Camera goes up and down 50 m along the sun axis, static sun | No jobs: the move along the sun axis needs no scroll (8.6) |

Measure and record (not pass criteria, input for the tuning):
- GPU time of the tracers for 8, 16, 24 and 32 pages, SWRT and HWRT,
  and rays/ms for sun-parallel build rays.
- GPU time of the raster producer for 16 pages at levels 0 and 3, with
  the heightmap and without it.
- GPU and CPU time of each preset at the budget, for the `pageCost` of
  the recipes.
- The lookup cost in `combine_shadows` and in the GI passes, against
  the classic static shadow.

## 15. Open issues

- **Terrain outside the max grid window.** The grid window is the terrain
  limit. The testGI grid of +- 1 km covers the windows of all levels of
  the defaults (12.4), but not the casters more than 400 m sunward of
  level 4, which a very low sun has. A game with a larger far level
  needs a second, coarse terrain trace or a shared grid of larger cells.
- **SWRT cost of long rays.** A closest-hit walk visits the nodes on the
  ray up to the hit. A low sun makes rays of kilometers that miss the
  terrain grid. A per-page leaf range (like the SWRT tile mask) is a
  possible optimization. It needs a closest-hit variant with a start and
  end node.
- **HWRT content that streams.** The LRU collision
  window invalidation of 12.4 retraces whole HW levels at each settle. A
  delta box from the bvh library would make it smaller.
- **Mixed sun directions across levels.** A seam band can mix two levels
  with different sun directions. Not checked yet in S2 and S7.
- **Tolerance for each level.** The doubling is a heuristic. Not checked
  yet with tall casters.
- **Raster cull in a game.** The testGI callback culls the trees and the
  level scene by the page frustum only. The rendinst cull of a game
  integration (DNG, WT) can do better, and so lower the `pageCost` of its
  raster levels.
- **Small dispatches.** Each dispatch waits for its slowest rays, so a
  frame that traces costs at least about 30 us GPU: a level 0 page costs
  31 us alone, 2.6 us among 24 and 1.8 us in a whole level. The
  pre-trace pages and the sweep pages of a sun animation go in batches
  (8.2), a sun faster than the budget too.
- **Memory of the inner pages.** The inner pages (8.2) keep their slots.
  A page indirection could give back at most 6% at the defaults.
- **Large worlds.** The library is camera-relative (5.5). The limits
  that stay are the float world positions of consumers that have no
  camera-relative position, the float world box, the world-space SWRT
  data, and the 16-bit
  page coordinates of a slot (5.3).
- **Alpha-tested casters.** Not in SWRT. The raster recipes draw them,
  and `rtss_trace_hwrt_cs` alpha-tests the main TLAS (7.2); with the testGI
  trees it costs 0.19 to 0.37 times a SWRT page.
- **Throughput.** All budgets assume cheap coherent build rays. On an
  RTX 4090 laptop, SWRT `rayTLAS`, testGI berlin: 206k rays/ms with the
  sun at 60 deg, 130k at 10 deg, so the 21 sweep pages of a frame (85k
  rays) cost about 0.4-0.7 ms. With the terrain march: 54k rays/ms at
  60 deg and 25k at 10 deg, 3.9 ms for 24 pages at a low sun.
- **Normal offset at a low sun** (S7, S10): the offset lifts
  the receiver, which shortens a shadow by the lift / tan(elevation);
  view 5 finds 1.5 % of the pixels too light at 20 deg and 14 % at 4
  deg.
- **A patch lit against the reference at zenith 70** (S10):
  0.26 % of the pixels at the bottom of the screen, not at zenith 50,
  not explained.
- **Raster at speed and on a teleport** (S4, S5): 8 raster
  pages in each frame fell 1.5 s late at 50 m/s. At cost 1.5 a frame has
  16 raster pages, and the validity burst of a teleport has 170 (about
  2.4 to 9 ms of the testGI CPU).
- **Tolerance clamp** (S2): the clamp to half the fast limit
  can fall below the error of a level for a moment when the sun sinks
  (1.24 times at level 4).
- **Page budget for other layouts** (5.1.1): smaller pages
  need a larger `updatePagesPerFrame` for the same sweep speed; a `pageCost` in
  proportion to `S^2` would keep one budget for all layouts.
- **A REQUIRED page with no blocked pre-trace** (S3): 1 page
  in 1 of 519 samples with each producer during a walk, not explained,
  so S3 is not decided. The ratio of the two S3 legs (straight and
  diagonal) is not explained either.

## 16. Deferred work

Changes that a review of the worst frame and of the convergence found,
which wait for the traced modes of War Thunder, for a faster sun or for
a change of the testGI frame:
- **Added riExtra in the traced modes (WT).** A TLAS that adds a
  riExtra invalidates all levels, also the raster levels of mode 3, so
  in a battle each destruction rebuilds every page, about 23 frames at
  the full budget. The boxes of the added instances can go to
  `invalidateBoxes`, as the destroyed ones do, of the traced levels
  only, with a full invalidate for the first TLAS and after a device
  reset.
- **A destroyed riExtra on the traced levels (WT).** Its box goes to
  all levels at once and again when the TLAS without it is on the GPU,
  and the first trace reads the old TLAS. Only the raster levels need
  it at once.
- **The TLAS of the rtss (WT).** The SWRT TLAS is built and uploaded in
  each frame while the rtss uses it, also when nothing changed. It can
  skip the build when the screen shadows are off and no add or remove
  waits.
- **Sweep calls that run ahead.** A paced sweep of a raster level builds
  about one page in a call. A call could take the next pages of its row
  at page cost, before their deadline: up to 8 times fewer calls, with
  the same worst frame, but more GPU in the frames with a call. At the
  real-time sun a level sweeps every few minutes, so this waits for a
  project with a fast sun; it bends the pacing rule of 8.2.
- **Region culls in testGI.** testGI culls its raster regions after
  `update()` and waits for them, while WT culls them during the frame.
  A `render()` at the end of the testGI frame removes the wait, with
  one frame of latency.
