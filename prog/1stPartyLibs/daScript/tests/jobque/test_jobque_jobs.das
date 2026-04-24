options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

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

