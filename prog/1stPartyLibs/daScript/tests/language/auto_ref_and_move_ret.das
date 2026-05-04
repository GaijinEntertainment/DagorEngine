options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def armr_abs(i : int) : int {
    return i < 0 ? -i : i
}

def armr_foo(var bar : array<int>) : array<int> {
    var b & = unsafe(bar)
    for (x in range(10)) {
        push(b, x)
    }
    return <- b
}

[test]
def test_auto_ref_and_move_ret(t : T?) {
    t |> run("auto ref and move return") @(t : T?) {
        var a : array<int>
        for (x in range(10)) {
            push(a, 10 - x)
        }
        var tf <- armr_foo(a)
        t |> equal(length(a), 0)
        t |> equal(length(tf), 20)
        for (x, y in range(-10, 10), tf) {
            t |> equal(armr_abs(x), y)
        }
    }
}
