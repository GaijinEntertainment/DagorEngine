options gen2
module _llvm_split_modules_mod_a

require _llvm_split_modules_mod_b

struct public SplitPair {
    lo : int
    hi : int
}

// cross-partition direct call a -> b, plus the shared interp-call glob
def public helper_a(x : int) : int => helper_b(x) + slow_b(x)

// cmres-returning cross-partition callee (struct result exercises the cmres impl ABI)
def public make_pair(x : int) : SplitPair => SplitPair(lo = helper_b(x), hi = x * x)
