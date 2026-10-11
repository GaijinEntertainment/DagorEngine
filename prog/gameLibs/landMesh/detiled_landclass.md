# De-tiled landclass

A `texture`-only landclass (and the colormap of a detailed one) repeats
with its tile period. De-tiling shows up to `LAND_DETILE_MAX_CLUSTERS`
turned and shifted copies of the tile (clusters) and lets a smooth
world-space noise pick which two clusters blend at a pixel. Off by
default: a landclass without the keys renders exactly as before, and an
all-off level takes the old shader path.

`detiled_landclass.html` next to this file is the same subject for
artists: an interactive page with the dune and forest recipes. It runs
a copy of the de-tile math and repeats the setup guidance below, so an
edit to either must update it.

## What it buys

One tile covers a landclass without its period showing, at no cost in
level data: the keys sit in the landclass asset and the level ships
unchanged. The same map turns the objects the landclass generates, so
tiled entities and planted objects keep the placement they were authored
against instead of drifting off the pattern.

The price is one noise sample per pixel, a second texture tap inside the
blend band, and the tile has to be stochastic.

## Parameters

Per landclass, in the `detail{}` block of `<name>.land.blk`:

```
detail {
  texture:t="dunes_tex_d"
  size:p2=512,512
  detileClusters:i=3        // 0 (default) = off; 1 = plain rotation by mainDir; 2..8 = de-tiled
  detileMainDir:r=30        // degrees, world direction of the texture U axis
  detileAngleSpread:r=20    // degrees, cluster k >= 1 takes its own stratum of mainDir +- spread (see The map)
}
```

The keys travel in the existing `detail{}` DataBlock: no bindump or
`land` resource format change, and daBuild and the editor export copy
the block as is. `LandDetile::load` (`publicInclude/landMesh/landDetile.h`)
is the one parser: clusters are clamped to `0..LAND_DETILE_MAX_CLUSTERS`,
and `size <= 0` keeps de-tiling off.

Per level, `landDetileNoiseSize:r=497` (metres per noise feature): one
noise for every landclass of the level, so a later single-pass renderer
evaluates it once per pixel. A game that renders passes it to
`land_detile::set_noise_size()` at landmesh load (WT `strmLevel.cpp`,
DNG `levelES.cpp.inl`); a dedicated server loads no shader dump, so it
calls `set_noise_size_cpu()`, which its RI generation reads. The
harness reads the size from `settings.blk`.

## Setting it up

1. Open the landclass asset, `<name>.land.blk`, and find its `detail{}`
   block. Check `size:p2` first: it is the tile period in metres and the
   whole map is built on it, so a wrong size de-tiles at the wrong scale.
   Only a simple or a `detailmap` landclass de-tiles: `LandDetile::load()`
   turns the keys off when `detail{}` has a `shader`, or a `splattingmap`
   and no `detailmap`, so the objects stay on the plain grid that the
   ground of such a landclass shows.
2. Add `detileClusters:i=4` and look at it. Four is what the harness
   gives a simple, detailed or detailed_r landclass whose asset sets no
   count; raise it only while the repeat still shows.
3. For a directional texture add `detileMainDir` (degrees, the world
   direction of the texture U axis), keep the count at 2..4 and give it
   a small `detileAngleSpread`.
4. Rebuild the asset. The keys ride inside `detail{}`, so daBuild copies
   them with the block and no level is re-exported; the daEditor reads
   the asset source, so reloading the asset is enough there.
5. Set `landDetileNoiseSize:r` in the level blk only if the default 497 m
   does not suit the level. It applies to every landclass of the level.
6. Look at the result: in the daEditor viewport, or in the harness, where
   `lc.list` prints what each landclass now does and `lc.debug` tints the
   de-tiled ones. Tune with `lc.detile` and `lc.noise` without a rebuild,
   then write the values you settled on back into the asset.

When nothing changes, the landclass is off (`detileClusters:i=0` or a
`size` of 0), or the renderer never created the noise LUT, which takes
the level down the old path as a whole. When the texture goes soft in
patches, the tile is structured; see below. When dunes fight each other,
the spread is too wide. When objects move but the ground does not, the
level's noise size changed: the objects follow the same value.

## Choosing the values

`detileClusters` is how many copies of the tile the landclass shows, and
the smallest count that hides the period is the right one. The ground
costs the same from two clusters up, since a pixel reads at most two
copies whatever the count; an all-off level skips the fold and the noise,
and one cluster takes a single turned tap and no mean. The objects cost
a little more for each cluster: planted generation walks the cell once
and picks the cluster of each spot, and tiled generation reads only
the clusters that can show in the cell.
More clusters also switch more often, in narrower bands spread
over the noise range.

