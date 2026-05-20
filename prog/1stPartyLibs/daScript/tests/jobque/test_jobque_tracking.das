options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

[test]
def test_tracking_clean_run(t : T?) {
    t |> run("channel create+remove leaves no leak") @(t : T?) {
        unsafe {
            var ch = channel_create()
            t |> success(ch != null)
            ch |> channel_remove
            t |> success(ch == null)
        }
    }
    t |> run("lockbox create+remove leaves no leak") @(t : T?) {
        unsafe {
            var box = lock_box_create()
            t |> success(box != null)
            box |> lock_box_remove
            t |> success(box == null)
        }
    }
    t |> run("job_status create+remove leaves no leak") @(t : T?) {
        unsafe {
            var js = job_status_create()
            t |> success(js != null)
            js |> job_status_remove
            t |> success(js == null)
        }
    }
    t |> run("with_channel scoped cleanup") @(t : T?) {
        with_channel() $(ch) {
            t |> success(ch != null)
        }
    }
    t |> run("with_lock_box scoped cleanup") @(t : T?) {
        with_lock_box() $(box) {
            t |> success(box != null)
        }
    }
    t |> run("with_job_status scoped cleanup") @(t : T?) {
        with_job_status(1) $(js) {
            t |> success(js != null)
            js |> notify
        }
    }
}

[test]
def test_tracking_atomics_standalone(t : T?) {
    t |> run("atomic32 create+remove works without JobStatus") @(t : T?) {
        unsafe {
            var a = atomic32_create()
            a |> set(42)
            t |> equal(42, a |> get)
            a |> atomic32_remove
            t |> success(a == null)
        }
    }
    t |> run("atomic64 create+remove works without JobStatus") @(t : T?) {
        unsafe {
            var a = atomic64_create()
            a |> set(123l)
            t |> equal(123l, a |> get)
            a |> atomic64_remove
            t |> success(a == null)
        }
    }
    t |> run("with_atomic32 scoped") @(t : T?) {
        with_atomic32() $(a) {
            a |> set(7)
            t |> equal(7, a |> get)
        }
    }
    t |> run("with_atomic64 scoped") @(t : T?) {
        with_atomic64() $(a) {
            a |> set(99l)
            t |> equal(99l, a |> get)
        }
    }
}
