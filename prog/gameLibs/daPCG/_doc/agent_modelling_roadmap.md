# daCSG and daSurf: agent modelling roadmap

The goal: an agent gets "create a sport bike" or "a medieval house" and writes a small,
readable, editable model program. Its output ships: a good silhouette, clean shading, a
sane triangle count, a node tree with a skeleton, collision, LODs and UVs. Materials and
textures exist; only the model is missing.

## Premise

- **Where it runs.** Mostly an offline tool. Some assets build at load time with a disk
  cache. In rare cases, in some games or events, art is generated for gameplay, for
  example around the player.
- **Agent feedback.** Render and look: the agent judges images, not only numbers.
- **Texturing.** Mixed: tiling and trim sheets for environment, unique UVs and baked maps
  for hero assets.
- **Asset classes.** Hard-surface vehicles, architecture, props. Organic and terrain-like
  shapes go into a separate library.
- **References.** One photo, one concept image, or several photos from real scenes at any
  angle. Clean front, side and top views are rare.

## Three principles

1. **The program is the asset.** Nodes, LODs, collision, pivots and UV seams come from the
   program's structure, which knows what a bolt and a wheel are. Mesh post-processing that
   throws the structure away is the last resort.
2. **Move judgement out of the model.** Every decision a weaker model gets wrong
   (proportions, bevel sizes, segment counts, where detail goes) belongs in a kit with
   good defaults, a check with a fix hint, or a tool that tunes numbers.
3. **Show the agent what an artist looks at.** One command gives the views, the wireframe,
   the part names and the budget. The agent fixes what it sees, not what it guesses.

## 1. The first gap: nodes, skeleton, collision

A motorbike is not a shippable resource without its node tree. The skeleton exporter loads
the dag and reduces its nodes (`prog/tools/sceneTools/assetExp/exporters/exp_skeleton.cpp`).
Collision comes from node properties: `collidable:b=yes` with `collision:t="mesh"`, or box,
convex, capsule (`_docs/source/assets/collisions/static_object_collision.md`). Today the
console writes one node with one merged mesh.

**A scene producer, not a change to daCSG or daSurf.** The executor by its nature returns
one Solid, which is one mesh, and that is right. A node is a layer over meshes: a mesh, a
matrix, a node script, materials and collision. A new scene producer builds that tree over
meshes from any producer and writes the dag. The dag writer the console uses already takes
a node tree: `Node` in `prog/tools/sharedInclude/libTools/dagFileRW/dagFileNode.h` has
`child`, `parent`, `tm`, `wtm`, `name` and `script`.

- **A node holds a mesh:** in this step a csg Solid (a closed daSurf shell through `solid()`
  too), built alone, so nodes can overlap at joints. Later: open Sheet and Shell parts
  (open_surfaces_proposal.md), a mesh file, the organic library and threaded builds.
- **Pivot and parent.** The node matrix places the pivot, and the producer moves the mesh
  into the pivot's frame. A wheel turns about its axle and a fork steers about its head
  tube with no other data.
- **Animated rigid nodes.** Hard-surface vehicles need no rigging: a node is plain, and a
  part that moves is an animated rigid node with its mesh; a dynModel keeps an
  `animated_node` or `separate_mesh_node` mesh apart, a rendInst makes one mesh. The
  reduceNodes rules and the bike tree are in scene_nodes_proposal.md section 2. Node names
  follow the game's conventions, so the vehicle kit names them, not the model author.
- **Collision per node.** By default a node's visual mesh is also its collision. A node
  can choose a lower LOD of its mesh, a box, a convex hull, a capsule, or a separate
  collision mesh from any producer. The producer writes a separate collision dag:
  `ref_skeleton` binds its nodes to the visual ones, and `renderable:b=no` makes a node
  collision only.
- **LOD sets.** The producer evaluates each node's mesh once per LOD, at that LOD's
  quality, and writes one dag per LOD (`.lod00.dag`, `.lod01.dag`, ...) with the same nodes
  and matrices; a model may take the LOD and leave out small details (section 8).
- **Edits across nodes are plain das.** A groove through the frame and the tank is a
  helper that subtracts the tool from both solids before the nodes take them.
- **Pose check.** The look command renders the model with nodes moved (fork steered 30
  degrees, wheels turned, suspension compressed), so the agent sees a wrong pivot at once.

## 2. What makes a model look good

