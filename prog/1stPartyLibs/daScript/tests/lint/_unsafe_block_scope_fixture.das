options gen2
options auto_inline_functions = false

[unsafe_operation]
def needs_unsafe(a : int) : int {
    print("u{a}\n")
    return a
}

[export]
def fixture_function() : int {
    var rc = 0
    unsafe {
        rc = needs_unsafe(5)
    }
    return rc
}
