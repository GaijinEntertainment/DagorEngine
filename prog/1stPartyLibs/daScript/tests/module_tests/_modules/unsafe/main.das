// unsafe/main.das — Entry point for the unsafe module permission test
//
// Tests that module_allowed_unsafe() from the project file correctly enables
// the `implicit` parameter qualifier in something_unsafe.das. The function
// implicit_feint() takes a `string implicit` parameter — this is only allowed
// when the module's project file grants unsafe permission.
//
// Also includes implicit_feint_failed() with the `implicit` keyword commented
// out — if uncommented, it would fail because this is the root script (empty
// module name), and module_allowed_unsafe returns false for empty names.

options gen2
require something_unsafe

// uncomment implicit for failed compilation
def implicit_feint_failed(s : string /*implicit*/) {
    feint(s)
}

[export]
def test {
    implicit_feint("hello, world!\n")
    implicit_feint_failed("this can fail if implicit is uncommented.\n")
    return true
}

[export]
def main {
    test()
}

options log
