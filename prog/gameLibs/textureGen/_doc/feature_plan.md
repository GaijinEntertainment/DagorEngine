# textureGen: node feature plan (Substance import and Material Maker coverage)

One ordered plan for the node work of textureGen. It joins two goals: the import of Substance
Designer graphs, and the node features that Material Maker has and textureGen does not have. A
feature that both goals need is one work item. `graph_text_format.md` describes the IR and the das
front-end that this plan builds on.

Terms. SD = Substance Designer. An SD "atomic" node is a built-in filter (`compFilter`); a
"library" node is an instance of another graph (`compInstance`): `sbs://` graphs come from the SD
standard library, local ones from `tools/substance_library` (Gaijin graphs, kept in CVS). A "kind"
is one atomic filter, one `sbs://` graph or one missing local graph; a local graph or a helper
graph counts as the kinds inside it. MM = Material Maker; an "MM row" is one row of the Material
Maker survey (a node type or a graph feature). An "HLSL block" is the HLSL of one node in the
fixed layout of section 6; a "region" is a set of HLSL blocks fused into one pass.
A "placeholder" is a constant node in place of a kind that the importer cannot lower;
a "stand-in" is an existing texgen node, or a short chain of them, with another look or other units.

Numbers come from three surveys: a census of the 181 SD sample graphs of the art team, a survey of
textureGen against the kinds that they need (have, partial or missing per kind), and a survey of
Material Maker (master 7d6daae3c74c, 399 node files).

## 1. Goal and scope

- Import SD `.sbs` material graphs into textureGen, and get the packed game outputs (`tex_d`,
  `tex_n`, `tex_r`, `tex_ao`) that look like the SD renders.
- Give hand-built graphs the Material Maker node set where it matters for baked texture maps:
  scatter, seeds, patterns, fill data, tileable noises, node upgrades, parameter expressions and
  loops, in this order.
- Write every new node as one HLSL block in one fixed layout, then fuse chains of blocks into one
  pass of plain HLSL (section 6).
- The editor offers every new node as a normal node, so artists can also use the import nodes by
  hand.
- Not in scope: `.sbsar` (compiled archives); bit-exact results for random nodes (the SD random
  generator and noise tables are not public); SD frames and comments; exposed SD graph parameters
  (no sample graph has one); MM painting, 3D preview and editor UI; the MM rows that section 2.2
  lists as not planned.

## 2. What the inputs say

### 2.1 The 181 SD sample graphs

| What | Value |
|---|---|
| Files / graphs | 181 / 188 (7 files add a helper graph) |
| Nodes per graph (min / median / max) | 14 / 106 / 201 |
| Atomic filter nodes / library instances / outputs | 9145 / 8570 / 1520 |
| Exposed parameters, graph input pins | none: every graph is a closed generator |
| Atomic filter types used directly | 20 (FX-Map is a 21st, inside local graphs) |
| Library graphs instanced directly | 124: 113 `sbs://`, 8 local, 1 local file missing, 2 helper graphs |
| Kinds needed in total / per graph (median) | 135 / 33 |
| Graphs that need FX-Maps / pixel processors | 139 / 112 (through local graphs, see below) |
| Parameters driven by a function graph | 27; 24 fold to a constant at import, 3 are per pixel |
| Graph size | 2048 (absolute) in 107 graphs; generators run 2x to 16x the graph size |
| Nodes larger than 4096 in SD | 162 (must be clamped) |
| Outputs used by the game | `tex_d` 177, `tex_n` 177, `tex_r` 30, `tex_ao` 10 |
| Reference PNGs | 409: 2048^2 343, 256^2 45, 512^2 14, 4096^2 6, 512x256 1; 301 are 16 bit |

Atomic filters (nodes / graphs): blend 3981 / 179, gradient map 1294 / 178, levels 827 / 164,
uniform color 734 / 180, transformation 2D 590 / 149, directional warp 432 / 116, blur 393 / 133,
distance 256 / 91, sharpen 182 / 140, dynamic gradient 145 / 85, normal 87 / 67, warp 86 / 77,
directional motion blur 56 / 46, curve 35 / 30, HSL 22 / 17, bitmap 14 / 6, passthrough 6 / 2,
pixel processor 3 / 3, grayscale conversion 1 / 1, SVG 1 / 1. Blend modes in use: copy, overlay,
multiply, subtract, max, min, add, add_sub; 16% of the blends have a mask.

Most used `sbs://` graphs (graphs that use them): rgba_merge and rgb-a_merge 179, auto_levels 177,
invert 177, curvature_smooth 165, curvature 160, rgba_split 158, histogram_scan 147,
non_uniform_blur 129, highpass 114, gradient_linear_1 94, perlin_noise_zoom 94, shape 94,
bnw_spots_2 (two versions) 89 and 88, white_noise 86, blur_hq 83, hbao 81,
height_to_normal_world_units 78, cells_1 73, tile_sampler 71, mosaic 67, pattern_shape 67,
fractal_sum_base_2 64.

Local graphs (graphs that use them, what they need): bilinear_warp 109 (4 directional warps),
particle_scatter 97 (FX-Map), roughness_to_smoothness_conversion 86 (pixel processor),
noise_crumple 61 (FX-Map), brick_tile_generator 40 (FX-Map), microdetail_index 23 (pixel
processor), highpass_hq_grayscale 8, uv_rotator 7 (pixel processor). So FX-Maps come only from
three Gaijin graphs, and pixel processors mostly from three more; 3 graphs (moss_coat_a,
wood_plain_board_tile_a and _b) have a pixel processor of their own.

Missing inputs: 11 bitmap / SVG resource files (15 references in 7 graphs), and the local package
`gradient_dynamic_u_variation.sbs` (5 graphs). The art team has to supply them.

The long tail is the standard library: 113 of the 135 kinds are `sbs://` graphs whose sources ship
with SD and are not in the repo.

### 2.2 Material Maker

Material Maker is an open source (MIT) procedural texture generator on Godot 4. The survey read
399 node files (`.mmg`: 340 shader nodes, 52 graphs, 7 materials) and 328 library entries. It has
123 rows: 20 that textureGen has, 45 partial, 58 missing.

