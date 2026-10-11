options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// PERF032: an @exact_size array grown by resize without explicit capacity
//
// @exact_size on an array declaration (field, global, local, by-ref parameter) declares an
// input-scaled buffer: one that can cross max_unreserved_size in a single grow, where an
// unreserved resize panics. On such an array every resize/resize_no_init must follow a
// reserve or ensure_capacity of the same receiver earlier in the function; sizing it through
// a helper that reserves (reserve_resize and kin) is transparent.

expect 31208:9

require daslib/perf_lint

struct State {
    @exact_size buf : array<float>
    plain : array<float>
    @exact_size not_an_array : int      // PERF032 - not an array
}

var @exact_size g_scale : array<int>
var @exact_size g_not_an_array : float   // PERF032 - not an array

// === Positives ===

def bad_field(var st : State; n : int) {
    st.buf |> resize(n)                 // PERF032
}

def bad_local(n : int) {
    var @exact_size local : array<int>
    local |> resize_no_init(n)          // PERF032
    print("{length(local)}\n")
}

def bad_global(n : int) {
    g_scale |> resize(n)                // PERF032
}

def bad_reserve_after(var st : State; n : int) {
    st.buf |> resize(n)                 // PERF032 - the reserve comes later
    st.buf |> reserve(n)
}

def bad_param(@exact_size var a : array<float>&; n : int) {
    a |> resize(n)                      // PERF032 - annotated parameter
}

def bad_decl_type() {
    var @exact_size x : int = 3         // PERF032 - not an array
    print("{x}\n")
}

def bad_param_type(@exact_size n : int) {   // PERF032 - not an array
    print("{n}\n")
}

// === Negatives ===

def good_reserve_first(var st : State; n : int) {
    st.buf |> reserve(n)
    st.buf |> resize(n)
}

def good_ensure(var st : State; n : int) {
    st.buf |> ensure_capacity(n)
    st.buf |> resize(n)
}

// the scratch one-shots establish capacity too
def good_scratch_reserve(var st : State; n : int) {
    unsafe {
        st.buf |> scratch_reserve(n)
    }
    st.buf |> resize(n)
}

def good_scratch_ensure(var st : State; n : int) {
    unsafe {
        st.buf |> scratch_ensure_capacity(n)
    }
    st.buf |> resize(n)
}

def good_scratch_resize(var st : State; n : int) {
    unsafe {
        st.buf |> scratch_resize(n)
    }
    st.buf |> resize(n)
}

// not annotated: no contract
def good_plain(var st : State; n : int) {
    st.plain |> resize(n)
}

// the reserving helper: its own annotated parameter is reserved before the resize
def grow_exact(@exact_size var a : array<float>&; n : int) {
    a |> reserve(n)
    a |> resize(n)
}

def good_via_helper(var st : State; n : int) {
    grow_exact(st.buf, n)
}

def good_local_reserve(n : int) {
    var @exact_size local : array<int>
    local |> reserve(n)
    local |> resize(n)
    print("{length(local)}\n")
}

[export]
def main() {
    var st = State()
    bad_field(st, 3)
    bad_local(3)
    bad_global(3)
    bad_reserve_after(st, 3)
    bad_param(st.buf, 3)
    bad_decl_type()
    bad_param_type(1)
    good_scratch_reserve(st, 3)
    good_scratch_ensure(st, 3)
    good_scratch_resize(st, 3)
    print("{g_not_an_array}\n")
    good_reserve_first(st, 3)
    good_ensure(st, 3)
    good_plain(st, 3)
    good_via_helper(st, 3)
    good_local_reserve(3)
}
