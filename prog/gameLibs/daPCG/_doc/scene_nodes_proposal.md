# Scene nodes: design proposal

Status: the recommended answers of section 9 decided 2026-10-26; question 4 changed 2026-10-27.

Base: dacsg at 89b4f19a8fdc. This is the scene producer of the roadmap, section 1
([agent_modelling_roadmap.md](agent_modelling_roadmap.md)): the first piece of "ship and look".
The facts below were read in the code; the file:line references are on that commit.

## 1. Goal and non-goals

Goal. A model returns a tree of named nodes. Each node has its own mesh (a csg Solid, so also a
daSurf shell through `solid()`), a pivot frame, and the node script keys that the engine reads.
The console writes one .dag with that tree. The sport bike becomes a body with a steering head,
a front suspension, two wheels and a swingarm, so a dynModel of it can turn its wheels and steer.

Today the console writes one node with one merged mesh (console/dagFile.cpp:138-188), and the
dag exporter flattens any tree it gets (`preserve_hier` false, dagFile.cpp:181).

Non-goals of this step:
- Skinning. Hard-surface vehicles move rigid nodes; a skinned mesh is out of scope.
- A separate collision dag and LOD sets (`.lod00.dag`, ...). They come after this step (section
  8); this step writes the collision keys on the visual nodes only.
- Animation data. The producer writes the tree and the pivots; the game's animchar or physics
  moves the nodes.
- Any change to daCSG or daSurf. The executor returns one mesh per root, which is right; a node is
  a layer over meshes (roadmap section 1, section 11 "Hierarchy inside daCSG").

## 2. The DSL

A new das module `scene` with one handle type, built like `Solid`:
an int id into a state of the model's `dacsg::ModelContext`.

| call | notes |
|---|---|
| `node(name, mesh)` | a root node; `mesh` is a Solid, `empty()` for a node with no mesh |
| `n.child(name, mesh, pivot)` | a child of `n`; returns the child |
| `pivot(p)` | a frame at `p`, axes as the model's |
| `pivot_y(p, axis)` | a frame at `p` whose local Y is `axis` (a wheel: Y is the axle) |
| `pivot_z(p, axis)` | a frame at `p` whose local Z is `axis` (a steering head: Z is the head axis) |
| `n.collision(kind)` | `Collision.none` `mesh` `box` `convex` `capsule` `sphere`; returns `n` |
| `n.animated()` | the node moves: `animated_node:b=yes`; returns `n` |
| `n.script(text)` | extra node script lines, as written (`"phmat:t=\"steel\""`); returns `n` |

- The model keeps its one export: `[export] def model() : Node`. The console takes `Solid` as
  today, or `Node`, by the name of the returned struct (main.cpp:310-315 tests "Solid" now).
- The author places every part in model space, as today. The pivot is also given in model space;
  the producer works out each node's local matrix and moves its mesh into its frame. So no part
  is built twice, and moving a pivot moves no geometry.
- `pivot_y` and `pivot_z` take local X from the model axis least parallel to `axis` (on a tie,
  the first of X, Y, Z), made orthogonal; the frame is right-handed like the model's, never mirrored.
- A node is plain and gets no key: reduceNodes drops a node with neither `animated_node:b=yes`
  nor `helper_node:b=yes` unless a kept node is below it or a skin names it; collapse_nodes keeps
  an `animated_node` or `separate_mesh_node` mesh apart; a rendInst merges every node. A part that
  moves is an animated rigid node, `n.animated()`: it keeps its node and its mesh. No rigging.
- Names: unique in the tree, 1 to 64 characters of `[A-Za-z0-9_]`. In M1 the model gives
  the names the game's physics looks for (the bike below); a vehicle kit comes later (section 8).

The sport bike (both versions), with the names the engine's motorcycle physics looks for
(joltMotorcycle.cpp, loadDesc):

