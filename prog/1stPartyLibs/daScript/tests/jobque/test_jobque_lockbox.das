options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

// Note: LockBox set/get boost helpers use Feature() which calls context->shared_from_this().
// This crashes when the context is not managed by shared_ptr (as in dastest main context).
// LockBox set/get must be tested inside new_job/new_thread where contexts are properly
// managed via shared_ptr.

struct BoxedInt {
    value : int
}

struct IntVal {
    v : int
}

// ---- Basic tests (no set/get, no job context needed) ----

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
    t |> run("lockbox initially ready") @(t : T?) {
        with_lock_box() $(box) {
            t |> equal(true, box.isValid)
            t |> equal(true, box.isReady)
        }
    }
}

// ---- Set/get tests inside job contexts ----

[test]
def test_lockbox_set_get(t : T?) {
    t |> run("set and get via job") @(t : T?) {
        with_job_que() {
            with_lock_box() $(box) {
                with_channel(1) $(ch) {
                    // Set the value in a job (where context has proper shared_ptr)
                    new_job() @() {
                        box |> set(BoxedInt(value = 42))
                        ch |> push_clone(IntVal(v = 1))
                        box |> release
                        ch |> notify_and_release
                    }
                    // Wait for result
                    var done = false
                    ch |> for_each_clone() $(val : IntVal#) {
                        done = true
                    }
                    t |> equal(true, done)
                }
            }
        }
    }
}

[test]
def test_lockbox_producer_consumer(t : T?) {
    t |> run("producer sets, consumer reads via lockbox") @(t : T?) {
        with_job_que() {
            with_lock_box() $(box) {
                with_channel(2) $(ch) {
                    // Producer: set value in lockbox
                    new_job() @() {
                        box |> set(BoxedInt(value = 99))
                        box |> release
                        ch |> notify_and_release
                    }
                    // Consumer: busy-wait until value is available, then read it
                    new_job() @() {
                        var found = false
                        while (!found) {
                            found = box |> get() $(val : BoxedInt#) {
                                ch |> push_clone(IntVal(v = val.value))
                            }
                        }
                        box |> release
                        ch |> notify_and_release
                    }
                    // Collect results
                    var result = 0
                    ch |> for_each_clone() $(val : IntVal#) {
                        result = val.v
                    }
                    t |> equal(99, result)
                }
            }
        }
    }
}

