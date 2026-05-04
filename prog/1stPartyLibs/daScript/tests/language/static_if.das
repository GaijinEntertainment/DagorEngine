options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct SiFoo {
    a, b : int
}

struct SiBar {
    c, d : int
}

[sideeffects]
def si_setA(var obj; val : auto(valT)) {
    if (typeinfo has_field<a>(obj)) {
        if (typeinfo typename(obj.a) == typeinfo typename(type<valT -const>)) {
            obj.a = val
        }
    }
}

let {
    si_canQQ : bool const = false
}

[test]
def test_static_if(t : T?) {
    t |> run("static_if with has_field") @(t : T?) {
        var f : SiFoo
        var b : SiBar
        si_setA(f, 1)
        si_setA(b, 2)
        t |> equal(f.a, 1)
    }
    t |> run("const false eliminates broken code") @(t : T?) {
        if (si_canQQ) {
            broken = badly  // compiled out
        }
        if (2 + 2 == 4) {
            static_assert(true, "nothing to look at")
        } else {
            badly = broken  // compiled out
        }
        t |> success(true)
    }
}
