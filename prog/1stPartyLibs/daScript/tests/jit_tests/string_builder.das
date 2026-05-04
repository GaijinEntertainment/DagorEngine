options gen2
require dastest/testing_boost

require strings

def abc_jit(a : int; b : float; c : string) {
    return "a = {a} b = {b} c = {c}"
}

[test]
def test_ref(t : T?) {
    t |> run("string builder") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@abc_jit))
        t |> equal("a = 1 b = 2 c = 3", abc_jit(1, 2., "3"))
    }
}


