// Tests regular [] index operations across all types and index types
options gen2
require dastest/testing_boost public

[test]
def test_pointer_index(t : T?) {
    t |> run("pointer index int") @(t : T?) {
        var ar = fixed_array(10, 20, 30)
        var p : int? = unsafe(addr(ar[0]))
        var val = unsafe(p[0])
        t |> equal(val, 10)
        val = unsafe(p[2])
        t |> equal(val, 30)
    }
    t |> run("pointer index uint") @(t : T?) {
        var ar = fixed_array(10, 20, 30)
        var p : int? = unsafe(addr(ar[0]))
        var val = unsafe(p[0u])
        t |> equal(val, 10)
        val = unsafe(p[2u])
        t |> equal(val, 30)
    }
    t |> run("pointer index int64") @(t : T?) {
        var ar = fixed_array(10, 20, 30)
        var p : int? = unsafe(addr(ar[0]))
        var val = unsafe(p[0l])
        t |> equal(val, 10)
        val = unsafe(p[2l])
        t |> equal(val, 30)
    }
    t |> run("pointer index uint64") @(t : T?) {
        var ar = fixed_array(10, 20, 30)
        var p : int? = unsafe(addr(ar[0]))
        var val = unsafe(p[0ul])
        t |> equal(val, 10)
        val = unsafe(p[2ul])
        t |> equal(val, 30)
    }
}

[test]
def test_array_index(t : T?) {
    t |> run("array index int") @(t : T?) {
        var ar <- [10, 20, 30]
        t |> equal(ar[0], 10)
        t |> equal(ar[2], 30)
    }
    t |> run("array index uint") @(t : T?) {
        var ar <- [10, 20, 30]
        t |> equal(ar[0u], 10)
        t |> equal(ar[2u], 30)
    }
}

[test]
def test_fixed_array_index(t : T?) {
    t |> run("fixed array index int") @(t : T?) {
        var di = fixed_array(10, 20, 30)
        t |> equal(di[0], 10)
        t |> equal(di[2], 30)
    }
    t |> run("fixed array index uint") @(t : T?) {
        var di = fixed_array(10, 20, 30)
        t |> equal(di[0u], 10)
        t |> equal(di[2u], 30)
    }
}

[test]
def test_vector_index(t : T?) {
    t |> run("vector index int") @(t : T?) {
        var vec = int3(10, 20, 30)
        t |> equal(vec[0], 10)
        t |> equal(vec[2], 30)
    }
    t |> run("vector index uint") @(t : T?) {
        var vec = int3(10, 20, 30)
        t |> equal(vec[0u], 10)
        t |> equal(vec[2u], 30)
    }
}

[test]
def test_table_index(t : T?) {
    t |> run("table index string key") @(t : T?) {
        var ta <- {"a" => 10, "b" => 20}
        t |> equal(ta["a"], 10)
        t |> equal(ta["b"], 20)
    }
    t |> run("table index int key") @(t : T?) {
        var ta <- {1 => 10, 2 => 20}
        t |> equal(ta[1], 10)
        t |> equal(ta[2], 20)
    }
}

[test]
def test_struct_pointer_index(t : T?) {
    t |> run("struct pointer index") @(t : T?) {
        var ar = fixed_array(int2(1, 2), int2(3, 4), int2(5, 6))
        var p : int2? = unsafe(addr(ar[0]))
        var val = unsafe(p[1])
        t |> equal(val.x, 3)
        t |> equal(val.y, 4)
    }
}
