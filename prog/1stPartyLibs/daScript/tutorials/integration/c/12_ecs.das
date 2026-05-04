// Tutorial 12: Mock ECS — User Script
//
// This is the user-facing daslang script.  The user defines:
//   - A struct declaring the component layout (field names match C arrays)
//   - @required globals for host-provided parameters
//   - [es] functions with no arguments (macro injects them from the struct)
//
// The user never sees registration boilerplate — the [es] macro handles it.

options gen2

require tutorial_c_12
require ecs_macro

// Host-provided parameter — C writes this before each tick.
var @required dt : float

// Component struct — field names must match C-side component array names.
// The [es] macro reads this struct and adds its fields as function arguments.
struct Movement {
    position : float3
    velocity : float3
}

// ES system: update positions by velocity.
// The macro transforms this into:
//   [export] def update(var position : float3&; var velocity : float3&)
// and generates an [init] that calls ecs_register("update", ...).
[es]
def update() {
    position += velocity * dt
}

// ES system: apply gravity to velocity.
[es]
def apply_gravity() {
    velocity.y -= 9.8 * dt
}

[export]
def test() {
    print("ECS ready\n")
}
