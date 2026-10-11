options gen2
options indenting = 4

require daslib/flatten
require math

//! `sincos(x, s, c)` is whitelisted and writes its results through `float&` out-parameters.
//! The lowering owns no predicated form for a store it did not emit, so the call must be
//! refused: dropping it silently returned the pre-call values, and the invisible store then
//! let CSE share a subexpression straight across the mutation.

[flatten]
def withrefarg(x, y : float) : float {
    var s = x * 2.0
    var c = y * 3.0
    sincos(x, s, c)
    return s + c
}

[export]
def main {
    print("{withrefarg(0.7, 0.3)}\n")
}
