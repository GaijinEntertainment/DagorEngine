// Empty-body for loops. Codegen must not mis-select the fused single-statement
// for-node (a dead single statement is 1 AST node but 0 SimNodes), and an empty
// body over a SIDE-EFFECTING source must still evaluate the source — the empty-loop
// elision is gated on side-effect-free sources only.
options gen2
require dastest/testing_boost public

var g_src_calls = 0

def counting_source : array<int> {
    g_src_calls++
    return [1, 2, 3]
}

[test]
def test_for_empty_body(t : T?) {
    t |> run("empty body over array still iterates the source") @(t : T?) {
        var iters = 0
        for (_x in [1, 2, 3]) {
        } finally {
            iters++
        }
        t |> equal(iters, 3)
    }
    t |> run("empty body over range still iterates the source") @(t : T?) {
        var iters = 0
        for (_i in range(5)) {
        } finally {
            iters++
        }
        t |> equal(iters, 5)
    }
    t |> run("empty body over fixed array still iterates the source") @(t : T?) {
        let fa = fixed_array(7, 8, 9)
        var iters = 0
        for (_x in fa) {
        } finally {
            iters++
        }
        t |> equal(iters, 3)
    }
    t |> run("pure empty loop is a no-op") @(t : T?) {
        for (_x in [1, 2, 3]) {
        }
        t |> success(true)
    }
    t |> run("side-effecting source is NOT elided") @(t : T?) {
        g_src_calls = 0
        for (_x in counting_source()) {
        }
        t |> equal(g_src_calls, 1)
    }
}
