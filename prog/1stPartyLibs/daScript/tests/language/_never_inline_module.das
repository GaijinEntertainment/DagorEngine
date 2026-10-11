options gen2
options never_inline

module _never_inline_module public

// fixture for `options never_inline` (inline_never_module.das): nothing defined here
// may be spliced

[inline]
def nvm_add(a, b : int) : int => a + b

def nvm_chain(x : int) : int {
    return nvm_add(x, 1) + 1    // internal MustCall site: the skipped pass leaves it a call
}

def nvm_apply(x : int; blk : block<(v : int) : int>) : int => invoke(blk, x)

def public nvm_gen(a : auto(TT)) : TT {
    return _::nvm_step(a)
}

def public nvm_guarded(a : auto(TT); blk : block<(v : TT) : TT>) : TT {
    if (a < 0) {
        return -a
    }
    return invoke(blk, a)
}
