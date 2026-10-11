options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost
require math

// Wrapper struct required by boost helpers (push_clone, for_each_clone, etc.)
// POD types (int, float) cannot be used with new/delete required by these helpers.
struct IntVal {
    v : int
}

// ---- JobStatus tests (no threading) ----

[test]
def test_with_job_status(t : T?) {
    t |> run("with_job_status creates and destroys") @(t : T?) {
        var entered = false
        with_job_status(1) $(status) {
            entered = true
            t |> equal(false, status.isReady)
            t |> equal(true, status.isValid)
        }
        t |> equal(true, entered)
    }
    t |> run("notify makes status ready") @(t : T?) {
        with_job_status(1) $(status) {
            t |> equal(false, status.isReady)
            status |> notify
            t |> equal(true, status.isReady)
        }
    }
    t |> run("multiple notifications") @(t : T?) {
        with_job_status(3) $(status) {
            t |> equal(false, status.isReady)
            status |> notify
            t |> equal(false, status.isReady)
            status |> notify
            t |> equal(false, status.isReady)
            status |> notify
            t |> equal(true, status.isReady)
        }
    }
    t |> run("zero count is immediately ready") @(t : T?) {
        with_job_status(0) $(status) {
            t |> equal(true, status.isReady)
        }
    }
    t |> run("append increments count") @(t : T?) {
        with_job_status(0) $(status) {
            t |> equal(true, status.isReady)
            status |> append(2)
            t |> equal(false, status.isReady)
            status |> notify
            t |> equal(false, status.isReady)
            status |> notify
            t |> equal(true, status.isReady)
        }
    }
    t |> run("notify_and_release sets status null") @(t : T?) {
        with_job_status(1) $(status) {
            status |> add_ref
            t |> equal(false, status.isReady)
            status |> notify_and_release
            // status is now null (notify_and_release sets pointer to null)
            t |> equal(true, status == null)
        }
    }
}

// ---- with_job_que tests ----

[test]
def test_with_job_que(t : T?) {
    t |> run("with_job_que block runs") @(t : T?) {
        var entered = false
        with_job_que() {
            entered = true
            let hw = get_total_hw_jobs()
            t |> equal(true, hw > 0)
        }
        t |> equal(true, entered)
    }
    t |> run("get_total_hw_threads positive") @(t : T?) {
        let threads = get_total_hw_threads()
        t |> equal(true, threads > 0)
    }
}

// ---- new_job tests (use with_channel(N) + notify_and_release pattern) ----

[test]
def test_new_job_basic(t : T?) {
    t |> run("new_job executes lambda in thread pool") @(t : T?) {
        with_job_que() {
            with_channel(1) $(ch) {
                new_job() @() {
                    ch |> push_clone(IntVal(v = 42))
                    ch |> notify_and_release
                }
                var result = 0
                ch |> for_each_clone() $(val : IntVal#) {
                    result = val.v
                }
                t |> equal(42, result)
            }
        }
    }
}

[test]
def test_new_job_multiple(t : T?) {
    t |> run("multiple jobs push to same channel") @(t : T?) {
        with_job_que() {
            let NUM_JOBS = 4
            with_channel(NUM_JOBS) $(ch) {
                for (i in range(NUM_JOBS)) {
                    new_job() @() {
                        ch |> push_clone(IntVal(v = i * 10))
                        ch |> notify_and_release
                    }
                }
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                t |> equal(NUM_JOBS, length(results))
                sort(results)
                for (i in range(NUM_JOBS)) {
                    t |> equal(i * 10, results[i])
                }
            }
        }
    }
}

// ---- new_thread test ----

[test]
def test_new_thread_basic(t : T?) {
    t |> run("new_thread executes in dedicated thread") @(t : T?) {
        with_job_que() {
            with_channel(1) $(ch) {
                new_thread() @() {
                    ch |> push_clone(IntVal(v = 99))
                    ch |> notify_and_release
                }
                var result = 0
                ch |> for_each_clone() $(val : IntVal#) {
                    result = val.v
                }
                t |> equal(99, result)
            }
        }
    }
}

// ---- Captures test ----

[test]
def test_job_with_captures(t : T?) {
    t |> run("job captures local variables") @(t : T?) {
        with_job_que() {
            let multiplier = 7
            with_channel(1) $(ch) {
                new_job() @() {
                    ch |> push_clone(IntVal(v = 6 * multiplier))
                    ch |> notify_and_release
                }
                var result = 0
                ch |> for_each_clone() $(val : IntVal#) {
                    result = val.v
                }
                t |> equal(42, result)
            }
        }
    }
}

// ---- join test: uses both status and channel ----

[test]
def test_job_status_join(t : T?) {
    t |> run("join waits for job completion") @(t : T?) {
        with_job_que() {
            with_job_status(1) $(status) {
                with_channel(1) $(ch) {
                    new_job() @() {
                        ch |> push_clone(IntVal(v = 123))
                        ch |> notify_and_release
                        status |> notify_and_release
                    }
                    status |> join
                    var result = 0
                    ch |> for_each_clone() $(val : IntVal#) {
                        result = val.v
                    }
                    t |> equal(123, result)
                }
            }
        }
    }
}

