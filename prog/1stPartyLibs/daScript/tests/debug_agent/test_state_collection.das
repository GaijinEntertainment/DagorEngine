// Test: state collection via onCollect and onVariable
options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/debugger
require daslib/rtti

var collect_count : int = 0
var variable_count : int = 0

class CollectorAgent : DapiDebugAgent {
    def override onCollect(var ctx : Context; at : LineInfo) : void {
        collect_count++
    }
    def override onVariable(var ctx : Context; category, name : string; info : TypeInfo; data : void?) : void {
        variable_count++
    }
}

var collector_ready : bool = false

def setup_collector(ctx : Context) {
    install_new_debug_agent(new CollectorAgent(), "collector")
    collector_ready = true
}

def ensure_collector() {
    if (!collector_ready) {
        fork_debug_agent_context(@@setup_collector)
    }
}

[export, pinvoke]
def read_collect_count(var result : int?) {
    unsafe {
        *result = collect_count
    }
}

[export, pinvoke]
def read_variable_count(var result : int?) {
    unsafe {
        *result = variable_count
    }
}

[export, pinvoke]
def reset_collector_state() {
    collect_count = 0
    variable_count = 0
}

[test]
def test_on_collect_called(t : T?) {
    t |> run("collect_debug_agent_state triggers onCollect") @(t : T?) {
        ensure_collector()
        unsafe {
            invoke_in_context(get_debug_agent_context("collector"), "reset_collector_state")
        }
        collect_debug_agent_state(this_context(), get_line_info(1))
        var count = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("collector"), "read_collect_count", addr(count))
        }
        t |> equal(count > 0, true)
    }
}

[test]
def test_on_collect_increments(t : T?) {
    t |> run("multiple collect calls increment counter") @(t : T?) {
        ensure_collector()
        unsafe {
            invoke_in_context(get_debug_agent_context("collector"), "reset_collector_state")
        }
        collect_debug_agent_state(this_context(), get_line_info(1))
        collect_debug_agent_state(this_context(), get_line_info(1))
        var count = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("collector"), "read_collect_count", addr(count))
        }
        t |> equal(count >= 2, true)
    }
}

[test]
def test_cleanup(t : T?) {
    t |> run("delete_debug_agent_context removes collector agent") @(t : T?) {
        ensure_collector()
        t |> equal(has_debug_agent_context("collector"), true)
        delete_debug_agent_context("collector")
        t |> equal(has_debug_agent_context("collector"), false)
        collector_ready = false
    }
}
