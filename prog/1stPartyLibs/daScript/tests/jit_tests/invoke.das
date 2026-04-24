options gen2
require dastest/testing_boost

var g_A = 0
var g_B = 0.

def call_function(a : int; b : float) {
    g_A = a
    g_B = b
    return 13
}

def test_invoke_call(fun : function<(a : int; b : float) : int>) {
    return invoke(fun, 1, 2.0)
}

struct Foo {
    a : int
    b : float
}

def call2cmres_function(a : int; b : float) {
    g_A = a
    g_B = b
    return Foo(a = a + 10, b = b + 10.)
}

def test_invoke_call2cmres(fun : function<(a : int; b : float) : Foo>) {
    return invoke(fun, 1, 2.0)
}

def test_invoke_lambda(lmb : lambda<(a : int; b : float) : int>) {
    return invoke(lmb, 1, 2.0)
}

def test_invoke_and_delete_lambda(var lmb : lambda<(a : int; b : float) : int>) {
    let res = invoke(lmb, 1, 2.0)
    delete lmb
    return res
}

def test_invoke_lambda2cmres(lmb : lambda<(a : int; b : float) : Foo>) {
    return invoke(lmb, 1, 2.0)
}

def test_invoke_block(blk : block<(a : int; b : float) : int>) {
    return invoke(blk, 1, 2.0)
}

def test_invoke_block2cmres(blk : block<(a : int; b : float) : Foo>) {
    return invoke(blk, 1, 2.0)
}

[test]
def test_ref(t : T?) {
    t |> run("invoke call") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_invoke_call))
        g_A = 0
        g_B = 0.
        t |> equal(13, test_invoke_call(@@call_function))
        t |> equal(1, g_A)
        t |> equal(2.0, g_B)
    }
    t |> run("invoke call2cmres") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_invoke_call2cmres))
        g_A = 0
        g_B = 0.
        let res = test_invoke_call2cmres(@@call2cmres_function)
        t |> equal(11, res.a)
        t |> equal(12., res.b)
        t |> equal(1, g_A)
        t |> equal(2.0, g_B)
    }
    t |> run("invoke lambda") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_invoke_lambda))
        g_A = 0
        g_B = 0.
        let res = test_invoke_lambda() @(a : int; b : float) : int {
            g_A = a
            g_B = b
            return 13
        }
        t |> equal(13, res)
        t |> equal(1, g_A)
        t |> equal(2.0, g_B)
    }
    t |> run("invoke and delete lambda") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_invoke_and_delete_lambda))
        g_A = 0
        g_B = 0.
        let res = test_invoke_and_delete_lambda() @(a : int; b : float) : int {
            g_A = a
            return 13
        } finally {
            g_B = 2.0
        }
        t |> equal(13, res)
        t |> equal(1, g_A)
        t |> equal(2.0, g_B)
    }
        // now empty lambda

    t |> run("invoke lambda2cmres") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_invoke_lambda2cmres))
        g_A = 0
        g_B = 0.
        let res = test_invoke_lambda2cmres() @(a : int; b : float) : Foo {
            g_A = a
            g_B = b
            return Foo(a = a + 10, b = b + 10.)
        }
        t |> equal(11, res.a)
        t |> equal(12., res.b)
        t |> equal(1, g_A)
        t |> equal(2.0, g_B)
    }
    t |> run("invoke block") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_invoke_block))
        g_A = 0
        g_B = 0.
        let res = test_invoke_block() $(a : int; b : float) : int {
            g_A = a
            g_B = b
            return 13
        }
        t |> equal(13, res)
        t |> equal(1, g_A)
        t |> equal(2.0, g_B)
    }
    t |> run("invoke block2cmres") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_invoke_block2cmres))
        g_A = 0
        g_B = 0.
        let res = test_invoke_block2cmres() $(a : int; b : float) : Foo {
            g_A = a
            g_B = b
            return Foo(a = a + 10, b = b + 10.)
        }
        t |> equal(11, res.a)
        t |> equal(12., res.b)
        t |> equal(1, g_A)
        t |> equal(2.0, g_B)
    }
    t |> run("invoke null") @(t : T?) {
        try {
            test_invoke_call(default<function<(a : int; b : float) : int>>);
            panic("unreachable")
        } recover {}
        try {
            test_invoke_lambda(default<lambda<(a : int; b : float) : int>>);
            panic("unreachable")
        } recover {}
    }
}


