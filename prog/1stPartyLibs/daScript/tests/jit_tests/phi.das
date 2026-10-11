options gen2
require dastest/testing_boost

require daslib/faker
require daslib/constant_expression

[jit, sideeffects]
def test_min(a, b : int) {
    return a < b ? a : b    // nolint:PERF015 -- the ternary is the PHI shape under test; min() would not emit one
}

[jit, sideeffects]
def test_min(a, b, c : int) {
    if (a < b) {
        return a < c ? a : c    // nolint:PERF015 -- the ternary is the PHI shape under test; min() would not emit one
    } else {
        return b < c ? b : c    // nolint:PERF015 -- the ternary is the PHI shape under test; min() would not emit one
    }
}

[test]
def test_phi(t : T?) {

    t |> success(!jit_enabled() || is_jit_function(@@ < (a : int; b : int) : bool > test_min))
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : int; b : int; c : int) : bool > test_min))

    t |> equal(1, test_min(1, 2))
    t |> equal(1, test_min(2, 1))
    t |> equal(1, test_min(1, 2, 3))
    t |> equal(1, test_min(1, 3, 2))
    t |> equal(1, test_min(2, 1, 3))
    t |> equal(1, test_min(2, 3, 1))
    t |> equal(1, test_min(3, 1, 2))
    t |> equal(1, test_min(3, 2, 1))

    t |> run("min2") @(t : T?) {
        var fake <- Faker()
        let a = fake |> random_int
        let b = fake |> random_int
        t |> equal(test_min(a, b), a < b ? a : b)    // nolint:PERF015 -- the oracle mirrors the ternary under test
    }

    t |> run("min3") @(t : T?) {
        var fake <- Faker()
        let a = fake |> random_int
        let b = fake |> random_int
        let c = fake |> random_int
        t |> equal(test_min(a, b, c), a < b ? (a < c ? a : c) : (b < c ? b : c))    // nolint:PERF015 -- the oracle mirrors the ternaries under test
    }
}
