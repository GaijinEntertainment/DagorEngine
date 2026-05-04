options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

struct IntVal {
    v : int
}

// ---- Single-item edge case ----

[test]
def test_channel_single_item(t : T?) {
    t |> run("single push then for_each_clone") @(t : T?) {
        with_channel() $(ch) {
            ch |> push_clone(IntVal(v = 777))
            var results : array<int>
            ch |> for_each_clone() $(val : IntVal#) {
                results |> push(val.v)
            }
            t |> equal(1, length(results))
            t |> equal(777, results[0])
        }
    }
}

// ---- Large batch ----

[test]
def test_channel_large_batch(t : T?) {
    t |> run("push and pop 100 items") @(t : T?) {
        let N = 100
        with_channel() $(ch) {
            for (i in range(N)) {
                ch |> push_clone(IntVal(v = i))
            }
            t |> equal(N, ch.total)
            var results : array<int>
            ch |> for_each_clone() $(val : IntVal#) {
                results |> push(val.v)
            }
            t |> equal(N, length(results))
            for (i in range(N)) {
                t |> equal(i, results[i])
            }
        }
    }
}

// ---- Empty channel pop ----

[test]
def test_channel_empty_pop(t : T?) {
    t |> run("pop from empty channel returns false") @(t : T?) {
        with_channel() $(ch) {
            var found = false
            ch |> pop_and_clone_one() $(val : IntVal#) {
                found = true
            }
            t |> equal(false, found)
        }
    }
}

// ---- Lockbox create/destroy ----

[test]
def test_lockbox_basics(t : T?) {
    t |> run("with_lock_box creates and destroys") @(t : T?) {
        var entered = false
        with_lock_box() $(box) {
            entered = true
            t |> equal(true, box.isValid)
        }
        t |> equal(true, entered)
    }
}

// ---- Channel properties tracking ----

[test]
def test_channel_properties(t : T?) {
    t |> run("isEmpty and total track correctly") @(t : T?) {
        with_channel() $(ch) {
            t |> equal(true, ch.isEmpty)
            t |> equal(0, ch.total)
            ch |> push_clone(IntVal(v = 1))
            t |> equal(false, ch.isEmpty)
            t |> equal(1, ch.total)
            ch |> push_clone(IntVal(v = 2))
            t |> equal(2, ch.total)
            ch |> push_clone(IntVal(v = 3))
            t |> equal(3, ch.total)
        }
    }
}

// ---- gather_and_forward ----

[test]
def test_gather_and_forward(t : T?) {
    t |> run("gather_and_forward copies data between channels") @(t : T?) {
        with_channel() $(src) {
            with_channel() $(dst) {
                src |> push_clone(IntVal(v = 10))
                src |> push_clone(IntVal(v = 20))
                src |> push_clone(IntVal(v = 30))
                var gathered : array<int>
                src |> gather_and_forward(dst) $(val : IntVal#) {
                    gathered |> push(val.v)
                }
                t |> equal(3, length(gathered))
                // dst should now have the data
                t |> equal(3, dst.total)
                var forwarded : array<int>
                dst |> for_each_clone() $(val : IntVal#) {
                    forwarded |> push(val.v)
                }
                t |> equal(3, length(forwarded))
            }
        }
    }
}

// ---- Concurrent jobs pushing to same channel ----

[test]
def test_concurrent_jobs(t : T?) {
    t |> run("multiple jobs pushing to same channel") @(t : T?) {
        with_job_que() {
            let NUM_JOBS = 8
            with_channel(NUM_JOBS) $(ch) {
                for (i in range(NUM_JOBS)) {
                    new_job() @() {
                        ch |> push_clone(IntVal(v = i))
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
                    t |> equal(i, results[i])
                }
            }
        }
    }
}

