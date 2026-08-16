options gen2
options auto_inline_functions = true
require dastest/testing_boost
require _inline_shadow_helper

// A devirt site that DECLINES (can_shadow capture risk) after the eager path already hoisted
// its impure evaluation prefix used to orphan the `__inl<N>_pre*` reference: the statement was
// rewritten in place, then the decline discarded the pending temp declarations - 30838 at best,
// a segfault in the access-flags pass at worst. All four ingredients are load-bearing: two
// devirt sites in one statement, an if-condition (forces the eager path), an impure literal
// body (forces the prefix hoist), and a caller declaration spelling the can_shadow name.

var g = 0

def apply2(k : block<(team : int) : int>) : int {
    var minn = 1   // nolint:LINT003 — a `let` would const-fold into k(1) and change the pinned devirt shape
    if (k(1) < k(minn)) {
        return 1
    }
    return 0
}

// the function ARGUMENT spells the can_shadow name (a `var team` let would also trip the
// pre-existing aot_module RenameVar shadow bug, which is not what this test pins)
def run_it(team : int) : int {
    let r = apply2() $ [shadowable] (team : int) : int {
        g += team   // impure: forces the prefix hoist
        return team * 2
    }
    return r + team
}

[test]
def test_declined_site_keeps_its_statement(t : T?) {
    g = 0
    t |> equal(run_it(10), 10)
    t |> equal(g, 2)
}
