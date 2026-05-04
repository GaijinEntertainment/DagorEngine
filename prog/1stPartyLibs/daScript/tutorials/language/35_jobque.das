// Tutorial 35: Job Queue (jobque)
//
// This tutorial covers:
//   - Initializing the job queue with with_job_que
//   - Spawning background jobs with new_job
//   - Channel: push_clone, for_each_clone, notify_and_release
//   - JobStatus: with_job_status, notify_and_release, join
//   - Wait groups: with_wait_group and done
//   - LockBox: set and get
//   - parallel_for for index-range parallelism
//   - Spawning dedicated threads with new_thread
//
// requires: daslib/jobque_boost
// Run: daslang.exe tutorials/language/35_jobque.das

options gen2

require daslib/jobque_boost

// === What is jobque? ===

// The jobque module provides a work-stealing job queue for multi-threaded
// execution.  Jobs run on a thread pool, communicate through typed channels,
// and synchronize via job statuses and wait groups.
//
// The main entry point is `with_job_que`, which initializes the thread pool.
// Inside that scope you can spawn jobs with `new_job`, create channels with
// `with_channel`, and use higher-level helpers like `parallel_for`.
//
// Channel and lock-box data must be a struct or handled type — primitives
// like int or string cannot be heap-allocated.  Wrap them in a struct.

// Simple wrapper structs for sending primitives through channels and lock boxes
struct IntVal {
    v : int
}

struct StringVal {
    s : string
}

struct WorkResult {
    index : int
    value : int
}

// === Starting the job queue ===

def starting_the_job_queue() {
    print("\n=== Starting the job queue ===\n")

    // with_job_que initializes the thread pool and runs the block.
    // All job-queue operations must happen inside this scope.
    with_job_que() {
        print("Job queue is active\n")
    }
    // output: Job queue is active
}

// === Spawning jobs ===

def spawning_jobs() {
    print("\n=== Spawning jobs ===\n")

    // new_job spawns work on the thread pool.
    // Each job runs in a cloned context — it does not share memory
    // with the calling thread.  Use channels or lock boxes to communicate.
    with_job_que() {
        with_channel(1) $(ch) {
            new_job() @() {
                ch |> push_clone(IntVal(v = 42))
                ch |> notify_and_release
            }
            ch |> for_each_clone() $(val : IntVal#) {
                print("Received from job: {val.v}\n")
            }
        }
    }
    // output: Received from job: 42
}

// === Channels ===

def channels_demo() {
    print("\n=== Channels ===\n")

    // A Channel is a thread-safe FIFO queue for passing typed data between jobs.
    // Create one with `with_channel(expected_count)`, where the count is the
    // number of notify_and_release calls expected before the channel closes.
    // Use push_clone to send data and for_each_clone to receive.

    with_job_que() {
        with_channel(2) $(ch) {
            // Spawn two jobs, each pushing a value
            new_job() @() {
                ch |> push_clone(StringVal(s = "hello"))
                ch |> notify_and_release
            }
            new_job() @() {
                ch |> push_clone(StringVal(s = "world"))
                ch |> notify_and_release
            }
            // Collect all values — for_each_clone blocks until the channel is
            // depleted (both jobs have called notify_and_release)
            var messages : array<string>
            ch |> for_each_clone() $(msg : StringVal#) {
                messages |> push(clone_string(msg.s))
            }
            sort(messages)
            for (m in messages) {
                print("  {m}\n")
            }
        }
    }
    // output:
    //   hello
    //   world
}

// === Structs through channels ===

def structs_through_channels() {
    print("\n=== Structs through channels ===\n")

    // Channels work with any struct type — here we send WorkResult values.
    with_job_que() {
        with_channel(1) $(ch) {
            new_job() @() {
                for (i in range(3)) {
                    ch |> push_clone(WorkResult(index = i, value = i * i))
                }
                ch |> notify_and_release
            }
            ch |> for_each_clone() $(r : WorkResult#) {
                print("  [{r.index}] = {r.value}\n")
            }
        }
    }
    // output:
    //   [0] = 0
    //   [1] = 1
    //   [2] = 4
}

// === Multiple producers ===

def multiple_producers() {
    print("\n=== Multiple producers ===\n")

    // When multiple jobs push to the same channel, set the channel count
    // to the number of producers.  for_each_clone blocks until all have
    // called notify_and_release.
    with_job_que() {
        let num_producers = 3
        with_channel(num_producers) $(ch) {
            for (p in range(num_producers)) {
                new_job() @() {
                    ch |> push_clone(IntVal(v = p * 100))
                    ch |> notify_and_release
                }
            }
            var results : array<int>
            ch |> for_each_clone() $(val : IntVal#) {
                results |> push(val.v)
            }
            sort(results)
            print("producers: {results}\n")
        }
    }
    // output: producers: [0, 100, 200]
}