// ---- persistent job queue: create_job_que / destroy_job_que ----

[test]
def test_persistent_job_que(t : T?) {
    t |> run("new_job runs on a create_job_que pool with no with_job_que block") @(t : T?) {
        create_job_que()
        let NUM_JOBS = 4
        with_channel(NUM_JOBS) $(ch) {
            for (i in range(NUM_JOBS)) {
                new_job() @() {
                    ch |> push_clone(IntVal(v = i * 10))
                    ch |> notify_and_release
                }
            }
            var results : array<int>
            ch |> for_each_clone() $(val : IntVal#) {
                results |> push(val.v)
            }
            t |> equal(NUM_JOBS, length(results))
            sort(results)
            for (i in range(NUM_JOBS)) {
                t |> equal(i * 10, results[i])
            }
        }
        destroy_job_que()
    }
    t |> run("create_job_que works again after destroy_job_que teardown") @(t : T?) {
        create_job_que()
        with_channel(1) $(ch) {
            new_job() @() {
                ch |> push_clone(IntVal(v = 77))
                ch |> notify_and_release
            }
            var result = 0
            ch |> for_each_clone() $(val : IntVal#) {
                result = val.v
            }
            t |> equal(77, result)
        }
        destroy_job_que()
    }
}


// ---- the frame-loop dispatch: one persistent que, polled, never joined ----

// A pass is ready only once every band has notified, so a bounded spin separates
// "still running" from "wedged" — an unbounded one would hang the suite instead of
// failing it if a band's notification is ever lost.
let POLL_LIMIT = 200000000

def private poll_ready(var status : JobStatus?) : bool {
    for (_i in range(POLL_LIMIT)) {
        if (status.isReady) {
            return true
        }
    }
    return false
}

[test]
def test_persistent_que_frame_loop(t : T?) {
    //! The Physarum / Path Tracer Lab dispatch. Their frame loop is driven externally
    //! (emscripten set_main_loop), so a pass is polled through isReady rather than
    //! joined, and the que outlives every pass instead of being rebuilt per frame.
    //! A per-frame new_thread was used here instead for a while; on emscripten that
    //! costs a pthread create/exit round trip through the main thread per band per
    //! frame, which is what this shape exists to avoid.
    t |> run("repeated multi-band passes each land exactly once") @(t : T?) {
        create_job_que()
        let N = 64
        let BANDS = 4
        let PASSES = 25
        var data : array<int>
        data |> resize(N)
        var pdata = unsafe(addr(data))
        var every_pass_ready = true
        for (_p in range(PASSES)) {
            var status = job_status_create()
            status |> append(BANDS)
            let chunk = (N + BANDS - 1) / BANDS
            for (c in range(BANDS)) {
                let lo = c * chunk
                let hi = min((c + 1) * chunk, N)
                var st = status
                new_job() <| @capture(= pdata, = lo, = hi, = st) {
                    for (i in range(lo, hi)) {
                        (*pdata)[i] ++
                    }
                    st |> notify_and_release
                }
            }
            every_pass_ready = every_pass_ready && poll_ready(status)
            unsafe(job_status_remove(status))
        }
        t |> success(every_pass_ready, "every pass reached isReady")
        var advanced = 0
        for (i in range(N)) {
            if (data[i] == PASSES) {
                advanced++
            }
        }
        t |> equal(advanced, N, "every element advanced exactly once per pass")
        destroy_job_que()
    }
}

[test]
def test_persistent_que_empty_band(t : T?) {
    t |> run("a band with no work still balances the wait group") @(t : T?) {
        //! Physarum dispatches MAX_BANDS bands whatever the agent count, so a band
        //! whose range is empty notifies WITHOUT a job. Miss that and the wait group
        //! never reaches ready and the frame loop stalls forever.
        create_job_que()
        let N = 2
        let BANDS = 8          // more bands than elements: 6 of them get nothing
        var data : array<int>
        data |> resize(N)
        var pdata = unsafe(addr(data))
        var status = job_status_create()
        status |> append(BANDS)
        let chunk = (N + BANDS - 1) / BANDS
        for (c in range(BANDS)) {
            let lo = c * chunk
            let hi = min((c + 1) * chunk, N)
            if (lo >= hi) {
                // `notify`, NOT `notify_and_release`: append() raises the remaining count without
                // taking a reference, so releasing here would drop one that was never added.
                status |> notify
                continue
            }
            var st = status
            new_job() <| @capture(= pdata, = lo, = hi, = st) {
                for (i in range(lo, hi)) {
                    (*pdata)[i] ++
                }
                st |> notify_and_release
            }
        }
        t |> success(poll_ready(status), "the wait group balances when bands outnumber work")
        unsafe(job_status_remove(status))
        t |> equal(data[0], 1)
        t |> equal(data[1], 1)
        destroy_job_que()
    }
}
