options gen2
require dastest/testing_boost

require daslib/lpipe

var g_A = 0
var g_B = 0.

struct Foo {
    a : int
    b : float
}

def test_invoke_block_jit(lmb : block<(a : int; b : float) : int>) {
    return invoke(lmb, 1, 2.0)
}

def test_invoke_block(lmb : block<(a : int; b : float) : int>) {
    return invoke(lmb, 1, 2.0)
}

def test_invoke_block_refval(lmb : block<(a : int; b : float) : Foo>) {
    return invoke(lmb, 1, 2.0)
}

def test_invoke_block_void(lmb : block<(a : int; b : float) : void>) {
    invoke(lmb, 1, 2.0)
}

def test_make_block {
    var local_C = 123
    let res = test_invoke_block() $(a : int; b : float) : int {
        g_A = a
        g_B = b
        local_C = 456
        return 13
    }
    return (res, local_C)
}

def test_make_block_jit_invoke {
    var local_C = 123
    let res = test_invoke_block_jit() $(a : int; b : float) : int {
        g_A = a
        g_B = b
        local_C = 456
        return 13
    }
    return (res, local_C)
}

def test_make_block_return_refval {
    let res = test_invoke_block_refval() $(a : int; b : float) : Foo {
        g_A = a
        g_B = b
        return Foo(a = a, b = b)
    }
    return res
}

def test_make_block_capture_refval {
    var f : Foo
    test_invoke_block_void() $(a : int; b : float) : void {
        f.a = a
        f.b = b
    }
    return f
}

def test_make_block_nested(q : float) {
    var f : Foo
    test_invoke_block_void() $(a : int; b : float) : void {
        f.a = a
        f.b = b
        test_invoke_block_void() $(c : int; d : float) : void {
            f.a = c + 10
            f.b = b + q
        }
    }
    return f
}

var g_a_i : array<int>

def capture_args(var a : array<int>) {
    invoke() $(var b : array<int>) {
        invoke() {
            g_a_i <- b
        }
    }
    lpipe(a)
}

var g_i : int

def capture_args_ref(var a : int&) {
    invoke() $(var b : int&) {
        invoke() {
            g_i <- b
        }
    }
    lpipe(a)
}

[test]
def test_block(t : T?) {
    t |> run("make block") @(t : T?) {
        verify(!jit_enabled() || is_jit_function(@@test_make_block))
        g_A = 0
        g_B = 0.
        let res = test_make_block()
        t |> equal(13, res._0)
        t |> equal(456, res._1)
        t |> equal(1, g_A)
        t |> equal(2., g_B)
    }
    t |> run("make block, invoke with jit") @(t : T?) {
        verify(!jit_enabled() || is_jit_function(@@test_make_block_jit_invoke))
        g_A = 0
        g_B = 0.
        let res = test_make_block_jit_invoke()
        t |> equal(13, res._0)
        t |> equal(456, res._1)
        t |> equal(1, g_A)
        t |> equal(2., g_B)
    }
    t |> run("make block return refval") @(t : T?) {
        verify(!jit_enabled() || is_jit_function(@@test_make_block_return_refval))
        g_A = 0
        g_B = 0.
        let res = test_make_block_return_refval()
        t |> equal(1, res.a)
        t |> equal(2., res.b)
        t |> equal(1, g_A)
        t |> equal(2., g_B)
    }
    t |> run("make block capture refval") @(t : T?) {
        verify(!jit_enabled() || is_jit_function(@@test_make_block_capture_refval))
        var res = test_make_block_capture_refval()
        t |> equal(1, res.a)
        t |> equal(2., res.b)
    }
    t |> run("make block capture refval nested") @(t : T?) {
        verify(!jit_enabled() || is_jit_function(@@test_make_block_nested))
        var res = test_make_block_nested(10.)
        t |> equal(11, res.a)
        t |> equal(12., res.b)
    }
    t |> run("make block capture refval nested") @(t : T?) {
        verify(!jit_enabled() || is_jit_function(@@capture_args))
        var a <- [ 1, 2, 3, 4]
        g_a_i |> clear
        capture_args(a)
        t |> equal(4, g_a_i |> length)
        t |> equal(0, a |> length)
    }
    t |> run("make block capture by ref nested") @(t : T?) {
        verify(!jit_enabled() || is_jit_function(@@capture_args_ref))
        var a = 13
        g_i = 0
        capture_args_ref(a)
        t |> equal(13, g_i)
        t |> equal(a, 0)
    }
}

