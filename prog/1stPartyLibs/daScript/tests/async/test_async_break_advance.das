// Test: per-step coroutine advance pattern.
//
// `for (v in iter) { ... break }` is **destructive** — `SimNode_Seq2Iter`
// moves the iter out of the Sequence into the for-loop's local state, and
// the for-loop's loopend calls `close()` on it. After break+exit, the
// Sequence has `iter == nullptr` AND the iterator state itself is gone, so
// a subsequent `for (v in iter)` panics with "iterator is empty or already
// consumed". This is by design — `for-in` owns the iterator for the
// duration of the loop.
//
// The non-destructive per-step advance is `next(iter, value) : bool` from
// `daslib/builtin`, plus `empty(iter)` to detect exhaustion. That pattern
// preserves the Sequence's internal iter pointer across calls; daslib's
// own `cr_run_all` (`daslib/coroutines.das:142`) uses it. The dasImgui
// Phase 2 runtime's `advance_coroutines` follows the same pattern.
//
// This test pins the working pattern as a regression target.

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/async_boost

[async]
def three_yield() : void {
    await_next_frame()
    await_next_frame()
    await_next_frame()
}

[test]
def test_next_per_step_advance(t : T?) {
    t |> run("next/empty pattern advances one step at a time, repeatedly") @(t : T?) {
        var it <- three_yield()
        var advances = 0
        var loop_iter = 0
        while (loop_iter < 10) {
            loop_iter++
            var v : bool
            next(it, v)
            if (empty(it)) {
                break
            }
            advances++
        }
        // 3 await_next_frame calls before the implicit `return false` from
        // AsyncMacro — three successful next() yields, then iter goes empty.
        t |> equal(advances, 3, "next/empty: 3 yields before exhaustion")
    }
}
