// Tutorial 50: Structure-of-Arrays (SOA)
//
// This tutorial covers:
//   - What SOA is and why it helps performance
//   - The [soa] annotation and generated layout
//   - Indexed access (soa[i].field macro rewrite)
//   - Iteration with for-loops (SoaForLoop macro)
//   - Container operations: push, push_clone, emplace, erase, pop, clear
//   - Sizing: length, capacity, resize, reserve
//   - Swap and manual sorting
//   - Bulk conversion: from_array, to_array
//   - Particle simulation example
//   - Game entity table example
//
// Run: daslang.exe tutorials/language/50_soa.das

options gen2
options no_unused_function_arguments = false

require daslib/soa
require math

// ============================================================
// Section 1: What is SOA?
// ============================================================
// Normally a struct is stored as Array-of-Structures (AOS):
//   [ {x,y,z,w}, {x,y,z,w}, {x,y,z,w}, ... ]
//
// Structure-of-Arrays (SOA) rearranges this into parallel arrays:
//   xs: [x, x, x, ...]
//   ys: [y, y, y, ...]
//   ...
//
// This is friendlier to CPU caches when iterating over a single
// field (e.g. updating positions), because the data is contiguous.
//
// The [soa] annotation automates this transformation.

[soa]
struct Particle {
    pos   : float3
    vel   : float3
    life  : float
    color : float4
}

def demo_basic_soa() {
    print("=== basic SOA ===\n")
    // Declare the SOA container — Particle`SOA holds parallel arrays
    var particles : Particle`SOA

    // Push elements as if it were an ordinary array
    particles |> push <| Particle(pos = float3(0.0), vel = float3(1.0, 0.0, 0.0), life = 3.0, color = float4(1.0, 0.0, 0.0, 1.0))
    particles |> push <| Particle(pos = float3(5.0), vel = float3(0.0, 1.0, 0.0), life = 5.0, color = float4(0.0, 1.0, 0.0, 1.0))
    particles |> push <| Particle(pos = float3(10.0), vel = float3(0.0, 0.0, 1.0), life = 2.0, color = float4(0.0, 0.0, 1.0, 1.0))
    print("  count: {length(particles)}\n")

    // Indexed access: the macro rewrites soa[i].field to soa.field[i]
    print("  particles[0].pos  = {particles[0].pos}\n")
    print("  particles[1].life = {particles[1].life}\n")
    print("  particles[2].vel  = {particles[2].vel}\n")
}

// ============================================================
// Section 2: Iteration
// ============================================================
// The SoaForLoop macro transforms `for (it in soa)` into a
// multi-source loop over the individual column arrays — only
// the fields you actually access are iterated.

def demo_iteration() {
    print("\n=== iteration ===\n")
    var particles : Particle`SOA
    for (i in range(4)) {
        particles |> push <| Particle(pos = float3(float(i)), life = float(4 - i))
    }

    // Access both pos and life
    print("  all particles:\n")
    for (it in particles) {
        print("    pos={it.pos} life={it.life}\n")
    }

    // Mixed iteration with an index counter
    print("  with index:\n")
    for (idx, it in count(), particles) {
        print("    [{idx}] pos={it.pos}\n")
    }
}

// ============================================================
// Section 3: Container operations
// ============================================================

def demo_container_ops() {
    print("\n=== container operations ===\n")
    var soa : Particle`SOA

    // push — move semantics
    soa |> push <| Particle(pos = float3(1.0), life = 10.0)
    soa |> push <| Particle(pos = float3(2.0), life = 20.0)
    soa |> push <| Particle(pos = float3(3.0), life = 30.0)
    print("  after 3 pushes: length={length(soa)}\n")

    // push_clone — copy from a const value
    let p = Particle(pos = float3(4.0), life = 40.0)
    soa |> push_clone(p)
    print("  after push_clone: length={length(soa)}\n")

    // erase — remove by index, shifts remaining elements
    soa |> erase(1)
    print("  after erase(1): length={length(soa)}, [0].life={soa[0].life}, [1].life={soa[1].life}\n")

    // pop — remove last element
    soa |> pop()
    print("  after pop: length={length(soa)}\n")

    // clear — remove all elements
    soa |> clear()
    print("  after clear: length={length(soa)}\n")
}

// ============================================================
// Section 4: Sizing — resize, reserve, capacity
// ============================================================

def demo_sizing() {
    print("\n=== sizing ===\n")
    var soa : Particle`SOA

    // reserve — pre-allocate memory without changing length
    soa |> reserve(100)
    print("  after reserve(100): length={length(soa)}, capacity={capacity(soa)}\n")

    // push some elements
    for (i in range(10)) {
        soa |> push <| Particle(pos = float3(float(i)), life = float(i))
    }
    print("  after 10 pushes: length={length(soa)}\n")

    // resize — change length (truncate or extend with defaults)
    soa |> resize(5)
    print("  after resize(5): length={length(soa)}, [4].pos={soa[4].pos}\n")

    soa |> resize(8)
    print("  after resize(8): length={length(soa)}\n")
}

