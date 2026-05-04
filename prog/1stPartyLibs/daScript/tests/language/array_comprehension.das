options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[test]
def test_array_comprehension(t : T?) {
    t |> run("square comprehension") @(t : T?) {
        let q <- [for (x in range(10)); x * x]
        for (y, qV in range(10), q) {
            t |> equal(qV, y * y)
        }
    }
    t |> run("filtered comprehension") @(t : T?) {
        let a <- [for (x in range(10)); x * x; where(x & 1) == 0]
        var count = 0
        for (z in range(10)) {
            if ((z & 1) == 0) {
                t |> equal(a[count++], z * z)
            }
        }
    }
}