```das
require scene
require sportbike_parts

[export] def model() : Node {
    var body = node("body", frame() | engine() | exhaust() | pegs() | bodywork()).animated()
    let steer = body.child("steering", fork_top() | bars(), pivot_z(HEAD, STEER_AXIS)).animated()
    let front = steer.child("suspension_front", sliders() | fender(), pivot_z(HEAD, STEER_AXIS)).animated()
    front.child("wheel_front", front_wheel(), pivot_y(FRONT_AXLE, float3(0.0, 0.0, 1.0))).animated()
    body.child("swingarm", swingarm() | hugger() | chain(), pivot_y(SWING_PIVOT, float3(0.0, 0.0, 1.0))).animated()
    body.child("wheel_rear", rear_wheel(), pivot_y(REAR_AXLE, float3(0.0, 0.0, 1.0))).animated()
    return body
}
```

The physics rules for these nodes (joltMotorcycle.cpp, loadDesc and initBikeGeometrySetup):
steering's local Z is the steering axis, up and tilted back; suspension_front's local Z runs along
it; a wheel's local Y is the axle, its hub on the symmetry plane; every moving node is orthonormal,
not mirrored relative to body, and follows all the motion of a moving node above it. The physics
knows no swingarm (the rear wheel moves along the suspension and spins); the swingarm is an extra
animated node that an animation can drive. So wheel_rear is a child of body, not of swingarm: the physics
builds the wheel's local tm from its parent's rest tm, and under an animated arm it would move
twice. Today `fork()` is one part, so the bike splits it into the part that steers (`fork_top`:
clamps, stanchions) and the part that also slides (`sliders`: lower legs, calipers, axle).

## 3. Where the code lives

| piece | place | depends on |
|---|---|---|
| scene core: the node store, the pivot frames, the build | `prog/gameLibs/daPCG/scene/` (`scene_api.h`) | daCSG |
| das module `scene` | `prog/gameLibs/daPCG/das/` | the core, daScript |
| the multi-node .dag writer | the console (`prog/tools/daCSG/console/dagFile.cpp`) | libTools dagFileRW |
| the pose option, the renderable flag | dagrender (`prog/tools/dagUtils/dagRender`) | libTools dagFileRW |

The core has no daScript and no tools library, so a load-time or gameplay path can use it later
(roadmap section 8); writing a .dag needs libTools, so it stays in the tools. daPCG gets its first
code and goes into git on the dacsg branch (question 1).

```
// scene_api.h
struct SceneNode
{
  eastl::string name, model_script;
  eastl::string site;        // <file>:<line> of the model call
  int parent = -1;           // index of the parent; -1 for the root
  dacsg::TMatrixD wtm;       // the pivot frame in model space, orthonormal, right-handed
  dacsg::OpResult mesh;      // may be empty
  CollisionKind collision = CollisionKind::Default;
  bool animated = false;     // animated_node:b=yes
};
// resizes out to the node count, out[i] in node i's frame; partial on false, err from the first failed node
bool build_scene(dacsg::Graph &g, dag::ConstSpan<SceneNode> nodes, const dacsg::ExecuteParams &p,
                 dag::Vector<dacsg::Mesh> &out, dag::Vector<dacsg::ExecuteStats> *stats, eastl::string &err);
```

The build puts the node into its frame in the graph, in double: `g.transform(mesh,
inverse(wtm))` (graph_api.h:178), then one `execute` per node. The graph is read-only during an
execute (execute.cpp:452), so the nodes run one after another, each with all threads; parallel
executes wait until a test shows two at once are safe. Each node gets its own auto tolerance from
its own box (execute_api.h:25), so a fine part is not snapped to the grid of a big body.

## 4. The console

- `run_model` takes a `Node` result too (two names in result_kind); `-check` prints the tree:
  one line per node with its parent, its pivot and its csg node count.
