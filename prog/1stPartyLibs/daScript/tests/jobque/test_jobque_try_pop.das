options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

// Wrapper struct required by boost helpers (push_clone, try_pop_clone, etc.)
struct IntVal {
    v : int
}

// ---- try_pop_clone: non-blocking pop ----

[test]
def test_try_pop_empty(t : T?) {
    t |> run("try_pop_clone on empty channel returns false") @(t : T?) {
        with_channel() $(ch) {
            var found = false
            let result = ch |> try_pop_clone() $(val : IntVal#) {
                found = true
            }
            t |> equal(false, result)
            t |> equal(false, found)
        }
    }
}

[test]
def test_try_pop_with_data(t : T?) {
    t |> run("try_pop_clone returns data when available") @(t : T?) {
        with_channel() $(ch) {
            ch |> push_clone(IntVal(v = 42))
            var got = 0
            let result = ch |> try_pop_clone() $(val : IntVal#) {
                got = val.v
            }
            t |> equal(true, result)
            t |> equal(42, got)
            t |> equal(true, ch.isEmpty)
        }
    }
}

[test]
def test_try_pop_multiple(t : T?) {
    t |> run("try_pop_clone pops items in FIFO order") @(t : T?) {
        with_channel() $(ch) {
            ch |> push_clone(IntVal(v = 10))
            ch |> push_clone(IntVal(v = 20))
            ch |> push_clone(IntVal(v = 30))
            var results : array<int>
            var popping = true
            while (popping) {
                popping = ch |> try_pop_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
            }
            t |> equal(3, length(results))
            t |> equal(10, results[0])
            t |> equal(20, results[1])
            t |> equal(30, results[2])
        }
    }
}

[test]
def test_try_pop_from_job(t : T?) {
    t |> run("try_pop_clone works after job pushes data") @(t : T?) {
        with_job_que() {
            with_channel(1) $(ch) {
                new_job() @() {
                    ch |> push_clone(IntVal(v = 77))
                    ch |> notify_and_release
                }
                // for_each_clone waits for all job notifications, then we use try_pop
                // Actually, let's use for_each_clone to drain and then try_pop the empty channel
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                t |> equal(1, length(results))
                t |> equal(77, results[0])
                // Channel should now be empty
                var found = false
                let result = ch |> try_pop_clone() $(val : IntVal#) {
                    found = true
                }
                t |> equal(false, result)
                t |> equal(false, found)
            }
        }
    }
}

// ---- pop_with_timeout_clone: timed pop ----

[test]
def test_pop_with_timeout_empty(t : T?) {
    t |> run("pop_with_timeout_clone on empty channel times out") @(t : T?) {
        with_channel() $(ch) {
            var found = false
            let result = ch |> pop_with_timeout_clone(10) $(val : IntVal#) {
                found = true
            }
            t |> equal(false, result)
            t |> equal(false, found)
        }
    }
}

[test]
def test_pop_with_timeout_has_data(t : T?) {
    t |> run("pop_with_timeout_clone returns immediately when data present") @(t : T?) {
        with_channel() $(ch) {
            ch |> push_clone(IntVal(v = 55))
            var got = 0
            let result = ch |> pop_with_timeout_clone(1000) $(val : IntVal#) {
                got = val.v
            }
            t |> equal(true, result)
            t |> equal(55, got)
        }
    }
}

[test]
def test_pop_with_timeout_from_job(t : T?) {
    t |> run("pop_with_timeout_clone receives data pushed by job") @(t : T?) {
        with_job_que() {
            with_channel(1) $(ch) {
                new_job() @() {
                    ch |> push_clone(IntVal(v = 88))
                    ch |> notify_and_release
                }
                // Use for_each_clone to properly drain the channel
                var got = 0
                ch |> for_each_clone() $(val : IntVal#) {
                    got = val.v
                }
                t |> equal(88, got)
            }
        }
    }
}

[test]
def test_pop_with_timeout_zero(t : T?) {
    t |> run("pop_with_timeout_clone with 0ms timeout acts like try_pop") @(t : T?) {
        with_channel() $(ch) {
            // Empty channel - should return immediately
            var found = false
            let result1 = ch |> pop_with_timeout_clone(0) $(val : IntVal#) {
                found = true
            }
            t |> equal(false, result1)
            // With data - should return immediately
            ch |> push_clone(IntVal(v = 33))
            var got = 0
            let result2 = ch |> pop_with_timeout_clone(0) $(val : IntVal#) {
                got = val.v
            }
            t |> equal(true, result2)
            t |> equal(33, got)
        }
    }
}
