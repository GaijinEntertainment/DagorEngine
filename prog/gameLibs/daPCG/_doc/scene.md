# scene DSL sheet

`require scene` makes a model a tree of named nodes: `[export] def model() : Node`
returns the root (the console refuses another node). Each node has a csg `Solid`
and a pivot frame, both in model space; the .dag holds each mesh in its frame.

```das
options gen2
require csg
require scene

[export] def model() : Node {
    let body = node("body", box(1.6, 0.4, 0.5).at(0.0, 0.6, 0.0)).script("phmat:t=\"steel\"")
    let axle = float3(0.6, 0.3, 0.0)
    let wheel = cylinder(0.3, 0.12).rot_x(90.0).at(axle)
    body.child("wheel", wheel, pivot_y(axle, float3(0.0, 0.0, 1.0))).collision(Collision.convex).animated()
    return body
}
```

| call | notes |
|---|---|
| `node(name, mesh)` | the root; `empty()` for a node with no mesh |
| `n.child(name, mesh, pivot)` | a child of `n`; returns the child |
| `pivot(p)` | a frame at `p` with the model's axes |
| `pivot_y(p, axis)` `pivot_z(p, axis)` | local Y (a wheel's axle) or Z (a steering head) along `axis`; X from the least parallel model axis; right-handed |
| `n.collision(kind)` | `Collision.none` `mesh` `box` `convex` `capsule` `sphere`; no call: the engine default |
| `n.animated()` | the node moves: `animated_node:b=yes`; returns `n` |
| `n.script(text)` | a node script line, as written; returns `n` |

Names: unique, 1 to 64 characters of `A-Za-z0-9_`. A node is plain: the engine
merges its mesh into its parent's, and a rendInst merges all nodes. A part that
moves calls `n.animated()`, and keeps its node and its mesh, or its node alone
with no mesh. A pivot moves no geometry. Console: `-check` prints the tree,
`-o:<out.dag> -nodes` reads the .dag back; dagrender `-pose:<node>,<x|y|z>,<deg>`
turns a node.
