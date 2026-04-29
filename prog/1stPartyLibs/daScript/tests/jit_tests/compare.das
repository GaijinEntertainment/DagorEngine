options gen2
require dastest/testing_boost

require daslib/fuzzer
require daslib/faker

def eq_jit(a, b) {
    return a == b
}

def test_eq(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > eq_jit))
    t |> equal(a == b, eq_jit(a, b), "a == b")
}

def neq_jit(a, b) {
    return a != b
}

def test_neq(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > neq_jit))
    t |> equal(a != b, neq_jit(a, b), "a != b")
}

def less_jit(a, b) {
    return a < b
}

def test_less(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > less_jit))
    t |> equal(a < b, less_jit(a, b), "a < b")
}

def leq_jit(a, b) {
    return a <= b
}

def test_leq(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > leq_jit))
    t |> equal(a <= b, leq_jit(a, b), "a <= b")
}

def gt_jit(a, b) {
    return a > b
}

def test_gt(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > gt_jit))
    t |> equal(a > b, gt_jit(a, b), "a > b")
}

def geq_jit(a, b) {
    return a >= b
}

def test_geq(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > geq_jit))
    t |> equal(a >= b, geq_jit(a, b), "a >= b")
}

[test]
def test_arithmetics_op2(t : T?) {
    t |> run("== != >= <= > <") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_eq_neq_op2(t, fake, "test_eq")
            fuzz_eq_neq_op2(t, fake, "test_neq")
            fuzz_compareable_op2(t, fake, "test_less")
            fuzz_compareable_op2(t, fake, "test_gt")
            fuzz_compareable_op2(t, fake, "test_leq")
            fuzz_compareable_op2(t, fake, "test_geq")
        }
    }
}
