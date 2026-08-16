// Cross-module callees with unsafe bodies for optimization_inline_unsafe.das: a compiled
// module loses its `unsafe { }` wrappers to foldUnsafe, so splicing these bodies exercises
// the inliner's generated re-authorization wrapper. `_`-prefix keeps dastest from running it.
options gen2

module _inline_unsafe_helper public

def sum_via_ptr(src : array<int>; blk : block<(v : int) : int>) : int {
    // multi-statement block-form unsafe body
    var total = 0
    unsafe {
        var p = addr(total)
        for (v in src) {
            *p += invoke(blk, v)
        }
    }
    return total
}

def first_via_ptr(src : array<int>; blk : block<(v : int) : int>) : int {
    // the invoke inside the unsafe region devirtualizes on a later round: its arg
    // temp (deref/addr) must anchor INSIDE the spliced wrapper block
    var r : int
    unsafe {
        r = invoke(blk, *addr(src[0]))
    }
    return r
}

def expr_unsafe_sum(src : array<int>; blk : block<(v : int) : int>) : int {
    // expression-form unsafe only: alwaysSafe is sticky on the cloned nodes; the splice
    // drops the user-said bit so the caller module's unsafe accounting stays unchanged
    var total = 0
    for (v in src) {
        total += invoke(blk, unsafe(reinterpret<int>(uint(v))))
    }
    return total
}

[unsafe_deref]
def deref_first(src : array<int>; blk : block<(v : int) : int>) : int {
    var r : int
    unsafe {
        r = invoke(blk, *addr(src[0]))
    }
    return r
}

[inline]
def scaled_bits(x : int; k : int) : int {
    // [inline] MUST callee with a block-form unsafe body: spliced wrapper-less, this
    // re-errored `cast requires unsafe` at the call site before the generated wrapper
    var r : int
    unsafe {
        r = int(reinterpret<uint>(x)) * k
    }
    return r
}
