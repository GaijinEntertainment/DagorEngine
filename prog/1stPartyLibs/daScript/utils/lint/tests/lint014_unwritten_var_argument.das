options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT014: mutable (var) function argument is never written
//
// A by-ref parameter declared `var` whose body never writes it forces every
// caller's argument to be mutable for nothing (the caller side of issue #3090).
// Drop the var; callers may then declare their arguments with `let`.
//
// Cascade: a var param that is itself passed onward to a mutable slot is
// skipped — the leaf callee gets flagged first; fixing it re-exposes this one.

expect 50503:5

require daslib/lint

struct private Box {
    value : int
}

// === Positives — var param never written ===

def bad_struct(var b : Box) : int {   // LINT014
    return b.value
}

def bad_fixed_array(var a : float[4][4]) : string {   // LINT014
    return "{a[0][0]}"
}

def bad_explicit_ref(var n : int&) : int {   // LINT014
    return n + 1
}

struct private PtrBox {
    p : int?
}

// pointer field flowing only into a void? slot — void? accepts a const pointer,
// so nothing demands b's mutability (by-value pointer params themselves are
// exempt; the sink stamp peels field access back to the root struct param)
def bad_field_void_sink(var b : PtrBox) {   // LINT014
    var v : void?
    v = b.p
    print("{v != null}")
}

// pointer field returned through a const-pointer result — `int? const` accepts
// a const pointer, so nothing demands b's mutability
def bad_field_const_ptr_return(var b : PtrBox) : int? const {   // LINT014
    return b.p
}

// === Negatives ===

// writes its param — var is earned
def good_writes(var b : Box) : int {
    b.value = 5
    return b.value
}

// passes the param to a writing callee — access_ref propagates, no warning
def good_pass_to_writer(var b : Box) : int {
    return good_writes(b)
}

// passes the param to a mutable slot of a NON-writing callee — flag the leaf
// (bad_struct above), not this transitive demander
def good_cascade(var b : Box) : int {
    return bad_struct(b)
}

// workhorse by-value var is a mutable local copy, not a caller burden
def good_by_value(var n : int) : int {
    return n + 1
}

// finalize signature is dictated by the delete protocol
def finalize(var b : Box) {
    print("finalizing {b.value}\n")
}

// underscore prefix means leave-it-alone
def good_underscore(var _b : Box) : int {
    return _b.value
}

struct private Node {
    next : int?
}

// pointer field stored into a mutable-pointee field — without var the field reads
// as `int? const` and the store trips error 30915, so var is earned
def good_field_mutable_sink(var b : PtrBox) : Node {
    return Node(next = b.p)
}
