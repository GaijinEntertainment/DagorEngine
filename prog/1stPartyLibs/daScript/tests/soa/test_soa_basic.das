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
// SOA struct generation
// ===========================

[test]
def test_soa_struct_exists(t : T?) {
    t |> run("particle SOA type is created") @(t : T?) {
        var soa : Particle`SOA
        t |> equal(length(soa), 0)
    }
    t |> run("entity SOA type is created") @(t : T?) {
        var soa : Entity`SOA
        t |> equal(length(soa), 0)
    }
    t |> run("single field SOA type is created") @(t : T?) {
        var soa : SingleField`SOA
        t |> equal(length(soa), 0)
    }
}

// ===========================
// Push and length
// ===========================

[test]
def test_soa_push_and_length(t : T?) {
    t |> run("empty SOA has length 0") @(t : T?) {
        var soa : Particle`SOA
        t |> equal(length(soa), 0)
    }
    t |> run("push increases length") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(1.0), color = float4(1.0))
        t |> equal(length(soa), 1)
    }
    t |> run("push multiple elements") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(5)) {
            soa |> push <| Particle(pos = float3(i))
        }
        t |> equal(length(soa), 5)
    }
    t |> run("push preserves field values") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(1.0, 2.0, 3.0), color = float4(0.5, 0.6, 0.7, 0.8))
        t |> equal(soa[0].pos, float3(1.0, 2.0, 3.0))
        t |> equal(soa[0].color, float4(0.5, 0.6, 0.7, 0.8))
    }
    t |> run("push entity with string field") @(t : T?) {
        var soa : Entity`SOA
        soa |> push <| Entity(id = 42, name = "hero", health = 100.0, alive = true)
        t |> equal(length(soa), 1)
        t |> equal(soa[0].id, 42)
        t |> equal(soa[0].name, "hero")
        t |> equal(soa[0].health, 100.0)
        t |> equal(soa[0].alive, true)
    }
    t |> run("push single-field struct") @(t : T?) {
        var soa : SingleField`SOA
        soa |> push <| SingleField(value = 99)
        t |> equal(length(soa), 1)
        t |> equal(soa[0].value, 99)
    }
}

// ===========================
// Indexed access ([] operator and field rewrite)
// ===========================

[test]
def test_soa_index_access(t : T?) {
    t |> run("soa[i].field returns correct value") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(3)) {
            soa |> push <| Particle(pos = float3(i), color = float4(float(i) * 0.1))
        }
        t |> equal(soa[0].pos, float3(0))
        t |> equal(soa[1].pos, float3(1))
        t |> equal(soa[2].pos, float3(2))
    }
    t |> run("different fields are independent") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(10.0), color = float4(20.0))
        t |> equal(soa[0].pos, float3(10.0))
        t |> equal(soa[0].color, float4(20.0))
    }
    t |> run("index last element") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(10)) {
            soa |> push <| Particle(pos = float3(i))
        }
        t |> equal(soa[9].pos, float3(9))
    }
    t |> run("string field access by index") @(t : T?) {
        var soa : Entity`SOA
        soa |> push <| Entity(id = 1, name = "alice")
        soa |> push <| Entity(id = 2, name = "bob")
        t |> equal(soa[0].name, "alice")
        t |> equal(soa[1].name, "bob")
    }
    t |> run("bool field access by index") @(t : T?) {
        var soa : Entity`SOA
        soa |> push <| Entity(id = 1, alive = true)
        soa |> push <| Entity(id = 2, alive = false)
        t |> equal(soa[0].alive, true)
        t |> equal(soa[1].alive, false)
    }
}

// ===========================
// Erase
// ===========================

