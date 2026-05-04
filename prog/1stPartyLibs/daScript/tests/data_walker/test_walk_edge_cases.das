// Tests for DapiDataWalker — edge cases and miscellaneous
//
// Covers: null/non-null pointers, lambdas, edge values (zero, negative,
//         min/max int), deeply nested data, adapter reuse, scalar counting

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dw_common


// ============================================================
// Local walker class
// ============================================================

// Counts all scalar visits
class CountWalker : DapiDataWalker {
    count : int = 0
    def override Bool(var value : bool&) : void { count++; }
    def override Int(var value : int&) : void { count++; }
    def override UInt(var value : uint&) : void { count++; }
    def override Float(var value : float&) : void { count++; }
    def override Double(var value : double&) : void { count++; }
    def override String(var value : string&) : void { count++; }
    def override Int64(var value : int64&) : void { count++; }
    def override UInt64(var value : uint64&) : void { count++; }
    def override Int8(var value : int8&) : void { count++; }
    def override UInt8(var value : uint8&) : void { count++; }
    def override Int16(var value : int16&) : void { count++; }
    def override UInt16(var value : uint16&) : void { count++; }
}


// ============================================================
// Pointers
// ============================================================

[test]
def test_walk_null_pointer(t : T?) {
    t |> run("null pointer") @(t : T?) {
        var walker = new LogWalker()
        var p : SimpleStruct?
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(p), typeinfo rtti_typeinfo(p))
        }
        // Null pointer: beforePtr → Null → afterPtr (walker still brackets with ptr callbacks)
        var expected = "beforePtr\n"
        expected += "Null\n"
        expected += "afterPtr\n"
        t |> equal(walker.log, expected)
        unsafe { delete walker; }
    }
    t |> run("non-null pointer") @(t : T?) {
        var walker = new LogWalker()
        var s = SimpleStruct(x = 7, y = 1.5)
        var p : SimpleStruct? = unsafe(addr(s))
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(p), typeinfo rtti_typeinfo(p))
        }
        t |> equal(find(walker.log, "beforePtr") != -1, true)
        t |> equal(find(walker.log, "beforeStruct:SimpleStruct") != -1, true)
        t |> equal(find(walker.log, "Int:7") != -1, true)
        t |> equal(find(walker.log, "afterPtr") != -1, true)
        unsafe { delete walker; }
    }
}


// ============================================================
// Lambdas
// ============================================================

[test]
def test_walk_lambda(t : T?) {
    t |> run("lambda with capture") @(t : T?) {
        var walker = new LogWalker()
        var captured_value = 99
        var lam <- @ {
            return captured_value
        }
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(lam), typeinfo rtti_typeinfo(lam))
        }
        t |> equal(find(walker.log, "beforeLambda") != -1, true)
        t |> equal(find(walker.log, "afterLambda") != -1, true)
        unsafe { delete walker; }
    }
}


// ============================================================
// Edge values
// ============================================================

[test]
def test_walk_edge_values(t : T?) {
    t |> run("zero int") @(t : T?) {
        var walker = new LogWalker()
        var x = 0
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Int:0\n")
        unsafe { delete walker; }
    }
    t |> run("negative int") @(t : T?) {
        var walker = new LogWalker()
        var x = -42
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Int:-42\n")
        unsafe { delete walker; }
    }
    t |> run("negative float") @(t : T?) {
        var walker = new LogWalker()
        var x = -1.5
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Float:-1.5\n")
        unsafe { delete walker; }
    }
    t |> run("max int") @(t : T?) {
        var walker = new LogWalker()
        var x = 2147483647
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Int:2147483647\n")
        unsafe { delete walker; }
    }
    t |> run("min int") @(t : T?) {
        var walker = new LogWalker()
        var x = -2147483647 - 1
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Int:-2147483648\n")
        unsafe { delete walker; }
    }
}


// ============================================================
// Deeply nested data
// ============================================================

[test]
def test_deeply_nested(t : T?) {
    t |> run("nested struct in array") @(t : T?) {
        var walker = new LogWalker()
        var root = TreeNode(value = 1)
        root.children |> emplace(TreeNode(value = 2))
        root.children |> emplace(TreeNode(value = 3))
        root.children[0].children |> emplace(TreeNode(value = 4))
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(root), typeinfo rtti_typeinfo(root))
        }
        // All 4 values should be visited
        t |> equal(find(walker.log, "Int:1") != -1, true)
        t |> equal(find(walker.log, "Int:2") != -1, true)
        t |> equal(find(walker.log, "Int:3") != -1, true)
        t |> equal(find(walker.log, "Int:4") != -1, true)
        unsafe { delete walker; }
    }
}


// ============================================================
// Adapter reuse
// ============================================================

[test]
def test_reuse_adapter(t : T?) {
    t |> run("walk twice with same adapter") @(t : T?) {
        var walker = new LogWalker()
        var inscope adapter <- make_data_walker(walker)
        var x = 10
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Int:10\n")
        // Reset log and walk again
        walker.log = ""
        var y = 20
        unsafe {
            adapter |> walk_data(addr(y), typeinfo rtti_typeinfo(y))
        }
        t |> equal(walker.log, "Int:20\n")
        unsafe { delete walker; }
    }
}


// ============================================================
// Scalar counting
// ============================================================

[test]
def test_count_scalars(t : T?) {
    t |> run("count in struct") @(t : T?) {
        var walker = new CountWalker()
        var s = SimpleStruct(x = 1, y = 2.0)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(s), typeinfo rtti_typeinfo(s))
        }
        t |> equal(walker.count, 2)   // x(int) + y(float)
        unsafe { delete walker; }
    }
    t |> run("count in nested struct") @(t : T?) {
        var walker = new CountWalker()
        var s = NestedStruct(inner = SimpleStruct(x = 1, y = 2.0), tag = "test")
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(s), typeinfo rtti_typeinfo(s))
        }
        t |> equal(walker.count, 3)   // inner.x(int) + inner.y(float) + tag(string)
        unsafe { delete walker; }
    }
    t |> run("count in array") @(t : T?) {
        var walker = new CountWalker()
        var arr = [1, 2, 3, 4, 5]
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        t |> equal(walker.count, 5)
        unsafe { delete walker; }
    }
}
