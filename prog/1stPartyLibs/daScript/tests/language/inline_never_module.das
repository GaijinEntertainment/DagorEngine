options gen2
options rtti
options no_coverage

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost
require _never_inline_module

// structural proof for `options never_inline` (src/ast/ast_inline.cpp); fixture
// module: _never_inline_module.das - the run labels below carry the claims

[inline]
def nvm_step(x : int) : int => x + 1

var g_seed = 1

[never_inline]  // structural anchors resist the heuristic tier (skills/internal/tests_in_repo.md)
def target_must() : int {
    return nvm_add(g_seed, 2)
}

[never_inline]
def target_literal() : int {
    return nvm_apply(5 * g_seed) $(v : int) : int {
        return v * 2
    }
}

[never_inline]
def target_instance() : int {
    return nvm_gen(6 + g_seed)
}

[never_inline]
def target_baseline() : int {
    return nvm_step(3 + g_seed)
}

[never_inline]
def target_canon() : int {
    return nvm_guarded(6 + g_seed) $(v : int) : int {
        return v * 2
    }
}

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
    t |> success(!empty(body), "target function not found")
    let clean <- [for (ln in split(body, "\n")); ln; where !(ln |> has("coverage("))]
    return join(clean, "\n")
}

// a generic instance carries a mangled name (origin`name`hash) the exact-name
// for_each_function lookup misses - match by substring instead
def instance_body_of(t : T?; nm : string) : string {
    var body = ""
    compiling_module() |> for_each_function("") $(func) {
        if (empty(body) && find("{func.name}", nm) >= 0) {
            body = "{describe(func.body)}"
        }
    }
    t |> success(!empty(body), "instance not found")
    let clean <- [for (ln in split(body, "\n")); ln; where !(ln |> has("coverage("))]
    return join(clean, "\n")
}

[test]
def test_never_inline_module(t : T?) {
    g_seed = 1  // runtime taint, written here on purpose: pure targets with constant args fold + reap
    ast_gc_guard() {
        t |> run("[inline] callee from a never_inline module stays a call") @(t : T?) {
            let b = body_of(t, "target_must")
            t |> success(has(b, "nvm_add("), "call survives: {b}")
        }
        t |> run("block-literal tier declines a never_inline callee") @(t : T?) {
            let b = body_of(t, "target_literal")
            t |> success(has(b, "nvm_apply("), "call survives: {b}")
        }
        t |> run("generic instance neither inlines nor hosts splices") @(t : T?) {
            let b = body_of(t, "target_instance")
            t |> success(has(b, "nvm_gen"), "call survives: {b}")  // no paren: the instance call spells the mangled name
            let gb = instance_body_of(t, "nvm_gen")
            t |> success(has(gb, "nvm_step("), "instance body untouched: {gb}")
        }
        t |> run("baseline: the pass is live in this module") @(t : T?) {
            let b = body_of(t, "target_baseline")
            t |> success(!has(b, "nvm_step("), "[inline] splice fired: {b}")
        }
        t |> run("the pass skips the module's own bodies") @(t : T?) {
            var body = ""
            find_module(this_program(), "_never_inline_module") |> for_each_function("nvm_chain") $(func) {
                if (empty(body)) {
                    body = "{describe(func.body)}"
                }
            }
            t |> success(has(body, "nvm_add("), "internal [inline] call survives: {body}")
        }
        t |> run("flagged-origin instance skips return canonicalization") @(t : T?) {
            let gb = instance_body_of(t, "nvm_guarded")
            t |> success(!has(gb, "else"), "early-exit shape kept, no synthesized else: {gb}")
        }
        t |> run("the module bit is set and rtti-visible") @(t : T?) {
            t |> success(find_module(this_program(), "_never_inline_module").moduleFlags.neverInline, "neverInline flag")
        }
    }
    t |> run("targets still compute their values") @(t : T?) {
        t |> equal(target_must(), 3)
        t |> equal(target_literal(), 10)
        t |> equal(target_instance(), 8)
        t |> equal(target_baseline(), 5)
        t |> equal(target_canon(), 14)
    }
}
