options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def iaac_join(a : array<auto(numt)>) : numt {
    var tf : numt = numt(0)
    for (x in a) {
        tf += x
    }
    return tf
}

[unsafe_operation]
def iaac_findObject(objects : table<string; auto(TVal)>; guid : string) : TVal? {
    var foundObj : TVal?
    get(objects, guid) <| $(obj) {
        unsafe {
            foundObj = reinterpret<TVal?> addr(obj)
        }
    }
    return foundObj
}

[test]
def test_infer_alias_and_alias_ctor(t : T?) {
    t |> run("generic join with auto alias") @(t : T?) {
        var a : array<float>
        push(a, 1.0)
        push(a, 2.0)
        push(a, 3.0)
        t |> equal(iaac_join(a), 6.0)
    }
    t |> run("generic findObject with auto alias") @(t : T?) {
        let tab <- {"one" => 1, "two" => 2}
        unsafe {
            t |> equal(iaac_findObject(tab, "one") ?? 0, 1)
            t |> equal(iaac_findObject(tab, "two") ?? 0, 2)
        }
    }
}
