options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// STYLE029: 'require X' used only for ONE module X re-exports — require it
// directly and drop X. Silent when >=2 re-exports are used (aggregation
// facade, e.g. daslib/ast) or the single used re-export is builtin.

expect 31209:1

require daslib/style_lint
require _style029_proxy        // only leaf used — FIRES
require _style029_direct       // used directly — silent
require _style029_mixed_proxy  // math + leaf both used — silent

[export]
def use_them(v : float) : int {
    // sqrt(v) keeps the math use alive through const-folding
    return style029_leaf_value() + style029_direct_value() + int(sqrt(v))
}