How MM runs a node: most nodes are shader nodes. A node does not make an image; it makes GLSL that
computes a value for a uv. Connected nodes inline into one shader, and an image exists only at a
Buffer node or at an output. In textureGen every node is one pixel shader pass (or a C++ node with
several passes) into a register: a texture with its own size, format and wrap rule.

What this means for a port:

- Pointwise and gather nodes port well. Each becomes one HLSL block: `$in(uv)` becomes a read,
  parameters become constants. MM has its own GLSL to HLSL text filter (`gen_material.gd`,
  `hlsl_defs.tmpl`) that shows the renames. A port keeps the MIT notice (Rodolphe Suescun and
  contributors).
- Nodes that sample an input many times (blur, warp, tiler) are cheaper in textureGen: they read a
  finished register, where MM needs a Buffer in front of them.
- Inputs that run again with another seed (Variations, the variations option of Tiler and
  Splatter) need fusion first.
- Code-only wire types (SDF 3D, 3D textures) have no register form. SDF 2D and fill data fit: a
  float register holds a distance, an RGBA register holds a bounding box.

This plan covers 63 of the 103 partial or missing rows; section 5 names them per phase. Not
planned:

- rows that need fusion and code-only types: Variations, SDF 3D, Render (raymarching), 3D
  textures, Shader materials, Custom code node;
- rows that are mostly editor widgets: Curve / Poly Curve / Splines, Pixels, Distort (lattice),
  Easy SDF;
- rows that need built subgraphs or variadic pins (the IR keeps one input per template in pin, so
  a variadic pin has no IR form): Subgraphs, Variadic nodes, Material workflow, Static PBR
  material node;
- rows of low value for baked maps: Glyph generators, Runes / I Ching / Fibers, Pack nodes,
  Randomize, Supersample, Mesh Map;
- small rows that can join a phase when a graph needs them: Profile, Sine wave, Custom Tiles,
  Tones, Tones Map / Step / Range, Color Pick, Palettize / Pixelize, Color spaces, Normal to Height
  (as a loop graph after phase 9), Bevel, Binary Smooth, Denoiser, Edge-preserving smoothing,
  Swirl / Spherize, Warp Dilation, Crop / Trimsheet, SDF operators, SDF transforms, sdShow,
  Box / Sphere.

Where textureGen is ahead: erosion on the CPU or the GPU with flow outputs; scatter by splat
(particles appended on the GPU and drawn as quads, cheaper than a gather loop for many instances);
per-node size and format rules (any size up to 8192, 12 register formats); the das front-end and
incremental recompute.

### 2.3 textureGen today

The daEditorX GraphEditor offers 176 node kinds: 104 descriptors in `base_nodes.blk` and 72
shader-editor templates with a texture or particle output (it lists 77; the other 5 are helpers
with value outputs only), and 8 subgraphs besides the 176. Most run as one full-screen pixel shader
pass; 11 are C++ nodes with several passes or CPU work (distance field by jump flooding,
autolevels, convolution, blur iterational, fill areas, erosion, ...); particles are scattered into
a UAV buffer and drawn as instanced quads. There is no compute shader. The node descriptors and
shaders (`commonData/graphEditor`) are in CVS, not in git.

Shaders run in two ways: HLSL text that the driver compiles at run time (parameters at b0; only
DX11 in daEditorX, see Q9), or dshl variants that dsc2 builds per graph into a bindump
(parameters at b3; dsc2 is a subprocess that runs for seconds on the texgen worker). Inputs are
`t0` to `t15`, with a wrap flag per slot in a constant buffer at b1. Most shaders sample by hand
with texel fetches. A node packs at most 128 floats of parameters.

Against the SD node set, the inventory checked 65 nodes: 19 exist, 33 exist in part, 13 are
missing. Against the 135 kinds that the samples need: 26 have a texgen node (maybe with a
parameter formula), 67 are partial (a chain of nodes, a stand-in, or a common option is missing),
42 are missing. Only 2 graphs need no missing kind. The three large gaps:

1. FX-Map and the scatter family (tile sampler, splatters, spot and cell noises). The particles
   path is a base, but it has no quadtree and no pattern choice per instance. Phases 2 and 6.
2. Function graphs: no pixel processor in the IR (shader templates exist, but only the legacy web
   editor makes new ones), no value processor, and scalar pins take constants only. Phases 1, 8, 11.
3. Graph structure: no front-end fills the IR subgraphs and the conversions skip them; the IR
   has no graph seed and no headless compile (the IR to command BLK compile lives in the editor
   plugin); the `texgraph` tool knows only `base_nodes.blk`. Phases 0 and 3.

Semantic differences the importer must handle: no implicit gray / color conversion; float
registers do not clamp; one wrap flag for both axes, correct only on power-of-two sizes; no mips
(a large reduction aliases); `get normal` writes DirectX-style normals with a cell size, not an
intensity; units mix uv fractions, degrees, turns and radians; random nodes use other hashes than
SD.

The texture node loads its file as a managed texture, so it uses the engine image loaders that the
tool registers: SVG (nanosvg), PNG, JPEG, PSD and AVIF in daEditorX, all but PNG and AVIF in
`texgenRun`. The file picker of the node (`base_nodes.blk`) lists only .dds, .ddsx, .tga and .tif.

## 3. Decisions

D1. The importer emits the IR. `.sbs` -> `texgen::Graph` -> `ir_to_graph_data` -> editor graph BLK,
and `graph_to_das` for a script. It is a new mode of the `texgraph` tool (`texgraph -import-sbs`),
with its code in `prog/tools/libTools/graphEditor` and the reader on `prog/3rdPartyLibs/pugixml`.
The editor gets an "Import .sbs" command when its import / export script step
(`graph_text_format.md` step 3) lands.

D2. Library instances are flattened into the importing graph. The IR has subgraphs, but neither
`ir_to_graph_data` nor `graph_to_das` converts them, and the importer resolves size, format and
seed per instance anyway. Graphs get larger (median 106 SD nodes, about 2x to 3x after
flattening); revisit when those conversions handle subgraphs.

