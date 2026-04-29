// Test: invoke_debug_agent_method with various argument counts
// This tests the fix for the off-by-one bug in argument count validation
options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/debugger

var method_value : int = 0

class MethodAgent : DapiDebugAgent {
    base_value : int = 100
    def set_zero() {
        method_value = 0
    }
    def set_from_arg(v : int) {
        method_value = v
    }
    def set_sum(a : int; b : int) {
        method_value = a + b
    }
    def set_base_value() {
        method_value = self.base_value
    }
}

var method_agent_ready : bool = false

def setup_method_agent(ctx : Context) {
    install_new_debug_agent(new MethodAgent(), "method_test")
    method_agent_ready = true
}

def ensure_method_agent() {
    if (!method_agent_ready) {
        fork_debug_agent_context(@@setup_method_agent)
    }
}

[export, pinvoke]
def read_method_value(var result : int?) {
    unsafe {
        *result = method_value
    }
}

[test]
def test_method_no_extra_args(t : T?) {
    t |> run("invoke_debug_agent_method with 0 user args") @(t : T?) {
        ensure_method_agent()
        // First set a known value via invoke_in_context so we can detect change
        unsafe {
            invoke_debug_agent_method("method_test", "set_zero")
        }
        var result = -1
        unsafe {
            invoke_in_context(get_debug_agent_context("method_test"), "read_method_value", addr(result))
        }
        t |> equal(result, 0)
    }
}

[test]
def test_method_one_arg(t : T?) {
    t |> run("invoke_debug_agent_method with 1 user arg") @(t : T?) {
        ensure_method_agent()
        unsafe {
            invoke_debug_agent_method("method_test", "set_from_arg", 77)
        }
        var result = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("method_test"), "read_method_value", addr(result))
        }
        t |> equal(result, 77)
    }
}

[test]
def test_method_two_args(t : T?) {
    t |> run("invoke_debug_agent_method with 2 user args") @(t : T?) {
        ensure_method_agent()
        unsafe {
            invoke_debug_agent_method("method_test", "set_sum", 30, 12)
        }
        var result = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("method_test"), "read_method_value", addr(result))
        }
        t |> equal(result, 42)
    }
}

[test]
def test_method_reads_agent_field(t : T?) {
    t |> run("method can read agent fields via self") @(t : T?) {
        ensure_method_agent()
        unsafe {
            invoke_debug_agent_method("method_test", "set_base_value")
        }
        var result = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("method_test"), "read_method_value", addr(result))
        }
        t |> equal(result, 100)
    }
}

[test]
def test_cleanup(t : T?) {
    t |> run("delete_debug_agent_context removes method_test agent") @(t : T?) {
        ensure_method_agent()
        t |> equal(has_debug_agent_context("method_test"), true)
        delete_debug_agent_context("method_test")
        t |> equal(has_debug_agent_context("method_test"), false)
        method_agent_ready = false
    }
}
