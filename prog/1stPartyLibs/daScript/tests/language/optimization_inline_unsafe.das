options gen2
options rtti
options no_coverage

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost
require _inline_unsafe_helper

// unsafe callee bodies splice under a generated `unsafe { }` wrapper
// (src/ast/ast_inline.cpp): a compiled callee lost its wrappers to foldUnsafe, so a
// wrapper-less spliced body re-errors against the caller on re-infer. FIRED proof: the
// cross-module calls (and the [inline] MUST call) are gone, invokes inside the unsafe
// region devirtualize; behavioral: values identical. The negative pins the kept gate:
// [unsafe_deref]/[unsafe_operation] change runtime semantics when spliced (re-infer
// re-stamps deref null-check elision from the CALLER's flag), so they stay calls.

def local_unsafe_sum(src : array<int>; blk : block<(v : int) : int>) : int {
    // same-module callee: its own `unsafe { }` wrapper is still present at patch time
    var total = 0
    unsafe {
        var p = addr(total)
        for (v in src) {
            *p += invoke(blk, v)
        }
    }
    return total
}

[export]
def target_block_unsafe(src : array<int>) : int {
    return src |> sum_via_ptr() $(v) => v * v
}

[export]
def target_single_return_unsafe(src : array<int>) : int {
    return src |> first_via_ptr() $(v) => v + 10
}

[export]
def target_expr_unsafe(src : array<int>) : int {
    return src |> expr_unsafe_sum() $(v) => v * 3
}

[export]
def target_local_unsafe(src : array<int>) : int {
    return src |> local_unsafe_sum() $(v) => v - 1
}

[export]
def target_unsafe_deref(src : array<int>) : int {
    return src |> deref_first() $(v) => v + 1
}

[export]
def target_must_unsafe(x : int) : int {
    return scaled_bits(x, 7)
}

[export]
def target_devirt_unsafe_literal(x : int) : int {
    // caller-origin literal with expression-form unsafe: the devirt splice keeps the
    // caller's own unsafe accounting (userSaidItsSafe survives non-callee splices)
    let blk <- $(v : int) : int {
        return unsafe(reinterpret<int>(uint(v))) + 1
    }
    return invoke(blk, x)
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
def test_inline_unsafe_fired(t : T?) {
    ast_gc_guard() {
        t |> run("cross-module block-form unsafe body splices") @(t : T?) {
            let b = body_of(t, "target_block_unsafe")
            t |> success(!has(b, "sum_via_ptr("), "call is gone: {b}")
            t |> success(!has(b, "invoke("), "invoke devirtualized: {b}")
        }
        t |> run("invoke inside the unsafe region devirtualizes") @(t : T?) {
            let b = body_of(t, "target_single_return_unsafe")
            t |> success(!has(b, "first_via_ptr("), "call is gone: {b}")
            t |> success(!has(b, "invoke("), "invoke devirtualized: {b}")
        }
        t |> run("expression-form unsafe body splices") @(t : T?) {
            let b = body_of(t, "target_expr_unsafe")
            t |> success(!has(b, "expr_unsafe_sum("), "call is gone: {b}")
            t |> success(!has(b, "invoke("), "invoke devirtualized: {b}")
        }
        t |> run("same-module unsafe body splices") @(t : T?) {
            let b = body_of(t, "target_local_unsafe")
            t |> success(!has(b, "local_unsafe_sum("), "call is gone: {b}")
            t |> success(!has(b, "invoke("), "invoke devirtualized: {b}")
        }
        t |> run("[unsafe_deref] callee stays a call") @(t : T?) {
            let b = body_of(t, "target_unsafe_deref")
            t |> success(has(b, "deref_first("), "call stays regular: {b}")
        }
        t |> run("[inline] MUST callee with unsafe body splices") @(t : T?) {
            let b = body_of(t, "target_must_unsafe")
            t |> success(!has(b, "scaled_bits("), "call is gone: {b}")
        }
        t |> run("caller-origin unsafe literal devirtualizes") @(t : T?) {
            let b = body_of(t, "target_devirt_unsafe_literal")
            t |> success(!has(b, "invoke("), "invoke devirtualized: {b}")
        }
    }
}

[test]
def test_inline_unsafe_behavior(t : T?) {
    t |> run("values preserved") @(t : T?) {
        let src <- [1, 2, 3, 4]
        t |> equal(target_block_unsafe(src), 30)
        t |> equal(target_single_return_unsafe(src), 11)
        t |> equal(target_expr_unsafe(src), 30)
        t |> equal(target_local_unsafe(src), 6)
        t |> equal(target_unsafe_deref(src), 2)
        t |> equal(target_must_unsafe(6), 42)
        t |> equal(target_devirt_unsafe_literal(41), 42)
    }
}
