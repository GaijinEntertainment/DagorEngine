options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/defer

def defer_check(var x : int&) {
    defer() {
        // 2nd
        assert(x == 2)
        x *= 2
    }
    defer() {
        // 1st
        assert(x == 3)
        x--
    }
    x += 2
}

def defer_check_del(var a : array<int>) : int {
    a <- [for (x in range(10)); x * x]   // nolint:PERF030 - the parameter arrives empty; this is the fixture the defer-delete test needs
    var inscope tmp <- a
    a <- tmp                             // nolint:PERF030 - a was just emptied by the move into tmp; the round trip is the tested construct
    defer() {
        delete a
    }
    return length(a)
}

def defer_delete_check(var a : array<int>) : int {
    a <- [for (x in range(5)); x]   // nolint:PERF030 - the parameter arrives empty; this is the fixture the defer_delete arm needs
    defer_delete(a)
    return length(a)
}

[sideeffects]
def defer_check_short_pipe() {
    var x = 1
    {
        x++
        defer <| $() {      // nolint:STYLE002 - the short pipe spelling is what this arm covers
            x--
        }
    }
    assert(x == 1)
}

def defer_in_lambda_bare_block() : int2 {
    var count = 0   // nolint:LINT003 - `let` makes the lambda's captured copy const, breaking count++
    var lam <- @ capture(copy(count)) () : int {
        {
            defer() {
                count++
            }
            count += 10
        }
        return count
    }
    let first = invoke(lam)
    let second = invoke(lam)
    unsafe { delete lam; }
    return int2(first, second)
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
    t |> run("defer_delete (deprecated spelling) still deletes at scope exit") @(t : T?) {
        var a : array<int>
        t |> equal(defer_delete_check(a), 5)
        t |> equal(length(a), 0)
    }
    t |> run("defer with short pipe syntax") @(t : T?) {
        defer_check_short_pipe()
        t |> success(true)
    }
    t |> run("defer in a bare block inside a lambda fires per call, not on delete") @(t : T?) {
        t |> equal(defer_in_lambda_bare_block(), int2(11, 22))
    }
}
