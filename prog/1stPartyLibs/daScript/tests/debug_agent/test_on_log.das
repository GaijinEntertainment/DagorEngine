// Test: to_log and print trigger onLog on debug agents
// Verifies the fix for Context::to_out routing through debug agents
options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/debugger

var log_count : int = 0
var last_log_level : int = -1

class LogAgent : DapiDebugAgent {
    def override onLog(context : Context?; at : LineInfo const?; level : int; text : string#) : bool {
        log_count++
        last_log_level = level
        return false  // don't suppress output
    }
}

var log_agent_ready : bool = false

def setup_log_agent(ctx : Context) {
    install_new_debug_agent(new LogAgent(), "log_test")
    log_agent_ready = true
}

def ensure_log_agent() {
    if (!log_agent_ready) {
        fork_debug_agent_context(@@setup_log_agent)
    }
}

[export, pinvoke]
def read_log_count(var result : int?) {
    unsafe {
        *result = log_count
    }
}

[export, pinvoke]
def read_last_level(var result : int?) {
    unsafe {
        *result = last_log_level
    }
}

[export, pinvoke]
def reset_log_state() {
    log_count = 0
    last_log_level = -1
}

[test]
def test_to_log_triggers_on_log(t : T?) {
    t |> run("to_log routes through onLog") @(t : T?) {
        ensure_log_agent()
        unsafe {
            invoke_in_context(get_debug_agent_context("log_test"), "reset_log_state")
        }
        to_log(LOG_INFO, "test message\n")
        var count = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("log_test"), "read_log_count", addr(count))
        }
        t |> equal(count > 0, true)
    }
}

[test]
def test_print_triggers_on_log(t : T?) {
    t |> run("print routes through onLog") @(t : T?) {
        ensure_log_agent()
        unsafe {
            invoke_in_context(get_debug_agent_context("log_test"), "reset_log_state")
        }
        // this print is intentional. do not feint
        print("test print message\n")
        var count = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("log_test"), "read_log_count", addr(count))
        }
        t |> equal(count > 0, true)
    }
}

[test]
def test_on_log_receives_level(t : T?) {
    t |> run("onLog receives the log level") @(t : T?) {
        ensure_log_agent()
        unsafe {
            invoke_in_context(get_debug_agent_context("log_test"), "reset_log_state")
        }
        to_log(LOG_WARNING, "warning test\n")
        var level = -1
        unsafe {
            invoke_in_context(get_debug_agent_context("log_test"), "read_last_level", addr(level))
        }
        t |> equal(level, int(LOG_WARNING))
    }
}

[test]
def test_cleanup(t : T?) {
    t |> run("delete_debug_agent_context removes log_test agent") @(t : T?) {
        ensure_log_agent()
        t |> equal(has_debug_agent_context("log_test"), true)
        delete_debug_agent_context("log_test")
        t |> equal(has_debug_agent_context("log_test"), false)
        log_agent_ready = false
    }
}
