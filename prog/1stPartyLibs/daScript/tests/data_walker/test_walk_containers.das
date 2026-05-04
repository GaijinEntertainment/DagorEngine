// Tests for DapiDataWalker — containers (arrays, dim, tables)
//
// Covers: dynamic array, fixed array (dim), tables (string keys, int keys),
//         empty containers, single-element, array of structs, array of strings,
//         array of tuples, table with multiple entries, key-value ordering

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dw_common


// ============================================================
// Dynamic arrays
// ============================================================

[test]
def test_walk_array(t : T?) {
    t |> run("array of ints") @(t : T?) {
        var walker = new LogWalker()
        var arr = [10, 20, 30]
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        var expected = "beforeArray\n"
        expected += "beforeArrayData:3\n"
        expected += "beforeElem:0:false\n"
        expected += "Int:10\n"
        expected += "afterElem:0:false\n"
        expected += "beforeElem:1:false\n"
        expected += "Int:20\n"
        expected += "afterElem:1:false\n"
        expected += "beforeElem:2:true\n"
        expected += "Int:30\n"
        expected += "afterElem:2:true\n"
        expected += "afterArrayData:3\n"
        expected += "afterArray\n"
        t |> equal(walker.log, expected)
        unsafe { delete walker; }
    }
    t |> run("empty array") @(t : T?) {
        var walker = new LogWalker()
        var arr : array<int>
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        var expected = "beforeArray\n"
        expected += "beforeArrayData:0\n"
        expected += "afterArrayData:0\n"
        expected += "afterArray\n"
        t |> equal(walker.log, expected)
        unsafe { delete walker; }
    }
    t |> run("single element array") @(t : T?) {
        var walker = new LogWalker()
        var arr = [42]
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        // Single element: index=0, last=true
        t |> equal(find(walker.log, "beforeElem:0:true") != -1, true)
        t |> equal(find(walker.log, "Int:42") != -1, true)
        unsafe { delete walker; }
    }
    t |> run("array of structs") @(t : T?) {
        var walker = new LogWalker()
        var arr : array<SimpleStruct>
        arr |> push(SimpleStruct(x = 1, y = 1.0))
        arr |> push(SimpleStruct(x = 2, y = 2.0))
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        t |> equal(find(walker.log, "beforeArray") != -1, true)
        t |> equal(find(walker.log, "beforeStruct:SimpleStruct") != -1, true)
        t |> equal(find(walker.log, "Int:1") != -1, true)
        t |> equal(find(walker.log, "Int:2") != -1, true)
        unsafe { delete walker; }
    }
    t |> run("array of strings") @(t : T?) {
        var walker = new LogWalker()
        var arr = ["foo", "bar", "baz"]
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        t |> equal(find(walker.log, "String:foo") != -1, true)
        t |> equal(find(walker.log, "String:bar") != -1, true)
        t |> equal(find(walker.log, "String:baz") != -1, true)
        unsafe { delete walker; }
    }
    t |> run("array of tuples") @(t : T?) {
        var walker = new LogWalker()
        var arr : array<tuple<int; string>>
        arr |> push((1, "one"))
        arr |> push((2, "two"))
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        t |> equal(find(walker.log, "beforeArray") != -1, true)
        t |> equal(find(walker.log, "beforeTuple") != -1, true)
        t |> equal(find(walker.log, "Int:1") != -1, true)
        t |> equal(find(walker.log, "String:one") != -1, true)
        t |> equal(find(walker.log, "Int:2") != -1, true)
        t |> equal(find(walker.log, "String:two") != -1, true)
        unsafe { delete walker; }
    }
}


// ============================================================
// Fixed-size arrays (dim)
// ============================================================

[test]
def test_walk_dim(t : T?) {
    t |> run("int dim") @(t : T?) {
        var walker = new LogWalker()
        var arr : int[3]
        arr[0] = 100
        arr[1] = 200
        arr[2] = 300
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        t |> equal(find(walker.log, "beforeDim") != -1, true)
        t |> equal(find(walker.log, "Int:100") != -1, true)
        t |> equal(find(walker.log, "Int:200") != -1, true)
        t |> equal(find(walker.log, "Int:300") != -1, true)
        t |> equal(find(walker.log, "afterDim") != -1, true)
        unsafe { delete walker; }
    }
}


// ============================================================
// Tables
// ============================================================

[test]
def test_walk_table(t : T?) {
    t |> run("table string to int") @(t : T?) {
        var walker = new LogWalker()
        var tab : table<string; int>
        tab |> insert("alpha", 1)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(tab), typeinfo rtti_typeinfo(tab))
        }
        t |> equal(find(walker.log, "beforeTable") != -1, true)
        t |> equal(find(walker.log, "String:alpha") != -1, true)
        t |> equal(find(walker.log, "Int:1") != -1, true)
        t |> equal(find(walker.log, "afterTable") != -1, true)
        unsafe { delete walker; }
    }
    t |> run("empty table") @(t : T?) {
        var walker = new LogWalker()
        var tab : table<string; int>
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(tab), typeinfo rtti_typeinfo(tab))
        }
        var expected = "beforeTable\n"
        expected += "afterTable\n"
        t |> equal(walker.log, expected)
        unsafe { delete walker; }
    }
    t |> run("table key-value order") @(t : T?) {
        // Each entry: beforeKey → key value → afterKey → beforeVal → val value → afterVal
        var walker = new LogWalker()
        var tab : table<string; int>
        tab |> insert("x", 42)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(tab), typeinfo rtti_typeinfo(tab))
        }
        var key_pos = find(walker.log, "beforeKey")
        var key_after_pos = find(walker.log, "afterKey")
        var val_pos = find(walker.log, "beforeVal")
        var val_after_pos = find(walker.log, "afterVal")
        // Key should come before value
        t |> equal(key_pos < key_after_pos, true)
        t |> equal(key_after_pos < val_pos, true)
        t |> equal(val_pos < val_after_pos, true)
        unsafe { delete walker; }
    }
    t |> run("table int to string") @(t : T?) {
        var walker = new LogWalker()
        var tab : table<int; string>
        tab |> insert(42, "answer")
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(tab), typeinfo rtti_typeinfo(tab))
        }
        t |> equal(find(walker.log, "Int:42") != -1, true)
        t |> equal(find(walker.log, "String:answer") != -1, true)
        unsafe { delete walker; }
    }
    t |> run("table with 3 entries") @(t : T?) {
        var walker = new LogWalker()
        var tab : table<string; int>
        tab |> insert("a", 1)
        tab |> insert("b", 2)
        tab |> insert("c", 3)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(tab), typeinfo rtti_typeinfo(tab))
        }
        // All 3 keys and values should appear (order may vary due to hash table)
        t |> equal(find(walker.log, "String:a") != -1, true)
        t |> equal(find(walker.log, "String:b") != -1, true)
        t |> equal(find(walker.log, "String:c") != -1, true)
        t |> equal(find(walker.log, "Int:1") != -1, true)
        t |> equal(find(walker.log, "Int:2") != -1, true)
        t |> equal(find(walker.log, "Int:3") != -1, true)
        unsafe { delete walker; }
    }
}
