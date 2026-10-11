options gen2
options max_fast_call_depth = 256

require dastest/testing_boost public
require daslib/ast
require daslib/rtti
require daslib/debugger
require daslib/fio
require strings
require _fast_call_depth_recursers

//! The recursers live in a no_aot module, so this file runs under AOT and JIT as well: each
//! sweep exercises its own recover handler (the try node, jit_try_recover, das_try_recover).
//! An instrumented run (dastest --cov-path, the debugger) rewrites every body, so nothing is
//! fastcall and the guard counts nothing: the exactness arms skip there, by the probe below.

let DEPTH_LIMIT = 256

let UNARMED = "fastcall is off in this run (instrumented bodies), the guard counts nothing"

//! The guard's own message from an unbounded recursion; any other panic means fastcall is off.
def guard_armed : bool {
    let why = panic_text() $ {
        g_sink = rec_forever(g_no_fold)
    }
    return find(why, "max_fast_call_depth") >= 0
}

[test]
def test_fast_call_depth_panics(t : T?) {
    g_no_fold = 0
    if (!guard_armed()) {
        t |> skip(UNARMED)
        return
    }
    t |> run("unbounded fastcall recursion panics naming the option") @(t : T?) {
        let why = panic_text() $ {
            g_sink = rec_forever(g_no_fold)
        }
        t |> success(find(why, "stack overflow, max_fast_call_depth 256 exceeded while calling") >= 0, why)
    }
    t |> run("one argument: at the cap passes, one call past it panics") @(t : T?) {
        t |> equal(framed_rec_sum(DEPTH_LIMIT - 1 + g_no_fold), triangular(DEPTH_LIMIT - 1))
        t |> success(panic_text() $ { g_sink = framed_rec_sum(DEPTH_LIMIT + g_no_fold) } != "", "one call past the cap must panic")
    }
    t |> run("two plain arguments, the fused shape") @(t : T?) {
        t |> equal(framed_rec_sum2(DEPTH_LIMIT - 1 + g_no_fold), triangular(DEPTH_LIMIT - 1))
        t |> success(panic_text() $ { g_sink = framed_rec_sum2(DEPTH_LIMIT + g_no_fold) } != "", "one call past the cap must panic")
    }
    t |> run("a computed left argument, the fused Any-left shape") @(t : T?) {
        t |> equal(framed_rec_any_left(DEPTH_LIMIT - 1 + g_no_fold), 7)
        t |> success(panic_text() $ { g_sink = framed_rec_any_left(DEPTH_LIMIT + g_no_fold) } != "", "one call past the cap must panic")
    }
    t |> run("a computed right argument, the fused Any-right shape") @(t : T?) {
        t |> equal(framed_rec_any_right(DEPTH_LIMIT - 1 + g_no_fold), 7)
        t |> success(panic_text() $ { g_sink = framed_rec_any_right(DEPTH_LIMIT + g_no_fold) } != "", "one call past the cap must panic")
    }
    t |> run("nine arguments, the variadic node") @(t : T?) {
        t |> equal(framed_rec_nine(DEPTH_LIMIT - 1 + g_no_fold), 36)
        t |> success(panic_text() $ { g_sink = framed_rec_nine(DEPTH_LIMIT + g_no_fold) } != "", "one call past the cap must panic")
    }
}

[test]
def test_recovered_overflow_leaves_no_drift(t : T?) {
    g_no_fold = 0
    if (!guard_armed()) {
        t |> skip(UNARMED)
        return
    }
    t |> run("a thousand recovered overflows, then a recursion at the cap still passes") @(t : T?) {
        for (_i in range(1000)) {
            try {
                g_sink = rec_forever(g_no_fold)
            } recover {
            }
        }
        t |> equal(framed_rec_sum(DEPTH_LIMIT - 1 + g_no_fold), triangular(DEPTH_LIMIT - 1))
    }
}

//! A program with no options line, capped at 16 by the host's CodeOfPolicies: at_cap nests 16
//! calls, past_cap 17, and overflow never returns.
let HOST_CAPPED_SOURCE = "options gen2\nvar g_no_fold = 0\nvar g_sink = 0\ndef rec_sum(n : int) : int \{\n    return n == 0 ? 0 : n + rec_sum(n - 1)\n\}\ndef rec_forever(n : int) : int \{\n    return rec_forever(n + 1)\n\}\n[export]\ndef at_cap \{\n    g_sink = rec_sum(15 + g_no_fold)\n\}\n[export]\ndef past_cap \{\n    g_sink = rec_sum(16 + g_no_fold)\n\}\n[export]\ndef overflow \{\n    g_sink = rec_forever(g_no_fold)\n\}\n"

[test]
def test_host_policy_and_host_catch(t : T?) {
    // dagor: no test_temp_dir() in the vendored dastest, and an inscope local cannot live in a run lambda
    let tmp = create_temp_file_result("host_capped", ".das")
    t |> success(tmp is value, "temporary fixture created")
    if (!(tmp is value)) return
    let path = unsafe(tmp.value)
    fwrite(path, HOST_CAPPED_SOURCE)
    {
        var inscope access <- make_file_access("")
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.max_fast_call_depth = 16u
                cop.threadlock_context = true
                compile_file(path, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                    t |> success(ok, "the capped program compiles: {issues}")
                    return if (!ok)
                    simulate(program) $(sok; ctx; serrors) {
                        t |> success(sok, "the capped program simulates: {serrors}")
                        return if (!sok)
                        unsafe {
                            t |> equal(panic_text() $ { invoke_in_context(ctx, "at_cap") }, "")
                            let past = panic_text() $ { invoke_in_context(ctx, "past_cap") }
                            t |> success(find(past, "max_fast_call_depth 16 exceeded") >= 0, "the host policy is the cap: {past}")
                            let escaped = panic_text() $ { invoke_in_context(ctx, "overflow") }
                            t |> success(find(escaped, "max_fast_call_depth 16 exceeded") >= 0, "the overflow escapes to the caller: {escaped}")
                            t |> equal(panic_text() $ { invoke_in_context(ctx, "at_cap") }, "")
                        }
                    }
                }
            }
        }
    }
    remove(path)
}
