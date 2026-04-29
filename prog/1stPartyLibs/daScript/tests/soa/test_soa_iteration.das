options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/soa

// ===========================
// Test structs
// ===========================

[soa]
struct Particle {
    pos : float3
    color : float4
}

[soa]
struct Entity {
    id : int
    name : string
    health : float
    alive : bool
}

[soa]
struct SingleField {
    value : int
}

// ===========================
// Basic for-loop iteration
// ===========================

[test]
def test_soa_for_loop_basic(t : T?) {
    t |> run("iterate over SOA with for loop") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(3)) {
            soa |> push <| Particle(pos = float3(i))
        }
        var count = 0
        for (it in soa) {
            count ++
        }
        t |> equal(count, 3)
    }
    t |> run("for loop accesses correct field values") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(10.0))
        soa |> push <| Particle(pos = float3(20.0))
        soa |> push <| Particle(pos = float3(30.0))
        var sum = float3(0)
        for (it in soa) {
            sum += it.pos
        }
        t |> equal(sum, float3(60.0))
    }
    t |> run("for loop accesses multiple fields") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(1.0), color = float4(2.0))
        soa |> push <| Particle(pos = float3(3.0), color = float4(4.0))
        var pos_sum = float3(0)
        var color_sum = float4(0)
        for (it in soa) {
            pos_sum += it.pos
            color_sum += it.color
        }
        t |> equal(pos_sum, float3(4.0))
        t |> equal(color_sum, float4(6.0))
    }
    t |> run("for loop over empty SOA does not execute") @(t : T?) {
        var soa : Particle`SOA
        var count = 0
        for (it in soa) {
            count ++
        }
        t |> equal(count, 0)
    }
    t |> run("for loop single field struct") @(t : T?) {
        var soa : SingleField`SOA
        for (i in range(4)) {
            soa |> push <| SingleField(value = i * 10)
        }
        var sum = 0
        for (it in soa) {
            sum += it.value
        }
        t |> equal(sum, 60) // 0+10+20+30
    }
}

// ===========================
// For loop with entity (string + bool fields)
// ===========================

[test]
def test_soa_for_loop_entity(t : T?) {
    t |> run("iterate entity SOA accessing int field") @(t : T?) {
        var soa : Entity`SOA
        for (i in range(3)) {
            soa |> push <| Entity(id = i + 1)
        }
        var id_sum = 0
        for (it in soa) {
            id_sum += it.id
        }
        t |> equal(id_sum, 6) // 1+2+3
    }
    t |> run("iterate entity SOA accessing string field") @(t : T?) {
        var soa : Entity`SOA
        soa |> push <| Entity(id = 1, name = "a")
        soa |> push <| Entity(id = 2, name = "b")
        var names = ""
        for (it in soa) {
            names += it.name
        }
        t |> equal(names, "ab")
    }
    t |> run("iterate entity SOA accessing bool field") @(t : T?) {
        var soa : Entity`SOA
        soa |> push <| Entity(id = 1, alive = true)
        soa |> push <| Entity(id = 2, alive = false)
        soa |> push <| Entity(id = 3, alive = true)
        var alive_count = 0
        for (it in soa) {
            if (it.alive) {
                alive_count ++
            }
        }
        t |> equal(alive_count, 2)
    }
}

// ===========================
// Mixed for-loop (SOA + other iterators)
// ===========================

[test]
def test_soa_for_loop_mixed(t : T?) {
    t |> run("SOA with count() iterator") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(3)) {
            soa |> push <| Particle(pos = float3(i))
        }
        var indices : array<int>
        for (idx, it in count(), soa) {
            indices |> push(idx)
        }
        t |> equal(length(indices), 3)
        t |> equal(indices[0], 0)
        t |> equal(indices[1], 1)
        t |> equal(indices[2], 2)
    }
    t |> run("SOA with range iterator") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(3)) {
            soa |> push <| Particle(pos = float3(i))
        }
        var sum = 0
        for (it, r in soa, range(3)) {
            sum += r
        }
        t |> equal(sum, 3) // 0+1+2
    }
}

// ===========================
// For loop after erase
// ===========================

[test]
def test_soa_for_loop_after_erase(t : T?) {
    t |> run("iterate after erase") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(5)) {
            soa |> push <| Particle(pos = float3(i))
        }
        soa |> erase(2) // remove middle element
        var count = 0
        var sum = float3(0)
        for (it in soa) {
            sum += it.pos
            count ++
        }
        t |> equal(count, 4)
        // 0+1+3+4 = 8
        t |> equal(sum, float3(8.0))
    }
}

// ===========================
// For loop accessing only one field (optimization check)
// ===========================

[test]
def test_soa_for_loop_single_field_access(t : T?) {
    t |> run("accessing only pos in particle loop") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(1.0), color = float4(99.0))
        soa |> push <| Particle(pos = float3(2.0), color = float4(99.0))
        var sum = float3(0)
        for (it in soa) {
            sum += it.pos
        }
        t |> equal(sum, float3(3.0))
    }
    t |> run("accessing only color in particle loop") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(99.0), color = float4(1.0))
        soa |> push <| Particle(pos = float3(99.0), color = float4(2.0))
        var sum = float4(0)
        for (it in soa) {
            sum += it.color
        }
        t |> equal(sum, float4(3.0))
    }
}
