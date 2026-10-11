options gen2
require dastest/testing_boost

def push_range(n : int) : array<int> {
    var res : array<int>   // nolint:STYLE027 - the push loop IS the path under test
    for (i in range(n)) {
        res |> push(i * 3)   // nolint:PERF006 - growing from empty is the case under test
    }
    return <- res
}

def push_after_reserve(n : int) : int {
    var a : array<int>
    a |> reserve(n)
    for (i in range(n)) {
        a |> push(i)
    }
    var s = 0
    for (x in a) {
        s += x
    }
    delete a
    return s
}

def push_strings() : int {
    var a : array<string>   // nolint:STYLE012 - a run of pushes IS the path under test
    a |> push("x")
    a |> push("yy")
    a |> push("zzz")
    var n = 0
    for (s in a) {
        n += length(s)
    }
    delete a
    return n
}

def push_during_iteration(var a : array<int>) : bool {
    var threw = false
    try {
        for (_x in a) {
            a |> push(1)   // nolint:PERF006 - this push must throw, not grow
        }
    } recover {
        threw = true
    }
    return threw
}

[test]
def test_array_push_fast(t : T?) {
    t |> run("grows from empty across capacity steps") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@push_range))
        var a <- push_range(100)
        t |> equal(length(a), 100)
        t |> equal(a[0], 0)
        t |> equal(a[99], 297)
        t |> success(capacity(a) >= 100)
        delete a
    }
    t |> run("reserved capacity takes the inline path") @(t : T?) {
        t |> equal(push_after_reserve(100), 4950)
    }
    t |> run("non-POD elements") @(t : T?) {
        t |> equal(push_strings(), 6)
    }
    t |> run("push into a locked array still throws") @(t : T?) {
        var a <- push_range(4)
        t |> success(push_during_iteration(a))
    }
}
