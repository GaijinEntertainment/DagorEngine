# Centers of Rotation Skinning

The centers of rotation skinning is in the engine and off by default. This page
is for the project that turns it on and for the render team that owns it: what
is in the engine, how to enable it, what it costs, where it stops and how to
check it.

Le and Hodgins, "Real-time Skeletal Skinning with Optimized Centers of
Rotation", SIGGRAPH 2016. A per-vertex center `c` is precomputed from the rest
pose and the weights. At runtime the vertex rotates by the weighted blend of
the bone rotations `q` around `c`, and `c` itself moves with linear blend
skinning (LBS): `v' = R(q) (v - c) + LBS(c)`. A vertex whose center offset is
zero (one bone, or no triangle shares two of its bones) keeps its LBS
position; its normal takes the blended rotation, which equals the LBS one for
a single bone.

## What Is in the Engine

- **Project switch:** `cor_skinning_support`, an interval in
  `prog/daNetGame/shaders/skinning_inc2.dshl` that each shader build assumes.
  `assume_if_not_assumed` keeps it off, so a project that does nothing gets the
  shaders, the variants and the vertex channels it had before. A project turns
  it on with `cor_skinning_support:i=1` in the `assume_vars` of every shader
  build that compiles the dynamic shaders: the game dumps, the Asset Viewer
  dump and the exporter (tools) dump.
  All three must agree, because cooked vertex data is matched against the shader
  channel list (`ScriptedShaderMaterial::checkChannels`): the exporter dump
  decides whether the builder writes the channel, the game dump whether the
  shaders read it.
- **Shaders:** with the switch on, every daNetGame dynamic shader that expands
  `DYNAMIC_SIMPLE_VS_INPUT` (the character shaders `dynamic_skin`,
  `dynamic_masked_chrome_bump`, `dynamic_hair`, `dynamic_painted_by_mask`
  among them) declares the `color8 tc[8] = extra[60]` channel in its skinned
  variant (`num_bones == four_bones`,
  `prog/daNetGame/shaders/dynamic_simple_inc.dshl`). A dynamic shader with its
  own channel list, such as `dynamic_layered`, does not take part.
  `skinning_inc2.dshl` blends the four bone quaternions (antipodal sign fixed
  against the heaviest bone) and applies the formula for the current and the
  previous frame. A zero scale bone gives no rotation to the blend, and a
  vertex with a weight on it stays on LBS. A part the node collapser hides
  vanishes whole through the collapser bits. No shader gets a new variant:
  the project switch is assumed.
- **Bone rows:** with the switch on, dynrend
  (`prog/gameLibs/render/dynmodelRenderer`), which fills the bone rows of
  daNetGame and of War Thunder, sends a bone as three rows of rotation
  quaternion, position and scale, not as the rows of its 3x4 matrix
  (`calc_bone_tm`). The row count does not change, and the animation pipeline
  keeps its matrices: dynrend decomposes each bone matrix when it fills the
  rows. The daNetGame animchar update builds the rows of a skin once in a
  frame (`cache_animchar_skin_bone_rows`) and keeps them in the scene
  instance; each pass copies them. A node write after that drops the kept
  rows, and the passes decompose again. Every skinned shader of the project
  reads this form. A skinned shader without the channel takes the same path
  with a zero center, and the code that needs a skinning matrix (cloth, eyes,
  eight-bone skins, the non-batched ray tracing prepass) builds it from the
  three rows.
- **War Thunder:** its own dynamic shaders in `skyquake/prog/shaders/source`
  (`dynamic_simple_inc.dshl`, `skinning_inc2.dshl` and the classes that expand
  its `DYNAMIC_SIMPLE_VS_INPUT` with `use_skinning`) take the same project
  switch, channel, bone rows and formula. Both projects share the math in
  `prog/gameLibs/render/shaders/cor_skinning.hlsl`.
- **Builder:** `prog/tools/libTools/shaderResBuilder/corSkinning.cpp` computes the
  centers (similarity sigma 0.1, edge subdivision by weight distance 0.1, up to
  3 levels, sub triangles grouped by quantized weight vector). The builder in
  `shSkinMeshData.cpp` runs it for every skin whose material declares
  `extra[60]` in the exporter dump, on every Level of Detail (LOD), with the
  same quantized 4-bone weights the shader sees, and writes `extra[60]` per
  mesh vertex as four bytes: `c - v` as three signed bytes, times the scale
  `2^((w - 200) / 8)` of the fourth byte (`encode_cor_offset`; the shaders
  read it with `cor_offset_decode`). No option in `application.blk`, in the
  asset properties or in a DAG node selects it: the exporter dump does.
