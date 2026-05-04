options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct GpiFoo {
    i : int = 5
    a : array<int>
}

var g_gpi : GpiFoo? = new GpiFoo()

var g_gpi_pg = new GpiFoo()

[test]
def test_global_ptr_init(t : T?) {
    t |> run("global pointer struct access") @(t : T?) {
        push(g_gpi.a, 10)
        t |> equal(g_gpi.i, 5)
    }
    t |> run("global pointer default init") @(t : T?) {
        t |> equal(g_gpi_pg.i, 5)
    }
}
