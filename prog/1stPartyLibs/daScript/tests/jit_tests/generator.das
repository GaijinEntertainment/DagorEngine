options gen2
require dastest/testing_boost

def test_for {
    return <- generator<int>() <| $() {
        for (t in range(10)) {
            yield t
        }
        return false
    }
}

[test]
def test_generator(t : T?) {
    t |> run("make generator") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_for))
        var count = 0
        for (i, j in range(10), test_for()) {
            t |> equal(i, j)
            count ++
        }
        t |> equal(10, count)
    }
}

