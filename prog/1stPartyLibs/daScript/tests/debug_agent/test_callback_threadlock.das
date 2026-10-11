options gen2
options no_aot
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/debugger
require daslib/jobque_boost
require daslib/fio

var callback_entered : Atomic32?
var callback_release : Atomic32?
var callback_active : Atomic32?
var callback_overlap : Atomic32?

class CallbackThreadlockAgent : DapiDebugAgent {
    def override onTick() {
        callback_active |> set(1)
        callback_entered |> set(1)
        while ((callback_release |> get) == 0) {
            sleep(10u)
        }
        callback_active |> set(0)
    }
}

[export, pinvoke]
def configure_callback(var entered, release, active, overlap : Atomic32?) {
    callback_entered = entered
    callback_release = release
    callback_active = active
    callback_overlap = overlap
}

[export, pinvoke]
def probe_callback_overlap() {
    if ((callback_active |> get) != 0) {
        callback_overlap |> set(1)
    }
}

def setup_callback_agent(_ctx : Context) {
    install_new_debug_agent(new CallbackThreadlockAgent(), "callback_threadlock_test")
}

[test]
def test_callback_and_pinvoke_are_serialized(t : T?) {
    t |> run("debug-agent callback and pinvoke share the context lock") @(t : T?) {
        fork_debug_agent_context(@@setup_callback_agent)
        var entered = atomic32_create()
        var release = atomic32_create()
        var active = atomic32_create()
        var overlap = atomic32_create()
        var tick_done = atomic32_create()
        let agent_context = unsafe(addr(get_debug_agent_context("callback_threadlock_test")))
        unsafe(invoke_in_context(
            *agent_context,
            "configure_callback",
            entered,
            release,
            active,
            overlap))
        with_job_que() {
            new_thread() @() {
                tick_debug_agent("callback_threadlock_test")
                tick_done |> set(1)
            }
            while ((entered |> get) == 0) {
                sleep(10u)
            }
            let refused = !unsafe(try_invoke_in_context(*agent_context, "probe_callback_overlap"))
            t |> success(refused, "a pinvoke is refused while the callback holds the context lock")
            t |> equal(overlap |> get, 0)
            release |> set(1)
            while ((tick_done |> get) == 0) {
                sleep(10u)
            }
            let ran = unsafe(try_invoke_in_context(*agent_context, "probe_callback_overlap"))
            t |> success(ran, "a pinvoke runs once the callback returned")
        }
        t |> equal(overlap |> get, 0)
        delete_debug_agent_context("callback_threadlock_test")
        unsafe {
            atomic32_remove(entered)
            atomic32_remove(release)
            atomic32_remove(active)
            atomic32_remove(overlap)
            atomic32_remove(tick_done)
        }
    }
}
