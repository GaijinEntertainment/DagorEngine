options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
// strings cross the channel between thread contexts
options multiple_contexts

require dastest/testing_boost public
require daslib/debugger
require daslib/jobque_boost

class ThreadLocalAgent : DapiDebugAgent {
    def store_value(var result : int?) {
        *result = 42
    }
}

[export, pinvoke]
def agent_store_value(agent : void?; var result : int?) {
    *result = 7
}

def setup_thread_local_agent(ctx : Context) {
    install_new_thread_local_debug_agent(new ThreadLocalAgent())
}

struct EmptyCategoryResults {
    no_agent_method : string
    no_agent_function : string
    missing_method : string
    missing_function : string
    method_value : int
    function_value : int
}

// "" compiles to a null category pointer
def collect_empty_category_results() : EmptyCategoryResults {
    var res : EmptyCategoryResults
    var value = 0
    res.no_agent_method = panic_text() {
        unsafe {
            invoke_debug_agent_method("", "store_value", addr(value))
        }
    }
    res.no_agent_function = panic_text() {
        unsafe {
            invoke_debug_agent_function("", "agent_store_value", addr(value))
        }
    }
    fork_debug_agent_context(@@setup_thread_local_agent)
    res.missing_method = panic_text() {
        unsafe(invoke_debug_agent_method("", "no_such_method"))
    }
    res.missing_function = panic_text() {
        unsafe(invoke_debug_agent_function("", "no_such_function"))
    }
    unsafe {
        invoke_debug_agent_method("", "store_value", addr(value))
    }
    res.method_value = value
    unsafe {
        invoke_debug_agent_function("", "agent_store_value", addr(value))
    }
    res.function_value = value
    return res
}

[test]
def test_empty_category(t : T?) {
    var got = false
    with_job_que() {
        with_channel(1) $(ch) {
            // new_thread exit removes the thread-local agent
            new_thread() @() {
                ch |> push_clone(collect_empty_category_results())
                ch |> notify_and_release
            }
            ch |> for_each_clone() $(res : EmptyCategoryResults#) {
                got = true
                t |> equal(res.no_agent_method, "no thread-local debug agent available")
                t |> equal(res.no_agent_function, "no thread-local debug agent available")
                t |> equal(res.missing_method, "debug agent '' is a CPP-only agent")
                t |> equal(res.missing_function, "function 'no_such_function' not found in debug agent ''")
                t |> equal(res.method_value, 42)
                t |> equal(res.function_value, 7)
            }
        }
    }
    t |> success(got, "thread must report results")
}