// === JobStatus and with_job_status ===

def job_status_demo() {
    print("\n=== JobStatus and with_job_status ===\n")

    // A JobStatus tracks completion of asynchronous work.
    // `with_job_status(count)` creates a status expecting `count` notifications.
    // Each call to `notify_and_release` decrements the counter.
    // `join` blocks until all notifications arrive.

    with_job_que() {
        with_job_status(3) $(status) {
            for (i in range(3)) {
                new_job() @() {
                    // each job completes
                    status |> notify_and_release
                }
            }
            status |> join  // wait for all 3 notifications
        }
        print("All 3 jobs finished\n")
    }
    // output: All 3 jobs finished
}

// === Wait groups: with_wait_group and done ===

def wait_group_demo() {
    print("\n=== Wait groups ===\n")

    // with_wait_group is a convenience wrapper around with_job_status + join.
    // `done(wg)` is an alias for `notify_and_release`.

    with_job_que() {
        with_wait_group(3) $(wg) {
            for (i in range(3)) {
                new_job() @() {
                    print("  job {i} done\n")
                    wg |> done
                }
            }
        }
        // At this point all 3 jobs have finished
        print("all jobs complete\n")
    }
    // output: all jobs complete
}

// === LockBox — shared mutable state ===

struct BoxCounter {
    value : int
}

def lockbox_demo() {
    print("\n=== LockBox — shared mutable state ===\n")

    // A LockBox holds a single value protected by a mutex.
    // - set: store a value
    // - get: read the value (block receives the stored value)
    // - update: atomically read-modify-write
    // - clear: remove the value
    //
    // Data must be a struct — primitives cannot be heap-allocated.
    // Note: set/get must run inside a job context (new_job or new_thread)
    // because the underlying API requires a shared_ptr-managed context.

    with_job_que() {
        with_lock_box() $(box) {
            with_channel(1) $(ch) {
                new_job() @() {
                    // Set and read inside a job context
                    box |> set(BoxCounter(value = 42))
                    box |> get() $(c : BoxCounter#) {
                        ch |> push_clone(IntVal(v = c.value))
                    }
                    box |> release
                    ch |> notify_and_release
                }
                ch |> for_each_clone() $(val : IntVal#) {
                    print("lockbox value: {val.v}\n")
                }
            }
        }
    }
    // output: lockbox value: 42
}

// === parallel_for ===

def parallel_for_demo() {
    print("\n=== parallel_for ===\n")

    // parallel_for splits a range [begin..end) into chunks and runs them
    // on the job queue.  The block body is automatically wrapped in new_job
    // by a macro — you just write sequential code inside the block.
    //
    // The block receives (job_begin, job_end, wg):
    //   job_begin, job_end — the index sub-range for this chunk
    //   wg — a JobStatus to call done(wg) when finished

    with_job_que() {
        let num_jobs = 3
        with_channel(num_jobs) $(ch) {
            parallel_for(0, 10, num_jobs) $(job_begin, job_end, wg) {
                for (i in range(job_begin, job_end)) {
                    ch |> push_clone(IntVal(v = i * i))
                }
                ch |> notify_and_release
                wg |> done
            }
            var results : array<int>
            ch |> for_each_clone() $(val : IntVal#) {
                results |> push(val.v)
            }
            sort(results)
            print("squares: {results}\n")
        }
    }
    // output: squares: [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]
}

// === new_thread ===

def new_thread_demo() {
    print("\n=== new_thread ===\n")

    // new_thread creates a dedicated OS thread (not from the thread pool).
    // Use it for long-lived work that should not block the job queue.

    with_job_que() {
        with_channel(1) $(ch) {
            new_thread() @() {
                ch |> push_clone(StringVal(s = "from thread"))
                ch |> notify_and_release
            }
            ch |> for_each_clone() $(msg : StringVal#) {
                print("{msg.s}\n")
            }
        }
    }
    // output: from thread
}

[export]
def main() {
    starting_the_job_queue()
    spawning_jobs()
    channels_demo()
    structs_through_channels()
    multiple_producers()
    job_status_demo()
    wait_group_demo()
    lockbox_demo()
    parallel_for_demo()
    new_thread_demo()
    return
}