// ============================================================
// Section 5: Swap and sorting
// ============================================================

[soa]
struct SortItem {
    key : int
    tag : string
}

def demo_swap_and_sort() {
    print("\n=== swap and sorting ===\n")
    var soa : SortItem`SOA
    soa |> push <| SortItem(key = 30, tag = "gamma")
    soa |> push <| SortItem(key = 10, tag = "alpha")
    soa |> push <| SortItem(key = 20, tag = "beta")

    // swap — exchanges all fields at once
    soa |> swap(0, 2)
    print("  after swap(0,2): [{soa[0].key}={soa[0].tag}] [{soa[1].key}={soa[1].tag}] [{soa[2].key}={soa[2].tag}]\n")

    // Bubble sort using swap
    let n = length(soa)
    for (pass_idx in range(n)) {
        for (k in range(n - 1)) {
            if (soa[k].key > soa[k + 1].key) {
                soa |> swap(k, k + 1)
            }
        }
    }
    print("  sorted: ")
    for (it in soa) {
        print("[{it.key}={it.tag}] ")
    }
    print("\n")
}

// ============================================================
// Section 6: Bulk conversion — from_array, to_array
// ============================================================

[soa]
struct Vec2 {
    x : float
    y : float
}

def demo_conversion() {
    print("\n=== bulk conversion ===\n")

    // Build a regular array
    var arr : array<Vec2>
    arr |> push <| Vec2(x = 1.0, y = 2.0)
    arr |> push <| Vec2(x = 3.0, y = 4.0)
    arr |> push <| Vec2(x = 5.0, y = 6.0)
    print("  AOS array length: {length(arr)}\n")

    // Convert AOS → SOA
    var soa : Vec2`SOA
    soa |> from_array(arr)
    print("  SOA length after from_array: {length(soa)}\n")
    print("  soa[1].x={soa[1].x}, soa[1].y={soa[1].y}\n")

    // Convert SOA → AOS
    var result <- to_array(soa)
    print("  AOS result length: {length(result)}\n")
    for (idx in range(length(result))) {
        print("    [{idx}] x={result[idx].x} y={result[idx].y}\n")
    }
}

// ============================================================
// Section 7: Particle simulation
// ============================================================

def demo_particle_sim() {
    print("\n=== particle simulation ===\n")
    var particles : Particle`SOA
    particles |> reserve(100)

    // Spawn particles
    for (i in range(5)) {
        let fi = float(i)
        particles |> push <| Particle(
            pos   = float3(fi * 2.0, 0.0, 0.0),
            vel   = float3(0.0, 1.0 + fi * 0.5, 0.0),
            life  = 3.0 + fi,
            color = float4(fi / 4.0, 1.0 - fi / 4.0, 0.5, 1.0)
        )
    }
    print("  spawned {length(particles)} particles\n")

    // Simulate 3 steps
    let dt = 1.0
    for (step in range(3)) {
        // Update positions and decrease life
        for (it in particles) {
            it.pos += it.vel * dt
            it.life -= dt
        }
        // Remove dead particles (iterate backwards to avoid index issues)
        var i = length(particles) - 1
        while (i >= 0) {
            if (particles[i].life <= 0.0) {
                particles |> erase(i)
            }
            i --
        }
        print("  step {step + 1}: {length(particles)} alive\n")
    }

    // Print survivors
    for (it in particles) {
        print("    pos={it.pos} life={it.life}\n")
    }
}

// ============================================================
// Section 8: Game entity table
// ============================================================

[soa]
struct GameEntity {
    id     : int
    name   : string
    health : float
    alive  : bool
}

def demo_entity_table() {
    print("\n=== game entity table ===\n")
    var entities : GameEntity`SOA
    entities |> push <| GameEntity(id = 1, name = "warrior", health = 100.0, alive = true)
    entities |> push <| GameEntity(id = 2, name = "mage",    health = 60.0,  alive = true)
    entities |> push <| GameEntity(id = 3, name = "archer",  health = 80.0,  alive = true)
    entities |> push <| GameEntity(id = 4, name = "thief",   health = 50.0,  alive = true)

    // Apply damage to all entities
    for (it in entities) {
        it.health -= 55.0
        if (it.health <= 0.0) {
            it.alive = false
        }
    }

    // Print status
    for (idx, it in count(), entities) {
        print("  [{it.id}] {it.name}: health={it.health} alive={it.alive}\n")
    }

    // Count alive
    var alive_count = 0
    for (it in entities) {
        if (it.alive) {
            alive_count ++
        }
    }
    print("  alive: {alive_count}/{length(entities)}\n")

    // Convert to AOS for serialization
    var snapshot <- to_array(entities)
    print("  snapshot has {length(snapshot)} entities\n")
}

[export]
def main() {
    demo_basic_soa()
    demo_iteration()
    demo_container_ops()
    demo_sizing()
    demo_swap_and_sort()
    demo_conversion()
    demo_particle_sim()
    demo_entity_table()
}
