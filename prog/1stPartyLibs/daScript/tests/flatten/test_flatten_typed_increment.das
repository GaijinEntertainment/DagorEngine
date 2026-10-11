options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten

// A predicated `++` / `--` lowers to `x + 1` / `x - 1`; the synthesized 1 must carry x's
// own type, or a uint / int64 / uint64 / float counter fails to type-check.

[flatten]
def count_u(x : int) : uint {
    var u = 0u
    for (i in range(4)) {
        if (i > x) {
            u++
        }
    }
    return u
}

[flatten]
def count_l(x : int) : int64 {
    var l = 0l
    for (i in range(4)) {
        if (i > x) {
            l--
        }
    }
    return l
}

[flatten]
def count_ul(x : int) : uint64 {
    var ul = 10ul
    for (i in range(4)) {
        if (i > x) {
            ul--
        }
    }
    return ul
}

[flatten]
def count_f(x : int) : float {
    var f = 0.0
    for (i in range(4)) {
        if (i > x) {
            f++
        }
    }
    return f
}

[flatten]
def count_d(x : int) : double {
    var d = 0.0lf
    for (i in range(4)) {
        if (i > x) {
            d--
        }
    }
    return d
}

[test]
def test_predicated_increment_keeps_the_counter_type(t : T?) {
    t |> run("uint / int64 / uint64 / float / double counters under a runtime predicate") @(t : T?) {
        t |> equal(count_u(1), 2u)
        t |> equal(count_l(1), -2l)
        t |> equal(count_ul(1), 8ul)
        t |> equal(count_f(1), 2.0)
        t |> equal(count_d(1), -2.0lf)
    }
}
