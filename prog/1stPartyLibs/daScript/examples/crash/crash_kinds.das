options gen2

// One deliberate failure per run, so a supervisor + dump collector + symbol bundle can be validated
// against a known shape. The point is coverage of what each tier *cannot* see:
//
//   - the in-process handler (src/hal/crash_handler.cpp) prints a symbolized stack to stderr for
//     SEH faults, but declines EXCEPTION_STACK_OVERFLOW and never sees fast-fail (abort/terminate);
//   - WER local dumps catch fast-fail and stack overflow, but only if configured;
//   - a das panic is neither: the host prints `EXCEPTION: ...` and exits 1 with no fault at all.
//
// A run that produces no evidence in any tier is the interesting result, not a broken test.

module crash_kinds public

require crash
require daslib/defer
require daslib/fio                    // exit()
require daslib/jobque_boost

enum CrashKind {
    none                //!< control: start, print, return 0
    das_panic           //!< das panic — host prints `EXCEPTION:`, exit 1, no fault, no dump
    das_verify          //!< failed verify() — same path, and unlike assert survives Release
    das_exit_code       //!< main returns a non-zero code — clean exit, no output, no dump
    das_fio_exit        //!< script calls fio's exit(1) directly, bypassing main's return entirely
    das_array_bounds    //!< out-of-range index — das-level runtime error
    null_write          //!< native AV write (0xC0000005) — handler prints a stack
    null_read           //!< native AV read (0xC0000005)
    wild_read           //!< native AV read at ~0 offset from -1 — a corrupted-pointer shape
    divide_by_zero      //!< native integer divide by zero (0xC0000094)
    stack_overflow      //!< native stack exhaustion (0xC00000FD) — handler declines it
    uncaught_exception  //!< C++ throw past the boundary: terminate -> abort (0xC0000409 subcode 7)
    native_terminate    //!< direct std::terminate() — probes whether set_terminate is reachable
    native_abort        //!< direct abort() — fast-fail with no exception in flight
    worker_null_write   //!< native AV on a jobque worker — the shape seen in dasllama-server
    worker_das_panic    //!< das panic on a jobque worker
}

def crash_kind_is_native(kind : CrashKind) : bool {
    return !(kind == CrashKind.none || kind == CrashKind.das_panic || kind == CrashKind.das_verify ||
             kind == CrashKind.das_exit_code || kind == CrashKind.das_array_bounds ||
             kind == CrashKind.worker_das_panic)
}

struct private WorkerDone {
    ok : int
}

// The channel is the synchronization and the timeout is the verdict: reaching it means the worker
// failure neither killed the process nor surfaced anywhere, which is the outcome worth knowing
// about. Waiting here rather than leaning on `with_job_que`'s scope-exit drain keeps that verdict
// this function's own — the drain would otherwise decide the outcome before the check runs.
def private run_on_worker(use_panic : bool) : int {
    var completed = false
    with_job_que() {
        with_channel(1) $(ch) {
            new_job() @() {
                if (use_panic) {
                    panic("deliberate das panic on a jobque worker")
                } else {
                    native_null_write()
                }
                // Only reached if the failure above did not stop this worker.
                ch |> push_clone(WorkerDone(ok = 1))
                ch |> notify_and_release
            }
            // The value carries nothing — arrival is the whole signal.
            ch |> pop_with_timeout_clone(5000) $(_v : WorkerDone#) {
                completed = true
            }
        }
    }
    print("crash: worker returned, job_completed={completed} — the process survived a worker failure\n")
    return completed ? 0 : 3
}

def private das_out_of_bounds() : int {
    var xs <- [1, 2, 3]
    defer() { delete xs }
    var total = 0
    // Deliberately walks past the end; the index is opaque enough that the bounds check stays.
    for (i in range(0, length(xs) + 4)) {
        total += xs[i]
    }
    return total
}

//! Run one failure mode. Returns only for the kinds that are not supposed to fail (`none`), or
//! yields the requested exit code for `das_exit_code`.
def run_crash_kind(kind : CrashKind; exit_code : int) : int {
    if (kind == CrashKind.none) {
        return 0
    } elif (kind == CrashKind.das_panic) {
        panic("deliberate das panic from examples/crash")
    } elif (kind == CrashKind.das_verify) {
        verify(false, "deliberate verify failure from examples/crash")
    } elif (kind == CrashKind.das_exit_code) {
        return exit_code
    } elif (kind == CrashKind.das_fio_exit) {
        // Terminates the process here — main never returns, so the host's exit-code plumbing and
        // every EXCEPTION path are bypassed.
        unsafe(exit(exit_code))
    } elif (kind == CrashKind.das_array_bounds) {
        print("unreachable: {das_out_of_bounds()}\n")
    } elif (kind == CrashKind.null_write) {
        native_null_write()
    } elif (kind == CrashKind.null_read) {
        print("unreachable: {native_null_read()}\n")
    } elif (kind == CrashKind.wild_read) {
        print("unreachable: {native_wild_read()}\n")
    } elif (kind == CrashKind.divide_by_zero) {
        print("unreachable: {native_divide_by_zero()}\n")
    } elif (kind == CrashKind.stack_overflow) {
        native_stack_overflow()
    } elif (kind == CrashKind.uncaught_exception) {
        native_uncaught_exception()
    } elif (kind == CrashKind.native_terminate) {
        native_terminate()
    } elif (kind == CrashKind.native_abort) {
        native_abort()
    } elif (kind == CrashKind.worker_null_write) {
        // A fault on a worker thread, not the main thread — the dasllama-server signature.
        return run_on_worker(false)
    } elif (kind == CrashKind.worker_das_panic) {
        return run_on_worker(true)
    }
    return 0
}
