options gen2
options rtti
options no_coverage

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost
require _inline_scope_helper

// structural FIRED proof + behavioral safety net for cross-module splices whose
// bodies use the callee module's private symbols (src/ast/ast_inline.cpp): the
// spliced body clone rides a generated `with (module <origin>)` wrapper, so its
// privates and require graph resolve exactly as at the callee's own compile.
// argument and prefix temps bind OUTSIDE the wrapper (the mirror rule): caller
// expressions keep resolving at the caller, proven by the g_shared collision test.

var g_shared = 555      // collides with _inline_scope_helper's g_shared on purpose

def has(s, sub : string) : bool {
    return !empty(sub) && find(s, sub) >= 0
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

[export]
def target_scoped_add(a, b : int) : int {
    return scoped_add(a, b)
}

[export]
def target_scoped_pick(v : int) : int {
    return scoped_pick(v)
}

[export]
def target_mirror_rule() : int {
    return scoped_echo(g_shared)
}

[export]
def target_block_literal(src : array<int>) : int {
    return scoped_filter_sum(src) $(v) => v % 2 == 0
}

[export]
def target_inside_with(a, b : int) : int {
    var r : int
    with (module _inline_scope_helper) {
        // the enclosing scope IS the resolution context - the splice lands bare here
        r = scoped_add(a, b)
    }
    return r
}

[test]
def test_scope_wrapped_inline_fired(t : T?) {
    ast_gc_guard() {
        t |> run("[inline] callee with private use splices under with (module)") @(t : T?) {
            let b = body_of(t, "target_scoped_add")
            t |> success(!has(b, "scoped_add("), "call is gone: {b}")
            t |> success(has(b, "hidden_boost("), "private call spliced in: {b}")
        }
        t |> run("multi-return callee rides the same wrapped splice") @(t : T?) {
            let b = body_of(t, "target_scoped_pick")
            t |> success(!has(b, "scoped_pick("), "call is gone: {b}")
        }
        t |> run("call inside user with (module) splices bare") @(t : T?) {
            let b = body_of(t, "target_inside_with")
            t |> success(!has(b, "scoped_add("), "call is gone: {b}")
        }
    }
}

[test]
def test_scope_wrapped_inline_behavior(t : T?) {
    t |> run("private helper and private global values") @(t : T?) {
        t |> equal(target_scoped_add(1, 2), (1 + 7) + (2 + 7))
        t |> equal(target_scoped_pick(5), 5 + 7)
        t |> equal(target_scoped_pick(-5), -(5 + 7))
        t |> equal(target_inside_with(3, 4), (3 + 7) + (4 + 7))
    }
    t |> run("mirror rule: caller global wins over callee module's same-named global") @(t : T?) {
        // caller g_shared = 555 substitutes outside the wrap; body's own g_shared
        // read resolves to the helper's (100). wrong binding gives 100100 or 555555
        t |> equal(target_mirror_rule(), 555 * 1000 + 100)
    }
    t |> run("block-literal auto tier with private use in the body") @(t : T?) {
        // even numbers boosted by secret_bias: (2+7) + (4+7)
        t |> equal(target_block_literal([1, 2, 3, 4]), 9 + 11)
    }
    t |> run("private global write stays visible to the callee module") @(t : T?) {
        with (module _inline_scope_helper) {
            secret_bias = 20
        }
        t |> equal(target_scoped_add(1, 2), (1 + 20) + (2 + 20))
        with (module _inline_scope_helper) {
            secret_bias = 7
        }
    }
}
