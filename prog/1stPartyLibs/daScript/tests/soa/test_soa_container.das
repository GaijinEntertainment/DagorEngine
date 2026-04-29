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
// resize
// ===========================

[test]
def test_soa_resize(t : T?) {
    t |> run("resize grows SOA") @(t : T?) {
        var soa : Particle`SOA
        soa |> resize(3)
        t |> equal(length(soa), 3)
    }
    t |> run("resize shrinks SOA") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(5)) {
            soa |> push <| Particle(pos = float3(i))
        }
        soa |> resize(2)
        t |> equal(length(soa), 2)
        t |> equal(soa[0].pos, float3(0))
        t |> equal(soa[1].pos, float3(1))
    }
    t |> run("resize to zero clears all") @(t : T?) {
        var soa : Entity`SOA
        for (i in range(3)) {
            soa |> push <| Entity(id = i)
        }
        soa |> resize(0)
        t |> equal(length(soa), 0)
    }
    t |> run("resize preserves existing elements") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(1.0, 2.0, 3.0))
        soa |> resize(3)
        t |> equal(soa[0].pos, float3(1.0, 2.0, 3.0))
    }
}

// ===========================
// reserve
// ===========================

[test]
def test_soa_reserve(t : T?) {
    t |> run("reserve does not change length") @(t : T?) {
        var soa : Particle`SOA
        soa |> reserve(100)
        t |> equal(length(soa), 0)
    }
    t |> run("reserve then push works") @(t : T?) {
        var soa : Particle`SOA
        soa |> reserve(10)
        for (i in range(10)) {
            soa |> push <| Particle(pos = float3(i))
        }
        t |> equal(length(soa), 10)
        t |> equal(soa[9].pos, float3(9))
    }
}

// ===========================
// clear
// ===========================

[test]
def test_soa_clear(t : T?) {
    t |> run("clear empties SOA") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(5)) {
            soa |> push <| Particle(pos = float3(i))
        }
        soa |> clear()
        t |> equal(length(soa), 0)
    }
    t |> run("clear on empty is no-op") @(t : T?) {
        var soa : Particle`SOA
        soa |> clear()
        t |> equal(length(soa), 0)
    }
    t |> run("push after clear works") @(t : T?) {
        var soa : Entity`SOA
        soa |> push <| Entity(id = 1, name = "a")
        soa |> clear()
        soa |> push <| Entity(id = 2, name = "b")
        t |> equal(length(soa), 1)
        t |> equal(soa[0].id, 2)
        t |> equal(soa[0].name, "b")
    }
}

// ===========================
// pop
// ===========================

[test]
def test_soa_pop(t : T?) {
    t |> run("pop removes last element") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(3)) {
            soa |> push <| Particle(pos = float3(i))
        }
        soa |> pop()
        t |> equal(length(soa), 2)
        t |> equal(soa[0].pos, float3(0))
        t |> equal(soa[1].pos, float3(1))
    }
    t |> run("pop until empty") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(1.0))
        soa |> push <| Particle(pos = float3(2.0))
        soa |> pop()
        soa |> pop()
        t |> equal(length(soa), 0)
    }
    t |> run("pop entity with strings") @(t : T?) {
        var soa : Entity`SOA
        soa |> push <| Entity(id = 1, name = "a")
        soa |> push <| Entity(id = 2, name = "b")
        soa |> pop()
        t |> equal(length(soa), 1)
        t |> equal(soa[0].name, "a")
    }
}

// ===========================
// capacity
// ===========================

[test]
def test_soa_capacity(t : T?) {
    t |> run("capacity >= length") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(10)) {
            soa |> push <| Particle(pos = float3(i))
        }
        t |> equal(capacity(soa) >= length(soa), true)
    }
    t |> run("capacity grows after reserve") @(t : T?) {
        var soa : Particle`SOA
        soa |> reserve(100)
        t |> equal(capacity(soa) >= 100, true)
    }
}

// ===========================
// swap
// ===========================

[test]
def test_soa_swap(t : T?) {
    t |> run("swap two elements") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(1.0), color = float4(10.0))
        soa |> push <| Particle(pos = float3(2.0), color = float4(20.0))
        soa |> push <| Particle(pos = float3(3.0), color = float4(30.0))
        soa |> swap(0, 2)
        t |> equal(soa[0].pos, float3(3.0))
        t |> equal(soa[0].color, float4(30.0))
        t |> equal(soa[2].pos, float3(1.0))
        t |> equal(soa[2].color, float4(10.0))
        // middle element unchanged
        t |> equal(soa[1].pos, float3(2.0))
    }
    t |> run("swap same index is no-op") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(5.0))
        soa |> swap(0, 0)
        t |> equal(soa[0].pos, float3(5.0))
    }
    t |> run("swap entity with strings") @(t : T?) {
        var soa : Entity`SOA
        soa |> push <| Entity(id = 1, name = "first", health = 10.0)
        soa |> push <| Entity(id = 2, name = "second", health = 20.0)
        soa |> swap(0, 1)
        t |> equal(soa[0].id, 2)
        t |> equal(soa[0].name, "second")
        t |> equal(soa[1].id, 1)
        t |> equal(soa[1].name, "first")
    }
    t |> run("swap for manual sort") @(t : T?) {
        var soa : SingleField`SOA
        soa |> push <| SingleField(value = 30)
        soa |> push <| SingleField(value = 10)
        soa |> push <| SingleField(value = 20)
        // bubble sort using swap
        for (pass_idx in range(3)) {
            for (k in range(2)) {
                if (soa[k].value > soa[k + 1].value) {
                    soa |> swap(k, k + 1)
                }
            }
        }
        t |> equal(soa[0].value, 10)
        t |> equal(soa[1].value, 20)
        t |> equal(soa[2].value, 30)
    }
}