`detileAngleSpread` is the key a directional texture cares about, because
it says how far a cluster may turn away from `detileMainDir`:

- Isotropic tiles - gravel, grass, a canopy seen from above - take the
  default 180 degrees. A forest needs nothing but the count, 4 to 6; the
  trees the landclass plants turn with their cluster, so they stop
  repeating with the texture.
- Directional tiles - barchan dunes, ploughed soil, leaning grass - keep
  their direction only if the spread is small. Point `detileMainDir`
  along the wind, set `detileAngleSpread` to 10..30 degrees, and keep
  the count at 2..4: the clusters then differ mostly by their offsets,
  which is enough to break the period, and every crest still reads as
  the same wind. The full 180 degrees turns dunes against each other and
  the desert stops making sense.
- `detileClusters:i=1` turns the tile to `detileMainDir` and does nothing
  else. It aligns a texture to the world; it hides no repeat.

`landDetileNoiseSize` is the world size of one noise feature, so roughly
how far one cluster holds before another takes over. It is per level, and
the same value drives the objects, so a level that changes it moves them.

## What not to de-tile

A structured tile - fields, paving, anything whose pattern the eye
continues - shows both clusters in the blend band as a double exposure.
De-tiling is for stochastic textures, and the band is 60% of the noise
range, not a corner case.

## The map

Cluster `k` is a rigid transform of world metres: `q = R(angle_k) p + ofs_k`.
Cluster 0 is exact (`angle_0 = mainDir`, `ofs_0 = 0`, so
`detileClusters:i=1` is a plain rotation); clusters `k >= 1` take a
stratum each of `mainDir +- spread`, jittered inside the middle half of
it by a pcg hash, and an offset from the R2 low discrepancy sequence.
Both spread by construction; independent draws could leave a pair
close in angle and in offset at once, one cluster the landclass pays
for twice. The rendInstMath selftest pins one cluster offset of the sequence and
holds the minimum separation at every count; it does not pin every value,
and the shader twin `detile_cluster` agrees by sharing
`land_detile.hlsli` and by being changed with it, not by a test.
No table is uploaded: the shader computes its clusters, and the runtime
computes its CPU table once per landclass at load. The map needs no tc
origin, so everything derived from the tile can follow it with the same
math on the CPU.

Selection: `n = smooth_noise(p / noiseSize)` (quintic value noise over
the 64x64 `noise_64_tex_l8` LUT, `render/shaders/noise/smooth_noise_lut.dshl`),
`s = n * (clusters - 1)`; cluster `floor(s)` fades into `floor(s) + 1`
with a hard core of `LAND_DETILE_HARDNESS` on each side and a smoothstep
between (40% of the noise range shows one cluster, 60% blends two). The
selection repeats with the LUT every `64 * noiseSize` metres; the
cluster transforms are period-free, so only which pair blends where
repeats, with the same tiles.

Blend: `c = saturate((lerp(c1, c0, w) - mean) * rsqrt(w^2 + (1 - w)^2) + mean)`,
variance preserving; `mean` is the coarsest mip of the blended texture
(the colormap, or a slot's detail texture) read on the GPU (correct
per channel, always resident under streaming).

Per pixel of the clipmap tile fill (`shaders/land_detile_inc.dshl`):

```
uv      = worldPos.xz * land_detail_mul_offset.xy + land_detail_mul_offset.zw  // as before
p       = detile_pixel(worldPos.xz).world   // a mirrored border cell reflects the map
n       = smooth noise of p, one per pixel, shared by every slot
seed, w = detile_nodes(n * (clusters - 1))
uv_k    = uv + (q_k(p) - worldPos.xz) * (tile, -tile)
c0      = SampleGrad(uv_seed, the tc gradients turned with the cluster)
c1      = SampleGrad(uv_seed+1, ...) where w < 1
```

`SampleGrad` with the turned gradients keeps mip selection right and
puts no gradient op inside the branch; rotation keeps lengths, so
anisotropy is unchanged. A plain slot of an on level samples with the
state's gradients too, since its quad can straddle a de-tiled cell. The
noise is computed once per pixel, and the nodes, the two cluster
transforms and the mean once per landclass slot into a `DetileState`
(once per pixel in the per-landclass draws); each tap then costs one or
two `tex2Dgrad`. Off is the exact old path.

Mirrored border cells: the weights fold picks the mirrored cell's
record, so `detile_pixel` reflects the world position the same way, and
the tap offset starts from the raw pixel, whose `uv` is not reflected:
the band beyond the map shows reflected placements of the tile,
continuous at the mirror line. There the ground moves against the raw
pixel on the reflected axis, so the tc gradients and the finite
difference offsets (`tc_ofs`) take the fold's sign before they turn.

