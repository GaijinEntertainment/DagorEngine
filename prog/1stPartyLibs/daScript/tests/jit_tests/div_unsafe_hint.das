options gen2
require dastest/testing_boost

def div_checked(a : int, b : int) {
    return a / b
}

[hint(unsafe_division_check)]
def div_unchecked(a : int, b : int) {
    return a / b
}

[hint(unsafe_division_check)]
def mod_unchecked(a : int, b : int) {
    return a % b
}

[hint(unsafe_division_check)]
def mod64_unchecked(a : int64, b : int64) {
    return a % b
}

[hint(unsafe_division_check)]
def set_div_unchecked(a : int, b : int) {
    var r = a
    r /= b
    return r
}

[hint(unsafe_division_check)]
def set_mod_unchecked(a : int, b : int) {
    var r = a
    r %= b
    return r
}

[hint(unsafe_division_check)]
def count_divisors(n : int) {
    var count = 0
    for (i in range(1, n + 1)) {
        if (n % i == 0) {
            count++
        }
    }
    return count
}

[test]
def test_unsafe_division_check(t : T?) {
    t |> run("plain values under the hint") @@(t : T?) {
        t |> equal(div_unchecked(-14, 7), -2, "-14 / 7")
        t |> equal(mod_unchecked(-14, 3), -2, "-14 % 3")
        t |> equal(mod64_unchecked(-14l, 3l), -2l, "-14 % 3 int64")
        t |> equal(set_div_unchecked(100, 7), 14, "100 /= 7")
        t |> equal(set_mod_unchecked(100, 7), 2, "100 %= 7")
        t |> equal(count_divisors(360), 24, "divisors of 360")
    }
    t |> run("the hint stays with its function") @@(t : T?) {
        var panicked = false
        try {
            feint("{div_checked(INT_MIN, -1)}")
        } recover {
            panicked = true
        }
        t |> success(panicked, "an un-hinted INT_MIN / -1 beside hinted functions still panics")
    }
}