D3. The SD standard library is rebuilt as own nodes, inside each phase the most used
kind first. Its sources are not here, and an own node is one fused pass where the SD graph is
many atomic nodes. If the sources become available (Q1), the importer can also lower a rare
library kind by importing its graph, which then needs an FX-Map interpreter and the pixel
processor compiler.

D4. Match targets per node class, checked against the reference PNGs: exact math for pointwise
nodes (within 1-2 LSB); close for filters and multi-pass nodes; look only for random generators
and scatters. The artists accept or reject the look on a side-by-side review page.

D5. New SD-compatible nodes take SD units: angles in turns, distances in pixels at a 256
reference, sizes as log2 offsets. The importer then copies values. Existing texgen nodes keep
their units. A node that the import and the MM-level set both need follows this rule (D15).

D6. New nodes go into git, not into the CVS data folder: a node pack (descriptor + HLSL block,
D11) in `prog/gameLibs/textureGen`, which the node registry of phase 0 (0.2) loads in addition to
`base_nodes.blk`. Then shader and code are reviewed together, and unit tests run them on the stub
driver in CI.

D7. One expression tree for function graphs: SD function XML (and, from phase 8, expression text
in a scalar parameter) -> (a) a value that the CPU evaluates in the compile, (b) HLSL for a pixel
processor, emitted as a generated block (D11, phase 11). The three Gaijin pixel processors are
hand-written blocks first (phase 1); the compiler must give the same values. FX-Maps: own
nodes for the three Gaijin FX-Map graphs: particle_scatter and brick_tile_generator on the gather
tiler (D14, phase 2), noise_crumple as a gather noise (phase 6). A general FX-Map interpreter only
if D3 changes or new art needs one.

D8. Seeds of imported nodes: the node key of D13 is a stable hash of (instance path of the node,
its seed in the SD file). The same seed gives the same result; another seed gives another
variant. Values do not match SD.

D9. Sizes and formats: the importer computes the SD size of each node. It writes an explicit size
for `relativeTo` 0 (absolute) and 1 (parent), the rule of 0.5 for 2 (input), and an explicit 4096
(a setting) with a report line for a larger size. Gray formats map to R8 / R16 / R16F / R32F, color
to ARGB8 / ARGB16 / ARGB16F / ARGB32F. Where SD converts gray and color implicitly, the importer
inserts the conversion node, until the compile converts at the edge (phase 7).

D10. A kind the importer cannot lower becomes a placeholder node (a constant 0.5 gray or color)
and a report line. A bitmap or SVG node whose file is missing (Q4) does the same until the file
comes. A partial kind lowers to its stand-in with a report line, until its own node lands. Every
graph imports and runs from the first milestone, and the report gives the missing kinds per graph,
so each new node shows its effect on real graphs at once.

D11. Node shaders are HLSL blocks in one fixed layout (section 6): plain HLSL compiled at run
time, not dshl (Q9 for the compiler on each driver). Every new node from phase 0 on is a block.
A one-block wrapper runs a block alone until fusion lands, so no node is written twice.

D12. Fusion (phase 13) puts blocks into regions by the rules of section 6.4. A fused region gives
the same values as its nodes run alone. Fusion comes after the MM-level phases and after the
legacy conversion.

D13. Seeds: a graph seed in the graph options, and a seed per node that inherits by default:
node seed = hash(graph seed, node key). The node key is the node id, or for an imported node the
D8 hash. A subgraph instance passes its seed to its nodes. A locked seed keeps its value. Existing
nodes with a seed parameter keep their explicit values, so existing graphs do not change.

D14. Scatter is a gather. The tiler and the splatter are blocks in which each pixel visits the
grid cells whose instances can reach it. There is no UAV and no instance counter. The SD tile
sampler family, shape_splatter, splatter_circular, particle_scatter and brick_tile_generator use
them. The particle path stays for the existing particle nodes.

D15. One node per shared feature. Where the import and the MM-level set need the same feature,
there is one node pack, with SD semantics and units (D5) where SD has the node. MM-only options
are more parameters whose defaults keep the SD result.

D16. Order: the MM-level features in the order of phases 2 to 9, then the legacy conversion
(phase 12), then fusion (phase 13). The SD-only phases fit around them; the importer frame comes
first, because it measures every later phase on the 181 graphs.

## 4. Phases

Phases 0 to 9, 12 and 13 run in this order. Phases 1, 10 and 11 hold SD-only work. Phase 10 needs
only phases 0 and 1 (phase 9 only for its loop versions), and phase 11 needs phase 8, so a second
person can do phase 10 in parallel with phases 2 to 9, and phase 11 after phase 8. Phase W waits
for the art team and can land at any time.

| Phase | Content | SD kinds | Graphs that need it | No placeholder | Fully covered | Size |
|---|---|---|---|---|---|---|
| 0 | Foundations: headless compile, registry and packs, HLSL blocks, size rule, validation (5.0) | - | - | - | - | XL |
| 1 | Importer frame, converters, stand-ins, Gaijin graphs (5.1) | 26 + 3 Gaijin pixel processors | 181 | 2 | 0 | XL |
| 2 | a. Gather tiler and splatter (5.2) | 7 + 2 Gaijin FX-Map graphs | 170 | 3 | 0 | L |
| 3 | b. Seeds (5.3) | - | 176 | 3 | 0 | M |
| 4 | c. Patterns and gradients (5.4) | 16 | 170 | 7 | 0 | L |
| 5 | d. Fill data (5.5) | 7 | 58 | 8 | 0 | M |
| 6 | e. Tileable noises (5.6) | 27 + 1 Gaijin FX-Map graph | 178 | 12 | 0 | L |
| 7 | f. Node upgrades and the SD atomic set (5.7) | 20 | 181 | 78 | 2 | L |
| 8 | g. Parameter expressions (5.8) | - | 9 | 78 | 2 | L |
| 9 | h. Iterate (5.9) | - | - | 78 | 2 | L |
| 10 | SD filters, height and normal nodes (5.10) | 29 | 179 | 175 | 175 | L |
| 11 | Pixel processor compiler (5.11) | 3 direct pixel processors | 3 | 176 | 176 | M |
| 12 | Legacy node conversion (5.12) | - | - | 176 | 176 | L |
| 13 | Fusion (5.13) | - | - | 176 | 176 | L |
| W | Missing files from the art team (5.14) | 1 + 11 files | 5 + 7 | 181 | 181 | S |