Shippable hard-surface art is mostly "mid-poly": real chamfers on the edges that catch
light, face-weighted normals so big faces stay flat, trim sheets for material breakup, and
detail only where the eye goes. Four gaps decide most of the look:

- **Razor edges.** Every boolean edge is sharp. A 5 to 20 mm chamfer with weighted normals
  is the largest single visual gain. A subdivision leaf with creases (section 6) is the
  second route, for soft bodies.
- **Proportions.** The photo overlay measures them; kits with real dimensions fix them at
  the source.
- **Detail density.** Panel lines, bolts, seams and trims where they show. Kits place
  them, LOD tiers remove them.
- **Material breakup.** Named materials and trim-sheet bands, not one colour per part.

## 3. Tools for a strong model (Opus)

Ranked by expected gain in result quality. Items 1 to 4 also help a weaker model the most.

1. **One look command.** Build, then render the agent's own model: first from each
   reference's camera, then from the sides no reference shows. A shaded pass with real
   material looks, a wireframe overlay to judge triangle density, a table per node and
   named part (triangles, bounds, dimensions). One call per iteration instead of five.
2. **Photo references at any angle.** Each photo gets its own solved camera (pose and
   focal length) from 4 to 8 named points: the agent writes the pixel position of model
   anchors such as the front wheel centre or the seat top, and the tool solves by DLT and a
   least-squares refine, then prints the residual in pixels. Not from the silhouette: the
   camera and the shape trade off against each other. The agent traces a mask polygon of
   the object and one for things in front of it; the outline gap and an edge distance are
   measured inside the mask, and the gaps of all photos add up to one score. Scale comes
   from the type card of real dimensions, not from the photo. A concept image has no true
   perspective, so it gets no solve: the agent takes its proportions and style. A photo of
   another variant shows only part of the truth, so its gap counts less. Today's overlay is
   orthographic and expects a side view with visible wheel circles.
3. **Part focus.** Highlight or isolate a named part in the render; ask for a dimension
   by part name ("seat height above ground", "gap between tank and frame"). The agent asks
   the model, not its own arithmetic.
4. **Lint with fix hints.** Floating parts, parts that only touch, coplanar contact that
   cleanup must repair, hidden interior faces that waste triangles, walls thinner than a
   texel, details too small for LOD0, unnamed parts, over budget. Each message names the
   part and the likely fix.
5. **Fast rebuild.** An on-disk cache per graph subtree, keyed by the node hash that
   exists already. Editing the tank rebuilds the tank and the last booleans. The same
   cache serves the load-time path.
6. **Before and after.** The previous build's views side by side, changed pixels marked.
7. **Game-ready output.** The node tree from the scene producer (section 1), named
   materials, a LOD chain, UVs. Without these no model ships, however good it looks.
8. **Engine look.** A final check in the engine's own shaders with the real textures, for
   example an asset viewer screenshot. The software renderer judges shape; only the engine
   judges the finished look.

**Sides without a reference** are judged against what the class must be: symmetry, the
real dimensions of a per-class sheet, the kit's proportions and the model's own knowledge
of the object. Their renders are still needed: that is where a weak model's errors hide.

## 4. Enablers for a weaker model (Sonnet)

A weaker model fails at 3D coordinate arithmetic, at taste (proportions, bevel sizes,
detail placement) and at long edit loops. Each enabler takes one of these away.

1. **Kits in das** with parameters and good defaults. A wheel, a window, a roof are right
   before the model touches them.
2. **Anchors and relative placement.** Parts attach to named sockets, stack on tops, sit
   between two parts, follow a curve. No coordinate arithmetic.
3. **Templates per asset class.** A bike or house template is a parameter block plus
   stages; the model fills parameters first, then edits stages.
4. **Lint with fix hints.** The model applies the hint instead of finding the cause.
5. **Parameter fit.** `-fit` scores a model against a reference mesh today; extend it to
   tune named parameters against the reference images. The model picks the structure,
   the tool picks the numbers.
6. **Cookbook and style sheets.** Complete small models per class, and style sheets
   ("medieval: stone plinth, timber frame, 45 to 60 degree roof, small deep windows").
7. **Opus as the critic.** The weaker model builds and iterates; one Opus pass reviews the
   renders and lists fixes.
8. **An eval suite.** Fixed prompts (sport bike, medieval house, barrel, crate, bridge),
   scored on validity, budget and a judged look score, run before and after each feature.

