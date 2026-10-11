options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT023: mutable (var) by-value argument is written but never read
//
// A by-value parameter is a local copy, so a write the body never reads back
// is unobservable - the classic out-parameter that never reaches its caller
// (`var why : string` assigned on the error path, `why` empty at the call site).
// Declare it `T&` for an out-parameter, otherwise the write is dead.
//
// LINT014's mirror: that rule catches a `var` nothing writes; this one catches
// a `var` nothing reads. Only plain stores keep a candidate alive - `=`, `:=`,
// `<-`, compound ops, a swizzle store, a statement-level ++/--. Any other
// appearance (a read, `return`, passing on, addr-of, capture, a write through
// a pointer's pointee) is a use.

expect 50503:10

require daslib/lint

// === Positives ===

def bad_out_param(url : string; var why : string) : int {   // LINT023
    if (empty(url)) {
        why = "empty url"
        return 0
    }
    return 1
}

def bad_accumulate(var n : int) {   // LINT023
    n += 1
}

def bad_swizzle(var v : float3) {   // LINT023
    v.x = 1.0
}

def bad_incr(var n : int) {   // LINT023
    n++
}

def bad_store_in_block(var n : int) {   // LINT023 - a closure block writes the same local copy
    for (x in [1, 2]) {
        invoke($() {
            n = x
        })
    }
}

def bad_two(var a : int; var b : float) {   // LINT023 on a only - b is read
    a = 1
    b = 2.0
    print("{b}")
}

def bad_swizzle_opassign(var v : float3) {   // LINT023 - compound store through a swizzle
    v.x += 1.0
}

def bad_move_lambda(var f : lambda<void>) {   // LINT023 - a move into the by-value copy
    f <- @() {
        print("x\n")
    }
}

struct private Foo {
    x : int
}

def bad_ptr_reassign(var f : Foo?) {   // LINT023 - the pointer itself is a by-value copy
    f = null
}

def bad_read_before_store(var tex : uint) {   // LINT023 - the read precedes the store, so nothing reads it back
    if (tex != 0u) {
        print("{tex}\n")
        tex = 0u
    }
}

// === Negatives ===

def good_goto_backwards(var n : int; times : int) {
    var k = 0
    label 1:
    print("{n}\n")
    n = k
    k++
    if (k < times) {
        goto label 1
    }
}

def good_read_in_block(var n : int) {
    n = 1
    invoke($() {
        print("{n}\n")
    })
}

def good_read_in_loop(var n : int) {
    for (i in range(2)) {
        print("{n}\n")
        n = i
    }
}

def good_write_then_read(var n : int) : int {
    n = n * 2
    return n
}

def good_ref(url : string; var why : string&) : int {
    if (empty(url)) {
        why = "empty url"
        return 0
    }
    return 1
}

def bump(var x : int&) {
    x++
}

// passed to a mutable-ref slot: the callee may read it before writing
def good_pass_mutable_ref(var n : int) {
    bump(n)
}

def good_addr(var n : int) {
    n = 5
    let p = unsafe(addr(n))
    print("{p != null}")
}

// captured by a lambda: the capture site reads the copy
def good_captured(var n : int) {
    n = 5
    let lam <- @ capture(copy(n)) () {
        print("{n}")
    }
    invoke(lam)
}

def good_incr_used(var n : int) : int {
    let y = n++
    return y
}

// a write through a pointer param reaches the caller's object - a use, not a store
def good_ptr(var f : Foo?) {
    f.x = 1
}

// by-ref params propagate their writes (LINT014 territory)
def good_struct(var f : Foo) {
    f.x = 1
}

// leave-it-alone name and [unused_argument]
def good_underscore(var _n : int) {
    _n = 1
}

[unused_argument(n)]
def good_marked(var n : int) {
    n = 1
}

[export]
def main() {
    var why = ""
    print("{bad_out_param("", why)} {why}\n")
    bad_accumulate(1)
    bad_swizzle(float3(0.0))
    bad_incr(1)
    bad_store_in_block(1)
    bad_two(1, 2.0)
    bad_swizzle_opassign(float3(0.0))
    let lam <- @() {
        print("y\n")
    }
    bad_move_lambda(lam)
    bad_ptr_reassign(null)
    print("{good_write_then_read(3)} {good_ref("", why)}\n")
    good_pass_mutable_ref(1)
    good_addr(1)
    good_captured(1)
    print("{good_incr_used(1)}\n")
    var f = new Foo()
    good_ptr(f)
    unsafe { delete f; }
    var s = Foo()
    good_struct(s)
    good_underscore(1)
    good_marked(1)
    drop_handle_probe()
}

def private drop_handle_probe() {
    bad_read_before_store(7u)
    good_read_in_block(1)
    good_read_in_loop(1)
    good_goto_backwards(1, 3)
}
