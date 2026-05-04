// Tests for DapiDataWalker — tuples and variants
//
// Covers: tuple (2-element, 3-element), variant (int/string/float alternatives)

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dw_common


[test]
def test_walk_tuple(t : T?) {
    t |> run("int-string tuple") @(t : T?) {
        var walker = new LogWalker()
        var tup : tuple<int; string> = (42, "hello")
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(tup), typeinfo rtti_typeinfo(tup))
        }
        var expected = "beforeTuple\n"
        expected += "beforeTupleEntry:0:false\n"
        expected += "Int:42\n"
        expected += "afterTupleEntry:0:false\n"
        expected += "beforeTupleEntry:1:true\n"
        expected += "String:hello\n"
        expected += "afterTupleEntry:1:true\n"
        expected += "afterTuple\n"
        t |> equal(walker.log, expected)
        unsafe { delete walker; }
    }
    t |> run("3-element tuple") @(t : T?) {
        var walker = new LogWalker()
        var tup : tuple<int; float; bool> = (1, 2.0, true)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(tup), typeinfo rtti_typeinfo(tup))
        }
        // Middle element should not be last
        t |> equal(find(walker.log, "beforeTupleEntry:1:false") != -1, true)
        // Last element should be last
        t |> equal(find(walker.log, "beforeTupleEntry:2:true") != -1, true)
        unsafe { delete walker; }
    }
}


[test]
def test_walk_variant(t : T?) {
    t |> run("variant int alternative") @(t : T?) {
        var walker = new LogWalker()
        var v : MyVariant = MyVariant(i = 42)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(find(walker.log, "beforeVariant") != -1, true)
        t |> equal(find(walker.log, "Int:42") != -1, true)
        t |> equal(find(walker.log, "afterVariant") != -1, true)
        // Should NOT contain String or Float callbacks
        t |> equal(find(walker.log, "String:") == -1, true)
        t |> equal(find(walker.log, "Float:") == -1, true)
        unsafe { delete walker; }
    }
    t |> run("variant string alternative") @(t : T?) {
        var walker = new LogWalker()
        var v : MyVariant = MyVariant(s = "hello")
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(find(walker.log, "beforeVariant") != -1, true)
        t |> equal(find(walker.log, "String:hello") != -1, true)
        t |> equal(find(walker.log, "afterVariant") != -1, true)
        t |> equal(find(walker.log, "Int:") == -1, true)
        unsafe { delete walker; }
    }
    t |> run("variant float alternative") @(t : T?) {
        var walker = new LogWalker()
        var v : MyVariant = MyVariant(f = 9.5)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(v), typeinfo rtti_typeinfo(v))
        }
        t |> equal(find(walker.log, "Float:9.5") != -1, true)
        unsafe { delete walker; }
    }
}