Today:

```das
var wall = box(6.0, 3.0, 0.4).at(0.0, 1.5, 0.0)
for (i in range(3)) {
    let x = -2.0 + float(i) * 2.0
    wall -= box(0.8, 1.2, 0.6).at(x, 1.6, 0.0)
    wall |= box(1.0, 0.08, 0.5).at(x, 0.96, 0.0)
}
```

With a kit and anchors:

```das
require kit/arch
var wall = arch::wall(len = 6.0, h = 3.0, style = timber)
wall = wall.openings(arch::window(0.8, 1.2, sill = 0.9), count = 3, spacing = 2.0)
let roof = arch::roof_gable(pitch = 50.0).on(wall, top)
```

## 5. Libraries

Already in the engine, reuse:

| Library | Use |
|---|---|
| meshoptimizer (`prog/3rdPartyLibs/meshoptimizer`) | Simplification for far LODs only; vertex cache and overdraw order of every output mesh. The tree has 0.18, used for vertex cache and the index codec; attribute-aware simplification needs an update. |
| V-HACD (`prog/3rdPartyLibs/math/vhacd`) | Convex pieces for collision when the game asks for them. Most nodes need none: the visual mesh or its convex hull is the collision. |
| Clipper 4 (`prog/3rdPartyLibs/math/clipper-4.x`) | 2D offset and boolean of profiles: panel insets, window frames, mouldings, bevel profiles, holes in extrude profiles. Clipper2 (Boost license) is its newer form if the old API is a burden. |
| binPack2D (`prog/3rdPartyLibs/binPack2D`) | Chart packing. Primitives already carry their own 0..1 charts, so hero UVs need a packer over those charts and a second UV set for lightmaps, not an unwrap. Every chart spans 0..1 whatever the part's size, so the packer first scales each chart by its world area to one texel density; daSurf's charts follow the surface parameters, not arc length, so they stretch where the knots crowd. The engine's unwrapMapping is the old scene builder's lightmap mapper, not for assets. |
| freetype (`prog/3rdPartyLibs/freetype`) | Glyph outlines for a `text()` primitive. |
| impostorBaker (`prog/tools/sceneTools/impostorBaker`) | The last LOD of rendInsts. |

Candidates to add:

| Library | License | Why |
|---|---|---|
| xatlas | MIT | Unwrap and atlas for imported meshes and the organic library only; own primitives need a packer, not an unwrap. |
| CoACD | MIT | Better convex decomposition than V-HACD for concave props, if V-HACD results are poor. |
| FastNoise2 | MIT | Noise for the organic library; or a small own set, since few kinds are needed. |

Write ourselves:

- **The scene producer** (section 1). It changes nothing in daCSG or daSurf.
- **Subdivision with creases.** Catmull-Clark with semi-sharp creases is a few hundred
  lines; OpenSubdiv is far larger than this needs.
- **Bevel and chamfer.** It must know daCSG's topology and the sag rule.
- **A baker** for normal and AO maps from the same program at high quality onto the low
  LOD. daCSG's BVH already does the ray queries.
- **The organic mesher** (section 9).

## 6. Additions to daCSG and daSurf