How the SD numbers are counted (out of 181 graphs; computed from the census, which gives the
kinds each graph needs, also inside the local graphs it instances, and from the have / partial /
missing status per kind of the textureGen survey):

- SD kinds: the kinds that the phase lowers to their own node, converts to an existing node
  (phase 1) or gets as a file (phase W). The phases hold all 135 kinds once; the plain numbers add
  to 133. The other 2 kinds, FX-Map and pixel processor, are split by the local graph that brings
  them (the parts after a "+", and phase 11): particle_scatter (97 graphs), brick_tile_generator
  (40), noise_crumple (61); roughness_to_smoothness_conversion (86), microdetail_index (23),
  uv_rotator (7), and the 3 graphs with a pixel processor of their own (one part, not 3 kinds).
- Graphs that need it: the graphs with at least one kind of the phase. For phase 3, the graphs
  that set node seeds; for phase 8, the graphs with function-driven parameters.
- No placeholder (cumulative): graphs whose every kind lowers to its own node, an existing node
  or a stand-in. It counts a stand-in for each of the 67 partial kinds, so it is an upper bound.
- Fully covered (cumulative): graphs whose every kind lowers to its own node, or to an existing
  node with a parameter formula (the 26 kinds that texgen has).
- Options are not counted: gradient map flat and smooth interpolation (20 and 8 graphs), automatic
  mips of transformation 2D (153), per-axis tiling (7) and the like come with phase 7. Seeds
  (phase 3), function-driven parameters (phase 8) and the 11 resource files (phase W) are not
  counted.

Both curves stay low for a long time, because a graph needs 33 kinds (median). Phase 7 removes
sharpen, the most common missing kind (140 graphs); phase 10 removes mosaic (67) and the other
filters. If phase 10 ends before phase 2 starts, the no-placeholder curve reaches 28 after phase 6,
and phase 7 ends at 175 fully covered.

The SD gap survey names phases of an SD-only order. They map to this plan as follows:

| Phase in the SD gap survey | Phase here |
|---|---|
| 0 Prerequisites | 0 (plus 0.3, HLSL blocks) |
| 1 Importer frame | 1; seeds go to 3, constant folding to 8 |
| 2 Atomic nodes | 1 (kinds that texgen has), 7 (the others) |
| 3 Function graphs, Gaijin graphs | 1 (plain graphs, hand-written pixel processors), 2 (particle_scatter, brick_tile_generator), 6 (noise_crumple), 8 (CPU half), 11 (HLSL half) |
| 4a Library filters, shapes, gradients | 1 (kinds that texgen has), 4 (patterns, gradients), 7 (channels, adjustments, transforms), 10 (blurs, warps, height, normal, light) |
| 4b Noise generators | 6; brick_1, weave_generator and cube_3d go to 4 |
| 5 Scatter and flood fill | 2 (tile sampler family), 4 (scratches), 5 (flood fill) |
| 6 Missing files | W |

## 5. Work items

Each item gives what it is, why (SD graph counts, MM rows), its size and what it needs.

### 5.0 Phase 0: foundations (no SD code, no new node)

**0.1 Headless compile** (L; needs: -)
- What: the IR to command BLK compile moves out of the editor plugin into
  `gameLibs/textureGen/compile` (`graph_text_format.md` step 4). `texgraph`, `texgenRun` and unit
  tests run a graph without the editor.
- Why: every later phase tests on it. The seed rule (phase 3), automatic conversion and switch
  (phase 7), expressions (phase 8), loops (phase 9) and fusion (phase 13) live in it.

**0.2 Node registry and node packs** (M; needs: -)
- What: one registry for base nodes, shader templates, subgraphs and node packs. A node pack is
  one folder in git with the descriptor and the HLSL block (D6). `texgraph`, the das generator and
  the GraphEditor node list read the registry.
- Why: every new node of this plan is a pack. MM: Node definition file.

**0.3 HLSL blocks** (M; needs 0.2)
- What: the fixed layout and the block form of section 6, and a wrapper that runs one block as one
  pass on the run-time HLSL path. A pin default can be an HLSL expression of uv.
- Why: new nodes are written once, in the form that fusion needs (D11). MM: Input defaults as
  expressions.

**0.5 Size rule relative to input** (S; needs: -)
- What: "relative to input N" in log2 steps, with the SD primary input (blend: the background).
- Why: 217 nodes in 58 SD graphs set a size relative to their input.

**0.6 Validation harness** (M; needs 0.1)
- What: `texgenRun` writes the finals; a compare tool reads the reference PNGs (8 and 16 bit),
  brings both to one size and reports per channel mean absolute error, histogram distance, a
  structural similarity score and, for normals, the mean angle error. A review page shows SD and
  texgen side by side for the 181 graphs.
- Why: every SD node is checked against D4 with it; phase 13 compares fused and unfused runs with
  the same tool.

### 5.1 Phase 1: importer frame (SD only)

**1.1 Reader** (M; needs: -)
- Both XML spellings (the long one SD saves, the compact one of the split samples); dependencies
  `sbs://`, relative paths and `?himself`; resources; nodes, connections, parameters, parameter
  arrays (gradient keys, curve points), function graphs as trees, outputs with their identifiers
  and usages.

**1.2 Lowering frame** (L; needs 0.1, 0.2, 0.5, 1.1)
- A table from SD kind to a converter that writes one or more IR nodes and maps the parameters;
  defaults for every parameter that the file leaves out (a missing parameter means the SD
  default).
- Size, format and tiling propagation (D9); library instances flattened (D2); placeholders and
  stand-ins (D10).
- Outputs: each game output (`tex_d`, `tex_n`, `tex_r`, `tex_ao` and the numbered variants)
  becomes a final with that name; the PBR template outputs that the game does not use are
  dropped.
- `texgraph -nodes <base_nodes.blk> -import-sbs <file or folder> -lib <dir> -out-dir <dir>` writes
  an editor graph BLK, a das script and a report per graph (D1).
- Seeds wait for phase 3: imported random nodes keep their default seed, and the report lists the
  SD seeds. Function-driven parameters wait for phase 8: they take their SD default, with a
  report line.

