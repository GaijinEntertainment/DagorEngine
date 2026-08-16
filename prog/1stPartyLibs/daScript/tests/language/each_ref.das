// each_ref — lambda-to-ref-iterator, both entry points: user-written lambdas and the
// ref-generator lowering (generator<T&> compiles into an each_ref call). Writes through
// the iterated ref prove the elements are genuine references, not copies.
options gen2
require dastest/testing_boost public

var g_src : array<int>

def make_lam() : lambda<(var a : int?&) : bool> {
    var idx = 0  // nolint:LINT003 — the lambda's captured copy mutates; a let capture is const
    return <- @(var a : int?&) : bool {
        if (idx >= length(g_src)) {
            return false
        }
        unsafe {
            a = addr(g_src[idx])
        }
        idx++
        return true
    }
}

[test]
def test_each_ref(t : T?) {
    t |> run("user lambda yields writable refs") @(t : T?) {
        g_src <- [10, 20, 30]
        let lam <- make_lam()
        var sum = 0
        for (x in each_ref(lam)) {
            sum += x
            x = x * 10
        }
        t |> equal(60, sum)
        t |> equal(100, g_src[0])
        t |> equal(200, g_src[1])
        t |> equal(300, g_src[2])
    }
    t |> run("ref generator lowers to each_ref") @(t : T?) {
        g_src <- [1, 2, 3]
        var gen <- generator<int&> {
            for (i in range(length(g_src))) {
                yield g_src[i]
            }
            return false
        }
        for (x in gen) {
            x += 5
        }
        t |> equal(6, g_src[0])
        t |> equal(7, g_src[1])
        t |> equal(8, g_src[2])
    }
}
