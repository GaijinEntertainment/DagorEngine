options gen2
options auto_inline_functions = false   // lint fixtures assert source call/assignment shapes

// LINT016: string clone syntax is a pointer copy unless multiple_contexts is enabled.
// Use ordinary copy/push for same-context storage, and clone_string explicitly when
// a string crosses contexts. Temporary string# sources are the lifetime-safe exception.

expect 50503:13

var bad_global := "global"                       // LINT016

require daslib/lint

def bad_assign(var dst : string; src : string) {
    dst := src                                      // LINT016
}

def bad_redundant_assign_wrapper(var dst : string; src : string) {
    dst := clone_string(src)                        // LINT016
}

def bad_local_init(src : string) {
    var dst := src                                  // LINT016
    print(dst)
}

def bad_redundant_init_wrapper(src : string) {
    var dst := clone_string(src)                    // LINT016
    print(dst)
}

def bad_pipe_push(var dst : array<string>; src : string) {
    dst |> push_clone(src)                          // LINT016
}

def bad_dot_push(var dst : array<string>; src : string) {
    dst.push_clone(src)                             // LINT016
}

def bad_plain_push(var dst : array<string>; src : string) {
    push_clone(dst, src)                            // LINT016
}

def bad_indexed_push(var dst : array<string>; src : string) {
    dst |> push_clone(src, 0)                       // LINT016
}

def bad_redundant_wrapper(var dst : array<string>; src : string) {
    dst |> push_clone(clone_string(src))            // LINT016
}

def good_same_context(var dst : string; var arr : array<string>; src : string) {
    dst = src
    arr |> push(src)
}

def good_cross_context(var dst : string; var arr : array<string>; src : string) {
    dst = clone_string(src)
    arr |> push(clone_string(src))
}

def borrow_string(src : string) : string# {
    unsafe {
        return reinterpret<string#>(src)
    }
}

def bad_redundant_temp_init_wrapper(src : string) {
    let dst := clone_string(borrow_string(src))     // LINT016
    print(dst)
}

def good_temporary(var dst : string; var arr : array<string>; src : string) {
    let retained := borrow_string(src)
    dst := borrow_string(src)
    arr |> push_clone(borrow_string(src))
    print(retained)
}

def good_non_string(var dst : array<int>; value : int) {
    dst |> push_clone(value)
}

def generic_clone(var dst : auto(T); src : T) {
    dst := src
}

def good_generic_instance(var dst : string; src : string) {
    generic_clone(dst, src)
}