**1.3 Converters to existing nodes** (M; needs 1.2, 0.6)
- The 26 kinds that texgen has: uniform color, gradient map, levels, directional warp, blur,
  normal, distance, warp, bitmap, passthrough, rgba_merge, rgba_split, auto_levels, invert,
  histogram_scan, white_noise (two versions), blur_hq (two versions), threshold, quantize (two
  versions), checker, vector_warp, flood_fill_to_random_grayscale (two versions). Each converter
  maps the units of section 2.3 and is checked against D4. A kind whose converter misses its D4
  target gets its own node in phase 7.

**1.4 Stand-ins** (M; needs 1.2)
- A stand-in for each partial kind (D10): an existing node or a short chain. Examples: blend as
  the blend modes plus a lerp for opacity and mask; the SD noises as perlin or voronoi; curvature
  from the height that made the normal. The phase that brings the own node of a kind removes its
  stand-in.

**1.5 Gaijin graphs** (S; needs 0.3, 1.2)
- bilinear_warp (4 directional warps, 109 graphs) and highpass_hq_grayscale (8 graphs) import as
  plain graphs.
- The pixel processors of roughness_to_smoothness_conversion (clamp((1 - x^(1/6)) / 0.7),
  86 graphs), microdetail_index (16 inputs, 23 graphs) and uv_rotator (rotates uv by a map,
  7 graphs) become hand-written blocks (D7). The FX-Map graphs come in phases 2 and 6.

After phase 1 every graph imports and runs; 2 graphs have no placeholder.

### 5.2 Phase 2 (a): gather tiler and splatter

**Tiler and splatter** (L; needs 0.2, 0.3; the SD checks need phase 1)
- What: one instance generator and one family of gather blocks (D14). An instance comes from a
  hash of its cell and the seed, and from input maps (mask, scale, rotation, offset, color,
  value). Each pixel visits the cells whose instances can reach it and joins their values by MAX
  or ADD, or in instance order for the copy and alpha modes (a float target for signed values).
  Pattern inputs form an atlas of 1 to 16 images. Outputs: value, random color, instance uv,
  center uv. Layouts:
  - grid: tile_sampler (gray and color), tile_generator, tile_random, MM Tiler; a brick layout
    with a row offset serves brick_tile_generator (Gaijin FX-Map, 16 brick inputs);
  - rings and spiral: splatter_circular, MM Circle Splatter;
  - random points binned into cells: MM Splatter, shape_splatter (and to mask), and
    particle_scatter (Gaijin FX-Map: rejection sampling against a probability map; size, height
    and rotation from maps).
- SD: 7 kinds in 76 graphs (tile_sampler 71, tile_generator 12; 10 graphs need one of its missing
  kinds), plus particle_scatter (97 graphs) and brick_tile_generator (40): 170 graphs in all.
- MM: Tiler, Tiler Color / Advanced Tiler, Splatter, Circle Splatter. The cell walk is also the
  base of the spot noises of phase 6.

### 5.3 Phase 3 (b): seeds

**Seeds** (M; needs 0.1)
- What: D13 in the IR (a graph seed in the graph options, a seed mode and value per node), in the
  compile (the hash), in the editor (new, copy, paste, lock) and in the das module. A random block
  reads its seed from its parameters. The importer writes the graph seed and the D8 key.
- SD: a graph seed in 99 graphs; inherited node seed offsets on 1015 nodes in 176 graphs. Without
  it, all instances of one noise in a graph show one variant.
- MM: Seeds. It is the base for Variations (after fusion, not planned here).

### 5.4 Phase 4 (c): patterns and gradients

**Patterns and gradients** (L, many S blocks; needs 0.3, phase 3; scratches need phase 2)
- What: one block per family:
  - gradients: SD gradient_linear 1, 2, 3, gradient_circular, gradient_axial; MM linear, radial,
    circular and spiral ramps with repeat, rotate and mirror;
  - shapes: SD shape (17 patterns; library versions shape and pattern_shape) and polygon_2 (two
    library versions); MM Shape (circle, N-gon, star, curved star, rays);
  - tilings: SD stripes, mesh_2, brick_1, weave_generator, cube_3d; MM generic waveform pattern,
    bricks (six bonds, mortar, bevel), uneven and skewed bricks, tile shapes (hex, Cairo, arc
    pavement, mesh), weaves, truchet;
  - scratches: SD scratches_generator, scratches_generator_normal; MM Scratches;
  - SDF 2D: the convention (a raw distance in uv units in a float register) and the MM SDF
    shapes.
- Bricks and tile shapes get a fill data output in phase 5.
- SD: 16 kinds in 170 graphs; 155 of them need one of its missing kinds. The gradients alone are
  in 127 graphs.
- MM: Shape, Gradients, Generic pattern, Bricks, Uneven and skewed bricks, Tile shapes, Weaves,
  Truchet, Scratches, SDF 2D wire type, SDF shapes.

### 5.5 Phase 5 (d): fill data

**Fill data and its readers** (M; needs 0.3, phase 3)
- What: the fill areas node keeps the minimum and maximum x and y of each island through its
  passes, and writes the bounding box per pixel into an RGBA register (fill data). Blocks read
  it: to random gray and color, to position, to size, to uv, to gradient (angle and its
  variation), to orientation, select one area, and the mapper that places an input per island
  (floodfill_mapper). The bricks and tile shapes of phase 4 and the voronoi of phase 6 write fill
  data too.
- SD: 7 kinds in 58 graphs, and all 58 need one of its missing kinds (flood_fill_to_gradient in
  24, 23 and 7 graphs over its three versions, floodfill_mapper in 26).
- MM: Fill, Fill companions.

### 5.6 Phase 6 (e): tileable noises

