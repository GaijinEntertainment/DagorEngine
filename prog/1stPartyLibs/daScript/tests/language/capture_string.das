options gen2
options no_coverage

module foobar

require dastest/testing_boost public

require daslib/ast_boost

[export]
def no_capture(a : string) {
    feint(a)
}

[export]
def capture_by_string_builder() {
    feint("g = {g}")
}

var g = "hello"

[export]
def capture_to_global(a : string) {
    g = a
}

[export]
def capture_by_calling_capture(a : string) {
    capture_to_global(a)
}

[export]
def capture_by_return(a : string) {
    return a
}

[export]
def capture_by_set_arg(var a : string&) {
    a = "world"
}

[export]
def no_capture_by_set_arg(var a : string&) {
    feint(a)
}

[export]
def capture_by_calling_builtin_capture(a : string) {
    for (t in each(a)) {        // via builtin each
        feint("t = {t}")
    }
}

[_macro]
def test() {
    for_each_function(this_module(), "") <| $(fn) {
        if (fn.name == "no_capture") {
            assert(!fn.moreFlags.captureString)
            assert(!fn.moreFlags.callCaptureString)
        } elif (fn.name == "capture_to_global") {
            assert(fn.moreFlags.captureString)
            assert(!fn.moreFlags.callCaptureString)
        } elif (fn.name == "capture_by_calling_capture") {
            assert(fn.moreFlags.captureString)
            assert(fn.moreFlags.callCaptureString)
        } elif (fn.name == "capture_by_return") {
            assert(fn.moreFlags.captureString)
            assert(!fn.moreFlags.callCaptureString)
        } elif (fn.name == "capture_by_set_arg") {
            assert(fn.moreFlags.captureString)
            assert(!fn.moreFlags.callCaptureString)
        } elif (fn.name == "no_capture_by_set_arg") {
            assert(!fn.moreFlags.captureString)
            assert(!fn.moreFlags.callCaptureString)
        } elif (fn.name == "capture_by_calling_builtin_capture") {
            assert(fn.moreFlags.captureString)
            assert(fn.moreFlags.callCaptureString)
        } elif (fn.name == "capture_by_string_builder") {
            assert(fn.moreFlags.captureString)
            assert(!fn.moreFlags.callCaptureString)
            assert(fn.moreFlags.hasStringBuilder)
        } else {
            pass
        }
    }
}

[test]
def test_capture_string(t : T?) {
    pass    // all good, testing is done in macro
}
