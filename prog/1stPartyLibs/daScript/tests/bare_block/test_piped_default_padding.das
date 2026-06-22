options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require UnitTest

// ============================================================
// helpers: defaults before the block parameter
// ============================================================

def pad_b(_a : int; b : float = 3.25; blk : block<(y : float) : float>) : float {
    return invoke(blk, b)
}

def pad_two(a : int; b : int = 7; c : float = 0.5; blk : block<(y : int; z : float) : float>) : float {
    return invoke(blk, a + b, c)
}

def pad_all(a : int = 13; blk : block<(x : int) : int>) : int {
    return invoke(blk, a)
}

def pad_lambda(a : int; b : int = 5; lam : lambda<(x : int) : int>) : int {
    return invoke(lam, a + b)
}

def pad_funcptr(a : int; b : int = 6; fn : function<(x : int) : int>) : int {
    return invoke(fn, a + b)
}

def side_default() : int {
    return 21
}

def pad_call_default(a : int; b : int = side_default(); blk : block<(x : int) : int>) : int {
    return invoke(blk, a + b)
}

struct PadPoint {
    x : int = 3
    y : int = 4
}

def pad_struct(a : int; p : PadPoint = default<PadPoint>; blk : block<(s : int) : int>) : int {
    return invoke(blk, a + p.x + p.y)
}

def pad_or(a : int | float; b : int = 2; blk : block<(x : int) : int>) : int {
    return invoke(blk, b)
}

// generic callee: auto first arg + auto in the block signature
def gpad(a : auto; b : float = 1.5; blk : block<(x : auto; y : float) : float>) : float {
    return invoke(blk, a, b)
}

// ============================================================
// A. core padding
// ============================================================

[test]
def test_pad_one_default(t : T?) {
    t |> run("one default padded before the block") @(t : T?) {
        let r = pad_b(1) $(y : float) : float {
            return y;
        }
        t |> equal(r, 3.25)
    }
    t |> run("explicit <| pipe pads the same way") @(t : T?) {
        let r = pad_b(1) <| $(y : float) : float {
            return y;
        }
        t |> equal(r, 3.25)
    }
}

[test]
def test_pad_two_defaults(t : T?) {
    t |> run("two defaults padded, block param types forward-inferred") @(t : T?) {
        let r = pad_two(10) $(y; z) {
            return float(y) + z;
        }
        t |> equal(r, 17.5)
    }
}

[test]
def test_pad_from_position_zero(t : T?) {
    t |> run("all arguments defaulted") @(t : T?) {
        let r = pad_all() $(x : int) : int {
            return x * 2;
        }
        t |> equal(r, 26)
    }
}

[test]
def test_full_positional_regression(t : T?) {
    t |> run("fully positional piped call is unchanged") @(t : T?) {
        let r = pad_b(1, 9.0) $(y : float) : float {
            return y;
        }
        t |> equal(r, 9.0)
    }
}

[test]
def test_pad_lambda_argument(t : T?) {
    t |> run("piped lambda lands on lambda param behind a default") @(t : T?) {
        let r = pad_lambda(1) <| @(x : int) : int {
            return x + 100;
        }
        t |> equal(r, 106)
    }
}

[test]
def test_pad_funcptr_argument(t : T?) {
    t |> run("piped function pointer lands on function param behind a default") @(t : T?) {
        let r = pad_funcptr(1) <| @@(x : int) : int {
            return x + 200;
        }
        t |> equal(r, 207)
    }
}

[test]
def test_pad_call_default(t : T?) {
    t |> run("default that is a function call is cloned to the call site") @(t : T?) {
        let r = pad_call_default(1) $(x : int) : int {
            return x;
        }
        t |> equal(r, 22)
    }
}

[test]
def test_pad_struct_default(t : T?) {
    t |> run("default<struct> padded before the block") @(t : T?) {
        let r = pad_struct(1) $(s : int) : int {
            return s;
        }
        t |> equal(r, 8)
    }
}

[test]
def test_pad_or_type(t : T?) {
    t |> run("OR-type prefix argument, int flavor") @(t : T?) {
        let r = pad_or(1) $(x : int) : int {
            return x + 10;
        }
        t |> equal(r, 12)
    }
    t |> run("OR-type prefix argument, float flavor") @(t : T?) {
        let r = pad_or(1.0) $(x : int) : int {
            return x + 20;
        }
        t |> equal(r, 22)
    }
}

[test]
def test_pad_generic_callee(t : T?) {
    t |> run("generic callee instanced after padding") @(t : T?) {
        let r = gpad(2) $(x : int; y : float) : float {
            return float(x) + y;
        }
        t |> equal(r, 3.5)
    }
}

// ============================================================
// B. overload ranking
// ============================================================

def pick(_a : int; b : int = 1; blk : block<() : int>) : int {
    return 300 + b + invoke(blk)
}

def pick(_a : int; b : int = 1; c : int = 2; blk : block<() : int>) : int {
    return 400 + b + c + invoke(blk)
}

def cdist(var _a : int&; b : int = 1; blk : block<() : int>) : int {
    return 1000 + invoke(blk)
}

def cdist(_a : int; b : int = 2; blk : block<() : int>) : int {
    return 2000 + invoke(blk)
}

