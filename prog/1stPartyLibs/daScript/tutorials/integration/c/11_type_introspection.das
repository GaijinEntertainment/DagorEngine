options gen2
options rtti

// Companion script for the C "Type Introspection" tutorial.
// Defines a struct, an enum, and exported functions that the C host
// inspects via the type introspection API.

enum Direction {
    north
    south
    east
    west
}

struct Transform {
    x : float = 0.0
    y : float = 0.0
    z : float = 0.0
    scale : float = 1.0
}

struct Entity {
    name : string
    transform : Transform = Transform()
    health : int
    direction : Direction
}

var entities : array<Entity>

[export]
def spawn_entity(n : string; px, py, pz : float) {
    entities |> emplace(Entity(name = n, transform = Transform(x = px, y = py, z = pz)))
}

[export]
def get_entity_count() : int {
    return length(entities)
}
