options gen2
require dastest/testing_boost public
require daslib/math_boost

[test]
def test_aot_is_active(t : T?) {
    // Under fail_on_no_aot, reaching this point proves all functions AOT-linked
    t |> run("aot linked successfully") @(t : T?) {
        t |> success(true)
    }
}

[test]
def test_basic_math(t : T?) {
    t |> run("integer arithmetic") @(t : T?) {
        t |> equal(2 + 3, 5)
        t |> equal(10 - 3, 7)
        t |> equal(4 * 5, 20)
        t |> equal(15 / 3, 5)
        t |> equal(17 % 5, 2)
    }
    t |> run("float arithmetic") @(t : T?) {
        t |> equal(2.0 + 3.0, 5.0)
        t |> equal(10.0 - 3.0, 7.0)
        t |> equal(4.0 * 5.0, 20.0)
        t |> equal(15.0 / 3.0, 5.0)
    }
    t |> run("math functions") @(t : T?) {
        t |> equal(abs(-5), 5)
        t |> equal(min(3, 7), 3)
        t |> equal(max(3, 7), 7)
        t |> equal(clamp(5, 0, 3), 3)
        t |> equal(clamp(-1, 0, 3), 0)
    }
}

def add_values(a, b : int) : int {
    return a + b
}

def factorial(n : int) : int {
    if (n <= 1) {
        return 1
    }
    return n * factorial(n - 1)
}

[test]
def test_function_calls(t : T?) {
    t |> run("simple function") @(t : T?) {
        t |> equal(add_values(3, 4), 7)
        t |> equal(add_values(-1, 1), 0)
    }
    t |> run("recursive function") @(t : T?) {
        t |> equal(factorial(1), 1)
        t |> equal(factorial(5), 120)
        t |> equal(factorial(10), 3628800)
    }
}

def apply_op(a, b : int; op : function<(x : int; y : int) : int>) : int {
    return invoke(op, a, b)
}

def fn_add(x, y : int) : int {
    return x + y
}

def fn_mul(x, y : int) : int {
    return x * y
}

[test]
def test_function_pointers(t : T?) {
    t |> run("function pointer call") @(t : T?) {
        t |> equal(apply_op(3, 4, @@fn_add), 7)
        t |> equal(apply_op(3, 4, @@fn_mul), 12)
    }
}
