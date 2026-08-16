// Regression: a `finally` (here via daslib/defer) runs on EVERY block exit, including
// an early `return` that lexically precedes a later variable's declaration. The
// interpreter zeroes such locals at block entry, but the stack-reuse optimization
// could hand a finally-referenced local's slot to an earlier sub-scope and clobber
// the zero -- so the finally saw garbage (and crashed when it dereferenced a pointer
// local). The stack allocator now disables slot reuse for blocks that have a `finally`,
// so those locals keep distinct, zeroed slots when the finally fires before their
// initializer ran.
options gen2
require dastest/testing_boost public
require daslib/defer

var g_seen_int = -1

def private trigger_int(early : bool) {
    if (early) {
        // sub-scope locals occupying the slot `x` would otherwise reuse on this path
        var junk : int[16]
        for (i in range(16)) {
            junk[i] = 0x40000000 + i
        }
        return
    }
    var x = 12345 // nolint:LINT003 - must stay a slot-backed local; let would const-fold it
    defer() {
        g_seen_int = x   // on the early path x is never initialized; must read as 0, not garbage
    }
}

[test]
def test_finally_local_zeroed_on_early_exit(t : T?) {
    t |> run("finally sees a zeroed local on an early exit before its declaration") @(t : T?) {
        g_seen_int = -1
        trigger_int(true)
        t |> equal(g_seen_int, 0)
    }
}
