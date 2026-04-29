options gen2
require dastest/testing_boost

require daslib/fuzzer
require daslib/faker
require daslib/constant_expression

def add_jit(a, b) {
    return a + b
}

def sub_jit(a, b) {
    return a - b
}

def mul_jit(a, b) {
    return a * b
}

def div_jit(a, b) {
    return a / b
}

def mod_jit(a, b) {
    return a % b
}

def test_add(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > add_jit))
    t |> strictEqual(a + b, add_jit(a, b), "a + b")
}

def test_sub(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > sub_jit))
    t |> strictEqual(a - b, sub_jit(a, b), "a - b")
}

def test_mul(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > mul_jit))
    t |> strictEqual(a * b, mul_jit(a, b), "a * b")
}

def test_div(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > div_jit))
    if (b != TT()) {
        t |> strictEqual(a / b, div_jit(a, b), "a / b")
    }
}

def test_div_vec(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > div_jit))
    t |> strictEqual(a / b, div_jit(a, b), "a / b")
}

def test_mod(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > mod_jit))
    if (b != TT()) {
        t |> strictEqual(a % b, mod_jit(a, b), "a % b")
    }
}

def add_equ_jit(a, b) {
    var c = a
    c += b
    return c
}

def sub_equ_jit(a, b) {
    var c = a
    c -= b
    return c
}

def mul_equ_jit(a, b) {
    var c = a
    c *= b
    return c
}

def div_equ_jit(a, b) {
    var c = a
    c /= b
    return c
}

def mod_equ_jit(a, b) {
    var c = a
    c %= b
    return c
}

def test_add_equ(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > add_equ_jit))
    t |> strictEqual(a + b, add_equ_jit(a, b), "a + b")
}

def test_sub_equ(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > sub_equ_jit))
    t |> strictEqual(a - b, sub_equ_jit(a, b), "a - b")
}

def test_mul_equ(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > mul_equ_jit))
    t |> strictEqual(a * b, mul_equ_jit(a, b), "a * b")
}

def test_div_equ(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > div_equ_jit))
    if (b != TT()) {
        t |> strictEqual(a / b, div_equ_jit(a, b), "a / b")
    }
}

def test_div_equ_vec(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > div_equ_jit))
    t |> strictEqual(a / b, div_equ_jit(a, b), "a / b")
}

def test_mod_equ(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : TT) : void > mod_equ_jit))
    if (b != TT()) {
        t |> strictEqual(a % b, mod_equ_jit(a, b), "a % b")
    }
}

[test]
def test_arithmetics_op2(t : T?) {
    t |> run("+, -, *, /, %") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_and_vector_op2(t, fake, "test_add")
            fuzz_numeric_and_vector_op2(t, fake, "test_sub")
            fuzz_numeric_and_vector_op2(t, fake, "test_mul")
            fuzz_numeric_and_vector_op2(t, fake, "test_div")
            fuzz_numeric_op2(t, fake, "test_mod")
        }
    }

    t |> run("+=, -=, *=, /=, %=") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_and_vector_op2(t, fake, "test_add_equ")
            fuzz_numeric_and_vector_op2(t, fake, "test_sub_equ")
            fuzz_numeric_and_vector_op2(t, fake, "test_mul_equ")
            fuzz_numeric_and_vector_op2(t, fake, "test_div_equ")
            fuzz_numeric_op2(t, fake, "test_mod_equ")
        }
    }
}

[test]
def test_arithmetics_vec_and_scalar_op2(t : T?) {
    t |> run("* / vector vs salar") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_vec_scal_op2(t, fake, "test_mul")
            fuzz_numeric_vec_scal_op2(t, fake, "test_div_vec")
            fuzz_numeric_scal_vec_op2(t, fake, "test_mul")
            fuzz_numeric_scal_vec_op2(t, fake, "test_div_vec")
        }
    }

    t |> run("*= /= vector vs scalar") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_vec_scal_op2(t, fake, "test_mul_equ")
            fuzz_numeric_vec_scal_op2(t, fake, "test_div_equ_vec")
        }
    }
}

def post_inc_jit(var a) {
    return a++
}

def post_dec_jit(var a) {
    return a--
}

def pre_inc_jit(var a) {
    ++a
    return a
}

def pre_dec_jit(var a) {
    --a
    return a
}

def test_post_inc(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > post_inc_jit))
    if (a != a) {
        return
    }
    t |> strictEqual(a, post_inc_jit(a), "return a ++")
}

def test_post_dec(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > post_dec_jit))
    if (a != a) {
        return
    }
    t |> strictEqual(a, post_dec_jit(a), "return a --")
}

def test_pre_inc(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > pre_inc_jit))
    if (a != a) {
        return
    }
    t |> strictEqual(a + TT(1), pre_inc_jit(a), "return ++ a")
}

def test_pre_dec(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > pre_dec_jit))
    if (a != a) {
        return
    }
    t |> strictEqual(a - TT(1), pre_dec_jit(a), "return -- a")
}

def neg_jit(a) {
    return -a
}

def test_neg(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > neg_jit))
    if (a != a) {
        return
    }
    t |> strictEqual(-a, neg_jit(a), "-a {a}")
}

[test]
def test_arithmetics_op1(t : T?) {
    t |> run("-") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_and_vector_op1(t, fake, "test_neg")
        }
    }
    t |> run("++, --, +++, ---") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op1(t, fake, "test_post_inc")
            fuzz_numeric_op1(t, fake, "test_post_dec")
            fuzz_numeric_op1(t, fake, "test_pre_inc")
            fuzz_numeric_op1(t, fake, "test_pre_dec")
        }
    }
}