| Addition | DSL sketch | Why |
|---|---|---|
| Segment counts follow quality | a given `seg` is the count at quality 1 | Today a given `seg` is used as given, and real models pin it: the sport bike sets it 50 times and gives 66526 triangles at quality 1, 0.5 and 0.25 alike. With `seg` scaled by quality (clamped 3..256), LODs come from the program. |
| Composition | `wheels() + frame() + bodywork()` | Parts that only sit together join with no boolean (the Concat node exists). The overlap-by-1-mm rules go and the build gets faster; a `-contacts` report lists near pairs by name. |
| Paint in csg | `x.paint(poly, colour, plane)` | As in surf, instead of a scaled copy intersected with a mask. |
| Bounds | `bounds(a)` | The graph's conservative bbox, eager, so placement is arithmetic, not a guess. |
| Text | `text(s, size, h)` | Glyph outlines from freetype, extruded with holes: signs, plates, labels. |
| Bevel on primitives | `box(...).bevel(0.01)`, `extrude(poly, h, bevel = 0.005)`, `convex(pts).round(0.02)` | A rounded hull is the hull of small spheres, exact and cheap; covers most body panels. |
| Edge chamfer after booleans | `(a - b).chamfer(0.005, angle = 30.0)` | Edges a boolean makes, which are most razor edges. A strip per crease edge with corner patches, the subtree used as a leaf after it (about 900 lines); start with convex edges. |
| Weighted normals | `.normals(weighted)`, default for chamfered parts | The mid-poly look with no bake. |
| Subdivision leaf (meshsmooth) | `subd(box(0.5, 0.3, 0.4), crease = 30.0)`, the cage a box, extrude or loft at a low segment count | Catmull-Clark with semi-sharp creases; the level comes from the sag rule, so LODs and collision are the same cage at fewer levels. |
| Anchors | `x.anchor("front_axle", p)`, `y.on(x, "front_axle")`, `y.on(x, top)` | Relative placement, the main enabler for a weaker model. |
| Named materials | `mat("painted_metal")` | A project material table instead of integer ids and five names fixed in the console. |
| Trim and density | `uv_trim("metal_trim", band = 3)`, `uv_density(512.0)` | Environment texturing with trim sheets at a fixed texel density. |
| Deformers | `x.deform_box(c0, ..., c7)`, `x.lattice(3, 3, 3).move(i, j, k, d)`, `bend`, `twist`, `taper`, `along(curve)`, `jitter(seed, amp)` | Wedge seats, tapered tanks, leaning walls, sagging ridges, curved rails. See below. |
| Patterns | `along(curve, n)`, `grid(nx, nz, step)`, `scatter(region, n, seed)` | Bolts along a seam, roof tiles, fence posts, stones in a wall. |
| 2D profile kit | `offset(poly, d)`, `round_corners(poly, r)`, `poly_union` | Window frames, mouldings, inset panels, through Clipper. |
| daSurf fillet | `fillet(sheetA, sheetB, r)` | The missing rolling-ball fillet; bodywork joints look modelled, not glued. |
| daSurf grooves | `groove(sheet, iso_u(s, 0.4), w, d)` | Panel lines that follow the surface. |

**Where meshsmooth goes.** On a leaf, before the booleans: the cage is a box, an extrude or
a loft at a low segment count, so it is quads and stays readable. Not on the boolean
result: that is triangles with slivers and no edge loops, and smoothing it wobbles. A
subdivision surface shrinks inside its cage; creases on the cage's hard edges keep those
edges, and the level comes from the sag rule like a segment count. The cage itself is the
collision and the farthest LOD. daSurf is the home: on a regular cage the limit surface is
the same bicubic B-spline that daSurf patches make, and subdivision adds closed bodies of
any topology (a tank, a helmet, a seat) from one cage.

**Deformers.** A deform box maps a part's bbox to eight target corners (a 2x2x2 lattice):
a box-like part becomes a wedge seat, a tapered tank or a leaning wall, and eight corners
are concrete numbers that an agent writes well. A finer lattice (3x3x3, 4x4x4) gives
bulges and sags by moving a few points; bend, twist, taper and a path deform cover the
usual sweeps; a seeded jitter of the lattice gives the hand-made irregularity of old
buildings and props. Like the scene producer, deformers sit over meshes, not in the
kernel: the subtree is evaluated, refined until the deformed surface stays within the sag
(a box face of two triangles cannot bend), mapped, given normals from the map's Jacobian,
checked for folds (the Jacobian must stay positive), and used as a leaf. UVs ride along,
so a deformed part keeps its charts; LODs and collision follow from the same program. On
daSurf the control net is mapped instead.

## 7. Standard primitives and kits

Native code only where a geometry algorithm is needed. Everything that is composition goes
into das kits, so the agent can read a kit's source and copy or change it.

- **Native, new:** rounded hull, superellipsoid and superquadric (body shapes), helix and
  thread sweeps, frustum with n sides, bevelled extrude.
- **Vehicle kit:** wheel, tyre with tread, rim with spokes, disc brake, fork, swingarm,
  exhaust with bends, seat, lamp, mirror, chain and sprocket, panel with bolts.
- **Architecture kit:** wall with openings, window and door with frames, arch, column
  orders, stairs, gable, hip and mansard roofs, roof tiles, timber frame, battlements,
  moulding profiles.
- **Props kit:** crate, barrel, chest, chair, table, shelf, lamp, bottle and jar by
  revolve, rope and chain, sign, fence.

