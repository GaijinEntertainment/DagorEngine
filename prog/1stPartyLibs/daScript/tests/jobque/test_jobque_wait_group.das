options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

struct IntVal {
    v : int
}

// ---- with_wait_group: convenience wrapper for with_job_status + join ----

[test]
def test_wait_group_basic(t : T?) {
    t |> run("with_wait_group waits for all notifications") @(t : T?) {
        with_job_que() {
            with_channel(3) $(ch) {
                with_wait_group(3) $(wg) {
                    for (i in range(3)) {
                        new_job() @() {
                            ch |> push_clone(IntVal(v = i * 10))
                            ch |> notify_and_release
                            wg |> notify_and_release
                        }
                    }
                }
                // After with_wait_group returns, all jobs are done
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                t |> equal(3, length(results))
                sort(results)
                t |> equal(0, results[0])
                t |> equal(10, results[1])
                t |> equal(20, results[2])
            }
        }
    }
}

[test]
def test_wait_group_zero_count(t : T?) {
    t |> run("with_wait_group with 0 count returns immediately") @(t : T?) {
        var entered = false
        with_wait_group(0) $(wg) {
            entered = true
            t |> equal(true, wg.isReady)
        }
        t |> equal(true, entered)
    }
}

[test]
def test_wait_group_no_count(t : T?) {
    t |> run("with_wait_group no-count variant with append") @(t : T?) {
        with_job_que() {
            with_channel(2) $(ch) {
                with_wait_group() $(wg) {
                    wg |> append(2)
                    for (i in range(2)) {
                        new_job() @() {
                            ch |> push_clone(IntVal(v = i + 1))
                            ch |> notify_and_release
                            wg |> notify_and_release
                        }
                    }
                }
                // All jobs completed
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                t |> equal(2, length(results))
                sort(results)
                t |> equal(1, results[0])
                t |> equal(2, results[1])
            }
        }
    }
}

// ---- done: ergonomic alias for notify_and_release ----

[test]
def test_done_alias(t : T?) {
    t |> run("done signals notification like notify_and_release") @(t : T?) {
        with_job_que() {
            with_channel(1) $(ch) {
                with_wait_group(1) $(wg) {
                    new_job() @() {
                        ch |> push_clone(IntVal(v = 777))
                        ch |> notify_and_release
                        wg |> done
                    }
                }
                var got = 0
                ch |> for_each_clone() $(val : IntVal#) {
                    got = val.v
                }
                t |> equal(777, got)
            }
        }
    }
}

[test]
def test_wait_group_single_job(t : T?) {
    t |> run("with_wait_group with single job") @(t : T?) {
        with_job_que() {
            with_channel(1) $(ch) {
                with_wait_group(1) $(wg) {
                    new_job() @() {
                        ch |> push_clone(IntVal(v = 999))
                        ch |> notify_and_release
                        wg |> notify_and_release
                    }
                }
                var got = 0
                ch |> for_each_clone() $(val : IntVal#) {
                    got = val.v
                }
                t |> equal(999, got)
            }
        }
    }
}

[test]
def test_wait_group_with_thread(t : T?) {
    t |> run("with_wait_group works with new_thread") @(t : T?) {
        with_job_que() {
            with_channel(1) $(ch) {
                with_wait_group(1) $(wg) {
                    new_thread() @() {
                        ch |> push_clone(IntVal(v = 123))
                        ch |> notify_and_release
                        wg |> notify_and_release
                    }
                }
                var got = 0
                ch |> for_each_clone() $(val : IntVal#) {
                    got = val.v
                }
                t |> equal(123, got)
            }
        }
    }
}
