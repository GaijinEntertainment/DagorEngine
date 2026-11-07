options gen2

require scene
require csg

struct Node {
    id : int
}

struct Pivot {
    p : float3
    axis : float3
    kind : int
}

enum Collision {
    none = 1
    mesh
    box
    convex
    capsule
    sphere
}

def node(name : string; mesh : Solid) : Node => Node(id = scene_node(name, mesh.id))
def child(n : Node; name : string; mesh : Solid; pivot : Pivot) : Node => Node(id = scene_child(n.id, name, mesh.id, pivot.p, pivot.axis, pivot.kind))

def pivot(p : float3) : Pivot => Pivot(p = p, kind = 0)
def pivot_y(p, axis : float3) : Pivot => Pivot(p = p, axis = axis, kind = 1)
def pivot_z(p, axis : float3) : Pivot => Pivot(p = p, axis = axis, kind = 2)

def collision(n : Node; kind : Collision) : Node => Node(id = scene_collision(n.id, int(kind)))
def animated(n : Node) : Node => Node(id = scene_animated(n.id))
def script(n : Node; text : string) : Node => Node(id = scene_script(n.id, text))
