options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/defer

def defer_check(var x : int&) {
    defer() <| $() {
        // 2nd
        assert(x == 2)
        x *= 2
    }
    defer() <| $() {
        // 1st
        assert(x == 3)
        x--
    }
    x += 2
}

def defer_check_del(var a : array<int>) : int {
    a <- [for (x in range(10)); x * x]
    var inscope tmp <- a
    a <- tmp
    defer() <| $() {
        delete a
    }
    return length(a)
}

[sideeffects]
def defer_check_short_pipe() {
    var x = 1
    {
        x++
        defer <| $() {
            x--
        }
    }
    assert(x == 1)
}

[test]
def test_defer(t : T?) {
    t |> run("defer execution order") @(t : T?) {
        var x = 1
        defer_check(x)
        t |> equal(x, 4)
    }
    t |> run("defer_delete cleans up array") @(t : T?) {
        var a : array<int>
        let len = defer_check_del(a)
        t |> equal(len, 10)
        t |> equal(length(a), 0)
    }
    t |> run("defer with short pipe syntax") @(t : T?) {
        defer_check_short_pipe()
        t |> success(true)
    }
}