## Code map

- `shaders/land_detile.hlsli`: `LAND_DETILE_MAX_CLUSTERS`,
  `LAND_DETILE_HARDNESS`, the 16-byte params record (clusters, angle,
  spread, size); shared with C++.
- `shaders/land_detile_inc.dshl`: `INIT_LAND_DETILE()` binds the params
  buffer and the noise LUT at fixed registers next to the weight atlas
  cell records (the land renderer nulls a sampler range per draw, and an
  allocator-assigned slot could land in it). `USE_LAND_DETILE()`:
  `detile_pixel`, `detile_pixel_state`, `detile_sample`,
  `detile_slot_sample_ofs`. `GET_DETILED_DETAIL_TEX_WEIGHTED()` is the
  seven-slot mixer of the trivial fill and the grass mask.
  `USE_LAND_DETILE_COLORMAP_STATE()`: the per-landclass draws set a
  PS-scope `land_detile_ps_state` once at entry, so WT's
  `SAMPLE_COLORMAP` inside `AdaptiveSampleColorTexture` reads it with no
  signature change (mega, indexed and decals keep their own define).
- `LandClassDetailTextures::detile`, parsed by `load_land_class`
  (`lmeshLandClasses.cpp`).
- `LandVtexRenderer::uploadDetileParams` (`lmeshRendererLandClasses.cpp`):
  one record per landclass in a ByteAddressBuffer indexed by the
  landclass id. A shader resolves any cell slot's id from the weight
  atlas cell record (`land_weight_cell_slot_id`), so no draw carries the
  params, and the seven-slot shaders de-tile every slot in one pass.
  `land_detile_on` is the level gate: an all-off level allocates no
  buffer and takes the exact old path; so does a level whose renderer
  never created the noise LUT (a null-SRV read is platform-defined), and
  so does the no-atlas fallback (one landclass, no cell records bound).
  The gate stops the reads, not the binds:
  every variant of a shader that expands `INIT_LAND_DETILE` declares
  both slots, so `bindDetileParams` binds both on each pass,
  null when absent: a stale resource of another type fails validation.
- The noise LUT (`init_and_get_l8_64_noise`) is held by the rendering
  client (WT `AcesApp::initRenderer`, DNG `WorldRenderer`, the harness,
  the editor viewport): landMesh itself cannot link gameLibs/render,
  dedicated servers link landMesh without it.
- `LandMeshRenderer::applyDetileOverride`: runtime edits for tools; the
  next `prepare` re-uploads, the caller invalidates its clipmap.

Not changed: which landclass a spot gets (masks, the weight atlas, the
cell records); the WT LC_MEGA, indexed and decal shaders (a hook there
must call `land_detile_colormap_state` before any `SAMPLE_COLORMAP`
tap); the detail texture and the flowmap of LC_DETAILED. Structured
textures (field patterns) show a double exposure in blend zones: this
technique is for stochastic textures.

## Everything that follows the tile

Everything an artist derives from the tile texture has to follow it:

| consumer | where | status |
| --- | --- | --- |
| colormap | `land_mesh_landclass_simple/_detailed` PS | done |
| grass mask | trivial shader `grass_mask` mode, 7 slots at once | done |
| editor grass placement | `de3_genGrassByDensGridMask.h` | gap: no cluster walk, so editor grass keeps the plain grid |
| tiled entities `resources{}` | `riGen/genObjByMask.h` + editor `de3_genObjByMask.h` | done |
| planted `obj_plant_generate{ densityMap }` | `riGen/genObjByDensGridMask.h` + `de3_genObjByDensGridMask.h` | done, except in a spline polygon that draws its own surface |
| planted without `densityMap` | uniform scatter, no tile alignment | nothing to follow |
| editor `images{}` via `ScriptParamImage` (script colorgen, image brush) | `hmlGenColorMap.cpp` (`calcMapping`) | done for unclamped world-mapped images |
| layered colorgen (`PostScriptParamLandLayer`) | its own image sampling | gap: not de-tiled |
| tiled entities inside a spline polygon | editor `de3_genObjInsidePoly.h` | done where the polygon leaves the ground to the landmesh and sets no tiling angle of its own |
| WT detailed / mega detail textures | their own `FLOWMAP_PS` rotation | unchanged |

One rule for all: a landclass maps a world position `p` to a cluster `k`
and a rigid transform `T_k` of world metres. Textures sample at
`tc(T_k(p))` and blend with `k + 1` near a transition; discrete things
(objects) use `k` only and switch where the blend weight crosses 0.5, in
the middle of the blend band.