[test]
def test_soa_erase(t : T?) {
    t |> run("erase reduces length") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(3)) {
            soa |> push <| Particle(pos = float3(i))
        }
        soa |> erase(1)
        t |> equal(length(soa), 2)
    }
    t |> run("erase removes correct element") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(3)) {
            soa |> push <| Particle(pos = float3(i))
        }
        soa |> erase(0)
        // after erasing index 0, elements shift down
        t |> equal(soa[0].pos, float3(1))
        t |> equal(soa[1].pos, float3(2))
    }
    t |> run("erase last element") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(3)) {
            soa |> push <| Particle(pos = float3(i))
        }
        soa |> erase(2)
        t |> equal(length(soa), 2)
        t |> equal(soa[0].pos, float3(0))
        t |> equal(soa[1].pos, float3(1))
    }
    t |> run("erase all one by one") @(t : T?) {
        var soa : Particle`SOA
        for (i in range(3)) {
            soa |> push <| Particle(pos = float3(i))
        }
        soa |> erase(0)
        soa |> erase(0)
        soa |> erase(0)
        t |> equal(length(soa), 0)
    }
    t |> run("erase entity preserves string fields") @(t : T?) {
        var soa : Entity`SOA
        soa |> push <| Entity(id = 1, name = "a")
        soa |> push <| Entity(id = 2, name = "b")
        soa |> push <| Entity(id = 3, name = "c")
        soa |> erase(1)
        t |> equal(length(soa), 2)
        t |> equal(soa[0].name, "a")
        t |> equal(soa[1].name, "c")
    }
}

// ===========================
// push_clone
// ===========================

[test]
def test_soa_push_clone(t : T?) {
    t |> run("push_clone adds element") @(t : T?) {
        var soa : Particle`SOA
        let p = Particle(pos = float3(5.0), color = float4(1.0))
        soa |> push_clone(p)
        t |> equal(length(soa), 1)
        t |> equal(soa[0].pos, float3(5.0))
    }
    t |> run("push_clone with string field") @(t : T?) {
        var soa : Entity`SOA
        let e = Entity(id = 7, name = "clone_test", health = 50.0, alive = true)
        soa |> push_clone(e)
        t |> equal(soa[0].id, 7)
        t |> equal(soa[0].name, "clone_test")
    }
    t |> run("push_clone multiple times") @(t : T?) {
        var soa : Particle`SOA
        let p = Particle(pos = float3(1.0))
        soa |> push_clone(p)
        soa |> push_clone(p)
        soa |> push_clone(p)
        t |> equal(length(soa), 3)
        t |> equal(soa[2].pos, float3(1.0))
    }
}

// ===========================
// emplace
// ===========================

[test]
def test_soa_emplace(t : T?) {
    t |> run("emplace adds element by move") @(t : T?) {
        var soa : Particle`SOA
        var p = Particle(pos = float3(3.0), color = float4(0.5))
        soa |> emplace(p)
        t |> equal(length(soa), 1)
        t |> equal(soa[0].pos, float3(3.0))
    }
    t |> run("emplace entity") @(t : T?) {
        var soa : Entity`SOA
        var e = Entity(id = 99, name = "moved", health = 75.0, alive = false)
        soa |> emplace(e)
        t |> equal(soa[0].id, 99)
        t |> equal(soa[0].alive, false)
    }
}

// ===========================
// Combined operations
// ===========================

[test]
def test_soa_combined(t : T?) {
    t |> run("push then erase then push") @(t : T?) {
        var soa : Particle`SOA
        soa |> push <| Particle(pos = float3(1.0))
        soa |> push <| Particle(pos = float3(2.0))
        soa |> push <| Particle(pos = float3(3.0))
        soa |> erase(1)
        soa |> push <| Particle(pos = float3(4.0))
        t |> equal(length(soa), 3)
        t |> equal(soa[0].pos, float3(1.0))
        t |> equal(soa[1].pos, float3(3.0))
        t |> equal(soa[2].pos, float3(4.0))
    }
    t |> run("many pushes and erases") @(t : T?) {
        var soa : Entity`SOA
        for (i in range(10)) {
            soa |> push <| Entity(id = i, name = "e{i}")
        }
        t |> equal(length(soa), 10)
        // erase even-indexed elements from front
        soa |> erase(0) // removes id=0, shifts
        soa |> erase(1) // removes id=2
        soa |> erase(2) // removes id=4
        t |> equal(length(soa), 7)
        t |> equal(soa[0].id, 1)
    }
}