For architecture, a repeated kit part stays an instance: the output is a composit of
rendInsts (`composit.blk`) where a window appears twelve times, not one merged mesh with
twelve copies. That saves memory and keeps per-part LODs.

**Generators: grammars over kits.** For buildings and trees the strongest tool is a
grammar, not a bigger kit. A building grammar in the style of CGA shape grammars splits a
lot into a mass, the mass into floors, a facade into bays and tiles, and places kit parts
(windows, doors, cornices, roofs) by rules. Tree generators do the same for branching:
L-systems, space colonization, or a SpeedTree-like stack of branch levels with leaf
cards. The design rule is the "Disney principle": any sentence that the grammar accepts is
a reasonable building or tree. The rules hold the architecture (floor heights, bay rhythm,
roof pitch, branch angles and taper), so an agent writes a short sentence of rules and
parameters, a seed gives variants, and even a weaker model cannot produce an implausible
result. A style is a rule set: a medieval house grammar and a modern block grammar share
one kit. The output is scene nodes with instanced kit parts, so LODs, collision and
composit instancing come with it. References to study before the design:

- P. Mueller, P. Wonka et al., "Procedural Modeling of Buildings", SIGGRAPH 2006:
  http://peterwonka.net/Publications/pdfs/2006.SG.Mueller.ProceduralModelingOfBuildings.final.pdf
- Procedural city generators, for example https://proceduralworldlab.com/procedural-city-generator/
- The grammar of Unreal Engine's PCG framework.
- L-systems, space colonization and SpeedTree for trees.

## 8. LOD pipeline

program -> LOD0 (full quality, all details) -> LOD1..n (lower quality, fewer details) ->
far LOD (simplify) -> last (impostor) -> `.lodNN.dag` and the asset blk.

**Prerequisite.** Evaluating again at lower quality works only when segment counts follow
quality (section 6); the csg-fit skill's rule "`seg = 12` on small parts" goes with it.

| Lever | How | Best for |
|---|---|---|
| Evaluate again at lower quality | The same program; quality and segment counts drop per LOD, and a subdivision leaf drops a level. Clean topology, no decimation artefacts. | Round parts, bodywork |
| Detail tiers | The model function takes the LOD and leaves out small parts; the scene producer can also drop whole nodes below the LOD's screen error. A subtracted detail that is left out closes cleanly, since the boolean runs again. | Bolts, panel lines, window bars |
| Simplify | meshoptimizer on the result, with a silhouette check. | The last mesh LODs |
| Impostor | The engine's impostor baker. | rendInsts far away |

- **Error control.** The asset's LOD ranges give each LOD a switch distance. The tool
  measures each LOD's geometric error against LOD0 with the BVH and picks the quality that
  keeps the error below about one pixel at the switch distance. A silhouette diff at that
  distance is the visible check.
- **Collision and shadow.** Per node as in section 1, or V-HACD pieces (section 5); for a
  subdivision leaf the lower LOD is its cage. A shadow proxy from a far LOD.

The three run paths:

| Path | What runs |
|---|---|
| Offline (most assets) | Everything: all LODs, unwrap and bake for hero assets, collision, the composit for kits. Writes `.lodNN.dag` and the asset blk. |
| Load time with a disk cache | The engine has no runtime path from a mesh to a renderable: rendInst takes a built resource, and no runtime code includes the shader mesh builder. So the cache is a build step through the normal exporter, without baking, keyed by a hash of the program, its required files, the library version and the parameters; the game loads a normal resource. |
| Gameplay-driven (rare) | Parameter changes of prebuilt kits; composition and few booleans, on a worker, on the manifold kernel, with a time budget. Bit-identical results on any thread count, which daCSG has, make it safe for network sync. It needs a small runtime vertex-stream builder for a fixed shader set (about 500 lines), the one runtime piece of this roadmap. |

## 9. Organic library

A separate library on signed distance fields, with daCSG as its consumer, as daSurf is.

- **Scope:** rocks, cliffs, caves, roots, trunks, organic props. Terrain height fields stay
  in the engine's landscape tools.
- **Ops:** spheres, capsules, tubes along curves with varying radius, smooth union and
  subtract, seeded noise displacement and domain warp.
- **Meshing:** dual contouring on an adaptive octree, so sharp rock facets stay sharp, at
  the model's sag. The result is a closed external leaf, so a doorway can be cut into a
  cliff with an ordinary subtract.