The twin picks its cluster from the raw world position, where the shader
picks from the mirrored one, so the two agree everywhere but the map's
outer band, which the border cells reflect.

The CPU twin is `land_detile::` in `publicInclude/landMesh/landDetile.h`
(header-only, no d3d: dedicated servers generate RI too), with
`render/l8_64_noise_lut_api.h` as the LUT byte's one home for the GPU
fill and the twin. `ClusterWalk` is the shared core of the de-tile
passes. Landclasses without the keys run the old code path unchanged
(one pass, the identity map); a de-tiled one places by its own rules,
not by the plain random sequence:

- Tiled (runtime and editor): `clusterRange` gives a range that holds
  every cluster that can show in the cell rect (a float margin can add
  one at an end), and only those are walked. For each,
  the rect is mapped into pattern space (`walkRect`), tile spots there
  are mapped back to the world (`toWorld`), and a spot is kept
  where the cell's rect `owns` it, the mask allows it, and its cluster
  `shows` there. The runtime reads the spots of a rect
  through an x-sorted index (`TiledSpots`); every spot seeds
  from `tiled_spot_seed`, so the visit order changes no spot's randoms.
  The editor's spline polygon walks every cluster, keeps a spot by
  `shows` and `point_inside_poly`, and keeps the plain random sequence.
- Planted (`plant_world`): the world grid of blocks (16 to 256 density
  cells a side, grown toward 64 metres) is walked once. A block is skipped
  where the density map is empty under its footprint for every cluster
  it can show (the runtime reads one bit per density leaf,
  `LeafOccupancy`, built at load), and in the runtime also
  where the landclass mask is empty (the editor tests the mask
  per spot only). A block visits its cells in an order that its seed
  scrambles (`block_cells4`), each cell once before any repeats, as the
  plain walk draws its cells. Four spots share each vector op: their `spot_hash`
  jitter and positions, then the density cell in a block of one cluster,
  or else the cluster of each spot. A spot is kept
  where the density map is set at its pattern-space position,
  and only a kept spot hashes its seed. Every spot seeds from a hash
  of its block and index, so the pass order changes no placement.

`noise_range` gives the exact range of the noise over a box:
inside one LUT cell the value is bilinear in the monotonic fades,
so the extremes are at the corners. The skips rest on it and only drop
spots that the per-spot tests drop. An object turns with its tile -
`rotateTm` for the tiled pattern, `rotateTmLocal` on the planted terrain
basis, so the sampled up axis stays - and a posInst entity re-picks
its rotation palette id from the turned axes.

Determinism (the RIGen contract: one compiled binary places identically
on every machine): the selection uses the LUT, the hash, `floor` and
`mul` only, and the one transcendental in the placement math, the
cluster `sincos`, is vecmath's polynomial, not libm. The posInst
palette re-pick (`clamp_euler_angles`) does evaluate libm `sinf` /
`cosf` per palette entry and stays stable: entries sit whole palette
steps apart, far beyond any ulp, so the nearest-entry pick cannot
flip; the contract is per compiled binary regardless. The GPU keeps its
own `sincos`; that ulp-class difference moves the shown pattern against
the objects by about a millimetre at 10 km, and between differently
compiled binaries it can flip a pick inside the blend band.
`prog/samples/unittest/rendInstMath -selftest` pins the
conventions, the hash bits, the LUT byte, the noise and node values, the
params record layout and the walk (CI: `land_detile_unit_tests`).

The per-level noise size reaches the CPU through the same
`land_detile::set_noise_size()`; `land_detile::noise_size` lives in the
landMesh-rendInst bridge (`rendinstIntegration/riLandClass.cpp`) because
rendInst links that bridge without the renderer. The daEditor reads the
level blk it already loads for the skies and the grass, and sets the
size on both sides: `land_detile::set_noise_size_cpu` for the plugin's
own generators and the color bake, and `IHmapService::onLevelBlkLoaded`
for the viewport and the services, which are a separate module with its
own copy.

## Harness (samples/testLandClass)

`lc.list` prints the landclasses with their de-tiling;
`lc.detile <name|*> <clusters> [mainDirDeg] [spreadDeg]` edits live, no
re-export; `lc.noise <metres>` sets the level noise size; `lc.debug`
tints the clipmap by family (red de-tiled, green mega, blue indexed).
`settings.blk` carries `landDetileNoiseSize:r` and
`landClassOverrides{ <name>{ detileClusters:i=6 ... } }`; the harness
de-tiles a simple, detailed or detailed_r landclass whose asset sets no
count with 4 clusters (WT levels cannot be re-exported from here).
