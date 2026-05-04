options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

struct IntVal {
    v : int
}

struct IndexedResult {
    idx : int
    val : int
}

// ===== parallel_for tests =====

[test]
def test_parallel_for_basic(t : T?) {
    t |> run("distributes range across jobs and waits") @(t : T?) {
        with_job_que() {
            let num_jobs = 3
            with_channel(num_jobs) $(ch) {
                parallel_for(0, 10, num_jobs) $(job_begin, job_end, wg) {
                    for (i in range(job_begin, job_end)) {
                        ch |> push_clone(IntVal(v = i))
                    }
                    ch |> notify_and_release
                    wg |> notify_and_release
                }
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                sort(results)
                t |> equal(10, length(results))
                for (i in range(10)) {
                    t |> equal(i, results[i])
                }
            }
        }
    }
}

[test]
def test_parallel_for_single_job(t : T?) {
    t |> run("1 job gives full range") @(t : T?) {
        with_job_que() {
            with_channel(1) $(ch) {
                parallel_for(0, 5, 1) $(job_begin, job_end, wg) {
                    for (i in range(job_begin, job_end)) {
                        ch |> push_clone(IntVal(v = i))
                    }
                    ch |> notify_and_release
                    wg |> notify_and_release
                }
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                sort(results)
                t |> equal(5, length(results))
                t |> equal(0, results[0])
                t |> equal(4, results[4])
            }
        }
    }
}

[test]
def test_parallel_for_more_jobs_than_items(t : T?) {
    t |> run("clamps jobs to range size") @(t : T?) {
        with_job_que() {
            // 100 requested but only 3 items, so actual_jobs = 3
            with_channel(3) $(ch) {
                parallel_for(0, 3, 100) $(job_begin, job_end, wg) {
                    for (i in range(job_begin, job_end)) {
                        ch |> push_clone(IntVal(v = i))
                    }
                    ch |> notify_and_release
                    wg |> notify_and_release
                }
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                sort(results)
                t |> equal(3, length(results))
                t |> equal(0, results[0])
                t |> equal(2, results[2])
            }
        }
    }
}

[test]
def test_parallel_for_empty_range(t : T?) {
    t |> run("empty range does nothing") @(t : T?) {
        with_job_que() {
            var called = false
            parallel_for(5, 5, 4) $(job_begin, job_end, wg) {
                called = true
            }
            t |> equal(false, called)
        }
    }
}

[test]
def test_parallel_for_negative_range(t : T?) {
    t |> run("inverted range does nothing") @(t : T?) {
        with_job_que() {
            var called = false
            parallel_for(10, 5, 4) $(job_begin, job_end, wg) {
                called = true
            }
            t |> equal(false, called)
        }
    }
}

[test]
def test_parallel_for_offset_range(t : T?) {
    t |> run("non-zero start range") @(t : T?) {
        with_job_que() {
            let num_jobs = 2
            with_channel(num_jobs) $(ch) {
                parallel_for(100, 105, num_jobs) $(job_begin, job_end, wg) {
                    for (i in range(job_begin, job_end)) {
                        ch |> push_clone(IntVal(v = i))
                    }
                    ch |> notify_and_release
                    wg |> notify_and_release
                }
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                sort(results)
                t |> equal(5, length(results))
                t |> equal(100, results[0])
                t |> equal(104, results[4])
            }
        }
    }
}

[test]
def test_parallel_for_large(t : T?) {
    t |> run("100 items across 8 jobs") @(t : T?) {
        with_job_que() {
            let num_jobs = 8
            with_channel(num_jobs) $(ch) {
                parallel_for(0, 100, num_jobs) $(job_begin, job_end, wg) {
                    for (i in range(job_begin, job_end)) {
                        ch |> push_clone(IntVal(v = i + 1000))
                    }
                    ch |> notify_and_release
                    wg |> notify_and_release
                }
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                sort(results)
                t |> equal(100, length(results))
                t |> equal(1000, results[0])
                t |> equal(1099, results[99])
            }
        }
    }
}

