// Cross-module callees whose bodies use THIS module's private symbols. The splice
// gate used to refuse them ("body uses private symbol"); they now splice under a
// generated `with (module _inline_scope_helper)` wrapper (optimization_inline_scope).
// secret_bias is a `var` so the private reference survives constant folding.
// `_`-prefix keeps dastest from running this file.
options gen2

module _inline_scope_helper public

var private secret_bias = 7

def private hidden_boost(x : int) : int => x + secret_bias

// same name as the caller's global: the mirror-rule collision probe. private, so the
// CALLER's unqualified g_shared stays unambiguous; inside the generated scope wrapper
// the body's read resolves here regardless (the enclosing module is not visible there)
var private g_shared = 100

[inline]
def scoped_add(a, b : int) : int {
    return hidden_boost(a) + hidden_boost(b)
}

[inline]
def scoped_pick(v : int) : int {
    // multi-return shape rides the same wrapped splice (flag tier + scope wrap compose)
    if (v > 0) {
        return hidden_boost(v)
    }
    return -hidden_boost(-v)
}

[inline]
def scoped_echo(a : int) : int {
    // reads the substituted argument AND this module's g_shared: the arg binds a temp
    // outside the wrap, so a caller global passed as `a` keeps the caller's value even
    // though this module has a same-named global
    return hidden_boost(a * 1000 + g_shared) - secret_bias
}

def scoped_filter_sum(src : array<int>; blk : block<(v : int) : bool>) : int {
    // auto tier (block-literal argument at the call site), private use in the body
    var total = 0
    for (v in src) {
        if (invoke(blk, v)) {
            total += hidden_boost(v)
        }
    }
    return total
}
