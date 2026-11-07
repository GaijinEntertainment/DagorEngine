# Open surfaces in the model output: design proposal

Status: proposed 2026-10-25; the five questions of section 8 wait for the product owner.

Base: dacsg at c64e490b0748. Checked: daSurf core includes only dagorInclude, dag_vector, EASTL
and ska_hash_map; `tessellate` gives open meshes (test_blend.cpp:50-56); only `solid` refuses an
open shell (tessellate.cpp:767); the surf module makes Solids in `solid()`, `thicken()` only; the
console takes `model() : Solid` only (main.cpp:309-366). Gates at change 813107: daSurf 7988 of
7989 lines, dsl.md 4796 of 4800 bytes, surf.md 2999 of 3000, the pair 7795 of 7800.

## 1. Goal and non-goals

Goal. A model returns a set of parts. Solid parts go through daCSG as today. Surface parts
(an open daSurf Sheet or Shell, a leaf card, a quad) go to the output mesh with no boolean.
The host joins all parts in one .obj and one .dag, each face with its material, colour, uv,
normal and smoothing group. Instances of one surface under a similarity share one
tessellation, so the 2000 leaf cards of a tree cost one card. A `model() : Solid` model
writes the same bytes as before.

Non-goals. A boolean between a surface and a part (the clip of section 6 is later). An open
mesh as a daCSG input: `ILeafSource` and the daCSG output stay closed solids, and daCSG gets
no change. Validation of a surface part as a solid. Instancing in the files: .obj and .dag
hold a copy per instance. Node trees, pivots, collision: the scene producer of
agent_modelling_roadmap.md 1, which later takes one Model per node. Uv control on sheets (the
daSurf trim, deferred.md). A host with no daslang.

## 2. The DSL: how a model returns parts