**Tileable noises** (L; needs 0.3, phases 2 and 3)
- What: one block per noise, tileable for whole periods, seeded (D13):
  - MM: FBM with a base choice (value, perlin, simplex, the cellular variants, voronoise, gabor),
    folds, lacunarity and offset; voronoi and cellular with wrap and a fill data output; triangle
    voronoi, anisotropic, wavelet and shard FBM noise; color value noise; block and color noise;
    Make Tileable; the composite noises (crystal, dirt, directional, clouds) as subgraph files;
  - SD: perlin_noise_zoom, perlin_noise, fractal_sum_base (two versions), bnw_spots 1, 2, 3,
    gaussian_spots 1, 2, cells 1, 3, 4, gaussian_noise, anisotropic_noise, directional_noise 1, 4,
    clouds_1, crystal_1, voronoi_fractal, grunge_map_002, starburst. Six of them have two
    library versions: bnw_spots 1 and 2, gaussian_spots 2, cells 1 and 4, anisotropic_noise. In
    SD most of these are FX-Map quadtrees; here each is one gather block that visits the few grid
    cells per octave that can reach a pixel (about 3x3 per level), calibrated by look against the
    reference PNGs (D4). The spot noises use the cell walk of phase 2;
  - noise_crumple (Gaijin FX-Map: a spiral walk of paraboloids) as an own gather block.
- SD: 27 kinds in 178 graphs; noise_crumple is in 61 of these 178. 63 graphs need starburst,
  grunge_map_002 or noise_crumple, which have no stand-in.
- MM: FBM, Voronoi, Triangle Voronoi, Anisotropic noise, Wavelet noise, Shard FBM, Color value
  noise, Block noise / Color noise, Composite noises, Make Tileable, Tiling.

### 5.7 Phase 7 (f): node upgrades and the SD atomic set

**Node upgrades** (L, many S and M blocks; needs 0.1, 0.3)
- Blend: one block with the SD modes (copy, add, subtract, multiply, add_sub, max, min, switch,
  divide, overlay, screen, soft light) and the other MM modes (23 in all), opacity, mask, crop
  rectangle and alpha mode. SD: 179 graphs, 3981 nodes; crop in 9 graphs, alpha mode in 3.
- Math: one block with an operation choice (the 37 MM operations: log, pow, round, floor, fract,
  mod, trig, step, ping-pong, snap and more, with clamp) and a vec3 version. SD: pow_grayscale
  (21 graphs).
- Transform: transformation 2D as one block with a 2x2 matrix and an offset, matte color, clamp,
  repeat or extend per axis, and map inputs; mirror, skew, safe_transform, cartesian to polar;
  Custom UV (read an input at the uv that a map gives, atlas, random rotation and scale per cell).
  Mips on demand for a register that a node reads with a level (automatic mip mode on 562 of 590
  SD transformation nodes). Per-axis wrap (H only on 28 nodes in 5 graphs, V only on 6 nodes in
  2).
- Filters and adjustments: sharpen (one 5-tap pass, 140 graphs), emboss, directional blur at any
  angle (46 graphs), grayscale conversion with weights and the MM modes, channel shuffle and one
  node each for rgb-a merge and split, contrast and luminosity, histogram scan with a position
  map, invert that keeps alpha, HSL in SD values, curve with Bezier keys, gradient map with
  flat and smooth interpolation (20 and 8 graphs) and no 40-key limit, dynamic gradient by row or
  column, uniform color in any range, normal with an intensity, OpenGL output and height in
  alpha, pbr_albedo_safe_color.
- Switch: picks one of several inputs by a parameter, resolved in the compile as bypass is.
- Automatic gray and color conversion at the edge: one conversion table in the compile. The
  importer then stops inserting conversion nodes (D9).
- Inputs: the texture node file picker gets .svg, .png, .jpg, .psd and .avif; `texgenRun` gets the
  PNG and AVIF loaders; a size reaches the loader as a name suffix, which the file checks of
  `TextureRegManager` strip. Render text gets several lines, alignment and color.
- SD: 20 kinds in 181 graphs; 148 graphs need one of its missing kinds (sharpen 140, pow 21,
  cartesian to polar 11, svg 1, pbr_albedo_safe_color 1). Blend (179) and transformation 2D (153:
  149 use it directly, 4 only inside brick_tile_generator) go from a stand-in to their own node.
- MM: Blend, Math, Transform, Shear / Mirror / Kaleidoscope, Circle Map, Custom UV, Grayscale,
  Sharpen, Emboss, Switch, Automatic type conversion, Buffer (mips), Directional Blur, Invert,
  Brightness / Contrast, Adjust HSV, Colormap, Swap Channels / Set Alpha, Parameter types
  (gradient modes), Image, SVG input, Text.

### 5.8 Phase 8 (g): parameter expressions

**Parameter expressions** (L; needs 0.1, phase 3)
- What: one typed expression tree (float1 to float4, int1, int2, bool; constants, vectors and
  swizzles, arithmetic, compare, logic, ifelse, math, `rand` from the node seed, named
  parameters, the output size) with a CPU evaluator in the compile. A scalar parameter can hold an
  expression instead of a literal: the IR `Value` gets an expression form, and the editor gets a
  text field. Named graph parameters and a Remote node: one control drives parameters of many
  nodes, with named sets of values. Parameters stay constants: a change evaluates again and
  compiles no shader.
- SD front-end (D7): function-driven parameters, package functions (Sigmoid) and the value
  processor read the SD function XML into the same tree.
- SD: 24 function-driven parameters in 9 graphs (blend opacity, uniform color, gradient_linear_3
  position, pattern_shape size; Sigmoid in 3 graphs). No kind changes, so the coverage stays.
- MM: Parameter expressions, Remote node.

### 5.9 Phase 9 (h): Iterate

**Iterate** (L; needs 0.1)
- What: a loop region in the graph between a begin and an end node. The body runs 1 to N times on
  two registers that swap; the iteration index is a constant; an option stops the loop when no
  pixel changes (a GPU count read back after a batch of iterations); an option shrinks the size
  per step. The IR, `graph_to_das` and the editor get a loop form (today `allowLoop` is off and
  `graph_to_das` rejects loops). The scheduler and the order search treat the loop as one node.
- SD: no sample graph needs a loop. slope_blur (32 samples) and mosaic (16) of phase 10 can be
  loops instead of C++ nodes.
- MM: Iterate Buffer, Multi-pass nodes as data.

### 5.10 Phase 10: SD filters, height and normal nodes (SD only)