// ===========================
// from_array
// ===========================

[test]
def test_soa_from_array(t : T?) {
    t |> run("from_array populates SOA") @(t : T?) {
        var soa : Particle`SOA
        var arr : array<Particle>
        arr |> push <| Particle(pos = float3(1.0), color = float4(10.0))
        arr |> push <| Particle(pos = float3(2.0), color = float4(20.0))
        arr |> push <| Particle(pos = float3(3.0), color = float4(30.0))
        soa |> from_array(arr)
        t |> equal(length(soa), 3)
        t |> equal(soa[0].pos, float3(1.0))
        t |> equal(soa[1].pos, float3(2.0))
        t |> equal(soa[2].pos, float3(3.0))
        t |> equal(soa[0].color, float4(10.0))
    }
    t |> run("from_array appends to existing") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(0.0))
        var arr : array<Particle>
        arr |> push <| Particle(pos = float3(1.0))
        arr |> push <| Particle(pos = float3(2.0))
        soa |> from_array(arr)
        t |> equal(length(soa), 3)
        t |> equal(soa[0].pos, float3(0.0))
        t |> equal(soa[1].pos, float3(1.0))
        t |> equal(soa[2].pos, float3(2.0))
    }
    t |> run("from_array with empty array is no-op") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(1.0))
        var arr : array<Particle>
        soa |> from_array(arr)
        t |> equal(length(soa), 1)
    }
    t |> run("from_array entity with strings") @(t : T?) {
        var soa : Entity`SOA
        var arr : array<Entity>
        arr |> push <| Entity(id = 1, name = "hero", health = 100.0, alive = true)
        arr |> push <| Entity(id = 2, name = "npc", health = 50.0, alive = false)
        soa |> from_array(arr)
        t |> equal(length(soa), 2)
        t |> equal(soa[0].name, "hero")
        t |> equal(soa[1].name, "npc")
        t |> equal(soa[1].alive, false)
    }
}

// ===========================
// to_array
// ===========================

[test]
def test_soa_to_array(t : T?) {
    t |> run("to_array converts SOA back") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(1.0, 2.0, 3.0), color = float4(0.1, 0.2, 0.3, 0.4))
        soa |> push <| Particle(pos = float3(4.0, 5.0, 6.0), color = float4(0.5, 0.6, 0.7, 0.8))
        var arr <- to_array(soa)
        t |> equal(length(arr), 2)
        t |> equal(arr[0].pos, float3(1.0, 2.0, 3.0))
        t |> equal(arr[0].color, float4(0.1, 0.2, 0.3, 0.4))
        t |> equal(arr[1].pos, float3(4.0, 5.0, 6.0))
        t |> equal(arr[1].color, float4(0.5, 0.6, 0.7, 0.8))
    }
    t |> run("to_array empty SOA returns empty array") @(t : T?) {
        var soa : Particle`SOA
        var arr <- to_array(soa)
        t |> equal(length(arr), 0)
    }
    t |> run("to_array entity with strings") @(t : T?) {
        var soa : Entity`SOA
        soa |> push <| Entity(id = 1, name = "a", health = 10.0, alive = true)
        soa |> push <| Entity(id = 2, name = "b", health = 20.0, alive = false)
        var arr <- to_array(soa)
        t |> equal(length(arr), 2)
        t |> equal(arr[0].id, 1)
        t |> equal(arr[0].name, "a")
        t |> equal(arr[1].alive, false)
    }
    t |> run("roundtrip from_array -> to_array") @(t : T?) {
        var original : array<Particle>
        original |> push <| Particle(pos = float3(10.0), color = float4(1.0))
        original |> push <| Particle(pos = float3(20.0), color = float4(2.0))
        original |> push <| Particle(pos = float3(30.0), color = float4(3.0))
        var soa : Particle`SOA
        soa |> from_array(original)
        var result <- to_array(soa)
        t |> equal(length(result), 3)
        for (idx in range(3)) {
            t |> equal(result[idx].pos, original[idx].pos)
            t |> equal(result[idx].color, original[idx].color)
        }
    }
    t |> run("roundtrip entity with strings") @(t : T?) {
        var original : array<Entity>
        original |> push <| Entity(id = 1, name = "hero", health = 100.0, alive = true)
        original |> push <| Entity(id = 2, name = "villain", health = 200.0, alive = false)
        var soa : Entity`SOA
        soa |> from_array(original)
        var result <- to_array(soa)
        t |> equal(length(result), 2)
        t |> equal(result[0].name, "hero")
        t |> equal(result[1].name, "villain")
        t |> equal(result[0].alive, true)
        t |> equal(result[1].alive, false)
    }
}
