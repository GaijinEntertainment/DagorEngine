# daCSG: specification

daCSG is a CSG library for closed triangle meshes with attributes, a daslang
DSL that builds CSG expressions, two boolean kernels that evaluate them, and
the tools and tests around them. `dsl.md` is the DSL sheet. SDF brushes
(with automatic unwrap and relax) are a later part with their own document.

## 1. Goals

- Robust: a boolean never crashes, never asserts on user data, never gives
  NaN or an open mesh. A case the kernel cannot do returns an error text.
- Fast: a model of about 300 primitives and 150k output triangles builds in
  seconds (section 13), and the bench keeps every kernel within the margins
  of its recorded rows (section 16).
- Good geometry: no sliver and no degenerate triangle that the cleanup can
  remove within its move bound (section 8), the rest counted; smooth
  shading groups kept, attributes (UV, color, material) carried through.
- Two kernels behind one interface, chosen per asset at execute time, and
  validated one against the other.
- A DSL that a model writer learns from one sheet (section 14).
- Small: a build with the manifold kernel alone is under half the size of
  carve, the library daCSG replaced, and one with the exact kernel alone
  under its own limit (section 16).

Non-goals: fillets and smooth blends (the SDF part), B-rep or exact curved
surfaces, glTF export, runtime use in a game (permitted, not tuned).

## 2. Layout

```
prog/gameLibs/daParallel/            the only calls into the engine thread pool
prog/gameLibs/daCSG/                 library gameLibs/daCSG.lib
  *_api.h            public headers, included as <gameLibs/daCSG/..._api.h>:
                     mesh_api.h, solid_api.h, graph_api.h, leaf_api.h,
                     execute_api.h, kernel_api.h, validate_api.h,
                     fp_api.h (the probes of section 11), and io_api.h,
                     whose code is the separate io lib below
  _doc/              this document, dsl.md
  mesh/              solid (halfedge, vertex normals), bvh, output normals,
                     the ear clipper, the pairwise tree walker
  graph/             graph, optimize
  io/                obj, dag Mesh conversion: outside daCSG.lib, a client
                     that reads or writes files lists gameLibs/daCSG/io
  prims/  validate/  the primitives; validate, compare, selfIntersect
  cleanup/           weld, degenerate, sliver, coplanar merge, metrics
  kernelManifold/    shadows, boolean, assemble, simplify; LICENSE (section 6)
  kernelExact/       bigint, exactPoint, predicates (int64 grid; shewchuk),
                     planes, registry, crossings, cdt, cells, ray, extract,
                     the output pass
  exec/              lowering, execute, attributes, cluster (the only layer
                     that names the cleanup)
  kernels/           the kernel registry, a lib per set a client links:
                     kernels/manifold, kernels/manifold_exact (the registry
                     source lies in the second, which the first builds too)
  das/               gameLibs/daCSG/das.lib: csg module (C++) + csg.das prelude
prog/tools/daCSG/    console/ (dacsg: run .das, write .dag/.obj), models/
prog/tools/dagUtils/dagRender/  dagrender exe: a .dag to a .png (software)
unittests/daCSG/     test exe: unit, degenerate, fuzz, cross-kernel (manifold
                     against exact), -bench with bench_baseline.csv, loc.py,
                     sheet_sync.py
```

Each folder is a jam lib that uses only the layers before it, in the order
above; the kernels use `mesh/` and `daParallel` only; `daCSG-options.jam` (the include
paths and `fp-options.jam`, the floating point flags of section 11) is
included by every daCSG layer's jamfile; `gameLibs/daCSG.lib` pulls the core
layers and no kernel, and a client that reads or writes files links
`gameLibs/daCSG/io` itself. A client lists exactly one kernel set,
`gameLibs/daCSG/kernels/manifold` or `kernels/manifold_exact` (the
console, meshCSG and the test exe take the second): the lib of that set's
kernels and of the registry built for it. A jam run reads a jamfile once,
so a set a variable chose would be the first client's for every client of
the run (`unittests/daCSG/kernelSets/` builds two in one run); an execute that
names a kernel the build left out fails with "kernel not built: <name>",
and `kernel_names()` lists the built ones, not the internal `concat`
kernel (Concat trees only, no boolean), which `find_kernel` still finds
by name. `daParallel = no ;` builds
`daParallel` as plain loops (`daParallel~serial.lib`) for a client without
the engine pool; a jam run reads that jamfile once, so the first
client's choice holds for every client of the run. Dependencies:
engine/math, eastl, dag containers, vecmath (bbox tests in broad phases),
engine/baseUtil (the thread pool, `daParallel` built with it) and
engine/osApiWrappers (the files of `io/`); no tools library, no daScript;
`.dag` writing (libTools dagFileRW) lives in the console tool only.
Namespace `dacsg`.
Every C++ source file takes the one-line copyright header, the `*_api.h`
public headers in the lib folder too; a jamfile takes none, and a .das
module (csg.das, surf.das) takes none, as the repo's .das files do.

## 3. Data model

### 3.1 Mesh (public exchange struct)

```
struct FaceInfo { uint32_t prim; uint32_t srcFace; uint32_t smgr; uint16_t mat; uint8_t attrSet; };
enum ChannelId : uint32_t { CH_UV = 0, CH_COLOR, CH_NORMAL, CH_UV2, CH_UV3, /* ... */ CH_UV10, CH_USER = 0x10000 };
struct Channel  { uint32_t id; uint8_t dim; dag::Vector<float> data; }; // per corner
struct Mesh {
  dag::Vector<Point3>   pos;       // float
  dag::Vector<uint32_t> tri;       // 3 per face, CCW seen from outside
  dag::Vector<FaceInfo> face;      // one per face
  dag::Vector<Channel>  channels;  // data.size() == face.size() * 3 * dim
  StringTableAllocator  userNames; // names of user channels only
  dag::Vector<uint32_t> userNameOfs; // by id - CH_USER; NO_NAME when unnamed
};
```

- Orientation is numeric: the outward normal of a face is
  `cross(p1 - p0, p2 - p0)` and the signed volume of a Solid is positive.
  The engine Mesh normal (`calc_facenorms`) is the opposite winding, so
  `to_dag_mesh` and `from_dag_mesh` swap v1 and v2; `.obj` keeps ours.
- Channels are per corner, so a UV seam on a sphere or cylinder survives.
  `CH_UV` (dim 2) and `CH_COLOR` (dim 4, linear RGBA; a primitive starts
  at gray 0.8, 0.8, 0.8, 1) exist in every Mesh that resolve or
  `addDefaultChannels` has filled; `solid_to_mesh` gives pos, tri and face
  with no channel; `CH_NORMAL` is dim 3; `CH_UV2`
  to `CH_UV10` are further uv sets (one id per tvert channel of the dag
  Mesh, pinned against `NUMMESHTCH`); `CH_USER + x` is a user channel with an
  optional name, interned once per graph when the DSL names one. A merge
  takes the union by id. `to_dag_mesh` maps `CH_UV` to `tvert[0]`,
  `CH_UVn` to `tvert[n-1]`, `CH_COLOR` to `cvert`/`cface`, and drops user
  channels with a warning; `.obj` writes `vt`, `vn`, one material per
  color. `addChannel` of an id that exists with another dim re-creates it
  zero-filled with the dim asked.
- The readers are the boundary with the file, so a bad index never reaches
  a kernel through a client that validates its own leaves: `read_obj`
  refuses what a Mesh cannot hold (a smoothing group above 32, more than
  65536 materials, an index out of range) and a line over 16 MiB with the
  line number, and
  `from_dag_mesh` refuses a face with a vertex index out of range. A face
  before the first `usemtl` has its own material. `write_obj` writes the
  smoothing groups, keys its materials by the float RGBA of the source
  color (8-bit bins would merge colors one step apart) and writes it with
  9 digits, so two materials never print one color, and gives a face the
  color of its first corner, since obj has no per-corner color. The reader
  takes a space or a tab after a tag.