**SD filters** (L; needs 0.3, phase 1; phase 9 for the loop versions)
- Blur and warp (11 kinds, 170 graphs, 81 with a missing kind): non_uniform_blur (anisotropy,
  asymmetry), highpass (two versions), slope_blur (two versions, min mode), mosaic (two versions),
  multi_directional_warp, non_uniform_directional_warp, vector_morph, fxaa.
- Height, normal and light (18 kinds, 174 graphs, 31 with a missing kind): curvature with a
  normal input, curvature_smooth (a pyramid), curvature_sobel, height_to_normal_world_units (two
  versions), hbao (two versions) and ambient_occlusion_2 with wrap at the borders, rt_bent_normal,
  normal_combine, normal_blend, height_normal_blend, normal_vector_rotation, height_blend,
  height_extrude, shape_extrude, edge_detect, mg_light.
- Cost: slope_blur (32 passes in the samples), mosaic (16) and non_uniform_blur (up to 16 x 9
  taps) are the GPU hot spots at 2048 to 4096; join their passes where the math allows.
- SD: 29 kinds in 179 graphs (97 with a missing kind). This is the last large step: fully covered
  goes from 2 to 175.
- MM: Slope Blur, Edge Detect, Normal Convert / Normal Blend, Smooth Curvature, Multi Warp.

### 5.11 Phase 11: pixel processor compiler (SD only)

**Pixel processor compiler** (M; needs 0.3, phase 8)
- What: the HLSL back-end of the expression tree (D7). An SD per-pixel function becomes a
  generated block with samplers (`samplelum`, `samplecol`; nearest or bilinear; wrap), `$pos`,
  `$size`, casts with the SD int rules, and `rand`. A generated block is cached by the hash of its
  function graph.
- Test: for the three Gaijin pixel processors the compiler gives the same values as their
  hand-written blocks of phase 1.
- SD: the 3 graphs with a pixel processor of their own, and the path for new SD art.

### 5.12 Phase 12: legacy node conversion

**Legacy node conversion** (L; needs 0.2, 0.3, phases 10 and 11)
- What: the existing node shaders become blocks in node packs in git (D6): the 77 shader templates
  of the legacy web editor (SH files with their generated HLSL), and the pixel shader programs of
  `default_shaders.blk` that phases 2 to 11 do not replace. Of the templates, a script converts 62
  from their generated HLSL, the 6 particle templates are moved by hand and keep the particle path
  (not blocks, 6.3), 3 helpers are moved by hand, and 6 with no use are deleted (4 of the 72 of
  section 2.3, and 2 of its 5 helpers). A converted node keeps its `templateUid`, name and pin
  names, so saved graphs load unchanged; the hashed parameter names stay behind a descriptor flag
  until a graph migration (`replaces:t`). Known defects of the legacy shader data are fixed or
  dropped here.
- Why: fusion joins only blocks. Templates are 7083 of the 25442 nodes of the 41 example graphs
  (JSON main graphs that the `texgenRun` benchmarks run) and 47486 of the 179256 nodes of all 242
  JSON main graphs; the 8 shipped subgraphs hold 188. 86% of the 7083 are of 39 pointwise templates.
- Test: `texgenRun` A/B on the example graphs with byte-equal finals, and a one-node test for each
  kept template that no example graph uses.

### 5.13 Phase 13: fusion

**Fusion** (L; needs 0.1, 0.3, phase 12)
- What: section 6: regions, the assembler, the text cache, the line markers, the materialize flag,
  and regions as nodes for the scheduler and for incremental recompute.
- Why: fewer draws and registers, and less GPU time for the flattened SD graphs at
  4096. Test: fused and unfused runs give the same values on the 181 SD graphs and on the example
  graphs.
- MM: Shader assembly. It is the base for Variations, SDF 3D, 3D textures and shader materials
  (not planned here).

### 5.14 Phase W: missing files from the art team

**Missing files** (S; waits for the art team, Q4)
- `gradient_dynamic_u_variation.sbs` (5 graphs) and 11 bitmap and SVG resource files (7 graphs).

## 6. HLSL blocks and fused shaders

### 6.1 Why a fixed layout

A fused shader is plain HLSL text that the texgen worker compiles at run time (Q9). The driver
interface gives no reflection: the CPU cannot ask a compiled shader which textures, samplers and
constants it uses. So every node shader and every fused shader uses one fixed layout, and the CPU
fills that layout from the node descriptors. The same layout serves a block that runs alone (the
one-block wrapper of phase 0) and a region of many blocks (phase 13), so a block is written once.
Fusion does not use dshl: dsc2 runs for seconds per graph, and a region changes when the graph
changes.

### 6.2 The layout

- Textures: slots `t0` to `t15` hold the external inputs of the pass (registers made outside the
  region), one slot per register, in the order the assembler finds them. 16 is the size of
  today's wrap table.
- Samplers: `s0` to `s15`; slot i gets the wrap or the clamp sampler of texture i, as today. Blocks
  read through the shared helpers (`linear_sample`, `get_texel`, `bicubic_sample`), which take the
  wrap flag of the slot.
- Slot constants (b1): the wrap flag of each slot, as today, and its size in pixels.
- Pass constants (b2): the output size and its inverse, and the sub-step offset: values that every
  block can read.
- Parameters (b0): one array of float4 of fixed size. Each node of the pass gets a slice that
  starts on a float4 boundary. In a slice, parameters follow the descriptor order in the
  `create_constant_buffer` layout that the runtime uses today: a value aligns to the power of two
  nearest to its size (a float3 aligns to 2, so it can cross a float4 boundary), and a node has at
  most 128 floats. Before each block the assembler writes one accessor per parameter name that
  reads the slice of that node, so a block uses parameter names and never an offset. A seed is a
  parameter.
- Outputs: `SV_Target0` to `SV_Target7` (the driver limit of 8), all of one size.

### 6.3 The block form

- A block is one HLSL function with a fixed signature per class, plus optional helper functions.
  It declares no texture, sampler, constant buffer or global value.