- **Channel ID:** `prog/dagorInclude/shaders/dag_corSkinningChannel.h`.
- **Ray tracing:** the batched bounding volume hierarchy (BVH) skinned
  prepasses of daNetGame and War Thunder read the same channel
  (`prog/gameLibs/bvh/bvh.cpp` channel parser, `bvh_processors.cpp`,
  `prog/daNetGame/shaders/rt/bvh_process_skinned_vertices.dshl`,
  `skyquake/prog/shaders/source/bvh_process_skinned_vertices_batched.dshl`), so
  the bottom-level acceleration structure (BLAS) matches the raster mesh.

## How to Enable in a Project

1. Turn the project switch on in `assume_vars` that every shader build of the
   project includes, the game, Asset Viewer and exporter dumps alike, so that
   they agree: `game_common_override.blk` in Active Matter and Enlisted,
   `common_assumes_wt.blk` in War Thunder:

   ```blk
   cor_skinning_support:i=1
   ```

   The Enlisted Oculus dump (`shaders_spirv_oculus.blk`) does not include
   `game_common_override.blk`, so it takes the line in its own file.

   Three War Thunder game dumps do not include `common_assumes_wt.blk`: the
   PS5 compatibility dump (`shaders_ps5_compatibility.blk`), the iOS
   compatibility dump (`shaders_metal_ios_compatibility_wt.blk`) and the Xbox
   One dump (`shaders_xboxOne.blk`). They take their assumes from
   `common_assumes_compatibility.blk` or `common_assumes.blk`, which War
   Thunder Mobile or VR Tanks share, so each takes the line in its own file.
   The VR Tanks Scarlett dump (`shaders_dx12xs_vrt.blk`) includes
   `shaders_xboxOne.blk`, so it removes the line again with
   `"@delete:cor_skinning_support":i=0` in its `common_assume_vars` override.

2. Recompile the game shaders, the Asset Viewer dump and the exporter dump.
3. Rebuild every skinned dynmodel: `daBuild-dev.exe -target:PC
   -rebuild:dynModel application.blk`. Land the rebuilt assets together with
   the game shaders: a skin cooked without the channel does not match a
   shader that declares it.

Rollout: Active Matter first (fewer outfits), then Enlisted and War Thunder.
A project that does not need it (War Thunder Mobile, for example) does
nothing. Owner: render.

## Cost

- **Vertex data:** +4&nbsp;bytes per vertex on every skinned LOD whose material
  declares the channel.
- **Vertex shader:** per application, one offset decode, one quaternion
  blend, one quaternion to matrix and four vector rotations. The skinned
  `dynamic_simple` vertex shader of Active Matter grows from 413 to 621 DXIL
  operations. The normal pass applies it twice, for the current and the
  previous frame (motion vectors); the depth and shadow passes once.
- **Bone rows on the CPU:** the decomposition takes 12&nbsp;ns for a bone,
  against 6&nbsp;ns for its matrix rows (clang, SSE4.1, one thread). The
  daNetGame animchar update (`animchar_before_render_es`) decomposes a bone
  once in a frame (twice in a frame that has no previous rows of the skin
  mesh: the first frame, a LOD switch) and each pass copies the rows: in the
  benchmark the bone rows of a character (3.1 passes of 69 bones) take 7.8k
  CPU cycles in a frame, against 7.2k for matrix rows and 14.1k when each
  pass decomposes. The kept rows take 96&nbsp;bytes for each bone of a
  skinned animchar. An animchar that another system updates (the damage model
  animchar, for example) and War Thunder decompose in each pass.
- **Build:** well under a second per LOD. The similarity of a sub triangle to a
  vertex depends on the weights alone, so the builder sums the sub triangles of
  one quantized weight vector into one group. Measured on the Active Matter
  trooper LOD0: the 10k-vertex torso skin takes about 0.2&nbsp;s, the whole
  character LOD0 plus LOD1 under 0.5&nbsp;s.
  The builder logs `cor skinning: N of M verts have a non-zero center offset,
  max/avg, weight groups, seconds` per skin.
- **Resource size in Active Matter:** with the switch on, every skinned
  dynmodel carries the channel on every LOD, and the PC resource packs that
  hold them grow by 37&nbsp;MB, from 1.88 to 1.92&nbsp;GB (+2.0%), of about
  6.4&nbsp;GB of resource packs in all. The 413 `am_*` character packs take
  35&nbsp;MB of it (+10% of their 341&nbsp;MB); the rest are weapon, gear and
  loot models and destructible cloth props (`*_destr`). The LODs after LOD1
  add little, because a far LOD has few vertices. The dedicated server packs
  do not change, because the server export skips dynmodels.
