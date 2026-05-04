options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def test_while_continue(t : T?) {
    var x = 0
    var y = 0
    while (x < 10) {
        x++
        if ((x & 1) == 0) {
            continue
        }
        y += x
    }
    t |> equal(y, 1 + 3 + 5 + 7 + 9)
}

def test_for_continue_impl(t : T?) {
    var y = 0
    for (x in range(10)) {
        if ((x & 1) == 0) {
            continue
        }
        y += x
    }
    t |> equal(y, 1 + 3 + 5 + 7 + 9)
}

def test_complex_for_continue(t : T?) {
    var y = 0
    let arr = fixed_array<int>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
    for (x, s in range(10), arr) {
        t |> equal(x, s)
        if ((x & 1) == 0) {
            continue
        }
        y += x
    }
    t |> equal(y, 1 + 3 + 5 + 7 + 9)
}

[test]
def test_continue(t : T?) {
    t |> run("while continue") @(t : T?) {
        test_while_continue(t)
    }
    t |> run("for continue") @(t : T?) {
        test_for_continue_impl(t)
    }
    t |> run("complex for continue") @(t : T?) {
        test_complex_for_continue(t)
    }
}
