options gen2
require dastest/testing_boost

struct Foo {
    a, b : int
}

struct Fooo {
    a, b : int[5]
}

def Foo(x, y : int) {
    return Foo(a = x, b = y)
}

def makeAll13(var a : int[10]) {
    for (i in range(10)) {
        a[i] = 13
    }
}

def addAll(var a : int[10]) {
    var c = 0
    for (i in range(10)) {
        c += a[i]
    }
    return c
}

def outOfRange(var a : int[10]; index : int) {
    return a[index]
}

def make_dim_cmres {
    return fixed_array<int>(1, 2, 3, 4, 5)
}

def make_dim_on_stack(which : bool) {
    var a = fixed_array<int>(1, 2, 3, 4, 5)
    var b = fixed_array<int>(6, 7, 8, 9, 10)
    return which ? a : b
}

def make_dim_with_index_call_to_cmres {
    return fixed_array<Foo>(Foo(1, 6), Foo(2, 7), Foo(3, 8), Foo(4, 9), Foo(5, 10))
}

def make_struct_with_subdim {
    return Fooo(
        a = fixed_array<int>(1, 2, 3, 4, 5),
        b = fixed_array<int>(6, 7, 8, 9, 10)
    )
}

[test]
def test_dim(t : T?) {
    t |> run("array []") @(t : T?) {
        var x : int[10]
        t |> success(!jit_enabled() || is_jit_function(@@makeAll13))
        t |> success(!jit_enabled() || is_jit_function(@@addAll))
        makeAll13(x)
        for (i in x) {
            t |> equal(i, 13)
        }
        t |> equal(addAll(x), 130)
    }
    t |> run("out of range") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@outOfRange))
        var x : int[10]
        var failed = false
        try {
            var r = outOfRange(x, 13)
            t |> success(r == 123456)// code never gets here
        } recover {
            failed = true
        }
        t |> success(failed, "out of range caused panic")
    }
    t |> run("make dim cmres") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@make_dim_cmres))
        var x = make_dim_cmres()
        t |> equal(length(x), 5)
        for (i in range(5)) {
            t |> equal(x[i], i + 1)
        }
    }
    t |> run("make dim on stack") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@make_dim_on_stack))
        var a = make_dim_on_stack(true)
        var b = make_dim_on_stack(false)
        t |> equal(length(a), 5)
        t |> equal(length(b), 5)
        for (i in range(5)) {
            t |> equal(a[i], i + 1)
            t |> equal(b[i], i + 6)
        }
    }
    t |> run("make dim with index to call cmres") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@make_dim_with_index_call_to_cmres))
        var x = make_dim_with_index_call_to_cmres()
        t |> equal(length(x), 5)
        for (i in range(5)) {
            t |> equal(x[i].a, i + 1)
            t |> equal(x[i].b, i + 6)
        }
    }
    t |> run("make struct with subdim") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@make_struct_with_subdim))
        var x = make_struct_with_subdim()
        for (i in range(5)) {
            t |> equal(x.a[i], i + 1)
            t |> equal(x.b[i], i + 6)
        }
    }
}