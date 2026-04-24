options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

let {
    DIM_N = 2
    DIM_M = 3
}

var {
    g_dim_a : float[DIM_N][DIM_M]
}

[test]
def test_fixed_array_2d(t : T?) {
    t |> run("write and read 2D dim") @(t : T?) {
        for (y in range(2)) {
            for (x in range(3)) {
                var f = float(y * 10 + x)
                g_dim_a[y][x] = f
            }
        }
        for (y in range(2)) {
            for (x in range(3)) {
                var f = float(y * 10 + x)
                t |> equal(g_dim_a[y][x], f)
            }
        }
    }
    t |> run("pointer arithmetic over dim") @(t : T?) {
        for (y in range(2)) {
            for (x in range(3)) {
                g_dim_a[y][x] = float(y * 10 + x)
            }
        }
        unsafe {
            var pa = addr(g_dim_a[0][0])
            for (y in range(2)) {
                for (x in range(3)) {
                    var o = x + y * 3
                    var f = float(y * 10 + x)
                    t |> equal(pa[o], f)
                }
            }
        }
    }
    t |> run("dim to string") @(t : T?) {
        for (y in range(2)) {
            for (x in range(3)) {
                g_dim_a[y][x] = float(y * 10 + x)
            }
        }
        t |> equal("{g_dim_a}", "[ [ 0, 1, 2], [ 10, 11, 12]]")
        var a0 = g_dim_a[0]
        t |> equal("{a0}", "[ 0, 1, 2]")
        var a1 = g_dim_a[1]
        t |> equal("{a1}", "[ 10, 11, 12]")
    }
}
