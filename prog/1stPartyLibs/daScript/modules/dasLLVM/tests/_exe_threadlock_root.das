options gen2
options threadlock_context = true

require daslib/debugger

// A standalone exe gets its context mutex from `options threadlock_context` the way the
// interpreter does: invoke_in_context on the exe's own context needs the mutex, and a program
// with no [pinvoke] function used to build an exe with none.

[export]
def ping {
    print("THREADLOCK_PING\n")
}

[export]
def main {
    unsafe {
        var ctx & = this_context()
        ctx |> invoke_in_context("ping")
    }
    print("THREADLOCK_DONE\n")
}