A. A `Model` value in surf.das: `struct Model { solid : Solid; surfaces : int }`. `|` of a
   Sheet or Shell with a Solid, Sheet, Shell or Model gives a Model: solids union in csg
   (`a.solid | b`, today's call), surfaces join. `[export] def model() : Model`.
B. A `Part` handle for all output: `part(solid)`, `surface(sheet)`, joined by `+`, never a
   boolean. Overlapping solids then keep their inner faces, or `+` unions them and is `|`
   under another name; every call site converts.
C. A second export, `[export] def surfaces() : array<Sheet>`, next to `model() : Solid`. No
   csg change, but the two share a skeleton (branch points) through globals or compute it
   twice, a sub-assembly (a window with its pane) is not one value, the clip has no place.

Recommended: A, the owner's rule as algebra on one type: `|` is a union for solids and a join
for surfaces; the later clip is `&` (keep inside) and `-` (keep outside) by a solid, the solid
part taking today's boolean. A Model is two ints: no new state; `model() : Solid` stays.

| call | notes |
|---|---|
| `a \| b` `a \|= b` | a Sheet or Shell with a Solid, Sheet, Shell or Model, or a Model with any of these: a `Model`; solids union, surfaces join (no boolean, no stitch) |
| `var m : Model` | the empty model: `empty()` and no surface |
| `card(w, h, fold = 0.0)` | open Sheet in xy: x -w/2 to w/2, y 0 to h (the stem at the origin), normal +z, uv (0, 0) bottom left to (1, 1) top right; `fold` (above -90, below 90 degrees) turns each half about y toward +z |
| `quad(sx, sz)` | open Sheet in xz, centred, normal +y, u along +x, v along -z |
| `solid(x)` | as today: makes a csg Solid of a closed shell x, for the booleans; x joined without `solid()` stays a surface part |

Instancing needs no call: put the transforms last. In `leaf.rot_x(-50.0).rot_y(a).at(p)` each
instance under a rigid map, a mirror or a uniform scale shares the tessellation of `leaf`. A
paint, `rgb` or `mat` after a transform makes a new content node: colour, then place.

```das
[export] def model() : Model {
    let leaf = card(0.08, 0.14, fold = 20.0).rgb(0.25, 0.5, 0.12).mat(5)
    var tree : Model
    tree |= cone(0.12, 0.03, 3.0).at(0.0, 1.5, 0.0).rgb(0.35, 0.25, 0.15)
    for (i in range(1500)) {
        let t = float(i) / 1500.0
        tree |= leaf.rot_x(-50.0).at(0.0, 0.0, 0.7 - 0.6 * t).rot_y(137.5 * float(i)).at(0.0, 1.2 + 1.7 * t, 0.0)
    }
    return tree
}
```

surf.das gets the struct, a private `as_model` per type, the overloads, and `card` and `quad`
as degree-1 patches (3 x 2 and 2 x 2 nets). The module binds `surf_part(x)` (a list leaf with
its call line) and `surf_join(a, b)` (a list node; 0 is the empty list). `card` tests
`w > 0.0 && h > 0.0` and `abs(fold) < 90.0` (a NaN fails each) before it makes a point or
calls `cos_sin`, and fails at once at the model line through `surf_error`, as a zero `rot`
axis does.

## 3. The host side

Where: the surf module (`gameLibs/daSurf/das.lib`). It keeps the surface graph and the part list
in the model's `dacsg::ModelContext`, it is already the only code that sees both libraries, and it
already turns a `SurfMesh` into a daCSG `Mesh` for the leaf (that becomes one shared function,
a refactor commit first). In the console the code is lost to dacsgView (daCSG 15.2); a third lib
adds a jam lib for about 150 lines with one client, so it waits for a host with no daslang.
daSurf core gets one public accessor and names no other library; daCSG gets nothing.

```
// graph_api.h: the first node under h that is not a Transform, and the map of the transforms
// above it; false past 2^450 or on a transform's build error
bool Graph::content(Handle h, Handle &inner, Affine &tm, eastl::string &err) const;
// das/surfModule_api.h (new): the surface parts of a Model, placed in list order into out (overwritten); an empty solidBox: no solid part
bool surface_parts(dacsg::ModelContext &ctx, int surfaces, const BBox3 &solidBox, float quality, int threads,
                   dacsg::Mesh &out, SurfacePartsStats &stats, eastl::string &err);
```

The host calls `surface_parts` on its own thread after `model()` returns:
1. Flatten the join tree left to right, explicit stack. Each join node stores its part count,
   a saturating sum, when it is made (a walk of the join DAG is exponential: `m = m | m`
   doubles); past 2^24 parts or the uint32 index range is an error at the join line.
2. Group. `content()` gives each part its inner node and map. A similarity (as daCSG 3.3:
   orthogonal to 1e-6 relative, s the cube root of |det|) keys it by (inner, float s); any
   other map tessellates the part alone through its chain, which daSurf composes and flips
   (spec 10). The key map lives for the call.
3. Chord. The box: the solid result's and each part's `Graph::bounds` (of the inner node,
   once) mapped. sag = 1e-3 of its diagonal / q^2, q the model's `set_quality` times
   `-quality` (daCSG spec 4 over the whole model); tolerance by daCSG's auto rule (5.1) over
   it. daCSG keeps its rule over its own root, so the solid part tessellates as before.
4. Tessellate each key once at world `s * I`, in parallel on the console's pool
   (`util/dag_parallelFor.h`, `-threads`): daSurf spec 8 lets tessellations of one graph run
   at once, and no node is added meanwhile. Each key fills its own slot; the first failed key
   in list order gives the error, whatever the thread count.
5. Place. Sizes by prefix sum, then each part writes its own range, no allocation in the
   parallel body: position R p + t in double, rounded to float; normal R n; a mirror
   (det < 0) swaps two corners of each face with their channels. List order: the bytes do
   not depend on the thread count.
6. Face data as the leaf has it: CH_UV (the sheet's u, v; a cap's region frame), CH_COLOR,
   mat and attrSet (SheetInfo), CH_NORMAL, smgr (faceSmgr); prim 0 (no csg node), srcFace the
   face in its tessellation. Stats: parts, tessellations, triangles, time, and the mat ids of
   open parts (a part whose tessellation is closed, a capped shell, is not open).

The console: `run_model` takes a `Solid` or a `Model` (two int32 by cmres); `-check` prints
the part count. It executes the solid part unless that is `empty()` and surfaces exist (the
empty-result error stays), calls `surface_parts` with the result's box, and appends that Mesh:
dagFile.cpp's `append_mesh` gets a mode that keeps CH_NORMAL (a channel that one side lacks
takes its default). `-validate`, `-fit` and `-compare` take the solid result alone and say so.
One stats line: parts, tessellations, triangles, ms.

Files. .obj: no io change: vt from CH_UV, vn from CH_NORMAL, `s` from the lowest smgr bit, a
material per colour. .dag: one node named after the model, as today; one slot per mat id; a
slot whose id an open part uses gets MATF_2SIDED (DagExporter writes DAG_MF_2SIDED); no back
faces. `to_dag_mesh` keeps the given normals and colours smooth regions anew; no surface
vertex is shared with another part, so no region crosses parts. dagrender lights a back face
by its front normal: for a two-sided slot it turns the normal to the eye.

