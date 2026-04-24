// Test: invoke_in_context with [pinvoke] functions
// Verifies cross-context function invocation with 0, 1, 2 args
options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/debugger

// Module-level variables — agent context gets its own copy
var invoke_counter : int = 0

[export, pinvoke]
def agent_inc() {
    invoke_counter++
}

[export, pinvoke]
def agent_add(amount : int) {
    invoke_counter += amount
}

[export, pinvoke]
def agent_set_sum(a : int; b : int) {
    invoke_counter = a + b
}

[export, pinvoke]
def agent_reset() {
    invoke_counter = 0
}

[export, pinvoke]
def agent_read_counter(var result : int?) {
    unsafe {
        *result = invoke_counter
    }
}

def setup_invoke_agent(ctx : Context) {
    install_new_debug_agent(new DapiDebugAgent(), "invoke_test")
}

var invoke_agent_ready : bool = false

def ensure_invoke_agent() {
    if (!invoke_agent_ready) {
        fork_debug_agent_context(@@setup_invoke_agent)
        invoke_agent_ready = true
    }
}

[test]
def test_invoke_no_args(t : T?) {
    t |> run("invoke_in_context with no extra args") @(t : T?) {
        ensure_invoke_agent()
        // reset agent state
        unsafe {
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_reset")
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_inc")
        }
        var result = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_read_counter", addr(result))
        }
        t |> equal(result, 1)
    }
}

[test]
def test_invoke_one_arg(t : T?) {
    t |> run("invoke_in_context with 1 arg") @(t : T?) {
        ensure_invoke_agent()
        unsafe {
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_reset")
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_add", 42)
        }
        var result = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_read_counter", addr(result))
        }
        t |> equal(result, 42)
    }
}

[test]
def test_invoke_two_args(t : T?) {
    t |> run("invoke_in_context with 2 args") @(t : T?) {
        ensure_invoke_agent()
        unsafe {
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_reset")
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_set_sum", 10, 32)
        }
        var result = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_read_counter", addr(result))
        }
        t |> equal(result, 42)
    }
}

[test]
def test_invoke_multiple_calls(t : T?) {
    t |> run("multiple invoke_in_context calls accumulate") @(t : T?) {
        ensure_invoke_agent()
        unsafe {
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_reset")
        }
        for (i in range(10)) {
            unsafe {
                invoke_in_context(get_debug_agent_context("invoke_test"), "agent_inc")
            }
        }
        var result = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_read_counter", addr(result))
        }
        t |> equal(result, 10)
    }
}

[test]
def test_local_unchanged(t : T?) {
    t |> run("local copy of global is unchanged by agent") @(t : T?) {
        ensure_invoke_agent()
        invoke_counter = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("invoke_test"), "agent_add", 100)
        }
        // local copy should still be 0
        t |> equal(invoke_counter, 0)
    }
}

[test]
def test_cleanup(t : T?) {
    t |> run("delete_debug_agent_context removes invoke_test agent") @(t : T?) {
        ensure_invoke_agent()
        t |> equal(has_debug_agent_context("invoke_test"), true)
        delete_debug_agent_context("invoke_test")
        t |> equal(has_debug_agent_context("invoke_test"), false)
        invoke_agent_ready = false
    }
}