- **Measured frame cost:** the MotionMatching benchmark
  (`gamedata/scenes/cor_skinning_bench.blk`, launcher
  `samples/MotionMatching/game/motion_matching_cor_bench.bat`) renders 100
  survivors (14k mesh vertices at LOD0; the grid mixes LOD0 and LOD1) at
  1920x1080 and writes the GPU time of the passes to the log, for the build it
  runs in: an A/B compares a sample build with the switch on against one with
  it off. Each phase gives the profiler `avg` of a pass (GPU columns) and the
  mean CPU animation time of its 6&nbsp;s measure. The table is the median of
  the six phases of a build from three runs, with the run order off, on, on,
  off, off, on, on an RTX 5070 Ti Laptop GPU, DX11:

  | Build | GPU frame | G-buffer dynamics | Shadow cascades | CPU animation |
  |---|---|---|---|---|
  | Switch off (linear blend) | 2.432&nbsp;ms | 0.341&nbsp;ms | 0.587&nbsp;ms | 0.147&nbsp;ms |
  | Switch on (centers of rotation) | 2.409&nbsp;ms | 0.333&nbsp;ms | 0.597&nbsp;ms | 0.141&nbsp;ms |

  The GPU cost of the centers is inside the noise of the runs (about 3%) for
  100 characters. CPU animation does not change. The survivor resource grows
  by 12% (87&nbsp;KB: 4&nbsp;bytes for each of the 21.6k vertices of LOD0 and
  LOD1, more than the mesh vertices because of the UV and normal seams), and
  by 14% (60&nbsp;KB) as the zstd-compressed resource on disk.

## Known Limits

- A bone goes to the shaders as rotation, position and scale, so a bone
  matrix with shear loses the shear, in every skinned shader of a project
  with the switch on. Bones carry no shear.
- The four-byte offset keeps seven bits and a sign for each axis, against
  the largest axis of the offset: the stored center is off by 0.3% of the
  offset length on average and by 0.8% at most (0.16&nbsp;mm at 6&nbsp;cm,
  up to 1.6&nbsp;mm at 23&nbsp;cm). The skinned position moves by that
  times the difference between the blended matrix and the blended rotation,
  which is zero in the bind pose.
- A non-uniform bone scale affects only the LBS part (the center), not the
  rotated offset. Characters do not scale bones non-uniformly.
- Projective wounds rewrite the bone weights of cut vertices at runtime. The
  precomputed center stays, so the cut region is an approximation. Those
  vertices lie inside the wound geometry.
- Adds no volume. Deltoid or pectoral bulge needs helper bones or corrective
  shapes, which is out of scope.
- A skin with more than 4 bones per vertex (`setBonePerVertex` above 4) gets
  zero offsets, so it keeps the LBS position; the builder logs a warning for it.
- No runtime off switch: a project checks the centers before it ships them.
  Turning them off takes the project switch off and an asset rebuild.
- Only the batched BVH skinned prepasses read the channel. The non-batched
  prepass keeps LBS for the ray-traced copy.

## Validation

1. The MotionMatching sample, scene `gamedata/scenes/cor_skinning_test.blk`
   (`samples/MotionMatching/game/motion_matching_cor_skinning.bat`): the
   survivor with the centers, looping one clip under a long-lens camera. The
   ImGui panel "CoR skinning" (F2 to click it) picks the clip (reload, rifle
   sprint, sprint, crouch walk, injured walk, rifle idle; each bends other
   joints) and sets the time speed; F8 pauses and resumes, Shift+F8 switches
   slow motion, F9 takes a screenshot, F10 frees the camera (WASD, mouse, E
   and C, Z and Q). Compare the shots with a sample build that has the switch
   off in `sample_shaders_override.blk`, with its game, Asset Viewer and
   exporter dumps recompiled and its skinned dynmodels rebuilt. Keep the
   shots of each build in their own folder: a shot's file name holds only the
   capture time.
2. Asset Viewer with the project's `application.blk`: open a soldier
   dynmodel, bend the arm with an animation, compare elbow, shoulder and knee
   with a build that has the project switch off.
3. **In game:** the same comparison, plus motion vectors (temporal
   anti-aliasing ghosting on moving characters) and ray-traced shadows or
   reflections on characters.
4. Compare the built resource sizes and the build log of a full dynmodel
   rebuild before and after enabling, for size and time; the vertex count does
   not change, the channel adds 4&nbsp;bytes per vertex.
