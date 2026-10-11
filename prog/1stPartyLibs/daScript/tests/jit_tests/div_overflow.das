options gen2
require dastest/testing_boost

def div_jit(a : int, b : int) {
    return a / b
}

def mod_jit(a : int, b : int) {
    return a % b
}

def div64_jit(a : int64, b : int64) {
    return a / b
}

def mod64_jit(a : int64, b : int64) {
    return a % b
}

def set_div_jit(a : int, b : int) {
    var r = a
    r /= b
    return r
}

def set_mod_jit(a : int, b : int) {
    var r = a
    r %= b
    return r
}

def set_div64_jit(a : int64, b : int64) {
    var r = a
    r /= b
    return r
}

def set_mod64_jit(a : int64, b : int64) {
    var r = a
    r %= b
    return r
}

def expect_div_panic(t : T?; msg : string; blk : block) {
    var failed = false
    try {
        invoke(blk)
    } recover {
        failed = true
    }
    t |> success(failed, msg)
}

[test]
def test_division_overflow(t : T?) {
    t |> run("modulo folds to zero") @@(t : T?) {
        t |> equal(mod_jit(INT_MIN, -1), 0, "INT_MIN % -1")
        t |> equal(mod64_jit(LONG_MIN, -1l), 0l, "LONG_MIN % -1")
        t |> equal(set_mod_jit(INT_MIN, -1), 0, "INT_MIN %= -1")
        t |> equal(set_mod64_jit(LONG_MIN, -1l), 0l, "LONG_MIN %= -1")
    }
    t |> run("division panics") @@(t : T?) {
        expect_div_panic(t, "INT_MIN / -1") $ {
            print("{div_jit(INT_MIN, -1)}")
        }
        expect_div_panic(t, "LONG_MIN / -1") $ {
            print("{div64_jit(LONG_MIN, -1l)}")
        }
        expect_div_panic(t, "INT_MIN /= -1") $ {
            print("{set_div_jit(INT_MIN, -1)}")
        }
        expect_div_panic(t, "LONG_MIN /= -1") $ {
            feint("{set_div64_jit(LONG_MIN, -1l)}")
        }
    }
    t |> run("plain values unaffected") @@(t : T?) {
        t |> equal(div_jit(-14, 7), -2, "-14 / 7")
        t |> equal(mod_jit(-14, 3), -2, "-14 % 3")
        t |> equal(div64_jit(-14l, 7l), -2l, "-14 / 7 int64")
    }
}
