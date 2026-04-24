options gen2
require dastest/testing_boost

def fnmad(a, b, c : int) {
    return a * b + c
}

[sideeffects]
def jit_addr {
    return @@fnmad
}

[sideeffects]
def jit_pass_addr {
    return take_fun(@@fnmad)
}

def take_fun(fn : function<(a, b, c : int) : int>) {
    return invoke(fn, 1, 2, 3)
}

[test]
def test_assert(t : T?) {
    t |> success(!jit_enabled() || is_jit_function(@@jit_addr))
    t |> success(!jit_enabled() || is_jit_function(@@jit_pass_addr))
    let res = invoke(jit_addr(), 1, 2, 3)
    t |> equal(5, res)
    let res_t = jit_pass_addr()
    t |> equal(5, res_t)
}

