options gen2

expect 30243 : 4

// issue #3426: field initializers on a non-composite make-type must not be
// silently discarded by the zero-value constant fold

enum E {
    a
}

typedef V = float4
typedef P = int?
typedef S = string

[export]
def main {
    var d1 = E(a = 1)       // enum has no fields; used to fold to the zero enumerant
    var d2 = V(x = 1.0)     // alias of a vector workhorse
    var d3 = P(a = 1)       // alias of a pointer
    var d4 = S(a = "x")     // alias of a string
}
