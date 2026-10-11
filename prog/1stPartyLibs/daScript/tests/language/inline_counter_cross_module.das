options gen2
options rtti
options no_coverage

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost
require _inline_counter_helper

// cross-module splice of a body that already carries interior _inl0_* temps
// (src/ast/ast_inline.cpp): pins that the interior temps survive the splice through
// the rename join (never minting `__`) and that the site mints its own distinct ids.
// the cross-module counter BUMP itself has no distinguishing test - a one-site splice
// renames interior temps into the site's namespace either way (claim stated in PR #3716)

var g_seed = 1

def impure() : int {
    g_seed += 0
    return g_seed
}

[never_inline]
def target_cross() : int {
    return cnt_outer(2 + impure())
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

[test]
def test_counter_bump(t : T?) {
    g_seed = 1
    ast_gc_guard() {
        t |> run("interior temps survive a cross-module splice, no `__` minted") @(t : T?) {
            let b = body_of(t, "target_cross")
            t |> success(has(b, "u_inl0_"), "interior temp lost its rename join: {b}")
            t |> success(has(b, "_arg_v"), "site arg temp missing: {b}")
            t |> success(!has(b, "__"), "GLSL-reserved `__` minted: {b}")
        }
    }
    t |> run("value") @(t : T?) {
        t |> equal(target_cross(), 8)
    }
}
