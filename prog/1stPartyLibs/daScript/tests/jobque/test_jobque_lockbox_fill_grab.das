options gen2
options no_aot
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

struct IntVal {
    v : int
}

// ---- fill/grab basics ----

[test]
def test_lockbox_fill_grab_ready(t : T?) {
    t |> run("lockbox starts isReady (empty)") @(t : T?) {
        with_lock_box() $(box) {
            t |> equal(true, box.isReady)
        }
    }
}

[test]
def test_lockbox_fill_marks_not_ready(t : T?) {
    t |> run("fill marks box as not ready") @(t : T?) {
        with_job_que() {
            with_lock_box() $(box) {
                with_channel(1) $(ch) {
                    new_job() @() {
                        let data = 42
                        box |> fill(data)
                        ch |> push_clone(IntVal(v = box.isReady ? 1 : 0))
                        box |> release
                        ch |> notify_and_release
                    }
                    var ready_after_fill = 1
                    ch |> for_each_clone() $(val : IntVal#) {
                        ready_after_fill = val.v
                    }
                    t |> equal(0, ready_after_fill)
                }
            }
        }
    }
}

[test]
def test_lockbox_grab_returns_data(t : T?) {
    t |> run("grab reads data and marks ready") @(t : T?) {
        with_job_que() {
            with_lock_box() $(box) {
                with_channel(1) $(ch) {
                    // fill and grab in the same job — data must outlive grab
                    new_job() @() {
                        var buf : array<float>
                        buf |> push(10.0)
                        buf |> push(20.0)
                        buf |> push(30.0)
                        box |> fill(buf)
                        var grabbed_sum = 0.0
                        var grabbed_len = 0
                        box |> grab() $(var arr : array<float>) {
                            grabbed_len = length(arr)
                            for (v in arr) {
                                grabbed_sum += v
                            }
                        }
                        ch |> push_clone(IntVal(v = grabbed_len))
                        box |> release
                        ch |> notify_and_release
                    }
                    var result_len = 0
                    ch |> for_each_clone() $(val : IntVal#) {
                        result_len = val.v
                    }
                    t |> equal(3, result_len)
                    t |> equal(true, box.isReady)
                }
            }
        }
    }
}

[test]
def test_lockbox_grab_empty_returns_false(t : T?) {
    t |> run("grab on empty box returns false") @(t : T?) {
        with_job_que() {
            with_lock_box() $(box) {
                with_channel(1) $(ch) {
                    new_job() @() {
                        let got = box |> grab() $(var data : int) {
                            feint("should not reach\n")
                        }
                        ch |> push_clone(IntVal(v = got ? 1 : 0))
                        box |> release
                        ch |> notify_and_release
                    }
                    var result = 1
                    ch |> for_each_clone() $(val : IntVal#) {
                        result = val.v
                    }
                    t |> equal(0, result)
                }
            }
        }
    }
}

[test]
def test_lockbox_fill_grab_cycle(t : T?) {
    t |> run("fill-grab-fill-grab cycle works") @(t : T?) {
        with_job_que() {
            with_lock_box() $(box) {
                with_channel(1) $(ch) {
                    new_job() @() {
                        var buf1 : array<float>
                        buf1 |> push(1.0)
                        buf1 |> push(2.0)
                        // cycle 1
                        box |> fill(buf1)
                        var sum1 = 0.0
                        box |> grab() $(var arr : array<float>) {
                            for (v in arr) {
                                sum1 += v
                            }
                        }
                        // cycle 2
                        var buf2 : array<float>
                        buf2 |> push(10.0)
                        buf2 |> push(20.0)
                        buf2 |> push(30.0)
                        box |> fill(buf2)
                        var sum2 = 0.0
                        box |> grab() $(var arr : array<float>) {
                            for (v in arr) {
                                sum2 += v
                            }
                        }
                        ch |> push_clone(IntVal(v = int(sum1 + sum2)))
                        box |> release
                        ch |> notify_and_release
                    }
                    var total = 0
                    ch |> for_each_clone() $(val : IntVal#) {
                        total = val.v
                    }
                    t |> equal(63, total)
                    t |> equal(true, box.isReady)
                }
            }
        }
    }
}