- A scene runs `build_scene`. `-validate` and `-compare` run per node and name the node in each
  line; `-fit` takes the merged world mesh (each node's mesh by its wtm).
- `.dag`: `exportGeometry` with `preserve_hier` true puts the tree under one unnamed root node
  (dagExporter.cpp:191), so `-nodes` prints the model root with parent `-`. Per node: the local tm
  (`inverse(parent wtm) * wtm`), the script, flags RENDERABLE, CASTSHADOW, RCVSHADOW only on a node
  with a mesh, and one material slot per mat id of its faces, named as today (dagFile.cpp:24-25).
- `.obj`: the merged world mesh, as today (io/obj.cpp writes one object); per-node groups need a
  daCSG io change and wait.
- A `Solid` model writes the same bytes as before.

dagrender: it draws only nodes with NODEFLG_RENDERABLE (today it draws every mesh node,
dagRender/main.cpp:264-313), and a new `-pose:<node>,<x|y|z>,<deg>` (repeatable) turns a node
about its own axis before calc_wtm. The look check renders the bike steered 30 degrees, both
wheels turned and the swingarm moved, so a wrong pivot shows at once (roadmap section 1).

## 5. Collision

`collision(kind)` writes `collidable:b=yes` and `collision:t="<kind>"` on the node; `none` writes
`collidable:b=no` (static_object_collision.md:20-31; the exporter also reads `sphere`,
exp_collision.cpp:648-661). With no call the node gets no key and the engine default holds. The
collision exporter binds a collision node to the node of the same name (exp_collision.cpp:921-932),
so the names that the visual tree gives are the names that a collision dag must use later.

## 6. Validation and errors

- At the call: a name that breaks the name rule or is used twice; a pivot whose `axis` is zero or
  not finite (`check_finite`, the float rules of daCSG spec 11); a mesh that is not a Solid of this
  model. Each error names the model line, as csg errors do.
- At the build: a node's execute error names the node: `node "wheel_front" (sportbike.das:31):`
  then daCSG's text.
- The core tests each name (the rule of section 2), each parent (an earlier node, so no cycle;
  -1 only for node 0) and each wtm: orthonormal to 1e-6, determinant +1, finite.
- Not checked: overlap between nodes (a joint overlaps by design), clearance in a pose (the look
  check shows it).

## 7. Tests

- Core (unittests, a new group): a three-node tree, one with no mesh; each node's mesh in its frame
  maps back to the same world mesh; pivot_y and pivot_z give the frames of section 2 for an axis
  along each model axis and a tilted one; a mirrored or non-orthonormal wtm is refused; an error
  names the node.
- Console (`sheet_sync.py`): a `test_nodes.das` model (a body, a wheel with `pivot_y`, a
  contact point); its .dag read back by load_ascene, which keeps the tree (a `-nodes` print of
  the written file), gives the names, parents, local tms, scripts and flags; a `Solid` model
  gives the same bytes as the base console.
- The bike: both bikes build on both kernels; the motorcycle rules of section 2 hold for the
  written tree; dagrender pictures in three poses.

## 8. Plan

Commits on dacsg, each its own Gerrit change later: [PLAN.md](PLAN.md) M1.1 to M1.4 (the
refactor, the feature, the use, the models). Later: PLAN.md M2 and M3; parallel node builds;
per-node .obj groups; a vehicle kit that names nodes per game.

## 9. Questions, decided 2026-10-26

1. Home. The core and the das module in `prog/gameLibs/daPCG` (its first code; recommended), or
   all of it in the console until a runtime user exists.
2. Model contract. `model()` returns `Node` or `Solid`, and the console reads the type
   (recommended), or a second export `scene()`.
3. Bike node names. The motorcycle physics names body, steering, suspension_front, wheel_front,
   wheel_rear, plus swingarm as an extra node (recommended), or the names of a game's existing
   bike asset (which one?).
4. Moving nodes. Decided 2026-10-27: a node moves only when the model calls `n.animated()`; the
   owner: "not a bone, just an animated rigid body". The other answer was that every node moves.
