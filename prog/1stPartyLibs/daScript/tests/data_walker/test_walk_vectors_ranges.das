// Tests for DapiDataWalker — vector and range types
//
// Covers: int2/3/4, uint2/3/4, float2/3/4, range, urange, range64, urange64

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dw_common


[test]
def test_walk_vectors(t : T?) {
    t |> run("int2") @(t : T?) {
        var walker = new LogWalker()
        var v = int2(10, 20)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(walker.log, "Int2:{v}\n")
        unsafe { delete walker; }
    }
    t |> run("int3") @(t : T?) {
        var walker = new LogWalker()
        var v = int3(1, 2, 3)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(walker.log, "Int3:{v}\n")
        unsafe { delete walker; }
    }
    t |> run("int4") @(t : T?) {
        var walker = new LogWalker()
        var v = int4(1, 2, 3, 4)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(walker.log, "Int4:{v}\n")
        unsafe { delete walker; }
    }
    t |> run("float2") @(t : T?) {
        var walker = new LogWalker()
        var v = float2(1.5, 2.5)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(walker.log, "Float2:{v}\n")
        unsafe { delete walker; }
    }
    t |> run("float3") @(t : T?) {
        var walker = new LogWalker()
        var v = float3(1.0, 2.0, 3.0)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(walker.log, "Float3:{v}\n")
        unsafe { delete walker; }
    }
    t |> run("float4") @(t : T?) {
        var walker = new LogWalker()
        var v = float4(1.0, 2.0, 3.0, 4.0)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(walker.log, "Float4:{v}\n")
        unsafe { delete walker; }
    }
    t |> run("uint2") @(t : T?) {
        var walker = new LogWalker()
        var v = uint2(1u, 2u)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(walker.log, "UInt2:{v}\n")
        unsafe { delete walker; }
    }
    t |> run("uint3") @(t : T?) {
        var walker = new LogWalker()
        var v = uint3(1u, 2u, 3u)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(walker.log, "UInt3:{v}\n")
        unsafe { delete walker; }
    }
    t |> run("uint4") @(t : T?) {
        var walker = new LogWalker()
        var v = uint4(1u, 2u, 3u, 4u)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(walker.log, "UInt4:{v}\n")
        unsafe { delete walker; }
    }
}


[test]
def test_walk_ranges(t : T?) {
    t |> run("range") @(t : T?) {
        var walker = new LogWalker()
        var r = range(0, 10)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(r), typeinfo rtti_typeinfo(r))
        }
        t |> equal(walker.log, "Range:{r}\n")
        unsafe { delete walker; }
    }
    t |> run("urange") @(t : T?) {
        var walker = new LogWalker()
        var r = urange(5u, 15u)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(r), typeinfo rtti_typeinfo(r))
        }
        t |> equal(walker.log, "URange:{r}\n")
        unsafe { delete walker; }
    }
    t |> run("range64") @(t : T?) {
        var walker = new LogWalker()
        var r = range64(0l, 100l)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(r), typeinfo rtti_typeinfo(r))
        }
        t |> equal(walker.log, "Range64:{r}\n")
        unsafe { delete walker; }
    }
    t |> run("urange64") @(t : T?) {
        var walker = new LogWalker()
        var r = urange64(10ul, 20ul)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(r), typeinfo rtti_typeinfo(r))
        }
        t |> equal(walker.log, "URange64:{r}\n")
        unsafe { delete walker; }
    }
}