Errors. A list leaf keeps the line of its `|`: `surface part (tree.das:31): Patch#12
(tree.das:20): the sheet has no area`, then daSurf's text with the node's own line. A build
error (a NaN size, a singular transform) rises here, as at `solid`.

## 4. Validation of an open surface part

Checked:
- At build (daSurf 3, 8): each number finite by `check_finite` before any compare or product;
  lengths, scales and transform columns within 2^450; a singular transform.
- In `card` (section 2) and `quad` (`sx > 0.0 && sz > 0.0`): range tests before a point is made.
- In `tessellate` (daSurf 5): no degenerate triangle (an apex is a fan, a collapsed side one
  triangle, a singular cell splits on its other diagonal); a sheet with no area, and a face of
  no area in floats after the merge, are errors that name the sheet and the cell; the float
  range of the mesh; a non-orientable stitch.
- In placement (new): the map within 2^450 (`content()`); each placed position within FLT_MAX
  in double before it narrows to float, so no float infinity is made; each placed face keeps an
  area over its float corners (|cross| > 0 in double), so a card 1 mm wide at 1e7 m is an error
  at its join line, not a hole; the caps of step 1.
- The joined surface Mesh passes `mesh_contract_breach`.

Not checked: closedness and volume (open by design); self-intersection in a part, between
parts and with solid parts (cards cross each other and the branches by design); coincident
copies; orientation between parts (each keeps its du x dv, or its stitch's).

## 5. Open primitives for a first version

New, two calls and no core code: `card` (leaves; grass as a card and its `rot_y(90.0)`; signs,
ivy) and `quad` (window panes, a floor or flat roof, a decal). Both are degree-1 patches; a
card's middle column lies on the y axis, so the midrib is a C0 grid line, smooth up to a fold
of 15 degrees and a crease above (the 30 degree crease angle). The uv is the patch (u, v): a
card shows the whole texture.

From daSurf already: extrude of an open curve (a wall strip, a fence, a ribbon), revolve of an
open profile (a bowl, a lamp shade; a disc is a revolved radius, its apex a fan), loft of open
sections (a roof, an awning, a leaf with a true outline), sweep (a blade or a vine, `scale` to
taper), patch, coons, blend; shells by stitch and `sym_*`; paint, rgb, mat, transforms. Rocks
stay closed solids. Not in the first version: a uv affine on sheets (atlas tiles, metres on a
roof; with the trim's `uv_arclen`), a card bent along v, a bulk placement call (question 5).

## 6. Later: the clip of a surface by a solid

`clip(S, B, keepInside)`: S a triangle mesh with channels, open or closed; B a csg node,
executed to a closed solid. Output: the pieces of S's faces inside (or outside) B, open, no
face of B. A piece on B's surface counts on the side its normal points to, the old carve
SLICE rule (ON_ORIENT_IN with IN, ON_ORIENT_OUT with OUT), so the two results partition S.
Each output face is a piece of one S face, so the 5.4 resolve gives its uv, colour, normal.

Where: a separate call in a new public header (clip_api.h), not a node kind: the result is
not a solid and cannot be a boolean operand, so every graph node stays a closed solid. B is a
graph node: it gets the optimizer, the instancing and either kernel as any root.

```
bool clip(Graph &g, OpResult b, const Mesh &surface, bool keepInside, const ExecuteParams &p,
          Mesh &out, ExecuteStats *stats = nullptr, eastl::string *err = nullptr);
```

exec/ executes B, then calls an optional kernel `clip` (a new IKernel virtual that refuses by
default; the fallback rule of 5.2 picks the other kernel).
- Manifold: the S half of a boolean. S's edges against B's faces and B's edges against S's
  faces, the symbolic perturbation pushing S along its own normals (Manifold's expandP) for
  the "points to" rule; the winding of S's vertices from B alone, defined as B is closed; S's
  faces assembled, B's dropped; a halfedge of S with no pair is S's border, not an error.
- Exact: S's triangles enter the arrangement as constraints with no winding component; B's
  cells as today; each S sub-triangle takes B's winding by the seeding ray of 7.2 step 9 from
  a dyadic interior point; one coplanar with a B plane group takes the cell its orientation
  sign faces.
- The cleanup (section 8) expects a closed mesh: the output takes only the cluster weld first.

DSL: `Model & Solid` and `Model - Solid` (or a Sheet or Shell on the left): the solid part
takes today's boolean, the surfaces the clip; a clip list node holds the child list and the
csg node, and the surf module clips the placed child mesh, so clipped instances share nothing.
dagCSG: meshCSG gets `slice(a, b, keepInside, r)` over `clip`; SLICE_IN and SLICE_OUT take an
open A again (58451d265bda made them need a closed A). Reference: the carve slice
(58451d265bda^) on the skyquake dags with open nodes; area(in) + area(out) = area(A).

## 7. Doc and spec changes; the plan phase

daSurf:
- `_doc/spec.md`: 1 (a sheet or shell is also an output part, not only a leaf); 2 (in:
  surface parts, card, quad, shared tessellations; later: the clip, in daCSG); 3 (Model is a
  das value like Solid; `Graph::content`); 4 (card, quad, the Model `|`); 5 Validation
  (section 4 here); 7 (Model, the new sheet caps); 8 ("never an open mesh" becomes "never an
  open mesh where a closed one is asked: solid, thicken, a leaf"); 9 (the tests below); new
  10.1 "Surface parts" (section 3 here); 11 (the new limit, the placement time); 12 (one line).
- `_doc/surf.md`: the Model sentence, rows for `|`, card and quad, the rule "transforms last
  share a tessellation, colour before you place". daPCG's `PLAN.md`: the phase below.
  `CLAUDE.md`: daSurf also gives open output parts.
- `graph_api.h`, `das/surfModule_api.h` (new): contract comments, with the code.

daCSG:
- `_doc/spec.md`: 14 (`model()` returns `Solid` or surf's `Model`, daSurf 10.1); 15.1 (parts
  after execute; -validate, -fit, -compare on the solid; two-sided slots; stats; -check); 15.3
  (dagrender two-sided light); 16 (sheet_sync cases); 17 (`tree.das`).
- `_doc/dsl.md`: no change (4796 of 4800 bytes; `model() : Solid` stays its contract).
- daPCG's `agent_modelling_roadmap.md` 6, 10: composition and the tree generator take Models.
  `unittests/daCSG/sheet_sync.py`: the caps, the Model calls in the sheet check, the tree on 1
  and 4 threads, the solid models' bytes against the base console.

```
## Phase 2b: surface parts in the model output

- Model and its `|`, card and quad, Graph::content, surface_parts, the console's
  Model path, two-sided slots and light; three commits: the shared conversion
  and append refactor, the feature, the console and the models.
- Done: models/tree.das, a trunk and 12 branches (cones and tubes, unioned in daCSG)
  with 2000 folded cards placed by a fixed seed, builds on both kernels:
  1 tessellation for the cards, one dag node, the leaf slot two-sided and the bark slot not,
  the same .obj and .mtl from 1 and 4 threads, a dagrender picture; a surfaces-only model
  (a quad, an open loft roof) builds with no execute; each `model() : Solid` sample writes
  the same bytes; test_parts.cpp: content() composes nested maps and fails past 2^450,
  a mirrored instance keeps du x dv outward, a non-similar instance equals its world
  tessellation, a 1 mm card at 1e7 m is the area error at its join line,
  a NaN size and a 90 degree fold fail at the model line, the count cap;
  the sheet caps and the line limit hold; the place step alone (2000 cards, one thread)
  is timed against the base exe on the same machine and recorded, with no absolute limit.
```

## 8. Open questions for the product owner

1. The join operator. `|` for a surface with any part (recommended: your rule as written, and
   `&` and `-` stay free for the clip), or only `+`, so no reader takes a leaf join for a
   boolean? Either way the roadmap's `+` composition joins surfaces when it lands.
2. Gates (Base). About 215 lines (surf.das 40, the das module 160, the core 15) and about
   550 bytes of surf.md. Recommended: limit 8250, surf.md 3600, the pair 8400; dsl.md as is.
3. Two-sided. Recommended: MATF_2SIDED on the .dag slot of each mat id an open part uses, no
   back faces (a mat id a solid also uses becomes two-sided too: keep ids apart). Or back
   faces as geometry, which doubles the leaf triangles.
4. Nodes. Recommended: one .dag node as today until the scene producer (roadmap 1). Or a
   second node for the surface parts now, so an import can keep leaves out of collision.
5. Instancing. Recommended: shared tessellations through trailing transforms now; a bulk
   call (`scatter(x, maps)`, one list node, no surf node per instance) when a model needs
   more than about 50k instances (grass), as each transform is a surf node of about half a
   kilobyte. Or the bulk call now.
