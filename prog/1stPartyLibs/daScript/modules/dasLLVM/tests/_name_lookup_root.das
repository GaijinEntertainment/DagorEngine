options gen2
require debugapi

var g_counter = 7

[export]
def foo(x : int) : int {
    return x + 1
}

[export]
def foo(x : float) : float {
    return x * 2.0
}

[export, sideeffects]
def bump : int {
    g_counter++
    return g_counter
}

[export]
def main {
    print("HAS_FOO {has_function(this_context(), "foo")}\n")
    print("HAS_BUMP {has_function(this_context(), "bump")}\n")
    print("HAS_NOPE {has_function(this_context(), "nope")}\n")
    print("BUMP {invoke("bump")}\n")
    unsafe {
        var counter = reinterpret<int?>(get_context_global_variable(addr(this_context()), "g_counter"))
        print("G_COUNTER {counter != null ? *counter : -1}\n")
        var missing = get_context_global_variable(addr(this_context()), "nope")
        print("G_NOPE {missing == null}\n")
    }
}
