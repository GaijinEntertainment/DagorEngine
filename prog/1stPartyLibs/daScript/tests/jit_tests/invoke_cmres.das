options gen2
require dastest/testing_boost

struct Big {
    a : int
    b : int
    c : int
    d : int
}

def make_big(k : int) : Big {
    return Big(a = k, b = k * 2, c = k * 3, d = k * 4)
}

def via_fn_ptr(fun : function<(k : int) : Big>; k : int) : Big {
    return invoke(fun, k)
}

def via_lambda(fun : lambda<(k : int) : Big>; k : int) : Big {
    return invoke(fun, k)
}

class Maker {
    scale : int
    def make(k : int) : Big {
        return Big(a = k * scale, b = k * scale * 2, c = k * scale * 3, d = k * scale * 4)
    }
}

def expect_big(t : T?; got : Big; a, b, c, d : int) {
    t |> equal(a, got.a)
    t |> equal(b, got.b)
    t |> equal(c, got.c)
    t |> equal(d, got.d)
}

[test]
def test_cmres_invoke(t : T?) {
    t |> run("function pointer") @(t : T?) {
        expect_big(t, via_fn_ptr(@@make_big, 7), 7, 14, 21, 28)
        expect_big(t, make_big(7), 7, 14, 21, 28)
    }
    t |> run("lambda") @(t : T?) {
        var fn <- @(k : int) : Big => Big(a = k, b = k * 2, c = k * 3, d = k * 4)
        expect_big(t, via_lambda(fn, 5), 5, 10, 15, 20)
        unsafe {
            delete fn
        }
    }
    t |> run("class method") @(t : T?) {
        var m = new Maker(scale = 3)
        expect_big(t, m->make(2), 6, 12, 18, 24)
        unsafe {
            delete m
        }
    }
    t |> run("result is not aliased across two invokes") @(t : T?) {
        let first = via_fn_ptr(@@make_big, 1)
        let second = via_fn_ptr(@@make_big, 9)
        expect_big(t, first, 1, 2, 3, 4)
        expect_big(t, second, 9, 18, 27, 36)
    }
}
