// Tests for DapiDataWalker — in-place mutation via walker callbacks
//
// Covers: mutate scalars, mutate struct fields, mutate array elements,
//         mutate strings in nested structs and arrays

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dw_common


// ============================================================
// Local walker classes for mutation tests
// ============================================================

// Doubles every int and float encountered
class Doubler : DapiDataWalker {
    def override Int(var value : int&) : void {
        value *= 2
    }
    def override Float(var value : float&) : void {
        value *= 2.0
    }
}

// Converts all strings to upper case
class StringUpperWalker : DapiDataWalker {
    def override String(var value : string&) : void {
        value = to_upper(value)
    }
}


// ============================================================
// Tests
// ============================================================

[test]
def test_mutation(t : T?) {
    t |> run("mutate scalar int") @(t : T?) {
        var walker = new Doubler()
        var x = 21
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(x, 42)
        unsafe { delete walker; }
    }
    t |> run("mutate struct fields") @(t : T?) {
        var walker = new Doubler()
        var s = SimpleStruct(x = 5, y = 1.5)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(s), typeinfo rtti_typeinfo(s))
        }
        t |> equal(s.x, 10)
        t |> equal(s.y, 3.0)
        unsafe { delete walker; }
    }
    t |> run("mutate array elements") @(t : T?) {
        var walker = new Doubler()
        var arr = [1, 2, 3]
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        t |> equal(arr[0], 2)
        t |> equal(arr[1], 4)
        t |> equal(arr[2], 6)
        unsafe { delete walker; }
    }
}


[test]
def test_mutate_nested(t : T?) {
    t |> run("mutate strings in struct") @(t : T?) {
        var walker = new StringUpperWalker()
        var s = NestedStruct(inner = SimpleStruct(x = 1, y = 2.0), tag = "hello")
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(s), typeinfo rtti_typeinfo(s))
        }
        t |> equal(s.tag, "HELLO")
        unsafe { delete walker; }
    }
    t |> run("mutate strings in array") @(t : T?) {
        var walker = new StringUpperWalker()
        var arr = ["abc", "def"]
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        t |> equal(arr[0], "ABC")
        t |> equal(arr[1], "DEF")
        unsafe { delete walker; }
    }
}
