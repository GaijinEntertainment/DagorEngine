options gen2
require dastest/testing_boost

def inc_local_ref(var a : int&) {
    unsafe {
        var b & = a
        return b++
    }
}

def inc_arg_ref(var a : int&) {
    return a++
}

var global_ref = 13

def get_global_ref : int & {
    unsafe {
        return global_ref
    }
}

[test]
def test_ref(t : T?) {
    t |> run("local ref") @(t : T?) {
        var a = 1
        let b = inc_local_ref(a)
        t |> equal(a, 2)
        t |> equal(b, 1)
        t |> success(!jit_enabled() || is_jit_function(@@inc_local_ref))
    }
    t |> run("arg ref") @(t : T?) {
        var a = 1
        let b = inc_arg_ref(a)
        t |> equal(a, 2)
        t |> equal(b, 1)
        t |> success(!jit_enabled() || is_jit_function(@@inc_arg_ref))
    }
    t |> run("global ref") @(t : T?) {
        t |> equal(13, global_ref)
        t |> equal(13, get_global_ref())
        get_global_ref() ++
        t |> equal(14, global_ref)
        t |> equal(14, get_global_ref())
        global_ref ++
        t |> equal(15, global_ref)
        t |> equal(15, get_global_ref())
        t |> success(!jit_enabled() || is_jit_function(@@get_global_ref))
    }
}

