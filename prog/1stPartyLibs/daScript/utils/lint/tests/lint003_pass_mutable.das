options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT003: pass-mutable suppression (issue #3090)
//
// A var whose only "mutable" use is being passed to a mutable-ref parameter
// of a callee that never writes it must NOT be suggested as const — a `let`
// argument no longer matches the `var` parameter and the build breaks.
//
// Bad pattern (the old false positive):
//   var mv : float[4][4]
//   print(probe(mv))         // LINT003 suggested `let mv` — a no-match compile error

expect 50503:2

require daslib/lint

struct private Holder {
    value : int
}

// never write their var params — these probe the caller-side suppression;
// the callee-side rule has its own fixture (lint014_unwritten_var_argument.das)
def private probe(var a : float[4][4]) : string {   // nolint:LINT014
    return "{a[0][0]}"
}

def private probe_struct(var s : Holder) : int {   // nolint:LINT014
    return s.value
}

// === Negatives — no LINT003: the signature demands mutability ===

def no_warn_fixed_array() {
    var mv : float[4][4]
    print(probe(mv))
}

def no_warn_struct() {
    var h = Holder(value = 1)
    print("{probe_struct(h)}")
}

def no_warn_element_peel() {
    var hs : Holder[2]
    print("{probe_struct(hs[0])}")
}

// note: the pointer-sink side of the suppression (a pointer field flowing into a
// mutable-pointee slot demands the root variable stay var) is unobservable here —
// LINT003's arms gate on canCloneFromConst, which is already false for any
// pointer-carrying type. lint014_unwritten_var_argument.das pins those shapes

// === Positives — LINT003 still fires when nothing demands mutability ===

def warn_simple() : int {
    var x = 13      // LINT003
    return x
}

def warn_struct_read_only() : int {
    var h = Holder(value = 2)   // LINT003
    return h.value
}

