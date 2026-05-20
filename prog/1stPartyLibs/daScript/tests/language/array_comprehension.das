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
    t |> run("multiline table comprehension (issue #2459)") @(t : T?) {
        var name_to_index <- {
            for (i in range(5));
            (i, i)
        }
        for (i in range(5)) {
            t |> equal(name_to_index[i], i)
        }
    }
    t |> run("multiline array comprehension square form") @(t : T?) {
        let q <- [
            for (x in range(4));
            x * x
        ]
        for (y, qV in range(4), q) {
            t |> equal(qV, y * y)
        }
    }
    t |> run("multiline filtered comprehension brace form") @(t : T?) {
        var even_squares <- {
            for (x in range(10));
            (x, x * x);
            where(x & 1) == 0
        }
        for (z in range(10)) {
            if ((z & 1) == 0) {
                t |> equal(even_squares?[z] ?? -1, z * z)
            } else {
                t |> equal(even_squares?[z] ?? -1, -1)
            }
        }
    }
}
