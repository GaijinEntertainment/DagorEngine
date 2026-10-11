// A generator lifts every top-level local into its capture structure, and a block can never
// live there — it is stack storage whose lifetime ends with the enclosing call, so the lifted
// field could only dangle. Infer used to let the declaration through and the failure surfaced
// downstream against the synthesized lambda ("_lambda_thismodule_N() can't be allocated
// locally" plus "block<...>& can't be cloned"), naming generated code instead of the line the
// author wrote. It is now rejected at the declaration, and the same reasoning rejects
// capturing a block into a lambda (30912) rather than reporting it as a copy/move failure.
options gen2
expect 30202 : 1, 30912 : 1

def private use_blk(blk : block<(v : int) : int>) : int {
    // capturing a block into a lambda outlives the block's stack frame
    return invoke(@(a : int) => invoke(blk, a), 21)
}

[export]
def main {
    var gen <- generator<int>() <| $() {
        // a block local inside a generator has nowhere to live
        let blk <- $(v : int) { return v * 2; }
        yield invoke(blk, 21)
        return false
    }
    for (x in gen) {
        print("{x}\n")
    }
    print("{use_blk($(v : int) { return v * 2; })}\n")
}
