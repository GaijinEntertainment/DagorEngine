// Shapes the fuzzer reaches that must be rejected, one diagnostic each. The expect list is
// an exact multiset: an error that stops being reported fails this file just as loudly as a
// new one, which is what keeps a crash from being traded for a silent accept.
options gen2
expect 30107, 30109, 30110:2, 30192, 30268, 30320, 30805, 30821, 30826, 30832

// A void expression handed to debug().
def v {
}

// An inherited field redeclared as sealed, in a class deriving from a template.
class template T {
}

class e : T {
    f : r<t>
    sealed f : int
}

// typeinfo is_argument outside of a function, in the dim expression of a field.
var g_probe = 1

struct l {
    f : int[typeinfo is_argument(g_probe)]
}

[export]
def main {
    debug(v())
}

// A for-source whose type still carries an unresolved dim: the lambda's argument type never
// reduces to a constant, so the synthesized each() call would hand the argument walk a type
// no rule covers. Reduced from the machine-minimized 701-byte payload.
def f9 : int {
    return 3
}

def for_source_of_expr_type {
    for (x in @@(a : int[f9()]) => 1) {
    }
}

// A function address whose argument type carries a dim that never reduces to a constant: the
// typed lookup must wait for the dim instead of matching against the unresolved type.
enum AddrDim {
    a
}

def address_of_expr_typed_argument {
    var x = @@ < (a : int[AddrDim.a]) > address_of_expr_typed_argument
}

// typeinfo vector_dim of a type that is not a vector.
def vector_dim_of_non_vector {
    let d = typeinfo vector_dim(1)
}
