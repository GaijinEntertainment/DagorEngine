require rtti
require debugapi
require jobque

// debug server

[export]
def debug_agent ( ctx : Context )
    print("from debugger\n")
    install_new_debug_agent(new DapiDebugAgent(), "sample") // note - no install = no category

[export, pinvoke]
def debug_agent_fn ( ctx:Context; fun:function )
    print("debug agent call {fun} from context {ctx}\n")
    unsafe
        ctx |> invoke_in_context(fun, "now")

// client

var
    wait_for_it = true

[export, pinvoke]
def stop_waiting(msg:string#)
    print("we stop waiting {msg}\n")
    wait_for_it = false

def has_agent
    return has_debug_agent_context("sample")

[export]
def main
    print("forking\n")
    print("has agent? {has_agent()}\n")
    fork_debug_agent_context(@@debug_agent)
    print("forked\n")
    print("has agent? {has_agent()}\n")
    unsafe
        get_debug_agent_context("sample") |> invoke_in_context("debug_agent_fn", this_context(), @@stop_waiting)
    while wait_for_it
        print("waiting\n")
    while true
        pass