- Per-face integers copy without interpolation: `mat`, `smgr`, `prim`
  (graph node id of the source primitive; a Cut's cap faces carry the Cut
  node's id, the node that made them, so `primDesc` is not for that id),
  `srcFace` (face index in its tessellation, the cap box's for a cap).
  Every output face is a piece of one primitive face or of a cap, so
  these are exact provenance.
- `smgr` follows the engine bit-mask rule inside a primitive (0 flat, two
  faces smooth when their bits intersect): a corner's normal averages the
  faces at its vertex whose bits meet its own face's bits, no transitive
  group, so masks 1, 3, 2 around a vertex smooth the 3 face with both and
  the 1 face with the 2 face not. Across a boolean an edge is
  smooth only when both faces come from the same leaf instance and their
  bits intersect, so an intersection crease between two smooth parts stays
  hard. The library computes the output normals itself (`CH_NORMAL` at
  export) with that rule; `to_dag_mesh` assigns fresh `smgr` bits per
  smooth region by greedy coloring (a region with no free bit takes the
  one fewest of its corners share, a flat face of the rule stays 0) and
  splits no vertex, so a round trip keeps the mesh closed: `facengr` holds
  the normals, including a crease inside a region that its one bit would
  smooth. A leaf may carry a `CH_NORMAL` of its own (a curved
  primitive's analytic normals, section 4, or an external leaf's, 3.3;
  both beat any smgr rule): the resolve interpolates it like any channel
  and the export uses it for that leaf's faces; across a crease the other
  side keeps its rule.
- `attrSet` is a bit mask (color, mat, uv projection) of the attributes a
  user set on the face; a primitive starts with none. An `Attr` node in
  the "only unset" mode skips faces whose bit is set (`rgb_uncolored`,
  `mat_unset`).

### 3.2 Solid (kernel object)

A closed, consistently oriented 2-manifold triangle mesh with double
positions, a halfedge array (Manifold layout: face f owns halfedges
3f..3f+2, `{startVert, endVert, pairedHalfedge}`), a bbox, flags
(`closed`, `convex`), vertex pseudo-normals (the angle-weighted mean of
the incident face normals; the manifold kernel's tie-break reads them) and
the per-face data of `Mesh`; no channel data (5.4 computes channels from
provenance). Only primitives, kernels and a validated import make one. A
Solid may hold several shells (a hollow object, or nested outward shells,
which the exact kernel winds past 1) and duplicated vertices where two
shells touch at a point or an edge; "closed" means every
halfedge has exactly one pair with the opposite direction. An empty Solid
is valid. Plane provenance, both arrays optional: `provenance` per vertex
holds up to three keys `geometry << 32 | plane` of the leaf planes the
vertex lies on (a leaf vertex takes the planes of its faces, a crossing
the two of its edge and the one it crossed; a moved vertex loses them),
and `facePlaneKey` per face the key of its leaf plane; an empty array or
an empty entry knows nothing.

### 3.3 Expression graph

```
struct PlaneD   { DPoint3 n; double d; };    // dagorInclude has no double plane
enum class NodeKind : uint8_t { Prim, Union, Subtract, Intersect, Concat, Cut, Transform, Attr, Empty };
struct OpResult { uint32_t id = 0; };        // 0 is the empty solid
class Graph {
  OpResult prim(const PrimDesc &);           // any PrimType but Leaf, which only leaf() makes; mesh import included
  OpResult leaf(ILeafSource *);              // an external leaf
  OpResult op(NodeKind, OpResult a, OpResult b);   // Union, Subtract, Intersect, Concat
  OpResult cut(OpResult a, const PlaneD &keepBelow);   // keeps dot(n, p) <= d
  OpResult transform(OpResult a, const TMatrixD &tm);
  OpResult attr(OpResult a, const AttrDesc &);
  void setName(OpResult, const char *, bool generated = false); // side table by handle, last write wins
};
```

- Nodes are immutable and hash-consed (same kind, children and payload:
  the existing id); names are a side table, not part of the hash. A
  shared subtree evaluates once: lowering (5.2) stops the transform fold
  at a node with two or more parents that contains a boolean, evaluates
  it once, and makes each use a rigid copy of the result (a negative
  determinant flips winding), so an instanced sub-assembly costs one
  boolean, not one per instance.
- Every graph walk is iterative with an explicit stack; a depth above 100k
  is an error text, so a long `-=` chain cannot overflow the C++ stack.
- A primitive node stores its parameters, not a mesh; the optimizer reads
  convexity, a conservative bbox and foldability from them, and the
  tessellation happens at execute time with the execute quality. Point
  lists live in a side array and are interned, so equal lists hash-cons.
  A description is normalized before hashing: unused fields zero, `seg` 0
  or clamped to 3..256.
- An external leaf is a primitive another library tessellates (daSurf,
  through its das module); daCSG names no other library. `ILeafSource`
  (leaf_api.h): `tessellate(worldTm, quality, tolerance, sag, Mesh &out,
  err)` with sag the chord error the leaf keeps in world units,
  `contentHash()` with `equals()` for hash-consing, a local conservative
  `bbox()`, `name()`; the owner outlives the graph. The optimizer prunes
  by the bbox grown by `reach()`, transformed, and grown along each axis
  to `reach()` times the product of the transforms' largest scales,
  never under the contract's `reach() * cbrt(|det|)`; a leaf is not
  convex and no transform folds into it; the graph bbox that sets the
  auto tolerance takes the source's bbox, so nothing tessellates before
  the tolerance is known. Under a similarity (a rigid map, a mirror or a
  uniform scale s, orthogonal to 1e-6 relative, float rotation
  precision) the instances share one tessellation per node and scale,
  made with `worldTm = s * I` and moved by the rest, a mirror flipped as
  for a primitive; under any other map the source tessellates per
  instance and flips itself when the determinant is negative. The mesh
  is validated as a mesh import (sizes, closed, oriented, the
  self-intersection test) unless `ExecuteParams::validateLeaves` is
  false, which skips that for every leaf source but the mesh contract
  (sizes, finite values, which no comparison may meet as a NaN), so a
  client that validated its meshes pays once; primitives and file
  imports are validated whatever the flag. A missing `CH_UV` or
  `CH_COLOR` gets its default; a `CH_NORMAL` must have dim 3. `prim` is
  the leaf node, `srcFace` the face index in the leaf mesh. A failed
  tessellation is the execute's error '<node name> (<file>:<line>):
  <source name>: <source text>'.
- `Concat` is a node kind, so the optimizer can write it into the graph.
- `Transform` holds a 4x3 matrix; no kernel sees one (5.2). The DSL's
  rotations follow the right-hand rule (a positive angle turns Y toward Z
  about X, Z toward X about Y, X toward Y about Z); the das module builds
  the matrix itself, since `rotxTM`/`rotyTM`/`rotzTM` of dag_TMatrix.h
  turn the other way. A test pins `rot_x(90)`: +Y goes to +Z. An angle
  drops its whole turns first (fmod, exact for any finite float), so a
  multiple of 90 degrees is exact at any size; a non-finite angle is the
  model error "an angle is not finite". An axis component not finite or
  past 1e18 is the model error "rot: an axis component is not finite or
  past 1e18": the limit keeps the float length of the axis finite. An
  axis with all components under 1e-15 is divided by its largest
  |component| before its length, as its float squares can flush to zero;
  only a zero axis is "rot: the axis is zero".
- `Attr` applies to every face of its subtree result: constant color,
  material id, flat or smooth shading, a UV projection (planar, box,
  cylinder, sphere) or a UV affine (scale, offset, rotation about the UV
  origin), with an `onlyUnset` flag (3.1). Lowering takes Attr nodes out
  of the tree as records `(leaf set, AttrDesc, per-leaf flip)` in tree
  order, applied by 5.4; consecutive UV affines with the same `onlyUnset`
  compose into one record. An `onlyUnset` affine skips the faces with the
  uv projection bit; an affine does not set it.
- `Cut` is lowered to `Intersect(a, H)`, H a box leaf with the cut plane
  as one face and the other five 1 % of a's bbox diagonal outside a's
  bbox on the keep side; a plane that misses a's bbox gives `a` or `Empty`
  with no kernel call. A kernel may offer a native clip (6.8) once it
  passes the same degenerate suite.
- A node has an optional name (`"tank"`) for errors, stats and provenance;
  the default is `<type>#<id>`.

## 4. Primitives

Y up, meters, degrees. The axis of a cylinder, cone, capsule, torus hole,
extrusion and revolution is Y. The analytic primitives (box to torus) are
centered at the origin; `extrude` keeps the polygon's (x, z) and centers
the height on y = 0; `revolve`, `convex` and `tube` use their points as
given. Every primitive is a closed manifold with analytic UV, smoothing
groups and `srcFace` provenance. `seg` is the segment count around the
axis, a given value clamped to 3..256: the das binding gives `seg : int
= 0` to rbox, sphere, ellipsoid, cubesphere, cylinder, cone, ccyl,
capsule, torus (with `segMinor`), revolve and tube; box, icosphere
(`level`), convex, extrude, loft and mesh take none. 0
takes `ceil(pi / acos(1 - sag / r))` in 3..256 for the largest radius r
(a torus counts R + r around, r along the tube), where `sag` is 1e-3 of
the root bbox diagonal / quality^2, divided by the largest scale of the
primitive's instances, and the same `sag` is every leaf's chord. With
1e-3, at quality 1 no facet of the sport bike sags more than its largest
round part did at the former 32 segments. `segMinor` (torus) defaults
to `seg / 2` when `seg` is given; `level` (icosphere) is the subdivision
depth 0..7. Every count along a profile is `seg` scaled to its arc and
rounded up, so a profile chord sags no more than a ring chord: a sphere
or ellipsoid meridian `seg / 2` stacks, a capsule dome `seg / 4`, an
rbox quarter arc at least `seg / 4`, and a cubesphere face side
`seg / 4` (so rbox and cubesphere cost about what a sphere costs). The
diagonal that splits a quad spans both chords and sags about twice the
value asked; the chords, not the diagonals, are the contract, and the
tests measure them against the analytic surface. A
tessellation's volume is inside the analytic surface; the tests bound the
gap by `c / seg^2` with c = 7 for the cylinder family, 18 for the
spheres, 35 for the torus.

| Primitive | Parameters | Convex | UV | Smoothing |
|---|---|---|---|---|
| box | size | yes | per face, planar | 6 flat groups |
| rbox | size, radius, seg | yes | box projection | corners smooth, faces flat |
| sphere | r, seg | yes | lat-long, one chart | smooth |
| ellipsoid | radii, seg | yes | lat-long, one chart | smooth |
| icosphere | r, level | yes | lat-long per vertex, seam and pole vertices split | smooth |
| cubesphere | r, seg | yes | six box charts | smooth |
| cylinder | r, h, seg | yes | side unwrap u=angle, v=height; caps planar | side smooth, caps flat |
| cone | rBottom, rTop, h, seg | yes | as cylinder | side smooth, caps flat |
| ccyl | r, h, chamfer, seg | yes | as cylinder | side smooth, chamfer smooth, caps flat |
| capsule | r, h (total), seg | yes | side unwrap, hemispheres lat-long | smooth |
| torus | R, r, seg, segMinor | no | u=major angle, v=minor angle | smooth |
| convex | points | yes | planar per face | flat per face |
| extrude | polygon (XZ, any winding, no holes), h | no | sides: u=perimeter, v=height; caps planar | sides by angle threshold, caps flat |
| revolve | profile (radius, y), angle | no | u=angle, v=profile length | by angle threshold, caps flat |
| tube | path, r, seg | no | u=angle, v=path length | smooth, caps flat |
| loft | sections of equal point count, closed | no (convex sections can still loft a concave body) | u=point index / n, v=distance between section centers; caps planar | bands smooth, caps flat; the orientation from the signed volume, so winding and order do not matter; `closed` joins the last section to the first |
| mesh | file path, read by the client's `ExecuteParams::meshLoader` | no | from file | from file |

- The executor validates a primitive after tessellation (section 9),
  before a kernel sees it; a parameter that cannot give a valid solid is
  an execute error with the node name. Parameter checks come first and
  reject a zero size, a torus with r >= R, a revolve profile that crosses
  itself or the axis, a self-crossing extrude polygon, a tube path with a
  repeated point or a bend radius below r, a loft whose sections lie in
  one plane (no volume to orient by). The executor runs the
  self-intersection test for every import and every primitive that can
  fold (extrude, revolve, tube, loft); the others cannot self-intersect
  by construction and skip it. One float guard follows every builder: a face
  with no area, or an edge within 8 float steps of its corners' size,
  fails the primitive with "below float precision at this size"; a
  corner past 1e18, where the builders' float squares overflow, with
  "beyond float range at this size" (the convex input check gives the
  same text; the extrude and revolve input checks give "polygon is
  invalid: a point is beyond float range at this size" and "profile is
  invalid: a point is beyond float range at this size"); a part whose
  extent is under 1e-6, where the builders' float squares underflow,
  with "below the smallest size at this precision"; all after the node
  name. A unit direction from a cross product or an area goes through
  `unit_or_zero` (exact down to 1e-154), never Dagor's `normalize`: that
  returns zero below 4e-19, which a thin cap of a part above the floor
  reaches. A parameter within 1e-6 of a limit where a profile part
  collapses is at the limit, from either side: the rbox radius at half
  the smallest size, the ccyl chamfer and the capsule r at h / 2, the
  revolve angle at 360; a tube ring within the float floor of the last
  one is skipped.
- `extrude` triangulates the cap polygon with the kernels' ear clipper;
  the DSL polygon has no holes (a hole is a subtract of a second extrude).
  `revolve`: an open profile whose first and last points lie on the axis
  (poles, no zero-area face), or a closed polygon off the axis; `angle <
  360` closes the sweep with two flat caps. `tube`: the circle swept along
  the path with a rotation-minimizing frame, each corner a torus-section
  elbow of bend radius 2r (less where a segment is too short, never at or
  below r), `ceil(turn / (180 / seg))` rings along the arc and rings at
  the tangent points, none at the corner vertex (a ring turning about the
  corner folds the inner side through the tube); flat caps, UV
  continuous; no boolean. A path whose non-adjacent segments come closer
  than 2r is an error ("path overlaps itself: split it in the model").
- `mesh` import: the loader of the execute call
  (`ExecuteParams::meshLoader`) reads the path as the model gave it (the
  DSL resolves a relative one against the calling file's folder); the
  library names no format, and without a loader the import is an error
  that names the field. The console reads `.obj` (`read_obj`: positions,
  uv; `usemtl` to `mat`, `s` to `smgr`; the other statements and the
  `.mtl` dropped, an unknown word or a malformed statement refused with
  its line) and `.dag` (via the engine Mesh, each face's material by its
  node's name for it: the id the export writes, `paint` to `metallic`
  0..4, `mat<id>` its id, a row name of the model (material.md) its id,
  else 0); a mesh that fails validation is an error, never a crash.
- The primitive sets `smgr` and, where it knows its surface (the lathe
  family with exact profile normals for sphere, ellipsoid, capsule and
  torus, rbox, cubesphere, icosphere, tube, the extrude sides), per-corner
  analytic normals in `CH_NORMAL`, so a curved primitive shades by its
  surface and not by its facets at a coarse `seg`; convex, loft, a plain
  box and a mesh import write none. `Attr` can force flat or smooth on a
  subtree, which clears those normals, so an explicit shading wins.

### 4.1 UV conventions

- Every chart spans 0..1. Box, rbox, cubesphere: one chart per face (u
  along the first tangent axis, v the second). Sphere, ellipsoid, capsule
  domes: lat-long, u = atan2(z, x) / 360 deg + 0.5 (the seam at -X, +X is
  0.5, +Z is 0.75), v = 0 at the south pole, 1 at the north; the same u
  rule serves every "u = angle" chart. Icosphere: the same per corner,
  every face that crosses the seam split at its crossing edge so no face
  spans it (positions shared, the mesh closed; a pole corner takes the
  mean u of the other two). Cylinder, cone, ccyl, capsule side: u =
  angle, v = length fraction (the chamfer of ccyl continues v). Tube: u
  = angle around the path from the start normal of its frame, the path
  direction crossed with +Y (with +X for a path along y), so a path
  along z has its seam at -X; v = length fraction. Caps planar, the
  cap's bbox mapped to 0..1. Torus: u = major, v = minor
  angle. Extrude: sides u = perimeter fraction (seam at the first polygon
  vertex), v = height, caps planar. Revolve: u = angle, v = profile
  length fraction. Convex: planar per face in its tangent frame, the face
  bbox to 0..1. Mesh import: as in the file.
- A boolean keeps the UV of each face's source primitive (5.4), so an
  intersection curve shows each side's chart.
- UV projections (`uv_planar`, `uv_box`, `uv_cyl`, `uv_sphere`) replace
  the UV of a subtree by a function of position and normal, 1 UV unit =
  `scale` meters (world-scale texturing); `uv_sphere` is angular like the
  sphere primitive's chart (u in turns, v in half turns from the south
  pole) and takes no scale. `uv_box` picks the chart by the
  dominant axis of the unsigned normal and the u direction by the sign,
  so a flipped face gets the mirrored chart. A projection is evaluated per
  corner on the final face (5.4), never interpolated; on a face that
  crosses the u seam of `uv_cyl` or `uv_sphere` the corners are shifted
  by whole turns to the narrowest span; a face whose corners go around the
  axis keeps its narrowest span, with the seam inside it. UV affines
  (`uv_scale`, `uv_offset`, `uv_rotate`) transform the existing UV about
  the UV origin, in call order, on a primitive, an op result and the
  final solid alike.
- The seam rule: UV is per corner, and the resolve evaluates each corner
  in its own face's chart (5.4), so a chart seam never bleeds, whatever
  the cleanup moved: a moved corner reads the nearest leaf triangle of
  that chart, else the nearest point of its face's own.

## 5. Execution

### 5.1 API

```
struct CleanupParams { double sliverAngleDeg = 0.5; double maxAspect = 1000; double flipDihedralDeg = 1;
                       double maxMoveFactor = 100;      // the surface stays within maxMoveFactor automatic
                                                        // tolerances, at least 2 tolerances; a vertex may move
                                                        // further (section 8)
                       bool wideCrossingCheck = false; };  // the crossing check near all new faces (section 8)
enum class Backend : uint8_t { Int64Grid, Shewchuk };   // the number backend of the exact kernel (7.1)
struct KernelParams  { double tolerance = 0;
                       ThreadBudget *budget = nullptr;         // section 12; execute sets its own
                       int threads = 0;                        // 0 = pool size
                       uint32_t maxArrangementTris = 1000000;  // 10.8
                       int gridBits = 40;                      // 1..40, 7.1
                       Backend backend = Backend::Int64Grid; };
using MeshLoader = dag::FunctionRef<bool(const char *path, Mesh &out, eastl::string &err) const>;
struct ExecuteParams {
  const char *kernel = "manifold";   // "manifold", "exact", registered test kernels
  float quality = 1.f;               // multiplies the model's set_quality
  bool optimize = true, cleanup = true, validate = true;
  bool clusterVertices = true;       // 10.10
  bool kernelFallback = true;        // a tree the kernel refuses goes to the other library kernel (5.2)
  bool validateLeaves = true;        // false: an ILeafSource mesh meets the contract check only, its client validated it (3.3)
  MeshLoader meshLoader = nullptr;   // reads the mesh imports of this call (4); not owned: its callable outlives the call
  CleanupParams cleanupParams;
  KernelParams kernelParams;         // tolerance 0 = auto, see below
};
bool execute(Graph &g, OpResult root, const ExecuteParams &p, Mesh &out,
             ExecuteStats *stats = nullptr, eastl::string *err = nullptr);
```

`tolerance = 0` means `max(1e-7 * diag, 8 * ulp_float(max |coordinate|))`
over the root bbox (a union's or a concat's is both operands' boxes, an
intersection's their overlap, or the first operand's when they are
disjoint, a subtract's its first operand's, a leaf's grown by its reach),
so a model far from the origin keeps a tolerance above the float grid.
A root bbox past the float range is an error with any tolerance, as the
output is floats. `execute` returns false with a text in `err` on any
failure and never asserts on data. `ExecuteStats` lists per node time,
triangle counts and cleanup counts, the kernel's counters, totals and
warnings.

### 5.2 Pipeline

1. Optimize the graph (section 10): every pass is optional, off with
   `optimize = false`, and correctness never depends on one.
2. Lower to a boolean tree, always. A Transform folds into the leaves when
   its linear part keeps exact contacts (a translation, an axis scale with
   any signs, a rotation by a multiple of 90 degrees, or a signed
   permutation times a diagonal). Any other transform above a boolean (a
   general rotation) stays above it and is applied to the subtree's
   result as a rigid copy, the path of the shared-subtree instancing
   (3.3): a folded general rotation turns every flush contact of the
   model into a near-tie that no symbolic rule and no grid snap can see,
   and both kernels then leave membranes. Lowering moves cut planes,
   rewrites Cut and takes Attr nodes out (3.3), tessellates the
   primitives with the quality and validates them (section 4), and turns
   a Union of parts whose bboxes expanded by the tolerance do not overlap
   into Concat (no boolean, the Solids are joined). Inner nodes are Union,
   Subtract, Intersect, Concat; leaves are Solids with a leaf index (one
   primitive node under one transform, or one use of a shared subtree).
3. Hand the tree to the kernel: `IKernel::evaluate(tree, params, out)`. A
   pairwise kernel evaluates bottom-up with `boolean(op, a, b)`; the exact
   kernel evaluates a whole tree at once. A shared subtree is its own
   tree, evaluated first; each use enters the parent tree as a leaf after
   its result validates (closed, self-intersection test), else the
   instance is folded into leaves and `ExecuteStats::warnings` gets "node
   <name>: a shared subtree does not validate: <text>; it is evaluated in
   the root's tree". A result whose self-intersection test
   finds crossings after a cleanup is first evaluated once more on the
   same kernel with the cleanup's wide crossing check (section 8), here and
   at the root before the fallback below; the rerun runs the kernel again
   rather than keep a copy, and `ExecuteStats::warnings` gets "node <name>:
   <n> self-intersecting face pairs after the cleanup; evaluated again
   with the wide crossing check". Fallback: when the kernel refuses a
   tree with its own error text, `kernelFallback` is set and the other
   library kernel is built, the other kernel evaluates the same tree and
   `ExecuteStats::warnings` gets "node <name> (<file>:<line>): <kernel>
   refused: <text>; evaluated by <other>"; a root result that fails
   `validate` is evaluated again the same way ("root: <kernel> result
   failed validation: <text>; evaluated by <other>"); when both fail, the
   error holds both texts and the output keeps the chosen kernel's mesh.
   The bench calls the kernels directly and the console's `-compare` runs
   without the fallback, so a gated or compared result is the kernel's
   own; the default build links manifold only, so there a refused tree
   stays an error.
4. Round every position of the result to float and store it back as
   double, so the cleanup and the validation see the values the Mesh will
   hold.
5. Cleanup pass (section 8).
6. Validate (section 9) when `validate` is set, with the self-intersection
   test: about a tenth of execute's time, paid so that a crossing a cleanup
   edit can leave (two parts closer than its move bound) is an error or a
   fallback, never a silent result. A failed validation is an error, the
   mesh is still returned for inspection.
7. Resolve attributes (5.4), convert to `Mesh`.

### 5.3 Kernel interface

```
struct IKernel {
  virtual const char *name() const = 0;
  virtual bool evaluate(const BoolTree &, const KernelParams &, Solid &out,
                        KernelStats &, eastl::string &err) = 0;
};
struct IPairwiseKernel : IKernel {   // evaluate() walks the tree bottom-up
  virtual bool concurrentBooleans() const { return false; }   // true: boolean() calls may run at once (12)
  virtual bool boolean(BoolTree::Kind op, const Solid &a, const Solid &b, const KernelParams &,
                       Solid &out, KernelStats &, eastl::string &err) = 0;   // a kernel is a singleton: no params as members
};
void register_kernel(IKernel *);     // a client's own kernel, by name
IKernel *find_kernel(const char *);
```

### 5.4 Attributes through a boolean

Every output face carries `(leaf, leafFace)`: the leaf instance in the
lowered tree and the face index in that leaf Solid; kernels carry no
per-vertex attribute data. The resolve computes, per output corner, the
barycentric coordinates of the corner in the leaf triangle that holds it
within the tolerance and mixes that triangle's corner channels with them.
That is the face's leaf face, else a leaf face of the same chart: a walk
from the face's leaf face across the leaf edges whose two vertices carry
corners of one class on both sides (a source's corners of a vertex,
joined across each edge where every channel is equal within float
rounding, as a daSurf normal per cell differs by 1e-16; computed at the
source's first leaf, like its face planes), nearest face first and no
farther than the leaf face by 4 tolerances, to the face that holds the
corner, else the nearest one it met, since a cleanup edit slides a corner
across leaf edges (a sliver's collapse runs along a flat strip) and off
the surface within its move bound; when the walk finds no nearer face,
the face's leaf face takes it projected onto its plane and clamped. A face
keeps `mat`, `smgr`, `attrSet`, `prim` and `srcFace` of its leaf face.
The leaf Solids stay alive until resolve;
leaves with different channel sets give the union, a missing channel
reads as zeros. A leaf mesh with its own `CH_NORMAL` (3.1) marks its
faces: the resolve maps their corner normals by the leaf instance's map
(the inverse transpose of its linear part, normalized), so a shared
tessellation under a rigid map or a mirror gets turned normals, negates
them on a kept face of a subtracted operand (it turns over in the
output), hands an own subtree's mark to its instances, and the output
normals keep the marked faces' normals; an Attr `flat` or `smooth` clears
the mark. A mesh import's normal channel is not kept.

Where faces of two leaves coincide in the output (touching parts of two
colors), the output face takes the leaf first in tree order among the
leaves whose face has the output orientation (for Subtract the minuend
first); the manifold kernel gives that by its perturbation (6.3), the
exact kernel applies the rule to a merged face (7.2.7), and the
cross-kernel test checks colors per face. Tree order is the optimized
tree's: the union flattening (10, pass 6) pairs touching leaves by
position, so the winner is deterministic but not always the earlier leaf
in source order; with `ExecuteParams::optimize` off the earlier leaf wins.

Attr records (3.3) then apply, outermost last, to every output face whose
leaf instance is in the record's leaf set. A projection evaluates in the
Attr node's frame: the record holds the inverse of the transforms composed
above the Attr, positions map through it and normals through the
transpose of the composed transforms' linear part (the record's
`toRoot`), so `part.uv_box(1.0).at(5.0, 0.0, 0.0)`
keeps the uv of the part at the origin while `part.at(5.0, 0.0,
0.0).uv_box(1.0)` projects the moved part. The normal is the face's final
normal times the record's flip (-1 when the path from the Attr up to
the root passes an odd number of Subtract right children): a face that
survives a boolean is on the boundary of every enclosing subtree result,
so this equals applying the Attr to that intermediate mesh.

## 6. Kernel "manifold"

Smith and Dodgson 2007 as the Manifold library (Lalish, Apache-2.0)
implements it: doubles, symbolic perturbation, every topological decision
consistent, the output always a closed manifold. Written from the paper
and Manifold's published description for our data model; the shadow
kernels of shadow.cpp translate Manifold's src/boolean3.cpp (Apache-2.0),
so `kernelManifold/LICENSE` holds that notice, and the notice ships with
every exe that links the manifold kernel.

1. Inputs: two Solids with halfedges and bboxes; `tolerance` as in 5.1.
2. Broad phase: a BVH per input over triangles (vecmath AABB tests); edges
   of P against faces of Q, vertices of P against faces of Q, and the
   symmetric pairs.
3. Shadow functions in the projection order x, y, z: `shadow01` vertex vs
   edge, `shadow11` edge vs edge, `shadow02` vertex vs face, `intersect12`
   edge vs face (an intersection vertex and a winding delta), `winding03`
   the winding of a vertex inside the other solid. A tie resolves by a
   symbolic perturbation of P along its vertex pseudo-normals (3.2):
   outward for Union, inward for Subtract and Intersect (`expandP`). The
   kernels are the three of boolean3.cpp: every comparison is
   `Shadows(p, q, dp, dq) = p == q ? (dp != 0 ? dp : -dq) < 0 : p < q`
   with p the P point; a kernel on a Q element against P reads it with the
   P point first, and the reverse vertex-face kernel keeps a pair when the
   P face lies below the Q vertex, so the Q vertex's winding is that sum
   negated. Unlike Manifold, which moves Q too, `dp` is P's first-order
   term in the axis, never interpolated (a vertex moves along its
   pseudo-normal, an edge along the sum of its two face normals, a face
   along its normal); only where `dp` is zero does Q's term `dq` decide,
   negated, Q moved outward the same way, in all three kernels. So a tie
   does not depend on the axis; one vertex of each run of edges without a
   winding change reads its winding from the faces in its column and the
   run takes it; a plane or line known by provenance (3.2) is a tie under
   the same rule. This makes exact contact come out right: a union of two
   boxes that share a face has no inner wall, a subtract with a flush face
   leaves no skin, and a face that the other operand touches keeps the
   first leaf (5.4).
   All decisions on one pair use the same comparisons of the same input
   coordinates, which keeps them consistent; section 11 keeps the compiler
   from breaking that.
   A plane parallel to z has no area in this projection: a vertex or an
   edge on it by provenance ties in z with each face and edge of that
   plane, also with those far from it, which an edge's crossing search
   (near the edge only) does not reach, and the winding identity (7)
   fails. So when the operands share a plane key (one leaf in both, as
   `(a & (w - b)) | (a & b)`) whose plane is within 1e-6 of parallel to z
   (n.z^2 < 1e-12 |n|^2), the boolean cycles the coordinates of both
   operands (positions, vertex normals, bboxes) so that x or y becomes z,
   and cycles the result back; a permutation is exact. It takes the axis
   whose least unit-normal component over the faces of all shared plane
   keys is larger; when both are under 1e-6, z stays.
4. Intersection vertices, one per (edge, face) pair `intersect12` reports.
   The position interpolates between the two shadow samples whose side
   differs (the paper's xyzz interpolation), never from the plane
   distances of the edge ends: for an edge within rounding of the face
   plane those put the point anywhere on the edge and the loops do not
   close. The samples interpolate from the nearer end and keep the
   coordinate they interpolate at; an edge-edge crossing takes its y from
   the flatter edge read at the crossing x by the interpolation a later
   shadow01 uses, an edge-face crossing its z from the flatter of edge and
   face. A crossing on an edge vertical in x then lies exactly where the
   next boolean reads that edge or face, so a result that reuses a leaf's
   face meets it in exact ties the perturbation decides; a shared cut
   through different triangulations, `(a & (w - b)) | (a & b)`, does not
   (its curve vertices differ by ulps), which the provenance ties take.
   Crossings within the tolerance of each other or of a solid vertex are
   one vertex (two solid vertices join only through a crossing), merged
   crossings alternate their directions along the pair's line, a crossing
   on an edge of its face splits that edge for both faces with s = 0, a
   crossing on a new edge splits it on both faces: this assembles the
   degenerate loops of a mirror or grid contact. A crossing off the plane
   of either face by more than the tolerance does not split it: it is
   near the edge only because a merge moved an end of the edge.
5. Assembly per face: retained edge pieces and new edges from face-face
   intersections. A new edge joins the loop only when the windings on the
   two sides of its crossing include the side's wanted winding (an operand
   that touches itself has winding 2 inside after the perturbation, so a
   new edge between its winding-1 and winding-2 regions bounds nothing
   kept); where the windings cannot tell, the loop graph can: a new edge
   from a vertex with more segments out than in to one with more in than
   out lies between two dropped regions and is dropped. The chain takes
   the sharpest left turn at a repeated vertex and cuts the part since the
   first visit into its own loop; two-vertex loops drop. Polygons with
   holes go to an ear clipper in the dominant-axis projection (Manifold's
   `Triangulate`: no ear that leaves a flat rest unless nothing else is
   left, a corner within 1e-9 rad of straight is no ear, in the forced
   mode of mesh/polygon.cpp too, where a stalled loop still clips so
   every loop edge lands in exactly one triangle). Then a triangle
   thinner than 8 tolerances (the
   smallest altitude in the projection) takes the other diagonal of its
   quad with a neighbour of the same piece when that raises the smaller
   altitude of the two, from a worklist until no flip gains (each flip
   raises the piece's sorted altitudes, so it ends) or after 8 flips
   per triangle, the hard bound of the worklist: a greedy clipper
   leaves a needle on a straight run of the cut as its forced last ear,
   and a needle's tip within the tolerance of the neighbour's plane reads
   as a coplanar overlap in the validator. The tolerance enters the
   triangulation only here.
6. Op rules by winding: union keeps P faces with w(Q) = 0 and Q faces with
   w(P) = 0; intersect keeps faces with w = 1; subtract keeps P faces with
   w(Q) = 0 and Q faces with w(P) = 1 flipped.
7. Output halfedges pair by vertex ids (an intersection vertex is created
   once per pair, so both sides see one id). A group of four or more first
   pairs skins, then flat faces whose third vertex lies within the
   tolerance of the edge line (a grid contact of an edge on a face leaves
   such a strip; it has no wedge angle), then takes the wedge order by the
   angle of the third vertex around the edge, faces within 1e-9 rad as a
   cluster ordered by the wedge before it (an opener first when it is
   empty, a closer first when solid; both start states are tried), never
   by halfedge index; when no start state pairs the group, faces less than
   the tolerance apart at the nearer third vertex join one cluster too
   (where the crossings of a fin thinner than the tolerance merge, its
   walls can cross); a vertex with several fans is duplicated per fan. A
   loop's signed area is taken relative to its first point, so a loop of
   several crossings at one point reads as zero. The per-edge winding
   identity (the winding at an edge's end equals the winding at its start
   plus its crossings) is checked; a mismatch is an error text, never an
   open mesh. Refused by design: a face pair that meets in an odd number
   of points. `simplify`: the skins first, keyed by their three positions
   and orientation parity (the two vertices a skin pair shares unite, no
   other merge), the pairing rebuilt; then edges shorter than the
   tolerance collapse under the link condition (section 8), flat faces
   flip or collapse; then the pinch:
   vertices still joined by an edge below the tolerance that the link
   condition kept apart (two shells in contact) merge, faces with a
   repeated vertex drop, the pairing with wedge order runs again, which
   gives two closed shells with duplicated vertices at a point or edge
   contact; a shell with |volume| <= tolerance * area (from one of its
   own vertices) drops as a flat loop closed on itself, so a shell of mean
   thickness 2 |volume| / area up to 2 tolerances goes (a slab of a 2 m
   box 2.5 tolerances thick at tolerance 0.1 is one: its walls add a
   quarter to its area); then validate closedness. A corner that grazes a face of the other operand within
   the tolerance gets one crossing per edge leaving it, diagonals
   included, and which pieces survive is decided far below the tolerance,
   so the structure can close as a handle of no volume, a pinch, or not
   at all; the kernel keeps it as the model's (no rule moves a vertex by
   more than its perturbation, the cleanup keeps the genus), the exact
   kernel can close it in a later boolean, so the kernels then differ by
   a handle at no volume and no surface distance. A model that needs one
   topology there needs the corner off the face by more than the
   tolerance.
8. Native cut (optional fast path): clip triangles by the plane, cap each
   loop with the ear clipper, nested loops as holes; a vertex exactly on
   the plane counts as removed (the plane moves by -epsilon, as a subtract
   of the half-space), each vertex is classified once and each crossing
   edge gets one point so both faces agree, a face in the plane is removed
   and the cap covers it. It enters only after it passes the degenerate
   suite of section 16; until then the box lowering of 3.3 serves.
9. Parallel: with `KernelParams::threads` above 1 (0 = the pool), the
   crossing search (one triangle-BVH query per edge, no edge BVH; chunks
   of 256 edges) and the per-face assembly (one worker per 512 new edges
   of a side, chunks of an eighth of a worker's share; a side with fewer
   stays serial) run under `dag_parallelFor.h` through `parallel_chunks`.
   Each worker has its own output and scratch (`assemble_face` reuses a
   `FaceScratch`: a cut face makes about 15 allocations, and under
   dlmalloc's one lock an allocating parallel assembly is slower than
   serial); each chunk records its output range and the ranges join in
   loop order, so vertex ids and triangles come out as on one thread and
   the lowest failing face gives the error. The BVH builds, the merge, the
   windings, the new edges, the pairing and simplify stay serial.

## 7. Kernel "exact"

A global arrangement: a whole subtree is one pass, no intermediate mesh.
Coplanar and touching cases need no perturbation; the output triangles come
straight from the arrangement.

### 7.1 Number backends

The arrangement is written against an `ExactPoint` and a predicate set
(`orient2d` in a projection axis, `orient3d`, compare, equality, point
against plane, wedge order around a line). `KernelParams::backend` (the
console's `-backend:`) selects one of two;
int64grid is the default, shewchuk the no-snap option.

- `int64grid`: the leaf vertices snap to one integer grid over the bbox of
  all leaves, `scale = 2^gridBits / maxExtent`, `gridBits = 40`, the
  center at 0 (coordinates fit 41 bits), as `int64`; 40 is chosen so that
  `orient3d` on grid points fits int128. The scale is the largest power of
  two with `extent * scale <= 2^gridBits` and the center is rounded to a
  multiple of the step, so input on a power-of-two grid comes back
  exactly; the snap is the only approximation (about 1e-12 of the model
  size); a triangle that the snap makes degenerate, or whose exact normal
  turns against its double normal, is the error "model too small for the
  grid". Intersection points are exact rationals held as homogeneous
  integers (X, Y, Z, W): an LPI (edge against plane), a TPI (three
  planes), an SSI (two coplanar edges that cross where no transverse plane
  holds either edge, as the diagonal of a box face; built as the LPI of
  one edge with the plane through the other along the projection axis, so
  it has LPI widths and is stored as one). Bit widths from 41-bit
  coordinates, gcd not assumed:

  | quantity | bits | limbs |
  |---|---|---|
  | coordinate difference | 42 | 1 |
  | plane normal component / offset | 84 / 126 | 2 |
  | orient3d of four grid points | 126 | int128 |
  | LPI X / W | 169 / 127 | 3 |
  | TPI X / W | 295 / 253 | 5 |
  | point against plane, LPI / TPI | 255 / 380 | 4 / 6 |
  | point equality or compare, TPI vs TPI | 548 | 9 |
  | orient2d of three LPI / three TPI points (projection) | 466 / 844 | 8 / 14 |
  | wedge order with the supporting line (7.2.8) | 337 | 6 |

  `BigInt<N>` is templated per predicate with that table (N = 14 covers
  the arrangement; no predicate takes four constructed points, since the
  wedge order uses the supporting line and the triangulation is not
  Delaunay); a debug build asserts on a carry out of the top limb, and a
  test runs every predicate on all-maximal inputs against an
  arbitrary-width reference. The filter: each implicit point is computed
  once as a double triple with one absolute error bound (`max|approx| *
  2^-51`, zero for a grid point); a predicate evaluates on those in
  interval arithmetic, every operation widened by `|x| * 2^-52 + DBL_MIN`
  each way (at least one ulp, no libm call, sound under flush-to-zero);
  an interval that excludes zero decides, else the bigint path; no
  rounding-mode switch. The semi-static bound of Attene 2020 is a later
  refinement. The homogeneous numerators never enter a double.
- `shewchuk`: the leaf vertices stay as given (`gridBits` ignored).
  `ExactPoint`, `Plane` and `ExactDir` carry a backend flag over a union
  of the bigint and the expansion payload, and every construction and
  exact sign dispatches on it behind the shared interval filter, so the
  arrangement reads one interface. `expansion.h` is the library's own
  copy of Shewchuk's primitives with a sticky overflow flag the kernel
  reports (a result past its terms, or a product that can reach 2^1000
  or rounds to 0 from nonzero terms), without adaptive stages, since
  the filter takes almost every call.
  Constructed points and planes keep their coordinates in an arena of the
  arrangement. Planes have no gcd form: equal planes compare equal by
  their ratios behind a 1e-12 double prefilter; the projection axis and a
  direction's largest component are chosen by exact value as int64grid
  does, so both backends build the same triangulation. In front of the
  expansions: a double-double stage (hi, lo and a rounding bound stored
  with the expansion) decides point against plane, orient2d and compare
  to 2^-100 of the operands; for `orient2d` two key stages (`set_key`,
  the point's name within its arrangement: three points on the line of
  one leaf edge, or three constructed points of the group all built on
  one other plane, are a tie by their keys, so the caller passes the
  group it works in and no arithmetic runs) and a direct-mapped memo
  (2^16 slots keyed by the three points' kinds, keys and the dropped
  axis, emptied per arrangement), since the triangulation asks the same
  triples again and again; an unkeyed point (the ray seed, a test point)
  takes neither. The cost that remains is the model's: unsnapped doubles
  keep near misses at any depth, so mirrored and copied parts send about
  three times as many calls past the filter as on int64grid.
  `KernelStats` gives the time of each phase and one count of the exact
  calls of all phases, `exact fallbacks`.

### 7.2 Pipeline

1. Snap (int64grid) or copy (shewchuk) the leaf Solids; the leaf index is
   the component of the winding vector.
2. Planes: each triangle gets a canonical exact plane (an exact cross
   product divided by the gcd, sign-normalized; the face keeps its
   orientation sign against it); equal planes form a plane group.
3. Pairs: one global BVH over all leaf triangles; pairs of different plane
   groups are tested across leaves and, unless the leaf is convex, within
   one leaf for faces that share no edge, since the leaf validation of
   section 9 is tolerance-based and a leaf that passed it (an uncleaned
   boolean result as an instance) can still cross itself exactly, and an
   uncut crossing gives two triangulations a common edge without a
   constraint, or a winding conflict.
4. Pair intersection, exact: the sides of both triangles first; then, when
   each triangle has an apex off the other's plane, four orient3d signs of
   the apex edges decide (the sign of such a tetrahedron is the order of
   the two edges' plane crossings along the common line up to one common
   factor), so four equal nonzero signs return before any LPI is built,
   which most plane-crossing pairs do; the rest take the exact cut with
   leaf vertices or LPI points as endpoints. Coplanar pairs are not
   intersected pairwise: all triangles of one plane group form one 2D
   arrangement whose points are the group's leaf vertices, every LPI and
   TPI on the plane and the crossings of its constraints (TPIs), and
   whose constraints are every triangle edge and intersection segment in
   the plane.
5. Point registry: leaf vertices enter first and dedupe by exact
   coordinates; every implicit point is keyed by its construction (LPI:
   edge key and plane group; TPI: the sorted plane triple; SSI: the two
   edge keys) and deduplicated geometrically against every point in its
   rounded-triple bucket and the 26 neighbours (a bucket is at least twice
   the largest approximation error wide), so a point reached by two
   constructions gets one id; ids are assigned after a sort by key, never
   in arrival order, so the thread count cannot change them. Per triangle
   every pair of its constraints is intersected (`orient2d` on implicit
   points): a crossing makes a point, collinear overlaps merge, every
   segment splits at every point on it; then every registry point is
   inserted into every triangle whose closure contains it (a BVH query,
   then an exact test), a point on a leaf edge into both triangles of that
   edge by exact parameter, so no T-junction can form.
6. Per plane group one constrained triangulation (not Delaunay: insert the
   points, then recover each constraint by cutting out the triangles it
   crosses and ear clipping the polygon on each side, as flips deadlock at
   a fan over collinear vertices; `orient2d` in the dominant-axis
   projection, and `compare_coord` for the insert order, are the only
   predicates; a group of one goes through the same code). Each
   sub-triangle belongs to every input triangle of the group that
   contains it, decided on the constraint graph.
7. Stitch sub-triangles by point id. A sub-triangle of k input triangles is
   one face with a contribution vector of k entries (+1 or -1 per leaf by
   orientation against the group normal); its provenance for 5.4 is the
   first contributing leaf in tree order whose orientation matches the
   output side.
8. Shells: every edge records its supporting line (a leaf edge, or n1 x n2
   of two plane groups). Around an edge with direction d the faces sort by
   their wedge direction u = n_f x d signed into the face; two wedges
   order by sign(det(n1, n2, d)), and when that is zero the planes are
   equal and sign(n1 . n2) with the orientation signs decides same or
   opposite. Face sides linked across edges form shells; a cell is one
   outer shell plus the shells it contains. An edge with one face whose
   members face both ways is a crease (a sheet folded back on itself
   within the tolerance): its two sides join, as the wedge loop does for
   k = 0.
9. Winding vectors, one seeding rule for every surface component: pick a
   face of it and a dyadic point strictly inside its projection, cast the
   exact axis ray from there through the original leaf triangles, a
   signed count per leaf (+1 where the ray leaves the leaf, -1 where it
   enters, so nested outward shells wind past 1), with the symbolic tie
   rule on the projection and "beyond the face" from the plane
   coefficients; the cell on that side follows, the
   other side adds the face's contribution. No point of another leaf lies
   in the interior of an arrangement face, so the origin is never
   degenerate (a leaf vertex can lie on another leaf's surface, which is
   why the ray starts at no vertex). A walk through a face adds its
   contribution with the crossing's sign; a conflict between two walks is
   an internal error that returns false.
10. Classify each cell with the boolean tree: a leaf reads `w[leaf] > 0`.
11. Extract the faces between an "in" and an "out" cell, oriented towards
    the out cell; around an edge with 2k output faces pair across each
    "in" wedge, and duplicate a vertex or an edge that belongs to more
    than one fan, so the output is a closed manifold with duplicated
    vertices at touching contacts. The output pass (outputPass.cpp) makes
    that so at the tolerance, for the pinches the crossings of float
    instances leave a micron apart: every output vertex within the
    tolerance of another joins the lowest id of its cluster (a cluster
    stays within one tolerance of its root; no face-turn guard, since a
    face thinner than the move has no reliable normal); faces with a
    repeated vertex and opposite pairs with the same three ids drop (two
    faces with the same ids in the same order both stay: slivers of
    different leaves in different planes, dropping one unbalances the
    edge groups); a group of two halfedges pairs if opposite, a bigger
    group keeps each arrangement edge's exact wedge order and pairs
    in-sector within itself, and only a halfedge whose in-sector
    neighbour dropped pairs by the double angles with the nearest
    leftover of the other direction (a double order over the whole group
    is noise for sheets a micron apart); fans by union-find of corners, a
    vertex copy per fan; a copy edge with two pairs on the same copies
    swaps the two pairs' mates (the fans are oriented, so the swap splits
    a fan at both ends and never joins one); the rounds go on while a
    pinch remains, at most 32, then the error text. The provenance of a
    merged face is its first member (lowest leaf index) whose
    orientation, flipped by its Subtract parity, faces the out cell.
    Exactly-zero-area faces drop before the coordinates round to double
    (int64grid: divided by the scale).
12. The shared cleanup pass (section 8) and validation (section 9).

A subtree whose leaves exceed `maxArrangementTris` is split into batches
(10.8); a batch result is rounded, cleaned, validated and
self-intersection tested, then enters the next level as a leaf.

## 8. Cleanup pass

Shared by both kernels; every step keeps the mesh closed and verifies that
after itself. Tolerance from 5.1, thresholds from `CleanupParams`. Every
local operation is checked before it applies:
- An edge collapse needs the link condition (the one-rings of the two
  endpoints share exactly the two apex vertices of the edge's faces), no
  face normal may turn by more than 90 deg, no face with the same three
  vertices may result, and every edge keeps exactly two faces.
- An edit keeps the surface within `maxMoveFactor * tolerance`: over the
  fan (a collapse: the fan of the removed vertex without the two faces
  that go; a cap move: the apex fan plus the two new faces), the distance
  of the moved vertex from each old face's plane, or for a sliver or a
  face under the tolerance high, whose plane is noise, of the old
  centroid from the face's new shape; and the distance of the vertex's
  old position from the new fan. In a flat fan whose faces keep their
  normals the new faces tile the old ones, so a slide along a flat region
  costs nothing, a slide along a sphere
  costs only the sagitta, a move off the surface is bounded, and a slide
  across a groove opening is refused since not every face of the fan
  stays; the vertex displacement itself is not bounded (a 5 mm strip
  replaced by a ramp moves its vertex 5 mm while no point of the surface
  moves more than half the strip).
- An edit keeps the chart borders that `execute` gives it: an edge is a
  border when its faces come from two leaves, or from leaf faces of one
  leaf that share no vertex or carry corners of two classes at a shared
  one (5.4: a paint edge, a uv seam). A face keeps its leaf face's look,
  so a border corner slid along a flat strip would carry a painted
  square's colour onto its neighbour as a wedge, past any resolve. A move
  shifts the border through the vertex by its old and new distances from
  the chord of the vertex's two border edges, summed (at a junction or a
  border's end, by the whole move), within the same bound as the surface;
  a sliver flip across a border is refused (the cap rule flips only a
  face under the tolerance high, which moves a border less than a
  tolerance), and an overlap side of two charts is as one of two looks.
- The bound counts automatic tolerances (5.1): with a caller's tolerance
  above the automatic one, `execute` scales `maxMoveFactor` by their
  ratio, to no less than 2 of the caller's tolerances (a vertex cluster's
  width, 10.10). A caller's tolerance widens the kernels' coincidence, not
  the cleanup's reach: at 1 mm on a 2 m box the cleanup moves the surface
  2 mm at most, so a slab, a fin or a slot thicker than that keeps its
  volume and a thinner one may go; at the automatic tolerance a feature
  under `maxMoveFactor` tolerances is noise and may go.
- A fold check: after the move no face of the fan may leave its shared
  edge the same way as its neighbour across it, read from positions (a
  sliver's normal is noise); the cap rule tests its two new faces against
  the four outer neighbours too; an edit whose one-ring neighbour was
  touched earlier in the same sweep waits for the next sweep, since its
  pairing is stale; a third vertex within the tolerance of the edge line
  counts as no way (collinear thirds read as parallel from 1e-15
  residues).
- A crossing check closes what the fold check cannot see: an edit's new
  faces may not cross each other or a face near the path of the moved
  vertex, by the validator's triangle test at the tolerance (a face that
  shares only a vertex with the fan, or none, a wall 0.1 mm away, is
  crossed too; a face that crossed the old fan adds no crossing). The near
  faces come from a BVH of the mesh, built at the first check of a step
  and refit at the first check of each later sweep, queried with the box
  of the path grown by the move bound and the tolerance; before the
  triangle test a pair ends when one face lies beyond an edge plane of the
  other by more than the tolerance (its corners on that edge excepted),
  which ends most pairs at a tenth of the cost. The path box can miss a
  crossing with a far face of a tilted fan: a collapse moves its vertex by
  the length of the edge, and the fan tilts out to its far edges.
  `CleanupParams::wideCrossingCheck` queries the box of all the new faces
  instead, at 1.3 to 2 times the cleanup's time on the models, so
  `execute` sets it only on the rerun of a result whose validation found
  crossings (5.2 step 3).
- A step runs up to 128 sweeps (the overlap step 8, the merge step 1) and
  stops at the first sweep that fixes nothing: a chain of collinear faces
  along one line, each with a flat twin across its long edge, gives up one
  face per end per sweep.
- A volume guard closes the global gap a local check cannot see: the
  signed volume before and after a step may differ by at most
  `maxMoveFactor * tolerance * area` (the area before the step; the same
  factor as the local bound, so a legitimate sliver pass stays under it),
  else the step is rolled back from its copy, counted in `unfixed` and
  flagged in `CleanupStats::rolledBack`, and the pass goes on; a step that
  changes the genus of the shells is rolled back the same way
  (`genusRolledBack`), except the coincident step, since a skin on the
  surface has no genus to keep.
- Two vertices that share no edge are never merged, so the duplicated
  vertices of a touching contact (7.2.11) stay.
- Every new face takes the FaceInfo, leafRef and facePlaneKey of a face it
  replaces: an overlap cell face those of the side's group face under its
  centroid, a fan face those of the face it splits, the two faces of a cap
  fix those of the neighbour across the cap's long edge.

1. Weld: collapse every edge shorter than the tolerance that passes the
   checks.
2. Coincident faces: remove each pair of faces with the same three vertices
   and opposite orientation (a zero-volume skin that float arithmetic in
   the model can leave), then check closedness again. Overlaps: two
   coplanar regions of opposite orientation that overlap by more than the
   tolerance (the manifold kernel's tie rules keep both sides of a contact
   region, each triangulated its own way) are replaced by their symmetric
   difference: live faces are keyed by a plane cell (a normal grid and an
   offset grid with margins, so two faces closer than the margins share a
   cell; the grid finds pairs of faces from one source plane, and two
   narrow faces whose normals differ by more than the margin can be
   missed, since a face's normal deviates by up to 2 tol / height), a
   pair of opposite faces of one cell is an overlap when all six
   vertices lie within the tolerance of the larger face's plane and the
   overlap is deeper than the tolerance, union-find joins the pairs into
   plane groups, the rims of both sides form one planar graph (a rim
   vertex within the tolerance of the other rim's vertex is welded, one
   within the tolerance of its edge is inserted, collinear shared edges
   merge; a proper crossing of the two rims refuses the group, since the
   kernels put a vertex at each crossing of their own) whose cells are
   walked as cycles with windings from the rim directions: a cell inside
   both sides goes, a cell inside one side is ear-clipped as that side, a
   face outside the group across a rim edge takes the seam vertices as a
   fan; a pinch vertex or an open rim refuses the group.
3. Degenerate faces (height over the longest edge at most the tolerance):
   a needle (one short edge) collapses that edge; a cap (the foot of the
   apex farther than max(height, tolerance) from both ends of the long
   edge) flips its long edge when the apex lies on that edge within the
   tolerance, else splits the long edge at the apex projection, in both
   faces, and collapses the new short edge (the same as moving the apex
   onto the edge, so no new vertex); a face that no rule can remove stays
   and is counted. Every flip needs the dihedral between the two new faces
   below `flipDihedralDeg` (the cap's own normal is noise), and a cap fix
   must raise the minimum angle of the two faces, or two faces flip back
   and forth; when the chosen rule is refused, the other is tried.
4. Slivers: a face with a minimum angle below `sliverAngleDeg` or an aspect
   ratio above `maxAspect` (the usual sliver is a needle whose short edge
   is long against the tolerance but whose height is tiny: a cut close to
   a tessellation vertex): first a flip of its long edge, which moves no
   vertex, when the neighbour across it has the face's look and chart, the
   dihedral with it is below
   `flipDihedralDeg` on the two new faces, the apex lies within the
   tolerance of the neighbour's plane on the far side of the shared edge,
   and the flip raises the minimum angle of both faces and leaves no
   sliver (a flip from 0.41 to 0.45 deg is not a fix); else the needle or
   cap rule by its shape; a sliver along a crease (dihedral near 90 deg)
   takes the cap rule, which moves its apex onto the crease by at most the
   sliver's height; a sliver whose fix would move a vertex beyond
   `maxMoveFactor * tolerance` stays and is counted.
5. Coplanar merge: faces with the same `leaf` and `leafFace` (the pieces of
   one source triangle) with every vertex within the tolerance of the
   group plane merge into a polygon (the unshared edges chained into
   loops: one outer plus holes, or several outers without holes) and are
   triangulated again by the shared ear clipper; no vertex moves, interior
   vertices drop, a vertex on another group's faces lies on a boundary
   loop and stays; provenance, material and attributes are the group's.
   The group stays as it was when the new triangles do not raise its
   smallest angle (or keep it with fewer faces), at a pinch vertex where
   two boundary edges leave, with several outers and a hole, or when the
   ear clipper refuses (`CleanupStats::mergeRemoved`). It re-triangulates
   a boolean's fans, which removes the shading streaks of long slivers
   under smooth normals; merging across source triangles is not done,
   since a corner outside its source triangle would resolve wrong UV and
   color.
6. Counters: fixes per step (one collapsed edge, one removed skin pair, one
   fixed needle or cap each count 1), faces still defective at the end
   (`unfixed`: a skin face, an edge shorter than the tolerance, a face of
   height at most the tolerance, a sliver), minimum angle histogram before
   and after.

The pass works in sweeps whose edits touch disjoint faces, pairs again
what each sweep touched, compacts faces at the end of a step (so a face
keeps its index for the BVH through the sweeps), tests the shape of only
the faces touched since its last sweep in the degenerate and sliver steps
(an edit that moves or renames a vertex touches its whole fan, so no other
face changes its shape), and at the end removes unused vertices and
rebuilds halfedges, normals and the bbox; a Solid that needed no edit is
left bit for bit. Policy: a check that fails skips that operation, never
the verification; when a step still opened the mesh, the Solid is restored
from the copy taken before that step and the step counts 0 fixes; the pass
reports what it could not fix, and validation (9) reports those as
quality, not as an error. Not done: moving a vertex onto the analytic
surface of its primitive; the primitive mesh is the surface, and
intersection vertices lie on it already.

## 9. Validation and comparison

`validate(const Solid &, flags) -> ValidationReport`: three halfedges per
face (else the report is that count alone, read before any halfedge is),
finite coordinates, every halfedge
paired, consistent orientation, no duplicate face (the same three
vertices in the same orientation; an opposite pair is a zero-volume skin
that `quality` counts and cleanup step 2 removes), every face area above
zero, a `provenance` or `facePlaneKey` array of its Solid's size when
present (`misalignedArrays`, 3.2), one face fan per used vertex
(`extraFans`: shells that touch at a vertex index, which 3.2 duplicates),
each shell's own signed volume, an inward shell inside the other
shells (their winding about its first face's center above one half, else
`strayInwardShells` and invalid; a cavity stays valid), Euler
characteristic reported, optional
self-intersection test (BVH triangle pairs, skins excluded; mandatory for
the primitives of section 4 and for imports; when the coordinates pass
the float range, `auto_tolerance` fails, its error is the report's and
`selfIntersectionTested` stays false). The pair test is double
precision, never the engine's float
test (its absolute epsilon after a unit scaling reads a vertex
centimeters away as on a small triangle's plane): unit normals and the
5.1 tolerance; a vertex within the tolerance of the other plane is on it;
a pair in one plane goes to 2D separating axes, where an overlap at or
below the tolerance is separation; otherwise each triangle needs vertices
strictly on both sides of the other's plane and the two plane sections
must overlap by more than the tolerance along the common line; a pair
that shares an edge or a vertex takes the same test. Each face's plane is
computed once, the BVH walk is unsorted, and the closedness and
duplicate-face checks count their keys by the lowest vertex and sort each
vertex's few keys (the order of one sort of all keys at a fraction of the
cost). `ValidationReport::firstIntersections` keeps up to 8 flagged pairs
with their crossing segment; the console tool prints them. The same test
is the instancing gate of lowering, so a false positive there costs
instancing.

`compare(const Solid &a, const Solid &b, CompareParams) -> CompareReport`:
relative volume difference (default limit 1e-4), relative area difference
(1e-3), N sampled points (default 10000, drawn from `seed`) in the joint
bbox classified by generalized winding number against both, a
disagreement counted only when the point is farther than the tolerance
from both surfaces (`tolerance`, 0: the automatic one of the joint bbox),
the sampled
two-sided surface distance, and the disagreeing points in sample order
(`disagreementPoints`, side 0 inside a only, 1 inside b only) up to
`maxListedDisagreements` (default 4096; 0 allocates nothing), a separate
cap from `maxDisagreements`, the count `equal` allows. `quality(const
Solid &, const CleanupParams &) -> QualityReport`: triangle count,
minimum angle histogram, sliver count, degenerate faces, non-manifold
edges, duplicate vertices, skin faces (`skinFaces`, in pairs of the same
vertices and opposite orientation), shortest edge. These validate the
manifold kernel against the exact kernel.

## 10. Optimizer

Passes on the Graph before lowering; each is one function with a test, and
none is needed for correctness (lowering does the folding that correctness
needs).

1. Dead node removal (unreachable from the root).
2. Transform compose and prune: chained transforms compose into one node,
   an exact identity goes and a near identity stays (its translation is a
   length, a relative bound would need the root bbox before the fold, and
   a move with its inverse composes to an exact zero); a named transform
   is a part label and stays a node, and so do two that are not finite or
   compose past 2^300, for the checks of section 11 to name; the fold into
   leaves and the stop at a shared boolean subtree are lowering, 3.3.
3. UV affine merge: consecutive UV affines with the same `onlyUnset` on
   one subtree become one.
4. Hash-consing after the passes above dedupes equal subtrees.
5. Bbox pruning with conservative bboxes from primitive parameters (a
   leaf's grown by its reach, once; a disjoint `Intersect` keeps its
   first operand's box, as the lowering does): `Subtract` of disjoint
   boxes (expanded by the tolerance) gives `a`;
   `Intersect` gives `Empty`; `Union` gives `Concat`. Before the prune,
   over the user's graph (a fold or a merge would hide an operand), every
   node it reaches runs its value checks (a transform, a cut plane, an
   attribute, a leaf's bbox and reach; section 11); then the lowering's
   frame checks run on its own walk, which takes a shared subtree once and
   no operand that an Empty discards, from the identity (a composition,
   a singular frame "...: the transform is singular", the cut plane in its
   frame, an attribute's inverse, a leaf's float world_tm; section 11);
   then every primitive runs its own parameter checks (the tessellation at
   its own `seg` and `segMinor`, or the fewest segments for an automatic
   count; an import checks its path and reads no file). Without the
   optimizer the lowering runs the three on the same walk before it
   tessellates, so the first invalid node is the error, with its name,
   either way.
6. Union flattening: nested unions become n-ary; the children sort by the
   Morton code of their bbox center and pair into a balanced tree, so near
   parts merge first; children disjoint from all others concat at the end.
   Each pair keeps its earlier source child on the left, but the pairing
   sets the coincident-face winner (5.4), not the source order.
   Subtract chains `(a - b) - c` become `a - (b | c)` (disjoint b and c give
   a free Concat and one boolean with a instead of two).
7. Convex fast path: the optimizer marks the top `Intersect` or `Cut` of
   each chain over convex primitives; when the lowering leaves that node
   it clips the chain in double (each leaf face clipped by the other
   leaves' planes, a weld, the T-junctions split, triangulated), validates
   the result like any leaf and puts one leaf in the tree; the chain's
   leaves stay in the tree's list, so the resolve follows a clipped face's
   own leaf reference one step. A clip costs about half of the booleans
   it replaces. The clip refuses, and the kernel evaluates the chain, when
   the coordinates exceed `tolerance * 1e15`, the bound vertex clustering
   (item 10) uses too: both key a weld grid by `floor(p / tolerance)` in
   int64, which overflows past it.
8. Exact kernel batching by the triangle budget (section 7). Triangle
   counts exist after tessellation, so the executor batches the lowered
   tree at the end of the lowering (its time counted as kernel time; both
   kernels and the fallback's rerun see the same batched tree), children
   first, for the exact kernel only: a region is the leaf triangles under
   a boolean node (not a Concat), a batch leaf counts by its result size;
   in a region above the budget, the bigger operand that is not a leaf and
   whose own kernel call fits (a Concat operand by its largest boolean
   region) is evaluated alone, then the other while the region is still
   above the budget; a batch keeps the lowered tree's leaf numbers, so
   the kernel's references and a leaf's own name the same leaves; a batch
   result is rounded, cleaned, validated with the self-intersection test,
   and put back as one leaf; a batch whose test finds crossings after a
   cleanup runs once more with the wide crossing check (section 8), and a
   batch that still does not validate is undone with the warning "node
   <name>: a batch of <n> triangles does not validate: <text>; its region
   is evaluated unbatched", which names the graph node of the batched
   region, as its errors do; an exact result
   usually has more triangles than its leaves, so a batch is undone when
   its result is not smaller; a region no batch fits stays the kernel's
   refusal, for the fallback. `maxArrangementTris` defaults to 1,000,000
   from about 2 KB per arrangement triangle against section 13's 2 GB;
   the console's `-max-tris:<n>` sets it and prints the largest region,
   the batches and the peak memory (`ExecuteStats::largestRegion`).
9. Order for quality (pairwise kernel): no rule beyond the Morton order of
   item 6. Bbox volume ascending and largest overlap first were measured
   by the slivers left after cleanup on the three models: no order wins
   everywhere and Morton is best on two of three; the console's
   `-validate` prints the quality counts that make the measure.
10. Vertex clustering (in the lowering, after the leaves are lowered and
    before the kernel call, `ExecuteParams::clusterVertices`, default on):
    the leaf vertices of one kernel call's set (one boolean subtree; each
    operand of a top Concat is its own set; a set of one leaf is skipped)
    within the tolerance of each other become one point, so contacts that
    float arithmetic in the model (an instance through an inexact
    rotation) left 1e-7 apart become exact ties for both kernels.
    Union-find over a grid hash, the closest pairs first; a cluster is
    never wider than 2 tolerances and never holds two points of one leaf;
    every member takes the position of the member with the lowest (leaf,
    vertex) index, never a centroid, so the first copy does not move; a
    leaf whose face would go flat or turn keeps its positions (the exact
    kernel refuses a flat input face); coordinates above `tolerance *
    1e15` skip the clustering (item 7).

## 11. Precision, floating point and NaN policy

- Every input float is checked finite (`check_finite`, dag_check_nan.h;
  dagor builds with fast math, so `isnan` folds away). No code names a
  NaN or an infinity or relies on their IEEE comparisons, and a division
  takes its zero branch before it divides. A value that could overflow
  is tested before it is made: an expansion product by its operands'
  exponents, a double-double one by those of its values and error
  bounds (undecided, for the expansions, when far), the product of the
  two execute qualities ("the quality product %g * %g must round
  to a finite float above zero"), a float uv or uv affine by its value
  in double ("a uv projection or a uv affine gives a uv beyond the float
  range"). Every entry of a transform, of a composition of transforms and
  of an attribute's inverse (none is made under an operand that an Empty
  discards), and every value of a cut plane, is finite and within 2^300,
  else execute fails:
  "<node>: the transform is not finite or has an entry past 2^300",
  "...: the transform composed with the ones above it has an entry past
  2^300", "...: the cut plane is not finite or has a value past 2^300",
  "...: the transform above it has no inverse within 2^300". So a
  determinant, a cofactor, an axis' square and a transformed vertex are
  finite with no test. What transforms make level by level, a node box
  and a leaf's reach, is tested by its exponents against 2^500, so a
  value a little under 2^500 can fail too: then a box is unknown in the
  optimizer and an error in the lowering ("...: the transform takes the
  box or the reach below it past 2^500"), and a sag scale stays at
  2^500, where a primitive takes its most segments anyway. A leaf
  source's bbox and reach are checked finite ("<node>: <source>: the
  bbox or the reach is not finite"), and its float world_tm is tested
  against FLT_MAX before it narrows ("...: the world transform passes
  the float range"); an entry that narrows under 2^-126 is zero, as FTZ
  and DAZ read it, and a float world_tm whose exact determinant is zero
  is an error ("...: the float world transform is singular"). A leaf
  frame that is not a similarity, or whose scale squares to zero, is
  tested in double first, so one whose double determinant is zero is
  "...: the transform is singular". An attribute's color is checked
  finite ("<node>: the color is not finite"), its uv affine too ("...:
  the uv affine is not finite"), a projection's scale above zero,
  UvSphere's aside ("...: the uv scale must be above zero"), and its
  axis not zero ("...: the projection axis is zero"). The cut tests
  1 / det, 1 / len and d / len by their exponents ("...: the cut plane
  is degenerate"). Not tested before it is made: a primitive's float
  corners, which the guard after every builder tests (section 4,
  `fits_float`), and a derived daSurf net, which daSurf bounds by 2^450
  and tests after, by its spec 8.
- The repo builds with fast math (`VCMathOption ?= -fp:fast` in
  `prog/_jBuild/defaults.jam`, `-ffast-math` in the android, iOS, linux,
  macOS, nswitch, ps4, ps5 and tvOS clang sets and the linux gcc set,
  every set but the Windows and Xbox ones). Both kernels need IEEE
  semantics: the manifold kernel's consistency holds only when the same
  expression compiles to the same instructions at every call site (no
  contraction into FMA at one site and not another, no reassociation);
  the interval filter and the Shewchuk expansions need correctly rounded
  single operations. `fp-options.jam` therefore adds `-fp:precise` to
  `CPPopt` on the MSVC platforms, and
  `_jBuild/_vc/add_fp_fast.jam` adds the `VCMathOption` default
  (`-fp:fast`) only when no `-fp:` option is present, so the compiler
  line carries `-fp:precise` alone while `VCMathOption` and
  `StrictAliasing` keep their defaults; for gcc and clang it adds
  `-fno-unsafe-math-optimizations -fno-trapping-math -ffp-contract=off`,
  `-fno-finite-math-only`, which the code does not need but which
  keeps one result on every target, and `-fsigned-zeros`, which
  `-fno-unsafe-math-optimizations` already restores but the code needs:
  a vertex normal, a resolved attribute and the instance key of a
  transform drop a negative zero by adding a zero, and under
  `-fno-signed-zeros` every float form of that folds away (of
  `-ffast-math`, only `-fno-math-errno` and the limited complex range
  stay; they change no result), `-mno-fma` on x86, with the `/clang:`
  prefix on the Windows clang build, and on every platform
  `-DVECMATH_NO_FMA`, since `v_madd` on NEON is an explicit `vfmaq` that
  `-ffp-contract=off` does not stop.
  `daCSG-options.jam` includes it after `defaults.jam` for every
  daCSG layer and daSurf's jamfile includes it directly, so the flags
  live once. Every predicate is a non-inline
  function in a .cpp file of the lib, never in a public header, so an exe
  built with fast math cannot recompile it. The unittest exe checks at
  start that a predicate call rounds `1.0 + 2^-53` as IEEE demands, that
  `two_sum(1e16, 1.0)` has a tail of exactly 1.0 and that a NaN compares
  unequal to itself (a smoke test that the flags reached the lib; the
  code does not need the last, and passes every other test without it).
- Internals are double; outputs are float; the result is rounded to float
  before cleanup (5.2) so the float mesh validates.
- No unguarded division: a plane from a degenerate triangle, a zero-length
  edge, a zero-area projection each take the degenerate branch.
- Tolerance is relative to the root bbox with a float-ulp floor (5.1); a
  2 m box centered at the origin gets about 9.5e-7 m, the eight ulps at
  1 m, above 1e-7 of its 3.46 m diagonal.

## 12. Threading

`dag_parallelFor.h` over the engine thread pool; the console starts a pool
of the logical core count, `-threads:<n>` sets it and 1 starts none. A
pool is optional: with no pool, threads = 1, or a build with
`daParallel = no ;` (section 2) every loop runs on the caller. One
ThreadBudget per execute call (`KernelParams::budget`) holds the call's
threads, the caller's included, and every parallel loop leases its extra
workers from it for its run, so the booleans of one height and the loops
inside them never run more threads at once than the call has
(test_manifold checks the peak count). Any thread count gives a bitwise
equal Mesh: the threads checks compare the Mesh on 1 and 4 threads bit
for bit, test_manifold's over the suite, the fuzz seeds and the
regression seeds, test_exact's over the suite and the regression seeds on
both backends. A parallel loop body does not allocate per item: an
allocation costs more than the work of most items, and dlmalloc (the
engine's default allocator on Windows) has one lock; the daCSG exes use
mimalloc. The manifold kernel's parallel phases are in
6.9. `IPairwiseKernel::evaluate` walks the tree by height: the nodes of
one height have no operand in common, so for a kernel whose
`concurrentBooleans()` is true (the manifold kernel) it runs
them on the pool, the biggest first; each boolean keeps the thread count
of the call and its loops queue jobs on the same pool, so a worker that
waits, or has no boolean left, helps the others; each node has its own
counters and error, joined in node order. The exact kernel runs two
phases on the pool, with workers sized by the work (one per 4096 pairs or
constraints, the caller always): the pair search in chunks of 1024 BVH
pairs, a worker with its own candidates, segments and arena, joined in
loop order with the references rebased and the arenas adopted; and the
per-group triangulation in chunks of about 1/32 of a worker's share of
the constraints, a worker reusing one CDT workspace and appending to flat
vectors with a per-group record, joined in group order, the first failed
group in order giving the error. The predicate counters and the Shewchuk
overflow flag are taken per chunk on the worker and joined to the caller
after each phase; the orient2d memo is a triangulation worker's scratch of
one run. The registry, the crossings, the insert, the cells, the ray and
the output pass are serial. `daParallel/parallel_api.h` holds the chunking and the
worker count for both kernels; `ThreadBudget` and `WorkerLease` are in
`kernel_api.h`, so a client kernel leases its workers from the call's
budget as the library's do.

## 13. Performance targets

Measured on a desktop CPU, dev build with optimization, one thread unless
stated. Besides these absolute targets, the bench of section 16 keeps
every kernel within the margins of its recorded rows, and the library
under its line limits.

| Case | manifold | exact |
|---|---|---|
| one boolean, 10k x 10k triangles, 1 thread | < 20 ms | < 200 ms |
| sport bike (about 300 prims, 150k output tris), 1 thread | < 3 s | < 20 s |
| same, thread pool | < 1 s | < 8 s |
| peak memory, sport bike | < 1 GB | < 2 GB |

The one-thread bike row for "manifold" is the tight one: a balanced union
tree visits every triangle once per level (about 9 levels of 150k to 300k
triangles), so it holds only with Concat for disjoint parts and with
instancing of shared sub-assemblies (3.3).

## 14. das module `csg`

- C++ module `csg` in `daCSG/das/` binds a flat function set: one builder
  per primitive (a round one with `seg : int = 0`, 0 = from its size;
  `csg_icosphere` takes r and level only), `csg_op(kind, a, b)`, `csg_cut`,
  `csg_transform`, the `csg_attr_*` set, `csg_name`, `csg_error`,
  `csg_set_quality`, each over `int` node ids and the Graph of the model's
  `dacsg::ModelContext`, which the host makes. They turn private
  to the module once `csg.das` (compiled as a program of its own)
  has bound them, so a model sees the DSL of `dsl.md`
  and these public names besides it: `cos_sin` and `turned` of the prelude,
  which the rotation of daSurf's `surf.das` calls; the `id` field
  of `Solid`, which `surf.das` sets on a Solid it makes; and the constants
  `CSG_UNION`, `CSG_SUBTRACT` and `CSG_INTERSECT`, as that step makes only
  functions private (without the private `csg_op` they make nothing).
  Each node-making binding takes the daslang context and records
  the model's call line in the node (`Graph::setSource`; a stack walk to
  the first frame outside csg.das, one short walk per node; a node made
  inside an inlined one-line helper reports the helper's call line), so
  errors, the stats table and the part labels read `<type>#<id>
  (<file>:<line>)`; `Graph::ownName` is the `.name()` or null,
  `Graph::autoName` the `<type>#<id>` form.
- The prelude `csg.das` (stringified into the module like
  `dasModules/common/dagorMath.das`) defines `struct Solid { id : int }`,
  the operators `|`, `-`, `&`, `|=`, `-=`, `&=`, and the dot-call methods
  of `dsl.md`. The DSL surface is daslang code, so a change there needs no
  C++. `color` takes `uint` (a hex literal is `uint` in daslang).
- `set_quality(q)` in the model sets the graph's quality; the host's
  `ExecuteParams.quality` multiplies it. A host compiles the model file
  with `csg` required, calls `[export] def model() : Solid`
  in a `dacsg::ModelContext` that it makes, then `execute` on the C++ side
  with the context's graph; a `Node` builds through daPCG's `build_scene`.
- `dsl.md` is all that a `Solid` model writer needs (a `surf` model adds
  daSurf's surf.md, a `Node` model daPCG's scene.md, a material table its
  material.md); it stays under 4800 bytes counted with LF line ends (with
  surf.md under 7800 together, daSurf spec 7; scene.md under 2000,
  material.md under 1800) and is kept in sync with `csg.das` by
  `sheet_sync.py`, which compiles every signature in the sheet on the
  console tool (built from the repo daScript, not the shipped das exe).

## 15. Tools

### 15.1 dacsg (console)

Built like `prog/1stPartyLibs/daScript/utils/daScript` (dagor console exe
with the daslang runtime and the `csg`, `surf` and daPCG `scene` and
`material` modules).
It writes no image: a picture of the result is `dacsg <model.das> -o:x.dag`
then `dagrender x.dag -png:x.png` (15.3).

```
dacsg <model.das> [-o:<out.dag>] [-obj:<out.obj>] [-kernel:<name>]
      [-quality:<f>] [-validate] [-check] [-compare:<k1,k2>]
      [-cleanup:on|off] [-optimize:on|off] [-cluster:on|off]
      [-max-tris:<n>] [-backend:int64grid|shewchuk] [-threads:<n>]
      [-fit:<ref.obj|ref.dag>] [-fit-min:<percent>] [-fit-dots:<file>]
      [-nodes] [-quiet]
```

- `model()` returns a `Solid`, or a `Node`: a tree of named nodes, each
  executed in its own pivot frame (daPCG
  [scene nodes proposal](../../daPCG/_doc/scene_nodes_proposal.md),
  [scene.md](../../daPCG/_doc/scene.md)). For a `Node`, `-check` prints a
  line per node, `-validate` and `-compare` run per mesh node, `-fit` and
  the `.obj` take the meshes joined in model space, and the `.dag` keeps
  the tree: local tms, the node scripts, no flags on a node with no mesh.
  `-nodes` with `-o` reads the `.dag` back and prints a line per node
  (parent, faces, hex NODEFLG_* flags, local tm, `|`-joined script lines).
- `.obj` with a `.mtl` that holds one material per distinct color (and
  `usemtl` groups). `.dag` through `DagExporter` with the result's normals
  and a material list on the node, one slot per mat id the faces use, in
  id order, named by the id: `paint`, `chrome`, `matte`, `glass`,
  `metallic` (class "simple", the diffuse color the renderer scales the
  base color by; an id above 4 gets `mat<id>`), so a sparse
  `mat(65535)` is one record, the dag opens with its materials in every
  tool, dagrender shades by the name and an import maps the name back to
  the id. A row of the model's daPCG `material` table (material.md) is the
  material of its id: its name, class, script lines with a `look=<name>`
  line last, texture asset names by slot (as given, not made relative to
  the `.dag`), two-sided flag and diffuse.
- `-compare` runs two kernels, each without the fallback, and prints the
  `compare` report; exit code 1 on failure. `-kernel` runs with the
  fallback, with a warning line after the phases line. `-cleanup:off`,
  `-optimize:off` and `-cluster:off` are diagnosis switches for the passes
  of sections 5 and 10.
- `-fit` reads a reference mesh (obj or dag) and runs `compare` with 2000
  samples; it prints the sampled agreement, the volumes, the surface
  distance and both bboxes, and exits 1 below `-fit-min` percent (default
  99). `-fit-dots:<file>` writes the disagreeing samples as dagrender
  `-dots` lines: red inside ours only, blue inside the reference only.
- `-check` compiles the model and runs `model()` only, prints the csg
  graph's node count and a line per material row, and executes nothing.
- Prints the stats table (per node time, triangles, cleanup counts) and,
  with `-validate`, the validation (self-intersection test included; an
  invalid result exits 1) and quality reports of the result; each flagged
  face pair names its parts, `faces 141
  (part "frame", convex#5 (sportbike.das:123)) x 662 (part "tank", ...)`:
  the part is the innermost `.name()` above the face's primitive (one
  graph walk from the root; a shared node takes its first path).
- A number out of its option's range or type, with a sign or a space
  before it or text after it, is an error with exit code 1: `-quality`
  finite above zero, `-fit-min` 0..100, `-threads` 1..256 (each pool
  worker takes a stack), `-max-tris` 1..4294967295. So is an empty path or
  one the tool cannot make absolute (on Windows, past `DAGOR_MAX_PATH`),
  and a `-kernel` or `-compare` name the build lacks, also under `-check`.

### 15.2 dacsgView (GUI)

Not in the tree yet. Its design: an exe on the testBed framework
(`prog/samples/testBed`, `commonFramework`) that takes `<model.das>
[-kernel:<name>]`, polls the file time each second and rebuilds on change,
shows a build error in the window and in the log; render modes lit (sun +
ambient, vertex color, the library normals), checker UV, provenance (a
color per primitive, the name under the cursor) and a wireframe overlay;
a stats overlay; console commands `csg.reload`, `csg.kernel`, `csg.mode`,
`csg.quality`, `screenshot.create`, `camera.at`; an orbit camera; one
dshl shader. Automated runs pass
`-config:directx/winOcclusionCheckEnabled:b=no` so a covered window still
draws, and `-quiet`.

### 15.3 dagrender (dag rasterizer)

A dag utility in `prog/tools/dagUtils/dagRender` with no dependency on
the CSG library: it reads a `.dag` with libTools dagFileRW, every
renderable mesh node with its transform, into one engine Mesh, and
renders it in software to a png (z-buffer, the dag's vertex colors,
else 0.8 gray, times the face material's diffuse, the dag's normals,
else the smoothing groups; a sun, a hemisphere ambient, a Blinn-Phong
highlight and a fresnel reflection of a procedural or `-env` equirect
environment; the ground plane with the model's shadow; materials by the
last `look=<name>` line of the material script, else by the dag material
name: paint, chrome, matte, glass, metallic, other names as paint; `-ss`
supersampling). Exit code 1 on an error.

```
dagrender <in.dag> -png:<out.png> [-view:<yaw>,<pitch>,<dist>]
          [-size:<w>x<h>] [-ref:<image>[,<alpha>]]
          [-ortho:<width>[,<cx>,<cy>]] [-grid] [-bg:<r>,<g>,<b>]
          [-env:<image>[,<yaw>]] [-ss:<n>] [-ref-diff:<out.png>]
          [-dots:<file>] [-pose:<node>,<x|y|z>,<deg>] [-quiet]
```

- `-pose` (repeatable) turns a node and its children rigidly about the
  node's own axis before the world tms, by the right-hand rule as csg
  `rot`; a node the dag lacks is an error.
- `-view` (default 40,25,0; dist 0 keeps the eye at the bbox sphere's
  distance and fills 90 % of the binding axis with the projected bbox
  corners, centered; pitch -90 to 90, the right axis at the poles from the
  yaw, so a top view keeps x and z with no tilt); `-size` default
  1024x768; with `-ref` it keeps the photo's aspect to a pixel.
- `-ref` puts the png over a photo (the blueprint workflow): the image
  gives the output size unless `-size` is given, the render blends over it
  with alpha (default 0.5), and the view is orthographic. `-ortho` makes
  the png orthographic, `width` meters across, centered at (cx, cy) on the
  view's right and up axes (default the bbox center and a width that fits
  the bbox), so a known dimension in a photo calibrates the frame; `-view`
  sets the direction (yaw 90, pitch 0 is the side view from +X). `-grid`
  draws 0.1 m lines on the ground plane, whole meters darker, for a model
  under 500 m within 100 km of the origin (past that a float holds no
  10 cm line: a printed note, no grid). A photo pixel
  lies over `-bg` by its own alpha (linear light) before the render blends
  over it.
- `-env:<image>[,<yaw>]`: from 8x4 up; the sun is the brightest texel of
  the upper half of the reflection level with 8 rows (4 a hemisphere, past
  its blur), else of the sharp image, its height clamped to 8 degrees; the
  ground is the mean of the sharp image's lower half, weighted by the
  rows' solid angle; a line prints both. The sharp image keeps 2 rows at
  least, so a wide short sky still has a row a hemisphere. Every level is
  a box filter that keeps a side's remainder: of n texels into m cells,
  texel x lies in cell x * m / n, each cell the mean of its own texels.
- An image dagrender reads (`-ref`, `-env`) holds at most 32767 pixels a
  side, the image loader's limit; a larger one is the named error, and
  every pixel index is taken in 64 bits.
- `-ref-diff:<out.png>` with the `-ref` frame compares the model's
  coverage mask with the photo's mask (its alpha channel when a pixel is
  not opaque, else "not within 8/255 of the corners' background color": an
  RGBA export whose alpha is all 255, as editors write opaque images,
  carries no silhouette in it), writes the diff (red model
  only, blue photo only) and prints the symmetric difference in square
  meters and as a percent of the photo mask, and the largest outline gap
  in meters in both directions (an exact distance transform of each
  outline). `-dots:<file>`: one dot per line, `x y z r g b`, each 3x3
  pixels over the image; dacsg's `-fit-dots` writes them.
- A number out of its option's range or type, with a space or a `+`
  before it or text after it, is an error with exit code 1 (a `-` is the
  number's own, which the range may refuse): `-size` 1..8192 each way,
  `-ss` 1..8, `-view` finite with dist 0 or more, `-ortho` width above
  zero and a finite center, `-bg` 0..1, the `-ref` alpha 0..1, the
  `-env` yaw finite. So is an empty path or one the tool cannot make
  absolute, as for dacsg. The comma tail of `-ref` or `-env` is that
  number when it reads wholly as one, `nan` and `inf` too, else part of
  the path.

## 16. Tests and benchmark

`unittests/daCSG` is a console exe with the `CHECK` macro style of
`unittests/sdfBlas`, one `test_*.cpp` per area, and a `-bench` mode.

- Numbers: `BigInt` against a second, naive implementation on random
  operands; every predicate on all-maximal inputs with an overflow check;
  interval filter against exact on random and tie cases; the IEEE rounding
  smoke test of section 11; the shewchuk backend against `int64grid`: the
  predicates on the suite's vertices, grid ties and nearest misses against
  a six-limb reference, and the boolean suite, the temple column and the
  sport bike parts on both backends with every leaf vertex on the grid
  (the snap the identity): equal shells, cells, faces, point counts,
  volumes within 1e-9.
- Primitives: every primitive at several `seg` values validates; volume
  within the tessellation bound of the analytic volume; Euler
  characteristic (sphere 2, torus 0); every channel has the right size; UV
  in range; a self-intersecting leaf (torus with r >= R) is an error with
  the node name in both kernels; a mirrored leaf and a mirrored subtree
  validate with positive signed volume; `rot_x(90)` maps +Y to +Z.
- Booleans: a canonical suite x {union, subtract, intersect, cut} x
  {manifold, exact}; each case lists its expected volume, area,
  shell count and Euler characteristic (box cases exact, curved cases from
  the tessellation). The degenerate cases: two boxes sharing a face (no
  face inside the union); a box minus a flush box (no face at the shared
  height); a box minus the same box (an empty result that validates); box
  and box identical; vertex on a face; edge on an edge; touching at one
  point; partial coplanar overlap; a sphere inside a box (union the box,
  subtract a hollow box); two nested spheres in a box; a box with a
  cavity; disjoint; tangent spheres; a slab subtract that leaves zero
  thickness; a cut exactly at a box face from both sides; two boxes whose
  bboxes touch exactly on a face (not concatenated); the suite translated
  by (1000, 0, 0). Each result validates and matches its expected values;
  the manifold and exact results agree by `compare` and per face by leaf,
  at a contact of two leaves too.
- Attributes: the UV at an intersection vertex of a cylinder cut by a box
  equals the cylinder's analytic UV at that position within 1e-5; the
  cavity walls of a subtract carry the cutter's color; `attrSet` survives;
  `rgb_uncolored` after a union keeps the colored faces' color; an Attr
  above a Subtract gives the subtree-level normal to a projection;
  touching parts of two colors give the first operand's color on the
  shared face in both kernels (equal boxes; a box that touches another
  from outside or from inside, with the face normal to x, y and z); a
  box exported to a dag Mesh has outward engine face normals; a crease
  between two smooth parts stays hard in the output normals.
- io: `test_obj` writes a box and a sphere and reads back their positions,
  uv and smoothing groups (normals and colours are written, as vn and one
  material per colour, and not read back), reads every statement of the
  grammar, refuses each malformed token, a smoothing group above 32, an
  index out of range (a vn index past the vn count too) and a line or
  statement over 16 MiB with the line number, and through the read seam
  reads the file in small chunks and fails a read cut short with an
  empty Mesh; `test_dagMesh` round-trips boxes and smooth regions through
  the engine Mesh closed with flat faces at smgr 0, refuses a face index
  out of range, and keeps the empty cases and a high-valence fan within a
  linear bound.
- Fuzz: seeded random trees of random primitives, 1000 per kernel; half of
  the trees put positions and sizes on a 1/8 m grid, rotations at
  multiples of 90 deg and mirrored copies, so contacts and coplanar
  overlaps are frequent. No crash; every result validates or the kernel
  returns an error text, with a budget: exact 0 errors (except "model too
  small for the grid"), manifold at most 0.5 %; each error is logged with
  its seed; the kernels agree.
- Determinism: the same model with `threads = 1` and the pool gives a
  bitwise equal Mesh. Exact batching: a tree above `maxArrangementTris`
  (set to 1000 for the test) gives the same `compare` result as unbatched.
  Instancing: the temple's column subtree evaluates once (stats).
- Quality: after cleanup no sliver whose height exceeds `maxMoveFactor *
  tolerance` remains, thinner leftovers are counted; triangle count at most
  4x the inputs; the float Mesh validates after rounding.
- Optimizer: each pass has a before/after test on a small graph and an
  "evaluates to the same solid" check. Sheet sync: every signature in
  `dsl.md`, `surf.md`, daPCG's `scene.md` and `material.md` compiles on the
  console tool, and the sheets stay under their byte caps (section 14).
- `-bench`: one row per case and kernel (manifold and exact, one thread):
  ms, peak KB, triangles, slivers, degenerate faces, non-manifold edges,
  duplicate vertices, closed; as text and CSV. Manifold is the reference
  column, and exact's ratios to it are printed as information. The driver
  runs each (case, kernel) in a child process of the same exe
  (`-bench:<case>:<kernel>`) and reads time and the OS peak commit above
  the commit at the start of the run from its output, so the peak is per
  run (mimalloc keeps no peak of its own; `MIMALLOC_EAGER_COMMIT=0` makes
  a child commit on demand, not a 32 MB segment up front; a Linux child
  prints its lifetime peak RSS in that column instead, information only,
  since the memory gate reads the csv's Windows machine); ms is the best
  of up to 10 runs within about 200 ms. A kernel's row times `evaluate`
  plus the cleanup pass and measures its quality counts after cleanup
  (what a user pays and gets). The reference is `bench_baseline.csv`, the
  kernels' rows recorded on one machine, which its first line names;
  `-bench:record` rewrites it, and a record is committed with its reason.
  The gates, this section their only home, per case and kernel against
  its own recorded row: the topology counts at or below the row's and
  closed where the row is; time within `baseline * (1 + TIME_MARGIN) +
  TIME_FLOOR_MS` and peak memory within `baseline * (1 + MEMORY_MARGIN) +
  MEMORY_FLOOR_KB` (the constants of bench.cpp; the floors absorb the
  drift of a small case over days), both gated only on the recording machine
  and printed elsewhere; a missing row is a red cell. The size gates: the
  library built with any one kernel (the root `fpProbe.cpp` and `*_api.h`
  headers, mesh, graph, io, prims, validate, cleanup, exec,
  kernels and that kernel; not the other kernel, the das module, the
  tools or the tests), in non-blank non-comment lines counted by
  `loc.py`, stays under its limit, printed with the table: 11800 for
  manifold, half of carve's 23.6k; 13625 for exact, whose two number
  backends write every formula twice; and the DSL sheet `dsl.md` under
  4800 bytes (section 14). Any red cell, or a build over its limit, fails
  the bench. The cases live in `unittests/daCSG/boolean_cases.h` with
  their expected values, shared with the kernel tests.
- Models: `temple.das`, `sportbike.das` and `chopper.das` build with both
  kernels inside the budgets of sections 13 and 17. CI (`_tests/tests.py`):
  `dacsg_unit_tests` runs the tests on Windows (and `loc.py`) and Linux;
  `dacsg_console` (Windows) builds the console and dagrender and runs
  `sheet_sync.py` in one step that deletes both binaries also when it
  fails; it also checks each model's triangles out on both kernels (each
  bike in 200k), both tools' option ranges and paths, the materials of a
  two-node `.dag` through the console and a sparse `mat(65535)`, the
  material table of `test_materials.das` in its `.dag`, a `-fit`
  of a result against its own `.obj`, and dagrender's use of a material's
  diffuse, a photo overlay with its silhouette diff and every image at its
  extremes; `test_nodes.das` checks a `Node` tree read back by `-nodes`
  and a dagrender `-pose`.
- Kernel sets: `unittests/daCSG/kernelSets/`, two exes one jam run builds
  (the second through `Dependencies`), each listing its own set; each
  prints the kernels it registers: `manifold`, and `manifold exact`.

The cross-kernel test runs every suite case and every fuzz tree on both
kernels, which must agree by `compare` and per face by leaf: a face fails
when it lies on no face of the other result or names a leaf with no face
at that point; a suite case also fails on a face that names the other
leaf at a contact of two leaves.

## 17. Sample models

Four of the models in `prog/tools/daCSG/models/`, three with no surf DSL:

- `temple.das`: a Doric peripteral temple (fluted columns, triglyph rows,
  pediments, a hollow cella); the benchmark model.
- `sportbike.das`: a sport bike (full fairing, clip-on bars, twin
  headlights, four-cylinder engine block, underseat or side exhaust, cast
  wheels), red and black livery as its reference photo has it, a white
  plate; the bodywork as hulls of sampled sections. The performance
  reference of section 13.
- `sportbike_surf.das`: the same bike with the bodywork as daSurf lofts
  through `section` curves, capped, the decals painted, the screen
  thickened (`require surf`); the daSurf showcase, with the same fit to
  the photo; its triangle budget is daSurf's spec, section 9. Both bikes
  require `sportbike_parts.das` and return `bike()`, a daPCG `scene` tree.
- `chopper.das`: a chopper (raked fork, teardrop tank, V-twin with fins,
  fishtail pipes, spoked front wheel, wide rear), black and chrome.

Acceptance for each bike: at most 200k triangles, builds inside the section
13 times with the manifold kernel (both summed over the nodes of a `Node`
model), validates, both kernels agree (each node of a tree), and the
rendered image reads as the bike type at a glance.
