options gen2
options indenting = 4
options rtti

require dastest/testing_boost public
require daslib/ast
require daslib/ast_boost
require daslib/templates_boost

//! describe() must be a pure function of node structure. The printer marks sticky per-node
//! print flags (argLevel / topLevel / bottomLevel); without the pre-print wipe a node once
//! printed as a call argument keeps argLevel forever and describes WITHOUT outer parens,
//! while an identical fresh node describes with them. CSE value-numbering keys on describe,
//! so the unstable key silently loses shares (glitch_2d spelled `_cse_2 + _cse_1` twice and
//! shared it zero times).

[test]
def test_describe_purity(t : T?) {
    ast_gc_guard() {
        var fresh = qmacro(1f - x)
        let d0 = "{describe(fresh)}"
        // print an identical sum in a CALL-ARG position — this used to stamp argLevel on it
        // for good, so its next standalone describe dropped the outer parens
        var wrapped = qmacro(sin(1f - x))
        let _whole = "{describe(wrapped)}"
        if (wrapped is ExprCall) {
            var inner = (wrapped as ExprCall).arguments[0]
            t |> equal("{describe(inner)}", d0, "print history must not change a subtree's describe")
        } else {
            t |> failure("expected qmacro(sin(…)) to parse as ExprCall, got {wrapped.__rtti}")
        }
        // the fresh node's own describe must not drift across repeated prints either
        t |> equal("{describe(fresh)}", d0, "repeated describe of the same node must be stable")
    }
}
