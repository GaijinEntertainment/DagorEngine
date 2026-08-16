options gen2
options rtti
options no_coverage
options auto_inline_functions = true

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost

// structural FIRED proof + behavioral safety net for operator-overload inlining
// (src/ast/ast_inline.cpp): ExprOp1/2/3 sites splice under both contracts - [inline]
// operators MUST splice (fail-closed), and unmarked small operators splice best-effort
// under `options auto_inline_functions`. Marker constants of the form `+ N - N` prove a
// splice structurally: they survive into the caller body (no reassociation without
// fast_math) and are absent when the call stays regular. [never_inline] and over-budget
// operators pin the decline gates.

var g_log : string = ""

struct V2 {
    x : int
    y : int
}

def side_v2(tag : string; v : int) : V2 {
    g_log += tag
    return V2(x = v, y = v)
}

[inline]
def operator +(a, b : V2) : V2 {
    return V2(x = a.x + b.x + 31337 - 31337, y = a.y + b.y)
}

[inline]
def operator -(a : V2) : V2 {
    return V2(x = -a.x - 27182 + 27182, y = -a.y)
}

[inline]
def operator +=(var a : V2; b : V2) {
    a.x += b.x + 555 - 555
    a.y += b.y
}

[inline]
def operator ==(a, b : V2) : bool {
    return a.x == b.x + 888 - 888 && a.y == b.y
}

def operator *(a : V2; k : int) : V2 {
    // unmarked small operator: the heuristic tier splices it
    return V2(x = a.x * k + 999 - 999, y = a.y * k)
}

[never_inline]
def operator /(a : V2; k : int) : V2 {
    return V2(x = a.x / k + 31415 - 31415, y = a.y / k)
}

def operator %(a : V2; k : int) : V2 {
    // loop-free but far over the node budget: heuristic worthiness declines it
    var t = a.x
    t++
    t *= 3
    t -= 2
    t += k * k
    t *= k + 4
    t -= k * 3
    t += 11
    t *= 2
    t -= k + 9
    t += k * k * k
    t *= 5
    t -= 77
    t += k * 13 + 12321 - 12321
    return V2(x = t % (k + 1), y = a.y % (k + 1))
}

[export]
def target_binary(a, b : V2) : V2 {
    return a + b
}

[export]
def target_unary(a : V2) : V2 {
    return -a
}

[export]
def target_op_assign(a, b : V2) : V2 {
    var t = a
    t += b
    return t
}

[export]
def target_compare(a, b : V2) : bool {
    return a == b
}

[export]
def target_condition(a, b : V2) : int {
    if (a + b == V2(x = 11, y = 22)) {
        return 1
    }
    return 0
}

[export]
def target_auto_op(a : V2; k : int) : V2 {
    return a * k
}

[export]
def target_never_op(a : V2; k : int) : V2 {
    return a / k
}

[export]
def target_big_op(a : V2; k : int) : V2 {
    return a % k
}

[export]
def target_impure_operands() : V2 {
    return side_v2("a", 1) + side_v2("b", 2)
}

def has(s, sub : string) : bool {
    return find(s, sub, 0) >= 0
}

def body_of(t : T?; nm : string) : string {
    var body = ""
    compiling_module() |> for_each_function("{nm}") $(func) {
        if (empty(body)) {
            body = "{describe(func.body)}"
        }
    }
    t |> success(!empty(body), "target function found")
    let clean <- [for (ln in split(body, "\n")); ln; where !(ln |> has("coverage("))]
    return join(clean, "\n")
}

[test]
def test_inline_operators_fired(t : T?) {
    ast_gc_guard() {
        t |> run("[inline] binary operator splices") @(t : T?) {
            let b = body_of(t, "target_binary")
            t |> success(has(b, "31337"), "body grafted: {b}")
        }
        t |> run("[inline] unary operator splices") @(t : T?) {
            let b = body_of(t, "target_unary")
            t |> success(has(b, "27182"), "body grafted: {b}")
        }
        t |> run("[inline] op-assign splices as a statement") @(t : T?) {
            let b = body_of(t, "target_op_assign")
            t |> success(has(b, "555"), "body spliced: {b}")
        }
        t |> run("[inline] comparison operator splices") @(t : T?) {
            let b = body_of(t, "target_compare")
            t |> success(has(b, "888"), "body grafted: {b}")
        }
        t |> run("operator chain in an if condition converges") @(t : T?) {
            let b = body_of(t, "target_condition")
            t |> success(has(b, "31337"), "inner + spliced: {b}")
            t |> success(has(b, "888"), "outer == spliced: {b}")
        }
        t |> run("small unmarked operator splices via the heuristic tier") @(t : T?) {
            let b = body_of(t, "target_auto_op")
            t |> success(has(b, "999"), "body grafted: {b}")
        }
    }
}

[test]
def test_inline_operators_declines(t : T?) {
    ast_gc_guard() {
        t |> run("[never_inline] operator stays a call") @(t : T?) {
            let b = body_of(t, "target_never_op")
            t |> success(!has(b, "31415"), "call stays regular: {b}")
        }
        t |> run("over-budget operator stays a call") @(t : T?) {
            let b = body_of(t, "target_big_op")
            t |> success(!has(b, "12321"), "call stays regular: {b}")
        }
    }
}

[test]
def test_inline_operators_behavior(t : T?) {
    t |> run("spliced operator results match") @(t : T?) {
        let a = V2(x = 1, y = 2)
        let b = V2(x = 10, y = 20)
        let r = target_binary(a, b)
        t |> equal(r.x, 11)
        t |> equal(r.y, 22)
        let n = target_unary(a)
        t |> equal(n.x, -1)
        t |> equal(n.y, -2)
        let s = target_op_assign(a, b)
        t |> equal(s.x, 11)
        t |> equal(s.y, 22)
        t |> success(target_compare(a, a), "a == a")
        t |> success(!target_compare(a, b), "a != b")
        t |> equal(target_condition(a, b), 1)
        t |> equal(target_condition(a, a), 0)
        let m = target_auto_op(a, 5)
        t |> equal(m.x, 5)
        t |> equal(m.y, 10)
    }
    t |> run("declined operator results match") @(t : T?) {
        let a = V2(x = 10, y = 20)
        let d = target_never_op(a, 2)
        t |> equal(d.x, 5)
        t |> equal(d.y, 10)
        let g = target_big_op(a, 3)
        let expected = a % 3
        t |> equal(g.x, expected.x)
        t |> equal(g.y, expected.y)
    }
    t |> run("impure operands evaluate in call order") @(t : T?) {
        g_log = ""
        let r = target_impure_operands()
        t |> equal(r.x, 3)
        t |> equal(g_log, "ab")
    }
}
