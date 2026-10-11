options gen2
require dastest/testing_boost
require jobque
require daslib/jobque_boost
require math

struct PinResult {
    pinned_on : int
    seen : int
}

[test]
def test_current_thread_affinity(t : T?) {
    t |> run("processor index is -1 or a valid index") @@(t : T?) {
        let cpu = get_current_processor()
        t |> success(cpu >= -1, "index {cpu}")
    }
    t |> run("a negative index is ignored") @@(t : T?) {
        set_current_thread_affinity(-1, true)
        t |> success(get_current_processor() >= -1, "still running after a negative pin")
    }
    t |> run("a hard pin moves a dedicated thread to another processor") @@(t : T?) {
        if (get_current_processor() < 0) {
            t |> skip("{get_platform_name()}: no processor index to re-read")
            return
        }
        with_job_que() {
            let processors = min(get_total_hw_threads(), 64)
            if (processors < 2) {
                t |> skip("one processor: nowhere to move")
                return
            }
            with_channel(1) $(ch) {
                new_thread() @() {
                    let target = (get_current_processor() + 1) % processors
                    set_current_thread_affinity(target, true)
                    var acc = 0
                    for (i in range(100000)) {
                        acc += i & 7
                    }
                    feint("{acc}")
                    let seen = get_current_processor()
                    set_current_thread_affinity(target, false)
                    ch |> push_clone(PinResult(pinned_on = target, seen = seen))
                    ch |> notify_and_release
                }
                var pinned_on = -1
                var seen = -2
                ch |> for_each_clone() $(r : PinResult#) {
                    pinned_on = r.pinned_on
                    seen = r.seen
                }
                t |> equal(seen, pinned_on, "on {pinned_on} after a pinned loop")
            }
        }
    }
}
