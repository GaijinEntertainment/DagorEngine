// Test: options threadlock_context creates a context mutex
// Verifies the fix for threadlock_context option propagation
options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/debugger

var tl_counter : int = 0

[export, pinvoke]
def tl_increment() {
    tl_counter++
}

[export, pinvoke]
def tl_read(var result : int?) {
    unsafe {
        *result = tl_counter
    }
}

[export, pinvoke]
def tl_reset() {
    tl_counter = 0
}

def setup_threadlock_agent(ctx : Context) {
    install_new_debug_agent(new DapiDebugAgent(), "threadlock_test")
}

var tl_agent_ready : bool = false

def ensure_tl_agent() {
    if (!tl_agent_ready) {
        fork_debug_agent_context(@@setup_threadlock_agent)
        tl_agent_ready = true
    }
}

[test]
def test_threadlock_invoke(t : T?) {
    t |> run("invoke_in_context succeeds with threadlock") @(t : T?) {
        ensure_tl_agent()
        unsafe {
            invoke_in_context(get_debug_agent_context("threadlock_test"), "tl_reset")
            invoke_in_context(get_debug_agent_context("threadlock_test"), "tl_increment")
        }
        var result = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("threadlock_test"), "tl_read", addr(result))
        }
        t |> equal(result, 1)
    }
}

[test]
def test_threadlock_multiple(t : T?) {
    t |> run("multiple invoke_in_context calls under threadlock") @(t : T?) {
        ensure_tl_agent()
        unsafe {
            invoke_in_context(get_debug_agent_context("threadlock_test"), "tl_reset")
        }
        for (i in range(5)) {
            unsafe {
                invoke_in_context(get_debug_agent_context("threadlock_test"), "tl_increment")
            }
        }
        var result = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("threadlock_test"), "tl_read", addr(result))
        }
        t |> equal(result, 5)
    }
}

[test]
def test_cleanup(t : T?) {
    t |> run("delete_debug_agent_context removes threadlock_test agent") @(t : T?) {
        ensure_tl_agent()
        t |> equal(has_debug_agent_context("threadlock_test"), true)
        delete_debug_agent_context("threadlock_test")
        t |> equal(has_debug_agent_context("threadlock_test"), false)
        tl_agent_ready = false
    }
}
