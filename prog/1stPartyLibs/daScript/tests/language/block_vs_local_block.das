options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct BvlFoo {
    a : int
}

var g_bvl = 0

def radd_bvl(var ext : int; b : block<(arg : int)>) {
    invoke(b, ext)
}

[test]
def test_block_vs_local_block(t : T?) {
    t |> run("block via pipe vs local block variable") @(t : T?) {
        let localVar = BvlFoo(a = 34)
        var localBlock = $(arg : int) {
            g_bvl = localVar.a + arg
        }
        g_bvl = 0
        radd_bvl(100) <| $(arg : int) {
            g_bvl = localVar.a + arg
        }
        t |> equal(g_bvl, 134)
        g_bvl = 0
        invoke(localBlock, 100)
        t |> equal(g_bvl, 134)
    }
}