[test]
def test_parallel_for_done(t : T?) {
    t |> run("works with done alias") @(t : T?) {
        with_job_que() {
            let num_jobs = 2
            with_channel(num_jobs) $(ch) {
                parallel_for(0, 6, num_jobs) $(job_begin, job_end, wg) {
                    for (i in range(job_begin, job_end)) {
                        ch |> push_clone(IntVal(v = i))
                    }
                    ch |> notify_and_release
                    wg |> done
                }
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                sort(results)
                t |> equal(6, length(results))
                for (i in range(6)) {
                    t |> equal(i, results[i])
                }
            }
        }
    }
}

// ===== parallel_for_each tests =====

[test]
def test_parallel_for_each_basic(t : T?) {
    t |> run("partitions array indices into chunks") @(t : T?) {
        with_job_que() {
            var input : array<int>
            for (i in range(8)) {
                input |> push(i)
            }
            let num_jobs = 3
            with_channel(num_jobs) $(ch) {
                parallel_for_each(input, num_jobs) $(job_begin, job_end, wg) {
                    for (i in range(job_begin, job_end)) {
                        ch |> push_clone(IntVal(v = i * 10))
                    }
                    ch |> notify_and_release
                    wg |> notify_and_release
                }
                var results : array<int>
                ch |> for_each_clone() $(val : IntVal#) {
                    results |> push(val.v)
                }
                sort(results)
                t |> equal(8, length(results))
                for (i in range(8)) {
                    t |> equal(i * 10, results[i])
                }
            }
        }
    }
}

[test]
def test_parallel_for_each_empty(t : T?) {
    t |> run("empty array does nothing") @(t : T?) {
        with_job_que() {
            var input : array<int>
            var called = false
            parallel_for_each(input, 4) $(job_begin, job_end, wg) {
                called = true
            }
            t |> equal(false, called)
        }
    }
}

// ===== parallel_map tests =====

[test]
def test_parallel_map_basic(t : T?) {
    t |> run("collects indexed results via channel") @(t : T?) {
        with_job_que() {
            var input : array<int>
            for (i in range(10)) {
                input |> push(i)
            }
            let num_jobs = 3
            with_channel(num_jobs) $(results_ch) {
                parallel_map(input, num_jobs, results_ch) $(job_begin, job_end, ch, wg) {
                    for (i in range(job_begin, job_end)) {
                        ch |> push_clone(IndexedResult(idx = i, val = i * i))
                    }
                    ch |> notify_and_release
                    wg |> notify_and_release
                }
                var results : array<int>
                results |> resize(10)
                results_ch |> for_each_clone() $(r : IndexedResult#) {
                    results[r.idx] = r.val
                }
                for (i in range(10)) {
                    t |> equal(i * i, results[i])
                }
            }
        }
    }
}

[test]
def test_parallel_map_empty(t : T?) {
    t |> run("empty array does nothing") @(t : T?) {
        with_job_que() {
            var input : array<int>
            with_channel(0) $(ch) {
                var called = false
                parallel_map(input, 4, ch) $(job_begin, job_end, results_ch, wg) {
                    called = true
                }
                t |> equal(false, called)
            }
        }
    }
}

[test]
def test_parallel_map_order(t : T?) {
    t |> run("results reordered by index") @(t : T?) {
        with_job_que() {
            var input : array<int>
            for (i in range(5)) {
                input |> push(i)
            }
            let num_jobs = 3
            with_channel(num_jobs) $(results_ch) {
                parallel_map(input, num_jobs, results_ch) $(job_begin, job_end, ch, wg) {
                    for (i in range(job_begin, job_end)) {
                        ch |> push_clone(IndexedResult(idx = i, val = (i + 1) * 100))
                    }
                    ch |> notify_and_release
                    wg |> notify_and_release
                }
                var results : array<int>
                results |> resize(5)
                results_ch |> for_each_clone() $(r : IndexedResult#) {
                    results[r.idx] = r.val
                }
                t |> equal(100, results[0])
                t |> equal(200, results[1])
                t |> equal(300, results[2])
                t |> equal(400, results[3])
                t |> equal(500, results[4])
            }
        }
    }
}
