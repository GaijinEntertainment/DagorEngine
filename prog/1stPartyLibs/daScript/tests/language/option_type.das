options gen2
require dastest/testing_boost public

def foo(var a : int& | auto) {
    a = 42
}

def bar(var a : auto | int&) {
    a = 42
}

[test]
def test_option_type(t : T?) {
    t |> run("preserve ref") @(t : T?) {
        var a : int
        bar(a)              // match foo(auto), not foo(int&)
        t |> equal(0, a)
        foo(a)              // match foo(int &), not foo(auto)
        t |> equal(42, a)
    }
}