- **UVs:** real UVs, not triplanar shading, which costs three texture fetches per layer.
  A smooth union or any field surface has no source face to inherit a chart from, unlike a
  csg boolean. An automatic unwrap (xatlas) cuts charts along creases and hidden sides; the rock takes
  a normal map baked from its dense surface onto the low LOD, plus a tiling detail
  texture. A box projection baked into UVs (one fetch, `uv_box`) is enough for small props
  whose seams do not show. Vertex colour masks by slope and height feed the engine's
  layered shaders.

## 10. Phased plan

Ordered by quality gain per effort. Sizes are rough line estimates.

**Phase 1: ship and look.** An Opus model that loads as a resource with skeleton and
collision, and looks good.
- Segment counts follow quality: about 50.
- Composition and the contacts report: about 270.
- Scene producer: node tree over any meshes, pivots, collision, LOD sets, dag writer, pose
  check: about 1000.
- Look command, photo references at any angle, part focus, before and after: about 1200.
- Bevel on primitives, rounded hull, weighted normals: about 1200.
- Subdivision leaf with creases, in daSurf: about 800.
- Named materials in the dag: about 200.
- LOD quality per level, far-LOD simplify: about 400.

**Phase 2: a weaker model.** The same result from Sonnet.
- Anchors and relative placement: about 400.
- Vehicle, architecture and props kits in das: about 4000 das lines.
- Building grammar over the architecture kit: about 1500 das lines.
- Lint with fix hints: about 600.
- Cookbook, style sheets, eval suite: docs and prompts.
- Parameter fit against the reference images: about 500.
- Paint in csg, bounds, text: about 500.
- Deform box, lattice, bend, twist, taper, path deform, jitter: about 650.

**Phase 3: production.** Hero and environment pipelines complete.
- Chart packer and a second UV set, normal and AO bake: about 1500.
- Trim-sheet mapping, texel density: about 400.
- Composit output for kits: about 300.
- Subtree disk cache, load-time path: about 500.

**Phase 4: reach.** New shape classes.
- Organic library: about 4000.
- Tree generator (branch levels or L-system, leaf cards, LODs): about 1500.
- daSurf rolling-ball fillet, grooves: about 1500.
- Patterns: about 300.
- Edge chamfer after booleans: research first.

## 11. What not to do

- **Decimation as the main LOD path.** It is for the far LODs only (section 8).
- **Meshsmooth of the boolean result.** Subdivide leaves, before the booleans (section 6).
- **Hand-written cages of hundreds of points.** They are not readable text; a cage comes
  from primitives.
- **Large generic geometry stacks** such as OpenVDB or CGAL: size and license cost far
  above what the organic library needs.
- **Style knowledge in native code.** It belongs in das kits (section 7).
- **One merged mesh for architecture.** Repeated parts stay instances.
- **Hierarchy inside daCSG.** Nodes belong to the scene producer (section 1).
- **Text-to-mesh models.** Their output is not small, readable or editable text.

## 12. Open questions

- Which engine path gives the final look check with real materials: an asset viewer
  screenshot mode, or a new tool?
- Do hero assets need a baked normal map, or is mid-poly with weighted normals enough for
  the target games?
- Which triangle and draw-call budgets per asset class should the lint enforce?
- Is the engine's auto unwrap good enough for hero assets, or is xatlas needed?
- Which node names and node properties does each game's vehicle template expect?

## 13. The independent view

A Fable agent answered the same questions alone, from the code and the models, and then
took in the same product owner inputs. It agrees with the sections above, which took
several of its points, and adds: no GUI editor; an eval on fixed prompts for each phase;
numbers before pixels for a weaker model (measures, contacts, type cards the lint checks).

**Where they still differ:**
- **Subdivision.** It makes subdivision its own module with box-modelling verbs (extrude a
  face, inset, loop cut, crease) and puts it late, after the kits, since a cage is the
  hardest thing for a model to write. This roadmap builds cages from primitives (box,
  extrude, loft) and puts the step in phase 1 in daSurf. Both are defensible; the eval
  should decide how well agents write cages.
- **Bevel.** It puts a bevel of every crease edge after evaluation before the other tools.
  This roadmap starts with bevels on primitives; the general chamfer is the bigger gain if
  its corner patches hold up, so it is the first item to prototype after them.
- **Fitting numbers.** It rejects any automatic fit, since nobody edits the output. This
  roadmap tunes only named parameters, which keeps the program editable.