def fn_vs_gen(_a : int; b : int = 1; blk : block<() : int>) : int {
    return 3000 + invoke(blk)
}

def fn_vs_gen(_a : auto; b : int = 1; blk : block<() : int>) : int {
    return 4000 + invoke(blk)
}

[test]
def test_least_padding_wins(t : T?) {
    t |> run("overload with fewer padded defaults wins") @(t : T?) {
        let r = pick(1) $ {
            return 7;
        }
        t |> equal(r, 308)
    }
}

[test]
def test_aligned_distance_tiebreak(t : T?) {
    t |> run("equal padding, exact constness wins") @(t : T?) {
        var v = 5 // nolint:LINT003 - `let` would flip resolution to the const overload
        let r = cdist(v) $ {
            return 1;
        }
        t |> equal(r, 1001)
    }
}

[test]
def test_function_beats_generic(t : T?) {
    t |> run("function wins over generic at equal padding") @(t : T?) {
        let r = fn_vs_gen(1) $ {
            return 1;
        }
        t |> equal(r, 3001)
    }
}

[test]
def test_builtin_beats_das_overload(t : T?) {
    t |> run("C++ builtin with fewer padded defaults wins over das overload") @(t : T?) {
        var got = 0.0
        testPipedDefaults(3) $(x : int; y : float) {
            got = float(x) + y;
        }
        t |> equal(got, 6.125)
    }
}

// the das overload above competes with the UnitTest builtin (a, b=3.125, blk, ctx, at):
// builtin pads 1 (b), this overload pads 2 (extra + b) - builtin must win
def testPipedDefaults(a : int; extra : int = 1; b : float = 2.0; blk : block<(x : int; y : float) : void>) {
    invoke(blk, a + extra + 1000, b)
}

// ============================================================
// C. call shapes
// ============================================================

class Widget {
    base : int = 10
    def scaled(k : int = 3; blk : block<(v : int) : int>) : int {
        return invoke(blk, base * k)
    }
}

[test]
def test_class_method_arrow(t : T?) {
    t |> run("obj->method() pads defaults before the block") @(t : T?) {
        var w = new Widget()
        let r = w->scaled() $(v : int) : int {
            return v + 1;
        }
        t |> equal(r, 31)
    }
}

[test]
def test_class_method_dot(t : T?) {
    t |> run("obj.method() pads defaults before the block") @(t : T?) {
        var w = new Widget()
        let r = w.scaled() $(v : int) : int {
            return v + 2;
        }
        t |> equal(r, 32)
    }
}

[test]
def test_module_qualified(t : T?) {
    t |> run("_:: qualified piped call pads") @(t : T?) {
        let r = _::pad_b(1) $(y : float) : float {
            return y;
        }
        t |> equal(r, 3.25)
    }
}

def via_generic(v : auto) : float {
    return pad_b(2) $(y : float) : float {
        return y * 2.0;
    }
}

[test]
def test_piped_call_inside_generic(t : T?) {
    t |> run("piped call survives generic instancing, two instances") @(t : T?) {
        t |> equal(via_generic(1), 6.5)
        t |> equal(via_generic("str"), 6.5)
    }
}

[test]
def test_prefix_and_trailing_pipe(t : T?) {
    t |> run("a |> foo() with trailing block pads in between") @(t : T?) {
        let r = 10 |> pad_two() $(y; z) {
            return float(y) + z;
        }
        t |> equal(r, 17.5)
    }
}

[test]
def test_cpp_builtin_with_fakes(t : T?) {
    t |> run("C++ extern: default before block, fakeContext/fakeLineInfo after") @(t : T?) {
        var got_a = 0
        var got_b = 0.0
        UnitTest::testPipedDefaults(9) $(x : int; y : float) {
            got_a = x;
            got_b = y;
        }
        t |> equal(got_a, 9)
        t |> equal(got_b, 3.125)
    }
}

[test]
def test_nested_piped_calls(t : T?) {
    t |> run("piped call inside a piped block") @(t : T?) {
        let r = pad_b(1) $(y : float) : float {
            let inner = pad_all() $(x : int) : int {
                return x;
            }
            return y + float(inner);
        }
        t |> equal(r, 16.25)
    }
}

def lam_then_blk(a : int; lam : lambda<(x : int) : int> = @ (x : int) => x; blk : block<() : int>) : int {
    return invoke(lam, invoke(blk) + a)
}

[test]
def test_landing_is_type_driven(t : T?) {
    t |> run("piped block skips a defaulted lambda param and lands on the block param") @(t : T?) {
        let r = lam_then_blk(1) $ {
            return 41;
        }
        t |> equal(r, 42)
    }
}

class Gadget {
    bias : int = 2
    def mixed(lam : lambda<(x : int) : int> = @ (x : int) => x; blk : block<() : int>) : int {
        return invoke(lam, invoke(blk) + bias)
    }
}

[test]
def test_method_landing_is_type_driven(t : T?) {
    t |> run("method piped block skips a defaulted lambda param") @(t : T?) {
        var g = new Gadget()
        let r = g->mixed() $ {
            return 40;
        }
        t |> equal(r, 42)
    }
}
