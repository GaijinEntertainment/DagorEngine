options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/flatten

//! lift_expr operand-coverage differential tests. A user helper call can hide
//! inside a member / swizzle / constructor / tuple / wrapper expression; lift_expr
//! must recurse into those operands and inline the call. Without the recursion the
//! call survives lowering and the post-pass verify scan rejects it as an "internal
//! error". Each twin is differential-checked against its original AND proven
//! call-free by the macro's cycle-2 scan.

struct P {
    a : int
    b : int
}

def mk(x : int) : P {
    return P(a = x + 1, b = x * 2)
}

def vec(x : float) : float4 {
    return float4(x, x + 1.0, x + 2.0, 1.0)
}

def sq(a : int) : int {
    return a * a
}

def addk(a : int; k : int) : int {
    return a + k
}

// ExprField — user call as the object of a `.field` read (wrapped in ExprRef2Value)
[flatten]
def f_field(x : int) : int {
    return mk(x).a + mk(x).b
}

// ExprSwizzle — user call as the value of a `.xyz` swizzle
[flatten]
def f_swizzle(x : float) : float3 {
    return vec(x).xyz
}

// ExprMakeStruct — user call inside a returned struct field
[flatten]
def f_makestruct(x : int) : P {
    return P(a = mk(x).b, b = mk(x).a)
}

// ExprMakeTuple — user calls inside a tuple constructor, then tuple field access
[flatten]
def f_tuple(x : int) : int {
    let tp = (mk(x).a, mk(x).b)
    return tp._0 * 10 + tp._1
}

// nested: a call buried under swizzle + field + op together
[flatten]
def f_nested(x : float) : float {
    return vec(x).x + vec(x * 2.0).y
}

// inline_call argument is itself a user call — the argument must be lifted
// (nested call inlined) BEFORE binding to the frame temp, else `sq(x)` survives
// in `var __flat_arg = sq(x)` and the cycle-2 scan rejects the twin.
[flatten]
def f_arg_call(x : int) : int {
    return addk(sq(x), sq(x + 1))
}

var GRID : array<int>

[init]
def fill_grid {
    GRID <- [-7, -1, 0, 1, 3, 10, 42]
}

[test]
def test_flatten_lift(t : T?) {
    t |> run("ExprField on a user call") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_field_flat(v), f_field(v))
        }
    }
    t |> run("ExprSwizzle on a user call") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_swizzle_flat(float(v)), f_swizzle(float(v)))
        }
    }
    t |> run("ExprMakeStruct field is a user call") @(t : T?) {
        for (v in GRID) {
            let a = f_makestruct(v)
            let b = f_makestruct_flat(v)
            t |> equal(a.a, b.a)
            t |> equal(a.b, b.b)
        }
    }
    t |> run("ExprMakeTuple values are user calls") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_tuple_flat(v), f_tuple(v))
        }
    }
    t |> run("nested swizzle + field + op") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_nested_flat(float(v)), f_nested(float(v)))
        }
    }
    t |> run("user call in an argument position") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_arg_call_flat(v), f_arg_call(v))
        }
    }
}