- Shared helpers come from the fixed prologue (today's `premain`). The helpers of a node kind go
  once into a shader, with a kind prefix; names in a block get the node id as a prefix (the
  `$name` of MM).
- Classes:
  - source: no inputs; a value from uv and parameters (noises, patterns, gradients);
  - pointwise: reads each input only at its own pixel (levels, blend, math, gradient map);
  - gather: reads an input at other places (blur, warp, transform, tiler); its inputs must be
    texture slots.
- An input that is not connected reads its default: a constant, or an HLSL expression of uv.
- A node with several outputs writes several values.
- Not blocks: C++ nodes (several passes or CPU work), particle scatter and splat, UAV outputs, the
  cache node and exports. They run as today.

### 6.4 Regions: what breaks a fusion

The compile groups blocks into regions. A region ends, and its value goes into a register, at:

- a node that is not a block;
- a gather block whose input comes from inside the region. A gather block takes only external
  inputs, so it starts a region (a blur, then levels and threshold in one pass);
- another output size: all nodes of a region have one size and one sub-step tiling;
- the 17th external input, the 9th output, or a full parameter array;
- the code limit of a region (a number of blocks or an instruction estimate, Q6);
- a pin with the "materialize" flag, the node that the editor previews, and the finals.

A node output that a node outside the region reads becomes an output of the region. A value that
two blocks of one region read is computed once. Formats do not break a region: an intermediate
value stays in the shader, but the assembler applies the clamp and the rounding of the format that
its register would have. So a fused region gives the same values as its nodes run alone.

### 6.5 Compile, cache and errors

- The text of a region is the prologue, the blocks and a main function. It is cached by the hash of
  its text, in memory and on disk, and compiled on the texgen worker.
- A parameter change writes constants only and makes no new shader. An option that picks code (an
  enum that changes the block text) makes a new text, which the cache has when it was used before.
- The assembler writes a `#line` marker per block, so a compile error points at the node.
- For the scheduler, the order search and incremental recompute, a region is one node; its hash
  joins the hashes of its nodes.
- Check: on the 181 SD graphs and the example graphs, fused and unfused runs give the same values
  (within 1 LSB of the register format).

## 7. Open questions

Q1. Can we get the SD standard library sources (`.sbs` files of an SD install) and `sbsrender`
(Substance Automation Toolkit) on a licensed machine? Sources make the rare library kinds
importable as graphs (D3); `sbsrender` makes one-node reference renders that settle the unknown
encodings and calibrate the noises. Both need an Adobe licence and a check of its terms.

Q2. Is a look-only match accepted for noises and scatters? Nothing else is possible without SD's
random generator.

Q3. What is the import for: editor graphs that artists keep editing in daEditorX, or only a bake to
the game textures? The first needs the editor command (D1) and a readable layout; the second needs
only the `texgraph` batch mode.

Q4. The 11 missing resource files and `gradient_dynamic_u_variation.sbs` must come from the art
team.

Q5. Encodings not proven from the data: the blend mode codes, format and tiling codes, gradient map
interpolation codes, the flood fill output layout, how inputs of another size are resampled,
filter borders with tiling off, and the seed inheritance formula. Q1 settles them; else an artist
checks each in the SD UI.

Q6. Are the numbers of the fixed layout right: 16 input slots, the size of the parameter array,
and the code limit of a region? Check them on the widest nodes (brick_tile_generator has 16 brick
inputs; the microdetail_index pixel processor reads 16 inputs) and on the regions of the 181 graphs
before phase 13.

Q7. Is a gather fast enough for the largest splatters (particle_scatter: 2000 instances in many
uses, sizes up to 2), or does the particle path stay as a second back-end of the same instance
generator? That back-end draws the instances in a fixed order, so both back-ends give one image.

Q8. Is the blend stand-in (a chain of existing nodes) good enough on the review page until phase
7? If not, the blend node moves into phase 1: 179 graphs and 3981 nodes use it.

Q9. Which compiler runs the HLSL blocks on each driver? daEditorX builds DX11, Vulkan and stub.
The multi-driver compiles HLSL at run time with d3dcompiler on DX11 and with DXC on DX12 (which
the editor does not build); Vulkan has no run-time compile, and daEditorX uses dsc2 today.
Options for Vulkan: DXC to SPIR-V on the worker, with the driver header written from the fixed
layout, so no reflection is needed; or a plain HLSL mode of dsc2 with the three buffers of the
layout moved past b2 (dsc2 reserves b0 to b2: parameters at b3, as today, slot and pass constants
on the next free slots), which keeps one compiler but starts a process per new region.

## 8. Risks

- Semantics come from the SD documentation (parameters only, almost no formulas) and from DiffMat
  (MIT CSAIL and Adobe Research, a PyTorch re-implementation tuned to SD renders). DiffMat is licensed
  for noncommercial research only: learn the semantics from it, copy no code.
- MM code is MIT: a port keeps the copyright notice.
- Flattened graphs plus SD's oversampled generators make large passes: the order search lowers the
  peak of live registers and the retain budget (`TextureRegManager`) limits the retained ones, but
  GPU time at 4096 needs joined passes (phase 10 cost notes) and fusion (phase 13).
- Wrap is correct only when the width and the height are each a power of two (a bit mask per
  axis in the sampling helpers). SD sizes are log2 values and the editor offers only powers of
  two, but a das script can set other sizes (`px(1000)`).
- 26 graphs carry a 2048 default parent size, but their reference PNGs were rendered with a 256
  parent: the validation must use the size the reference used.
- A long fused region compiles slowly and is hard to debug; MM documents low frame rates from very
  complex shaders. The code limit of a region, the materialize flag, the cache by text hash and the
  `#line` markers answer this.
- The fixed layout splits regions around wide nodes: a pass has at most 16 external inputs
  (registers made outside its region), 8 outputs and one parameter array. A gather block with
  inputs from more than 16 registers does not fit at all (Q6).
- The cost of a gather grows with the reach of an instance in cells: in the samples the tile
  sampler scale goes up to 19.54 and its position random up to 10 (Q7). The node bounds the cells
  it visits and writes a report line when it clamps.
- Stand-ins are work that the own node replaces later: keep each to an existing node or a short
  chain.
- Fill areas propagates at most 2 x steps x 512 pixels; large islands in 4096 graphs need more
  steps (phase 5).
- A new seed rule must not change existing graphs (D13).
